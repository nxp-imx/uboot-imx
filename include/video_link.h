/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2020 NXP
 */

#ifndef __VIDEO_LINK
#define __VIDEO_LINK

int video_link_init(void);

int video_link_shut_down(void);

struct udevice *video_link_get_next_device(struct udevice *curr_dev);

struct udevice *video_link_get_video_device(void);

int video_link_get_display_timings(struct display_timing *timings);

#endif
