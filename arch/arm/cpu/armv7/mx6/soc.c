/*
 * (C) Copyright 2007
 * Sascha Hauer, Pengutronix
 *
 * (C) Copyright 2009-2014 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/armv7.h>
#include <asm/pl310.h>
#include <asm/errno.h>
#include <asm/io.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/clock.h>
#include <asm/arch/sys_proto.h>
#include <asm/imx-common/boot_mode.h>
#include <asm/imx-common/dma.h>
#include <libfdt.h>
#include <stdbool.h>
#include <asm/arch/mxc_hdmi.h>
#include <asm/arch/crm_regs.h>
#ifdef CONFIG_IMX_UDC
#include <asm/arch/mx6_usbphy.h>
#include <usb/imx_udc.h>
#endif

enum ldo_reg {
	LDO_ARM,
	LDO_SOC,
	LDO_PU,
};

struct scu_regs {
	u32	ctrl;
	u32	config;
	u32	status;
	u32	invalidate;
	u32	fpga_rev;
};

#define TEMPERATURE_MIN		-40
#define TEMPERATURE_HOT		80
#define TEMPERATURE_MAX		125
#define FACTOR1			15976
#define FACTOR2			4297157
#define MEASURE_FREQ		327

#define REG_VALUE_TO_CEL(ratio, raw) \
	((raw_n40c - raw) * 100 / ratio - 40)

static unsigned int fuse = ~0;

u32 get_cpu_rev(void)
{
	struct anatop_regs *anatop = (struct anatop_regs *)ANATOP_BASE_ADDR;
	u32 reg = readl(&anatop->digprog_sololite);
	u32 type = ((reg >> 16) & 0xff);

	if (type != MXC_CPU_MX6SL) {
		reg = readl(&anatop->digprog);
		struct scu_regs *scu = (struct scu_regs *)SCU_BASE_ADDR;
		u32 cfg = readl(&scu->config) & 3;
		type = ((reg >> 16) & 0xff);
		if (type == MXC_CPU_MX6DL) {
			if (!cfg)
				type = MXC_CPU_MX6SOLO;
		}

		if (type == MXC_CPU_MX6Q) {
			if (cfg == 1)
				type = MXC_CPU_MX6D;
		}

	}
	reg &= 0xff;		/* mx6 silicon revision */
	return (type << 12) | (reg + 0x10);
}

#ifdef CONFIG_REVISION_TAG
u32 __weak get_board_rev(void)
{
	u32 cpurev = get_cpu_rev();
	u32 type = ((cpurev >> 12) & 0xff);
	if (type == MXC_CPU_MX6SOLO)
		cpurev = (MXC_CPU_MX6DL) << 12 | (cpurev & 0xFFF);

	if (type == MXC_CPU_MX6D)
		cpurev = (MXC_CPU_MX6Q) << 12 | (cpurev & 0xFFF);

	return cpurev;
}
#endif

void init_aips(void)
{
	struct aipstz_regs *aips1, *aips2;

	aips1 = (struct aipstz_regs *)AIPS1_BASE_ADDR;
	aips2 = (struct aipstz_regs *)AIPS2_BASE_ADDR;

	/*
	 * Set all MPROTx to be non-bufferable, trusted for R/W,
	 * not forced to user-mode.
	 */
	writel(0x77777777, &aips1->mprot0);
	writel(0x77777777, &aips1->mprot1);
	writel(0x77777777, &aips2->mprot0);
	writel(0x77777777, &aips2->mprot1);

	/*
	 * Set all OPACRx to be non-bufferable, not require
	 * supervisor privilege level for access,allow for
	 * write access and untrusted master access.
	 */
	writel(0x00000000, &aips1->opacr0);
	writel(0x00000000, &aips1->opacr1);
	writel(0x00000000, &aips1->opacr2);
	writel(0x00000000, &aips1->opacr3);
	writel(0x00000000, &aips1->opacr4);
	writel(0x00000000, &aips2->opacr0);
	writel(0x00000000, &aips2->opacr1);
	writel(0x00000000, &aips2->opacr2);
	writel(0x00000000, &aips2->opacr3);
	writel(0x00000000, &aips2->opacr4);
}

