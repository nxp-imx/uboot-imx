/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __IMX8_HDMI_H__
#define __IMX8_HDMI_H__

int imx8_hdmi_enable(int encoding, struct video_mode_settings *vms);
void imx8_hdmi_disable(void);

#endif /* __IMX8_HDMI_H__*/
