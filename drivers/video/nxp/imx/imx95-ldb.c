// SPDX-License-Identifier: GPL-2.0+

/*
 * Copyright 2023 NXP
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
#include <clk.h>
#include <generic-phy.h>
#include <regmap.h>
#include <syscon.h>
#include <media_bus_format.h>
#include <panel.h>

#define LVDS_PHY_CLK_CTRL		0x00
#define  LVDS_PHY_DIV2			BIT(0)

#define LDB_CH0_MODE_EN_TO_DI0		BIT(0)
#define LDB_CH0_MODE_EN_TO_DI1		(3 << 0)
#define LDB_CH0_MODE_EN_MASK		(3 << 0)
#define LDB_CH1_MODE_EN_TO_DI0		BIT(2)
#define LDB_CH1_MODE_EN_TO_DI1		(3 << 2)
#define LDB_CH1_MODE_EN_MASK		(3 << 2)
#define LDB_SPLIT_MODE_EN		BIT(4)
#define LDB_DATA_WIDTH_CH0_24		BIT(5)
#define LDB_BIT_MAP_CH0_JEIDA		BIT(6)
#define LDB_DATA_WIDTH_CH1_24		BIT(7)
#define LDB_BIT_MAP_CH1_JEIDA		BIT(8)
#define LDB_DI0_VS_POL_ACT_LOW		BIT(9)
#define LDB_DI1_VS_POL_ACT_LOW		BIT(10)
#define LDB_DI0_HS_POL_ACT_LOW		BIT(13)
#define LDB_DI1_HS_POL_ACT_LOW		BIT(14)
#define LDB_VSYNC_ADJ_EN		BIT(19)

#define MAX_LDB_CHAN_NUM		2

enum ldb_channel_link_type {
	LDB_CH_SINGLE_LINK,
	LDB_CH_DUAL_LINK_EVEN_ODD_PIXELS,
	LDB_CH_DUAL_LINK_ODD_EVEN_PIXELS,
};

struct imx95_ldb_channel {
	u32 chno;
	bool is_available;
	u32 out_bus_format;
	enum ldb_channel_link_type link_type;
	struct phy phy;
};

struct imx95_ldb {
	struct regmap *regmap;
	struct udevice *conn_dev;
	struct imx95_ldb_channel channel[MAX_LDB_CHAN_NUM];
	struct clk clk_ch[MAX_LDB_CHAN_NUM];
	struct clk clk_di[MAX_LDB_CHAN_NUM];
	unsigned int ctrl_reg;
	u32 ldb_ctrl;
	unsigned int available_ch_cnt;
	int active_chno;
	struct display_timing timings;
};

bool ldb_channel_is_split_link(struct imx95_ldb_channel *ldb_ch)
{
	return ldb_ch->link_type == LDB_CH_DUAL_LINK_EVEN_ODD_PIXELS ||
	       ldb_ch->link_type == LDB_CH_DUAL_LINK_ODD_EVEN_PIXELS;
}

static void imx95_ldb_mode_set(struct udevice *dev,
	const struct display_timing *timing)
{
	struct imx95_ldb *imx95_ldb = dev_get_priv(dev_get_parent(dev));
	struct imx95_ldb_channel *imx95_ldb_ch = &imx95_ldb->channel[imx95_ldb->active_chno];
	struct imx95_ldb_channel *imx95_ldb_ch_split;
	bool is_split = ldb_channel_is_split_link(imx95_ldb_ch);
	int ret;

	debug("%s, split %u, clk %u\n", __func__, is_split, timing->pixelclock.typ);

	ret = generic_phy_init(&imx95_ldb_ch->phy);
	if (ret < 0)
		dev_err(dev, "failed to initialize PHY: %d\n", ret);

	clk_set_rate(&imx95_ldb->clk_di[imx95_ldb_ch->chno], timing->pixelclock.typ);

	if (is_split) {
		imx95_ldb_ch_split =
			&imx95_ldb->channel[imx95_ldb->active_chno ^ 1];
		ret = generic_phy_init(&imx95_ldb_ch_split->phy);
		if (ret < 0)
			dev_err(dev, "failed to init slave PHY: %d\n", ret);

		imx95_ldb->ldb_ctrl |= LDB_SPLIT_MODE_EN;
	}

	imx95_ldb->ldb_ctrl |= LDB_VSYNC_ADJ_EN;

	if (imx95_ldb_ch->chno == 0 || is_split) {
		if (timing->flags & DISPLAY_FLAGS_VSYNC_LOW)
			imx95_ldb->ldb_ctrl |= LDB_DI0_VS_POL_ACT_LOW;
		else if (timing->flags & DISPLAY_FLAGS_VSYNC_HIGH)
			imx95_ldb->ldb_ctrl &= ~LDB_DI0_VS_POL_ACT_LOW;

		if (timing->flags & DISPLAY_FLAGS_HSYNC_LOW)
			imx95_ldb->ldb_ctrl |= LDB_DI0_HS_POL_ACT_LOW;
		else if (timing->flags & DISPLAY_FLAGS_HSYNC_HIGH)
			imx95_ldb->ldb_ctrl &= ~LDB_DI0_HS_POL_ACT_LOW;
	}
	if (imx95_ldb_ch->chno == 1 || is_split) {
		if (timing->flags & DISPLAY_FLAGS_VSYNC_LOW)
			imx95_ldb->ldb_ctrl |= LDB_DI1_VS_POL_ACT_LOW;
		else if (timing->flags & DISPLAY_FLAGS_VSYNC_HIGH)
			imx95_ldb->ldb_ctrl &= ~LDB_DI1_VS_POL_ACT_LOW;

		if (timing->flags & DISPLAY_FLAGS_HSYNC_LOW)
			imx95_ldb->ldb_ctrl |= LDB_DI1_HS_POL_ACT_LOW;
		else if (timing->flags & DISPLAY_FLAGS_HSYNC_HIGH)
			imx95_ldb->ldb_ctrl &= ~LDB_DI1_HS_POL_ACT_LOW;
	}

	debug("ldb out_bus format 0%x\n", imx95_ldb_ch->out_bus_format);

	switch (imx95_ldb_ch->out_bus_format) {
	case MEDIA_BUS_FMT_RGB666_1X7X3_SPWG:
		break;
	case MEDIA_BUS_FMT_RGB888_1X7X4_SPWG:
		if (imx95_ldb_ch->chno == 0 || is_split)
			imx95_ldb->ldb_ctrl |= LDB_DATA_WIDTH_CH0_24;
		if (imx95_ldb_ch->chno == 1 || is_split)
			imx95_ldb->ldb_ctrl |= LDB_DATA_WIDTH_CH1_24;
		break;
	case MEDIA_BUS_FMT_RGB888_1X7X4_JEIDA:
		if (imx95_ldb_ch->chno == 0 || is_split)
			imx95_ldb->ldb_ctrl |= LDB_DATA_WIDTH_CH0_24 |
					 LDB_BIT_MAP_CH0_JEIDA;
		if (imx95_ldb_ch->chno == 1 || is_split)
			imx95_ldb->ldb_ctrl |= LDB_DATA_WIDTH_CH1_24 |
					 LDB_BIT_MAP_CH1_JEIDA;
		break;
	}
}

static void imx95_ldb_enable(struct udevice *dev)
{
	struct imx95_ldb *imx95_ldb = dev_get_priv(dev_get_parent(dev));
	struct imx95_ldb_channel *imx95_ldb_ch = &imx95_ldb->channel[imx95_ldb->active_chno];
	bool is_split = ldb_channel_is_split_link(imx95_ldb_ch);
	int ret;

	clk_enable(&imx95_ldb->clk_ch[imx95_ldb_ch->chno]);
	clk_enable(&imx95_ldb->clk_di[imx95_ldb_ch->chno]);
	if (is_split) {
		clk_enable(&imx95_ldb->clk_ch[imx95_ldb_ch->chno ^ 1]);
		clk_enable(&imx95_ldb->clk_di[imx95_ldb_ch->chno ^ 1]);
	}

	if (imx95_ldb_ch->chno == 0 || is_split) {
		imx95_ldb->ldb_ctrl &= ~LDB_CH0_MODE_EN_MASK;
		imx95_ldb->ldb_ctrl |= LDB_CH0_MODE_EN_TO_DI0;
	}
	if (imx95_ldb_ch->chno == 1 || is_split) {
		imx95_ldb->ldb_ctrl &= ~LDB_CH1_MODE_EN_MASK;
		imx95_ldb->ldb_ctrl |= is_split ?
			       LDB_CH1_MODE_EN_TO_DI0 : LDB_CH1_MODE_EN_TO_DI1;
	}

	if (is_split) {
		/* PHY clock divider 2 */
		regmap_update_bits(imx95_ldb->regmap, LVDS_PHY_CLK_CTRL,
				   LVDS_PHY_DIV2, LVDS_PHY_DIV2);

		ret = generic_phy_power_on(&imx95_ldb->channel[0].phy);
		if (ret)
			dev_err(dev,
				"failed to power on channel0 PHY: %d\n", ret);

		ret = generic_phy_power_on(&imx95_ldb->channel[1].phy);
		if (ret)
			dev_err(dev,
				"failed to power on channel1 PHY: %d\n", ret);
	} else {
		ret = generic_phy_power_on(&imx95_ldb_ch->phy);
		if (ret)
			dev_err(dev, "failed to power on PHY: %d\n", ret);
	}

	regmap_write(imx95_ldb->regmap, imx95_ldb->ctrl_reg, imx95_ldb->ldb_ctrl);
}

