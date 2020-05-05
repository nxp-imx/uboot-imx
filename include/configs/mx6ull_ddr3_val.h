/*
 * Copyright (C) 2016 Freescale Semiconductor, Inc.
 *
 * Configuration settings for the Freescale i.MX6UL 14x14 DDR3 ARM2.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __MX6ULL_DDR3_VAL_CONFIG_H
#define __MX6ULL_DDR3_VAL_CONFIG_H


#define BOOTARGS_CMA_SIZE   ""

#include "mx6ul_val.h"

#define CONFIG_IOMUX_LPSR

#define PHYS_SDRAM_SIZE			SZ_1G

/*
 * TSC pins conflict with I2C1 bus, so after TSC
 * hardware rework, need to disable i2c1 bus, also
 * need to disable PMIC and ldo bypass check.
 */
#ifdef CONFIG_MX6ULL_DDR3_VAL_TSC_REWORK
#undef CONFIG_LDO_BYPASS_CHECK
#undef CONFIG_SYS_I2C_MXC
#undef CONFIG_SYS_I2C
#undef CONFIG_CMD_I2C
#undef CONFIG_POWER_PFUZE100_I2C_ADDR
#undef CONFIG_POWER_PFUZE100
#undef CONFIG_POWER_I2C
#undef CONFIG_POWER
#endif

#ifdef CONFIG_DM_ETH
#define CONFIG_CMD_MII
#define CONFIG_FEC_MXC
#define CONFIG_FEC_ENET_DEV 1

#if (CONFIG_FEC_ENET_DEV == 0)
#define IMX_FEC_BASE			ENET_BASE_ADDR
#define CONFIG_FEC_MXC_PHYADDR          0x1
#define CONFIG_FEC_XCV_TYPE             RMII
#define CONFIG_ETHPRIME			"eth0"
#elif (CONFIG_FEC_ENET_DEV == 1)
#define IMX_FEC_BASE			ENET2_BASE_ADDR
#define CONFIG_FEC_MXC_PHYADDR          0x2
#define CONFIG_FEC_XCV_TYPE             MII100
#define CONFIG_ETHPRIME			"eth1"
#endif

#define CONFIG_FEC_MXC_MDIO_BASE ENET2_BASE_ADDR
#endif


/* #define CONFIG_SPLASH_SCREEN*/
/* #define CONFIG_MXC_EPDC*/

/*
 * SPLASH SCREEN Configs
 */
#if defined(CONFIG_MXC_EPDC)
/*
 * Framebuffer and LCD
 */
#define	CONFIG_SPLASH_SCREEN

#undef LCD_TEST_PATTERN
/* #define CONFIG_SPLASH_IS_IN_MMC			1 */
#define LCD_BPP					LCD_MONOCHROME
/* #define CONFIG_SPLASH_SCREEN_ALIGN		1 */

#define CONFIG_WAVEFORM_BUF_SIZE		0x400000
#endif

#define CONFIG_MODULE_FUSE
#define CONFIG_OF_SYSTEM_SETUP

#endif
