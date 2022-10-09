// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2022 NXP
 */

#include <common.h>
#include <clk.h>
#include <dm.h>
#include <dm/device_compat.h>
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
#include <media_bus_format.h>

#define	DISPLAY_MUX_CTRL        0x60
#define	PARALLEL_DISP_FORMAT   0x700

enum imx93_pdf_format {
	RGB888_TO_RGB888 = 0x0,
	RGB888_TO_RGB666 = 0x1 << 8,
	RGB565_TO_RGB565 = 0x2 << 8,
};

struct imx93_pdf_priv {
	struct udevice *panel;
	struct display_timing adj;
	enum imx93_pdf_format format;
	void *__iomem addr;
};

static int imx93_pdf_attach(struct udevice *dev)
{
	struct imx93_pdf_priv *priv = dev_get_priv(dev);
	struct display_timing timings;
	int ret;

	priv->panel = video_link_get_next_device(dev);
	if (!priv->panel ||
		device_get_uclass_id(priv->panel) != UCLASS_PANEL) {
		dev_err(dev, "get panel device error\n");
		return -ENODEV;
	}

	ret = video_link_get_display_timings(&timings);
	if (ret) {
		dev_err(dev, "decode display timing error %d\n", ret);
		return ret;
	}

	priv->adj = timings;

	writel(priv->format, priv->addr + DISPLAY_MUX_CTRL);

	return 0;
}

static int imx93_pdf_check_timing(struct udevice *dev, struct display_timing *timing)
{
	struct imx93_pdf_priv *priv = dev_get_priv(dev);

	/* Ensure the bridge device attached to panel */
	if (!priv->panel) {
		dev_err(dev, "%s No panel device attached\n", __func__);
		return -ENOTCONN;
	}

	*timing = priv->adj;

	return 0;
}

static int imx93_pdf_probe(struct udevice *dev)
{
	struct imx93_pdf_priv *priv = dev_get_priv(dev);
	const char *fmt;
	u32 bus_format;
	int ret;

	priv->addr = (void __iomem *)dev_read_addr(dev_get_parent(dev));
	if ((fdt_addr_t)priv->addr == FDT_ADDR_T_NONE) {
		dev_err(dev, "not able to get addr\n");
		return -EINVAL;
	}

	ret = ofnode_read_string_index(dev_ofnode(dev), "fsl,interface-pix-fmt", 0, &fmt);
	if (!ret) {
		if (!strcmp(fmt, "rgb565"))
			bus_format = MEDIA_BUS_FMT_RGB565_1X16;
		else if (!strcmp(fmt, "rgb666"))
			bus_format = MEDIA_BUS_FMT_RGB666_1X18;
		else if (!strcmp(fmt, "rgb888"))
			bus_format = MEDIA_BUS_FMT_RGB888_1X24;

	}

	switch (bus_format) {
	case MEDIA_BUS_FMT_RGB565_1X16:
		priv->format = RGB565_TO_RGB565;
		break;
	case MEDIA_BUS_FMT_RGB666_1X18:
		priv->format = RGB888_TO_RGB666;
		break;
	case MEDIA_BUS_FMT_RGB888_1X24:
		priv->format = RGB888_TO_RGB888;
		break;
	default:
		dev_dbg(dev, "invalid bus format 0x%x\n", bus_format);
		return -EINVAL;
	}


	return 0;
}

static int imx93_pdf_remove(struct udevice *dev)
{
	struct imx93_pdf_priv *priv = dev_get_priv(dev);

	if (priv->panel)
		device_remove(priv->panel, DM_REMOVE_NORMAL);

	return 0;
}

static int imx93_pdf_set_backlight(struct udevice *dev, int percent)
{
	struct imx93_pdf_priv *priv = dev_get_priv(dev);
	int ret;

	ret = panel_enable_backlight(priv->panel);
	if (ret) {
		dev_err(dev, "panel %s enable backlight error %d\n", priv->panel->name, ret);
		return ret;
	}

	return 0;
}

struct video_bridge_ops imx93_pdf_ops = {
	.attach = imx93_pdf_attach,
	.check_timing = imx93_pdf_check_timing,
	.set_backlight = imx93_pdf_set_backlight,
};

static const struct udevice_id imx93_pdf_ids[] = {
	{ .compatible = "fsl,imx93-parallel-display-format" },
	{ }
};

U_BOOT_DRIVER(imx93_pdf_driver) = {
	.name				= "imx93_pdf_driver",
	.id				= UCLASS_VIDEO_BRIDGE,
	.of_match			= imx93_pdf_ids,
	.bind				= dm_scan_fdt_dev,
	.remove				= imx93_pdf_remove,
	.probe				= imx93_pdf_probe,
	.ops				= &imx93_pdf_ops,
	.priv_auto			= sizeof(struct imx93_pdf_priv),
};
