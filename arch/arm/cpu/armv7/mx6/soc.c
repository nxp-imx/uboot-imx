/*
 * (C) Copyright 2007
 * Sascha Hauer, Pengutronix
 *
 * (C) Copyright 2009-2013 Freescale Semiconductor, Inc.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <asm/armv7.h>
#include <asm/pl310.h>
#include <asm/errno.h>
#include <asm/io.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/crm_regs.h>
#include <asm/arch/clock.h>
#include <asm/arch/sys_proto.h>
#include <asm/imx-common/boot_mode.h>
#include <asm/imx-common/dma.h>
#include <libfdt.h>
#include <stdbool.h>

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

#if defined(CONFIG_SECURE_BOOT)
#include <asm/arch/mx6_secure.h>
#endif

u32 get_cpu_rev(void)
{
	struct anatop_regs *anatop = (struct anatop_regs *)ANATOP_BASE_ADDR;
	u32 reg = readl(&anatop->digprog_sololite);
	u32 type = ((reg >> 16) & 0xff);

	if (type != MXC_CPU_MX6SL) {
		reg = readl(&anatop->digprog);
		type = ((reg >> 16) & 0xff);
		if (type == MXC_CPU_MX6DL) {
			struct scu_regs *scu = (struct scu_regs *)SCU_BASE_ADDR;
			u32 cfg = readl(&scu->config) & 3;

			if (!cfg)
				type = MXC_CPU_MX6SOLO;
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

/*
 * Set the VDDSOC
 *
 * Mask out the REG_CORE[22:18] bits (REG2_TRIG) and set
 * them to the specified millivolt level.
 * Possible values are from 0.725V to 1.450V in steps of
 * 0.025V (25mV).
 */
