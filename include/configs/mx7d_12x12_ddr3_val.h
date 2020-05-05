/*
 * Copyright (C) 2015 Freescale Semiconductor, Inc.
 * Copyright 2017 NXP
 *
 * Configuration settings for the Freescale i.MX7D 12x12 DDR3 ARM2 board.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __MX7D_12X12_DDR3_VAL_CONFIG_H
#define __MX7D_12X12_DDR3_VAL_CONFIG_H

#define CONFIG_SYS_FSL_USDHC_NUM	2
#define CONFIG_SYS_MMC_ENV_DEV		1	/* USDHC3 */
#define CONFIG_SYS_MMC_ENV_PART		0	/* user area */
#define CONFIG_MMCROOT			"/dev/mmcblk1p2"  /* USDHC3 */

#define PHYS_SDRAM_SIZE			SZ_1G


#include "mx7d_val.h"

#endif