static void imx95_ldb_disable(struct udevice *dev)
{
	struct imx95_ldb *imx95_ldb = dev_get_priv(dev_get_parent(dev));
	struct imx95_ldb_channel *imx95_ldb_ch = &imx95_ldb->channel[imx95_ldb->active_chno];
	bool is_split = ldb_channel_is_split_link(imx95_ldb_ch);
	int ret;

	if (imx95_ldb_ch->chno == 0 || is_split)
		imx95_ldb->ldb_ctrl &= ~LDB_CH0_MODE_EN_MASK;
	if (imx95_ldb_ch->chno == 1 || is_split)
		imx95_ldb->ldb_ctrl &= ~LDB_CH1_MODE_EN_MASK;

	regmap_write(imx95_ldb->regmap, imx95_ldb->ctrl_reg, imx95_ldb->ldb_ctrl);

	if (is_split) {
		ret = generic_phy_power_off(&imx95_ldb->channel[0].phy);
		if (ret)
			dev_err(dev,
				"failed to power off channel0 PHY: %d\n", ret);
		ret = generic_phy_power_off(&imx95_ldb->channel[1].phy);
		if (ret)
			dev_err(dev,
				"failed to power off channel1 PHY: %d\n", ret);

		/* clean PHY clock divider 2 */
		regmap_update_bits(imx95_ldb->regmap, LVDS_PHY_CLK_CTRL, LVDS_PHY_DIV2, 0);

	} else {
		ret = generic_phy_power_off(&imx95_ldb_ch->phy);
		if (ret)
			dev_err(dev, "failed to power off PHY: %d\n", ret);
	}


	if (is_split) {
		clk_disable(&imx95_ldb->clk_di[imx95_ldb_ch->chno ^ 1]);
		clk_disable(&imx95_ldb->clk_ch[imx95_ldb_ch->chno ^ 1]);
	}
	clk_disable(&imx95_ldb->clk_di[imx95_ldb_ch->chno]);
	clk_disable(&imx95_ldb->clk_ch[imx95_ldb_ch->chno]);

}

