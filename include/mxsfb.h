/*
 * Copyright (C) 2014 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __MXSFB_H__
#define __MXSFB_H__

#ifdef CONFIG_VIDEO_MXS
int mxs_lcd_panel_setup(struct fb_videomode mode, int bpp,
	uint32_t base_addr);
#endif

#endif				/* __MXSFB_H__ */
