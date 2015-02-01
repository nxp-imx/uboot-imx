/*
 * Copyright (C) 2014-2015 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/armv7.h>
#include <asm/errno.h>
#include <asm/io.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/clock.h>
#include <asm/arch/sys_proto.h>
#include <asm/imx-common/boot_mode.h>
#include <asm/imx-common/dma.h>
#include <stdbool.h>
#include <asm/arch/crm_regs.h>
#ifdef CONFIG_FASTBOOT
#ifdef CONFIG_ANDROID_RECOVERY
#include <recovery.h>
#endif
#endif
#ifdef CONFIG_IMX_UDC
#include <asm/arch/mx7_usbphy.h>
#include <usb/imx_udc.h>
#endif

#define TEMPERATURE_MIN		-40
#define TEMPERATURE_HOT		80
#define TEMPERATURE_MAX		125
#define FACTOR1			15976
#define FACTOR2			4297157
#define MEASURE_FREQ		327

#define REG_VALUE_TO_CEL(ratio, raw) \
	((raw_n40c - raw) * 100 / ratio - 40)

static unsigned int fuse = ~0;

struct src *src_reg = (struct src *)SRC_BASE_ADDR;

u32 get_cpu_rev(void)
{
	struct mxc_ccm_anatop_reg *ccm_anatop = (struct mxc_ccm_anatop_reg *)
						 ANATOP_BASE_ADDR;
	u32 reg = readl(&ccm_anatop->digprog);
	u32 type = (reg >> 16) & 0xff;

	reg &= 0xff;
	return (type << 12) | reg;
}

#ifdef CONFIG_REVISION_TAG
u32 __weak get_board_rev(void)
{
	u32 cpurev = get_cpu_rev();
	u32 type = ((cpurev >> 12) & 0xff);

	if (type == MXC_CPU_MX7D)
		cpurev = (MXC_CPU_MX7D) << 12 | (cpurev & 0xFFF);

	return cpurev;
}
#endif

static int read_cpu_temperature(void)
{
	int temperature;
	unsigned int reg, tmp;
	unsigned int raw_25c, raw_n40c, ratio;
	struct mxc_ccm_anatop_reg *ccm_anatop = (struct mxc_ccm_anatop_reg *)
						 ANATOP_BASE_ADDR;
	struct ocotp_regs *ocotp = (struct ocotp_regs *)OCOTP_BASE_ADDR;
	struct fuse_bank *bank = &ocotp->bank[3];
	struct fuse_bank3_regs *fuse_bank3 =
			(struct fuse_bank3_regs *)bank->fuse_regs;

	enable_ocotp_clk(1);
	fuse = readl(&fuse_bank3->ana1);

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
	writel(TEMPMON_HW_ANADIG_TEMPSENSE1_POWER_DOWN_MASK, &ccm_anatop->tempsense0_clr);
	writel(PMU_REF_REFTOP_SELFBIASOFF_MASK, &ccm_anatop->ref_set);

	/* write measure freq */
	reg = readl(&ccm_anatop->tempsense1);
	reg &= ~TEMPMON_HW_ANADIG_TEMPSENSE1_MEASURE_FREQ_MASK;
	reg |= TEMPMON_HW_ANADIG_TEMPSENSE1_MEASURE_FREQ(MEASURE_FREQ);
	writel(reg, &ccm_anatop->tempsense1);

	writel(TEMPMON_HW_ANADIG_TEMPSENSE1_MEASURE_TEMP_MASK, &ccm_anatop->tempsense1_clr);
	writel(TEMPMON_HW_ANADIG_TEMPSENSE1_FINISHED_MASK, &ccm_anatop->tempsense1_clr);
	writel(TEMPMON_HW_ANADIG_TEMPSENSE1_MEASURE_TEMP_MASK, &ccm_anatop->tempsense1_set);

	while ((readl(&ccm_anatop->tempsense1) &
			TEMPMON_HW_ANADIG_TEMPSENSE1_FINISHED_MASK) == 0)
		udelay(10000);

	reg = readl(&ccm_anatop->tempsense1);
	tmp = (reg & TEMPMON_HW_ANADIG_TEMPSENSE1_TEMP_VALUE_MASK)
		>> TEMPMON_HW_ANADIG_TEMPSENSE1_TEMP_VALUE_SHIFT;
	writel(TEMPMON_HW_ANADIG_TEMPSENSE1_FINISHED_MASK, &ccm_anatop->tempsense1_clr);

	if (tmp <= raw_n40c)
		temperature = REG_VALUE_TO_CEL(ratio, tmp);
	else
		temperature = TEMPERATURE_MIN;
	/* power down anatop thermal sensor */
	writel(TEMPMON_HW_ANADIG_TEMPSENSE1_POWER_DOWN_MASK, &ccm_anatop->tempsense0_set);
	writel(PMU_REF_REFTOP_SELFBIASOFF_MASK, &ccm_anatop->ref_clr);

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

