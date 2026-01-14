// SPDX-License-Identifier: GPL-2.0+

/*
 * Copyright 2023-2026 NXP
 */

#include <clk.h>
#include <dm.h>
#include <dm/device_compat.h>
#include <video.h>
#include <video_bridge.h>
#include <video_link.h>
#include <asm/io.h>
#include <dm/device-internal.h>
#include <linux/iopoll.h>
#include <linux/err.h>
#include <linux/bitfield.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <regmap.h>
#include <syscon.h>

#define  DISP_IN_SEL		BIT(1)
#define  MODE			BIT(0)

#define CTRL(n)			(0x0 + 0x10000 * (n))
#define  YUV_CONV		BIT(13)
#define  PIXEL_FORMAT		BIT(12)
#define  DE_POLARITY		BIT(11)
#define  VSYNC_POLARITY		BIT(10)
#define  HSYNC_POLARITY		BIT(9)

#define SWRST(n)		(0x20 + 0x10000 * (n))
#define  SW_RST			BIT(1)

#define IE(n)			(0x30 + 0x10000 * (n))
#define IS(n)			(0x40 + 0x10000 * (n))
#define  FOVF(n)		BIT(0 + 1 * (n))

#define ICTRL(n)		(0x50 + 0x10000 * (n))
#define  WIDTH_MASK		GENMASK(11, 0)
#define  WIDTH(n)		FIELD_PREP(WIDTH_MASK, (n))

#define STREAMS			2
#define NO_INTER_STREAM		2

enum imx95_pinter_mode {
	BYPASS,
	STREAM0_SPLIT2,
	STREAM1_SPLIT2,
};

struct imx95_pinter {
	void __iomem *regs;
	struct regmap *regmap;
	struct clk clk_bus;
	enum imx95_pinter_mode mode;
	unsigned int ctrl_reg;

	struct display_timing adj;

	unsigned int sid;	/* stream id */
	struct udevice *pl_dev;
};

struct imx95_pinter_devdata {
	unsigned int ctrl_reg;
};

static void imx95_pinter_sw_reset(struct imx95_pinter *pinter)
{
	writel(SW_RST, pinter->regs + SWRST(pinter->sid));
	udelay(20);
	writel(0, pinter->regs + SWRST(pinter->sid));
}

static int imx95_pinter_bridge_attach(struct udevice *dev)
{
	struct imx95_pinter *pinter = dev_get_priv(dev);
	struct display_timing timings;
	int ret;

	debug("%s\n", __func__);

	pinter->pl_dev = video_link_get_next_device(dev);
	if (!pinter->pl_dev ||
		device_get_uclass_id(pinter->pl_dev) != UCLASS_VIDEO_BRIDGE) {
		dev_err(dev, "get pixel link device error\n");
		return -ENODEV;
	}

	ret = video_link_get_display_timings(&timings);
	if (ret) {
		dev_err(dev, "decode display timing error %d\n", ret);
		return ret;
	}

	pinter->adj = timings;

	ret = video_bridge_attach(pinter->pl_dev);
	if (ret) {
		dev_err(dev, "fail to attach pixel link device\n");
		return ret;
	}

	return 0;
}

static int imx95_pinter_check_timing(struct udevice *dev, struct display_timing *timing)
{
	struct imx95_pinter *pinter = dev_get_priv(dev);

	/* Ensure the bridge device attached with pixel link */
	if (!pinter->pl_dev) {
		dev_err(dev, "%s No pixel link device attached\n", __func__);
		return -ENOTCONN;
	}

	*timing = pinter->adj;

	return 0;
}

static int imx95_pinter_set_backlight(struct udevice *dev, int percent)
{
	struct imx95_pinter *pinter = dev_get_priv(dev);
	int ret;
	u32 val;

	switch (pinter->mode) {
	case BYPASS:
		regmap_write(pinter->regmap, pinter->ctrl_reg, 0);
		break;
	case STREAM0_SPLIT2:
		regmap_write(pinter->regmap, pinter->ctrl_reg, MODE);
		break;
	case STREAM1_SPLIT2:
		regmap_write(pinter->regmap, pinter->ctrl_reg,
			     MODE | DISP_IN_SEL);
		break;
	}

	imx95_pinter_sw_reset(pinter);

	/* HSYNC and VSYNC are active low. Data Enable is active high */
	val = HSYNC_POLARITY | VSYNC_POLARITY;
	writel(val, pinter->regs + CTRL(pinter->sid));

	/* set width if interleaving is used */
	if (pinter->mode != BYPASS)
		writel(WIDTH(pinter->adj.hactive.typ / 2), pinter->regs + ICTRL(pinter->sid));

	ret = video_bridge_set_backlight(pinter->pl_dev, percent);
	if (ret) {
		dev_err(dev, "fail to set backlight\n");
		return ret;
	}

	return 0;
}

