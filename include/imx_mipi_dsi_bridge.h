/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __IMX_MIPI_DSIM_BRIDGE_H__
#define __IMX_MIPI_DSIM_BRIDGE_H__

#include <linux/fb.h>

/* video mode */
#define MIPI_DSI_MODE_VIDEO		BIT(0)
/* video burst mode */
#define MIPI_DSI_MODE_VIDEO_BURST	BIT(1)
/* video pulse mode */
#define MIPI_DSI_MODE_VIDEO_SYNC_PULSE	BIT(2)
/* enable auto vertical count mode */
#define MIPI_DSI_MODE_VIDEO_AUTO_VERT	BIT(3)
/* enable hsync-end packets in vsync-pulse and v-porch area */
#define MIPI_DSI_MODE_VIDEO_HSE		BIT(4)
/* disable hfront-porch area */
#define MIPI_DSI_MODE_VIDEO_HFP		BIT(5)
/* disable hback-porch area */
#define MIPI_DSI_MODE_VIDEO_HBP		BIT(6)
/* disable hsync-active area */
#define MIPI_DSI_MODE_VIDEO_HSA		BIT(7)
/* flush display FIFO on vsync pulse */
#define MIPI_DSI_MODE_VSYNC_FLUSH	BIT(8)
/* disable EoT packets in HS mode */
#define MIPI_DSI_MODE_EOT_PACKET	BIT(9)
/* device supports non-continuous clock behavior (DSI spec 5.6.1) */
#define MIPI_DSI_CLOCK_NON_CONTINUOUS	BIT(10)
/* transmit data in low power */
#define MIPI_DSI_MODE_LPM		BIT(11)

#define DSI_CMD_BUF_MAXSIZE         (128)

enum mipi_dsi_pixel_format {
	MIPI_DSI_FMT_RGB888,
	MIPI_DSI_FMT_RGB666,
	MIPI_DSI_FMT_RGB666_PACKED,
	MIPI_DSI_FMT_RGB565,
};

struct mipi_dsi_client_dev {
	unsigned int channel;
	unsigned int lanes;
	enum mipi_dsi_pixel_format format;
	unsigned long mode_flags;
	const char *name;
};

struct mipi_dsi_client_driver {
	int	(*dsi_client_setup)(struct mipi_dsi_client_dev *panel_dev);
	const char *name;
};

struct mipi_dsi_bridge_driver {
	int (*attach)(struct mipi_dsi_bridge_driver *bridge_driver, struct mipi_dsi_client_dev *dsi_dev);
	int (*enable)(struct mipi_dsi_bridge_driver *bridge_driver);
	int (*disable)(struct mipi_dsi_bridge_driver *bridge_driver);
	int (*mode_set)(struct mipi_dsi_bridge_driver *bridge_driver, struct fb_videomode *pvmode);
	int (*pkt_write)(struct mipi_dsi_bridge_driver *bridge_driver, u8 data_type, const u8 *buf, int len);
	int (*add_client_driver)(struct mipi_dsi_bridge_driver *bridge_driver, struct mipi_dsi_client_driver *client_driver);
	const char *name;
	void *driver_private;
};

static inline int mipi_dsi_pixel_format_to_bpp(enum mipi_dsi_pixel_format fmt)
{
	switch (fmt) {
	case MIPI_DSI_FMT_RGB888:
	case MIPI_DSI_FMT_RGB666:
		return 24;

	case MIPI_DSI_FMT_RGB666_PACKED:
		return 18;

	case MIPI_DSI_FMT_RGB565:
		return 16;
	}

	return -EINVAL;
}

int imx_mipi_dsi_bridge_attach(struct mipi_dsi_client_dev *dsi_dev);
int imx_mipi_dsi_bridge_mode_set(struct fb_videomode *pvmode);
int imx_mipi_dsi_bridge_enable(void);
int imx_mipi_dsi_bridge_disable(void);
int imx_mipi_dsi_bridge_pkt_write(u8 data_type, const u8 *buf, int len);
int imx_mipi_dsi_bridge_add_client_driver(struct mipi_dsi_client_driver *client_driver);
int imx_mipi_dsi_bridge_register_driver(struct mipi_dsi_bridge_driver *driver);

#endif
