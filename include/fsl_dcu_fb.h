/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2014 Freescale Semiconductor, Inc.
 * Copyright 2018 NXP
 *
 * FSL DCU Framebuffer driver
 */

#ifndef __FSL_DCU_FB_H
#define __FSL_DCU_FB_H

#include <linux/fb.h>

int fsl_dcu_init(struct fb_info *fbinfo,
		 unsigned int xres,
		 unsigned int yres,
		 unsigned int pixel_format);

int fsl_dcu_fixedfb_setup(void *blob);

/* Prototypes for external board-specific functions */
int platform_dcu_init(struct fb_info *fbinfo,
		      unsigned int xres,
		      unsigned int yres,
		      const char *port,
		      struct fb_videomode *dcu_fb_videomode);
unsigned int dcu_set_pixel_clock(unsigned int pixclock);

#endif /* __FSL_DCU_FB_H */
