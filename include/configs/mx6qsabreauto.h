/*
 * Copyright (C) 2012-2013 Freescale Semiconductor, Inc.
 *
 * Configuration settings for the Freescale i.MX6Q SabreAuto board.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __MX6QSABREAUTO_CONFIG_H
#define __MX6QSABREAUTO_CONFIG_H

#define CONFIG_MACH_TYPE	3529
#define CONFIG_MXC_UART_BASE	UART4_BASE
#define CONFIG_CONSOLE_DEV		"ttymxc3"
#define CONFIG_MMCROOT			"/dev/mmcblk0p2"

/* USB Configs */
#define CONFIG_CMD_USB
#define CONFIG_USB_EHCI
#define CONFIG_USB_EHCI_MX6
#define CONFIG_USB_STORAGE
#define CONFIG_USB_HOST_ETHER
#define CONFIG_USB_ETHER_ASIX
#define CONFIG_MXC_USB_PORT	1
#define CONFIG_MXC_USB_PORTSC	(PORT_PTS_UTMI | PORT_PTS_PTW)
#define CONFIG_MXC_USB_FLAGS	0

#define CONFIG_SYS_USE_NAND

#include "mx6qsabre_common.h"
#include <asm/imx-common/gpio.h>

#define CONFIG_SYS_FSL_USDHC_NUM	2
#if defined(CONFIG_ENV_IS_IN_MMC)
#define CONFIG_SYS_MMC_ENV_DEV		0
#define CONFIG_SYS_MMC_ENV_PART		1	/* Boot partition 1 */
#endif

#ifdef CONFIG_SYS_USE_SPINOR
#define CONFIG_SF_DEFAULT_CS   (1|(IMX_GPIO_NR(3, 19)<<8))
#endif

#endif                         /* __MX6QSABREAUTO_CONFIG_H */
