// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2018 NXP
 * Peng Fan <peng.fan@nxp.com>
 */

#include <common.h>
#include <clk-uclass.h>
#include <dm.h>
#include <malloc.h>
#include <asm/arch/sci/sci.h>
#include <asm/arch/clock.h>
#include <dt-bindings/clock/imx8qxp-clock.h>
#include <dt-bindings/soc/imx_rsrc.h>
#include <misc.h>
#include <asm/arch/lpcg.h>

#include "clk-imx8.h"

struct imx8_clks_collect *soc_data[] = {
#if defined(CONFIG_IMX8QXP) || defined(CONFIG_IMX8DXL)
	&imx8qxp_clk_collect,
#endif
#ifdef CONFIG_IMX8QM
	&imx8qm_clk_collect,
#endif
};

static ulong __imx8_clk_get_rate(struct udevice *dev, ulong id);
static int __imx8_clk_enable(struct udevice *dev, ulong id, bool enable);
static ulong __imx8_clk_set_rate(struct udevice *dev, ulong id, unsigned long rate);

static struct imx8_clks_collect * find_clks_collect(struct udevice *dev)
{
	ulong data = (ulong)dev_get_driver_data(dev);
	int i;

	for (i = 0; i < ARRAY_SIZE(soc_data); i++) {
		if (soc_data[i]->match_flag == data)
			return soc_data[i];
	}

	return NULL;
}

static void * check_imx8_clk(struct udevice *dev, enum imx8_clk_type type, ulong id, u32 size_of_clk)
{
	u32 i, size;
	struct imx8_clks_collect *clks_col = find_clks_collect(dev);
	struct imx8_clk_header *hdr;
	ulong clks;

	if (!clks_col || !(clks_col->clks[type].type_clks)) {
		printf("%s fails to get clks for type %d\n",
		       __func__, type);
		return NULL;
	}

	clks = (ulong)(clks_col->clks[type].type_clks);
	size = clks_col->clks[type].num;

	for (i = 0; i < size; i++) {
		hdr = (struct imx8_clk_header *)clks;
		if (id == hdr->id)
			return (void *)hdr;

		clks += size_of_clk;
	}

	return NULL;
}

static ulong imx8_get_rate_lpcg(struct udevice *dev, struct imx8_lpcg_clks *lpcg_clk)
{
	if (lpcg_clk->parent_id != 0) {
		if (lpcg_is_clock_on(lpcg_clk->lpcg, lpcg_clk->bit_idx >> 2)) {
			return __imx8_clk_get_rate(dev, lpcg_clk->parent_id);
		} else {
			return 0;
		}
	} else {
		return -ENOSYS;
	}
}

static ulong imx8_get_rate_slice(struct udevice *dev, struct imx8_clks *slice_clk)
{
	int ret;
	u32 rate;

	ret = sc_pm_get_clock_rate(-1, slice_clk->rsrc, slice_clk->pm_clk,
				   (sc_pm_clock_rate_t *)&rate);
	if (ret) {
		printf("%s err %d\n", __func__, ret);
		return ret;
	}

	return rate;
}

static ulong imx8_get_rate_fixed(struct udevice *dev, struct imx8_fixed_clks *fixed_clk)
{
	return fixed_clk->rate;
}

static ulong __imx8_clk_get_rate(struct udevice *dev, ulong id)
{
	void* clkdata;

	clkdata = check_imx8_clk(dev, IMX8_CLK_LPCG, id, sizeof(struct imx8_lpcg_clks));
	if (clkdata) {
		return imx8_get_rate_lpcg(dev, (struct imx8_lpcg_clks *)clkdata);
	}

	clkdata = check_imx8_clk(dev, IMX8_CLK_SLICE, id, sizeof(struct imx8_clks));
	if (clkdata) {
		return imx8_get_rate_slice(dev, (struct imx8_clks *)clkdata);
	}

	clkdata = check_imx8_clk(dev, IMX8_CLK_FIXED, id, sizeof(struct imx8_fixed_clks));
	if (clkdata) {
		return imx8_get_rate_fixed(dev, (struct imx8_fixed_clks *)clkdata);
	}

	return -ENOSYS;
}

static ulong imx8_clk_get_rate(struct clk *clk)
{
	return __imx8_clk_get_rate(clk->dev, clk->id);
}

