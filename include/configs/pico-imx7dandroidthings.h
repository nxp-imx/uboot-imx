/*
 * Copyright (C) 2016 Freescale Semiconductor, Inc.
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __PICO_IMX7DANDROIDTHINGS_H
#define __PICO_IMX7DANDROIDTHINGS_H

#define TRUSTY_OS_ENTRY 0x9e000000
#define TRUSTY_OS_RAM_SIZE 0x2000000
#define TEE_HWPARTITION_ID 2
#define TRUSTY_OS_MMC_BLKS 0xFFF
#define TRUSTY_OS_PADDED_SZ 0x180000

#ifdef CONFIG_AVB_ATX
#define PERMANENT_ATTRIBUTE_HASH_OFFSET 0
#endif

#define AVB_RPMB
#ifdef AVB_RPMB
#define KEYSLOT_BLKS 0xFFF
#define KEYSLOT_HWPARTITION_ID 2
#endif

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

#define CONFIG_FSL_CAAM_KB
#define CONFIG_SHA1
#define CONFIG_SHA256

#define CONFIG_SYSTEM_RAMDISK_SUPPORT
#define CONFIG_AVB_SUPPORT

#ifdef CONFIG_SYS_MMC_ENV_DEV
#undef CONFIG_SYS_MMC_ENV_DEV
#define CONFIG_SYS_MMC_ENV_DEV		1   /* USDHC2 */
#endif

#ifdef CONFIG_SYS_MMC_ENV_PART
#undef CONFIG_SYS_MMC_ENV_PART
#define CONFIG_SYS_MMC_ENV_PART		1	/* boot0 area */
#endif


#ifdef CONFIG_AVB_SUPPORT
#define CONFIG_SUPPORT_EMMC_RPMB

#ifdef CONFIG_SYS_MALLOC_LEN
#undef CONFIG_SYS_MALLOC_LEN
#define CONFIG_SYS_MALLOC_LEN	 	(64 * SZ_1M)
#endif
/* fuse bank size in word */
/* infact 7D have no enough bits
 * set this size to 0 will disable
 * program/read FUSE */
#define CONFIG_AVB_FUSE_BANK_SIZEW 4
#define CONFIG_AVB_FUSE_BANK_START 14
#define CONFIG_AVB_FUSE_BANK_END 14
#endif

#define AVB_AB_I_UNDERSTAND_LIBAVB_AB_IS_DEPRECATED

/* Disable U-Boot logo */
#undef CONFIG_VIDEO_LOGO

#endif
/* __PICO_IMX7DANDROIDTHINGS_H */
