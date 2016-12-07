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

#ifdef CONFIG_FASTBOOT_LOCK
#define FSL_FASTBOOT_FB_DEV "mmc"
#ifndef CONFIG_EFI_PARTITION
#define CONFIG_ANDROID_FBMISC_PARTITION_MMC 10
#endif
#endif

#define FASTBOOT_ENCRYPT_LOCK

#define CONFIG_FSL_CAAM_KB
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

#define CONFIG_CMD_FS_GENERIC
#define CONFIG_CMD_EXT4

#ifdef CONFIG_AVB_SUPPORT
#define CONFIG_SUPPORT_EMMC_RPMB
/* fuse bank size in word */
/* infact 7D have no enough bits
 * set this size to 0 will disable
 * program/read FUSE */
#define CONFIG_AVB_FUSE_BANK_SIZEW 0
#define CONFIG_AVB_FUSE_BANK_START 0
#define CONFIG_AVB_FUSE_BANK_END 0
#endif

#endif

