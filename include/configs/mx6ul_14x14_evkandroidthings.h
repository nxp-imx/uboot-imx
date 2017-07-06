
/*
 * Copyright (C) 2015-2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __MX6UL_14x14_EVKANDROIDTHINGS_H
#define __MX6UL_14x14_EVKANDROIDTHINGS_H
#include "mx_android_common.h"
/* For NAND we don't support lock/unlock */
#ifndef CONFIG_NAND_BOOT
#define CONFIG_FASTBOOT_LOCK
#define FSL_FASTBOOT_FB_DEV "mmc"
#endif

#define CONFIG_FSL_CAAM_KB
#define CONFIG_CMD_FSL_CAAM_KB
#define CONFIG_SHA1
#define CONFIG_SHA256

#define FASTBOOT_ENCRYPT_LOCK
#define CONFIG_FSL_BOOTCTL
#ifdef CONFIG_AVB_SUPPORT

#ifdef CONFIG_SYS_MALLOC_LEN
#undef CONFIG_SYS_MALLOC_LEN
#define CONFIG_SYS_MALLOC_LEN           (32 * SZ_1M)
#endif

#define CONFIG_SUPPORT_EMMC_RPMB
#define CONFIG_PARTITION_UUIDS
/* fuse bank size in word */
#define CONFIG_AVB_FUSE_BANK_SIZEW 8
#define CONFIG_AVB_FUSE_BANK_START 10
#define CONFIG_AVB_FUSE_BANK_END 15
#endif

#define CONFIG_CMD_FAT

#endif
