// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019 NXP
 *
 */

#include <common.h>
#include <clk.h>
#include <dm.h>
#include <display.h>
#include <video.h>
#include <video_bridge.h>
#include <video_link.h>
#include <asm/io.h>
#include <dm/device-internal.h>
#include <linux/iopoll.h>
#include <linux/err.h>
#include <clk.h>

#include <asm/arch/imx8_lvds.h>
#include <asm/arch/imx8_mipi_dsi.h>
#include <power-domain.h>
#include <asm/arch/lpcg.h>
#include <asm/arch/sci/sci.h>
#include <regmap.h>
#include <syscon.h>

#define FLAG_COMBO	BIT(1)

#define LDB_PHY_OFFSET 0x1000
#define MIPI_PHY_OFFSET 0x8000

struct imx8_ldb_priv {
	struct regmap *gpr;
	struct udevice *conn_dev;
	u32 ldb_id;
	struct display_timing timings;
};

static int imx8_ldb_soc_setup(struct udevice *dev, sc_pm_clock_rate_t pixel_clock)
{
	sc_err_t err;
	sc_rsrc_t lvds_rsrc, mipi_rsrc;
	const char *pd_name;
	struct imx8_ldb_priv *priv = dev_get_priv(dev);
	ulong flag = dev_get_driver_data(dev);
	int lvds_id = priv->ldb_id;

	struct power_domain pd;
	int ret;

	debug("%s\n", __func__);

	if (lvds_id == 0) {
		lvds_rsrc = SC_R_LVDS_0;
		mipi_rsrc = SC_R_MIPI_0;
		pd_name = "lvds0_power_domain";
	} else {
		lvds_rsrc = SC_R_LVDS_1;
		mipi_rsrc = SC_R_MIPI_1;
		pd_name = "lvds1_power_domain";
	}
	/* Power up LVDS */
	if (!power_domain_lookup_name(pd_name, &pd)) {
		ret = power_domain_on(&pd);
		if (ret) {
			printf("%s Power up failed! (error = %d)\n", pd_name, ret);
			return -EIO;
		}
	} else {
		printf("%s lookup failed!\n", pd_name);
		return -EIO;
	}

	/* Setup clocks */
	err = sc_pm_set_clock_rate(-1, lvds_rsrc, SC_PM_CLK_BYPASS, &pixel_clock);
	if (err != SC_ERR_NONE) {
		printf("LVDS set rate SC_PM_CLK_BYPASS failed! (error = %d)\n", err);
		return -EIO;
	}

	err = sc_pm_set_clock_rate(-1, lvds_rsrc, SC_PM_CLK_PER, &pixel_clock);
	if (err != SC_ERR_NONE) {
		printf("LVDS set rate SC_PM_CLK_BYPASS failed! (error = %d)\n", err);
		return -EIO;
	}

	err = sc_pm_set_clock_rate(-1, lvds_rsrc, SC_PM_CLK_PHY, &pixel_clock);
	if (err != SC_ERR_NONE) {
		printf("LVDS set rate SC_PM_CLK_BYPASS failed! (error = %d)\n", err);
		return -EIO;
	}

	if (flag & FLAG_COMBO) {
		/* For QXP, there is only one DC, and two pixel links to each LVDS with a mux provided.
		  * We connect LVDS0 to pixel link 0, lVDS1 to pixel link 1 from DC
		  */

		/* Configure to LVDS mode not MIPI DSI */
		err = sc_misc_set_control(-1, mipi_rsrc, SC_C_MODE, 1);
		if (err != SC_ERR_NONE) {
			printf("LVDS sc_misc_set_control SC_C_MODE failed! (error = %d)\n", err);
			return -EIO;
		}

		/* Configure to LVDS mode with single channel */
		err = sc_misc_set_control(-1, mipi_rsrc, SC_C_DUAL_MODE, 0);
		if (err != SC_ERR_NONE) {
			printf("LVDS sc_misc_set_control SC_C_DUAL_MODE failed! (error = %d)\n", err);
			return -EIO;
		}

		err = sc_misc_set_control(-1, mipi_rsrc, SC_C_PXL_LINK_SEL, lvds_id);
		if (err != SC_ERR_NONE) {
			printf("LVDS sc_misc_set_control SC_C_PXL_LINK_SEL failed! (error = %d)\n", err);
			return -EIO;
		}
	}

	err = sc_pm_clock_enable(-1, lvds_rsrc, SC_PM_CLK_BYPASS, true, false);
	if (err != SC_ERR_NONE) {
		printf("LVDS enable clock SC_PM_CLK_BYPASS failed! (error = %d)\n", err);
		return -EIO;
	}

	err = sc_pm_clock_enable(-1, lvds_rsrc, SC_PM_CLK_PER, true, false);
	if (err != SC_ERR_NONE) {
		printf("LVDS enable clock SC_PM_CLK_PER failed! (error = %d)\n", err);
		return -EIO;
	}

	err = sc_pm_clock_enable(-1, lvds_rsrc, SC_PM_CLK_PHY, true, false);
	if (err != SC_ERR_NONE) {
		printf("LVDS enable clock SC_PM_CLK_PHY failed! (error = %d)\n", err);
		return -EIO;
	}

	return 0;
}

