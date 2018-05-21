
/*
 * Copyright (C) 2015-2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __MX6UL_EVK_ANDROID_H
#define __MX6UL_EVK_ANDROID_H
#include "mx_android_common.h"

#if defined(CONFIG_FASTBOOT_STORAGE_NAND)
#define ANDROID_FASTBOOT_NAND_PARTS "16m@64m(boot) 16m@80m(recovery) 810m@96m(android_root)ubifs"
#endif

#endif