static ulong imx8_set_rate_lpcg(struct udevice *dev, struct imx8_lpcg_clks *lpcg_clk, unsigned long rate)
{
	if (lpcg_clk->parent_id != 0) {
		return __imx8_clk_set_rate(dev, lpcg_clk->parent_id, rate);
	} else {
		return -ENOSYS;
	}
}

static ulong imx8_set_rate_slice(struct udevice *dev, struct imx8_clks *slice_clk, unsigned long rate)
{
	int ret;
	u32 new_rate = rate;

	ret = sc_pm_set_clock_rate(-1, slice_clk->rsrc, slice_clk->pm_clk, &new_rate);
	if (ret) {
		printf("%s err %d\n", __func__, ret);
		return ret;
	}

	return new_rate;
}

static ulong imx8_set_rate_gpr(struct udevice *dev, struct imx8_gpr_clks *gpr_clk, unsigned long rate)
{
	ulong parent_rate;
	u32 val;
	int ret;

	if (gpr_clk->parent_id == 0)
		return -ENOSYS;

	parent_rate = __imx8_clk_get_rate(dev, gpr_clk->parent_id);
	if (parent_rate > 0) {
		val = (rate < parent_rate) ? 1 : 0;

		ret = sc_misc_set_control(-1, gpr_clk->rsrc,
			gpr_clk->gpr_id, val);
		if (ret) {
			printf("%s err %d\n", __func__, ret);
			return ret;
		}

		return rate;
	}

	return -ENOSYS;
}

static ulong __imx8_clk_set_rate(struct udevice *dev, ulong id, unsigned long rate)
{
	void* clkdata;

	clkdata = check_imx8_clk(dev, IMX8_CLK_SLICE, id, sizeof(struct imx8_clks));
	if (clkdata) {
		return imx8_set_rate_slice(dev, (struct imx8_clks *)clkdata, rate);
	}

	clkdata = check_imx8_clk(dev, IMX8_CLK_LPCG, id, sizeof(struct imx8_lpcg_clks));
	if (clkdata) {
		return imx8_set_rate_lpcg(dev, (struct imx8_lpcg_clks *)clkdata, rate);
	}

	clkdata = check_imx8_clk(dev, IMX8_CLK_GPR, id, sizeof(struct imx8_gpr_clks));
	if (clkdata) {
		return imx8_set_rate_gpr(dev, (struct imx8_gpr_clks *)clkdata, rate);
	}

	return -ENOSYS;
}

static ulong imx8_clk_set_rate(struct clk *clk, unsigned long rate)
{
	return __imx8_clk_set_rate(clk->dev, clk->id, rate);

}

static int imx8_enable_slice(struct udevice *dev, struct imx8_clks *slice_clk, bool enable)
{
	int ret;

	ret = sc_pm_clock_enable(-1, slice_clk->rsrc, slice_clk->pm_clk, enable, 0);
	if (ret) {
		printf("%s err %d\n", __func__, ret);
		return ret;
	}

	return 0;
}

static int imx8_enable_lpcg(struct udevice *dev, struct imx8_lpcg_clks *lpcg_clk, bool enable)
{
	if (enable) {
		if (lpcg_clk->parent_id != 0) {
			__imx8_clk_enable(dev, lpcg_clk->parent_id, enable);
		}

		lpcg_clock_on(lpcg_clk->lpcg, lpcg_clk->bit_idx >> 2);
	} else {
		lpcg_clock_off(lpcg_clk->lpcg, lpcg_clk->bit_idx >> 2);

		if (lpcg_clk->parent_id != 0) {
			__imx8_clk_enable(dev, lpcg_clk->parent_id, enable);
		}
	}

	return 0;
}

static int __imx8_clk_enable(struct udevice *dev, ulong id, bool enable)
{
	void* clkdata;

	clkdata = check_imx8_clk(dev, IMX8_CLK_LPCG, id, sizeof(struct imx8_lpcg_clks));
	if (clkdata) {
		return imx8_enable_lpcg(dev, (struct imx8_lpcg_clks *)clkdata, enable);
	}

	clkdata = check_imx8_clk(dev, IMX8_CLK_SLICE, id, sizeof(struct imx8_clks));
	if (clkdata) {
		return imx8_enable_slice(dev, (struct imx8_clks *)clkdata, enable);
	}

	return -ENOSYS;
}

static int imx8_clk_disable(struct clk *clk)
{
	return __imx8_clk_enable(clk->dev, clk->id, 0);
}