static void init_aips(void)
{
	struct aipstz_regs *aips1, *aips2, *aips3;

	aips1 = (struct aipstz_regs *)AIPS1_ON_BASE_ADDR;
	aips2 = (struct aipstz_regs *)AIPS2_ON_BASE_ADDR;
	aips3 = (struct aipstz_regs *)AIPS3_ON_BASE_ADDR;

	/*
	 * Set all MPROTx to be non-bufferable, trusted for R/W,
	 * not forced to user-mode.
	 */
	writel(0x77777777, &aips1->mprot0);
	writel(0x77777777, &aips1->mprot1);
	writel(0x77777777, &aips2->mprot0);
	writel(0x77777777, &aips2->mprot1);
	writel(0x77777777, &aips3->mprot0);
	writel(0x77777777, &aips3->mprot1);

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
	writel(0x00000000, &aips3->opacr0);
	writel(0x00000000, &aips3->opacr1);
	writel(0x00000000, &aips3->opacr2);
	writel(0x00000000, &aips3->opacr3);
	writel(0x00000000, &aips3->opacr4);
}

static void imx_set_pcie_phy_power_down(void)
{
	/* TODO */
}

static void imx_set_wdog_powerdown(bool enable)
{
	struct wdog_regs *wdog1 = (struct wdog_regs *)WDOG1_BASE_ADDR;
	struct wdog_regs *wdog2 = (struct wdog_regs *)WDOG2_BASE_ADDR;
	struct wdog_regs *wdog3 = (struct wdog_regs *)WDOG3_BASE_ADDR;
	struct wdog_regs *wdog4 = (struct wdog_regs *)WDOG4_BASE_ADDR;

	writew(enable, &wdog1->wmcr);
	writew(enable, &wdog2->wmcr);
	writew(enable, &wdog3->wmcr);
	writew(enable, &wdog4->wmcr);
}

int arch_cpu_init(void)
{
	init_aips();

	/* Disable PDE bit of WMCR register */
	imx_set_wdog_powerdown(false);

	imx_set_pcie_phy_power_down();

#ifdef CONFIG_APBH_DMA
	/* Start APBH DMA */
	mxs_dma_init();
#endif

	return 0;
}

#ifdef CONFIG_BOARD_POSTCLK_INIT
int board_postclk_init(void)
{
	/*
	 * We do not need to set LDO_SOC as i.mx6, since LDO_ARM and LDO_SOC
	 * does not exist. Check "Figure 7-9. i.MX7Dual Power Diagram"
	 */
	return 0;
}
#endif

#ifdef CONFIG_SERIAL_TAG
void get_board_serial(struct tag_serialnr *serialnr)
{
	/* TODO */
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
	struct fuse_bank *bank = &ocotp->bank[9];
	struct fuse_bank9_regs *fuse =
		(struct fuse_bank9_regs *)bank->fuse_regs;

	if (0 == dev_id) {
		u32 value = readl(&fuse->mac_addr1);
		mac[0] = (value >> 8);
		mac[1] = value;

		value = readl(&fuse->mac_addr0);
		mac[2] = value >> 24;
		mac[3] = value >> 16;
		mac[4] = value >> 8;
		mac[5] = value;
	} else {
		u32 value = readl(&fuse->mac_addr2);
		mac[0] = value >> 24;
		mac[1] = value >> 16;
		mac[2] = value >> 8;
		mac[3] = value;

		value = readl(&fuse->mac_addr1);
		mac[4] = value >> 24;
		mac[5] = value >> 16;
	}
}
#endif

int arch_auxiliary_core_up(u32 core_id, u32 boot_private_data)
{
	/* TODO */
	return 0;
}

int arch_auxiliary_core_check_up(u32 core_id)
{
	uint32_t val;

	val = readl(&src_reg->m4rcr);
	if (val & 0x1)
		return 0; /* assert in reset */

	return 1;
}

