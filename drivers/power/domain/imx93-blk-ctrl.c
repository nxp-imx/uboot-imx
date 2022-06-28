// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2022 NXP
 */

#include <common.h>
#include <dm.h>
#include <malloc.h>
#include <power-domain-uclass.h>
#include <asm/io.h>
#include <dm/device-internal.h>
#include <dm/device.h>
#include <dt-bindings/power/imx93-power.h>
#include <clk.h>

#define BLK_SFT_RSTN	0x0
#define BLK_CLK_EN	0x4

#define BLK_MAX_CLKS 4
#define DOMAIN_MAX_CLKS 4

struct imx93_blk_ctrl_domain {
	struct clk clks[DOMAIN_MAX_CLKS];
};

struct imx93_blk_ctrl {
	void __iomem *base;
	struct clk clks[BLK_MAX_CLKS];
	struct imx93_blk_ctrl_domain *domains;
};

struct imx93_blk_ctrl_domain_data {
	const char *name;
	const char * const *clk_names;
	int num_clks;
	u32 rst_mask;
	u32 clk_mask;
};

struct imx93_blk_ctrl_data {
	int max_reg;
	const struct imx93_blk_ctrl_domain_data *domains;
	const struct imx93_blk_ctrl_domain_data *bus;
	int num_domains;
};

static int imx93_blk_ctrl_request(struct power_domain *power_domain)
{
	return 0;
}

static int imx93_blk_ctrl_free(struct power_domain *power_domain)
{
	return 0;
}

static int imx93_blk_ctrl_enable_bus_clk(struct udevice *dev, bool enable)
{
	int ret, i;
	struct imx93_blk_ctrl *priv = (struct imx93_blk_ctrl *)dev_get_priv(dev);
	struct imx93_blk_ctrl_data *drv_data =
		(struct imx93_blk_ctrl_data *)dev_get_driver_data(dev);

	for (i = 0; i < drv_data->bus->num_clks; i++) {
		if (enable)
			ret = clk_enable(&priv->clks[i]);
		else
			ret = clk_disable(&priv->clks[i]);
		if (ret) {
			printf("Failed to %s bus clk %s\n", enable ? "enable" : "disable", drv_data->bus->clk_names[i]);
			return ret;
		}
	}

	return 0;
}

static int imx93_blk_ctrl_enable_domain_clk(struct udevice *dev, ulong domain_id, bool enable)
{
	int ret, i;
	struct imx93_blk_ctrl *priv = (struct imx93_blk_ctrl *)dev_get_priv(dev);
	struct imx93_blk_ctrl_data *drv_data =
		(struct imx93_blk_ctrl_data *)dev_get_driver_data(dev);

	debug("%s num_clk %u\n", __func__, drv_data->domains[domain_id].num_clks);

	for (i = 0; i < drv_data->domains[domain_id].num_clks; i++) {
		debug("%s clk %s\n", __func__, drv_data->domains[domain_id].clk_names[i]);
		if (enable)
			ret = clk_enable(&priv->domains[domain_id].clks[i]);
		else
			ret = clk_disable(&priv->domains[domain_id].clks[i]);
		if (ret) {
			printf("Failed to %s domain clk %s\n", enable ? "enable" : "disable", drv_data->domains[domain_id].clk_names[i]);
			return ret;
		}
	}

	return 0;
}

static int imx93_blk_ctrl_power_on(struct power_domain *power_domain)
{
	struct udevice *dev = power_domain->dev;
	struct imx93_blk_ctrl *priv = (struct imx93_blk_ctrl *)dev_get_priv(dev);
	struct imx93_blk_ctrl_data *drv_data =
		(struct imx93_blk_ctrl_data *)dev_get_driver_data(dev);

	debug("%s, id %lu\n", __func__, power_domain->id);

	imx93_blk_ctrl_enable_bus_clk(dev, true);
	imx93_blk_ctrl_enable_domain_clk(dev, power_domain->id, true);

	/* ungate clk */
	clrbits_le32(priv->base + BLK_CLK_EN, drv_data->domains[power_domain->id].clk_mask);

	/* release reset */
	setbits_le32(priv->base + BLK_SFT_RSTN, drv_data->domains[power_domain->id].rst_mask);

	return 0;
}

static int imx93_blk_ctrl_power_off(struct power_domain *power_domain)
{
	struct udevice *dev = power_domain->dev;
	struct imx93_blk_ctrl *priv = (struct imx93_blk_ctrl *)dev_get_priv(dev);
	struct imx93_blk_ctrl_data *drv_data =
		(struct imx93_blk_ctrl_data *)dev_get_driver_data(dev);

	debug("%s, id %lu\n", __func__, power_domain->id);

	/* assert reset */
	clrbits_le32(priv->base + BLK_SFT_RSTN, drv_data->domains[power_domain->id].rst_mask);

	/* gate clk */
	setbits_le32(priv->base + BLK_CLK_EN, drv_data->domains[power_domain->id].clk_mask);

	imx93_blk_ctrl_enable_domain_clk(dev, power_domain->id, false);

	imx93_blk_ctrl_enable_bus_clk(dev, false);

	return 0;
}

