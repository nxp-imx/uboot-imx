// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019 NXP
 */

#ifndef __IMX_LCDIFV3_H__
#define __IMX_LCDIFV3_H__

#include <linux/fb.h>

struct display_panel {
	unsigned int reg_base;
	unsigned int width;
	unsigned int height;
	unsigned int gdfindex;
	unsigned int gdfbytespp;
};

void lcdifv3_get_panel(struct display_panel *panel);
void lcdifv3_power_down(void);
int lcdifv3_panel_setup(struct fb_videomode mode, int bpp,
	uint32_t base_addr);

#endif				/* __IMX_LCDIFV3_H__ */
