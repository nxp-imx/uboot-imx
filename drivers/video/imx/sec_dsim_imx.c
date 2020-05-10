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

/* fixed phy ref clk rate */
#define PHY_REF_CLK		27000000

struct imx_sec_dsim_priv {
	struct mipi_dsi_device device;
	void __iomem *base;
	struct udevice *panel;
	struct udevice *dsi_host;
	struct reset_ctl_bulk soft_resetn;
	struct reset_ctl_bulk clk_enable;
	struct reset_ctl_bulk mipi_reset;
};

#if IS_ENABLED(CONFIG_DM_RESET)
static int sec_dsim_rstc_reset(struct reset_ctl_bulk *rstc, bool assert)
{
	int ret;

	if (!rstc)
		return 0;

	ret = assert ? reset_assert_bulk(rstc)	:
		       reset_deassert_bulk(rstc);

	return ret;
}

static int sec_dsim_of_parse_resets(struct udevice *dev)
{
	int ret;
	ofnode parent, child;
	struct ofnode_phandle_args args;
	struct reset_ctl_bulk rstc;
	const char *compat;
	uint32_t rstc_num = 0;

	struct imx_sec_dsim_priv *priv = dev_get_priv(dev);

	ret = dev_read_phandle_with_args(dev, "resets", "#reset-cells", 0,
					 0, &args);
	if (ret)
		return ret;

	parent = args.node;
	ofnode_for_each_subnode(child, parent) {
		compat = ofnode_get_property(child, "compatible", NULL);
		if (!compat)
			continue;

		ret = reset_get_bulk_nodev(child, &rstc);
		if (ret)
			continue;

		if (!of_compat_cmp("dsi,soft-resetn", compat, 0)) {
			priv->soft_resetn = rstc;
			rstc_num++;
		} else if (!of_compat_cmp("dsi,clk-enable", compat, 0)) {
			priv->clk_enable = rstc;
			rstc_num++;
		} else if (!of_compat_cmp("dsi,mipi-reset", compat, 0)) {
			priv->mipi_reset = rstc;
			rstc_num++;
		} else
			dev_warn(dev, "invalid dsim reset node: %s\n", compat);
	}

	if (!rstc_num) {
		dev_err(dev, "no invalid reset control exists\n");
		return -EINVAL;
	}

	return 0;
}
#endif

static int imx_sec_dsim_attach(struct udevice *dev)
{
	struct imx_sec_dsim_priv *priv = dev_get_priv(dev);
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

	ret = dsi_host_init(priv->dsi_host, device, &timings, 4,
			    NULL);
	if (ret) {
		dev_err(dev, "failed to initialize mipi dsi host\n");
		return ret;
	}

	return 0;
}

static int imx_sec_dsim_set_backlight(struct udevice *dev, int percent)
{
	struct imx_sec_dsim_priv *priv = dev_get_priv(dev);
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

static int imx_sec_dsim_probe(struct udevice *dev)
{
	struct imx_sec_dsim_priv *priv = dev_get_priv(dev);
	struct mipi_dsi_device *device = &priv->device;

	device->dev = dev;

#if IS_ENABLED(CONFIG_DM_RESET)
	int ret;
	/* Allow to not have resets */
	ret = sec_dsim_of_parse_resets(dev);
	if (!ret) {
		ret = sec_dsim_rstc_reset(&priv->soft_resetn, false);
		if (ret) {
			dev_err(dev, "deassert soft_resetn failed\n");
			return ret;
		}

		ret = sec_dsim_rstc_reset(&priv->clk_enable, true);
		if (ret) {
			dev_err(dev, "assert clk_enable failed\n");
			return ret;
		}

		ret = sec_dsim_rstc_reset(&priv->mipi_reset, false);
		if (ret) {
			dev_err(dev, "deassert mipi_reset failed\n");
			return ret;
		}
	}
#endif

	return 0;
}

static int imx_sec_dsim_remove(struct udevice *dev)
{
	struct imx_sec_dsim_priv *priv = dev_get_priv(dev);
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

struct video_bridge_ops imx_sec_dsim_ops = {
	.attach = imx_sec_dsim_attach,
	.set_backlight = imx_sec_dsim_set_backlight,
};

static const struct udevice_id imx_sec_dsim_ids[] = {
	{ .compatible = "fsl,imx8mm-mipi-dsim" },
	{ .compatible = "fsl,imx8mn-mipi-dsim" },
	{ .compatible = "fsl,imx8mp-mipi-dsim" },
	{ }
};

U_BOOT_DRIVER(imx_sec_dsim) = {
	.name				= "imx_sec_dsim",
	.id				= UCLASS_VIDEO_BRIDGE,
	.of_match			= imx_sec_dsim_ids,
	.bind				= dm_scan_fdt_dev,
	.remove 			= imx_sec_dsim_remove,
	.probe				= imx_sec_dsim_probe,
	.ops				= &imx_sec_dsim_ops,
	.priv_auto_alloc_size		= sizeof(struct imx_sec_dsim_priv),
};
