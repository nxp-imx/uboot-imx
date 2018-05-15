/*
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __MULTA_IMX7D_ANDROIDTHINGS_H
#define __MULTA_IMX7D_ANDROIDTHINGS_H
#define TRUSTY_OS_ENTRY 0x9e000000
#define TRUSTY_OS_RAM_SIZE 0x2000000
#define TEE_HWPARTITION_ID 2
#define TRUSTY_OS_MMC_BLKS 0xFFF

#ifdef CONFIG_IMX_TRUSTY_OS
#define NON_SECURE_FASTBOOT
#define TRUSTY_KEYSLOT_PACKAGE
#endif
#include "mx_android_common.h"

/* For NAND we don't support lock/unlock */
#ifndef CONFIG_NAND_BOOT
#define CONFIG_FASTBOOT_LOCK
#define CONFIG_ENABLE_LOCKSTATUS_SUPPORT
#define FSL_FASTBOOT_FB_DEV "mmc"
#endif

#define CONFIG_ANDROID_AB_SUPPORT
#define FASTBOOT_ENCRYPT_LOCK

#define CONFIG_FSL_CAAM_KB
#define CONFIG_SHA1
#define CONFIG_SHA256


#ifdef CONFIG_SYS_MMC_ENV_DEV
#undef CONFIG_SYS_MMC_ENV_DEV
#define CONFIG_SYS_MMC_ENV_DEV		1   /* USDHC2 */
#endif

#ifdef CONFIG_SYS_MMC_ENV_PART
#undef CONFIG_SYS_MMC_ENV_PART
#define CONFIG_SYS_MMC_ENV_PART		1	/* boot0 area */
#endif

#define CONFIG_SYSTEM_RAMDISK_SUPPORT



#define CONFIG_AVB_SUPPORT
#ifdef CONFIG_AVB_SUPPORT
#define CONFIG_SUPPORT_EMMC_RPMB
#ifdef CONFIG_SYS_MALLOC_LEN
#undef CONFIG_SYS_MALLOC_LEN
#define CONFIG_SYS_MALLOC_LEN	 	(32 * SZ_1M)
#endif
/* fuse bank size in word */
/* infact 7D have no enough bits
 * set this size to 0 will disable
 * program/read FUSE */
#define CONFIG_AVB_FUSE_BANK_SIZEW 0
#define CONFIG_AVB_FUSE_BANK_START 0
#define CONFIG_AVB_FUSE_BANK_END 0
#endif

#define AVB_AB_I_UNDERSTAND_LIBAVB_AB_IS_DEPRECATED

#endif