static void clear_ldo_ramp(void)
{
	struct anatop_regs *anatop = (struct anatop_regs *)ANATOP_BASE_ADDR;
	int reg;

	/* ROM may modify LDO ramp up time according to fuse setting, so in
	 * order to be in the safe side we neeed to reset these settings to
	 * match the reset value: 0'b00
	 */
	reg = readl(&anatop->ana_misc2);
	reg &= ~(0x3f << 24);
	writel(reg, &anatop->ana_misc2);
}

/*
 * Set the VDDSOC
 *
 * Mask out the REG_CORE[22:18] bits (REG2_TRIG) and set
 * them to the specified millivolt level.
 * Possible values are from 0.725V to 1.450V in steps of
 * 0.025V (25mV).
 */
static int set_ldo_voltage(enum ldo_reg ldo, u32 mv)
{
	struct anatop_regs *anatop = (struct anatop_regs *)ANATOP_BASE_ADDR;
	u32 val, step, old, reg = readl(&anatop->reg_core);
	u8 shift;

	if (mv < 725)
		val = 0x00;	/* Power gated off */
	else if (mv > 1450)
		val = 0x1F;	/* Power FET switched full on. No regulation */
	else
		val = (mv - 700) / 25;

	clear_ldo_ramp();

	switch (ldo) {
	case LDO_SOC:
		shift = 18;
		break;
	case LDO_PU:
		shift = 9;
		break;
	case LDO_ARM:
		shift = 0;
		break;
	default:
		return -EINVAL;
	}

	old = (reg & (0x1F << shift)) >> shift;
	step = abs(val - old);
	if (step == 0)
		return 0;

	reg = (reg & ~(0x1F << shift)) | (val << shift);
	writel(reg, &anatop->reg_core);

	/*
	 * The LDO ramp-up is based on 64 clock cycles of 24 MHz = 2.6 us per
	 * step
	 */
	udelay(3 * step);

	return 0;
}

static void imx_set_wdog_powerdown(bool enable)
{
	struct wdog_regs *wdog1 = (struct wdog_regs *)WDOG1_BASE_ADDR;
	struct wdog_regs *wdog2 = (struct wdog_regs *)WDOG2_BASE_ADDR;

	/* Write to the PDE (Power Down Enable) bit */
	writew(enable, &wdog1->wmcr);
	writew(enable, &wdog2->wmcr);
}