static int imx8_clk_enable(struct clk *clk)
{
	return __imx8_clk_enable(clk->dev, clk->id, 1);
}

static int imx8_set_parent_mux(struct udevice *dev, struct imx8_mux_clks *mux_clk, ulong pid)
{
	u32 i;
	int ret;
	struct imx8_clks *slice_clkdata;

	slice_clkdata = check_imx8_clk(dev, IMX8_CLK_SLICE, mux_clk->slice_clk_id, sizeof(struct imx8_clks));
	if (!slice_clkdata) {
		printf("Error: fail to find slice clk %lu for this mux %lu\n", mux_clk->slice_clk_id, mux_clk->hdr.id);
		return -EINVAL;
	}

	for (i = 0; i< CLK_IMX8_MAX_MUX_SEL; i++) {
		if (pid == mux_clk->parent_clks[i]) {
			ret = sc_pm_set_clock_parent(-1, slice_clkdata->rsrc,  slice_clkdata->pm_clk, i);
			if (ret)
				printf("Error: fail to set clock parent rsrc %d, pm_clk %d, parent clk %d\n",
					slice_clkdata->rsrc,  slice_clkdata->pm_clk, i);
			return ret;
		}
	}

	return -ENOSYS;
}

static int imx8_clk_set_parent(struct clk *clk, struct clk *parent)
{
	void* clkdata;

	clkdata = check_imx8_clk(clk->dev, IMX8_CLK_MUX, clk->id, sizeof(struct imx8_mux_clks));
	if (clkdata) {
		return imx8_set_parent_mux(clk->dev, (struct imx8_mux_clks *)clkdata, parent->id);
	}

	return -ENOSYS;
}

#if CONFIG_IS_ENABLED(CMD_CLK)
int soc_clk_dump(void)
{
	struct udevice *dev;
	struct clk clk;
	unsigned long rate;
	int i, ret;
	u32 size;
	struct imx8_clks *clks;
	struct imx8_clks_collect *clks_col;

	ret = uclass_get_device_by_driver(UCLASS_CLK,
					  DM_GET_DRIVER(imx8_clk), &dev);
	if (ret)
		return ret;

	printf("Clk\t\tHz\n");

	clks_col = find_clks_collect(dev);

	if (!clks_col || !(clks_col->clks[IMX8_CLK_SLICE].type_clks)) {
		printf("%s fails to get clks for type %d\n",
		       __func__, IMX8_CLK_SLICE);
		return -ENODEV;
	}

	clks = (struct imx8_clks *)(clks_col->clks[IMX8_CLK_SLICE].type_clks);
	size = clks_col->clks[IMX8_CLK_SLICE].num;

	for (i = 0; i < size; i++) {
		clk.id = clks[i].hdr.id;
		ret = clk_request(dev, &clk);
		if (ret < 0) {
			debug("%s clk_request() failed: %d\n", __func__, ret);
			continue;
		}

		ret = clk_get_rate(&clk);
		rate = ret;

		clk_free(&clk);

		if (ret == -ENOTSUPP) {
			printf("clk ID %lu not supported yet\n",
			       clks[i].hdr.id);
			continue;
		}
		if (ret < 0) {
			printf("%s %lu: get_rate err: %d\n",
			       __func__, clks[i].hdr.id, ret);
			continue;
		}

		printf("%s(%3lu):\t%lu\n",
		       clks[i].hdr.name, clks[i].hdr.id, rate);
	}

	return 0;
}

#endif

static struct clk_ops imx8_clk_ops = {
	.set_rate = imx8_clk_set_rate,
	.get_rate = imx8_clk_get_rate,
	.enable = imx8_clk_enable,
	.disable = imx8_clk_disable,
	.set_parent = imx8_clk_set_parent,
};

static int imx8_clk_probe(struct udevice *dev)
{
	return 0;
}

static const struct udevice_id imx8_clk_ids[] = {
	{ .compatible = "fsl,imx8qxp-clk", .data = FLAG_CLK_IMX8_IMX8QXP, },
	{ .compatible = "fsl,imx8qm-clk", .data = FLAG_CLK_IMX8_IMX8QM, },
	{ },
};

U_BOOT_DRIVER(imx8_clk) = {
	.name = "clk_imx8",
	.id = UCLASS_CLK,
	.of_match = imx8_clk_ids,
	.ops = &imx8_clk_ops,
	.probe = imx8_clk_probe,
	.flags = DM_FLAG_PRE_RELOC,
};
