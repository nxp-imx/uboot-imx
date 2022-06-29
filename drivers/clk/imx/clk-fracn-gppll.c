// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2022 NXP
 *
 * Author: Alice Guo <alice.guo@nxp.com>
 */

#include <clk-uclass.h>
#include <div64.h>
#include <dm.h>
#include <linux/bitfield.h>
#include <linux/iopoll.h>
#include <linux/kernel.h>

#include "clk.h"

#define PLL_CTRL	0x00
#define CLKMUX_BYPASS	BIT(2)
#define CLKMUX_EN	BIT(1)
#define POWERUP		BIT(0)
#define PLL_NUMERATOR	0x40
#define PLL_MFN_MASK	GENMASK(31, 2)
#define PLL_DENOMINATOR	0x50
#define PLL_MFD_MASK	GENMASK(29, 0)
#define PLL_DIV		0x60
#define PLL_MFI_MASK	GENMASK(24, 16)
#define PLL_RDIV_MASK	GENMASK(15, 13)
#define PLL_ODIV_MASK	GENMASK(7, 0)
#define PLL_STATUS	0xf0
#define PLL_LOCK	BIT(0)

#define PLL_FRACN_GP(_rate, _mfi, _mfn, _mfd, _rdiv, _odiv)	\
	{							\
		.rate	=	(_rate),			\
		.mfi	=	(_mfi),				\
		.mfn	=	(_mfn),				\
		.mfd	=	(_mfd),				\
		.rdiv	=	(_rdiv),			\
		.odiv	=	(_odiv),			\
	}

static const struct imx93_pll_fracn_gp fracn_tbl[] = {
	PLL_FRACN_GP(650000000U, 81, 0, 1, 0, 3),
	PLL_FRACN_GP(594000000U, 198, 0, 1, 0, 8),
	PLL_FRACN_GP(560000000U, 70, 0, 1, 0, 3),
	PLL_FRACN_GP(498000000U, 83, 0, 1, 0, 4),
	PLL_FRACN_GP(484000000U, 121, 0, 1, 0, 6),
	PLL_FRACN_GP(445333333U, 167, 0, 1, 0, 9),
	PLL_FRACN_GP(400000000U, 50, 0, 1, 0, 3),
	PLL_FRACN_GP(393216000U, 81, 92, 100, 0, 5),
	PLL_FRACN_GP(300000000U, 150, 0, 1, 0, 12)
};

struct imx93_pll {
	struct clk	clk;
	void __iomem	*base;
	const struct imx93_pll_fracn_gp	*tbl;
};

#define to_imx93_pll(_c) container_of(_c, struct imx93_pll, clk)

static int imx93_wait_pll_lock(struct imx93_pll *pll)
{
	u32 val;

	return readl_poll_timeout(pll->base + PLL_STATUS, val, (val & PLL_LOCK), 200);
}

static const struct imx93_pll_fracn_gp *imx93_pll_get_cfg(struct imx93_pll *pll, ulong rate)
{
	const struct imx93_pll_fracn_gp	*tbl = pll->tbl;

	for (int i = 0; i < ARRAY_SIZE(fracn_tbl); i++)
		if (tbl[i].rate == rate)
			return &tbl[i];

	return NULL;
}

static ulong imx93_pll_set_rate(struct clk *clk, ulong rate)
{
	struct imx93_pll *pll = to_imx93_pll(clk);
	const struct imx93_pll_fracn_gp	*cfg = NULL;
	u32 tmp, pll_div, ana_mfn;
	int ret;

	cfg = imx93_pll_get_cfg(pll, rate);
	if (!cfg)
		return -EINVAL;

	tmp = readl_relaxed(pll->base + PLL_CTRL);
	tmp &= ~CLKMUX_EN;
	writel_relaxed(tmp, pll->base + PLL_CTRL);
	tmp &= ~POWERUP;
	writel_relaxed(tmp, pll->base + PLL_CTRL);
	tmp &= ~CLKMUX_BYPASS;
	writel_relaxed(tmp, pll->base + PLL_CTRL);

	pll_div = FIELD_PREP(PLL_MFI_MASK, cfg->mfi) |
		  FIELD_PREP(PLL_RDIV_MASK, cfg->rdiv) | cfg->odiv;
	writel_relaxed(pll_div, pll->base + PLL_DIV);
	writel_relaxed(cfg->mfd, pll->base + PLL_DENOMINATOR);
	writel_relaxed(FIELD_PREP(PLL_MFN_MASK, cfg->mfn), pll->base + PLL_NUMERATOR);

	udelay(5);

	tmp |= POWERUP;
	writel_relaxed(tmp, pll->base + PLL_CTRL);

	ret = imx93_wait_pll_lock(pll);
	if (ret)
		return ret;

	tmp |= CLKMUX_EN;
	writel_relaxed(tmp, pll->base + PLL_CTRL);

	ana_mfn = FIELD_GET(PLL_MFN_MASK, readl_relaxed(pll->base + PLL_STATUS));
	WARN(ana_mfn != cfg->mfn, "ana_mfn != cfg->mfn\n");

	return 0;
}

