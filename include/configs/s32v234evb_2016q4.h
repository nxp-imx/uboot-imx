/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2015-2016 Freescale Semiconductor, Inc.
 * (C) Copyright 2016,2020 NXP
 */

/*
 * Configuration settings for the Freescale S32V234 boards,
 * 2016, quarter 4 versions.
 */

#ifndef __S32V234EVB_2016Q4_CONFIG_H
#define __S32V234EVB_2016Q4_CONFIG_H

/* The configurations of this board depend on the definitions in this file and
 * the ones in the header included at the end, configs/s32v234_common.h.
 */

#define CONFIG_DDR_INIT_DELAY 100

/* HDMI configs */
#define CONFIG_FSL_DCU_SII9022A
#define CONFIG_SYS_I2C_MXC_I2C2         /* enable I2C bus 2 */
#define CONFIG_SYS_I2C_DVI_BUS_NUM      1
#define CONFIG_SYS_I2C_DVI_ADDR         0x39

/* Ethernet config */
#define CONFIG_FEC_XCV_TYPE	RGMII
#define CONFIG_FEC_MXC_PHYADDR	3

/* we include this file here because it depends on the above definitions */
#include <configs/s32v234_common.h>

#endif