static int read_cpu_temperature(void)
{
	int temperature;
	unsigned int ccm_ccgr2;
	unsigned int reg, tmp;
	unsigned int raw_25c, raw_n40c, ratio;
	struct anatop_regs *anatop = (struct anatop_regs *)ANATOP_BASE_ADDR;
	struct mxc_ccm_reg *mxc_ccm = (struct mxc_ccm_reg *)CCM_BASE_ADDR;
	struct ocotp_regs *ocotp = (struct ocotp_regs *)OCOTP_BASE_ADDR;
	struct fuse_bank *bank = &ocotp->bank[1];
	struct fuse_bank1_regs *fuse_bank1 =
			(struct fuse_bank1_regs *)bank->fuse_regs;

	/* need to make sure pll3 is enabled for thermal sensor */
	if ((readl(&anatop->usb1_pll_480_ctrl) &
			BM_ANADIG_USB1_PLL_480_CTRL_LOCK) == 0) {
		/* enable pll's power */
		writel(BM_ANADIG_USB1_PLL_480_CTRL_POWER,
				&anatop->usb1_pll_480_ctrl_set);
		writel(0x80, &anatop->ana_misc2_clr);
		/* wait for pll lock */
		while ((readl(&anatop->usb1_pll_480_ctrl) &
			BM_ANADIG_USB1_PLL_480_CTRL_LOCK) == 0)
			;
		/* disable bypass */
		writel(BM_ANADIG_USB1_PLL_480_CTRL_BYPASS,
				&anatop->usb1_pll_480_ctrl_clr);
		/* enable pll output */
		writel(BM_ANADIG_USB1_PLL_480_CTRL_ENABLE,
				&anatop->usb1_pll_480_ctrl_set);
	}

	ccm_ccgr2 = readl(&mxc_ccm->CCGR2);
	/* enable OCOTP_CTRL clock in CCGR2 */
	writel(ccm_ccgr2 | MXC_CCM_CCGR2_OCOTP_CTRL_MASK, &mxc_ccm->CCGR2);
	fuse = readl(&fuse_bank1->ana1);

	/* restore CCGR2 */
	writel(ccm_ccgr2, &mxc_ccm->CCGR2);

	if (fuse == 0 || fuse == 0xffffffff || (fuse & 0xfff00000) == 0)
		return TEMPERATURE_MIN;

	/*
	 * fuse data layout:
	 * [31:20] sensor value @ 25C
	 * [19:8] sensor value of hot
	 * [7:0] hot temperature value
	 */
	raw_25c = fuse >> 20;

	/*
	 * The universal equation for thermal sensor
	 * is slope = 0.4297157 - (0.0015976 * 25C fuse),
	 * here we convert them to integer to make them
	 * easy for counting, FACTOR1 is 15976,
	 * FACTOR2 is 4297157. Our ratio = -100 * slope
	 */
	ratio = ((FACTOR1 * raw_25c - FACTOR2) + 50000) / 100000;

	debug("Thermal sensor with ratio = %d\n", ratio);

	raw_n40c = raw_25c + (13 * ratio) / 20;

	/*
	 * now we only use single measure, every time we read
	 * the temperature, we will power on/down anadig thermal
	 * module
	 */
	writel(BM_ANADIG_TEMPSENSE0_POWER_DOWN, &anatop->tempsense0_clr);
	writel(BM_ANADIG_ANA_MISC0_REFTOP_SELBIASOFF, &anatop->ana_misc0_set);

	/* write measure freq */
	reg = readl(&anatop->tempsense1);
	reg &= ~BM_ANADIG_TEMPSENSE1_MEASURE_FREQ;
	reg |= MEASURE_FREQ;
	writel(reg, &anatop->tempsense1);

	writel(BM_ANADIG_TEMPSENSE0_MEASURE_TEMP, &anatop->tempsense0_clr);
	writel(BM_ANADIG_TEMPSENSE0_FINISHED, &anatop->tempsense0_clr);
	writel(BM_ANADIG_TEMPSENSE0_MEASURE_TEMP, &anatop->tempsense0_set);

	while ((readl(&anatop->tempsense0) &
			BM_ANADIG_TEMPSENSE0_FINISHED) == 0)
		udelay(10000);

	reg = readl(&anatop->tempsense0);
	tmp = (reg & BM_ANADIG_TEMPSENSE0_TEMP_VALUE)
		>> BP_ANADIG_TEMPSENSE0_TEMP_VALUE;
	writel(BM_ANADIG_TEMPSENSE0_FINISHED, &anatop->tempsense0_clr);

	if (tmp <= raw_n40c)
		temperature = REG_VALUE_TO_CEL(ratio, tmp);
	else
		temperature = TEMPERATURE_MIN;
	/* power down anatop thermal sensor */
	writel(BM_ANADIG_TEMPSENSE0_POWER_DOWN, &anatop->tempsense0_set);
	writel(BM_ANADIG_ANA_MISC0_REFTOP_SELBIASOFF, &anatop->ana_misc0_clr);

	return temperature;
}

void check_cpu_temperature(void)
{
	int cpu_tmp = 0;

	cpu_tmp = read_cpu_temperature();
	while (cpu_tmp > TEMPERATURE_MIN && cpu_tmp < TEMPERATURE_MAX) {
		if (cpu_tmp >= TEMPERATURE_HOT) {
			printf("CPU is %d C, too hot to boot, waiting...\n",
				cpu_tmp);
			udelay(5000000);
			cpu_tmp = read_cpu_temperature();
		} else
			break;
	}
	if (cpu_tmp > TEMPERATURE_MIN && cpu_tmp < TEMPERATURE_MAX)
		printf("CPU:   Temperature %d C, calibration data: 0x%x\n",
			cpu_tmp, fuse);
	else
		printf("CPU:   Temperature: can't get valid data!\n");
}

