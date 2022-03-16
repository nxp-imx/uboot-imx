// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019 NXP
 *
 */

#include <common.h>
#include <clk.h>
#include <dm.h>
#include <dm/device_compat.h>
#include <display.h>
#include <video.h>
#include <video_bridge.h>
#include <video_link.h>
#include <asm/io.h>
#include <dm/device-internal.h>
#include <linux/iopoll.h>
#include <linux/err.h>
#include <regmap.h>
#include <syscon.h>

#define IMX6SX_GPR5_DISP_MUX_LDB_CTRL_MASK		(0x1 << 3)
#define IMX6SX_GPR5_DISP_MUX_LDB_CTRL_LCDIF1	(0x0 << 3)
#define IMX6SX_GPR5_DISP_MUX_LDB_CTRL_LCDIF2	(0x1 << 3)

enum {
	LVDS_BIT_MAP_SPWG,
	LVDS_BIT_MAP_JEIDA,
};

static const char *ldb_bit_mappings[] = {
	[LVDS_BIT_MAP_SPWG] = "spwg",
	[LVDS_BIT_MAP_JEIDA] = "jeida",
};

struct imx6sx_ldb_priv {
	struct regmap *gpr;
	u32 data_width;
	int data_map;
	struct display_timing timings;

};

static int imx6sx_ldb_setup(struct udevice *dev, const struct display_timing *timing)
{
	struct imx6sx_ldb_priv *priv = dev_get_priv(dev);
	struct udevice *video_dev;
	u32 ctrl;

	ctrl = IOMUXC_GPR2_LVDS_CH0_MODE_ENABLED_DI0;

	if (priv->data_width == 18)
		ctrl |= IOMUXC_GPR2_DATA_WIDTH_CH0_18BIT;
	else
		ctrl |= IOMUXC_GPR2_DATA_WIDTH_CH0_24BIT;

	if (priv->data_map == LVDS_BIT_MAP_SPWG)
		ctrl |= IOMUXC_GPR2_BIT_MAPPING_CH0_SPWG;
	else
		ctrl |= IOMUXC_GPR2_BIT_MAPPING_CH0_JEIDA;

	if (timing->flags & DISPLAY_FLAGS_VSYNC_HIGH)
		ctrl |= IOMUXC_GPR2_DI0_VS_POLARITY_ACTIVE_HIGH;
	else
		ctrl |= IOMUXC_GPR2_DI0_VS_POLARITY_ACTIVE_LOW;

	/* GPR6 */
	regmap_write(priv->gpr, 0x18, ctrl);

	/* GPR5 */
	video_dev = video_link_get_video_device();
	if (!video_dev) {
		printf("Fail to find video device\n");
		return -ENODEV;
	}

	if (dev_seq(video_dev) == 0) /* lcdif 1 */
		regmap_update_bits(priv->gpr, 0x14,
			IMX6SX_GPR5_DISP_MUX_LDB_CTRL_MASK, IMX6SX_GPR5_DISP_MUX_LDB_CTRL_LCDIF1);
	else
		regmap_update_bits(priv->gpr, 0x14,
			IMX6SX_GPR5_DISP_MUX_LDB_CTRL_MASK, IMX6SX_GPR5_DISP_MUX_LDB_CTRL_LCDIF2);
	return 0;
}

int imx6sx_ldb_read_timing(struct udevice *dev, struct display_timing *timing)
{
	struct imx6sx_ldb_priv *priv = dev_get_priv(dev);

	if (dev->plat_ == NULL)
		return -EINVAL;

	if (timing) {
		memcpy(timing, &priv->timings, sizeof(struct display_timing));
		return 0;
	}

	return -EINVAL;
}

int imx6sx_ldb_enable(struct udevice *dev, int panel_bpp,
		      const struct display_timing *timing)
{
	int ret;

	if (dev->plat_ == NULL)
		return -EINVAL;

	ret = imx6sx_ldb_setup(dev, timing);

	return ret;
}

static int of_get_data_mapping(struct udevice *dev)
{
    const char *bm;
    int i;

    bm = dev_read_string(dev, "fsl,data-mapping");
    if (bm == NULL)
        return -EINVAL;

    for (i = 0; i < ARRAY_SIZE(ldb_bit_mappings); i++)
        if (!strcasecmp(bm, ldb_bit_mappings[i]))
            return i;

    return -EINVAL;
}

static int imx6sx_ldb_probe(struct udevice *dev)
{
	struct imx6sx_ldb_priv *priv = dev_get_priv(dev);
	int ret;

	debug("%s\n", __func__);

	if (dev->plat_ == NULL) {
		priv->gpr = syscon_regmap_lookup_by_phandle(dev, "gpr");
		if (IS_ERR(priv->gpr)) {
			printf("fail to get gpr regmap\n");
			return PTR_ERR(priv->gpr);
		}
	} else {

		struct imx6sx_ldb_priv *parent_priv = dev_get_priv(dev->parent);

		ret = dev_read_u32(dev, "fsl,data-width", &priv->data_width);
		if (ret || (priv->data_width != 18 && priv->data_width != 24)) {
			printf("data width not set or invalid\n");
			return ret;
		}

		priv->data_map= of_get_data_mapping(dev);
		if (priv->data_map < 0) {
			printf("data map not set or invalid\n");
			return priv->data_map;
		}

		priv->gpr = parent_priv->gpr;

		ret = video_link_get_display_timings(&priv->timings);
		if (ret) {
			printf("decode display timing error %d\n", ret);
			return ret;
		}
	}

	return 0;
}

static int imx6sx_ldb_bind(struct udevice *dev)
{
	ofnode lvds_ch_node;
	int ret = 0;

	lvds_ch_node = ofnode_find_subnode(dev_ofnode(dev), "lvds-channel@0");
	if (ofnode_valid(lvds_ch_node)) {
		ret = device_bind(dev, dev->driver, "lvds-channel@0", (void *)1,
			lvds_ch_node, NULL);
		if (ret)
			printf("Error binding driver '%s': %d\n", dev->driver->name,
				ret);
	}

	return ret;
}

static int imx6sx_ldb_remove(struct udevice *dev)
{
	struct imx6sx_ldb_priv *priv = dev_get_priv(dev);

	debug("%s\n", __func__);

	regmap_update_bits(priv->gpr, 0x18,
			IOMUXC_GPR2_LVDS_CH0_MODE_MASK, IOMUXC_GPR2_LVDS_CH0_MODE_DISABLED);

	return 0;
}

struct dm_display_ops imx6sx_ldb_ops = {
	.read_timing = imx6sx_ldb_read_timing,
	.enable = imx6sx_ldb_enable,
};

static const struct udevice_id imx6sx_ldb_ids[] = {
	{ .compatible = "fsl,imx6sx-ldb" },
	{ }
};

U_BOOT_DRIVER(imx6sx_ldb) = {
	.name				= "imx6sx_ldb",
	.id				= UCLASS_DISPLAY,
	.of_match			= imx6sx_ldb_ids,
	.bind				= imx6sx_ldb_bind,
	.probe				= imx6sx_ldb_probe,
	.ops				= &imx6sx_ldb_ops,
	.remove 			= imx6sx_ldb_remove,
	.priv_auto		= sizeof(struct imx6sx_ldb_priv),
};
