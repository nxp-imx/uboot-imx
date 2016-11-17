
/*
 * Copyright (C) 2015-2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __MX6UL_EVK_BRILLO_H
#define __MX6UL_EVK_BRILLO_H


#define CONFIG_CMD_FAT
#define CONFIG_DOS_PARTITION

#define CONFIG_FASTBOOT_LOCK
#ifdef CONFIG_FASTBOOT_LOCK
#define FSL_FASTBOOT_FB_DEV "mmc"
#ifndef CONFIG_EFI_PARTITION
#define CONFIG_ANDROID_FBMISC_PARTITION_MMC 10
#endif
#endif

#define CONFIG_FSL_CAAM_KB
#define CONFIG_SHA1
#define CONFIG_SHA256

#define CONFIG_SYS_BOOTM_LEN 0x1000000

#define CONFIG_CMD_READ

#define FASTBOOT_ENCRYPT_LOCK

#define CONFIG_SUPPORT_EMMC_RPMB

#endif
