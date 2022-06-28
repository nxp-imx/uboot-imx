// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2022 NXP
 */

#include <common.h>
#include <asm/io.h>
#include <dm.h>
#include <errno.h>
#include <generic-phy.h>
#include <linux/bitfield.h>
#include <linux/bitops.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <clk.h>
#include <regmap.h>
#include <dm/device_compat.h>
#include <phy-mipi-dphy.h>
#include <div64.h>

/* DPHY registers */
#define DSI_REG			0x4c
#define  CFGCLKFREQRANGE_MASK	GENMASK(5, 0)
#define  CFGCLKFREQRANGE(x)	FIELD_PREP(CFGCLKFREQRANGE_MASK, (x))
#define  CLKSEL_MASK		GENMASK(7, 6)
#define  CLKSEL_STOP		FIELD_PREP(CLKSEL_MASK, 0)
#define  CLKSEL_GEN		FIELD_PREP(CLKSEL_MASK, 1)
#define  CLKSEL_EXT		FIELD_PREP(CLKSEL_MASK, 2)
#define  HSFREQRANGE_MASK	GENMASK(14, 8)
#define  HSFREQRANGE(x)		FIELD_PREP(HSFREQRANGE_MASK, (x))
#define  UPDATE_PLL		BIT(17)
#define  SHADOW_CLR		BIT(18)
#define  CLK_EXT		BIT(19)

#define DSI_WRITE_REG0		0x50
#define  M_MASK			GENMASK(9, 0)
#define  M(x)			FIELD_PREP(M_MASK, ((x) - 2))
#define  N_MASK			GENMASK(13, 10)
#define  N(x)			FIELD_PREP(N_MASK, ((x) - 1))
#define  VCO_CTRL_MASK		GENMASK(19, 14)
#define  VCO_CTRL(x)		FIELD_PREP(VCO_CTRL_MASK, (x))
#define  PROP_CTRL_MASK		GENMASK(25, 20)
#define  PROP_CTRL(x)		FIELD_PREP(PROP_CTRL_MASK, (x))
#define  INT_CTRL_MASK		GENMASK(31, 26)
#define  INT_CTRL(x)		FIELD_PREP(INT_CTRL_MASK, (x))

#define DSI_WRITE_REG1		0x54
#define  GMP_CTRL_MASK		GENMASK(1, 0)
#define  GMP_CTRL(x)		FIELD_PREP(GMP_CTRL_MASK, (x))
#define  CPBIAS_CTRL_MASK	GENMASK(8, 2)
#define  CPBIAS_CTRL(x)		FIELD_PREP(CPBIAS_CTRL_MASK, (x))
#define  PLL_SHADOW_CTRL	BIT(9)

#define DSI_READ_REG1		0x5c
#define  LOCK_PLL		BIT(10)

#define MHZ(x)			((x) * 1000000UL)

#define REF_CLK_RATE_MAX	MHZ(64)
#define REF_CLK_RATE_MIN	MHZ(2)
#define FOUT_MAX		MHZ(1250)
#define FOUT_MIN		MHZ(40)
#define FVCO_DIV_FACTOR		MHZ(80)

#define MBPS(x)			((x) * 1000000UL)

#define DATA_RATE_MAX_SPEED	MBPS(2500)
#define DATA_RATE_MIN_SPEED	MBPS(80)

#define M_MAX			625UL
#define M_MIN			64UL

#define N_MAX			16U
#define N_MIN			1U

#define PLL_LOCK_SLEEP		10
#define PLL_LOCK_TIMEOUT	1000

struct dw_dphy_cfg {
	u32 m;	/* PLL Feedback Multiplication Ratio */
	u32 n;	/* PLL Input Frequency Division Ratio */
};

struct dw_dphy_priv {
	struct regmap *regmap;
	struct clk ref_clk;
	struct clk cfg_clk;
	unsigned long ref_clk_rate;
};

struct dw_dphy_vco_prop {
	unsigned int max_fout;
	u8 vco_cntl;
	u8 prop_cntl;
};

struct dw_dphy_hsfreqrange {
	unsigned int max_mbps;
	u8 hsfreqrange;
};

/* Databook Table 3-13 Charge-pump Programmability */
static const struct dw_dphy_vco_prop vco_prop_map[] = {
	{   55, 0x3f, 0x0d },
	{   82, 0x37, 0x0d },
	{  110, 0x2f, 0x0d },
	{  165, 0x27, 0x0d },
	{  220, 0x1f, 0x0d },
	{  330, 0x17, 0x0d },
	{  440, 0x0f, 0x0d },
	{  660, 0x07, 0x0d },
	{ 1149, 0x03, 0x0d },
	{ 1152, 0x01, 0x0d },
	{ 1250, 0x01, 0x0e },
};