int imx95_ldb_read_timing(struct udevice *dev, struct display_timing *timing)
{
	struct imx95_ldb *priv = dev_get_priv(dev);
	ulong drv_data = dev_get_driver_data(dev);

	if (drv_data)
		return -EINVAL;

	if (timing) {
		memcpy(timing, &priv->timings, sizeof(struct display_timing));
		return 0;
	}

	return -EINVAL;
}

int imx95_ldb_enable_display(struct udevice *dev, int panel_bpp,
		      const struct display_timing *timing)
{
	struct imx95_ldb *priv = dev_get_priv(dev);
	ulong drv_data = dev_get_driver_data(dev);
	int ret;

	if (drv_data)
		return -EINVAL;

	imx95_ldb_mode_set(dev, &priv->timings);
	imx95_ldb_enable(dev);

	if (IS_ENABLED(CONFIG_VIDEO_BRIDGE)) {
		if (priv->conn_dev &&
			device_get_uclass_id(priv->conn_dev) == UCLASS_VIDEO_BRIDGE) {
			ret = video_bridge_set_backlight(priv->conn_dev, 80);
			if (ret) {
				dev_err(dev, "fail to set backlight\n");
				return ret;
			}
		}
	}

	if (IS_ENABLED(CONFIG_PANEL)) {
		if (priv->conn_dev &&
			device_get_uclass_id(priv->conn_dev) == UCLASS_PANEL) {
			ret = panel_enable_backlight(priv->conn_dev);
			if (ret) {
				dev_err(dev, "fail to set backlight\n");
				return ret;
			}
		}
	}

	return 0;
}

