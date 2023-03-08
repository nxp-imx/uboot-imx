// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2023 NXP
 */

#include <common.h>
#include <dm.h>
#include <malloc.h>
#include <power-domain-uclass.h>
#include <asm/io.h>
#include <dm/device-internal.h>
#include <dm/device.h>
#include <dt-bindings/power/imx8mm-power.h>
#include <dt-bindings/power/imx8mn-power.h>
#include <dt-bindings/power/imx8mp-power.h>
#include <clk.h>
#include <linux/delay.h>

#define BLK_SFT_RSTN	0x0
#define BLK_CLK_EN	0x4
#define BLK_MIPI_RESET_DIV	0x8 /* Mini/Nano/Plus DISPLAY_BLK_CTRL only */

#define DOMAIN_MAX_CLKS 4

struct imx8m_blk_ctrl_domain {
	struct clk clks[DOMAIN_MAX_CLKS];
	struct power_domain power_dev;
};

struct imx8m_blk_ctrl {
	void __iomem *base;
	struct power_domain bus_power_dev;
	struct imx8m_blk_ctrl_domain *domains;
};

struct imx8m_blk_ctrl_domain_data {
	const char *name;
	const char * const *clk_names;
	const char *gpc_name;
	int num_clks;
	u32 rst_mask;
	u32 clk_mask;
	u32 mipi_phy_rst_mask;
};

struct imx8m_blk_ctrl_data {
	int max_reg;
	const struct imx8m_blk_ctrl_domain_data *domains;
	int num_domains;
	u32 bus_rst_mask;
	u32 bus_clk_mask;
};

static int imx8m_blk_ctrl_request(struct power_domain *power_domain)
{
	return 0;
}

static int imx8m_blk_ctrl_free(struct power_domain *power_domain)
{
	return 0;
}

static int imx8m_blk_ctrl_enable_domain_clk(struct udevice *dev, ulong domain_id, bool enable)
{
	int ret, i;
	struct imx8m_blk_ctrl *priv = (struct imx8m_blk_ctrl *)dev_get_priv(dev);
	struct imx8m_blk_ctrl_data *drv_data =
		(struct imx8m_blk_ctrl_data *)dev_get_driver_data(dev);

	debug("%s num_clk %u\n", __func__, drv_data->domains[domain_id].num_clks);

	for (i = 0; i < drv_data->domains[domain_id].num_clks; i++) {
		debug("%s clk %s\n", __func__, drv_data->domains[domain_id].clk_names[i]);
		if (enable)
			ret = clk_enable(&priv->domains[domain_id].clks[i]);
		else
			ret = clk_disable(&priv->domains[domain_id].clks[i]);
		if (ret && ret != -ENOENT) {
			printf("Failed to %s domain clk %s\n", enable ? "enable" : "disable", drv_data->domains[domain_id].clk_names[i]);
			return ret;
		}
	}

	return 0;
}

static int imx8m_blk_ctrl_power_on(struct power_domain *power_domain)
{
	struct udevice *dev = power_domain->dev;
	struct imx8m_blk_ctrl *priv = (struct imx8m_blk_ctrl *)dev_get_priv(dev);
	struct imx8m_blk_ctrl_data *drv_data =
		(struct imx8m_blk_ctrl_data *)dev_get_driver_data(dev);
	int ret;

	debug("%s, id %lu\n", __func__, power_domain->id);

	if (!priv->domains[power_domain->id].power_dev.dev)
		return -ENODEV;

	ret = power_domain_on(&priv->bus_power_dev);
	if (ret < 0) {
		printf("Failed to power up bus domain %d\n", ret);
		return ret;
	}

	/* Enable bus clock and deassert bus reset */
	setbits_le32(priv->base + BLK_CLK_EN, drv_data->bus_clk_mask);
	setbits_le32(priv->base + BLK_SFT_RSTN, drv_data->bus_rst_mask);

	/* wait for reset to propagate */
	udelay(5);

	/* put devices into reset */
	clrbits_le32(priv->base + BLK_SFT_RSTN, drv_data->domains[power_domain->id].rst_mask);
	if (drv_data->domains[power_domain->id].mipi_phy_rst_mask)
		clrbits_le32(priv->base + BLK_MIPI_RESET_DIV, drv_data->domains[power_domain->id].mipi_phy_rst_mask);

	/* enable upstream and blk-ctrl clocks to allow reset to propagate */
	ret = imx8m_blk_ctrl_enable_domain_clk(dev, power_domain->id, true);
	if (ret) {
		printf("failed to enable clocks\n");
		goto bus_powerdown;
	}

	/* ungate clk */
	setbits_le32(priv->base + BLK_CLK_EN, drv_data->domains[power_domain->id].clk_mask);

	/* power up upstream GPC domain */
	ret = power_domain_on(&priv->domains[power_domain->id].power_dev);
	if (ret < 0) {
		printf("Failed to power up peripheral domain %d\n", ret);
		goto clk_disable;
	}

	/* wait for reset to propagate */
	udelay(5);

	/* release reset */
	setbits_le32(priv->base + BLK_SFT_RSTN, drv_data->domains[power_domain->id].rst_mask);
	if (drv_data->domains[power_domain->id].mipi_phy_rst_mask)
		setbits_le32(priv->base + BLK_MIPI_RESET_DIV, drv_data->domains[power_domain->id].mipi_phy_rst_mask);

	return 0;
clk_disable:
	imx8m_blk_ctrl_enable_domain_clk(dev, power_domain->id, false);
bus_powerdown:
	power_domain_off(&priv->bus_power_dev);
	return ret;
}

