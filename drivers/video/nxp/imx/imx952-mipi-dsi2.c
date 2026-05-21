// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2025-2026 NXP
 */

#include <asm/io.h>
#include <clk.h>
#include <dm.h>
#include <dm/device_compat.h>
#include <dm/device-internal.h>
#include <div64.h>
#include <dsi_host.h>
#include <generic-phy.h>
#include <linux/bitfield.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/iopoll.h>
#include <linux/kernel.h>
#include <linux/time.h>
#include <mipi_dsi.h>
#include <mux.h>
#include <panel.h>
#include <phy-mipi-dphy.h>
#include <regmap.h>
#include <syscon.h>
#include <video_link.h>
#include <video_bridge.h>

#define DSI_HOST_CONFIGURATION		0x14
#define PIXEL_LINK_FORMAT_MASK		GENMASK(2, 0)
#define SHUTDOWN			BIT(4)
#define COLORMODE			BIT(5)

#define IMX952_DSI_ENDPOINT_PL0		0
#define IMX952_DSI_ENDPOINT_PL1		1

#define PIXEL_LINK_STREAMS		2

#define MAX_ESC_CLK_RATE_HZ			20000000

enum dsi_pixel_link_format {
	RGB_24BIT,
	RGB_30BIT,
	RGB_18BIT,
	RGB_16BIT,
	YCBCR_20BIT_422,
	YCBCR_16BIT_422,
};

struct imx952_dsi2_priv {
	struct mipi_dsi_device device;
	struct udevice *panel;
	struct udevice *dsi_host;

	void __iomem *base;
	struct regmap *dsi_csr;
	struct clk *clk_pixel;
	struct clk *clk_cfg;
	struct clk *clk_ref;
	struct clk *clk_phy_pll;
	struct phy phy;
	struct mux_control *mux;
	bool use_pl0;

	unsigned long ref_clk_rate;

	struct phy_configure_opts_mipi_dphy phy_cfg;

	unsigned int lane_mbps; /* per lane */
	u32 lanes;
	u32 format;
	struct display_timing adj;
	unsigned long esc_clk_rate;
	unsigned long mode_flags;
	bool phy_submode;
	bool hs2lp_lp2hs_quirk;
};

static inline unsigned long data_rate_to_fout(unsigned long data_rate)
{
	/* Fout is half of data rate */
	return data_rate / 2;
}

static int imx952_dsi2_phy_init(void *priv_data)
{
	struct imx952_dsi2_priv *dsi = priv_data;
	struct udevice *dev = dsi->device.dev;
	int bpp, ret;

	ret = mux_control_try_select(dsi->mux, !dsi->use_pl0);
	if (ret < 0) {
		dev_err(dev, "failed to select the pixel link connected to the DSI host controller %d\n", ret);
		return ret;
	}

	bpp = mipi_dsi_pixel_format_to_bpp(dsi->format);
	if (bpp < 0) {
		dev_err(dev, "failed to obtain the number of bits per pixel\n");
		return bpp;
	}

	switch (bpp) {
	case 24:
		regmap_write(dsi->dsi_csr, DSI_HOST_CONFIGURATION, RGB_24BIT);
		break;
	case 18:
		regmap_write(dsi->dsi_csr, DSI_HOST_CONFIGURATION, RGB_18BIT);
		break;
	case 16:
		regmap_write(dsi->dsi_csr, DSI_HOST_CONFIGURATION, RGB_16BIT);
		break;
	default:
		dev_err(dev, "invalid bpp %d\n", bpp);
		return -EINVAL;
	}

	ret = generic_phy_set_mode(&dsi->phy, PHY_MODE_MIPI_DPHY, dsi->phy_submode);
	if (ret < 0) {
		dev_err(dev, "failed to set phy mode: %d\n", ret);
		return ret;
	}

	ret = generic_phy_init(&dsi->phy);
	if (ret < 0) {
		dev_err(dev, "failed to init phy: %d\n", ret);
		return ret;
	}

	return 0;
}

