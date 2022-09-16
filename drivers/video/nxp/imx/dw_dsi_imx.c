// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2022 NXP
 *
 */

#include <common.h>
#include <clk.h>
#include <dm.h>
#include <dm/device_compat.h>
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
#include <phy-mipi-dphy.h>
#include <generic-phy.h>

#define MSEC_PER_SEC			1000

struct dw_dsi_imx_priv {
	struct mipi_dsi_device device;
	struct udevice *panel;
	struct udevice *dsi_host;

	struct clk byte_clk;

	struct phy phy;
	struct phy_configure_opts_mipi_dphy phy_cfg;

	unsigned int lane_mbps; /* per lane */
	u32 lanes;
	u32 format;
	struct display_timing adj;
};

static int dw_mipi_dsi_imx_phy_init(void *priv_data)
{
	struct dw_dsi_imx_priv *dsi = priv_data;
	int ret;

	ret = generic_phy_init(&dsi->phy);
	if (ret < 0) {
		dev_err(dsi->device.dev, "failed to init phy: %d\n", ret);
		return ret;
	}

	ret = generic_phy_configure(&dsi->phy, &dsi->phy_cfg);
	if (ret < 0) {
		dev_err(dsi->device.dev, "failed to configure phy: %d\n", ret);
		goto uninit_phy;
	}

	ret = generic_phy_power_on(&dsi->phy);
	if (ret < 0) {
		dev_err(dsi->device.dev, "failed to power on phy: %d\n", ret);
		goto uninit_phy;
	}

	return ret;

uninit_phy:
	generic_phy_exit(&dsi->phy);
	return ret;
}

static int
dw_mipi_dsi_get_lane_mbps(void *priv_data, struct display_timing *timings,
								 u32 lanes, u32 format, unsigned int *lane_mbps)
{
	struct dw_dsi_imx_priv *dsi = priv_data;
	int bpp;
	int ret;

	bpp = mipi_dsi_pixel_format_to_bpp(format);
	if (bpp < 0) {
		dev_dbg(dsi->device.dev,
			      "failed to get bpp for pixel format %d\n",
			      format);
		return bpp;
	}

	dsi->lane_mbps = DIV_ROUND_UP((timings->pixelclock.typ / 1000) * (bpp / lanes), MSEC_PER_SEC);
	*lane_mbps = dsi->lane_mbps;

	debug("lane_mbps %u, bpp %d\n", *lane_mbps, bpp);

	ret = phy_mipi_dphy_get_default_config(timings->pixelclock.typ,
					       bpp, lanes,
					       &dsi->phy_cfg);
	if (ret < 0) {
		dev_dbg(dsi->device.dev, "failed to get default phy cfg %d\n", ret);
		return ret;
	}

	return 0;
}

struct hstt {
	unsigned int maxfreq;
	struct mipi_dsi_phy_timing timing;
};

#define HSTT(_maxfreq, _c_lp2hs, _c_hs2lp, _d_lp2hs, _d_hs2lp)	\
{								\
	.maxfreq = (_maxfreq),					\
	.timing = {						\
		.clk_lp2hs = (_c_lp2hs),			\
		.clk_hs2lp = (_c_hs2lp),			\
		.data_lp2hs = (_d_lp2hs),			\
		.data_hs2lp = (_d_hs2lp),			\
	}							\
}

