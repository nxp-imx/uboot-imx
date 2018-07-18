/*
 * Copyright (C) 2016 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */


#ifndef __MIPI_DSI_NORTHWEST_H
#define __MIPI_DSI_NORTHWEST_H

#include <linux/fb.h>

/* Setup mipi dsi host driver instance, with base address and SIM address provided */
int mipi_dsi_northwest_setup(u32 base_addr, u32 sim_addr);

#endif