static void set_ahb_rate(u32 val)
{
	struct mxc_ccm_reg *mxc_ccm = (struct mxc_ccm_reg *)CCM_BASE_ADDR;
	u32 reg, div;

	div = get_periph_clk() / val - 1;
	reg = readl(&mxc_ccm->cbcdr);

	writel((reg & (~MXC_CCM_CBCDR_AHB_PODF_MASK)) |
		(div << MXC_CCM_CBCDR_AHB_PODF_OFFSET), &mxc_ccm->cbcdr);
}

static void clear_mmdc_ch_mask(void)
{
	struct mxc_ccm_reg *mxc_ccm = (struct mxc_ccm_reg *)CCM_BASE_ADDR;

	/* Clear MMDC channel mask */
	writel(0, &mxc_ccm->ccdr);
}

int arch_cpu_init(void)
{
	init_aips();

	/* Need to clear MMDC_CHx_MASK to make warm reset work. */
	clear_mmdc_ch_mask();

	/*
	 * When low freq boot is enabled, ROM will not set AHB
	 * freq, so we need to ensure AHB freq is 132MHz in such
	 * scenario.
	 */
	if (mxc_get_clock(MXC_ARM_CLK) == 396000000)
		set_ahb_rate(132000000);

	imx_set_wdog_powerdown(false); /* Disable PDE bit of WMCR register */

#ifdef CONFIG_APBH_DMA
	/* Start APBH DMA */
	mxs_dma_init();
#endif

	return 0;
}

int board_postclk_init(void)
{
	set_ldo_voltage(LDO_SOC, 1175);	/* Set VDDSOC to 1.175V */

	return 0;
}

#ifndef CONFIG_SYS_DCACHE_OFF
void enable_caches(void)
{
#if defined(CONFIG_SYS_ARM_CACHE_WRITETHROUGH)
	enum dcache_option option = DCACHE_WRITETHROUGH;
#else
	enum dcache_option option = DCACHE_WRITEBACK;
#endif

	/* Avoid random hang when download by usb */
	invalidate_dcache_all();

	/* Enable D-cache. I-cache is already enabled in start.S */
	dcache_enable();

	/* Enable caching on OCRAM and ROM */
	mmu_set_region_dcache_behaviour(ROMCP_ARB_BASE_ADDR,
					ROMCP_ARB_END_ADDR,
					option);
	mmu_set_region_dcache_behaviour(IRAM_BASE_ADDR,
					IRAM_SIZE,
					option);
}
#endif

#if defined(CONFIG_FEC_MXC)
void imx_get_mac_from_fuse(int dev_id, unsigned char *mac)
{
	struct ocotp_regs *ocotp = (struct ocotp_regs *)OCOTP_BASE_ADDR;
	struct fuse_bank *bank = &ocotp->bank[4];
	struct fuse_bank4_regs *fuse =
			(struct fuse_bank4_regs *)bank->fuse_regs;

	u32 value = readl(&fuse->mac_addr_high);
	mac[0] = (value >> 8);
	mac[1] = value ;

	value = readl(&fuse->mac_addr_low);
	mac[2] = value >> 24 ;
	mac[3] = value >> 16 ;
	mac[4] = value >> 8 ;
	mac[5] = value ;

}
#endif

void boot_mode_apply(unsigned cfg_val)
{
	unsigned reg;
	struct src *psrc = (struct src *)SRC_BASE_ADDR;
	writel(cfg_val, &psrc->gpr9);
	reg = readl(&psrc->gpr10);
	if (cfg_val)
		reg |= 1 << 28;
	else
		reg &= ~(1 << 28);
	writel(reg, &psrc->gpr10);
}
/*
 * cfg_val will be used for
 * Boot_cfg4[7:0]:Boot_cfg3[7:0]:Boot_cfg2[7:0]:Boot_cfg1[7:0]
 * After reset, if GPR10[28] is 1, ROM will copy GPR9[25:0]
 * to SBMR1, which will determine the boot device.
 */
