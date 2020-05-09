// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019 NXP
 *
 */

#include <common.h>
#include <clk.h>
#include <dm.h>
#include <regmap.h>
#include <reset.h>
#include <reset-uclass.h>
#include <regmap.h>
#include <dt-bindings/reset/imx8mm-dispmix.h>
#include <dt-bindings/reset/imx8mn-dispmix.h>

/* DISPMIX GPR registers */
#define DISPLAY_MIX_SFT_RSTN_CSR		0x00
#define DISPLAY_MIX_CLK_EN_CSR		0x00
#define GPR_MIPI_RESET_DIV			0x00

struct dispmix_reset_priv {
	struct regmap *map;
	bool active_low;
};

struct dispmix_reset_entry {
	uint32_t reg_off;
	uint32_t bit_off;
};

struct dispmix_reset_drvdata {
	const struct dispmix_reset_entry *resets;
	ulong nr_resets;
};

#define RESET_ENTRY(id, reg, bit)			\
	[id] = { .reg_off = (reg), .bit_off = (bit) }

static const struct dispmix_reset_entry imx8mm_sft_rstn[] = {
	/* dispmix reset entry */
	RESET_ENTRY(IMX8MM_CSI_BRIDGE_CHIP_RESET,
		    DISPLAY_MIX_SFT_RSTN_CSR, 0),
	RESET_ENTRY(IMX8MM_CSI_BRIDGE_IPG_HARD_ASYNC_RESET,
		    DISPLAY_MIX_SFT_RSTN_CSR, 1),
	RESET_ENTRY(IMX8MM_CSI_BRIDGE_CSI_HRESET,
		    DISPLAY_MIX_SFT_RSTN_CSR, 2),
	RESET_ENTRY(IMX8MM_CAMERA_PIXEL_RESET,
		    DISPLAY_MIX_SFT_RSTN_CSR, 3),
	RESET_ENTRY(IMX8MM_MIPI_CSI_I_PRESET,
		    DISPLAY_MIX_SFT_RSTN_CSR, 4),
	RESET_ENTRY(IMX8MM_MIPI_DSI_I_PRESET,
		    DISPLAY_MIX_SFT_RSTN_CSR, 5),
	RESET_ENTRY(IMX8MM_BUS_RSTN_BLK_SYNC,
		    DISPLAY_MIX_SFT_RSTN_CSR, 6),
};

static const struct dispmix_reset_entry imx8mm_clk_en[] = {
	/* dispmix clock enable entry */
	RESET_ENTRY(IMX8MM_CSI_BRIDGE_CSI_HCLK_EN,
		    DISPLAY_MIX_CLK_EN_CSR,  0),
	RESET_ENTRY(IMX8MM_CSI_BRIDGE_SPU_CLK_EN,
		    DISPLAY_MIX_CLK_EN_CSR,  1),
	RESET_ENTRY(IMX8MM_CSI_BRIDGE_MEM_WRAPPER_CLK_EN,
		    DISPLAY_MIX_CLK_EN_CSR,  2),
	RESET_ENTRY(IMX8MM_CSI_BRIDGE_IPG_CLK_EN,
		    DISPLAY_MIX_CLK_EN_CSR,  3),
	RESET_ENTRY(IMX8MM_CSI_BRIDGE_IPG_CLK_S_EN,
		    DISPLAY_MIX_CLK_EN_CSR,  4),
	RESET_ENTRY(IMX8MM_CSI_BRIDGE_IPG_CLK_S_RAW_EN,
		    DISPLAY_MIX_CLK_EN_CSR,  5),
	RESET_ENTRY(IMX8MM_LCDIF_APB_CLK_EN,
		    DISPLAY_MIX_CLK_EN_CSR,  6),
	RESET_ENTRY(IMX8MM_LCDIF_PIXEL_CLK_EN,
		    DISPLAY_MIX_CLK_EN_CSR,  7),
	RESET_ENTRY(IMX8MM_MIPI_DSI_PCLK_EN,
		    DISPLAY_MIX_CLK_EN_CSR,  8),
	RESET_ENTRY(IMX8MM_MIPI_DSI_CLKREF_EN,
		    DISPLAY_MIX_CLK_EN_CSR,  9),
	RESET_ENTRY(IMX8MM_MIPI_CSI_ACLK_EN,
		    DISPLAY_MIX_CLK_EN_CSR, 10),
	RESET_ENTRY(IMX8MM_MIPI_CSI_PCLK_EN,
		    DISPLAY_MIX_CLK_EN_CSR, 11),
	RESET_ENTRY(IMX8MM_BUS_BLK_CLK_EN,
		    DISPLAY_MIX_CLK_EN_CSR, 12),
};

