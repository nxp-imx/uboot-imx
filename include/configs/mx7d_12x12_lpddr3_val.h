/*
 * Copyright (C) 2014-2015 Freescale Semiconductor, Inc.
 *
 * Configuration settings for the Freescale i.MX7D 12x12 LPDDR3 ARM2 board.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __MX7D_12X12_LPDDR3_VAL_CONFIG_H
#define __MX7D_12X12_LPDDR3_VAL_CONFIG_H

#define CFG_SYS_FSL_USDHC_NUM    3
#define CFG_MMCROOT			"/dev/mmcblk0p2"  /* USDHC1 */

#define PHYS_SDRAM_SIZE			SZ_2G

#include "mx7d_val.h"

#endif