static struct video_bridge_ops imx95_pinter_bridge_ops = {
	.attach = imx95_pinter_bridge_attach,
	.check_timing = imx95_pinter_check_timing,
	.set_backlight = imx95_pinter_set_backlight,
};

static int imx95_pinter_probe(struct udevice *dev)
{
	struct imx95_pinter_devdata *devdata;
	struct imx95_pinter *pinter = dev_get_priv(dev);
	u8 inter_stream;
	int ret = 0;

	/* Only probe for channel  */
	if (dev->plat_ != NULL) {
		pinter->regs = (void __iomem *)dev_read_addr(dev_get_parent(dev));
		if (IS_ERR(pinter->regs))
			return PTR_ERR(pinter->regs);

		pinter->sid = (unsigned int)dev_read_addr(dev);
		if ((fdt_addr_t)pinter->sid == FDT_ADDR_T_NONE) {
			dev_err(dev, "Fail to get reg\n");
			return -EINVAL;
		}

		pinter->regmap = syscon_regmap_lookup_by_phandle(dev_get_parent(dev), "nxp,blk-ctrl");
		if (IS_ERR(pinter->regmap)) {
			dev_err(dev, "failed to get blk-ctrl regmap\n");
			return IS_ERR(pinter->regmap);
		}

		ret = clk_get_by_name(dev_get_parent(dev), NULL, &pinter->clk_bus);
		if (ret) {
			dev_err(dev, "failed to get bus clock\n");
			return IS_ERR(pinter->regmap);
		}

		clk_prepare_enable(&pinter->clk_bus);

		inter_stream = NO_INTER_STREAM;
		inter_stream = dev_read_u8_default(dev_get_parent(dev), "nxp,pixel-interleaving-stream-id",
					  NO_INTER_STREAM);
		switch (inter_stream) {
		case 0:
			pinter->mode = STREAM0_SPLIT2;
			break;
		case 1:
			pinter->mode = STREAM1_SPLIT2;
			break;
		default:
			pinter->mode = BYPASS;
			break;
		}

		devdata = (struct imx95_pinter_devdata *)dev_get_driver_data(dev_get_parent(dev));
		pinter->ctrl_reg = devdata->ctrl_reg;
	}

	return ret;
}

static int imx95_pinter_remove(struct udevice *dev)
{
	struct imx95_pinter *pinter = dev_get_priv(dev);

	if (dev->plat_ != NULL) {
		device_remove(pinter->pl_dev, DM_REMOVE_NORMAL);
	}

	return 0;
}

static int imx95_pinter_bind(struct udevice *dev)
{
	ofnode ch_node;
	int ret = 0;

	ch_node = ofnode_find_subnode(dev_ofnode(dev), "channel@0");
	if (ofnode_valid(ch_node)) {
		ret = device_bind(dev, dev->driver, "channel@0", (void *)1,
			ch_node, NULL);
		if (ret)
			printf("Error binding driver '%s': %d\n", dev->driver->name,
				ret);
	}

	ch_node = ofnode_find_subnode(dev_ofnode(dev), "channel@1");
	if (ofnode_valid(ch_node)) {
		ret = device_bind(dev, dev->driver, "channel@1", (void *)2,
			ch_node, NULL);
		if (ret)
			printf("Error binding driver '%s': %d\n", dev->driver->name,
				ret);
	}

	return ret;
}

static const struct imx95_pinter_devdata imx95_pinter_devdata = {
	.ctrl_reg = 0x4,
};

static const struct imx95_pinter_devdata imx952_pinter_devdata = {
	.ctrl_reg = 0x8,
};

static const struct udevice_id imx95_pinter_dt_ids[] = {
	{ .compatible = "nxp,imx95-pixel-interleaver", .data = (ulong)&imx95_pinter_devdata, },
	{ .compatible = "nxp,imx952-pixel-interleaver", .data = (ulong)&imx952_pinter_devdata, },
	{ }
};

U_BOOT_DRIVER(imx95_pinter_driver) = {
	.name				= "imx95_pinter_driver",
	.id				= UCLASS_VIDEO_BRIDGE,
	.of_match			= imx95_pinter_dt_ids,
	.bind				= imx95_pinter_bind,
	.remove				= imx95_pinter_remove,
	.probe				= imx95_pinter_probe,
	.ops				= &imx95_pinter_bridge_ops,
	.priv_auto			= sizeof(struct imx95_pinter),
};
