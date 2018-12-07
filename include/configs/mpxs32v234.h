/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2015-2016 Freescale Semiconductor, Inc.
 * (C) Copyright 2017 MicroSys Electronics GmbH
 * Copyright 2018-2020 NXP
 */

/*
 * Configuration settings for the MicroSys MPXS32V234 board.
 */

#ifndef __MPXS32V234_H
#define __MPXS32V234_H

/* The configurations of this board depend on the definitions in this file and
 * the ones in the header included at the end, configs/s32v234_common.h
 */

#define CONFIG_MPXS32V234_R2

#define CONFIG_DDR_INIT_DELAY 100

/* Config DCU */
#define CONFIG_FSL_DCU_FB

/* HDMI TFP410P configuration */
#define CONFIG_SYS_HDMI_BUS_NUM		2
#define CONFIG_SYS_I2C_HDMI_ADDR	0x3F
#define CONFIG_SYS_HDMI_REG_CTL_1_MODE	0x8

#define TFP410P_CTL_1_MODE_RSVD		BIT(7)
#define TFP410P_CTL_1_MODE_TDIS		BIT(6)
#define TFP410P_CTL_1_MODE_VEN		BIT(5)
#define TFP410P_CTL_1_MODE_HEN		BIT(4)
#define TFP410P_CTL_1_MODE_DSEL		BIT(3)
#define TFP410P_CTL_1_MODE_BSEL		BIT(2)
#define TFP410P_CTL_1_MODE_EDGE		BIT(1)
#define TFP410P_CTL_1_MODE_PD		BIT(0)

/*
 * RTC configuration
 */
#define CONFIG_SYS_I2C_MXC_I2C2
#define CONFIG_SYS_I2C_MXC_I2C3

#define CONFIG_SYS_RTC_BUS_NUM          2
#define CONFIG_SYS_I2C_RTC_ADDR		0x51

#define CONFIG_RTC_PCF85263

/* Ethernet config */
#define CONFIG_FEC_XCV_TYPE	RGMII
#define CONFIG_FEC_MXC_PHYADDR  1

#define	FDT_FILE fsl-s32v234sbc.dtb

#define CONFIG_BOARD_EXTRA_ENV_SETTINGS "dvi_on=i2c dev 2; i2c mw 3f 8 35\0"

#define CONFIG_PCIE_EXT_CLOCK

/* we include this file here because it depends on the above definitions */
#include <configs/s32v234_common.h>

#endif
