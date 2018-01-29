/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
 */

#ifndef __ASM_ARCH_VIDEO_COMMON_H__
#define __ASM_ARCH_VIDEO_COMMON_H__
#include <linux/fb.h>
#include <video_fb.h>

struct video_mode_settings {
	uint32_t pixelclock;	/* horizontal resolution	*/
	uint16_t xres;		/* horizontal resolution	*/
	uint16_t yres;		/* vertical resolution		*/
	uint16_t hfp;		/* horizontal front porch	*/
	uint16_t hbp;		/* horizontal back porch	*/
	uint16_t vfp;		/* vertical front porch		*/
	uint16_t vbp;		/* vertical back porch		*/
	uint16_t hsync;		/* horizontal sync pulse width	*/
	uint16_t vsync;		/* vertical sync pulse width	*/
	bool hpol;		/* horizontal pulse polarity	*/
	bool vpol;		/* vertical pulse polarity	*/
};

#define	PS2KHZ(ps)	(1000000000UL / (ps))
struct video_mode_settings *imx8m_get_gmode(void);
GraphicDevice *imx8m_get_gd(void);
void imx8m_show_gmode(void);
void imx8m_create_color_bar(
	void *start_address,
	struct video_mode_settings *vms);
int imx8m_fb_init(
	struct fb_videomode const *mode,
	uint8_t disp,
	uint32_t pixfmt);
void imx8m_fb_disable(void);

#endif /* __ASM_ARCH_VIDEO_COMMON_H__ */
