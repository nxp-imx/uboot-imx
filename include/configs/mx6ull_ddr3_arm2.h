/*
 * Copyright (C) 2016 Freescale Semiconductor, Inc.
 *
 * Configuration settings for the Freescale i.MX6UL 14x14 DDR3 ARM2.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __MX6ULL_DDR3_ARM2_CONFIG_H
#define __MX6ULL_DDR3_ARM2_CONFIG_H

#define CONFIG_DEFAULT_FDT_FILE  "imx6ull-14x14-ddr3-arm2.dtb"

#ifdef CONFIG_SYS_BOOT_QSPI
#define CONFIG_SYS_USE_QSPI
#define CONFIG_ENV_IS_IN_SPI_FLASH
#elif defined CONFIG_SYS_BOOT_SPINOR
#define CONFIG_SYS_USE_SPINOR
#define CONFIG_ENV_IS_IN_SPI_FLASH
#elif defined CONFIG_SYS_BOOT_NAND
#define CONFIG_SYS_USE_NAND
#define CONFIG_ENV_IS_IN_NAND
#else
#ifndef CONFIG_MX6ULL_DDR3_ARM2_EMMC_REWORK
#define CONFIG_SYS_USE_QSPI
#endif
#define CONFIG_ENV_IS_IN_MMC
#endif

#define CONFIG_VIDEO
#define CONFIG_FSL_USDHC
#define CONFIG_BOOTARGS_CMA_SIZE   ""

#include "mx6ul_arm2.h"

#define CONFIG_IOMUX_LPSR

#define PHYS_SDRAM_SIZE			SZ_1G

/*
 * TSC pins conflict with I2C1 bus, so after TSC
 * hardware rework, need to disable i2c1 bus, also
 * need to disable PMIC and ldo bypass check.
 */
#ifdef CONFIG_MX6ULL_DDR3_ARM2_TSC_REWORK
#undef CONFIG_LDO_BYPASS_CHECK
#undef CONFIG_SYS_I2C_MXC
#undef CONFIG_SYS_I2C
#undef CONFIG_CMD_I2C
#undef CONFIG_POWER_PFUZE100_I2C_ADDR
#undef CONFIG_POWER_PFUZE100
#undef CONFIG_POWER_I2C
#undef CONFIG_POWER
#endif

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

#ifdef CONFIG_CMD_NET
#define CONFIG_CMD_PING
#define CONFIG_CMD_DHCP
#define CONFIG_CMD_MII
#define CONFIG_FEC_MXC
#define CONFIG_MII
#define CONFIG_FEC_ENET_DEV 1

#if (CONFIG_FEC_ENET_DEV == 0)
#define IMX_FEC_BASE			ENET_BASE_ADDR
#define CONFIG_FEC_MXC_PHYADDR          0x1
#define CONFIG_FEC_XCV_TYPE             RMII
#elif (CONFIG_FEC_ENET_DEV == 1)
#define IMX_FEC_BASE			ENET2_BASE_ADDR
#define CONFIG_FEC_MXC_PHYADDR          0x2
#define CONFIG_FEC_XCV_TYPE             MII100
#endif
#define CONFIG_ETHPRIME                 "FEC"

#define CONFIG_PHYLIB
#define CONFIG_PHY_MICREL
#define CONFIG_FEC_DMA_MINALIGN		64
#endif

#ifdef CONFIG_VIDEO
#define CONFIG_CFB_CONSOLE
#define CONFIG_VIDEO_MXS
#define CONFIG_VIDEO_LOGO
#define CONFIG_VIDEO_SW_CURSOR
#define CONFIG_VGA_AS_SINGLE_DEVICE
#define CONFIG_SYS_CONSOLE_IS_IN_ENV
#define CONFIG_SPLASH_SCREEN
#define CONFIG_SPLASH_SCREEN_ALIGN
#define CONFIG_CMD_BMP
#define CONFIG_BMP_16BPP
#define CONFIG_VIDEO_BMP_RLE8
#define CONFIG_VIDEO_BMP_LOGO
#define CONFIG_IMX_VIDEO_SKIP
#endif

/* #define CONFIG_SPLASH_SCREEN*/
/* #define CONFIG_MXC_EPDC*/

/*
 * SPLASH SCREEN Configs
 */
#if defined(CONFIG_SPLASH_SCREEN) && defined(CONFIG_MXC_EPDC)
/*
 * Framebuffer and LCD
 */
#define	CONFIG_CFB_CONSOLE
#define CONFIG_CMD_BMP
#define CONFIG_LCD
#define CONFIG_SYS_CONSOLE_IS_IN_ENV

#undef LCD_TEST_PATTERN
/* #define CONFIG_SPLASH_IS_IN_MMC			1 */
#define LCD_BPP					LCD_MONOCHROME
/* #define CONFIG_SPLASH_SCREEN_ALIGN		1 */

#define CONFIG_WAVEFORM_BUF_SIZE		0x400000
#endif

#endif