static ulong imx93_pll_get_rate(struct clk *clk)
{
	struct imx93_pll *pll = to_imx93_pll(clk);
	const struct imx93_pll_fracn_gp	*tbl = pll->tbl;
	u32 pll_numerator, pll_denominator, pll_div;
	u32 mfn, mfd, mfi, rdiv, odiv;
	u64 fvco = clk_get_parent_rate(clk);

	pll_numerator = readl_relaxed(pll->base + PLL_NUMERATOR);
	mfn = FIELD_GET(PLL_MFN_MASK, pll_numerator);

	pll_denominator = readl_relaxed(pll->base + PLL_DENOMINATOR);
	mfd = FIELD_GET(PLL_MFD_MASK, pll_denominator);

	pll_div = readl_relaxed(pll->base + PLL_DIV);
	mfi = FIELD_GET(PLL_MFI_MASK, pll_div);
	rdiv = FIELD_GET(PLL_RDIV_MASK, pll_div);
	odiv = FIELD_GET(PLL_ODIV_MASK, pll_div);

	for (int i = 0; i < ARRAY_SIZE(fracn_tbl); i++)
		if (tbl[i].mfn == mfn && tbl[i].mfd == mfd &&
		    tbl[i].rdiv == rdiv && tbl[i].odiv == odiv)
			return tbl[i].rate;

	if (rdiv == 0)
		rdiv = 1;

	switch (odiv) {
	case 0 /* 00000000b */:
		odiv = 2;
		break;
	case 1 /* 00000001b */:
		odiv = 3;
		break;
	case 127 /* 01111111b */:
		odiv = 255;
		break;
	default:
		break;
	}

	fvco = fvco * mfi * mfd + fvco * mfn;
	do_div(fvco, mfd * rdiv * odiv);

	return (ulong)fvco;
}

static int imx93_pll_enable(struct clk *clk)
{
	struct imx93_pll *pll = to_imx93_pll(clk);
	u32 val;
	int ret;

	val = readl_relaxed(pll->base + PLL_CTRL);
	if (val & POWERUP)
		return 0;

	val |= CLKMUX_BYPASS;
	writel_relaxed(val, pll->base + PLL_CTRL);
	val |= POWERUP;
	writel_relaxed(val, pll->base + PLL_CTRL);
	val |= CLKMUX_EN;
	writel_relaxed(val, pll->base + PLL_CTRL);

	ret = imx93_wait_pll_lock(pll);
	if (ret)
		return ret;

	val &= ~CLKMUX_BYPASS;
	writel_relaxed(val, pll->base + PLL_CTRL);

	return 0;
}

static int imx93_pll_disable(struct clk *clk)
{
	struct imx93_pll *pll = to_imx93_pll(clk);
	u32 val;

	val = readl_relaxed(pll->base + PLL_CTRL);
	val &= ~POWERUP;
	writel_relaxed(val, pll->base + PLL_CTRL);

	return 0;
}

static struct clk_ops imx93_pll_ops = {
	.set_rate = imx93_pll_set_rate,
	.get_rate = imx93_pll_get_rate,
	.enable = imx93_pll_enable,
	.disable = imx93_pll_disable,
};

struct clk *clk_register_imx93_pll(const char *name, const char *parent_name,
				   void __iomem *base)
{
	struct imx93_pll *pll;
	int ret;

	pll = kzalloc(sizeof(*pll), GFP_KERNEL);
	if (!pll)
		return ERR_PTR(-ENOMEM);

	pll->base = base;
	pll->tbl = fracn_tbl;

	ret = clk_register(&pll->clk, "imx93_pll", name, parent_name);
	if (ret) {
		printf("%s: failed to register pll: %d\n", __func__, ret);
		kfree(pll);
		return ERR_PTR(ret);
	}

	return &pll->clk;
}

U_BOOT_DRIVER(imx93_pll) = {
	.name = "imx93_pll",
	.id = UCLASS_CLK,
	.ops = &imx93_pll_ops,
	.flags = DM_FLAG_PRE_RELOC,
};