const struct boot_mode soc_boot_modes[] = {
	{"normal",	MAKE_CFGVAL(0x00, 0x00, 0x00, 0x00)},
	/* reserved value should start rom usb */
	{"usb",		MAKE_CFGVAL(0x01, 0x00, 0x00, 0x00)},
	{"sata",	MAKE_CFGVAL(0x20, 0x00, 0x00, 0x00)},
	{"escpi1:0",	MAKE_CFGVAL(0x30, 0x00, 0x00, 0x08)},
	{"escpi1:1",	MAKE_CFGVAL(0x30, 0x00, 0x00, 0x18)},
	{"escpi1:2",	MAKE_CFGVAL(0x30, 0x00, 0x00, 0x28)},
	{"escpi1:3",	MAKE_CFGVAL(0x30, 0x00, 0x00, 0x38)},
	/* 4 bit bus width */
	{"esdhc1",	MAKE_CFGVAL(0x40, 0x20, 0x00, 0x00)},
	{"esdhc2",	MAKE_CFGVAL(0x40, 0x28, 0x00, 0x00)},
	{"esdhc3",	MAKE_CFGVAL(0x40, 0x30, 0x00, 0x00)},
	{"esdhc4",	MAKE_CFGVAL(0x40, 0x38, 0x00, 0x00)},
	{NULL,		0},
};

void s_init(void)
{
	struct anatop_regs *anatop = (struct anatop_regs *)ANATOP_BASE_ADDR;
	int is_6q = is_cpu_type(MXC_CPU_MX6Q);
	u32 mask480;
	u32 mask528;

	/* Due to hardware limitation, on MX6Q we need to gate/ungate all PFDs
	 * to make sure PFD is working right, otherwise, PFDs may
	 * not output clock after reset, MX6DL and MX6SL have added 396M pfd
	 * workaround in ROM code, as bus clock need it
	 */

	mask480 = ANATOP_PFD_CLKGATE_MASK(0) |
		ANATOP_PFD_CLKGATE_MASK(1) |
		ANATOP_PFD_CLKGATE_MASK(2) |
		ANATOP_PFD_CLKGATE_MASK(3);
	mask528 = ANATOP_PFD_CLKGATE_MASK(0) |
		ANATOP_PFD_CLKGATE_MASK(1) |
		ANATOP_PFD_CLKGATE_MASK(3);

	/*
	 * Don't reset PFD2 on DL/S
	 */
	if (is_6q)
		mask528 |= ANATOP_PFD_CLKGATE_MASK(2);
	writel(mask480, &anatop->pfd_480_set);
	writel(mask528, &anatop->pfd_528_set);
	writel(mask480, &anatop->pfd_480_clr);
	writel(mask528, &anatop->pfd_528_clr);
}

#ifdef CONFIG_LDO_BYPASS_CHECK
DECLARE_GLOBAL_DATA_PTR;
static int ldo_bypass;

int check_ldo_bypass(void)
{
	const int *ldo_mode;
	int node;

	/* get the right fdt_blob from the global working_fdt */
	gd->fdt_blob = working_fdt;
	/* Get the node from FDT for anatop ldo-bypass */
	node = fdt_node_offset_by_compatible(gd->fdt_blob, -1,
		"fsl,imx6q-gpc");
	if (node < 0) {
		printf("No gpc device node %d, force to ldo-enable.\n", node);
		return 0;
	}
	ldo_mode = fdt_getprop(gd->fdt_blob, node, "fsl,ldo-bypass", NULL);
	/*
	 * return 1 if "fsl,ldo-bypass = <1>", else return 0 if
	 * "fsl,ldo-bypass = <0>" or no "fsl,ldo-bypass" property
	 */
	ldo_bypass = fdt32_to_cpu(*ldo_mode) == 1 ? 1 : 0;

	return ldo_bypass;
}

