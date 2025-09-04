// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2025 NXP
 */

#include <asm/arch/sys_proto.h>
#include <asm/arch/clock.h>
#include <asm/arch/mu.h>
#include <asm/mach-imx/boot_mode.h>
#include <asm/sections.h>
#include <hang.h>
#include <init.h>
#include <spl.h>
#include <asm/mach-imx/ele_api.h>
#include <asm/gpio.h>
#include <linux/delay.h>
#include <../dts/imx94-clock.h>
#include "crrm.h"

DECLARE_GLOBAL_DATA_PTR;

int spl_board_boot_device(enum boot_device boot_dev_spl)
{
	switch (boot_dev_spl) {
	case SD1_BOOT:
	case MMC1_BOOT:
		return BOOT_DEVICE_MMC1;
	case SD2_BOOT:
	case MMC2_BOOT:
		return BOOT_DEVICE_MMC2;
	case USB_BOOT:
		return BOOT_DEVICE_BOARD;
	case QSPI_BOOT:
		return BOOT_DEVICE_SPI;
	default:
		return BOOT_DEVICE_NONE;
	}
}

#if !IS_ENABLED(CONFIG_IMX_CRRM)
static void xspi_nor_reset(void)
{
	int ret;
	u32 resp = 0;

	ret = ele_set_gmid(&resp);
	if (ret)
		printf("Fail to set GMID: %d, resp 0x%x\n", ret, resp);

	/* Set MTO to max */
	imx_clk_scmi_enable(IMX94_CLK_XSPI1, true);
	imx_clk_scmi_enable(IMX94_CLK_XSPI2, true);

	writel(0xffffffff, 0x42b90928);
	writel(0xffffffff, 0x42be0928);

	return;
}
#endif

void spl_board_init(void)
{
	puts("Normal Boot\n");
}

/* SCMI suport by default */
void board_init_f(ulong dummy)
{
	int ret;

	/* Clear the BSS. */
	memset(__bss_start, 0, __bss_end - __bss_start);

#ifdef CONFIG_SPL_RECOVER_DATA_SECTION
	if (IS_ENABLED(CONFIG_SPL_BUILD))
		spl_save_restore_data();
#endif

	timer_init();

	spl_early_init();

	/* Need enable SCMI drivers and ELE driver before enabling console */
	ret = imx9_probe_mu();
	if (ret)
		hang(); /* if MU not probed, nothing can output, just hang here */

	arch_cpu_init();

	board_early_init_f();

	preloader_console_init();

	debug("SOC: 0x%x\n", gd->arch.soc_rev);
	debug("LC: 0x%x\n", gd->arch.lifecycle);

	get_reset_reason(true, false);

	/* Will set ARM freq to max rate */
	clock_init_late();

	ret = ele_start_rng();
	if (ret)
		printf("Fail to start RNG: %d\n", ret);

#if IS_ENABLED(CONFIG_IMX_CRRM)
	ret = crrm_spl_init();
	if (ret) {
		printf("CRRM error, boot stop\n");
		hang();
	}
#else
	xspi_nor_reset();
#endif

	board_init_r(NULL, 0);
}

#ifdef CONFIG_ANDROID_SUPPORT
int board_get_emmc_id(void) {
	return 0;
}
#endif
