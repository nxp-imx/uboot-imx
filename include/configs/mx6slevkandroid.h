
/*
 * Copyright (C) 2013-2016 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef MX6SL_EVK_ANDROID_H
#define MX6SL_EVK_ANDROID_H
#include "mx_android_common.h"

#include <asm/imx-common/mxc_key_defs.h>

/*keyboard mapping*/
#define CONFIG_VOL_DOWN_KEY     KEY_BACK
#define CONFIG_POWER_KEY        KEY_5

#define CONFIG_MXC_KPD
#define CONFIG_MXC_KEYMAPPING \
	{       \
		KEY_SELECT, KEY_BACK, KEY_1,     KEY_2, \
		KEY_3,      KEY_4,    KEY_5,     KEY_MENU, \
		KEY_6,      KEY_7,    KEY_8,     KEY_9, \
		KEY_UP,     KEY_LEFT, KEY_RIGHT, KEY_DOWN, \
	}
#define CONFIG_MXC_KPD_COLMAX 4
#define CONFIG_MXC_KPD_ROWMAX 4

#endif
