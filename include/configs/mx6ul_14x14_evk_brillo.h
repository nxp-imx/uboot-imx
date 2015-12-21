
/*
 * Copyright (C) 2015 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __MX6UL_EVK_BRILLO_H
#define __MX6UL_EVK_BRILLO_H

#define CONFIG_ANDROID_BOOT_B_PARTITION_MMC 7
#define CONFIG_ANDROID_SYSTEM_B_PARTITION_MMC 8
#define CONFIG_ANDROID_MISC_PARTITION_MMC 9

#define CONFIG_CMD_FAT
#define CONFIG_DOS_PARTITION

#define CONFIG_SYS_BOOTM_LEN 0x1000000

#define CONFIG_CMD_READ

#endif
