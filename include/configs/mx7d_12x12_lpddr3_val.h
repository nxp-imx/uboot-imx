/*
 * Copyright (C) 2014-2015 Freescale Semiconductor, Inc.
 *
 * Configuration settings for the Freescale i.MX7D 12x12 LPDDR3 ARM2 board.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __MX7D_12X12_LPDDR3_VAL_CONFIG_H
#define __MX7D_12X12_LPDDR3_VAL_CONFIG_H

#define CONFIG_SYS_FSL_USDHC_NUM    3
#define CONFIG_SYS_MMC_ENV_DEV		0   /* USDHC1 */
#define CONFIG_SYS_MMC_ENV_PART		0	/* user area */
#define CONFIG_MMCROOT			"/dev/mmcblk0p2"  /* USDHC1 */

#define PHYS_SDRAM_SIZE			SZ_2G

#define CONFIG_FEC_MXC
#define CONFIG_FEC_XCV_TYPE             RGMII
#ifdef CONFIG_DM_ETH
#define CONFIG_ETHPRIME                 "eth0"
#else
#define CONFIG_ETHPRIME                 "FEC"
#endif
#define CONFIG_FEC_MXC_PHYADDR          1

#define CONFIG_PHY_ATHEROS

/* ENET1 */
#define IMX_FEC_BASE			ENET_IPS_BASE_ADDR

/* #define CONFIG_SPLASH_SCREEN*/
/* #define CONFIG_MXC_EPDC*/

#include "mx7d_val.h"

#endif