static const struct dispmix_reset_entry imx8mm_mipi_rst[] = {
	/* mipi lanes reset entry */
	RESET_ENTRY(IMX8MM_MIPI_S_RESET,
		    GPR_MIPI_RESET_DIV, 16),
	RESET_ENTRY(IMX8MM_MIPI_M_RESET,
		    GPR_MIPI_RESET_DIV, 17),
};

static const struct dispmix_reset_entry imx8mn_sft_rstn[] = {
	/* dispmix reset entry */
	RESET_ENTRY(IMX8MN_MIPI_DSI_PCLK_RESET,
		    DISPLAY_MIX_SFT_RSTN_CSR, 0),
	RESET_ENTRY(IMX8MN_MIPI_DSI_CLKREF_RESET,
		    DISPLAY_MIX_SFT_RSTN_CSR, 1),
	RESET_ENTRY(IMX8MN_MIPI_CSI_PCLK_RESET,
		    DISPLAY_MIX_SFT_RSTN_CSR, 2),
	RESET_ENTRY(IMX8MN_MIPI_CSI_ACLK_RESET,
		    DISPLAY_MIX_SFT_RSTN_CSR, 3),
	RESET_ENTRY(IMX8MN_LCDIF_PIXEL_CLK_RESET,
		    DISPLAY_MIX_SFT_RSTN_CSR, 4),
	RESET_ENTRY(IMX8MN_LCDIF_APB_CLK_RESET,
		    DISPLAY_MIX_SFT_RSTN_CSR, 5),
	RESET_ENTRY(IMX8MN_ISI_PROC_CLK_RESET,
		    DISPLAY_MIX_SFT_RSTN_CSR, 6),
	RESET_ENTRY(IMX8MN_ISI_APB_CLK_RESET,
		    DISPLAY_MIX_SFT_RSTN_CSR, 7),
	RESET_ENTRY(IMX8MN_BUS_BLK_CLK_RESET,
		    DISPLAY_MIX_SFT_RSTN_CSR, 8),
};

static const struct dispmix_reset_entry imx8mn_clk_en[] = {
	/* dispmix clock enable entry */
	RESET_ENTRY(IMX8MN_MIPI_DSI_PCLK_EN,
		    DISPLAY_MIX_CLK_EN_CSR,  0),
	RESET_ENTRY(IMX8MN_MIPI_DSI_CLKREF_EN,
		    DISPLAY_MIX_CLK_EN_CSR,  1),
	RESET_ENTRY(IMX8MN_MIPI_CSI_PCLK_EN,
		    DISPLAY_MIX_CLK_EN_CSR,  2),
	RESET_ENTRY(IMX8MN_MIPI_CSI_ACLK_EN,
		    DISPLAY_MIX_CLK_EN_CSR,  3),
	RESET_ENTRY(IMX8MN_LCDIF_PIXEL_CLK_EN,
		    DISPLAY_MIX_CLK_EN_CSR,  4),
	RESET_ENTRY(IMX8MN_LCDIF_APB_CLK_EN,
		    DISPLAY_MIX_CLK_EN_CSR,  5),
	RESET_ENTRY(IMX8MN_ISI_PROC_CLK_EN,
		    DISPLAY_MIX_CLK_EN_CSR,  6),
	RESET_ENTRY(IMX8MN_ISI_APB_CLK_EN,
		    DISPLAY_MIX_CLK_EN_CSR,  7),
	RESET_ENTRY(IMX8MN_BUS_BLK_CLK_EN,
		    DISPLAY_MIX_CLK_EN_CSR,  8),
};

static const struct dispmix_reset_entry imx8mn_mipi_rst[] = {
	/* mipi lanes reset entry */
	RESET_ENTRY(IMX8MN_MIPI_S_RESET,
		    GPR_MIPI_RESET_DIV, 16),
	RESET_ENTRY(IMX8MN_MIPI_M_RESET,
		    GPR_MIPI_RESET_DIV, 17),
};

static const struct dispmix_reset_drvdata imx8mm_sft_rstn_pdata = {
	.resets    = imx8mm_sft_rstn,
	.nr_resets = IMX8MM_DISPMIX_SFT_RSTN_NUM,
};

static const struct dispmix_reset_drvdata imx8mm_clk_en_pdata = {
	.resets    = imx8mm_clk_en,
	.nr_resets = IMX8MM_DISPMIX_CLK_EN_NUM,
};

static const struct dispmix_reset_drvdata imx8mm_mipi_rst_pdata = {
	.resets    = imx8mm_mipi_rst,
	.nr_resets = IMX8MM_MIPI_RESET_NUM,
};

