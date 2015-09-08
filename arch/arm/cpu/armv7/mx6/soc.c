/*
 * (C) Copyright 2007
 * Sascha Hauer, Pengutronix
 *
 * (C) Copyright 2009-2015 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/armv7.h>
#include <asm/bootm.h>
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
#include <dm.h>
#include <imx_thermal.h>
#include <mxsfb.h>
#ifdef CONFIG_FSL_FASTBOOT
#ifdef CONFIG_ANDROID_RECOVERY
#include <recovery.h>
#endif
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

#if defined(CONFIG_IMX_THERMAL)
static const struct imx_thermal_plat imx6_thermal_plat = {
	.regs = (void *)ANATOP_BASE_ADDR,
	.fuse_bank = 1,
	.fuse_word = 6,
};

U_BOOT_DEVICE(imx6_thermal) = {
	.name = "imx_thermal",
	.platdata = &imx6_thermal_plat,
};
#endif

u32 get_nr_cpus(void)
{
	struct scu_regs *scu = (struct scu_regs *)SCU_BASE_ADDR;
	return readl(&scu->config) & 3;
}

u32 get_cpu_rev(void)
{
	struct anatop_regs *anatop = (struct anatop_regs *)ANATOP_BASE_ADDR;
	u32 reg = readl(&anatop->digprog_sololite);
	u32 type = ((reg >> 16) & 0xff);
	u32 major;

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
	major = ((reg >> 8) & 0xff);
	reg &= 0xff;		/* mx6 silicon revision */
	return (type << 12) | (reg + (0x10 * (major + 1)));
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
#ifdef CONFIG_MX6SX
	struct aipstz_regs *aips3;
#endif

	aips1 = (struct aipstz_regs *)AIPS1_BASE_ADDR;
	aips2 = (struct aipstz_regs *)AIPS2_BASE_ADDR;
#ifdef CONFIG_MX6SX
	aips3 = (struct aipstz_regs *)AIPS3_CONFIG_BASE_ADDR;
#endif

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

#ifdef CONFIG_MX6SX
	/*
	 * Set all MPROTx to be non-bufferable, trusted for R/W,
	 * not forced to user-mode.
	 */
	writel(0x77777777, &aips3->mprot0);
	writel(0x77777777, &aips3->mprot1);

	/*
	 * Set all OPACRx to be non-bufferable, not require
	 * supervisor privilege level for access,allow for
	 * write access and untrusted master access.
	 */
	writel(0x00000000, &aips3->opacr0);
	writel(0x00000000, &aips3->opacr1);
	writel(0x00000000, &aips3->opacr2);
	writel(0x00000000, &aips3->opacr3);
	writel(0x00000000, &aips3->opacr4);
#endif
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
 * Set the PMU_REG_CORE register
 *
 * Set LDO_SOC/PU/ARM regulators to the specified millivolt level.
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

#if (defined(CONFIG_MX6SX) || defined(CONFIG_MX6UL))
	struct wdog_regs *wdog3 = (struct wdog_regs *)WDOG3_BASE_ADDR;
	writew(enable, &wdog3->wmcr);
#endif

	/* Write to the PDE (Power Down Enable) bit */
	writew(enable, &wdog1->wmcr);
	writew(enable, &wdog2->wmcr);
}

#if !defined(CONFIG_MX6UL)
static void set_ahb_rate(u32 val)
{
	struct mxc_ccm_reg *mxc_ccm = (struct mxc_ccm_reg *)CCM_BASE_ADDR;
	u32 reg, div;

	div = get_periph_clk() / val - 1;
	reg = readl(&mxc_ccm->cbcdr);

	writel((reg & (~MXC_CCM_CBCDR_AHB_PODF_MASK)) |
		(div << MXC_CCM_CBCDR_AHB_PODF_OFFSET), &mxc_ccm->cbcdr);
}
#endif