static int of_get_bus_format(struct udevice *dev, u32 *bus_format)
{
	const char *bm;
	int ret;
	u32 dw;

	bm = dev_read_string(dev, "fsl,data-mapping");
	if (bm == NULL)
		return -EINVAL;

	ret = dev_read_u32(dev, "fsl,data-width", &dw);
	if (ret || (dw != 18 && dw != 24)) {
		printf("data width not set or invalid\n");
		return -EINVAL;
	}

	if (!strcasecmp(bm, "spwg") && dw == 18)
		*bus_format = MEDIA_BUS_FMT_RGB666_1X7X3_SPWG;
	else if (!strcasecmp(bm, "spwg") && dw == 24)
		*bus_format = MEDIA_BUS_FMT_RGB888_1X7X4_SPWG;
	else if (!strcasecmp(bm, "jeida") && dw == 24)
		*bus_format = MEDIA_BUS_FMT_RGB888_1X7X4_JEIDA;
	else
		return -EINVAL;

	return 0;
}

static int imx95_ldb_probe(struct udevice *dev)
{
	struct imx95_ldb *priv = dev_get_priv(dev);
	ulong drv_data = dev_get_driver_data(dev);
	int ret, i;
	ofnode lvds_ch_node;
	u32 ch_id;
	const char *stat;

	if (!drv_data) { /* sub node*/

		ofnode ldb_ep, panel_ep;
		u32 phandle, dual_odd, dual_even;
		struct imx95_ldb *priv_parent = dev_get_priv(dev_get_parent(dev));

		priv->conn_dev = video_link_get_next_device(dev);
		if (!priv->conn_dev) {
			dev_err(dev, "can't find next device in video link\n");
			return -ENODEV;
		}

		ret = dev_read_u32(dev, "reg", &ch_id);
		if (ret) {
			dev_err(dev, "can't find reg property\n");
			return ret;
		}

		priv_parent->active_chno = ch_id;

		ret = of_get_bus_format(dev, &priv_parent->channel[ch_id].out_bus_format);
		if (ret) {
			dev_err(dev, "can't find property for bus format\n");
			return ret;
		}

		debug("ldb channel bus format 0x%x\n", priv_parent->channel[ch_id].out_bus_format);

		if (priv_parent->available_ch_cnt == 1) {
			priv_parent->channel[ch_id].link_type = LDB_CH_SINGLE_LINK;
		} else {
			ldb_ep = video_link_get_ep_to_nextdev(priv->conn_dev);
			if (!ofnode_valid(ldb_ep)) {
				dev_err(dev, "failed to get ldb channel endpoint\n");
				return -ENODEV;
			}

			ret = ofnode_read_u32(ldb_ep, "remote-endpoint", &phandle);
			if (ret) {
				dev_err(dev, "required remote-endpoint property isn't provided\n");
				return -ENODEV;
			}

			panel_ep = ofnode_get_by_phandle(phandle);
			if (!ofnode_valid(panel_ep)) {
				dev_err(dev, "failed to find remote-endpoint\n");
				return -ENODEV;
			}

			dual_odd = (u32)ofnode_read_bool(ofnode_get_parent(panel_ep), "dual-lvds-odd-pixels");
			dual_even = (u32)ofnode_read_bool(ofnode_get_parent(panel_ep), "dual-lvds-even-pixels");

			if (!dual_odd && !dual_even) {
				priv_parent->channel[ch_id].link_type = LDB_CH_SINGLE_LINK;
			} else if ((dual_odd && ch_id == 0) || (dual_even && ch_id == 1)) {
				priv_parent->channel[ch_id].link_type = LDB_CH_DUAL_LINK_ODD_EVEN_PIXELS;
			} else {
				dev_err(dev, "invalid dual link pixel order\n");
				return -EINVAL;
			}
		}

		ret = video_link_get_display_timings(&priv->timings);
		if (ret) {
			dev_err(dev, "decode display timing error %d\n", ret);
			return ret;
		}

		if (IS_ENABLED(CONFIG_VIDEO_BRIDGE)) {
			if (priv->conn_dev &&
				device_get_uclass_id(priv->conn_dev) == UCLASS_VIDEO_BRIDGE) {
				ret = video_bridge_attach(priv->conn_dev);
				if (ret) {
					dev_err(dev, "fail to attach bridge\n");
					return ret;
				}

				ret = video_bridge_set_active(priv->conn_dev, true);
				if (ret) {
					dev_err(dev, "fail to active bridge\n");
					return ret;
				}
			}
		}

		return 0;
	}

	for (i = 0; i < MAX_LDB_CHAN_NUM; i++) {
		char clk_name_ldb_ch[8], clk_name_ldb_di[8];

		snprintf(clk_name_ldb_ch, sizeof(clk_name_ldb_ch), "ldb_ch%d", i);
		snprintf(clk_name_ldb_di, sizeof(clk_name_ldb_di), "ldb_di%d", i);

		ret = clk_get_by_name(dev, clk_name_ldb_ch, &priv->clk_ch[i]);
		if (ret) {
			dev_err(dev, "failed to get clock %s\n", clk_name_ldb_ch);
			return ret;
		}

		ret = clk_get_by_name(dev, clk_name_ldb_di, &priv->clk_di[i]);
		if (ret) {
			dev_err(dev, "failed to get clock %s\n", clk_name_ldb_di);
			return ret;
		}
	}

	priv->regmap = syscon_node_to_regmap(dev_ofnode(dev_get_parent(dev)));
	if (IS_ERR(priv->regmap)) {
		ret = PTR_ERR(priv->regmap);
		dev_err(dev, "failed to get regmap: %d\n", ret);
		return ret;
	}

	priv->ctrl_reg = 0x4;
	priv->ldb_ctrl = 0;

	ofnode_for_each_subnode(lvds_ch_node, dev_ofnode(dev)) {
		if (ofnode_read_u32(lvds_ch_node, "reg", &ch_id)) {
			dev_err(dev, "missing reg property in node %s\n",
				ofnode_get_name(lvds_ch_node));
			return -EINVAL;
		}

		if (ch_id >= MAX_LDB_CHAN_NUM) {
			dev_err(dev, "invalid reg in node %s\n", ofnode_get_name(lvds_ch_node));
			return -EINVAL;
		}

		stat = ofnode_read_string(lvds_ch_node, "status");
		if (stat && strcmp(stat, "okay"))
			continue;

		ret = generic_phy_get_by_index_nodev(lvds_ch_node, 0, &priv->channel[ch_id].phy);
		if (ret) {
			dev_err(dev, "fail to get phy device\n");
			return -EINVAL;
		}

		priv->channel[ch_id].chno = ch_id;
		priv->channel[ch_id].is_available = true;

		priv->available_ch_cnt++;
	}

	if (priv->available_ch_cnt == 0) {
		dev_dbg(dev, "no available channel\n");
		return 0;
	}

	return 0;
}

