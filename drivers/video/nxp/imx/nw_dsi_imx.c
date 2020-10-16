// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019 NXP
 *
 */

#include <common.h>
#include <clk.h>
#include <dm.h>
#include <dsi_host.h>
#include <mipi_dsi.h>
#include <panel.h>
#include <reset.h>
#include <video.h>
#include <video_bridge.h>
#include <video_link.h>
#include <asm/io.h>
#include <asm/arch/gpio.h>
#include <dm/device-internal.h>
#include <linux/iopoll.h>
#include <linux/err.h>
#include <power/regulator.h>
#include <regmap.h>
#include <syscon.h>

struct nw_dsi_imx_priv {
	struct mipi_dsi_device device;
	struct udevice *panel;
	struct udevice *dsi_host;
	unsigned int data_lanes;
};

static int nw_dsi_imx_attach(struct udevice *dev)
{
	struct nw_dsi_imx_priv *priv = dev_get_priv(dev);
	struct mipi_dsi_device *device = &priv->device;
	struct mipi_dsi_panel_plat *mplat;
	struct display_timing timings;
	int ret;

	priv->panel = video_link_get_next_device(dev);
	if (!priv->panel ||
		device_get_uclass_id(priv->panel) != UCLASS_PANEL) {
		dev_err(dev, "get panel device error\n");
		return -ENODEV;
	}

	mplat = dev_get_platdata(priv->panel);
	mplat->device = &priv->device;

	ret = video_link_get_display_timings(&timings);
	if (ret) {
		dev_err(dev, "decode display timing error %d\n", ret);
		return ret;
	}

	ret = uclass_get_device(UCLASS_DSI_HOST, 0, &priv->dsi_host);
	if (ret) {
		dev_err(dev, "No video dsi host detected %d\n", ret);
		return ret;
	}

	ret = dsi_host_init(priv->dsi_host, device, &timings,
			priv->data_lanes,
			NULL);
	if (ret) {
		dev_err(dev, "failed to initialize mipi dsi host\n");
		return ret;
	}

	return 0;
}

static int nw_dsi_imx_set_backlight(struct udevice *dev, int percent)
{
	struct nw_dsi_imx_priv *priv = dev_get_priv(dev);
	int ret;

	ret = panel_enable_backlight(priv->panel);
	if (ret) {
		dev_err(dev, "panel %s enable backlight error %d\n",
			priv->panel->name, ret);
		return ret;
	}

	ret = dsi_host_enable(priv->dsi_host);
	if (ret) {
		dev_err(dev, "failed to enable mipi dsi host\n");
		return ret;
	}

	return 0;
}

static int nw_dsi_imx_probe(struct udevice *dev)
{
	struct nw_dsi_imx_priv *priv = dev_get_priv(dev);
	struct mipi_dsi_device *device = &priv->device;
	int ret;

	device->dev = dev;

	ret = dev_read_u32(dev, "data-lanes-num", &priv->data_lanes);
	if (ret) {
		printf("fail to get data lanes property %d\n", ret);
		return -EINVAL;
	}

	return ret;
}

static int nw_dsi_imx_remove(struct udevice *dev)
{
	struct nw_dsi_imx_priv *priv = dev_get_priv(dev);
	int ret;

	if (priv->panel)
		device_remove(priv->panel, DM_REMOVE_NORMAL);

	ret = dsi_host_disable(priv->dsi_host);
	if (ret) {
		dev_err(dev, "failed to enable mipi dsi host\n");
		return ret;
	}

	return 0;
}

struct video_bridge_ops nw_dsi_imx_ops = {
	.attach = nw_dsi_imx_attach,
	.set_backlight = nw_dsi_imx_set_backlight,
};

static const struct udevice_id nw_dsi_imx_ids[] = {
	{ .compatible = "fsl,imx7ulp-mipi-dsi" },
	{ }
};

U_BOOT_DRIVER(nw_dsi_imx) = {
	.name				= "nw_dsi_imx",
	.id				= UCLASS_VIDEO_BRIDGE,
	.of_match			= nw_dsi_imx_ids,
	.bind				= dm_scan_fdt_dev,
	.remove 			= nw_dsi_imx_remove,
	.probe				= nw_dsi_imx_probe,
	.ops				= &nw_dsi_imx_ops,
	.priv_auto_alloc_size		= sizeof(struct nw_dsi_imx_priv),
};