static void imx952_dsi2_phy_power_on(void *priv_data)
{
	struct imx952_dsi2_priv *dsi = priv_data;
	struct udevice *dev = dsi->device.dev;
	struct phy_configure_opts_mipi_dphy *dphy_opts = &dsi->phy_cfg;
	int ret;

	dphy_opts->lpx = PSEC_PER_SEC / dsi->esc_clk_rate;

	debug("PHY lpx = %ups\n", dphy_opts->lpx);

	ret = generic_phy_configure(&dsi->phy, &dsi->phy_cfg);
	if (ret < 0) {
		dev_err(dev, "failed to configure phy: %d\n", ret);
		return;
	}

	ret = generic_phy_power_on(&dsi->phy);
	if (ret < 0)
		dev_err(dev, "failed to power on phy: %d\n", ret);
}

static void imx952_dsi2_phy_power_off(void *priv_data)
{
	struct imx952_dsi2_priv *dsi = priv_data;
	struct udevice *dev = dsi->device.dev;
	int ret;

	ret = generic_phy_power_off(&dsi->phy);
	if (ret < 0)
		dev_err(dev, "failed to power off phy: %d\n", ret);

	ret = generic_phy_exit(&dsi->phy);
	if (ret < 0)
		dev_err(dev, "failed to exit phy: %d\n", ret);

	ret = mux_control_deselect(dsi->mux);
	if (ret < 0)
		dev_err(dev, "failed to deselect input: %d\n", ret);
}

static void imx952_dsi2_phy_get_iface(void *priv_data,
				      struct dw_mipi_dsi2_phy_iface *iface)
{
	/* PPI width is fixed to 8 bits in DCPHY */
	iface->ppi_width = 8;
	iface->phy_type = DW_MIPI_DSI2_DPHY;
}

static int
imx952_dsi2_get_phy_configure_opts(struct imx952_dsi2_priv *dsi,
				   struct display_timing *timings,
				   struct phy_configure_opts_mipi_dphy *phy_cfg,
				   unsigned long mode_flags, u32 lanes, u32 format)
{
	struct udevice *dev = dsi->device.dev;
	unsigned long target_pixel_clock;
	unsigned long pclk_rate = timings->pixelclock.typ;
	unsigned long fout;
	int bpp;
	int ret;

	bpp = mipi_dsi_pixel_format_to_bpp(format);
	if (bpp < 0) {
		dev_dbg(dev, "failed to get bpp for pixel format %d\n", format);
		return -EINVAL;
	}

	target_pixel_clock = pclk_rate;
	if (mode_flags & MIPI_DSI_MODE_VIDEO_BURST)
		target_pixel_clock = target_pixel_clock * 10 / 9;

	ret = phy_mipi_dphy_get_default_config(target_pixel_clock, bpp, lanes, phy_cfg);
	if (ret < 0) {
		dev_dbg(dev, "failed to get default phy cfg %d\n", ret);
		return ret;
	}

	fout = data_rate_to_fout(phy_cfg->hs_clk_rate);
	if (fout != clk_round_rate(dsi->clk_phy_pll, fout)) {
		dev_dbg(dev, "failed to round phy PLL clk rate %luHz\n", fout);
		return -EINVAL;
	}

	return 0;
}

static int
imx952_dsi2_phy_get_lane_mbps(void *priv_data, struct display_timing *timings,
			      u32 lanes, u32 format, unsigned int *lane_mbps)
{
	struct imx952_dsi2_priv *dsi = priv_data;
	struct phy_configure_opts_mipi_dphy phy_cfg;
	unsigned long mode_flags = dsi->device.mode_flags;
	struct udevice *dev = dsi->device.dev;
	int ret;

	ret = imx952_dsi2_get_phy_configure_opts(dsi, timings, &phy_cfg, mode_flags,
						 lanes, format);
	if (ret < 0) {
		dev_dbg(dev, "failed to get phy cfg opts %d\n", ret);
		return ret;
	}

	*lane_mbps = DIV_ROUND_UP(phy_cfg.hs_clk_rate, USEC_PER_SEC);

	memcpy(&dsi->phy_cfg, &phy_cfg, sizeof(phy_cfg));

	dev_dbg(dev, "get lane_mbps %u\n", *lane_mbps);

	return 0;
}

