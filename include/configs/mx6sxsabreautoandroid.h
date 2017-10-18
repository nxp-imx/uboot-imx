
/*
 * Copyright (C) 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __MX6SX_SABREAUTO_ANDROID_H
#define __MX6SX_SABREAUTO_ANDROID_H
#include "mx_android_common.h"

#define CONFIG_FASTBOOT_LOCK
#define FSL_FASTBOOT_FB_DEV "mmc"
#define FASTBOOT_ENCRYPT_LOCK
#define CONFIG_FSL_CAAM_KB
#define CONFIG_CMD_FSL_CAAM_KB
#define CONFIG_SHA1
#define CONFIG_SHA256

#endif
