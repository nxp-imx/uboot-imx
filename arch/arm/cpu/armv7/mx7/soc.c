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
#include <dm.h>
#include <imx_thermal.h>
#include <mxsfb.h>
#ifdef CONFIG_MXC_RDC
#include <asm/imx-common/rdc-sema.h>
#include <asm/arch/imx-rdc.h>
#endif
#ifdef CONFIG_FSL_FASTBOOT
#ifdef CONFIG_ANDROID_RECOVERY
#include <recovery.h>
#endif
#endif

struct src *src_reg = (struct src *)SRC_BASE_ADDR;

#if defined(CONFIG_IMX_THERMAL)
static const struct imx_thermal_plat imx7_thermal_plat = {
	.regs = (void *)ANATOP_BASE_ADDR,
	.fuse_bank = 3,
	.fuse_word = 3,
};

U_BOOT_DEVICE(imx7_thermal) = {
	.name = "imx_thermal",
	.platdata = &imx7_thermal_plat,
};
#endif

#ifdef CONFIG_MXC_RDC
static rdc_peri_cfg_t const resources[] = {
    (RDC_PER_SIM1 | RDC_DOMAIN(0)),
    (RDC_PER_SIM2 | RDC_DOMAIN(0)),
    (RDC_PER_UART1 | RDC_DOMAIN(0)),
    (RDC_PER_UART2 | RDC_DOMAIN(0)),
    (RDC_PER_UART3 | RDC_DOMAIN(0)),
    (RDC_PER_UART4 | RDC_DOMAIN(0)),
    (RDC_PER_UART5 | RDC_DOMAIN(0)),
    (RDC_PER_UART6 | RDC_DOMAIN(0)),
    (RDC_PER_UART7 | RDC_DOMAIN(0)),
    (RDC_PER_SAI1 | RDC_DOMAIN(0)),
    (RDC_PER_SAI2 | RDC_DOMAIN(0)),
    (RDC_PER_SAI3 | RDC_DOMAIN(0)),
    (RDC_PER_WDOG1 | RDC_DOMAIN(0)),
    (RDC_PER_WDOG2 | RDC_DOMAIN(0)),
    (RDC_PER_WDOG3 | RDC_DOMAIN(0)),
    (RDC_PER_WDOG4 | RDC_DOMAIN(0)),
    (RDC_PER_GPT1 | RDC_DOMAIN(0)),
    (RDC_PER_GPT2 | RDC_DOMAIN(0)),
    (RDC_PER_GPT3 | RDC_DOMAIN(0)),
    (RDC_PER_GPT4 | RDC_DOMAIN(0)),
    (RDC_PER_PWM1 | RDC_DOMAIN(0)),
    (RDC_PER_PWM2 | RDC_DOMAIN(0)),
    (RDC_PER_PWM3 | RDC_DOMAIN(0)),
    (RDC_PER_PWM4 | RDC_DOMAIN(0)),
    (RDC_PER_ENET1 | RDC_DOMAIN(0)),
    (RDC_PER_ENET2 | RDC_DOMAIN(0)),
};

static void isolate_resource(void)
{
    /* At default, all resources are in domain 0 - 3. Here we setup 
     * some resources to domain 0 where M4 codes will move the M4
     * out of this domain. Then M4 is not able to access them any longer.
     * This is a workaround for ic issue. In current design, if any peripheral 
     * was assigned to both A7 and M4, it will receive ipg_stop or ipg_wait 
     * when any of the 2 platforms enter low power mode. So M4 sleep will cause
     * some peripherals fail to work at A core side.
     */
    imx_rdc_setup_peripherals(resources, ARRAY_SIZE(resources));
}
#endif

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

static void imx_enet_mdio_fixup(void)
{
	struct iomuxc_gpr_base_regs *gpr_regs =
		(struct iomuxc_gpr_base_regs *)IOMUXC_GPR_BASE_ADDR;

	/*
	 * The management data input/output (MDIO) requires open-drain,
	 * i.MX7D TO1.0 ENET MDIO pin has no open drain, but TO1.1 supports
	 * this feature. So to TO1.1, need to enable open drain by setting
	 * bits GPR0[8:7].
	 */

	if (is_soc_rev(CHIP_REV_1_1) >= 0) {
		setbits_le32(&gpr_regs->gpr[0],
			     IOMUXC_GPR_GPR0_ENET_MDIO_OPEN_DRAIN_MASK);
	}
}

