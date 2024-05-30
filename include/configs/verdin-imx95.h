/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2023-2024 NXP
 */

#ifndef __IMX95_VERDIN_H
#define __IMX95_VERDIN_H

#include <linux/sizes.h>
#include <linux/stringify.h>
#include <asm/arch/imx-regs.h>

#define CFG_SYS_UBOOT_BASE	\
	(QSPI0_AMBA_BASE + CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR * 512)

/* Link Definitions */

#define CFG_SYS_INIT_RAM_ADDR        0x90000000
#define CFG_SYS_INIT_RAM_SIZE        0x200000

#define CFG_SYS_SDRAM_BASE           0x90000000
#define PHYS_SDRAM                      0x90000000
/* Totally 16GB */
#define PHYS_SDRAM_SIZE			0x70000000 /* 2GB  - 256MB DDR */
#define PHYS_SDRAM_2_SIZE 		0x380000000 /* 14GB */

#define CFG_SYS_SECURE_SDRAM_BASE	0x8A000000 /* Secure DDR region for A55, SPL could use first 2MB */
#define CFG_SYS_SECURE_SDRAM_SIZE	0x06000000

/* Using ULP WDOG for reset */
#define WDOG_BASE_ADDR          WDG3_BASE_ADDR

#if defined(CONFIG_ANDROID_SUPPORT)
#include "imx95_verdin_android.h"
#elif defined (CONFIG_ANDROID_AUTO_SUPPORT)
#include "imx95_verdin_android_auto.h"
#endif

#endif