static int imx952_dsi2_phy_get_timing(void *priv_data, unsigned int lane_mbps,
				      struct mipi_dsi_phy_timing *timing)
{
	struct imx952_dsi2_priv *dsi = priv_data;
	unsigned int lp2hs_m, lp2hs_b;
	unsigned int hs2lp_m, hs2lp_b;

	if (dsi->hs2lp_lp2hs_quirk) {
		timing->data_lp2hs = 0x10000;
		timing->data_hs2lp = 0x10000;
		dev_dbg(dsi->device.dev, "hs2lp_lp2hs_quirk\n");

		return 0;
	}

	/* PHY_LP2HS/HS2LP_TIME = DIV_ROUND_UP((lane_mbps * m), 100) + b */
	if (dsi->mode_flags & MIPI_DSI_CLOCK_NON_CONTINUOUS) {
		lp2hs_m = 13;
		lp2hs_b = 20;
		hs2lp_m = 7;
		hs2lp_b = 25;
	} else {
		lp2hs_m = 7;
		lp2hs_b = 20;
		hs2lp_m = 5;
		hs2lp_b = 10;
	}

	timing->data_lp2hs = (DIV_ROUND_UP(lane_mbps * lp2hs_m, 100) + lp2hs_b) << 16;
	timing->data_hs2lp = (DIV_ROUND_UP(lane_mbps * hs2lp_m, 100) + hs2lp_b) << 16;

	return 0;
}

static void
imx952_dsi2_phy_get_esc_clk_rate(void *priv_data, unsigned int *esc_clk_rate)
{
	struct imx952_dsi2_priv *dsi = priv_data;
	unsigned long pclk_rate;
	unsigned int div = 2;

	pclk_rate = clk_get_rate(dsi->clk_pixel);
	if (pclk_rate == 0)
		return;

	*esc_clk_rate = pclk_rate;
	while (*esc_clk_rate > MAX_ESC_CLK_RATE_HZ) {
		*esc_clk_rate = pclk_rate / div;
		div = div + 2;

		if (div > 126)
			return;
	}

	dsi->esc_clk_rate = *esc_clk_rate;

	debug("get esc_clk_rate = %u\n", *esc_clk_rate);
}

static const struct mipi_dsi_phy_ops imx952_dsi2_phy_ops = {
	.init = imx952_dsi2_phy_init,
	.power_on = imx952_dsi2_phy_power_on,
	.power_off = imx952_dsi2_phy_power_off,
	.get_interface = imx952_dsi2_phy_get_iface,
	.get_lane_mbps = imx952_dsi2_phy_get_lane_mbps,
	.get_timing = imx952_dsi2_phy_get_timing,
	.get_esc_clk_rate = imx952_dsi2_phy_get_esc_clk_rate,
};

static bool is_using_adv7535(struct udevice *panel)
{
	int ret;
	const char *compatible;

	ret = dev_read_string_index(panel, "compatible", 0, &compatible);
	if (!ret && !strcmp(compatible, "adi,adv7535"))
		return true;

	return false;
}

static int imx952_dsi2_attach(struct udevice *dev)
{
	struct imx952_dsi2_priv *priv = dev_get_priv(dev);
	struct mipi_dsi_device *device = &priv->device;
	struct mipi_dsi_panel_plat *mplat;
	struct display_timing timings;
	int ret, bpp;

	priv->panel = video_link_get_next_device(dev);
	if (!priv->panel || device_get_uclass_id(priv->panel) != UCLASS_PANEL) {
		dev_err(dev, "get panel device error\n");
		return -ENODEV;
	}

	mplat = dev_get_plat(priv->panel);
	mplat->device = device;

	ret = video_link_get_display_timings(&timings);
	if (ret) {
		dev_err(dev, "decode display timing error %d\n", ret);
		return ret;
	}

	bpp = mipi_dsi_pixel_format_to_bpp(device->format);
	if (bpp < 0) {
		dev_err(dev, "obtain the number of bits per pixel error %d\n", device->format);
		return bpp;
	}

	priv->lanes = device->lanes;
	priv->format = device->format;
	priv->adj = timings;

	if (is_using_adv7535(priv->panel) && priv->adj.pixelclock.typ >= 40000000)
		priv->phy_submode = true;
	else
		priv->phy_submode = false;

	priv->hs2lp_lp2hs_quirk = priv->phy_submode;

	ret = uclass_get_device(UCLASS_DSI_HOST, 0, &priv->dsi_host);
	if (ret) {
		dev_err(dev, "no video dsi2 host detected %d\n", ret);
		return ret;
	}

	ret = dsi_host_init(priv->dsi_host, device, &priv->adj, 4, &imx952_dsi2_phy_ops);
	if (ret) {
		dev_err(dev, "failed to initialize mipi dsi2 host\n");
		return ret;
	}

	return 0;
}

