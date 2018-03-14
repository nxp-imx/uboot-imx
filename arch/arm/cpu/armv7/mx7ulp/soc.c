/*
 * Copyright (C) 2016 Freescale Semiconductor, Inc.
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <asm/io.h>
#include <asm/arch/clock.h>
#include <asm/arch/imx-regs.h>
#include <asm/sections.h>
#include <asm/arch/sys_proto.h>
#include <asm/imx-common/hab.h>
#include <asm/imx-common/boot_mode.h>
#include <fdt_support.h>

static char *get_reset_cause(char *);

#if defined(CONFIG_SECURE_BOOT)
struct imx_sec_config_fuse_t const imx_sec_config_fuse = {
	.bank = 29,
	.word = 6,
};
#endif

#define ROM_VERSION_ADDR 0x80
u32 get_cpu_rev(void)
{
	/* Check the ROM version for cpu revision */
	uint32_t rom_version;
	rom_version = readl((void __iomem *)ROM_VERSION_ADDR);

	return (MXC_CPU_MX7ULP << 12) | (rom_version & 0xFF);
}

#ifdef CONFIG_REVISION_TAG
u32 __weak get_board_rev(void)
{
	return get_cpu_rev();
}
#endif

enum bt_mode get_boot_mode(void)
{
	u32 bt0_cfg = 0;

	bt0_cfg = readl(CMC0_RBASE + 0x40);
	bt0_cfg &= (BT0CFG_LPBOOT_MASK | BT0CFG_DUALBOOT_MASK);

	if (!(bt0_cfg & BT0CFG_LPBOOT_MASK)) {
		/* No low power boot */
		if (bt0_cfg & BT0CFG_DUALBOOT_MASK)
			return DUAL_BOOT;
		else
			return SINGLE_BOOT;
	}

	return LOW_POWER_BOOT;
}

#ifdef CONFIG_IMX_M4_BIND
char __firmware_image_start[0] __attribute__((section(".__firmware_image_start")));
char __firmware_image_end[0] __attribute__((section(".__firmware_image_end")));

int mcore_early_load_and_boot(void)
{
	u32 *src_addr = (u32 *)&__firmware_image_start;
	u32 *dest_addr = (u32 *)TCML_BASE; /*TCML*/
	u32 image_size = SZ_128K + SZ_64K; /* 192 KB*/
	u32 pc = 0, tag = 0;

	memcpy(dest_addr, src_addr, image_size);

	/* Set GP register to tell the M4 rom the image entry */
	/* We assume the M4 image has IVT head and padding which
	 * should be same as the one programmed into QSPI flash
	 */
	tag = *(dest_addr + 1024);
	if (tag != 0x402000d1 && tag !=0x412000d1)
		return -1;

	pc = *(dest_addr + 1025);

	writel(pc, SIM0_RBASE + 0x70); /*GP7*/

	return 0;
}
#endif

int arch_cpu_init(void)
{
#ifdef CONFIG_IMX_M4_BIND
	int ret;
	if (get_boot_mode() == SINGLE_BOOT) {
		ret = mcore_early_load_and_boot();
		if (ret)
			puts("Invalid M4 image, boot failed\n");
	}
#endif

	return 0;
}

#ifdef CONFIG_BOARD_POSTCLK_INIT
int board_postclk_init(void)
{
	return 0;
}
#endif

#define UNLOCK_WORD0 0xC520 /* 1st unlock word */
#define UNLOCK_WORD1 0xD928 /* 2nd unlock word */
#define REFRESH_WORD0 0xA602 /* 1st refresh word */
#define REFRESH_WORD1 0xB480 /* 2nd refresh word */

static void disable_wdog(u32 wdog_base)
{
	writel(UNLOCK_WORD0, (wdog_base + 0x04));
	writel(UNLOCK_WORD1, (wdog_base + 0x04));
	writel(0x0, (wdog_base + 0x0C)); /* Set WIN to 0 */
	writel(0x400, (wdog_base + 0x08)); /* Set timeout to default 0x400 */
	writel(0x120, (wdog_base + 0x00)); /* Disable it and set update */

	writel(REFRESH_WORD0, (wdog_base + 0x04)); /* Refresh the CNT */
	writel(REFRESH_WORD1, (wdog_base + 0x04));
}

