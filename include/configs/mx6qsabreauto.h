/*
 * Copyright (C) 2012-2014 Freescale Semiconductor, Inc.
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

#define CONFIG_SYS_USE_NAND

#include "mx6qsabre_common.h"
#include <asm/imx-common/gpio.h>

/*Since the pin conflicts on EIM D18, disable the USB host if the NOR flash is enabled */
#if !defined(CONFIG_SYS_USE_SPINOR) && !defined(CONFIG_SYS_USE_EIMNOR)
/* USB Configs */
#define CONFIG_CMD_USB
#define CONFIG_USB_EHCI
#define CONFIG_USB_EHCI_MX6
#define CONFIG_USB_STORAGE
#define CONFIG_EHCI_HCD_INIT_AFTER_RESET
#define CONFIG_USB_HOST_ETHER
#define CONFIG_USB_ETHER_ASIX
#define CONFIG_MXC_USB_PORTSC  (PORT_PTS_UTMI | PORT_PTS_PTW)
#define CONFIG_MXC_USB_FLAGS   0
#define CONFIG_USB_MAX_CONTROLLER_COUNT 2 /* Enabled USB controller number */

/* Enable multiple I2C bus support*/
#define CONFIG_I2C_MULTI_BUS

/* MAX7310 configs*/
#define CONFIG_MAX7310_IOEXP
#define CONFIG_IOEXP_DEVICES_NUM 3
#define CONFIG_IOEXP_DEV_PINS_NUM 8
#endif

#define CONFIG_SYS_FSL_USDHC_NUM	2
#define CONFIG_SYS_MMC_ENV_DEV		0
#define CONFIG_SYS_MMC_ENV_PART		0	/* user partition */

#ifdef CONFIG_SYS_USE_SPINOR
#define CONFIG_SF_DEFAULT_CS   (1|(IMX_GPIO_NR(3, 19)<<8))
#endif

#endif                         /* __MX6QSABREAUTO_CONFIG_H */