/* Databook Table 5-7 Frequency Ranges and Defaults */
static const struct dw_dphy_hsfreqrange hsfreqrange_map[] = {
	{   89, 0x00 },
	{   99, 0x10 },
	{  109, 0x20 },
	{  119, 0x30 },
	{  129, 0x01 },
	{  139, 0x11 },
	{  149, 0x21 },
	{  159, 0x31 },
	{  169, 0x02 },
	{  179, 0x12 },
	{  189, 0x22 },
	{  204, 0x32 },
	{  219, 0x03 },
	{  234, 0x13 },
	{  249, 0x23 },
	{  274, 0x33 },
	{  299, 0x04 },
	{  324, 0x14 },
	{  349, 0x25 },
	{  399, 0x35 },
	{  449, 0x05 },
	{  499, 0x16 },
	{  549, 0x26 },
	{  599, 0x37 },
	{  649, 0x07 },
	{  699, 0x18 },
	{  749, 0x28 },
	{  799, 0x39 },
	{  849, 0x09 },
	{  899, 0x19 },
	{  949, 0x29 },
	{  999, 0x3a },
	{ 1049, 0x0a },
	{ 1099, 0x1a },
	{ 1149, 0x2a },
	{ 1199, 0x3b },
	{ 1249, 0x0b },
	{ 1299, 0x1b },
	{ 1349, 0x2b },
	{ 1399, 0x3c },
	{ 1449, 0x0c },
	{ 1499, 0x1c },
	{ 1549, 0x2c },
	{ 1599, 0x3d },
	{ 1649, 0x0d },
	{ 1699, 0x1d },
	{ 1749, 0x2e },
	{ 1799, 0x3e },
	{ 1849, 0x0e },
	{ 1899, 0x1e },
	{ 1949, 0x2f },
	{ 1999, 0x3f },
	{ 2049, 0x0f },
	{ 2099, 0x40 },
	{ 2149, 0x41 },
	{ 2199, 0x42 },
	{ 2249, 0x43 },
	{ 2299, 0x44 },
	{ 2349, 0x45 },
	{ 2399, 0x46 },
	{ 2449, 0x47 },
	{ 2499, 0x48 },
	{ 2500, 0x49 },
};

static int phy_write(struct phy *phy, u32 value, unsigned int reg)
{
	struct dw_dphy_priv *priv = dev_get_priv(phy->dev);
	int ret;

	ret = regmap_write(priv->regmap, reg, value);
	if (ret < 0)
		dev_err(phy->dev, "failed to write reg %u: %d\n", reg, ret);
	return ret;
}

static inline unsigned long data_rate_to_fout(unsigned long data_rate)
{
	/* Fout is half of data rate */
	return data_rate / 2;
}

static int
dw_dphy_config_from_opts(struct phy *phy,
			 struct phy_configure_opts_mipi_dphy *dphy_opts,
			 struct dw_dphy_cfg *cfg)
{
	struct dw_dphy_priv *priv = dev_get_priv(phy->dev);
	unsigned long fin = priv->ref_clk_rate;
	unsigned long fout;
	unsigned long best_fout = 0;
	unsigned int fvco_div;
	unsigned int min_n, max_n, n, best_n;
	unsigned long m, best_m;
	unsigned long min_delta = ULONG_MAX;
	unsigned long tmp, delta;

	if (dphy_opts->hs_clk_rate < DATA_RATE_MIN_SPEED ||
	    dphy_opts->hs_clk_rate > DATA_RATE_MAX_SPEED) {
		dev_dbg(phy->dev, "invalid data rate per lane: %lu\n",
			dphy_opts->hs_clk_rate);
		return -EINVAL;
	}

	fout = data_rate_to_fout(dphy_opts->hs_clk_rate);

	/* Fout = Fvco / Fvco_div = (Fin * M) / (Fvco_div * N) */
	fvco_div = 8UL / min(DIV_ROUND_UP(fout, FVCO_DIV_FACTOR), 8UL);

	/* limitation: 2MHz <= Fin / N <= 8MHz */
	min_n = DIV_ROUND_UP(fin, MHZ(8));
	max_n = DIV_ROUND_DOWN_ULL(fin, MHZ(2));

	/* clamp possible N(s) */
	min_n = clamp(min_n, N_MIN, N_MAX);
	max_n = clamp(max_n, N_MIN, N_MAX);

	dev_dbg(phy->dev, "Fout = %lu, Fvco_div = %u, n_range = [%u, %u]\n",
		fout, fvco_div, min_n, max_n);