void init_wdog(void)
{
	/*
	 * ROM will configure WDOG1, disable it or enable it
	 * depending on FUSE. The update bit is set for reconfigurable.
	 * We have to use unlock sequence to reconfigure it.
	 * WDOG2 is not touched by ROM, so it will have default value
	 * which is enabled. We can directly configure it.
	 * To simplify the codes, we still use same reconfigure
	 * process as WDOG1. Because the update bit is not set for
	 * WDOG2, the unlock sequence won't take effect really.
	 * It actually directly configure the wdog.
	 * In this function, we will disable both WDOG1 and WDOG2,
	 * and set update bit for both. So that kernel can reconfigure them.
	 */
	disable_wdog(WDG1_RBASE);
	disable_wdog(WDG2_RBASE);
}


void s_init(void)
{
	/* Disable wdog */
	init_wdog();

	/* clock configuration. */
	clock_init();

	if (soc_rev() < CHIP_REV_2_0) {
		/* enable dumb pmic */
		writel((readl(SNVS_LP_LPCR) | SNVS_LPCR_DPEN), SNVS_LP_LPCR);

#if defined(CONFIG_ANDROID_SUPPORT)
		/* Enable RTC */
		writel((readl(SNVS_LP_LPCR) | SNVS_LPCR_SRTC_ENV), SNVS_LP_LPCR);
#endif
	}
	return;
}

#ifndef CONFIG_ULP_WATCHDOG
void reset_cpu(ulong addr)
{
	setbits_le32(SIM0_RBASE, SIM_SOPT1_A7_SW_RESET);
	while (1)
		;
}
#endif

#if defined(CONFIG_DISPLAY_CPUINFO)
const char *get_imx_type(u32 imxtype)
{
	return "7ULP";
}

int print_cpuinfo(void)
{
	u32 cpurev;
	char cause[18];

	cpurev = get_cpu_rev();

	printf("CPU:   Freescale i.MX%s rev%d.%d at %d MHz\n",
	       get_imx_type((cpurev & 0xFF000) >> 12),
	       (cpurev & 0x000F0) >> 4, (cpurev & 0x0000F) >> 0,
	       mxc_get_clock(MXC_ARM_CLK) / 1000000);

	printf("Reset cause: %s\n", get_reset_cause(cause));

	printf("Boot mode: ");
	switch (get_boot_mode()) {
	case LOW_POWER_BOOT:
		printf("Low power boot\n");
		break;
	case DUAL_BOOT:
		printf("Dual boot\n");
		break;
	case SINGLE_BOOT:
	default:
		printf("Single boot\n");
#ifdef CONFIG_IMX_M4_BIND
		if (readl(SIM0_RBASE + 0x70))
			printf("M4 start at 0x%x\n", readl(SIM0_RBASE + 0x70));
#endif
		break;
	}

	return 0;
}
#endif

#define CMC_SRS_TAMPER                    (1 << 31)
#define CMC_SRS_SECURITY                  (1 << 30)
#define CMC_SRS_TZWDG                     (1 << 29)
#define CMC_SRS_JTAG_RST                  (1 << 28)
#define CMC_SRS_CORE1                     (1 << 16)
#define CMC_SRS_LOCKUP                    (1 << 15)
#define CMC_SRS_SW                        (1 << 14)
#define CMC_SRS_WDG                       (1 << 13)
#define CMC_SRS_PIN_RESET                 (1 << 8)
#define CMC_SRS_WARM                      (1 << 4)
#define CMC_SRS_HVD                       (1 << 3)
#define CMC_SRS_LVD                       (1 << 2)
#define CMC_SRS_POR                       (1 << 1)
#define CMC_SRS_WUP                       (1 << 0)

static u32 reset_cause = -1;

