/*
 * Copyright (C) 2016 Freescale Semiconductor, Inc.
 * Copyright 2017-2018 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __IMX_VIDEO_H_
#define __IMX_VIDEO_H_

#include <linux/fb.h>
#if defined(CONFIG_VIDEO_IPUV3)
#include <ipu_pixfmt.h>
#elif defined(CONFIG_VIDEO_IMXDPUV1)
#include <imxdpuv1.h>
#include <asm/arch/video_common.h>
#elif defined(CONFIG_VIDEO_MXS)
#include <mxsfb.h>
#elif defined(CONFIG_VIDEO_IMXDCSS)
#include <asm/arch/video_common.h>
#endif

struct display_info_t {
	int	bus;
	int	addr;
	int	pixfmt;
	int	di;
	int	(*detect)(struct display_info_t const *dev);
	void	(*enable)(struct display_info_t const *dev);
	struct	fb_videomode mode;
};

#ifdef CONFIG_IMX_HDMI
extern int detect_hdmi(struct display_info_t const *dev);
#endif

#ifdef CONFIG_IMX_VIDEO_SKIP
extern struct display_info_t const displays[];
extern size_t display_count;
#endif

int ipu_set_ldb_clock(int rate);
#endif