	for (n = min_n; n <= max_n; n++) {
		/* M = (Fout * N * Fvco_div) / Fin */
		tmp = fout * n * fvco_div;
		m = DIV_ROUND_CLOSEST(tmp, fin);

		/* check M range */
		if (m < M_MIN || m > M_MAX)
			continue;

		/* calculate temporary Fout */
		tmp = m * fin;
		do_div(tmp, n * fvco_div);
		if (tmp < FOUT_MIN || tmp > FOUT_MAX)
			continue;

		delta = abs(fout - tmp);
		if (delta < min_delta) {
			best_n = n;
			best_m = m;
			min_delta = delta;
			best_fout = tmp;
		}
	}

	if (best_fout) {
		cfg->m = best_m;
		cfg->n = best_n;
		dphy_opts->hs_clk_rate = best_fout * 2;
		dev_dbg(phy->dev, "best Fout = %lu, m = %u, n = %u\n",
			best_fout, cfg->m, cfg->n);
	} else {
		dev_dbg(phy->dev, "failed to find best Fout\n");
		return -EINVAL;
	}

	return 0;
}

static void dw_dphy_clear_shadow(struct phy *phy)
{
	/* Select clock generation first. */
	phy_write(phy, CLKSEL_GEN, DSI_REG);

	/* Clear shadow after clock selection is done a while. */
	udelay(2);
	phy_write(phy, CLKSEL_GEN | SHADOW_CLR, DSI_REG);

	/*
	 * A minimum pulse of 5ns on shadow_clear signal,
	 * according to Databook Figure 3-3 Initialization Timing Diagram.
	 */
	udelay(2);
	phy_write(phy, CLKSEL_GEN, DSI_REG);
}

static u32 dw_dphy_get_cfgclkrange(struct phy *phy)
{
	struct dw_dphy_priv *priv = dev_get_priv(phy->dev);

	return (clk_get_rate(&priv->cfg_clk) / MHZ(1) - 17) * 4;
}

static u8 dw_dphy_get_hsfreqrange(struct phy_configure_opts_mipi_dphy *dphy_opts)
{
	unsigned int mbps = dphy_opts->hs_clk_rate / MHZ(1);
	int i;

	for (i = 0; i < ARRAY_SIZE(hsfreqrange_map); i++)
		if (mbps <= hsfreqrange_map[i].max_mbps)
			return hsfreqrange_map[i].hsfreqrange;

	return 0;
}

static u8 dw_dphy_get_vco(struct phy_configure_opts_mipi_dphy *dphy_opts)
{
	unsigned int fout = data_rate_to_fout(dphy_opts->hs_clk_rate) / MHZ(1);
	int i;

	for (i = 0; i < ARRAY_SIZE(vco_prop_map); i++)
		if (fout <= vco_prop_map[i].max_fout)
			return vco_prop_map[i].vco_cntl;

	return 0;
}

static u8 dw_dphy_get_prop(struct phy_configure_opts_mipi_dphy *dphy_opts)
{
	unsigned int fout = data_rate_to_fout(dphy_opts->hs_clk_rate) / MHZ(1);
	int i;

	for (i = 0; i < ARRAY_SIZE(vco_prop_map); i++)
		if (fout <= vco_prop_map[i].max_fout)
			return vco_prop_map[i].prop_cntl;

	return 0;
}

static int dw_dphy_configure(struct phy *phy, void *params)
{
	struct dw_dphy_cfg cfg = { 0 };
	u32 val;
	int ret;
	struct phy_configure_opts_mipi_dphy *opts = (struct phy_configure_opts_mipi_dphy *)params;

	ret = dw_dphy_config_from_opts(phy, opts, &cfg);
	if (ret)
		return ret;

	dw_dphy_clear_shadow(phy);

	/* reg */
	val = CLKSEL_GEN |
	      CFGCLKFREQRANGE(dw_dphy_get_cfgclkrange(phy)) |
	      HSFREQRANGE(dw_dphy_get_hsfreqrange(opts));
	phy_write(phy, val, DSI_REG);

	/* w_reg0 */
	val = M(cfg.m) | N(cfg.n) | INT_CTRL(0) |
	      VCO_CTRL(dw_dphy_get_vco(opts)) |
	      PROP_CTRL(dw_dphy_get_prop(opts));
	phy_write(phy, val, DSI_WRITE_REG0);

	/* w_reg1 */
	phy_write(phy, GMP_CTRL(1) | CPBIAS_CTRL(0x10), DSI_WRITE_REG1);

	return 0;
}

static void dw_dphy_clear_reg(struct phy *phy)
{
	phy_write(phy, 0, DSI_REG);
	phy_write(phy, 0, DSI_WRITE_REG0);
	phy_write(phy, 0, DSI_WRITE_REG1);
}