int check_1_2G(void)
{
	u32 reg;
	int result = 0;
	struct ocotp_regs *ocotp = (struct ocotp_regs *)OCOTP_BASE_ADDR;
	struct fuse_bank *bank = &ocotp->bank[0];
	struct fuse_bank0_regs *fuse_bank0 =
			(struct fuse_bank0_regs *)bank->fuse_regs;

	reg = readl(&fuse_bank0->cfg3);
	if (((reg >> 16) & 0x3) == 0x3) {
		if (ldo_bypass) {
			printf("Wrong dtb file used! i.MX6Q@1.2Ghz only "
				"works with ldo-enable mode!\n");
			/*
			 * Currently, only imx6q-sabresd board might be here,
			 * since only i.MX6Q support 1.2G and only Sabresd board
			 * support ldo-bypass mode. So hardcode here.
			 * You can also modify your board(i.MX6Q) dtb name if it
			 * supports both ldo-bypass and ldo-enable mode.
			 */
			printf("Please use imx6q-sabresd-ldo.dtb!\n");
			hang();
		}
		result = 1;
	}

	return result;
}

void set_anatop_bypass(void)
{
	struct anatop_regs *anatop = (struct anatop_regs *)ANATOP_BASE_ADDR;
	u32 reg = readl(&anatop->reg_core);

	/* bypass VDDARM/VDDSOC */
	reg = reg | (0x1F << 18) | 0x1F;
	writel(reg, &anatop->reg_core);
}
#endif

#ifdef CONFIG_IMX_HDMI
void imx_enable_hdmi_phy(void)
{
	struct hdmi_regs *hdmi = (struct hdmi_regs *)HDMI_ARB_BASE_ADDR;
	u8 reg;
	reg = readb(&hdmi->phy_conf0);
	reg |= HDMI_PHY_CONF0_PDZ_MASK;
	writeb(reg, &hdmi->phy_conf0);
	udelay(3000);
	reg |= HDMI_PHY_CONF0_ENTMDS_MASK;
	writeb(reg, &hdmi->phy_conf0);
	udelay(3000);
	reg |= HDMI_PHY_CONF0_GEN2_TXPWRON_MASK;
	writeb(reg, &hdmi->phy_conf0);
	writeb(HDMI_MC_PHYRSTZ_ASSERT, &hdmi->mc_phyrstz);
}

void imx_setup_hdmi(void)
{
	struct mxc_ccm_reg *mxc_ccm = (struct mxc_ccm_reg *)CCM_BASE_ADDR;
	struct hdmi_regs *hdmi  = (struct hdmi_regs *)HDMI_ARB_BASE_ADDR;
	int reg;

	/* Turn on HDMI PHY clock */
	reg = readl(&mxc_ccm->CCGR2);
	reg |=  MXC_CCM_CCGR2_HDMI_TX_IAHBCLK_MASK|
		 MXC_CCM_CCGR2_HDMI_TX_ISFRCLK_MASK;
	writel(reg, &mxc_ccm->CCGR2);
	writeb(HDMI_MC_PHYRSTZ_DEASSERT, &hdmi->mc_phyrstz);
	reg = readl(&mxc_ccm->chsccdr);
	reg &= ~(MXC_CCM_CHSCCDR_IPU1_DI0_PRE_CLK_SEL_MASK|
		 MXC_CCM_CHSCCDR_IPU1_DI0_PODF_MASK|
		 MXC_CCM_CHSCCDR_IPU1_DI0_CLK_SEL_MASK);
	reg |= (CHSCCDR_PODF_DIVIDE_BY_3
		 << MXC_CCM_CHSCCDR_IPU1_DI0_PODF_OFFSET)
		 |(CHSCCDR_IPU_PRE_CLK_540M_PFD
		 << MXC_CCM_CHSCCDR_IPU1_DI0_PRE_CLK_SEL_OFFSET);
	writel(reg, &mxc_ccm->chsccdr);
}
#endif