/* Table A-4 High-Speed Transition Times */
struct hstt hstt_table[] = {
	HSTT(80,   21,  17,  15, 10),
	HSTT(90,   23,  17,  16, 10),
	HSTT(100,  22,  17,  16, 10),
	HSTT(110,  25,  18,  17, 11),
	HSTT(120,  26,  20,  18, 11),
	HSTT(130,  27,  19,  19, 11),
	HSTT(140,  27,  19,  19, 11),
	HSTT(150,  28,  20,  20, 12),
	HSTT(160,  30,  21,  22, 13),
	HSTT(170,  30,  21,  23, 13),
	HSTT(180,  31,  21,  23, 13),
	HSTT(190,  32,  22,  24, 13),
	HSTT(205,  35,  22,  25, 13),
	HSTT(220,  37,  26,  27, 15),
	HSTT(235,  38,  28,  27, 16),
	HSTT(250,  41,  29,  30, 17),
	HSTT(275,  43,  29,  32, 18),
	HSTT(300,  45,  32,  35, 19),
	HSTT(325,  48,  33,  36, 18),
	HSTT(350,  51,  35,  40, 20),
	HSTT(400,  59,  37,  44, 21),
	HSTT(450,  65,  40,  49, 23),
	HSTT(500,  71,  41,  54, 24),
	HSTT(550,  77,  44,  57, 26),
	HSTT(600,  82,  46,  64, 27),
	HSTT(650,  87,  48,  67, 28),
	HSTT(700,  94,  52,  71, 29),
	HSTT(750,  99,  52,  75, 31),
	HSTT(800, 105,  55,  82, 32),
	HSTT(850, 110,  58,  85, 32),
	HSTT(900, 115,  58,  88, 35),
	HSTT(950, 120,  62,  93, 36),
	HSTT(1000, 128,  63,  99, 38),
	HSTT(1050, 132,  65, 102, 38),
	HSTT(1100, 138,  67, 106, 39),
	HSTT(1150, 146,  69, 112, 42),
	HSTT(1200, 151,  71, 117, 43),
	HSTT(1250, 153,  74, 120, 45),
	HSTT(1300, 160,  73, 124, 46),
	HSTT(1350, 165,  76, 130, 47),
	HSTT(1400, 172,  78, 134, 49),
	HSTT(1450, 177,  80, 138, 49),
	HSTT(1500, 183,  81, 143, 52),
	HSTT(1550, 191,  84, 147, 52),
	HSTT(1600, 194,  85, 152, 52),
	HSTT(1650, 201,  86, 155, 53),
	HSTT(1700, 208,  88, 161, 53),
	HSTT(1750, 212,  89, 165, 53),
	HSTT(1800, 220,  90, 171, 54),
	HSTT(1850, 223,  92, 175, 54),
	HSTT(1900, 231,  91, 180, 55),
	HSTT(1950, 236,  95, 185, 56),
	HSTT(2000, 243,  97, 190, 56),
	HSTT(2050, 248,  99, 194, 58),
	HSTT(2100, 252, 100, 199, 59),
	HSTT(2150, 259, 102, 204, 61),
	HSTT(2200, 266, 105, 210, 62),
	HSTT(2250, 269, 109, 213, 63),
	HSTT(2300, 272, 109, 217, 65),
	HSTT(2350, 281, 112, 225, 66),
	HSTT(2400, 283, 115, 226, 66),
	HSTT(2450, 282, 115, 226, 67),
	HSTT(2500, 281, 118, 227, 67),
};

static int
dw_mipi_dsi_phy_get_timing(void *priv_data, unsigned int lane_mbps,
			   struct mipi_dsi_phy_timing *timing)
{
	struct dw_dsi_imx_priv *dsi = priv_data;
	int i;

	for (i = 0; i < ARRAY_SIZE(hstt_table); i++)
		if (lane_mbps <= hstt_table[i].maxfreq)
			break;

	if (i == ARRAY_SIZE(hstt_table))
		i--;

	*timing = hstt_table[i].timing;

	dev_dbg(dsi->device.dev, "get phy timing for %u <= %u (lane_mbps)\n",
		      lane_mbps, hstt_table[i].maxfreq);

	return 0;
}

static const struct mipi_dsi_phy_ops dsi_imx_phy_ops = {
	.init = dw_mipi_dsi_imx_phy_init,
	.get_lane_mbps = dw_mipi_dsi_get_lane_mbps,
	.get_timing = dw_mipi_dsi_phy_get_timing,
};

static bool dw_dsi_imx_hcomponents_need_fixup(struct dw_dsi_imx_priv *dsi,
				       int bpp,
				       struct display_timing *timings)
{
	int htotal = timings->hactive.typ + timings->hfront_porch.typ +
		 timings->hback_porch.typ + timings->hsync_len.typ;
	int hsa = timings->hsync_len.typ;
	int hbp = timings->hback_porch.typ;
	int divisor = dsi->lanes * 8;

	/*
	 * It appears that (hcomponent * bpp) / (8 * lanes)
	 * should be no remainder.
	 */
	return !!((htotal * bpp) % divisor) ||
	       !!((hsa * bpp) % divisor) ||
	       !!((hbp * bpp) % divisor);
}

static int dw_dsi_imx_fixup_hcomponent(struct dw_dsi_imx_priv *dsi,
					    int bpp, int component)
{
	int divisor, i;

	divisor = dsi->lanes * 8;

	for (i = 0; i < divisor; i++) {
		if ((bpp * (component + i)) % divisor == 0) {
			component += i;
			break;
		}
	}

	return component;
}

static void dw_dsi_imx_fixup_hcomponents(struct dw_dsi_imx_priv *dsi,
				  int bpp,
				  struct display_timing *timings,
				  struct display_timing *adj)
{
	int hfp = timings->hfront_porch.typ;
	int hsa = timings->hsync_len.typ;
	int hbp = timings->hback_porch.typ;

	adj->hfront_porch.typ = dw_dsi_imx_fixup_hcomponent(dsi, bpp, hfp);
	adj->hsync_len.typ = dw_dsi_imx_fixup_hcomponent(dsi, bpp, hsa);
	adj->hback_porch.typ = dw_dsi_imx_fixup_hcomponent(dsi, bpp, hbp);
}

