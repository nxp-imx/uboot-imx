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
#define CFG_SYS_FSL_USDHC_NUM    2
#define CFG_MMCROOT			"/dev/mmcblk2p2"  /* USDHC3 */
#else
#define CFG_SYS_FSL_USDHC_NUM    3
#define CFG_MMCROOT			"/dev/mmcblk2p2"  /* USDHC3 */
#endif

#endif
