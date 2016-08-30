/*
 * (C) Copyright 2007
 * Sascha Hauer, Pengutronix
 *
 * (C) Copyright 2009-2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/errno.h>
#include <asm/io.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/clock.h>
#include <asm/arch/sys_proto.h>
#include <asm/imx-common/boot_mode.h>
#include <asm/imx-common/dma.h>
#include <asm/imx-common/hab.h>
#include <stdbool.h>
#include <asm/arch/mxc_hdmi.h>
#include <asm/arch/crm_regs.h>
#include <dm.h>
#include <imx_thermal.h>
#include <mmc.h>
#if defined(CONFIG_FSL_FASTBOOT) && defined(CONFIG_ANDROID_RECOVERY)
#include <recovery.h>
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

#if defined(CONFIG_SECURE_BOOT)
struct imx_sec_config_fuse_t const imx_sec_config_fuse = {
	.bank = 0,
	.word = 6,
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
	u32 major, cfg = 0;

	if (type != MXC_CPU_MX6SL) {
		reg = readl(&anatop->digprog);
		struct scu_regs *scu = (struct scu_regs *)SCU_BASE_ADDR;
		cfg = readl(&scu->config) & 3;
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
	if ((major >= 1) &&
	    ((type == MXC_CPU_MX6Q) || (type == MXC_CPU_MX6D))) {
		major--;
		type = MXC_CPU_MX6QP;
		if (cfg == 1)
			type = MXC_CPU_MX6DP;
	}
	reg &= 0xff;		/* mx6 silicon revision */
	return (type << 12) | (reg + (0x10 * (major + 1)));
}

/*
 * OCOTP_CFG3[17:16] (see Fusemap Description Table offset 0x440)
 * defines a 2-bit SPEED_GRADING
 */
#define OCOTP_CFG3_SPEED_SHIFT	16
#define OCOTP_CFG3_SPEED_800MHZ	0
#define OCOTP_CFG3_SPEED_850MHZ	1
#define OCOTP_CFG3_SPEED_1GHZ	2
#define OCOTP_CFG3_SPEED_1P2GHZ	3

/*
 * For i.MX6UL
 */
#define OCOTP_CFG3_SPEED_528MHZ 1
#define OCOTP_CFG3_SPEED_696MHZ 2

u32 get_cpu_speed_grade_hz(void)
{
	struct ocotp_regs *ocotp = (struct ocotp_regs *)OCOTP_BASE_ADDR;
	struct fuse_bank *bank = &ocotp->bank[0];
	struct fuse_bank0_regs *fuse =
		(struct fuse_bank0_regs *)bank->fuse_regs;
	uint32_t val;

	val = readl(&fuse->cfg3);
	val >>= OCOTP_CFG3_SPEED_SHIFT;
	val &= 0x3;

	if (is_cpu_type(MXC_CPU_MX6UL) || is_cpu_type(MXC_CPU_MX6ULL)) {
		if (val == OCOTP_CFG3_SPEED_528MHZ)
			return 528000000;
		else if (val == OCOTP_CFG3_SPEED_696MHZ)
			return 69600000;
		else
			return 0;
	}

	switch (val) {
	/* Valid for IMX6DQ */
	case OCOTP_CFG3_SPEED_1P2GHZ:
		if (is_cpu_type(MXC_CPU_MX6Q) || is_cpu_type(MXC_CPU_MX6D) ||
			is_cpu_type(MXC_CPU_MX6QP) || is_cpu_type(MXC_CPU_MX6DP))
			return 1200000000;
	/* Valid for IMX6SX/IMX6SDL/IMX6DQ */
	case OCOTP_CFG3_SPEED_1GHZ:
		return 996000000;
	/* Valid for IMX6DQ */
	case OCOTP_CFG3_SPEED_850MHZ:
		if (is_cpu_type(MXC_CPU_MX6Q) || is_cpu_type(MXC_CPU_MX6D) ||
			is_cpu_type(MXC_CPU_MX6QP) || is_cpu_type(MXC_CPU_MX6DP))
			return 852000000;
	/* Valid for IMX6SX/IMX6SDL/IMX6DQ */
	case OCOTP_CFG3_SPEED_800MHZ:
		return 792000000;
	}
	return 0;
}

/*
 * OCOTP_MEM0[7:6] (see Fusemap Description Table offset 0x480)
 * defines a 2-bit Temperature Grade
 *
 * return temperature grade and min/max temperature in celcius
 */
#define OCOTP_MEM0_TEMP_SHIFT          6

