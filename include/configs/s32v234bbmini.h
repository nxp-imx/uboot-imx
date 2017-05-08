/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2015-2016 Freescale Semiconductor, Inc.
 * (C) Copyright 2016-2017,2020 NXP
 */

/*
 * Configuration settings for the Freescale S32V234 mini Bluebox board
 */

#ifndef __CONFIG_H
#define __CONFIG_H

/* The configurations of this board depend on the definitions in this file and
 * the ones in the header included at the end, configs/s32v234evb_2016q4.h.
 */

#define	FDT_FILE s32v234-bbmini.dtb

#define CONFIG_PCIE_EP_MODE

#define CONFIG_BOARD_EXTRA_ENV_SETTINGS \
	"setphy=mii write 3 d 2; mii write 3 e 2; mii write 3 d 4002; " \
	"mii write 3 e 8000; mii write 3 0 8000;\0"

/* we include this file here because it depends on the above definitions */
#include <configs/s32v234evb_2016q4.h>

#endif
