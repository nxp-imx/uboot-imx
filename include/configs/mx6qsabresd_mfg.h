/*
 * Copyright (C) 2012-2013 Freescale Semiconductor, Inc.
 *
 * Configuration settings for the Freescale i.MX6Q SabreSD board.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.         See the
 * GNU General Public License for more details.
 */

#ifndef __MX6QSABRESD_MFG_CONFIG_H
#define __MX6QSABRESD_MFG_CONFIG_H

#define CONFIG_MACH_TYPE	3980
#define CONFIG_MXC_UART_BASE	UART1_BASE
#define CONFIG_CONSOLE_DEV		"ttymxc0"
#define CONFIG_MMCROOT			"/dev/mmcblk0p2"

#include "mx6qsabre_common.h"
#include <asm/imx-common/gpio.h>

#define CONFIG_SYS_FSL_USDHC_NUM	3
#define CONFIG_SYS_MMC_ENV_DEV		1	/* SDHC3 */
#define CONFIG_SYS_MMC_ENV_PART		0	/* user partition */

#ifdef CONFIG_SYS_USE_SPINOR
#define CONFIG_SF_DEFAULT_CS   (0|(IMX_GPIO_NR(4, 9)<<8))
#endif

#define CONFIG_MFG

#ifdef CONFIG_ENV_IS_IN_MMC
#undef CONFIG_ENV_IS_IN_MMC
#endif

#define CONFIG_ENV_IS_NOWHERE 1

#ifdef CONFIG_BOOTDELAY
#undef CONFIG_BOOTDELAY
#endif
#define CONFIG_BOOTDELAY 0

#ifdef CONFIG_BOOTARGS
#undef CONFIG_BOOTARGS
#endif
#define CONFIG_BOOTARGS "console=ttymxc0,115200 "\
			"rdinit=/linuxrc "\
			"enable_wait_mode=off"

#ifdef CONFIG_BOOTCOMMAND
#undef CONFIG_BOOTCOMMAND
#endif
#define CONFIG_BOOTCOMMAND      "bootm 0x12000000 0x12C00000 0x18000000"

#endif                         /* __MX6QSABRESD_CONFIG_H */
