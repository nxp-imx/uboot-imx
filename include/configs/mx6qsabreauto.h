/*
 * Copyright (C) 2012-2016 Freescale Semiconductor, Inc.
 *
 * Configuration settings for the Freescale i.MX6Q SabreAuto board.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __MX6QSABREAUTO_CONFIG_H
#define __MX6QSABREAUTO_CONFIG_H

#define CONFIG_MACH_TYPE	3529
#define CONFIG_MXC_UART_BASE	UART4_BASE
#define CONFIG_CONSOLE_DEV		"ttymxc3"
#define CONFIG_MMCROOT			"/dev/mmcblk2p2"  /* SDHC3 */
#define PHYS_SDRAM_SIZE		(2u * 1024 * 1024 * 1024)
#ifdef CONFIG_MX6SOLO
#undef PHYS_SDRAM_SIZE
#define PHYS_SDRAM_SIZE		(1u * 1024 * 1024 * 1024)
#endif

/*Since the pin conflicts on EIM D18, disable the USB host if the NOR flash is enabled */
#if !defined(CONFIG_SYS_USE_SPINOR) && !defined(CONFIG_SYS_USE_EIMNOR)
/* USB Configs */
#define CONFIG_CMD_USB
#define CONFIG_USB_EHCI
#define CONFIG_USB_EHCI_MX6
#define CONFIG_USB_STORAGE
#define CONFIG_USB_HOST_ETHER
#define CONFIG_USB_ETHER_ASIX
#define CONFIG_USB_MAX_CONTROLLER_COUNT 2
#define CONFIG_EHCI_HCD_INIT_AFTER_RESET	/* For OTG port */
#define CONFIG_MXC_USB_PORTSC	(PORT_PTS_UTMI | PORT_PTS_PTW)
#define CONFIG_MXC_USB_FLAGS	0

#define CONFIG_PCA953X
#define CONFIG_SYS_I2C_PCA953X_WIDTH	{ {0x30, 8}, {0x32, 8}, {0x34, 8} }
#endif

#define CONFIG_SYS_USE_NAND

#include "mx6sabre_common.h"

#undef CONFIG_MFG_NAND_PARTITION
#ifdef CONFIG_SYS_BOOT_NAND
#define CONFIG_MFG_NAND_PARTITION "mtdparts=8000000.nor:1m(boot),-(rootfs)\\\\;gpmi-nand:64m(boot),16m(kernel),16m(dtb),-(rootfs) "
#else
#define CONFIG_MFG_NAND_PARTITION ""
#endif

#define CONFIG_SYS_FSL_USDHC_NUM	2
#define CONFIG_SYS_MMC_ENV_DEV		1  /* SDHC3 */
#define CONFIG_SYS_MMC_ENV_PART                0       /* user partition */

#ifdef CONFIG_SYS_USE_SPINOR
#define CONFIG_SF_DEFAULT_CS   1
#endif

#endif                         /* __MX6QSABREAUTO_CONFIG_H */