u32 get_cpu_temp_grade(int *minc, int *maxc)
{
	struct ocotp_regs *ocotp = (struct ocotp_regs *)OCOTP_BASE_ADDR;
	struct fuse_bank *bank = &ocotp->bank[1];
	struct fuse_bank1_regs *fuse =
		(struct fuse_bank1_regs *)bank->fuse_regs;
	uint32_t val;

	val = readl(&fuse->mem0);
	val >>= OCOTP_MEM0_TEMP_SHIFT;
	val &= 0x3;

	if (minc && maxc) {
		if (val == TEMP_AUTOMOTIVE) {
			*minc = -40;
			*maxc = 125;
		} else if (val == TEMP_INDUSTRIAL) {
			*minc = -40;
			*maxc = 105;
		} else if (val == TEMP_EXTCOMMERCIAL) {
			*minc = -20;
			*maxc = 105;
		} else {
			*minc = 0;
			*maxc = 95;
		}
	}
	return val;
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

	if (type == MXC_CPU_MX6QP || type == MXC_CPU_MX6DP)
		cpurev = (MXC_CPU_MX6Q) << 12 | ((cpurev + 0x10) & 0xFFF);

	return cpurev;
}
#endif

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
	u32 reg;
	reg = readl(&mxc_ccm->ccdr);

	/* Clear MMDC channel mask */
	if (is_cpu_type(MXC_CPU_MX6SX) || is_cpu_type(MXC_CPU_MX6UL) ||
	    is_cpu_type(MXC_CPU_MX6SL) || is_cpu_type(MXC_CPU_MX6ULL))
		reg &= ~(MXC_CCM_CCDR_MMDC_CH1_HS_MASK);
	else
		reg &= ~(MXC_CCM_CCDR_MMDC_CH1_HS_MASK | MXC_CCM_CCDR_MMDC_CH0_HS_MASK);
	writel(reg, &mxc_ccm->ccdr);
}

#define OCOTP_MEM0_REFTOP_TRIM_SHIFT          8

