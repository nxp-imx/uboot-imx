/*
 * Copyright (C) 2014-2015 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __MXSFB_H__
#define __MXSFB_H__

#include <linux/fb.h>

#ifdef CONFIG_VIDEO_MXS
struct display_panel {
	unsigned int reg_base;
	unsigned int width;
	unsigned int height;
	unsigned int gdfindex;
	unsigned int gdfbytespp;
};

void mxs_lcd_get_panel(struct display_panel *panel);
void lcdif_power_down(void);
int mxs_lcd_panel_setup(struct fb_videomode mode, int bpp,
	uint32_t base_addr);
#endif

#endif				/* __MXSFB_H__ */
