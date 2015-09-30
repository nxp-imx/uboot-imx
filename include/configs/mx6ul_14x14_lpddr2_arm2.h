/*
 * Copyright (C) 2015 Freescale Semiconductor, Inc.
 *
 * Configuration settings for the Freescale i.MX6UL 14x14 LPDDR2 ARM2.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __MX6UL_14X14_LPDDR2_ARM2_CONFIG_H
#define __MX6UL_14X14_LPDDR2_ARM2_CONFIG_H

#define CONFIG_DEFAULT_FDT_FILE  "imx6ul-14x14-lpddr2-arm2.dtb"

#ifdef CONFIG_SYS_BOOT_QSPI
#define CONFIG_SYS_USE_QSPI
#define CONFIG_ENV_IS_IN_SPI_FLASH
#elif defined CONFIG_SYS_BOOT_SPINOR
#define CONFIG_SYS_USE_SPINOR
#define CONFIG_ENV_IS_IN_SPI_FLASH
#elif defined CONFIG_SYS_BOOT_EIMNOR
#define CONFIG_SYS_USE_EIMNOR
#define CONFIG_ENV_IS_IN_FLASH
#define CONFIG_SYS_FLASH_PROTECTION
#elif defined CONFIG_SYS_BOOT_NAND
#define CONFIG_SYS_USE_NAND
#define CONFIG_ENV_IS_IN_NAND
#else
#define CONFIG_ENV_IS_IN_MMC
#endif

#define CONFIG_FSL_USDHC
#define CONFIG_VIDEO
#ifdef CONFIG_SYS_BOOT_EIMNOR
/*
 * Conflicts with SD1/SD2/VIDEO/ENET
 * ENET is keeped, since only RXER conflicts.
 * If removed ENET, we can not boot kernel, since sd1/sd2 is disabled
 * when support weimnor.
 */
#undef CONFIG_FSL_USDHC
#undef CONFIG_VIDEO
#endif

#define CONFIG_BOOTARGS_CMA_SIZE   "cma=96M "

#include "mx6ul_arm2.h"

#define PHYS_SDRAM_SIZE			SZ_256M

#ifdef CONFIG_SYS_USE_SPINOR
#define CONFIG_CMD_SF
#define CONFIG_SPI_FLASH
#define CONFIG_SPI_FLASH_STMICRO
#define CONFIG_MXC_SPI
#define CONFIG_SF_DEFAULT_BUS  1
#define CONFIG_SF_DEFAULT_SPEED 20000000
#define CONFIG_SF_DEFAULT_MODE (SPI_MODE_0)
#define CONFIG_SF_DEFAULT_CS   0
#endif

#define CONFIG_CMD_NET
#ifdef CONFIG_CMD_NET
#define CONFIG_CMD_PING
#define CONFIG_CMD_DHCP
#define CONFIG_CMD_MII
#define CONFIG_FEC_MXC
#define CONFIG_MII
#define CONFIG_FEC_ENET_DEV 1  /* The ENET1 has pin conflict with UART1 */

#if (CONFIG_FEC_ENET_DEV == 0)
#define IMX_FEC_BASE			ENET_BASE_ADDR
#define CONFIG_FEC_MXC_PHYADDR          0x2
#define CONFIG_FEC_XCV_TYPE             MII100
#elif (CONFIG_FEC_ENET_DEV == 1)
#define IMX_FEC_BASE			ENET2_BASE_ADDR
#define CONFIG_FEC_MXC_PHYADDR          0x1
#define CONFIG_FEC_XCV_TYPE             RMII
#endif
#define CONFIG_ETHPRIME                 "FEC"

#define CONFIG_PHYLIB
#define CONFIG_PHY_MICREL
#define CONFIG_FEC_DMA_MINALIGN		64
#endif

#endif