void imx8_ldb_configure(struct udevice *dev)
{
	uint32_t mode;
	uint32_t phy_setting;
	struct imx8_ldb_priv *priv = dev_get_priv(dev);
	ulong flag = dev_get_driver_data(dev);

	if (flag & FLAG_COMBO) {
		mode =
			IMX_LVDS_SET_FIELD(LVDS_CTRL_CH0_MODE, LVDS_CTRL_CH0_MODE__DI0) |
			IMX_LVDS_SET_FIELD(LVDS_CTRL_CH0_DATA_WIDTH, LVDS_CTRL_CH0_DATA_WIDTH__24BIT) |
			IMX_LVDS_SET_FIELD(LVDS_CTRL_CH0_BIT_MAP, LVDS_CTRL_CH0_BIT_MAP__JEIDA);

		phy_setting = 0x4 << 5 | 0x4 << 2 | 1 << 1 | 0x1;
		regmap_write(priv->gpr, LDB_PHY_OFFSET + LVDS_PHY_CTRL, phy_setting);
		regmap_write(priv->gpr, LDB_PHY_OFFSET + LVDS_CTRL, mode);
		regmap_write(priv->gpr, LDB_PHY_OFFSET + MIPIv2_CSR_TX_ULPS, 0);
		regmap_write(priv->gpr, LDB_PHY_OFFSET + MIPIv2_CSR_PXL2DPI, MIPI_CSR_PXL2DPI_24_BIT);

		/* Power up PLL in MIPI DSI PHY */
		regmap_write(priv->gpr, MIPI_PHY_OFFSET + DPHY_PD_PLL, 0);
		regmap_write(priv->gpr, MIPI_PHY_OFFSET + DPHY_PD_TX, 0);
	} else {
		mode =
			IMX_LVDS_SET_FIELD(LVDS_CTRL_CH0_MODE, LVDS_CTRL_CH0_MODE__DI0) |
			IMX_LVDS_SET_FIELD(LVDS_CTRL_CH0_DATA_WIDTH, LVDS_CTRL_CH0_DATA_WIDTH__24BIT) |
			IMX_LVDS_SET_FIELD(LVDS_CTRL_CH0_BIT_MAP, LVDS_CTRL_CH0_BIT_MAP__JEIDA) |
			IMX_LVDS_SET_FIELD(LVDS_CTRL_CH0_10BIT_ENABLE, LVDS_CTRL_CH0_10BIT_ENABLE__10BIT) |
			IMX_LVDS_SET_FIELD(LVDS_CTRL_DI0_DATA_WIDTH, LVDS_CTRL_DI0_DATA_WIDTH__USE_30BIT);

		regmap_write(priv->gpr, LDB_PHY_OFFSET + LVDS_CTRL, mode);

		phy_setting =
			LVDS_PHY_CTRL_RFB_MASK |
			LVDS_PHY_CTRL_CH0_EN_MASK |
			(0 << LVDS_PHY_CTRL_M_SHIFT) |
			(0x04 << LVDS_PHY_CTRL_CCM_SHIFT) |
			(0x04 << LVDS_PHY_CTRL_CA_SHIFT);
		regmap_write(priv->gpr, LDB_PHY_OFFSET + LVDS_PHY_CTRL, phy_setting);
	}
}