static void clear_mmdc_ch_mask(void)
{
	struct mxc_ccm_reg *mxc_ccm = (struct mxc_ccm_reg *)CCM_BASE_ADDR;
	u32 reg;
	reg = readl(&mxc_ccm->ccdr);

	/* Clear MMDC channel mask */
	reg &= ~(MXC_CCM_CCDR_MMDC_CH1_HS_MASK | MXC_CCM_CCDR_MMDC_CH0_HS_MASK);
	writel(reg, &mxc_ccm->ccdr);
}

static void init_bandgap(void)
{
	struct anatop_regs *anatop = (struct anatop_regs *)ANATOP_BASE_ADDR;
	/*
	 * Ensure the bandgap has stabilized.
	 */
	while (!(readl(&anatop->ana_misc0) & 0x80))
		;
	/*
	 * For best noise performance of the analog blocks using the
	 * outputs of the bandgap, the reftop_selfbiasoff bit should
	 * be set.
	 */
	writel(BM_ANADIG_ANA_MISC0_REFTOP_SELBIASOFF, &anatop->ana_misc0_set);
}


#ifdef CONFIG_MX6SL
static void set_preclk_from_osc(void)
{
	struct mxc_ccm_reg *mxc_ccm = (struct mxc_ccm_reg *)CCM_BASE_ADDR;
	u32 reg;

	reg = readl(&mxc_ccm->cscmr1);
	reg |= MXC_CCM_CSCMR1_PER_CLK_SEL_MASK;
	writel(reg, &mxc_ccm->cscmr1);
}
#endif

#define SRC_SCR_WARM_RESET_ENABLE	0

static void init_src(void)
{
	struct src *src_regs = (struct src *)SRC_BASE_ADDR;
	u32 val;

	/*
	 * force warm reset sources to generate cold reset
	 * for a more reliable restart
	 */
	val = readl(&src_regs->scr);
	val &= ~(1 << SRC_SCR_WARM_RESET_ENABLE);
	writel(val, &src_regs->scr);
}

#ifdef CONFIG_MX6SX
void vadc_power_up(void)
{
	struct iomuxc *iomux = (struct iomuxc *)IOMUXC_BASE_ADDR;
	u32 val;

	/* csi0 */
	val = readl(&iomux->gpr[5]);
	val &= ~IMX6SX_GPR5_CSI1_MUX_CTRL_MASK,
	val |= IMX6SX_GPR5_CSI1_MUX_CTRL_CVD;
	writel(val, &iomux->gpr[5]);

	/* Power on vadc analog
	 * Power down vadc ext power */
	val = readl(GPC_BASE_ADDR + 0);
	val &= ~0x60000;
	writel(val, GPC_BASE_ADDR + 0);

	/* software reset afe  */
	val = readl(&iomux->gpr[1]);
	writel(val | 0x80000, &iomux->gpr[1]);

	udelay(10*1000);

	/* Release reset bit  */
	writel(val & ~0x80000, &iomux->gpr[1]);

	/* Power on vadc ext power */
	val = readl(GPC_BASE_ADDR + 0);
	val |= 0x40000;
	writel(val, GPC_BASE_ADDR + 0);
}

void vadc_power_down(void)
{
	struct iomuxc *iomux = (struct iomuxc *)IOMUXC_BASE_ADDR;
	u32 val;

	/* Power down vadc ext power
	 * Power off vadc analog */
	val = readl(GPC_BASE_ADDR + 0);
	val &= ~0x40000;
	val |= 0x20000;
	writel(val, GPC_BASE_ADDR + 0);

	/* clean csi0 connect to vadc  */
	val = readl(&iomux->gpr[5]);
	val &= ~IMX6SX_GPR5_CSI1_MUX_CTRL_MASK,
	writel(val, &iomux->gpr[5]);
}

void pcie_power_up(void)
{
	set_ldo_voltage(LDO_PU, 1100);	/* Set VDDPU to 1.1V */
}

void pcie_power_off(void)
{
	set_ldo_voltage(LDO_PU, 0);	/* Set VDDPU to 1.1V */
}
#endif

