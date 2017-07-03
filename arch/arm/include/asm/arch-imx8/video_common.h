/*
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
 */

#ifndef __ASM_ARCH_VIDEO_COMMON_H__
#define __ASM_ARCH_VIDEO_COMMON_H__

#include <asm/imx-common/sci/sci.h>
#include <linux/fb.h>

#define	PS2KHZ(ps)	(1000000000UL / (ps))

int lvds2hdmi_setup(int i2c_bus);
int lvds_soc_setup(int lvds_id, sc_pm_clock_rate_t pixel_clock);
void lvds_configure(int lvds_id);
int display_controller_setup(sc_pm_clock_rate_t pixel_clock);
int imxdpuv1_fb_init(struct fb_videomode const *mode, uint8_t disp, uint32_t pixfmt);
void imxdpuv1_fb_disable(void);

#endif /* __ASM_ARCH_VIDEO_COMMON_H__ */