int imx8_ldb_read_timing(struct udevice *dev, struct display_timing *timing)
{
	struct imx8_ldb_priv *priv = dev_get_priv(dev);

	if (dev->platdata == NULL)
		return -EINVAL;

	if (timing) {
		memcpy(timing, &priv->timings, sizeof(struct display_timing));
		return 0;
	}

	return -EINVAL;
}

int imx8_ldb_enable(struct udevice *dev, int panel_bpp,
		      const struct display_timing *timing)
{
	struct imx8_ldb_priv *priv = dev_get_priv(dev);
	int ret;

	if (dev->platdata == NULL) {
		imx8_ldb_soc_setup(dev, timing->pixelclock.typ);
		imx8_ldb_configure(dev);
	} else {

		display_enable(dev->parent, panel_bpp, &priv->timings);

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
	}

	return 0;
}

static int imx8_ldb_probe(struct udevice *dev)
{
	struct imx8_ldb_priv *priv = dev_get_priv(dev);
	int ret;

	debug("%s\n", __func__);

	if (dev->platdata == NULL) {

		priv->gpr = syscon_regmap_lookup_by_phandle(dev, "gpr");
		if (IS_ERR(priv->gpr)) {
			printf("fail to get gpr regmap\n");
			return PTR_ERR(priv->gpr);
		}

		/* Require to add alias in DTB */
		priv->ldb_id = dev->req_seq;

		debug("ldb_id %u\n", priv->ldb_id);
	} else {
		priv->conn_dev = video_link_get_next_device(dev);
		if (!priv->conn_dev) {
			debug("can't find next device in video link\n");
		}

		ret = video_link_get_display_timings(&priv->timings);
		if (ret) {
			printf("decode display timing error %d\n", ret);
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
	}

	return 0;
}

static int imx8_ldb_bind(struct udevice *dev)
{
	ofnode lvds_ch_node;
	int ret = 0;

	lvds_ch_node = ofnode_find_subnode(dev_ofnode(dev), "lvds-channel@0");
	if (ofnode_valid(lvds_ch_node)) {
		ret = device_bind_ofnode(dev, dev->driver, "lvds-channel@0", (void *)1,
			lvds_ch_node, NULL);
		if (ret)
			printf("Error binding driver '%s': %d\n", dev->driver->name,
				ret);
	}

	return ret;
}

struct dm_display_ops imx8_ldb_ops = {
	.read_timing = imx8_ldb_read_timing,
	.enable = imx8_ldb_enable,
};

static const struct udevice_id imx8_ldb_ids[] = {
	{ .compatible = "fsl,imx8qm-ldb" },
	{ .compatible = "fsl,imx8qxp-ldb", .data = FLAG_COMBO, },
	{ }
};

U_BOOT_DRIVER(imx8_ldb) = {
	.name				= "imx8_ldb",
	.id				= UCLASS_DISPLAY,
	.of_match			= imx8_ldb_ids,
	.bind				= imx8_ldb_bind,
	.probe				= imx8_ldb_probe,
	.ops				= &imx8_ldb_ops,
	.priv_auto_alloc_size		= sizeof(struct imx8_ldb_priv),
};