static void init_bandgap(void)
{
	struct anatop_regs *anatop = (struct anatop_regs *)ANATOP_BASE_ADDR;
	struct ocotp_regs *ocotp = (struct ocotp_regs *)OCOTP_BASE_ADDR;
	struct fuse_bank *bank = &ocotp->bank[1];
	struct fuse_bank1_regs *fuse =
		(struct fuse_bank1_regs *)bank->fuse_regs;
	uint32_t val;

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
	/*
	 * On i.MX6ULL,we need to set VBGADJ bits according to the
	 * REFTOP_TRIM[3:0] in fuse table
	 *	000 - set REFTOP_VBGADJ[2:0] to 3b'110,
	 *	110 - set REFTOP_VBGADJ[2:0] to 3b'000,
	 *	001 - set REFTOP_VBGADJ[2:0] to 3b'001,
	 *	010 - set REFTOP_VBGADJ[2:0] to 3b'010,
	 *	011 - set REFTOP_VBGADJ[2:0] to 3b'011,
	 *	100 - set REFTOP_VBGADJ[2:0] to 3b'100,
	 *	101 - set REFTOP_VBGADJ[2:0] to 3b'101,
	 *	111 - set REFTOP_VBGADJ[2:0] to 3b'111,
	 */
	if (is_cpu_type(MXC_CPU_MX6ULL)) {
		val = readl(&fuse->mem0);
		val >>= OCOTP_MEM0_REFTOP_TRIM_SHIFT;
		val &= 0x7;

		writel(val << BM_ANADIG_ANA_MISC0_REFTOP_VBGADJ_SHIFT,
		       &anatop->ana_misc0_set);
	}
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

static void set_uart_from_osc(void)
{
	u32 reg;

	/* set uart clk to OSC */
	reg = readl(CCM_BASE_ADDR + 0x24);
	reg |= MXC_CCM_CSCDR1_UART_CLK_SEL;
	writel(reg, CCM_BASE_ADDR + 0x24);
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

	if (!is_cpu_type(MXC_CPU_MX6SX)) {
		val = readl(IOMUXC_BASE_ADDR + 0x4);
		val |= 0x1 << 18;
		writel(val, IOMUXC_BASE_ADDR + 0x4);
	} else {
		val = readl(IOMUXC_GPR_BASE_ADDR + 0x30);
		val |= 0x1 << 30;
		writel(val, IOMUXC_GPR_BASE_ADDR + 0x30);
	}
}

int arch_cpu_init(void)
{
	if (!is_cpu_type(MXC_CPU_MX6SL) && !is_cpu_type(MXC_CPU_MX6SX)
	    && !is_cpu_type(MXC_CPU_MX6UL) && !is_cpu_type(MXC_CPU_MX6ULL)) {
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
	}

	init_aips();

	/* Need to clear MMDC_CHx_MASK to make warm reset work. */
	clear_mmdc_ch_mask();

	/*
	 * Disable self-bias circuit in the analog bandap.
	 * The self-bias circuit is used by the bandgap during startup.
	 * This bit should be set after the bandgap has initialized.
	 */
	init_bandgap();

	if (!is_cpu_type(MXC_CPU_MX6UL) && !is_cpu_type(MXC_CPU_MX6ULL)) {
		/*
		 * When low freq boot is enabled, ROM will not set AHB
		 * freq, so we need to ensure AHB freq is 132MHz in such
		 * scenario.
		 */
		if (mxc_get_clock(MXC_ARM_CLK) == 396000000)
			set_ahb_rate(132000000);
	}

	if (is_cpu_type(MXC_CPU_MX6UL)) {
		if (is_soc_rev(CHIP_REV_1_0)) {
			/*
			 * According to the design team's requirement on i.MX6UL,
			 * the PMIC_STBY_REQ PAD should be configured as open
			 * drain 100K (0x0000b8a0).
			 */
			writel(0x0000b8a0, IOMUXC_BASE_ADDR + 0x29c);
		} else {
			/*
			 * From TO1.1, SNVS adds internal pull up control for POR_B,
			 * the register filed is GPBIT[1:0], after system boot up,
			 * it can be set to 2b'01 to disable internal pull up.
			 * It can save about 30uA power in SNVS mode.
			 */
			writel((readl(MX6UL_SNVS_LP_BASE_ADDR + 0x10) & (~0x1400)) | 0x400,
				MX6UL_SNVS_LP_BASE_ADDR + 0x10);
		}
	}

	if (is_cpu_type(MXC_CPU_MX6ULL)) {
		/*
		 * GPBIT[1:0] is suggested to set to 2'b11:
		 * 2'b00 : always PUP100K
		 * 2'b01 : PUP100K when PMIC_ON_REQ or SOC_NOT_FAIL
		 * 2'b10 : always disable PUP100K
		 * 2'b11 : PDN100K when SOC_FAIL, PUP100K when SOC_NOT_FAIL
		 * register offset is different from i.MX6UL, since
		 * i.MX6UL is fixed by ECO.
		 */
		writel(readl(MX6UL_SNVS_LP_BASE_ADDR) |
			0x3, MX6UL_SNVS_LP_BASE_ADDR);
	}

		/* Set perclk to source from OSC 24MHz */
#if defined(CONFIG_MX6SL)
	set_preclk_from_osc();
#endif

	if (is_cpu_type(MXC_CPU_MX6SX))
		set_uart_from_osc();

	imx_set_wdog_powerdown(false); /* Disable PDE bit of WMCR register */

	if (!is_cpu_type(MXC_CPU_MX6SL) && !is_cpu_type(MXC_CPU_MX6UL) &&
	    !is_cpu_type(MXC_CPU_MX6ULL))
		imx_set_pcie_phy_power_down();

	if (!is_mx6dqp() && !is_cpu_type(MXC_CPU_MX6UL) &&
	    !is_cpu_type(MXC_CPU_MX6ULL))
		imx_set_vddpu_power_down();

#ifdef CONFIG_APBH_DMA
	/* Start APBH DMA */
	mxs_dma_init();
#endif

	init_src();

	if (is_mx6dqp())
		writel(0x80000201, 0xbb0608);

	return 0;
}

#ifdef CONFIG_ENV_IS_IN_MMC
__weak int board_mmc_get_env_dev(int devno)
{
	return CONFIG_SYS_MMC_ENV_DEV;
}

static int mmc_get_boot_dev(void)
{
	struct src *src_regs = (struct src *)SRC_BASE_ADDR;
	u32 soc_sbmr = readl(&src_regs->sbmr1);
	u32 bootsel;
	int devno;

	/*
	 * Refer to
	 * "i.MX 6Dual/6Quad Applications Processor Reference Manual"
	 * Chapter "8.5.3.1 Expansion Device eFUSE Configuration"
	 * i.MX6SL/SX/UL has same layout.
	 */
	bootsel = (soc_sbmr & 0x000000FF) >> 6;

	/* No boot from sd/mmc */
	if (bootsel != 1)
		return -1;

	/* BOOT_CFG2[3] and BOOT_CFG2[4] */
	devno = (soc_sbmr & 0x00001800) >> 11;

	return devno;
}

int mmc_get_env_dev(void)
{
	int devno = mmc_get_boot_dev();

	/* If not boot from sd/mmc, use default value */
	if (devno < 0)
		return CONFIG_SYS_MMC_ENV_DEV;

	return board_mmc_get_env_dev(devno);
}

#ifdef CONFIG_SYS_MMC_ENV_PART
__weak int board_mmc_get_env_part(int devno)
{
	return CONFIG_SYS_MMC_ENV_PART;
}

uint mmc_get_env_part(struct mmc *mmc)
{
	int devno = mmc_get_boot_dev();

	/* If not boot from sd/mmc, use default value */
	if (devno < 0)
		return CONFIG_SYS_MMC_ENV_PART;

	return board_mmc_get_env_part(devno);
}
#endif
#endif

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

#if defined(CONFIG_FEC_MXC)
void imx_get_mac_from_fuse(int dev_id, unsigned char *mac)
{
	struct ocotp_regs *ocotp = (struct ocotp_regs *)OCOTP_BASE_ADDR;
	struct fuse_bank *bank = &ocotp->bank[4];
	struct fuse_bank4_regs *fuse =
			(struct fuse_bank4_regs *)bank->fuse_regs;

	if ((is_cpu_type(MXC_CPU_MX6SX) || is_cpu_type(MXC_CPU_MX6UL) ||
	    is_cpu_type(MXC_CPU_MX6ULL)) && dev_id == 1) {
		u32 value = readl(&fuse->mac_addr2);
		mac[0] = value >> 24 ;
		mac[1] = value >> 16 ;
		mac[2] = value >> 8 ;
		mac[3] = value ;

		value = readl(&fuse->mac_addr1);
		mac[4] = value >> 24 ;
		mac[5] = value >> 16 ;

	} else {
		u32 value = readl(&fuse->mac_addr1);
		mac[0] = (value >> 8);
		mac[1] = value ;

		value = readl(&fuse->mac_addr0);
		mac[2] = value >> 24 ;
		mac[3] = value >> 16 ;
		mac[4] = value >> 8 ;
		mac[5] = value ;
	}

}
#endif

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
	lcdif_power_down();
#endif
}

