/*
 * Copyright (C) 2014 Freescale Semiconductor, Inc.
 *
 * Configuration settings for the Freescale i.MX6SX 17x17 ARM2 board.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __MX6SX_17X17_ARM2_CONFIG_H
#define __MX6SX_17X17_ARM2_CONFIG_H

#include "mx6sx_arm2.h"

#ifdef CONFIG_SYS_USE_SPINOR  /* Pin conflict between SPI-NOR and SD2 */
#define CONFIG_SYS_FSL_USDHC_NUM    2
#define CONFIG_SYS_MMC_ENV_DEV		0
#define CONFIG_SYS_MMC_ENV_PART		0	/* user area */
#else
#define CONFIG_SYS_FSL_USDHC_NUM    3
#define CONFIG_SYS_MMC_ENV_DEV		1
#define CONFIG_SYS_MMC_ENV_PART		0	/* user area */
#endif
#endif
