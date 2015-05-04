/*
 * Copyright (C) 2014-2015 Freescale Semiconductor, Inc.
 *
 * Configuration settings for the Freescale i.MX7.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */


#ifndef __MX7_COMMON_H
#define __MX7_COMMON_H

#define CONFIG_BOARD_POSTCLK_INIT
#define CONFIG_MXC_GPT_HCLK

#define CONFIG_SYSCOUNTER_TIMER
#define CONFIG_SC_TIMER_CLK 8000000 /* 8Mhz */

#define CONFIG_IOMUX_LPSR
#define CONFIG_IMX_FIXED_IVT_OFFSET
#endif
