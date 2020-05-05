/*
 * Copyright (C) 2014 Freescale Semiconductor, Inc.
 *
 * Configuration settings for the Freescale i.MX6SX 19x19 ARM2 board.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __MX6SX_19X19_VAL_CONFIG_H
#define __MX6SX_19X19_VAL_CONFIG_H

#ifdef CONFIG_VIDEO
#define CONFIG_VIDEO_GIS
#endif

#include "mx6sx_val.h"

#define CONFIG_SYS_FSL_USDHC_NUM    1
#define CONFIG_SYS_MMC_ENV_DEV		0   /* USDHC1 */
#define CONFIG_SYS_MMC_ENV_PART		0	/* user area */
#define CONFIG_MMCROOT			"/dev/mmcblk0p2"  /* USDHC1 */

#endif
