/*
 * Copyright (C) 2014-2015 Freescale Semiconductor, Inc.
 *
 * Configuration settings for the Freescale i.MX7D 19x19 DDR3 ARM2 board.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __MX7D_19X19_DDR3_VAL_CONFIG_H
#define __MX7D_19X19_DDR3_VAL_CONFIG_H

#define CFG_SYS_FSL_USDHC_NUM    3
#define CFG_MMCROOT			"/dev/mmcblk1p2"  /* USDHC2 */

#define PHYS_SDRAM_SIZE			SZ_1G

#include "mx7d_val.h"

#endif
