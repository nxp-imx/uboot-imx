
/*
 * Copyright (C) 2015-2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __PICOSOM_IMX6UL_ANDROID_THINGS_H
#define __PICOSOM_IMX6UL_ANDROID_THINGS_H
#include "mx_android_common.h"
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
#define CONFIG_AVB_FUSE
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


#endif