static void set_epdc_qos(void)
{
#define REGS_QOS_BASE     QOSC_IPS_BASE_ADDR
#define REGS_QOS_EPDC     (QOSC_IPS_BASE_ADDR + 0x3400)
#define REGS_QOS_PXP0     (QOSC_IPS_BASE_ADDR + 0x2C00)
#define REGS_QOS_PXP1     (QOSC_IPS_BASE_ADDR + 0x3C00)

	writel(0, REGS_QOS_BASE);  /*  Disable clkgate & soft_reset */
	writel(0, REGS_QOS_BASE + 0x60);  /*  Enable all masters */
	writel(0, REGS_QOS_EPDC);   /*  Disable clkgate & soft_reset */
	writel(0, REGS_QOS_PXP0);   /*  Disable clkgate & soft_reset */
	writel(0, REGS_QOS_PXP1);   /*  Disable clkgate & soft_reset */

	writel(0x0f020722, REGS_QOS_EPDC + 0xd0);   /*  WR, init = 7 with red flag */
	writel(0x0f020722, REGS_QOS_EPDC + 0xe0);   /*  RD,  init = 7 with red flag */

	writel(1, REGS_QOS_PXP0);   /*  OT_CTRL_EN =1 */
	writel(1, REGS_QOS_PXP1);   /*  OT_CTRL_EN =1 */

	writel(0x0f020222, REGS_QOS_PXP0 + 0x50);   /*  WR,  init = 2 with red flag */
	writel(0x0f020222, REGS_QOS_PXP1 + 0x50);   /*  WR,  init = 2 with red flag */
	writel(0x0f020222, REGS_QOS_PXP0 + 0x60);   /*  rD,  init = 2 with red flag */
	writel(0x0f020222, REGS_QOS_PXP1 + 0x60);   /*  rD,  init = 2 with red flag */
	writel(0x0f020422, REGS_QOS_PXP0 + 0x70);   /*  tOTAL,  init = 4 with red flag */
	writel(0x0f020422, REGS_QOS_PXP1 + 0x70);   /*  TOTAL,  init = 4 with red flag */

	writel(0xe080, IOMUXC_GPR_BASE_ADDR + 0x0034); /* EPDC AW/AR CACHE ENABLE */
}

int arch_cpu_init(void)
{
	init_aips();

	/* Disable PDE bit of WMCR register */
	imx_set_wdog_powerdown(false);

	imx_set_pcie_phy_power_down();

	imx_enet_mdio_fixup();

#ifdef CONFIG_APBH_DMA
	/* Start APBH DMA */
	mxs_dma_init();
#endif

	set_epdc_qos();

#ifdef CONFIG_MXC_RDC
    isolate_resource();
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
	struct ocotp_regs *ocotp = (struct ocotp_regs *)OCOTP_BASE_ADDR;
	struct fuse_bank *bank = &ocotp->bank[0];
	struct fuse_bank0_regs *fuse =
		(struct fuse_bank0_regs *)bank->fuse_regs;

	serialnr->low = fuse->tester0;
	serialnr->high = fuse->tester1;
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
	u32 stack, pc;

	if (!boot_private_data)
		return 1;

	stack = *(u32 *)boot_private_data;
	pc = *(u32 *)(boot_private_data + 4);

	/* Set the stack and pc to M4 bootROM */
	writel(stack, M4_BOOTROM_BASE_ADDR);
	writel(pc, M4_BOOTROM_BASE_ADDR + 4);

	/* Enable M4 */
	setbits_le32(&src_reg->m4rcr, 0x00000008);
	clrbits_le32(&src_reg->m4rcr, 0x00000001);

	return 0;
}

int arch_auxiliary_core_check_up(u32 core_id)
{
	uint32_t val;

	val = readl(&src_reg->m4rcr);
	if (val & 0x00000001)
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

void set_wdog_reset(struct wdog_regs *wdog)
{
	u32 reg = readw(&wdog->wcr);
	/*
	 * Output WDOG_B signal to reset external pmic or POR_B decided by
	 * the board desgin. Without external reset, the peripherals/DDR/
	 * PMIC are not reset, that may cause system working abnormal.
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
	struct bootrom_sw_info **p =
		(struct bootrom_sw_info **)ROM_SW_INFO_ADDR;

	enum boot_device boot_dev = SD1_BOOT;
	u8 boot_type = (*p)->boot_dev_type;
	u8 boot_instance = (*p)->boot_dev_instance;

	switch (boot_type) {
	case BOOT_TYPE_SD:
		boot_dev = boot_instance + SD1_BOOT;
		break;
	case BOOT_TYPE_MMC:
		boot_dev = boot_instance + MMC1_BOOT;
		break;
	case BOOT_TYPE_NAND:
		boot_dev = NAND_BOOT;
		break;
	case BOOT_TYPE_QSPI:
		boot_dev = QSPI_BOOT;
		break;
	case BOOT_TYPE_WEIM:
		boot_dev = WEIM_NOR_BOOT;
		break;
	case BOOT_TYPE_SPINOR:
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

void reset_misc(void)
{
#ifdef CONFIG_VIDEO_MXS
	lcdif_power_down();
#endif
}

#ifdef CONFIG_FSL_FASTBOOT

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

void fastboot_enable_flag(void)
{
	setbits_le32(SNVS_BASE_ADDR + SNVS_LPGPR,
		ANDROID_FASTBOOT_BOOT);
}
#endif /*CONFIG_FSL_FASTBOOT*/