static int imx8m_blk_ctrl_power_off(struct power_domain *power_domain)
{
	struct udevice *dev = power_domain->dev;
	struct imx8m_blk_ctrl *priv = (struct imx8m_blk_ctrl *)dev_get_priv(dev);
	struct imx8m_blk_ctrl_data *drv_data =
		(struct imx8m_blk_ctrl_data *)dev_get_driver_data(dev);

	debug("%s, id %lu\n", __func__, power_domain->id);

	if (!priv->domains[power_domain->id].power_dev.dev)
		return -ENODEV;

	/* put devices into reset and disable clocks */
	if (drv_data->domains[power_domain->id].mipi_phy_rst_mask)
		clrbits_le32(priv->base + BLK_MIPI_RESET_DIV, drv_data->domains[power_domain->id].mipi_phy_rst_mask);

	/* assert reset */
	clrbits_le32(priv->base + BLK_SFT_RSTN, drv_data->domains[power_domain->id].rst_mask);

	/* gate clk */
	clrbits_le32(priv->base + BLK_CLK_EN, drv_data->domains[power_domain->id].clk_mask);

	/* power down upstream GPC domain */
	power_domain_off(&priv->domains[power_domain->id].power_dev);

	imx8m_blk_ctrl_enable_domain_clk(dev, power_domain->id, false);

	/* power down bus domain */
	power_domain_off(&priv->bus_power_dev);

	return 0;
}

