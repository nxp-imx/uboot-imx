/*
 * Copyright (C) 2014 Freescale Semiconductor, Inc.
 *
 * Configuration settings for the Freescale i.MX6SX 17x17 ARM2 board.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __MX6SX_17X17_VAL_CONFIG_H
#define __MX6SX_17X17_VAL_CONFIG_H

#include "mx6sx_val.h"

#ifdef CONFIG_MXC_SPI  /* Pin conflict between SPI-NOR and SD2 */
#define CONFIG_SYS_FSL_USDHC_NUM    2
#define CONFIG_SYS_MMC_ENV_DEV		2   /* USDHC3 */
#define CONFIG_SYS_MMC_ENV_PART		0	/* user area */
#define CONFIG_MMCROOT			"/dev/mmcblk2p2"  /* USDHC3 */
#else
#define CONFIG_SYS_FSL_USDHC_NUM    3
#define CONFIG_SYS_MMC_ENV_DEV		2   /* USDHC3 */
#define CONFIG_SYS_MMC_ENV_PART		0	/* user area */
#define CONFIG_MMCROOT			"/dev/mmcblk2p2"  /* USDHC3 */
#endif

#ifdef CONFIG_MXC_SPI
#undef CONFIG_SYS_FLASH_SECT_SIZE
#undef CONFIG_SYS_MAX_FLASH_SECT
#define CONFIG_SYS_FLASH_SECT_SIZE	(256 * 1024)
#define CONFIG_SYS_MAX_FLASH_SECT 512   /* max number of sectors on one chip */
#define CONFIG_SYS_FLASH_PROTECTION
#endif

#endif