#ifndef CONFIG_MX6UL
static void imx_set_vddpu_power_down(void)
{
	struct anatop_regs *anatop = (struct anatop_regs *)ANATOP_BASE_ADDR;
	u32 val;

	/* need to power down xPU in GPC before turn off PU LDO */
	val = readl(GPC_BASE_ADDR + 0x260);
	writel(val | 0x1, GPC_BASE_ADDR + 0x260);

	val = readl(GPC_BASE_ADDR + 0x0);
	writel(val | 0x1, GPC_BASE_ADDR + 0x0);
	while (readl(GPC_BASE_ADDR + 0x0) & 0x1)
		;

	/* disable VDDPU */
	val = 0x3e00;
	writel(val, &anatop->reg_core_clr);
}
#endif

#if !(defined(CONFIG_MX6SL) || defined(CONFIG_MX6UL))
static void imx_set_pcie_phy_power_down(void)
{
	u32 val;

#ifndef CONFIG_MX6SX
	val = readl(IOMUXC_BASE_ADDR + 0x4);
	val |= 0x1 << 18;
	writel(val, IOMUXC_BASE_ADDR + 0x4);
#else
	val = readl(IOMUXC_GPR_BASE_ADDR + 0x30);
	val |= 0x1 << 30;
	writel(val, IOMUXC_GPR_BASE_ADDR + 0x30);
#endif
}
#endif

int arch_cpu_init(void)
{
	/* Clear the Align bit in SCTLR */
	set_cr(get_cr() & ~CR_A);

#if !defined(CONFIG_MX6SX) && !defined(CONFIG_MX6SL) && !defined(CONFIG_MX6UL)
	/*
	 * imx6sl doesn't have pcie at all.
	 * this bit is not used by imx6sx anymore
	 */
	u32 val;

	/*
	 * There are about 0.02% percentage, random pcie link down
	 * when warm-reset is used.
	 * clear the ref_ssp_en bit16 of gpr1 to workaround it.
	 * then warm-reset imx6q/dl/solo again.
	 */
	val = readl(IOMUXC_BASE_ADDR + 0x4);
	if (val & (0x1 << 16)) {
		val &= ~(0x1 << 16);
		writel(val, IOMUXC_BASE_ADDR + 0x4);
		reset_cpu(0);
	}
#endif

	init_aips();

	/* Need to clear MMDC_CHx_MASK to make warm reset work. */
	clear_mmdc_ch_mask();

	/*
	 * Disable self-bias circuit in the analog bandap.
	 * The self-bias circuit is used by the bandgap during startup.
	 * This bit should be set after the bandgap has initialized.
	 */
	init_bandgap();

#if !defined(CONFIG_MX6UL)
	/*
	 * When low freq boot is enabled, ROM will not set AHB
	 * freq, so we need to ensure AHB freq is 132MHz in such
	 * scenario.
	 */
	if (mxc_get_clock(MXC_ARM_CLK) == 396000000)
		set_ahb_rate(132000000);
#endif

#if defined(CONFIG_MX6UL)
	/*
	 * According to the design team's requirement on i.MX6UL,
	 * the PMIC_STBY_REQ PAD should be configured as open
	 * drain 100K (0x0000b8a0).
	 */
	writel(0x0000b8a0, IOMUXC_BASE_ADDR + 0x29c);
#endif

	/* Set perclk to source from OSC 24MHz */
#if defined(CONFIG_MX6SL)
	set_preclk_from_osc();
#endif

#ifdef CONFIG_MX6SX
	u32 reg;

	/* set uart clk to OSC */
	reg = readl(CCM_BASE_ADDR + 0x24);
	reg |= MXC_CCM_CSCDR1_UART_CLK_SEL;
	writel(reg, CCM_BASE_ADDR + 0x24);
#endif

	imx_set_wdog_powerdown(false); /* Disable PDE bit of WMCR register */

#if !(defined(CONFIG_MX6SL) || defined(CONFIG_MX6UL))
	imx_set_pcie_phy_power_down();
#endif

#if !defined(CONFIG_MX6UL)
	if (!is_mx6dqp())
		imx_set_vddpu_power_down();
#endif

#ifdef CONFIG_APBH_DMA
	/* Start APBH DMA */
	mxs_dma_init();
#endif

	init_src();
	
	if (is_mx6dqp())
		writel(0x80000201, 0xbb0608);

	return 0;
}