static int imx95_ldb_remove(struct udevice *dev)
{
	ulong drv_data = dev_get_driver_data(dev);

	if (drv_data)
		return -EINVAL;

	imx95_ldb_disable(dev);

	return 0;
}

static int imx95_ldb_bind(struct udevice *dev)
{
	ofnode lvds_ch_node;
	u32 ch_id;
	ulong drv_data = dev_get_driver_data(dev);
	int ret = 0;

	if (drv_data) {
		/* Parent lvds phy node, bind each subnode to driver */
		ofnode_for_each_subnode(lvds_ch_node, dev_ofnode(dev)) {
			if (ofnode_read_u32(lvds_ch_node, "reg", &ch_id)) {
				dev_err(dev, "missing reg property in node %s\n",
					ofnode_get_name(lvds_ch_node));
				return -EINVAL;
			}

			if (ch_id >= MAX_LDB_CHAN_NUM) {
				dev_err(dev, "invalid reg in node %s\n", ofnode_get_name(lvds_ch_node));
				return -EINVAL;
			}

			ret = device_bind(dev, dev->driver, ofnode_get_name(lvds_ch_node),
				(void *)(ulong)ch_id, lvds_ch_node, NULL);
			if (ret)
				printf("Error binding driver '%s': %d\n", dev->driver->name,
					ret);
		}
	}

	return ret;
}

struct dm_display_ops imx95_ldb_ops = {
	.read_timing = imx95_ldb_read_timing,
	.enable = imx95_ldb_enable_display,
};

static const struct udevice_id imx95_ldb_ids[] = {
	{ .compatible = "fsl,imx95-ldb", .data = 1 }, /* data is used to indicate parent device */
	{ /* sentinel */ }
};

U_BOOT_DRIVER(imx95_ldb) = {
	.name				= "imx95_ldb",
	.id				= UCLASS_DISPLAY,
	.of_match			= imx95_ldb_ids,
	.bind				= imx95_ldb_bind,
	.probe				= imx95_ldb_probe,
	.remove				= imx95_ldb_remove,
	.ops				= &imx95_ldb_ops,
	.priv_auto		= sizeof(struct imx95_ldb),
};
