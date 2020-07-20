/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2016 NXP
 *
 */

/*
 * Configuration settings for the Freescale S32V234 EVB board,
 * Schematics 29288.
 */

#ifndef __CONFIG_H
#define __CONFIG_H

/* The configurations of this board depend on the definitions in this file and
 * the ones in the header included at the end, configs/s32v234_common.h.
 */

#define FDT_FILE fsl-s32v234-evb.dtb

#define CONFIG_DDR_INIT_DELAY 100

/* we include this file here because it depends on the above definitions */
#include <configs/s32v234_common.h>

#endif
