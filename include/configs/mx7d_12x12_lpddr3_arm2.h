/*
 * Copyright (C) 2014-2015 Freescale Semiconductor, Inc.
 *
 * Configuration settings for the Freescale i.MX7D 12x12 LPDDR3 ARM2 board.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __MX7D_12X12_LPDDR3_ARM2_CONFIG_H
#define __MX7D_12X12_LPDDR3_ARM2_CONFIG_H

#define CONFIG_SYS_FSL_USDHC_NUM    3
#define CONFIG_SYS_MMC_ENV_DEV		0   /* USDHC1 */
#define CONFIG_SYS_MMC_ENV_PART		0	/* user area */
#define CONFIG_MMCROOT			"/dev/mmcblk0p2"  /* USDHC1 */

#define PHYS_SDRAM_SIZE			SZ_2G

#define CONFIG_CMD_MII
#define CONFIG_FEC_MXC
#define CONFIG_MII
#define CONFIG_FEC_XCV_TYPE             RGMII
#define CONFIG_ETHPRIME                 "FEC"
#define CONFIG_FEC_MXC_PHYADDR          1

#define CONFIG_PHYLIB
#define CONFIG_PHY_ATHEROS

/* ENET1 */
#define IMX_FEC_BASE			ENET_IPS_BASE_ADDR

#ifdef CONFIG_SYS_BOOT_QSPI
#define CONFIG_SYS_USE_QSPI
#define CONFIG_ENV_IS_IN_SPI_FLASH
#elif defined CONFIG_SYS_BOOT_SPINOR
#define CONFIG_SYS_USE_SPINOR
#define CONFIG_ENV_IS_IN_SPI_FLASH
#else
#define CONFIG_SYS_USE_QSPI   /* Enable the QSPI flash at default */
#define CONFIG_ENV_IS_IN_MMC
#endif

/* PMIC */
#define CONFIG_POWER
#define CONFIG_POWER_I2C
#define CONFIG_POWER_PFUZE3000
#define CONFIG_POWER_PFUZE3000_I2C_ADDR	0x08

#ifdef CONFIG_SYS_USE_SPINOR
#define CONFIG_CMD_SF
#define CONFIG_SPI_FLASH
#define CONFIG_SPI_FLASH_STMICRO
#define CONFIG_MXC_SPI
#define CONFIG_SF_DEFAULT_BUS  0
#define CONFIG_SF_DEFAULT_SPEED 20000000
#define CONFIG_SF_DEFAULT_MODE (SPI_MODE_0)
#define CONFIG_SF_DEFAULT_CS   0
#endif

#define CONFIG_VIDEO
/* #define CONFIG_SPLASH_SCREEN*/
/* #define CONFIG_MXC_EPDC*/

#define CONFIG_USE_PLUGIN
#include "mx7d_arm2.h"

#endif
