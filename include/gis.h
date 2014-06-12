/*
 * Copyright (C) 2014 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef GIS_H
#define GIS_H

#define FMT_YUV444		0
#define FMT_YUYV		1
#define FMT_UYVY		2
#define FMT_RGB565		3
#define FMT_RGB888		4

void mxc_enable_gis(void);
void mxc_disable_gis(void);

#endif
