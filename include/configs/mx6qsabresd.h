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

#ifndef __MX6QSABRESD_CONFIG_H
#define __MX6QSABRESD_CONFIG_H

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

/*#define CONFIG_SPLASH_SCREEN*/
/*#define CONFIG_MXC_EPDC*/

/*
 * SPLASH SCREEN Configs
 */
#ifdef CONFIG_SPLASH_SCREEN
	/*
	 * Framebuffer and LCD
	 */
	#define CONFIG_CMD_BMP
	#define CONFIG_LCD
	#define CONFIG_FB_BASE				(CONFIG_SYS_TEXT_BASE + 0x300000)
	#define CONFIG_SYS_CONSOLE_IS_IN_ENV
#ifdef CONFIG_MXC_EPDC
	#undef LCD_TEST_PATTERN
	/* #define CONFIG_SPLASH_IS_IN_MMC			1 */
	#define LCD_BPP					LCD_MONOCHROME
	/* #define CONFIG_SPLASH_SCREEN_ALIGN		1 */

	#define CONFIG_WORKING_BUF_ADDR			(CONFIG_SYS_TEXT_BASE + 0x100000)
	#define CONFIG_WAVEFORM_BUF_ADDR		(CONFIG_SYS_TEXT_BASE + 0x200000)
	#define CONFIG_WAVEFORM_FILE_OFFSET		0x600000
	#define CONFIG_WAVEFORM_FILE_SIZE		0xF0A00
	#define CONFIG_WAVEFORM_FILE_IN_MMC

#ifdef CONFIG_SPLASH_IS_IN_MMC
	#define CONFIG_SPLASH_IMG_OFFSET		0x4c000
	#define CONFIG_SPLASH_IMG_SIZE			0x19000
#endif
#endif
#endif /* CONFIG_SPLASH_SCREEN */

#endif                         /* __MX6QSABRESD_CONFIG_H */
