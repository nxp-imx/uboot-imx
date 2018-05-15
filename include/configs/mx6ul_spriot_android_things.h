
/*
 * Copyright (C) 2015-2016 Freescale Semiconductor, Inc.
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __MX6UL_SPRIOT_ANDROID_THINGS_H
#define __MX6UL_SPRIOT_ANDROID_THINGS_H
#include "mx_android_common.h"

#ifdef CONFIG_AVB_ATX
#define PERMANENT_ATTRIBUTE_HASH_OFFSET 32
#endif

#define AVB_RPMB
#ifdef AVB_RPMB
#define KEYSLOT_BLKS 0xFFF
#define KEYSLOT_HWPARTITION_ID 2
#endif

/* For NAND we don't support lock/unlock */
#ifndef CONFIG_NAND_BOOT
#define CONFIG_FASTBOOT_LOCK
#define CONFIG_ENABLE_LOCKSTATUS_SUPPORT
#define FSL_FASTBOOT_FB_DEV "mmc"
#endif

#define CONFIG_ANDROID_AB_SUPPORT
#define CONFIG_FSL_CAAM_KB
#define CONFIG_CMD_FSL_CAAM_KB
#define CONFIG_SHA1
#define CONFIG_SHA256

#define CONFIG_AVB_SUPPORT
#define CONFIG_SYSTEM_RAMDISK_SUPPORT
#ifdef CONFIG_AVB_SUPPORT

#ifdef CONFIG_SYS_MALLOC_LEN
#undef CONFIG_SYS_MALLOC_LEN
#define CONFIG_SYS_MALLOC_LEN		(32 * SZ_1M)
#endif

#define CONFIG_SUPPORT_EMMC_RPMB
/* fuse bank size in word */
#define CONFIG_AVB_FUSE_BANK_SIZEW 8
#define CONFIG_AVB_FUSE_BANK_START 10
#define CONFIG_AVB_FUSE_BANK_END 15
#endif

#define AVB_AB_I_UNDERSTAND_LIBAVB_AB_IS_DEPRECATED

#endif
/* __MX6UL_SPRIOT_ANDROID_THINGS_H */
