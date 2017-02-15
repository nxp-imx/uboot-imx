/*
 * Copyright (C) 2016 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */


#ifndef __MIPI_DSI_NORTHWEST_H
#define __MIPI_DSI_NORTHWEST_H

#include <linux/fb.h>

#define	DSI_CMD_BUF_MAXSIZE         (128)

/*
 * device structure for mipi-dsi based lcd panel.
 *
 * @name: name of the device to use with this device,
 *    the name will be used for binding driver.
 * @mode: video mode parameters for the panel.
 * @bpp:  bits per pixel. only 24 bits is supported.
 * @virtual_ch_id: virtual channel id for this dsi device.
 * @data_lane_num:  the data lane number, max is 2.
 * @host: pointer to the host driver instance, will be setup
 *    during register device.
 */
struct mipi_dsi_northwest_panel_device {
	const char			*name;
	struct fb_videomode mode;
	int				 bpp;
	u32				 virtual_ch_id;
	u32				 data_lane_num;

	struct mipi_dsi_northwest_info *host;
};


/*
 * driver structure for mipi-dsi based lcd panel.
 *
 * this structure should be registered by lcd panel driver.
 * mipi-dsi driver seeks lcd panel registered through name field
 * and calls these callback functions in appropriate time.
 *
 * @name: name of the driver to use with this device, or an
 *	alias for that name.
 * @mipi_panel_setup: callback pointer for initializing lcd panel based on mipi
 *	dsi interface.
 */
struct mipi_dsi_northwest_panel_driver {
	const char			*name;

	int	(*mipi_panel_setup)(struct mipi_dsi_northwest_panel_device *panel_dev);
};

/*
 * mipi-dsi northwest driver information structure, holds useful data for the driver.
 */
struct mipi_dsi_northwest_info {
	u32			mmio_base;
	u32			sim_base;
	int			enabled;
	struct mipi_dsi_northwest_panel_device *dsi_panel_dev;
	struct mipi_dsi_northwest_panel_driver *dsi_panel_drv;

	int (*mipi_dsi_pkt_write)(struct mipi_dsi_northwest_info *mipi_dsi,
			u8 data_type, const u32 *buf, int len);
};

/* Setup mipi dsi host driver instance, with base address and SIM address provided */
int mipi_dsi_northwest_setup(u32 base_addr, u32 sim_addr);

/* Create a LCD panel device, will search the panel driver to bind with them */
int mipi_dsi_northwest_register_panel_device(struct mipi_dsi_northwest_panel_device *panel_dev);

/* Register a LCD panel driver, will search the panel device to bind with them */
int mipi_dsi_northwest_register_panel_driver(struct mipi_dsi_northwest_panel_driver *panel_drv);

/* Enable the mipi dsi display */
int mipi_dsi_northwest_enable(void);

/* Disable and shutdown the mipi dsi display */
int mipi_dsi_northwest_shutdown(void);

void hx8363_init(void);

#endif