int board_postclk_init(void)
{
	set_ldo_voltage(LDO_SOC, 1175);	/* Set VDDSOC to 1.175V */

	return 0;
}

#ifdef CONFIG_SERIAL_TAG
void get_board_serial(struct tag_serialnr *serialnr)
{
	struct ocotp_regs *ocotp = (struct ocotp_regs *)OCOTP_BASE_ADDR;
	struct fuse_bank *bank = &ocotp->bank[0];
	struct fuse_bank0_regs *fuse =
		(struct fuse_bank0_regs *)bank->fuse_regs;

	serialnr->low = fuse->uid_low;
	serialnr->high = fuse->uid_high;
}
#endif

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

#if (defined(CONFIG_MX6SX) || defined(CONFIG_MX6UL))
	if (0 == dev_id) {
		u32 value = readl(&fuse->mac_addr1);
		mac[0] = (value >> 8);
		mac[1] = value ;

		value = readl(&fuse->mac_addr0);
		mac[2] = value >> 24 ;
		mac[3] = value >> 16 ;
		mac[4] = value >> 8 ;
		mac[5] = value ;
	} else {
		u32 value = readl(&fuse->mac_addr2);
		mac[0] = value >> 24 ;
		mac[1] = value >> 16 ;
		mac[2] = value >> 8 ;
		mac[3] = value ;

		value = readl(&fuse->mac_addr1);
		mac[4] = value >> 24 ;
		mac[5] = value >> 16 ;
	}
#else
	u32 value = readl(&fuse->mac_addr_high);
	mac[0] = (value >> 8);
	mac[1] = value ;

	value = readl(&fuse->mac_addr_low);
	mac[2] = value >> 24 ;
	mac[3] = value >> 16 ;
	mac[4] = value >> 8 ;
	mac[5] = value ;

#endif
}
#endif

#ifdef CONFIG_MX6SX
int arch_auxiliary_core_up(u32 core_id, u32 boot_private_data)
{
	struct src *src_reg;
	u32 stack, pc;

	if (!boot_private_data)
		return 1;

	stack = *(u32 *)boot_private_data;
	pc = *(u32 *)(boot_private_data + 4);

	/* Set the stack and pc to M4 bootROM */
	writel(stack, M4_BOOTROM_BASE_ADDR);
	writel(pc, M4_BOOTROM_BASE_ADDR + 4);

	/* Enable M4 */
	src_reg = (struct src *)SRC_BASE_ADDR;
	setbits_le32(&src_reg->scr, 0x00400000);
	clrbits_le32(&src_reg->scr, 0x00000010);

	return 0;
}