static int imx8m_blk_ctrl_probe(struct udevice *dev)
{
	int ret, i, j;
	struct imx8m_blk_ctrl *priv = (struct imx8m_blk_ctrl *)dev_get_priv(dev);
	struct imx8m_blk_ctrl_data *drv_data =
		(struct imx8m_blk_ctrl_data *)dev_get_driver_data(dev);

	priv->base = dev_read_addr_ptr(dev);
	if (!priv->base)
		return -EINVAL;

	priv->domains = kcalloc(drv_data->num_domains, sizeof(struct imx8m_blk_ctrl_domain), GFP_KERNEL);

	ret = power_domain_get_by_name(dev, &priv->bus_power_dev, "bus");
	if (ret) {
		printf("Failed to power_domain_get_by_name %s\n", "bus");
		return ret;
	}

	for (j = 0; j < drv_data->num_domains; j++) {
		ret = power_domain_get_by_name(dev, &priv->domains[j].power_dev, drv_data->domains[j].gpc_name);
		if (ret)
			continue;

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

static int imx8m_blk_ctrl_remove(struct udevice *dev)
{
	struct imx8m_blk_ctrl *priv = (struct imx8m_blk_ctrl *)dev_get_priv(dev);

	kfree(priv->domains);

	return 0;
}

static const struct imx8m_blk_ctrl_domain_data imx8mm_disp_blk_ctl_domain_data[] = {
	[IMX8MM_DISPBLK_PD_CSI_BRIDGE] = {
		.name = "dispblk-csi-bridge",
		.clk_names = (const char *[]){ "csi-bridge-axi", "csi-bridge-apb",
					       "csi-bridge-core", },
		.num_clks = 3,
		.gpc_name = "csi-bridge",
		.rst_mask = BIT(0) | BIT(1) | BIT(2),
		.clk_mask = BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(4) | BIT(5),
	},
	[IMX8MM_DISPBLK_PD_LCDIF] = {
		.name = "dispblk-lcdif",
		.clk_names = (const char *[]){ "lcdif-axi", "lcdif-apb", "lcdif-pix", },
		.num_clks = 3,
		.gpc_name = "lcdif",
		.clk_mask = BIT(6) | BIT(7),
	},
	[IMX8MM_DISPBLK_PD_MIPI_DSI] = {
		.name = "dispblk-mipi-dsi",
		.clk_names = (const char *[]){ "dsi-pclk", "dsi-ref", },
		.num_clks = 2,
		.gpc_name = "mipi-dsi",
		.rst_mask = BIT(5),
		.clk_mask = BIT(8) | BIT(9),
		.mipi_phy_rst_mask = BIT(17),
	},
	[IMX8MM_DISPBLK_PD_MIPI_CSI] = {
		.name = "dispblk-mipi-csi",
		.clk_names = (const char *[]){ "csi-aclk", "csi-pclk" },
		.num_clks = 2,
		.gpc_name = "mipi-csi",
		.rst_mask = BIT(3) | BIT(4),
		.clk_mask = BIT(10) | BIT(11),
		.mipi_phy_rst_mask = BIT(16),
	},
};

static const struct imx8m_blk_ctrl_data imx8mm_disp_blk_ctl_dev_data = {
	.max_reg = 0x2c,
	.domains = imx8mm_disp_blk_ctl_domain_data,
	.num_domains = ARRAY_SIZE(imx8mm_disp_blk_ctl_domain_data),
	.bus_rst_mask = BIT(6),
	.bus_clk_mask = BIT(12),
};

static const struct imx8m_blk_ctrl_domain_data imx8mn_disp_blk_ctl_domain_data[] = {
	[IMX8MN_DISPBLK_PD_MIPI_DSI] = {
		.name = "dispblk-mipi-dsi",
		.clk_names = (const char *[]){ "dsi-pclk", "dsi-ref", },
		.num_clks = 2,
		.gpc_name = "mipi-dsi",
		.rst_mask = BIT(0) | BIT(1),
		.clk_mask = BIT(0) | BIT(1),
		.mipi_phy_rst_mask = BIT(17),
	},
	[IMX8MN_DISPBLK_PD_MIPI_CSI] = {
		.name = "dispblk-mipi-csi",
		.clk_names = (const char *[]){ "csi-aclk", "csi-pclk" },
		.num_clks = 2,
		.gpc_name = "mipi-csi",
		.rst_mask = BIT(2) | BIT(3),
		.clk_mask = BIT(2) | BIT(3),
		.mipi_phy_rst_mask = BIT(16),
	},
	[IMX8MN_DISPBLK_PD_LCDIF] = {
		.name = "dispblk-lcdif",
		.clk_names = (const char *[]){ "lcdif-axi", "lcdif-apb", "lcdif-pix", },
		.num_clks = 3,
		.gpc_name = "lcdif",
		.rst_mask = BIT(4) | BIT(5),
		.clk_mask = BIT(4) | BIT(5),
	},
	[IMX8MN_DISPBLK_PD_ISI] = {
		.name = "dispblk-isi",
		.clk_names = (const char *[]){ "disp_axi", "disp_apb", "disp_axi_root",
						"disp_apb_root"},
		.num_clks = 4,
		.gpc_name = "isi",
		.rst_mask = BIT(6) | BIT(7),
		.clk_mask = BIT(6) | BIT(7),
	},
};

static const struct imx8m_blk_ctrl_data imx8mn_disp_blk_ctl_dev_data = {
	.max_reg = 0x84,
	.domains = imx8mn_disp_blk_ctl_domain_data,
	.num_domains = ARRAY_SIZE(imx8mn_disp_blk_ctl_domain_data),
	.bus_rst_mask = BIT(8),
	.bus_clk_mask = BIT(8),
};

static const struct imx8m_blk_ctrl_domain_data imx8mp_media_blk_ctl_domain_data[] = {
	[IMX8MP_MEDIABLK_PD_MIPI_DSI_1] = {
		.name = "mediablk-mipi-dsi-1",
		.clk_names = (const char *[]){ "apb", "phy", },
		.num_clks = 2,
		.gpc_name = "mipi-dsi1",
		.rst_mask = BIT(0) | BIT(1),
		.clk_mask = BIT(0) | BIT(1),
		.mipi_phy_rst_mask = BIT(17),
	},
	[IMX8MP_MEDIABLK_PD_MIPI_CSI2_1] = {
		.name = "mediablk-mipi-csi2-1",
		.clk_names = (const char *[]){ "apb", "cam1" },
		.num_clks = 2,
		.gpc_name = "mipi-csi1",
		.rst_mask = BIT(2) | BIT(3),
		.clk_mask = BIT(2) | BIT(3),
		.mipi_phy_rst_mask = BIT(16),
	},
	[IMX8MP_MEDIABLK_PD_LCDIF_1] = {
		.name = "mediablk-lcdif-1",
		.clk_names = (const char *[]){ "disp1", "apb", "axi", },
		.num_clks = 3,
		.gpc_name = "lcdif1",
		.rst_mask = BIT(4) | BIT(5) | BIT(23),
		.clk_mask = BIT(4) | BIT(5) | BIT(23),
	},
	[IMX8MP_MEDIABLK_PD_ISI] = {
		.name = "mediablk-isi",
		.clk_names = (const char *[]){ "axi", "apb" },
		.num_clks = 2,
		.gpc_name = "isi",
		.rst_mask = BIT(6) | BIT(7),
		.clk_mask = BIT(6) | BIT(7),
	},
	[IMX8MP_MEDIABLK_PD_MIPI_CSI2_2] = {
		.name = "mediablk-mipi-csi2-2",
		.clk_names = (const char *[]){ "apb", "cam2" },
		.num_clks = 2,
		.gpc_name = "mipi-csi2",
		.rst_mask = BIT(9) | BIT(10),
		.clk_mask = BIT(9) | BIT(10),
		.mipi_phy_rst_mask = BIT(30),
	},
	[IMX8MP_MEDIABLK_PD_LCDIF_2] = {
		.name = "mediablk-lcdif-2",
		.clk_names = (const char *[]){ "disp2", "apb", "axi", },
		.num_clks = 3,
		.gpc_name = "lcdif2",
		.rst_mask = BIT(11) | BIT(12) | BIT(24),
		.clk_mask = BIT(11) | BIT(12) | BIT(24),
	},
	[IMX8MP_MEDIABLK_PD_ISP] = {
		.name = "mediablk-isp",
		.clk_names = (const char *[]){ "isp", "axi", "apb" },
		.num_clks = 3,
		.gpc_name = "isp",
		.rst_mask = BIT(16) | BIT(17) | BIT(18),
		.clk_mask = BIT(16) | BIT(17) | BIT(18),
	},
	[IMX8MP_MEDIABLK_PD_DWE] = {
		.name = "mediablk-dwe",
		.clk_names = (const char *[]){ "axi", "apb" },
		.num_clks = 2,
		.gpc_name = "dwe",
		.rst_mask = BIT(19) | BIT(20) | BIT(21),
		.clk_mask = BIT(19) | BIT(20) | BIT(21),
	},
	[IMX8MP_MEDIABLK_PD_MIPI_DSI_2] = {
		.name = "mediablk-mipi-dsi-2",
		.clk_names = (const char *[]){ "phy", },
		.num_clks = 1,
		.gpc_name = "mipi-dsi2",
		.rst_mask = BIT(22),
		.clk_mask = BIT(22),
		.mipi_phy_rst_mask = BIT(29),
	},
};

static const struct imx8m_blk_ctrl_data imx8mp_media_blk_ctl_dev_data = {
	.max_reg = 0x138,
	.domains = imx8mp_media_blk_ctl_domain_data,
	.num_domains = ARRAY_SIZE(imx8mp_media_blk_ctl_domain_data),
	.bus_rst_mask = BIT(8),
	.bus_clk_mask = BIT(8),
};

static const struct udevice_id imx8m_blk_ctrl_ids[] = {
	{ .compatible = "fsl,imx8mm-disp-blk-ctrl", .data = (ulong)&imx8mm_disp_blk_ctl_dev_data },
	{ .compatible = "fsl,imx8mn-disp-blk-ctrl", .data = (ulong)&imx8mn_disp_blk_ctl_dev_data },
	{ .compatible = "fsl,imx8mp-media-blk-ctrl", .data = (ulong)&imx8mp_media_blk_ctl_dev_data },
	{ }
};

struct power_domain_ops imx8m_blk_ctrl_ops = {
	.request = imx8m_blk_ctrl_request,
	.rfree = imx8m_blk_ctrl_free,
	.on = imx8m_blk_ctrl_power_on,
	.off = imx8m_blk_ctrl_power_off,
};

U_BOOT_DRIVER(imx8m_blk_ctrl) = {
	.name = "imx8m_blk_ctrl",
	.id = UCLASS_POWER_DOMAIN,
	.of_match = imx8m_blk_ctrl_ids,
	.bind = dm_scan_fdt_dev,
	.probe = imx8m_blk_ctrl_probe,
	.remove = imx8m_blk_ctrl_remove,
	.priv_auto	= sizeof(struct imx8m_blk_ctrl),
	.ops = &imx8m_blk_ctrl_ops,
	.flags  = DM_FLAG_DEFAULT_PD_CTRL_OFF,
};
