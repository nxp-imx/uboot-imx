/*
 * Copyright (C) 2014 Freescale Semiconductor, Inc.
 *
 * Configuration settings for the Freescale i.MX6SX 17x17 ARM2 board.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __MX6SX_17X17_ARM2_CONFIG_H
#define __MX6SX_17X17_ARM2_CONFIG_H

#include "mx6sx_arm2.h"

#ifdef CONFIG_SYS_USE_SPINOR  /* Pin conflict between SPI-NOR and SD2 */
#define CONFIG_SYS_FSL_USDHC_NUM    2
#define CONFIG_SYS_MMC_ENV_DEV		0
#else
#define CONFIG_SYS_FSL_USDHC_NUM    3
#define CONFIG_SYS_MMC_ENV_DEV		1
#endif
#endif
