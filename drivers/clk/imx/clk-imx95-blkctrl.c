// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2023-2025 NXP
 *
 */

#include <asm/io.h>
#include <common.h>
#include <clk-uclass.h>
#include <dm.h>
#include <dt-bindings/clock/nxp,imx95-clock.h>
#include <linux/clk-provider.h>

#include "clk.h"

#define IMX95_LVDS_CSR_ID_BASE IMX95_CLK_END + 20

static int imx95_blkctrl_clk_xlate(struct clk *clk,
			struct ofnode_phandle_args *args)
{
	debug("%s(clk=%p)\n", __func__, clk);

	if (args->args_count > 1) {
		debug("Invalid args_count: %d\n", args->args_count);
		return -EINVAL;
	}

	if (args->args_count)
		clk->id = args->args[0] + IMX95_LVDS_CSR_ID_BASE;
	else
		clk->id = 0;

	clk->data = 0;

	return 0;
}

static int imx95_blkctrl_clk_probe(struct udevice *dev)
{
	void __iomem *addr;

	addr = dev_read_addr_ptr(dev);
	if (addr == (void *)FDT_ADDR_T_NONE) {
		printf("%s: No blkctrl register base address\n", __func__);
		return -EINVAL;
	}

	clk_dm(IMX95_CLK_DISPMIX_LVDS_PHY_DIV + IMX95_LVDS_CSR_ID_BASE,
		clk_register_fixed_factor(NULL, "ldb_phy_div", "ldbpll", 0, 1, 2));

	clk_dm(IMX95_CLK_DISPMIX_LVDS_CH0_GATE + IMX95_LVDS_CSR_ID_BASE,
		clk_register_gate2(NULL, "lvds_ch0_gate", "ldb_pll_div7", 0, addr + 0x0, 1, 0, 0, NULL));

	clk_dm(IMX95_CLK_DISPMIX_LVDS_CH1_GATE + IMX95_LVDS_CSR_ID_BASE,
		clk_register_gate2(NULL, "lvds_ch1_gate", "ldb_pll_div7", 0, addr + 0x0, 2, 0, 0, NULL));

	clk_dm(IMX95_CLK_DISPMIX_PIX_DI0_GATE + IMX95_LVDS_CSR_ID_BASE,
		clk_register_gate2(NULL, "lvds_di0_gate", "ldb_pll_div7", 0, addr + 0x0, 3, 0, 0, NULL));

	clk_dm(IMX95_CLK_DISPMIX_PIX_DI1_GATE + IMX95_LVDS_CSR_ID_BASE,
		clk_register_gate2(NULL, "lvds_di1_gate", "ldb_pll_div7", 0, addr + 0x0, 4, 0, 0, NULL));

	return 0;
}

const struct clk_ops imx95_blkctrl_clk_ops = {
	.set_rate = ccf_clk_set_rate,
	.get_rate = ccf_clk_get_rate,
	.set_parent = ccf_clk_set_parent,
	.enable = ccf_clk_enable,
	.disable = ccf_clk_disable,
	.of_xlate = imx95_blkctrl_clk_xlate,
};

static const struct udevice_id imx95_blkctrl_clk_ids[] = {
	{ .compatible = "fsl,imx95-dispmix-lvds-csr" },
	{ },
};

U_BOOT_DRIVER(imx95_blkctrl_clk) = {
	.name = "imx95_blkctrl_clk",
	.id = UCLASS_CLK,
	.of_match = imx95_blkctrl_clk_ids,
	.ops = &imx95_blkctrl_clk_ops,
	.probe = imx95_blkctrl_clk_probe,
#if CONFIG_IS_ENABLED(OF_REAL)
	.bind = dm_scan_fdt_dev,
#endif
	.flags = DM_FLAG_PRE_RELOC,
};