static int dw_dphy_init(struct phy *phy)
{
	struct dw_dphy_priv *priv = dev_get_priv(phy->dev);
	int ret;

	ret = clk_prepare_enable(&priv->cfg_clk);
	if (ret < 0) {
		dev_err(phy->dev, "failed to enable config clock: %d\n", ret);
		return ret;
	}

	dw_dphy_clear_reg(phy);

	return 0;
}

static int dw_dphy_exit(struct phy *phy)
{
	struct dw_dphy_priv *priv = dev_get_priv(phy->dev);

	dw_dphy_clear_reg(phy);
	clk_disable_unprepare(&priv->cfg_clk);

	return 0;
}

static int dw_dphy_update_pll(struct phy *phy)
{
	struct dw_dphy_priv *priv = dev_get_priv(phy->dev);
	int ret;

	ret = regmap_update_bits(priv->regmap, DSI_REG, UPDATE_PLL, UPDATE_PLL);
	if (ret < 0) {
		dev_err(phy->dev, "failed to set UPDATE_PLL: %d\n", ret);
		return ret;
	}

	/*
	 * The updatepll signal should be asserted for a minimum of four clkin
	 * cycles, according to Databook Figure 3-3 Initialization Timing
	 * Diagram.
	 */
	udelay(10);

	ret = regmap_update_bits(priv->regmap, DSI_REG, UPDATE_PLL, 0);
	if (ret < 0) {
		dev_err(phy->dev, "failed to clear UPDATE_PLL: %d\n", ret);
		return ret;
	}

	return 0;
}

static int dw_dphy_power_on(struct phy *phy)
{
	struct dw_dphy_priv *priv = dev_get_priv(phy->dev);
	int ret;

	ret = clk_prepare_enable(&priv->ref_clk);
	if (ret < 0) {
		dev_err(phy->dev, "failed to enable ref clock: %d\n", ret);
		return ret;
	}

	/*
	 * At least 10 refclk cycles are required before updatePLL assertion,
	 * according to Databook Figure 3-3 Initialization Timing Diagram.
	 */
	udelay(10);

	ret = dw_dphy_update_pll(phy);
	if (ret < 0) {
		clk_disable_unprepare(&priv->ref_clk);
		return ret;
	}

	return 0;
}

static int dw_dphy_power_off(struct phy *phy)
{
	struct dw_dphy_priv *priv = dev_get_priv(phy->dev);

	dw_dphy_clear_reg(phy);
	clk_disable_unprepare(&priv->ref_clk);

	return 0;
}

static const struct phy_ops imx_dw_dphy_phy_ops = {
	.init = dw_dphy_init,
	.exit = dw_dphy_exit,
	.power_on = dw_dphy_power_on,
	.power_off = dw_dphy_power_off,
	.configure = dw_dphy_configure,
};

static int imx_dw_dphy_probe(struct udevice *dev)
{
	struct dw_dphy_priv *priv = dev_get_priv(dev);
	int ret;

	ret = regmap_init_mem(ofnode_get_parent(dev_ofnode(dev)), &priv->regmap);
	if (ret) {
		dev_err(dev, "failed to get regmap %d\n", ret);
		return ret;
	}

#if CONFIG_IS_ENABLED(CLK)
	ret = clk_get_by_name(dev, "phy_cfg", &priv->cfg_clk);
	if (ret) {
		dev_err(dev, "failed to get config clock %d\n", ret);
		return ret;
	}

	ret = clk_get_by_name(dev, "phy_ref", &priv->ref_clk);
	if (ret) {
		dev_err(dev, "failed to get ref clock %d\n", ret);
		return ret;
	}

	priv->ref_clk_rate = clk_get_rate(&priv->ref_clk);
	if (priv->ref_clk_rate < REF_CLK_RATE_MIN ||
	    priv->ref_clk_rate > REF_CLK_RATE_MAX) {
		dev_err(dev, "invalid ref clock rate %lu\n",
			priv->ref_clk_rate);
		return -EINVAL;
	}
	dev_dbg(dev, "ref clock rate: %lu\n", priv->ref_clk_rate);
#endif

	return 0;
}

static int imx_dw_dphy_remove(struct udevice *dev)
{
	return 0;
}

static const struct udevice_id imx_dw_mipi_dphy_of_match[] = {
	{ .compatible = "fsl,imx93-mipi-dphy" },
	{ /* sentinel */ }
};

U_BOOT_DRIVER(imx_dw_mipi_dphy) = {
	.name = "imx_dw_mipi_dphy",
	.id = UCLASS_PHY,
	.of_match = imx_dw_mipi_dphy_of_match,
	.probe = imx_dw_dphy_probe,
	.remove = imx_dw_dphy_remove,
	.ops = &imx_dw_dphy_phy_ops,
	.priv_auto	= sizeof(struct dw_dphy_priv),
};
