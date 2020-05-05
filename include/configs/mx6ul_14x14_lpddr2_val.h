/*
 * Copyright (C) 2015 Freescale Semiconductor, Inc.
 *
 * Configuration settings for the Freescale i.MX6UL 14x14 LPDDR2 ARM2.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __MX6UL_14X14_LPDDR2_VAL_CONFIG_H
#define __MX6UL_14X14_LPDDR2_VAL_CONFIG_H

#ifdef CONFIG_MTD_NOR_FLASH
/*
 * Conflicts with SD1/SD2/VIDEO/ENET
 * ENET is keeped, since only RXER conflicts.
 * If removed ENET, we can not boot kernel, since sd1/sd2 is disabled
 * when support weimnor.
 */
#undef CONFIG_FSL_USDHC
#undef CONFIG_VIDEO
#endif

#define BOOTARGS_CMA_SIZE   "cma=96M "

#include "mx6ul_val.h"

#define PHYS_SDRAM_SIZE			SZ_256M


#ifdef CONFIG_DM_ETH
#define CONFIG_CMD_MII
#define CONFIG_FEC_MXC
#define CONFIG_FEC_ENET_DEV 1  /* The ENET1 has pin conflict with UART1 */

#if (CONFIG_FEC_ENET_DEV == 0)
#define IMX_FEC_BASE			ENET_BASE_ADDR
#define CONFIG_FEC_MXC_PHYADDR          0x2
#define CONFIG_FEC_XCV_TYPE             MII100
#define CONFIG_ETHPRIME			"eth0"
#elif (CONFIG_FEC_ENET_DEV == 1)
#define IMX_FEC_BASE			ENET2_BASE_ADDR
#define CONFIG_FEC_MXC_PHYADDR          0x1
#define CONFIG_FEC_XCV_TYPE             RMII
#define CONFIG_ETHPRIME			"eth1"
#endif

#define CONFIG_FEC_MXC_MDIO_BASE ENET2_BASE_ADDR
#endif

#endif
