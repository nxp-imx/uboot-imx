/*
 * Copyright (C) 2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __MX7DSABRESDBRILLO_H
#define __MX7DSABRESDBRILLO_H


#define CONFIG_CMD_FAT
#define CONFIG_DOS_PARTITION

#define CONFIG_FASTBOOT_LOCK

#define FSL_FASTBOOT_FB_DEV "mmc"
#define FSL_FASTBOOT_DATA_PART_NUM CONFIG_ANDROID_DATA_PARTITION_MMC

#define CONFIG_SHA1
#define CONFIG_SHA256

#define CONFIG_SYS_BOOTM_LEN 0x1000000

#define CONFIG_CMD_READ

#ifdef CONFIG_SYS_MMC_ENV_DEV
#undef CONFIG_SYS_MMC_ENV_DEV
#define CONFIG_SYS_MMC_ENV_DEV		1   /* USDHC2 */
#endif

#ifdef CONFIG_SYS_MMC_ENV_PART
#undef CONFIG_SYS_MMC_ENV_PART
#define CONFIG_SYS_MMC_ENV_PART		1	/* boot0 area */
#endif

#endif

