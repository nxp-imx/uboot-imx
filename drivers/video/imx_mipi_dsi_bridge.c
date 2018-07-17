/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <dm.h>
#include <asm/io.h>
#include <linux/errno.h>
#include <imx_mipi_dsi_bridge.h>

static struct mipi_dsi_bridge_driver *registered_driver = NULL;

int imx_mipi_dsi_bridge_attach(struct mipi_dsi_client_dev *dsi_dev)
{
	int ret = 0;

	if (!registered_driver)
		return -EPERM;

	if (registered_driver->attach)
		ret = registered_driver->attach(registered_driver, dsi_dev);

	return ret;
}

int imx_mipi_dsi_bridge_mode_set(struct fb_videomode *pvmode)
{
	int ret = 0;

	if (!registered_driver)
		return -EPERM;

	if (registered_driver->mode_set)
		ret = registered_driver->mode_set(registered_driver, pvmode);

	return ret;
}

int imx_mipi_dsi_bridge_enable(void)
{
	int ret = 0;

	if (!registered_driver)
		return -EPERM;

	if (registered_driver->enable)
		ret = registered_driver->enable(registered_driver);

	return ret;
}

int imx_mipi_dsi_bridge_disable(void)
{
	int ret = 0;

	if (!registered_driver)
		return -EPERM;

	if (registered_driver->disable)
		ret = registered_driver->disable(registered_driver);

	return ret;
}

int imx_mipi_dsi_bridge_pkt_write(u8 data_type, const u8 *buf, int len)
{
	int ret = 0;

	if (!registered_driver)
		return -EPERM;

	if (registered_driver->pkt_write)
		ret = registered_driver->pkt_write(registered_driver, data_type, buf, len);

	return ret;
}

int imx_mipi_dsi_bridge_add_client_driver(struct mipi_dsi_client_driver *client_driver)
{
	int ret = 0;

	if (!registered_driver)
		return -EPERM;

	if (registered_driver->add_client_driver)
		ret = registered_driver->add_client_driver(registered_driver, client_driver);

	return ret;
}

int imx_mipi_dsi_bridge_register_driver(struct mipi_dsi_bridge_driver *driver)
{
	if (!driver)
		return -EINVAL;

	if (registered_driver)
		return -EBUSY;

	registered_driver = driver;

	return 0;
}