static const struct dispmix_reset_drvdata imx8mn_sft_rstn_pdata = {
	.resets    = imx8mn_sft_rstn,
	.nr_resets = IMX8MN_DISPMIX_SFT_RSTN_NUM,
};

static const struct dispmix_reset_drvdata imx8mn_clk_en_pdata = {
	.resets    = imx8mn_clk_en,
	.nr_resets = IMX8MN_DISPMIX_CLK_EN_NUM,
};

static const struct dispmix_reset_drvdata imx8mn_mipi_rst_pdata = {
	.resets    = imx8mn_mipi_rst,
	.nr_resets = IMX8MN_MIPI_RESET_NUM,
};

static const struct udevice_id dispmix_reset_dt_ids[] = {
	{
		.compatible = "fsl,imx8mm-dispmix-sft-rstn",
		.data = (ulong)&imx8mm_sft_rstn_pdata,
	},
	{
		.compatible = "fsl,imx8mm-dispmix-clk-en",
		.data = (ulong)&imx8mm_clk_en_pdata,
	},
	{
		.compatible = "fsl,imx8mm-dispmix-mipi-rst",
		.data = (ulong)&imx8mm_mipi_rst_pdata,
	},
	{
		.compatible = "fsl,imx8mn-dispmix-sft-rstn",
		.data = (ulong)&imx8mn_sft_rstn_pdata,
	},
	{
		.compatible = "fsl,imx8mn-dispmix-clk-en",
		.data = (ulong)&imx8mn_clk_en_pdata,
	},
	{
		.compatible = "fsl,imx8mn-dispmix-mipi-rst",
		.data = (ulong)&imx8mn_mipi_rst_pdata,
	},
	{ /* sentinel */ }
};

static int dispmix_reset_assert(struct reset_ctl *rst)
{
	const struct dispmix_reset_entry *rstent;
	struct dispmix_reset_priv *priv = (struct dispmix_reset_priv *)dev_get_priv(rst->dev);
	const struct dispmix_reset_drvdata *drvdata = (const struct dispmix_reset_drvdata *)dev_get_driver_data(rst->dev);


	if (rst->id >= drvdata->nr_resets) {
		pr_info("dispmix reset: %lu is not a valid line\n", rst->id);
		return -EINVAL;
	}

	rstent = &drvdata->resets[rst->id];

	regmap_update_bits(priv->map, rstent->reg_off,
			   1 << rstent->bit_off,
			   !priv->active_low << rstent->bit_off);

	return 0;
}

static int dispmix_reset_deassert(struct reset_ctl *rst)
{
	const struct dispmix_reset_entry *rstent;
	struct dispmix_reset_priv *priv = (struct dispmix_reset_priv *)dev_get_priv(rst->dev);
	const struct dispmix_reset_drvdata *drvdata =
		(const struct dispmix_reset_drvdata *)dev_get_driver_data(rst->dev);


	if (rst->id >= drvdata->nr_resets) {
		pr_info("dispmix reset: %lu is not a valid line\n", rst->id);
		return -EINVAL;
	}

	rstent = &drvdata->resets[rst->id];

	regmap_update_bits(priv->map, rstent->reg_off,
			   1 << rstent->bit_off,
			   !!priv->active_low << rstent->bit_off);

	return 0;
}

static int dispmix_reset_free(struct reset_ctl *rst)
{
	return 0;
}

static int dispmix_reset_request(struct reset_ctl *rst)
{
	return 0;
}

static const struct reset_ops dispmix_reset_ops = {
	.request = dispmix_reset_request,
	.rfree = dispmix_reset_free,
	.rst_assert   = dispmix_reset_assert,
	.rst_deassert = dispmix_reset_deassert,
};

static int dispmix_reset_probe(struct udevice *dev)
{
	struct dispmix_reset_priv *priv = (struct dispmix_reset_priv *)dev_get_priv(dev);
	int ret;

	priv->active_low = dev_read_bool(dev, "active_low");

	ret = regmap_init_mem(dev_ofnode(dev), &priv->map);
	if (ret) {
		debug("%s: Could not initialize regmap (err = %d)\n", dev->name,
		      ret);
		return ret;
	}

	return 0;
}

U_BOOT_DRIVER(dispmix_reset) = {
	.name = "dispmix_reset",
	.id = UCLASS_RESET,
	.of_match = dispmix_reset_dt_ids,
	.ops = &dispmix_reset_ops,
	.probe = dispmix_reset_probe,
	.priv_auto_alloc_size = sizeof(struct dispmix_reset_priv),
};
