/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2015-2016 Freescale Semiconductor, Inc.
 * Copyright 2020 NXP
 *
 */

/*
 * Configuration settings for the Freescale S32V234PCIE board.
 */

#ifndef __CONFIG_H
#define __CONFIG_H

/* HDMI configs */
#define CONFIG_FSL_DCU_SII9022A
#define CONFIG_SYS_I2C_MXC_I2C2		/* enable I2C bus 2 */
#define CONFIG_SYS_I2C_DVI_BUS_NUM	1
#define CONFIG_SYS_I2C_DVI_ADDR		0x39

/* Ethernet config */
#define CONFIG_FEC_XCV_TYPE     RGMII
#define CONFIG_FEC_MXC_PHYADDR  1

#ifdef CONFIG_CMD_PCI
#define CONFIG_PCIE_EP_MODE
#endif

#define FDT_FILE fsl-s32v234-pcie.dtb

/* we include this file here because it depends on the above definitions */
#include <configs/s32v234_common.h>

#endif
