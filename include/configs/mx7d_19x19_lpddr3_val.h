/*
 * Copyright (C) 2015 Freescale Semiconductor, Inc.
 *
 * Configuration settings for the Freescale i.MX7D 19x19 LPDDR3 ARM2 board.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __MX7D_19X19_LPDDR3_VAL_CONFIG_H
#define __MX7D_19X19_LPDDR3_VAL_CONFIG_H

#define CFG_SYS_FSL_USDHC_NUM	1
#define CFG_MMCROOT			"/dev/mmcblk0p2"  /* USDHC1 */

#ifdef CONFIG_TARGET_MX7D_19X19_LPDDR2_VAL
#define PHYS_SDRAM_SIZE			SZ_512M
#else
#define PHYS_SDRAM_SIZE			SZ_2G
#endif

#include "mx7d_val.h"

#endif