void set_vddsoc(u32 mv)
{
	struct anatop_regs *anatop = (struct anatop_regs *)ANATOP_BASE_ADDR;
	u32 val, reg = readl(&anatop->reg_core);

	if (mv < 725)
		val = 0x00;	/* Power gated off */
	else if (mv > 1450)
		val = 0x1F;	/* Power FET switched full on. No regulation */
	else
		val = (mv - 700) / 25;

	/*
	 * Mask out the REG_CORE[22:18] bits (REG2_TRIG)
	 * and set them to the calculated value (0.7V + val * 0.25V)
	 */
	reg = (reg & ~(0x1F << 18)) | (val << 18);
	writel(reg, &anatop->reg_core);

	/* ROM may modify LDO ramp up time according to fuse setting for safe,
	 * we need to reset these settings to match the reset value: 0'b00
	 */
	reg = readl(&anatop->ana_misc2);
	reg &= ~(0x3f << 24);
	writel(reg, &anatop->ana_misc2);

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
	struct iim_regs *iim = (struct iim_regs *)IMX_IIM_BASE;
	struct fuse_bank *bank = &iim->bank[1];
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

static void imx_reset_pfd(void)
{
	struct anatop_regs *anatop = (struct anatop_regs *)ANATOP_BASE_ADDR;

	/*
	 * Per the IC design, we need to gate/ungate all the unused PFDs
	 * to make sure PFD is working correctly, otherwise, PFDs may not
	 * not output clock after reset.
	 */

	writel(BM_ANADIG_PFD_480_PFD3_CLKGATE  |
		BM_ANADIG_PFD_480_PFD2_CLKGATE |
		BM_ANADIG_PFD_480_PFD1_CLKGATE |
		BM_ANADIG_PFD_480_PFD0_CLKGATE, &anatop->pfd_480_set);
#ifdef CONFIG_MX6Q
	writel(BM_ANADIG_PFD_528_PFD2_CLKGATE  |
		BM_ANADIG_PFD_528_PFD1_CLKGATE |
		BM_ANADIG_PFD_528_PFD0_CLKGATE, &anatop->pfd_528_set);
#else
	writel(BM_ANADIG_PFD_528_PFD1_CLKGATE  |
		BM_ANADIG_PFD_528_PFD0_CLKGATE, &anatop->pfd_528_set);
#endif
	writel(BM_ANADIG_PFD_480_PFD3_CLKGATE  |
		BM_ANADIG_PFD_480_PFD2_CLKGATE |
		BM_ANADIG_PFD_480_PFD1_CLKGATE |
		BM_ANADIG_PFD_480_PFD0_CLKGATE, &anatop->pfd_480_clr);
#ifdef CONFIG_MX6Q
	writel(BM_ANADIG_PFD_528_PFD2_CLKGATE  |
		BM_ANADIG_PFD_528_PFD1_CLKGATE |
		BM_ANADIG_PFD_528_PFD0_CLKGATE, &anatop->pfd_528_clr);
#else
	writel(BM_ANADIG_PFD_528_PFD1_CLKGATE  |
		BM_ANADIG_PFD_528_PFD0_CLKGATE, &anatop->pfd_528_clr);
#endif
}

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

static void imx_set_pcie_phy_power_down(void)
{
	u32 val;

	val = readl(IOMUXC_BASE_ADDR + 0x4);
	val |= 0x1 << 18;
	writel(val, IOMUXC_BASE_ADDR + 0x4);
}

int arch_cpu_init(void)
{
	init_aips();
	set_vddsoc(1200);	/* Set VDDSOC to 1.2V */

	imx_set_wdog_powerdown(false); /* Disable PDE bit of WMCR register */

	imx_reset_pfd();
	imx_set_pcie_phy_power_down();
	imx_set_vddpu_power_down();

#ifdef CONFIG_APBH_DMA
	/* Start APBH DMA */
	mxs_dma_init();
#endif

	return 0;
}

#ifndef CONFIG_SYS_DCACHE_OFF
void enable_caches(void)
{
	/* Avoid random hang when download by usb */
	invalidate_dcache_all();
	/* Enable D-cache. I-cache is already enabled in start.S */
	dcache_enable();
}
#endif

#if defined(CONFIG_FEC_MXC)
void imx_get_mac_from_fuse(int dev_id, unsigned char *mac)
{
	struct iim_regs *iim = (struct iim_regs *)IMX_IIM_BASE;
	struct fuse_bank *bank = &iim->bank[4];
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
	struct iim_regs *iim = (struct iim_regs *)IMX_IIM_BASE;
	struct fuse_bank *bank = &iim->bank[0];
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

#ifndef CONFIG_SYS_L2CACHE_OFF
void v7_outer_cache_enable(void)
{
	struct pl310_regs *const pl310 =
		(struct pl310_regs *)CONFIG_SYS_PL310_BASE;

	writel(1, &pl310->pl310_ctrl);
}

void v7_outer_cache_disable(void)
{
	struct pl310_regs *const pl310 =
		(struct pl310_regs *)CONFIG_SYS_PL310_BASE;

	writel(0, &pl310->pl310_ctrl);
}
#endif /* !CONFIG_SYS_L2CACHE_OFF */

#ifdef CONFIG_ARCH_MISC_INIT
int arch_misc_init(void)
{
#ifdef CONFIG_SECURE_BOOT
	get_hab_status();
#endif
	return 0;
}
#endif /* !CONFIG_ARCH_MISC_INIT */

#ifdef CONFIG_SECURE_BOOT
/* -------- start of HAB API updates ------------*/
#define hab_rvt_report_event ((hab_rvt_report_event_t *)HAB_RVT_REPORT_EVENT)
#define hab_rvt_report_status ((hab_rvt_report_status_t *)HAB_RVT_REPORT_STATUS)
#define hab_rvt_authenticate_image \
	((hab_rvt_authenticate_image_t *)HAB_RVT_AUTHENTICATE_IMAGE)
#define hab_rvt_entry ((hab_rvt_entry_t *) HAB_RVT_ENTRY)
#define hab_rvt_exit ((hab_rvt_exit_t *) HAB_RVT_EXIT)
#define hab_rvt_clock_init HAB_RVT_CLOCK_INIT

#define IVT_SIZE		0x20
#define ALIGN_SIZE		0x1000
#define CSF_PAD_SIZE		0x2000

/*
 * +------------+  0x0 (DDR_UIMAGE_START) -
 * |   Header   |                          |
 * +------------+  0x40                    |
 * |            |                          |
 * |            |                          |
 * |            |                          |
 * |            |                          |
 * | Image Data |                          |
 * .            |                          |
 * .            |                           > Stuff to be authenticated ----+
 * .            |                          |                                |
 * |            |                          |                                |
 * |            |                          |                                |
 * +------------+                          |                                |
 * |            |                          |                                |
 * | Fill Data  |                          |                                |
 * |            |                          |                                |
 * +------------+ Align to ALIGN_SIZE      |                                |
 * |    IVT     |                          |                                |
 * +------------+ + IVT_SIZE              -                                 |
 * |            |                                                           |
 * |  CSF DATA  | <---------------------------------------------------------+
 * |            |
 * +------------+
 * |            |
 * | Fill Data  |
 * |            |
 * +------------+ + CSF_PAD_SIZE
 */

int check_hab_enable(void)
{
	u32 reg;
	int result = 0;
	struct iim_regs *iim = (struct iim_regs *)IMX_IIM_BASE;
	struct fuse_bank *bank = &iim->bank[0];
	struct fuse_bank0_regs *fuse_bank0 =
			(struct fuse_bank0_regs *)bank->fuse_regs;

	reg = readl(&fuse_bank0->cfg5);
	if (reg & 0x2)
		result = 1;

	return result;
}

void display_event(uint8_t *event_data, size_t bytes)
{
	uint32_t i;
	if ((event_data) && (bytes > 0)) {
		for (i = 0; i < bytes; i++) {
			if (i == 0)
				printf("\t0x%02x", event_data[i]);
			else if ((i % 8) == 0)
				printf("\n\t0x%02x", event_data[i]);
			else
				printf(" 0x%02x", event_data[i]);
		}
	}
}

int get_hab_status(void)
{
	uint32_t index = 0; /* Loop index */
	uint8_t event_data[128]; /* Event data buffer */
	size_t bytes = sizeof(event_data); /* Event size in bytes */
	hab_config_t config = 0;
	hab_state_t state = 0;

	/* Check HAB status */
	if (hab_rvt_report_status(&config, &state) != HAB_SUCCESS) {
		printf("\nHAB Configuration: 0x%02x, HAB State: 0x%02x\n",
			config, state);

		/* Display HAB Error events */
		while (hab_rvt_report_event(HAB_FAILURE, index, event_data,
				&bytes) == HAB_SUCCESS) {
			printf("\n");
			printf("--------- HAB Event %d -----------------\n",
					index + 1);
			printf("event data:\n");
			display_event(event_data, bytes);
			printf("\n");
			bytes = sizeof(event_data);
			index++;
		}
	}
	/* Display message if no HAB events are found */
	else {
		printf("\nHAB Configuration: 0x%02x, HAB State: 0x%02x\n",
			config, state);
		printf("No HAB Events Found!\n\n");
	}
	return 0;
}

void hab_caam_clock_enable(void)
{
	u32 reg = 0;

	reg = readl(CCM_BASE_ADDR + CLKCTL_CCGR0); /* CCGR0 */
	reg |= 0x3F00; /*CG4 ~ CG6, enable CAAM clocks*/
	writel(reg, CCM_BASE_ADDR + CLKCTL_CCGR0);
}


void hab_caam_clock_disable(void)
{
	u32 reg = 0;

	reg = readl(CCM_BASE_ADDR + CLKCTL_CCGR0); /* CCGR0 */
	reg &= ~0x3F00; /*CG4 ~ CG6, disable CAAM clocks*/
	writel(reg, CCM_BASE_ADDR + CLKCTL_CCGR0);
}

#ifdef DEBUG_AUTHENTICATE_IMAGE
void dump_mem(uint32_t addr, int size)
{
	int i;

	for (i = 0; i < size; i += 4) {
		if (i != 0) {
			if (i % 16 == 0)
				printf("\n");
			else
				printf(" ");
		}

		printf("0x%08x", *(uint32_t *)addr);
		addr += 4;
	}

	printf("\n");

	return;
}
#endif

uint32_t authenticate_image(uint32_t ddr_start, uint32_t image_size)
{
	uint32_t load_addr = 0;
	size_t bytes;
	ptrdiff_t ivt_offset = 0;
	int result = 0;
	ulong start;

	if (check_hab_enable() == 1) {
		printf("\nAuthenticate uImage from DDR location 0x%x...\n",
			ddr_start);

		hab_caam_clock_enable();

		if (hab_rvt_entry() == HAB_SUCCESS) {
			/* If not already aligned, Align to ALIGN_SIZE */
			ivt_offset = (image_size + ALIGN_SIZE - 1) &
					~(ALIGN_SIZE - 1);

			start = ddr_start;
			bytes = ivt_offset + IVT_SIZE + CSF_PAD_SIZE;

#ifdef DEBUG_AUTHENTICATE_IMAGE
			printf("\nivt_offset = 0x%x, ivt addr = 0x%x\n",
			       ivt_offset, ddr_start + ivt_offset);
			printf("Dumping IVT\n");
			dump_mem(ddr_start + ivt_offset, 0x20);

			printf("Dumping CSF Header\n");
			dump_mem(ddr_start + ivt_offset + 0x20, 0x40);

			get_hab_status();

			printf("\nCalling authenticate_image in ROM\n");
			printf("\tivt_offset = 0x%x\n\tstart = 0x%08x"
			       "\n\tbytes = 0x%x\n", ivt_offset, start, bytes);
#endif

			load_addr = (uint32_t)hab_rvt_authenticate_image(
					HAB_CID_UBOOT,
					ivt_offset, (void **)&start,
					(size_t *)&bytes, NULL);
			if (hab_rvt_exit() != HAB_SUCCESS) {
				printf("hab exit function fail\n");
				load_addr = 0;
			}
		} else
			printf("hab entry function fail\n");

		hab_caam_clock_disable();

		get_hab_status();
	}

	if ((!check_hab_enable()) || (load_addr != 0))
		result = 1;

	return result;
}
/* ----------- end of HAB API updates ------------*/
#endif