void boot_mode_apply(uint32_t cfg_val)
{
	uint32_t reg;
	writel(cfg_val, &src_reg->gpr9);
	reg = readl(&src_reg->gpr10);
	if (cfg_val)
		reg |= 1 << 28;
	else
		reg &= ~(1 << 28);
	writel(reg, &src_reg->gpr10);
}

/*
 * cfg_val will be used for
 * Boot_cfg4[7:0]:Boot_cfg3[7:0]:Boot_cfg2[7:0]:Boot_cfg1[7:0]
 * After reset, if GPR10[28] is 1, ROM will copy GPR9[25:0]
 * to SBMR1, which will determine the boot device.
 */
const struct boot_mode soc_boot_modes[] = {
	{"ecspi1:0",	MAKE_CFGVAL(0x00, 0x60, 0x00, 0x00)},
	{"ecspi1:1",	MAKE_CFGVAL(0x40, 0x62, 0x00, 0x00)},
	{"ecspi1:2",	MAKE_CFGVAL(0x80, 0x64, 0x00, 0x00)},
	{"ecspi1:3",	MAKE_CFGVAL(0xc0, 0x66, 0x00, 0x00)},

	{"weim",	MAKE_CFGVAL(0x00, 0x50, 0x00, 0x00)},
	{"qspi1",	MAKE_CFGVAL(0x10, 0x40, 0x00, 0x00)},
	/* 4 bit bus width */
	{"usdhc1",	MAKE_CFGVAL(0x10, 0x10, 0x00, 0x00)},
	{"usdhc2",	MAKE_CFGVAL(0x10, 0x14, 0x00, 0x00)},
	{"usdhc3",	MAKE_CFGVAL(0x10, 0x18, 0x00, 0x00)},
	{"mmc1",	MAKE_CFGVAL(0x10, 0x20, 0x00, 0x00)},
	{"mmc2",	MAKE_CFGVAL(0x10, 0x24, 0x00, 0x00)},
	{"mmc3",	MAKE_CFGVAL(0x10, 0x28, 0x00, 0x00)},
	{NULL,		0},
};

enum boot_device get_boot_device(void)
{
	enum boot_device boot_dev = UNKNOWN_BOOT;
	uint32_t soc_smbr = readl(&src_reg->sbmr1);
	uint32_t bt_mem_ctl = (soc_smbr & 0xF000) >> 12;
	uint32_t bt_dev_port = (soc_smbr & 0xC00) >> 10;
	uint32_t bt_mem_type = (soc_smbr & 0x800) >> 11;

	switch (bt_mem_ctl) {
	case 0x1:
		boot_dev = bt_dev_port + SD1_BOOT;
		break;
	case 0x2:
		boot_dev = bt_dev_port + MMC1_BOOT;
		break;
	case 0x3:
		boot_dev = NAND_BOOT;
		break;
	case 0x4:
		boot_dev = QSPI_BOOT;
		break;
	case 0x5:
		if (bt_mem_type)
			boot_dev = ONE_NAND_BOOT;
		else
			boot_dev = WEIM_NOR_BOOT;
		break;
	case 0x6:
		boot_dev = SPI_NOR_BOOT;
		break;
	default:
		break;
	}

	return boot_dev;
}

void s_init(void)
{
#if !defined CONFIG_SPL_BUILD
	/* Enable SMP mode for CPU0, by setting bit 6 of Auxiliary Ctl reg */
	asm volatile(
			"mrc p15, 0, r0, c1, c0, 1\n"
			"orr r0, r0, #1 << 6\n"
			"mcr p15, 0, r0, c1, c0, 1\n");
#endif
	/* clock configuration. */
	clock_init();

	return;
}

#ifdef CONFIG_FASTBOOT

#ifdef CONFIG_ANDROID_RECOVERY
#define ANDROID_RECOVERY_BOOT	(1 << 7)
/*
 * check if the recovery bit is set by kernel, it can be set by kernel
 * issue a command '# reboot recovery'
 */
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
/*
 * check if the recovery bit is set by kernel, it can be set by kernel
 * issue a command '# reboot fastboot'
 */
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
#endif /*CONFIG_FASTBOOT*/

#ifdef CONFIG_IMX_UDC
void set_usb_phy1_clk(void)
{
	/* TODO */
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