static char *get_reset_cause(char *ret)
{
	u32 cause1, cause = 0, srs = 0;
	u32 *reg_ssrs = (u32 *)(SRC_BASE_ADDR + 0x28);
	u32 *reg_srs = (u32 *)(SRC_BASE_ADDR + 0x20);

	if (!ret)
		return "null";

	srs = readl(reg_srs);
	cause1 = readl(reg_ssrs);
	writel(cause1, reg_ssrs);

	reset_cause = cause1;

	cause = cause1 & (CMC_SRS_POR | CMC_SRS_WUP | CMC_SRS_WARM);

	switch (cause) {
	case CMC_SRS_POR:
		sprintf(ret, "%s", "POR");
		break;
	case CMC_SRS_WUP:
		sprintf(ret, "%s", "WUP");
		break;
	case CMC_SRS_WARM:
		cause = cause1 & (CMC_SRS_WDG | CMC_SRS_SW |
			CMC_SRS_JTAG_RST);
		switch (cause) {
		case CMC_SRS_WDG:
			sprintf(ret, "%s", "WARM-WDG");
			break;
		case CMC_SRS_SW:
			sprintf(ret, "%s", "WARM-SW");
			break;
		case CMC_SRS_JTAG_RST:
			sprintf(ret, "%s", "WARM-JTAG");
			break;
		default:
			sprintf(ret, "%s", "WARM-UNKN");
			break;
		}
		break;
	default:
		sprintf(ret, "%s-%X", "UNKN", cause1);
		break;
	}

	debug("[%X] SRS[%X] %X - ", cause1, srs, srs^cause1);
	return ret;
}

void arch_preboot_os(void)
{
#if defined(CONFIG_VIDEO_MXS)
	lcdif_power_down();
#endif
	scg_disable_pll_pfd(SCG_APLL_PFD1_CLK);
	scg_disable_pll_pfd(SCG_APLL_PFD2_CLK);
	scg_disable_pll_pfd(SCG_APLL_PFD3_CLK);
}

#ifdef CONFIG_ENV_IS_IN_MMC
__weak int board_mmc_get_env_dev(int devno)
{
	return CONFIG_SYS_MMC_ENV_DEV;
}

int mmc_get_env_dev(void)
{
	int devno = 0;
	u32 bt1_cfg = 0;

	/* If not boot from sd/mmc, use default value */
	if (get_boot_mode() == LOW_POWER_BOOT)
		return CONFIG_SYS_MMC_ENV_DEV;

	bt1_cfg = readl(CMC1_RBASE + 0x40);
	devno = (bt1_cfg >> 9) & 0x7;

	return board_mmc_get_env_dev(devno);
}
#endif

#ifdef CONFIG_OF_SYSTEM_SETUP
int ft_system_setup(void *blob, bd_t *bd)
{
	if (get_boot_device() == USB_BOOT) {
		int rc;
		int nodeoff = fdt_path_offset(blob, "/ahb-bridge0@40000000/usdhc@40370000");
		if (nodeoff < 0)
			return 0; /* Not found, skip it */

		printf("Found usdhc0 node\n");
		if (fdt_get_property(blob, nodeoff, "vqmmc-supply", NULL) != NULL) {
			rc = fdt_delprop(blob, nodeoff, "vqmmc-supply");
			if (!rc) {
				printf("Removed vqmmc-supply property\n");

add:
				rc = fdt_setprop(blob, nodeoff, "no-1-8-v", NULL, 0);
				if (rc == -FDT_ERR_NOSPACE) {
					rc = fdt_increase_size(blob, 32);
					if (!rc)
						goto add;
				} else if (rc) {
					printf("Failed to add no-1-8-v property, %d\n", rc);
				} else {
					printf("Added no-1-8-v property\n");
				}
			} else {
				printf("Failed to remove vqmmc-supply property, %d\n", rc);
			}
		}
	}
	return 0;
}
#endif

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
	case BOOT_TYPE_USB:
		boot_dev = USB_BOOT;
		break;
	default:
		break;
	}

	return boot_dev;
}

#ifdef CONFIG_FSL_FASTBOOT
#ifdef CONFIG_SERIAL_TAG
void get_board_serial(struct tag_serialnr *serialnr)
{

	struct ocotp_regs *ocotp = (struct ocotp_regs *)OCOTP_BASE_ADDR;
	struct fuse_bank *bank = &ocotp->bank[1];
	struct fuse_bank1_regs *fuse =
		(struct fuse_bank1_regs *)bank->fuse_regs;
	serialnr->low = (fuse->cfg0 & 0xFFFF) + ((fuse->cfg1 & 0xFFFF) << 16);
	serialnr->high = (fuse->cfg2 & 0xFFFF) + ((fuse->cfg3 & 0xFFFF) << 16);
}
#endif /*CONFIG_SERIAL_TAG*/
#endif /*CONFIG_FSL_FASTBOOT*/