static int imx952_dsi2_set_backlight(struct udevice *dev, int percent)
{
	struct imx952_dsi2_priv *priv = dev_get_priv(dev);
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

static int imx952_dsi2_check_timing(struct udevice *dev, struct display_timing *timing)
{
	struct imx952_dsi2_priv *priv = dev_get_priv(dev);

	/* Ensure the bridge device attached to panel */
	if (!priv->panel) {
		dev_err(dev, "%s No panel device attached\n", __func__);
		return -ENOTCONN;
	}

	/* DSI force the Polarities as high */
	priv->adj.flags &= ~(DISPLAY_FLAGS_HSYNC_HIGH | DISPLAY_FLAGS_VSYNC_HIGH);
	priv->adj.flags |= DISPLAY_FLAGS_HSYNC_LOW | DISPLAY_FLAGS_VSYNC_LOW;

	*timing = priv->adj;

	return 0;
}

static int imx952_dsi2_get_clk(struct udevice *dev)
{
	struct imx952_dsi2_priv *dsi = dev_get_priv(dev);

	dsi->clk_pixel = devm_clk_get(dev, "pix");
	if (IS_ERR(dsi->clk_pixel))
		return PTR_ERR(dsi->clk_pixel);

	return 0;
}

static int imx952_dsi2_get_regmap(struct udevice *dev)
{
	struct imx952_dsi2_priv *dsi = dev_get_priv(dev);

	dsi->dsi_csr = syscon_regmap_lookup_by_phandle(dev, "nxp,display-dsi-csr");
	if (IS_ERR(dsi->dsi_csr)) {
		dev_err(dev, "failed to DSI CSR\n");
		return PTR_ERR(dsi->dsi_csr);
	}

	return 0;
}

static int imx952_dsi2_get_phy(struct udevice *dev)
{
	struct imx952_dsi2_priv *dsi = dev_get_priv(dev);
	int ret = 0;

	ret = generic_phy_get_by_name(dev, "dphy", &dsi->phy);
	if (ret) {
		dev_err(dev, "failed to get DPHY %d\n", ret);
		return ret;
	}

	return 0;
}

static int imx952_dsi2_get_mux(struct udevice *dev)
{
	struct imx952_dsi2_priv *dsi = dev_get_priv(dev);
	int ret = 0;

	ret = mux_get_by_index(dev, 0, &dsi->mux);
	if (ret)
		dev_err(dev, "failed to get mux controller used for pixel link select %d\n", ret);

	return ret;
}

static int imx952_dsi2_select_input(struct udevice *dev)
{
	struct imx952_dsi2_priv *dsi = dev_get_priv(dev);
	ofnode pl_ep_node, dsi_ep_node;
	u32 phandle;
	int ret;
	fdt_addr_t reg;

	pl_ep_node = video_link_get_ep_to_nextdev(dev);
	if (!ofnode_valid(pl_ep_node)) {
		dev_err(dev, "failed to get display pixel link endpoint\n");
		return -ENODEV;
	}

	ret = ofnode_read_u32(pl_ep_node, "remote-endpoint", &phandle);
	if (ret) {
		dev_err(dev, "failed to find remote-endpoint of display pixel link\n");
		return -ENODEV;
	}

	dsi_ep_node = ofnode_get_by_phandle(phandle);
	if (!ofnode_valid(dsi_ep_node)) {
		dev_err(dev, "failed to get ofnode from remote-endpoint of display pixel link\n");
		return -ENODEV;
	}

	reg = ofnode_get_addr_size_index_notrans(dsi_ep_node, 0, NULL);
	dsi->use_pl0 = reg ? false : true;

	return 0;
}

static int imx952_dsi2_parse_dt(struct udevice *dev)
{
	int ret;

	ret = imx952_dsi2_get_clk(dev);
	if (ret)
		return ret;

	ret = imx952_dsi2_get_regmap(dev);
	if (ret)
		return ret;

	ret = imx952_dsi2_get_phy(dev);
	if (ret)
		return ret;

	ret = imx952_dsi2_get_mux(dev);
	if (ret)
		return ret;

	ret = imx952_dsi2_select_input(dev);
	if (ret)
		return ret;

	return 0;
}

static int imx952_dsi2_imx_host_attach(void *priv_data,
				       struct mipi_dsi_device *device)
{
	struct imx952_dsi2_priv *dsi = priv_data;

	dsi->format = device->format;
	dsi->mode_flags = device->mode_flags;

	return 0;
}

static const struct dw_mipi_dsi2_host_ops imx952_dsi2_host_ops = {
	.attach = imx952_dsi2_imx_host_attach,
};

static int imx952_dsi2_probe(struct udevice *dev)
{
	struct imx952_dsi2_priv *priv = dev_get_priv(dev);
	struct dw_mipi_dsi2_plat_data *pdata = dev_get_plat(dev);
	int ret = 0;

	priv->device.dev = dev;
	priv->base = (void __iomem *)dev_read_addr(dev);
	if ((fdt_addr_t)priv->base == FDT_ADDR_T_NONE) {
		dev_err(dev, "failed to find dis2 v2 host register\n");
		return -EINVAL;
	}

	ret = imx952_dsi2_parse_dt(dev);
	if (ret)
		return ret;

	priv->clk_phy_pll = devm_clk_get(dev, "phy_pll");
	if (IS_ERR(priv->clk_phy_pll)) {
		dev_err(dev, "failed to get PHY PLL clk\n");
		return PTR_ERR(priv->clk_phy_pll);
	}

	pdata->max_data_lanes = 4;
	pdata->ipi_lanes = 1;
	pdata->ipi_fifo_depth = 960;
	pdata->ipi_mapping = DW_MIPI_DSI2_IPI_MAPPING_DPI_CONFIG1;
	pdata->cri_cmd_wr_pld_fifo_depth = 32;
	pdata->cri_cmd_rd_pld_fifo_depth = 128;
	pdata->host_ops = &imx952_dsi2_host_ops;
	pdata->priv_data = priv;

	return ret;
}

static int imx952_dsi2_remove(struct udevice *dev)
{
	struct imx952_dsi2_priv *priv = dev_get_priv(dev);
	int ret;

	if (priv->panel)
		device_remove(priv->panel, DM_REMOVE_NORMAL);

	ret = dsi_host_disable(priv->dsi_host);
	if (ret < 0 && ret != -ENOSYS)
		dev_err(dev, "failed to disable mipi dsi2 host\n");

	imx952_dsi2_phy_power_off(priv);

	return 0;
}

struct video_bridge_ops imx952_dsi2_ops = {
	.attach = imx952_dsi2_attach,
	.set_backlight = imx952_dsi2_set_backlight,
	.check_timing = imx952_dsi2_check_timing,
};

static const struct udevice_id imx952_dsi2_dt_ids[] = {
	{ .compatible = "nxp,imx952-mipi-dsi2", },
	{ /* sentinel */ }
};

U_BOOT_DRIVER(imx952_mipi_dsi2) = {
	.name		= "imx952_mipi_dsi2",
	.id		= UCLASS_VIDEO_BRIDGE,
	.of_match	= imx952_dsi2_dt_ids,
	.bind		= dm_scan_fdt_dev,
	.probe		= imx952_dsi2_probe,
	.remove		= imx952_dsi2_remove,
	.priv_auto	= sizeof(struct imx952_dsi2_priv),
	.plat_auto	= sizeof(struct dw_mipi_dsi2_plat_data),
	.ops		= &imx952_dsi2_ops,
};