static int imx93_blk_ctrl_probe(struct udevice *dev)
{
	int ret, i, j;
	struct imx93_blk_ctrl *priv = (struct imx93_blk_ctrl *)dev_get_priv(dev);
	struct imx93_blk_ctrl_data *drv_data =
		(struct imx93_blk_ctrl_data *)dev_get_driver_data(dev);

	priv->base = dev_read_addr_ptr(dev);
	if (!priv->base)
		return -EINVAL;

	priv->domains = kcalloc(drv_data->num_domains, sizeof(struct imx93_blk_ctrl_domain), GFP_KERNEL);

	for (i = 0; i < drv_data->bus->num_clks; i++) {
		ret = clk_get_by_name(dev, drv_data->bus->clk_names[i], &priv->clks[i]);
		if (ret) {
			printf("Failed to get clk %s\n", drv_data->bus->clk_names[i]);
			return ret;
		}
	}

	for (j = 0; j < drv_data->num_domains; j++) {
		for (i = 0; i < drv_data->domains[j].num_clks; i++) {
			ret = clk_get_by_name(dev, drv_data->domains[j].clk_names[i], &priv->domains[j].clks[i]);
			if (ret) {
				printf("Failed to get clk %s\n", drv_data->domains[j].clk_names[i]);
				return ret;
			}
		}
	}

	return 0;
}

static int imx93_blk_ctrl_remove(struct udevice *dev)
{
	struct imx93_blk_ctrl *priv = (struct imx93_blk_ctrl *)dev_get_priv(dev);

	kfree(priv->domains);

	return 0;
}

static const struct imx93_blk_ctrl_domain_data imx93_media_blk_ctl_bus_data = {
	.clk_names = (const char *[]){ "axi", "apb", "nic", },
	.num_clks = 3,
};

static const struct imx93_blk_ctrl_domain_data imx93_media_blk_ctl_domain_data[] = {
	[IMX93_MEDIABLK_PD_MIPI_DSI] = {
		.name = "mediablk-mipi-dsi",
		.clk_names = (const char *[]){ "dsi" },
		.num_clks = 1,
		.rst_mask = BIT(11) | BIT(12),
		.clk_mask = BIT(11) | BIT(12),
	},
	[IMX93_MEDIABLK_PD_MIPI_CSI] = {
		.name = "mediablk-mipi-csi",
		.clk_names = (const char *[]){ "cam", "csi" },
		.num_clks = 2,
		.rst_mask = BIT(9) | BIT(10),
		.clk_mask = BIT(9) | BIT(10),
	},
	[IMX93_MEDIABLK_PD_PXP] = {
		.name = "mediablk-pxp",
		.clk_names = (const char *[]){ "pxp" },
		.num_clks = 1,
		.rst_mask = BIT(7) | BIT(8),
		.clk_mask = BIT(7) | BIT(8),
	},
	[IMX93_MEDIABLK_PD_LCDIF] = {
		.name = "mediablk-lcdif",
		.clk_names = (const char *[]){ "disp", "lcdif" },
		.num_clks = 2,
		.rst_mask = BIT(4) | BIT(5) | BIT(6),
		.clk_mask = BIT(4) | BIT(5) | BIT(6),
	},
	[IMX93_MEDIABLK_PD_ISI] = {
		.name = "mediablk-isi",
		.clk_names = (const char *[]){ "isi" },
		.num_clks = 1,
		.rst_mask = BIT(2) | BIT(3),
		.clk_mask = BIT(2) | BIT(3),
	},
};

static const struct imx93_blk_ctrl_data imx93_media_blk_ctl_dev_data = {
	.max_reg = 0x8,
	.domains = imx93_media_blk_ctl_domain_data,
	.bus = &imx93_media_blk_ctl_bus_data,
	.num_domains = ARRAY_SIZE(imx93_media_blk_ctl_domain_data),
};

static const struct udevice_id imx93_blk_ctrl_ids[] = {
	{ .compatible = "fsl,imx93-media-blk-ctrl", .data = (ulong)&imx93_media_blk_ctl_dev_data },
	{ }
};

struct power_domain_ops imx93_blk_ctrl_ops = {
	.request = imx93_blk_ctrl_request,
	.rfree = imx93_blk_ctrl_free,
	.on = imx93_blk_ctrl_power_on,
	.off = imx93_blk_ctrl_power_off,
};

U_BOOT_DRIVER(imx93_blk_ctrl) = {
	.name = "imx93_blk_ctrl",
	.id = UCLASS_POWER_DOMAIN,
	.of_match = imx93_blk_ctrl_ids,
	.bind = dm_scan_fdt_dev,
	.probe = imx93_blk_ctrl_probe,
	.remove = imx93_blk_ctrl_remove,
	.priv_auto	= sizeof(struct imx93_blk_ctrl),
	.ops = &imx93_blk_ctrl_ops,
};