static int dw_dsi_imx_attach(struct udevice *dev)
{
	struct dw_dsi_imx_priv *priv = dev_get_priv(dev);
	struct mipi_dsi_device *device = &priv->device;
	struct mipi_dsi_panel_plat *mplat;
	struct display_timing timings;
	int ret, bpp;

	priv->panel = video_link_get_next_device(dev);
	if (!priv->panel ||
		device_get_uclass_id(priv->panel) != UCLASS_PANEL) {
		dev_err(dev, "get panel device error\n");
		return -ENODEV;
	}

	mplat = dev_get_plat(priv->panel);
	mplat->device = &priv->device;

	ret = video_link_get_display_timings(&timings);
	if (ret) {
		dev_err(dev, "decode display timing error %d\n", ret);
		return ret;
	}

	bpp = mipi_dsi_pixel_format_to_bpp(device->format);
	if (bpp < 0) {
		dev_err(dev, "failed to get bpp for pixel format %d\n", device->format);
		return bpp;
	}

	priv->lanes = device->lanes;
	priv->format = device->format;

	priv->adj = timings;
	if (dw_dsi_imx_hcomponents_need_fixup(priv, bpp, &timings))
		dw_dsi_imx_fixup_hcomponents(priv, bpp, &timings, &priv->adj);

	ret = uclass_get_device(UCLASS_DSI_HOST, 0, &priv->dsi_host);
	if (ret) {
		dev_err(dev, "No video dsi host detected %d\n", ret);
		return ret;
	}

	ret = dsi_host_init(priv->dsi_host, device, &priv->adj,
			4,
			&dsi_imx_phy_ops);
	if (ret) {
		dev_err(dev, "failed to initialize mipi dsi host\n");
		return ret;
	}

	return 0;
}

static int dw_dsi_imx_set_backlight(struct udevice *dev, int percent)
{
	struct dw_dsi_imx_priv *priv = dev_get_priv(dev);
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

static int dw_dsi_imx_check_timing(struct udevice *dev, struct display_timing *timing)
{
	struct dw_dsi_imx_priv *priv = dev_get_priv(dev);

	/* Ensure the bridge device attached to panel */
	if (!priv->panel) {
		dev_err(dev, "%s No panel device attached\n", __func__);
		return -ENOTCONN;
	}

	/* DSI force the Polarities as high */
	priv->adj.flags &= ~(DISPLAY_FLAGS_HSYNC_LOW | DISPLAY_FLAGS_VSYNC_LOW);
	priv->adj.flags |= DISPLAY_FLAGS_HSYNC_HIGH | DISPLAY_FLAGS_VSYNC_HIGH;

	*timing = priv->adj;

	return 0;
}

static int dw_dsi_imx_probe(struct udevice *dev)
{
	struct dw_dsi_imx_priv *priv = dev_get_priv(dev);
	struct mipi_dsi_device *device = &priv->device;
	int ret;

	device->dev = dev;

	ret = clk_get_by_name(device->dev, "byte", &priv->byte_clk);
	if (ret) {
		dev_err(dev, "byte clock get error %d\n", ret);
		return ret;
	}

	ret = clk_enable(&priv->byte_clk);
	if (ret) {
		dev_err(dev, "byte clock enable error %d\n", ret);
		return ret;
	}

	ret = generic_phy_get_by_name(dev, "dphy", &priv->phy);
	if (ret) {
		dev_err(dev, "failed to get phy: %d\n", ret);
		clk_disable(&priv->byte_clk);
		return ret;
	}

	return ret;
}

static int dw_dsi_imx_remove(struct udevice *dev)
{
	struct dw_dsi_imx_priv *priv = dev_get_priv(dev);
	int ret;

	if (priv->panel)
		device_remove(priv->panel, DM_REMOVE_NORMAL);

	ret = dsi_host_disable(priv->dsi_host);
	if (ret < 0 && ret != -ENOSYS)
		dev_err(dev, "failed to disable mipi dsi host\n");

	ret = generic_phy_power_off(&priv->phy);
	if (ret < 0)
		dev_err(dev, "failed to power off phy: %d\n", ret);

	ret = generic_phy_exit(&priv->phy);
	if (ret < 0)
		dev_err(dev, "failed to exit phy: %d\n", ret);

	device_remove(priv->phy.dev, DM_REMOVE_NORMAL);

	ret = clk_disable(&priv->byte_clk);
	if (ret)
		dev_err(dev, "byte clock disable error %d\n", ret);

	return 0;
}

struct video_bridge_ops dw_dsi_imx_ops = {
	.attach = dw_dsi_imx_attach,
	.set_backlight = dw_dsi_imx_set_backlight,
	.check_timing = dw_dsi_imx_check_timing,
};

static const struct udevice_id dw_dsi_imx_ids[] = {
	{ .compatible = "fsl,imx93-mipi-dsi" },
	{ }
};

U_BOOT_DRIVER(dw_dsi_imx) = {
	.name				= "dw_dsi_imx",
	.id					= UCLASS_VIDEO_BRIDGE,
	.of_match			= dw_dsi_imx_ids,
	.bind				= dm_scan_fdt_dev,
	.remove				= dw_dsi_imx_remove,
	.probe				= dw_dsi_imx_probe,
	.ops				= &dw_dsi_imx_ops,
	.priv_auto			= sizeof(struct dw_dsi_imx_priv),
};