int arch_auxiliary_core_check_up(u32 core_id)
{
	struct src *src_reg = (struct src *)SRC_BASE_ADDR;
	unsigned val;

	val = readl(&src_reg->scr);

	if (val & 0x00000010)
		return 0;  /* assert in reset */

	return 1;
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
 * After reset, if GPR10[28] is 1, ROM will use GPR9[25:0]
 * instead of SBMR1 to determine the boot device.
 */
const struct boot_mode soc_boot_modes[] = {
	{"normal",	MAKE_CFGVAL(0x00, 0x00, 0x00, 0x00)},
	/* reserved value should start rom usb */
	{"usb",		MAKE_CFGVAL(0x01, 0x00, 0x00, 0x00)},
	{"sata",	MAKE_CFGVAL(0x20, 0x00, 0x00, 0x00)},
	{"ecspi1:0",	MAKE_CFGVAL(0x30, 0x00, 0x00, 0x08)},
	{"ecspi1:1",	MAKE_CFGVAL(0x30, 0x00, 0x00, 0x18)},
	{"ecspi1:2",	MAKE_CFGVAL(0x30, 0x00, 0x00, 0x28)},
	{"ecspi1:3",	MAKE_CFGVAL(0x30, 0x00, 0x00, 0x38)},
	/* 4 bit bus width */
	{"esdhc1",	MAKE_CFGVAL(0x40, 0x20, 0x00, 0x00)},
	{"esdhc2",	MAKE_CFGVAL(0x40, 0x28, 0x00, 0x00)},
	{"esdhc3",	MAKE_CFGVAL(0x40, 0x30, 0x00, 0x00)},
	{"esdhc4",	MAKE_CFGVAL(0x40, 0x38, 0x00, 0x00)},
	{NULL,		0},
};

enum boot_device get_boot_device(void)
{
	enum boot_device boot_dev = UNKNOWN_BOOT;
	uint soc_sbmr = readl(SRC_BASE_ADDR + 0x4);
	uint bt_mem_ctl = (soc_sbmr & 0x000000FF) >> 4 ;
	uint bt_mem_type = (soc_sbmr & 0x00000008) >> 3;
	uint bt_dev_port = (soc_sbmr & 0x00001800) >> 11;

	switch (bt_mem_ctl) {
	case 0x0:
		if (bt_mem_type)
			boot_dev = ONE_NAND_BOOT;
		else
			boot_dev = WEIM_NOR_BOOT;
		break;
	case 0x2:
			boot_dev = SATA_BOOT;
		break;
	case 0x3:
		if (bt_mem_type)
			boot_dev = I2C_BOOT;
		else
			boot_dev = SPI_NOR_BOOT;
		break;
	case 0x4:
	case 0x5:
		boot_dev = bt_dev_port + SD1_BOOT;
		break;
	case 0x6:
	case 0x7:
		boot_dev = bt_dev_port + MMC1_BOOT;
		break;
	case 0x8 ... 0xf:
		boot_dev = NAND_BOOT;
		break;
	default:
		boot_dev = UNKNOWN_BOOT;
		break;
	}

    return boot_dev;
}

void s_init(void)
{
	struct anatop_regs *anatop = (struct anatop_regs *)ANATOP_BASE_ADDR;
	struct mxc_ccm_reg *ccm = (struct mxc_ccm_reg *)CCM_BASE_ADDR;
	u32 mask480;
	u32 mask528;
	u32 reg, periph1, periph2;

	if (is_cpu_type(MXC_CPU_MX6SX) || is_cpu_type(MXC_CPU_MX6UL))
		return;

	/* Due to hardware limitation, on MX6Q we need to gate/ungate all PFDs
	 * to make sure PFD is working right, otherwise, PFDs may
	 * not output clock after reset, MX6DL and MX6SL have added 396M pfd
	 * workaround in ROM code, as bus clock need it
	 */

	mask480 = ANATOP_PFD_CLKGATE_MASK(0) |
		ANATOP_PFD_CLKGATE_MASK(1) |
		ANATOP_PFD_CLKGATE_MASK(2) |
		ANATOP_PFD_CLKGATE_MASK(3);
	mask528 = ANATOP_PFD_CLKGATE_MASK(1) |
		ANATOP_PFD_CLKGATE_MASK(3);

	reg = readl(&ccm->cbcmr);
	periph2 = ((reg & MXC_CCM_CBCMR_PRE_PERIPH2_CLK_SEL_MASK)
		>> MXC_CCM_CBCMR_PRE_PERIPH2_CLK_SEL_OFFSET);
	periph1 = ((reg & MXC_CCM_CBCMR_PRE_PERIPH_CLK_SEL_MASK)
		>> MXC_CCM_CBCMR_PRE_PERIPH_CLK_SEL_OFFSET);

	/* Checking if PLL2 PFD0 or PLL2 PFD2 is using for periph clock */
	if ((periph2 != 0x2) && (periph1 != 0x2))
		mask528 |= ANATOP_PFD_CLKGATE_MASK(0);

	if ((periph2 != 0x1) && (periph1 != 0x1) &&
		(periph2 != 0x3) && (periph1 != 0x3))
		mask528 |= ANATOP_PFD_CLKGATE_MASK(2);

	writel(mask480, &anatop->pfd_480_set);
	writel(mask528, &anatop->pfd_528_set);
	writel(mask480, &anatop->pfd_480_clr);
	writel(mask528, &anatop->pfd_528_clr);
}

void set_wdog_reset(struct wdog_regs *wdog)
{
	u32 reg = readw(&wdog->wcr);
	/*
	 * use WDOG_B mode to reset external pmic because it's risky for the
	 * following watchdog reboot in case of cpu freq at lowest 400Mhz with
	 * ldo-bypass mode. Because boot frequency maybe higher 800Mhz i.e. So
	 * in ldo-bypass mode watchdog reset will only triger POR reset, not
	 * WDOG reset. But below code depends on hardware design, if HW didn't
	 * connect WDOG_B pin to external pmic such as i.mx6slevk, we can skip
	 * these code since it assumed boot from 400Mhz always.
	 */
	reg = readw(&wdog->wcr);
	reg |= 1 << 3;
	/*
	 * WDZST bit is write-once only bit. Align this bit in kernel,
	 * otherwise kernel code will have no chance to set this bit.
	 */
	reg |= 1 << 0;
	writew(reg, &wdog->wcr);
}

void reset_misc(void)
{    
#ifdef CONFIG_VIDEO_MXS
    if (is_cpu_type(MXC_CPU_MX6UL))
        lcdif_power_down();
#endif
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

static int arm_orig_podf;
void set_arm_freq_400M(bool is_400M)
{
	struct mxc_ccm_reg *mxc_ccm = (struct mxc_ccm_reg *)CCM_BASE_ADDR;

	if (is_400M)
		writel(0x1, &mxc_ccm->cacrr);
	else
		writel(arm_orig_podf, &mxc_ccm->cacrr);
}

void prep_anatop_bypass(void)
{
	struct mxc_ccm_reg *mxc_ccm = (struct mxc_ccm_reg *)CCM_BASE_ADDR;

	arm_orig_podf = readl(&mxc_ccm->cacrr);
	/*
	 * Downgrade ARM speed to 400Mhz as half of boot 800Mhz before ldo
	 * bypassed, also downgrade internal vddarm ldo to 0.975V.
	 * VDDARM_IN 0.975V + 125mV = 1.1V < Max(1.3V)
	 * otherwise at 800Mhz(i.mx6dl):
	 * VDDARM_IN 1.175V + 125mV = 1.3V = Max(1.3V)
	 * We need provide enough gap in this case.
	 * skip if boot from 400M.
	 */
	if (!arm_orig_podf)
		set_arm_freq_400M(true);
#if !defined(CONFIG_MX6DL) && !defined(CONFIG_MX6SX)
	set_ldo_voltage(LDO_ARM, 975);
#else
	set_ldo_voltage(LDO_ARM, 1150);
#endif
}

int set_anatop_bypass(int wdog_reset_pin)
{
	struct anatop_regs *anatop= (struct anatop_regs*)ANATOP_BASE_ADDR;
	struct wdog_regs *wdog;
	u32 reg = readl(&anatop->reg_core);

	/* bypass VDDARM/VDDSOC */
	reg = reg | (0x1F << 18) | 0x1F;
	writel(reg, &anatop->reg_core);

	if (wdog_reset_pin == 2)
		wdog = (struct wdog_regs *) WDOG2_BASE_ADDR;
	else if (wdog_reset_pin == 1)
		wdog = (struct wdog_regs *) WDOG1_BASE_ADDR;
	else
		return arm_orig_podf;
	set_wdog_reset(wdog);
	return arm_orig_podf;
}

void finish_anatop_bypass(void)
{
	if (!arm_orig_podf)
		set_arm_freq_400M(false);
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
	int reg, count;
	u8 val;

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

	/* Workaround to clear the overflow condition */
	if (readb(&hdmi->ih_fc_stat2) & HDMI_IH_FC_STAT2_OVERFLOW_MASK) {
		/* TMDS software reset */
		writeb((u8)~HDMI_MC_SWRSTZ_TMDSSWRST_REQ, &hdmi->mc_swrstz);
		val = readb(&hdmi->fc_invidconf);
		for (count = 0 ; count < 5 ; count++)
			writeb(val, &hdmi->fc_invidconf);
	}
}
#endif

#ifndef CONFIG_SYS_L2CACHE_OFF
#ifndef CONFIG_MX6UL
#define IOMUXC_GPR11_L2CACHE_AS_OCRAM 0x00000002
void v7_outer_cache_enable(void)
{
	struct pl310_regs *const pl310 = (struct pl310_regs *)L2_PL310_BASE;
	unsigned int val, cache_id;

#if defined CONFIG_MX6SL
	struct iomuxc *iomux = (struct iomuxc *)IOMUXC_BASE_ADDR;
	val = readl(&iomux->gpr[11]);
	if (val & IOMUXC_GPR11_L2CACHE_AS_OCRAM) {
		/* L2 cache configured as OCRAM, reset it */
		val &= ~IOMUXC_GPR11_L2CACHE_AS_OCRAM;
		writel(val, &iomux->gpr[11]);
	}
#endif

	/* Must disable the L2 before changing the latency parameters */
	clrbits_le32(&pl310->pl310_ctrl, L2X0_CTRL_EN);

	writel(0x132, &pl310->pl310_tag_latency_ctrl);
	writel(0x132, &pl310->pl310_data_latency_ctrl);

	val = readl(&pl310->pl310_prefetch_ctrl);

	/* Turn on the L2 I/D prefetch, double linefill */
	/* Set prefetch offset with any value except 23 as per errata 765569 */
	val |= 0x7000000f;

	/*
	 * The L2 cache controller(PL310) version on the i.MX6D/Q is r3p1-50rel0
	 * The L2 cache controller(PL310) version on the i.MX6DL/SOLO/SL/SX/DQP
	 * is r3p2.
	 * But according to ARM PL310 errata: 752271
	 * ID: 752271: Double linefill feature can cause data corruption
	 * Fault Status: Present in: r3p0, r3p1, r3p1-50rel0. Fixed in r3p2
	 * Workaround: The only workaround to this erratum is to disable the
	 * double linefill feature. This is the default behavior.
	 */
	cache_id = readl(&pl310->pl310_cache_id);
	if (((cache_id & L2X0_CACHE_ID_PART_MASK) == L2X0_CACHE_ID_PART_L310)
	    && ((cache_id & L2X0_CACHE_ID_RTL_MASK) < L2X0_CACHE_ID_RTL_R3P2))
		val &= ~(1 << 30);
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
#endif
#endif /* !CONFIG_SYS_L2CACHE_OFF */

#ifdef CONFIG_FSL_FASTBOOT

#ifdef CONFIG_ANDROID_RECOVERY
#define ANDROID_RECOVERY_BOOT  (1 << 7)
/* check if the recovery bit is set by kernel, it can be set by kernel
 * issue a command '# reboot recovery' */
int recovery_check_and_clean_flag(void)
{
	int flag_set = 0;
	u32 reg;
	reg = readl(SNVS_BASE_ADDR + SNVS_LPGPR);

	flag_set = !!(reg & ANDROID_RECOVERY_BOOT);
    printf("check_and_clean: reg %x, flag_set %d\n", reg, flag_set);
	/* clean it in case looping infinite here.... */
	if (flag_set) {
		reg &= ~ANDROID_RECOVERY_BOOT;
		writel(reg, SNVS_BASE_ADDR + SNVS_LPGPR);
	}

	return flag_set;
}
#endif /*CONFIG_ANDROID_RECOVERY*/

#define ANDROID_FASTBOOT_BOOT  (1 << 8)
/* check if the recovery bit is set by kernel, it can be set by kernel
 * issue a command '# reboot fastboot' */
int fastboot_check_and_clean_flag(void)
{
	int flag_set = 0;
	u32 reg;

	reg = readl(SNVS_BASE_ADDR + SNVS_LPGPR);

	flag_set = !!(reg & ANDROID_FASTBOOT_BOOT);

	/* clean it in case looping infinite here.... */
	if (flag_set) {
		reg &= ~ANDROID_FASTBOOT_BOOT;
		writel(reg, SNVS_BASE_ADDR + SNVS_LPGPR);
	}

	return flag_set;
}

void fastboot_enable_flag(void)
{
	setbits_le32(SNVS_BASE_ADDR + SNVS_LPGPR,
		ANDROID_FASTBOOT_BOOT);
}
#endif /*CONFIG_FSL_FASTBOOT*/
