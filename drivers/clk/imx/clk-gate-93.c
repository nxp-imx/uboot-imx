// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2022 NXP
 *
 * Author: Alice Guo <alice.guo@nxp.com>
 */

#include <asm/io.h>
#include <clk-uclass.h>
#include <dm.h>
#include <linux/bug.h>
#include <linux/clk-provider.h>

#include "clk.h"

#define LPCG_DIRECT		0x0
#define LPCG_LPM_CUR		0x1c
#define LPM_SETTING_OFF		0x0
#define LPM_SETTING_ON		0x4
#define LPCG_AUTHEN		0x30
#define WHITE_LIST_DM0		16
#define DOMAIN_ID_A55		3
#define TZ_NS			BIT(9)
#define CPULPM_MOD		BIT(2)

struct imx93_clk_gate {
	struct clk	clk;
	void __iomem	*reg_base;
	u8		lpcg_on_offset;
	u8		lpcg_on_ctrl;
	u8		lpcg_on_mask;
	ulong		flags;
};

#define to_imx93_clk_gate(_clk) container_of(_clk, struct imx93_clk_gate, clk)

static bool imx93_clk_gate_check_authen(void __iomem *reg_base)
{
	u32 authen;

	authen = readl(reg_base + LPCG_AUTHEN);
	if (!(authen & TZ_NS) || !(authen & BIT(WHITE_LIST_DM0 + DOMAIN_ID_A55)))
		return false;

	return true;
}

static void imx93_clk_gate_ctrl_hw(struct clk *clk, bool enable)
{
	struct imx93_clk_gate *gate = to_imx93_clk_gate(clk);
	u32 v;

	v = readl(gate->reg_base + LPCG_AUTHEN);
	if (v & CPULPM_MOD) {
		v = enable ? LPM_SETTING_ON : LPM_SETTING_OFF;
		writel(v, gate->reg_base + LPCG_LPM_CUR);
	} else {
		v = readl(gate->reg_base + LPCG_DIRECT);
		v &= ~(gate->lpcg_on_mask << gate->lpcg_on_offset);
		if (enable)
			v |= (gate->lpcg_on_ctrl & gate->lpcg_on_mask) << gate->lpcg_on_offset;
		writel(v, gate->reg_base + LPCG_DIRECT);
	}
}

static int imx93_clk_gate_enable(struct clk *clk)
{
	struct imx93_clk_gate *gate = to_imx93_clk_gate(clk);

	if (!imx93_clk_gate_check_authen(gate->reg_base))
		return -EINVAL;

	imx93_clk_gate_ctrl_hw(clk, true);

	return 0;
}

static int imx93_clk_gate_disable(struct clk *clk)
{
	struct imx93_clk_gate *gate = to_imx93_clk_gate(clk);

	if (!imx93_clk_gate_check_authen(gate->reg_base))
		return -EINVAL;

	imx93_clk_gate_ctrl_hw(clk, false);

	return 0;
}

static ulong imx93_clk_gate_set_rate(struct clk *clk, ulong rate)
{
	struct clk *parent = clk_get_parent(clk);

	if (parent)
		return clk_set_rate(parent, rate);

	return -ENODEV;
}

static const struct clk_ops imx93_clk_gate_ops = {
	.set_rate = imx93_clk_gate_set_rate,
	.enable = imx93_clk_gate_enable,
	.disable = imx93_clk_gate_disable,
	.get_rate = clk_generic_get_rate,
};

static struct clk *register_clk_gate(const char *name, const char *parent_name,
				     void __iomem *reg_base, u8 lpcg_on_offset,
				     u8 lpcg_on_ctrl, u8 lpcg_on_mask, ulong flags)
{
	struct imx93_clk_gate *gate;
	int ret;

	gate = kzalloc(sizeof(*gate), GFP_KERNEL);
	if (!gate)
		return ERR_PTR(-ENOMEM);

	gate->reg_base = reg_base;
	gate->lpcg_on_offset = lpcg_on_offset;
	gate->lpcg_on_ctrl = lpcg_on_ctrl;
	gate->lpcg_on_mask = lpcg_on_mask;
	gate->flags = flags;

	ret = clk_register(&gate->clk, "imx93_clk_gate", name, parent_name);
	if (ret) {
		kfree(gate);
		return ERR_PTR(ret);
	}

	return &gate->clk;
}

struct clk *clk_register_imx93_clk_gate(const char *name, const char *parent_name,
					void __iomem *reg_base, u8 lpcg_on_offset,
					ulong flags)
{
	return register_clk_gate(name, parent_name, reg_base, lpcg_on_offset, 1,
				 1, flags | CLK_SET_RATE_PARENT | CLK_OPS_PARENT_ENABLE);
}

U_BOOT_DRIVER(imx93_clk_gate) = {
	.name = "imx93_clk_gate",
	.id = UCLASS_CLK,
	.ops = &imx93_clk_gate_ops,
	.flags = DM_FLAG_PRE_RELOC,
};