#ifndef CONFIG_SYS_L2CACHE_OFF
#define IOMUXC_GPR11_L2CACHE_AS_OCRAM 0x00000002
void v7_outer_cache_enable(void)
{
	struct pl310_regs *const pl310 = (struct pl310_regs *)L2_PL310_BASE;
	unsigned int val;

#if defined CONFIG_MX6SL
	struct iomuxc *iomux = (struct iomuxc *)IOMUXC_BASE_ADDR;
	val = readl(&iomux->gpr[11]);
	if (val & IOMUXC_GPR11_L2CACHE_AS_OCRAM) {
		/* L2 cache configured as OCRAM, reset it */
		val &= ~IOMUXC_GPR11_L2CACHE_AS_OCRAM;
		writel(val, &iomux->gpr[11]);
	}
#endif

	writel(0x132, &pl310->pl310_tag_latency_ctrl);
	writel(0x132, &pl310->pl310_data_latency_ctrl);

	val = readl(&pl310->pl310_prefetch_ctrl);

	/* Turn on the L2 I/D prefetch */
	val |= 0x30000000;

	/*
	 * The L2 cache controller(PL310) version on the i.MX6D/Q is r3p1-50rel0
	 * The L2 cache controller(PL310) version on the i.MX6DL/SOLO/SL is r3p2
	 * But according to ARM PL310 errata: 752271
	 * ID: 752271: Double linefill feature can cause data corruption
	 * Fault Status: Present in: r3p0, r3p1, r3p1-50rel0. Fixed in r3p2
	 * Workaround: The only workaround to this erratum is to disable the
	 * double linefill feature. This is the default behavior.
	 */

#ifndef CONFIG_MX6Q
	val |= 0x40800000;
#endif
	writel(val, &pl310->pl310_prefetch_ctrl);

	val = readl(&pl310->pl310_power_ctrl);
	val |= L2X0_DYNAMIC_CLK_GATING_EN;
	val |= L2X0_STNDBY_MODE_EN;
	writel(val, &pl310->pl310_power_ctrl);

	setbits_le32(&pl310->pl310_ctrl, L2X0_CTRL_EN);
}

void v7_outer_cache_disable(void)
{
	struct pl310_regs *const pl310 = (struct pl310_regs *)L2_PL310_BASE;

	clrbits_le32(&pl310->pl310_ctrl, L2X0_CTRL_EN);
}
#endif /* !CONFIG_SYS_L2CACHE_OFF */

#ifdef CONFIG_IMX_UDC
void set_usboh3_clk(void)
{
	udc_pins_setting();
}

void set_usb_phy1_clk(void)
{
	/* make sure pll3 is enable here */
	writel((BM_ANADIG_USB1_CHRG_DETECT_EN_B |
		BM_ANADIG_USB1_CHRG_DETECT_CHK_CHRG_B),
		ANATOP_BASE_ADDR + HW_ANADIG_USB1_CHRG_DETECT_SET);

	writel(BM_ANADIG_USB1_PLL_480_CTRL_EN_USB_CLKS,
		ANATOP_BASE_ADDR + HW_ANADIG_USB1_PLL_480_CTRL_SET);
}
void enable_usb_phy1_clk(unsigned char enable)
{
	if (enable)
		writel(BM_USBPHY_CTRL_CLKGATE,
			USB_PHY0_BASE_ADDR + HW_USBPHY_CTRL_CLR);
	else
		writel(BM_USBPHY_CTRL_CLKGATE,
			USB_PHY0_BASE_ADDR + HW_USBPHY_CTRL_SET);
}

void reset_usb_phy1(void)
{
	/* Reset USBPHY module */
	u32 temp;
	temp = readl(USB_PHY0_BASE_ADDR + HW_USBPHY_CTRL);
	temp |= BM_USBPHY_CTRL_SFTRST;
	writel(temp, USB_PHY0_BASE_ADDR + HW_USBPHY_CTRL);
	udelay(10);

	/* Remove CLKGATE and SFTRST */
	temp = readl(USB_PHY0_BASE_ADDR + HW_USBPHY_CTRL);
	temp &= ~(BM_USBPHY_CTRL_CLKGATE | BM_USBPHY_CTRL_SFTRST);
	writel(temp, USB_PHY0_BASE_ADDR + HW_USBPHY_CTRL);
	udelay(10);

	/* Power up the PHY */
	writel(0, USB_PHY0_BASE_ADDR + HW_USBPHY_PWD);
}
#endif