void s_init(void)
{
	struct anatop_regs *anatop = (struct anatop_regs *)ANATOP_BASE_ADDR;
	struct mxc_ccm_reg *ccm = (struct mxc_ccm_reg *)CCM_BASE_ADDR;
	u32 mask480;
	u32 mask528;
	u32 reg, periph1, periph2;

	if (is_cpu_type(MXC_CPU_MX6SX) || is_cpu_type(MXC_CPU_MX6UL) ||
	    is_cpu_type(MXC_CPU_MX6ULL))
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

#ifdef CONFIG_IMX_BOOTAUX
int arch_auxiliary_core_up(u32 core_id, u32 boot_private_data)
{
	struct src *src_reg;
	u32 stack, pc;

	if (!boot_private_data)
		return -EINVAL;

	stack = *(u32 *)boot_private_data;
	pc = *(u32 *)(boot_private_data + 4);

	/* Set the stack and pc to M4 bootROM */
	writel(stack, M4_BOOTROM_BASE_ADDR);
	writel(pc, M4_BOOTROM_BASE_ADDR + 4);

	/* Enable M4 */
	src_reg = (struct src *)SRC_BASE_ADDR;
	clrsetbits_le32(&src_reg->scr, SRC_SCR_M4C_NON_SCLR_RST_MASK,
			SRC_SCR_M4_ENABLE_MASK);

	return 0;
}

int arch_auxiliary_core_check_up(u32 core_id)
{
	struct src *src_reg = (struct src *)SRC_BASE_ADDR;
	unsigned val;

	val = readl(&src_reg->scr);

	if (val & SRC_SCR_M4C_NON_SCLR_RST_MASK)
		return 0;  /* assert in reset */

	return 1;
}
#endif

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

	if (!is_cpu_type(MXC_CPU_MX6DL) && !is_cpu_type(MXC_CPU_MX6SX))
		set_ldo_voltage(LDO_ARM, 975);
	else
		set_ldo_voltage(LDO_ARM, 1150);
}

int set_anatop_bypass(int wdog_reset_pin)
{
	struct anatop_regs *anatop = (struct anatop_regs *)ANATOP_BASE_ADDR;
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

