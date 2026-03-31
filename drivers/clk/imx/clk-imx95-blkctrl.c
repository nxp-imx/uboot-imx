// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2023 NXP
 *
 */

#include <asm/io.h>
#include <clk-uclass.h>
#include <dm.h>
#include <dt-bindings/clock/nxp,imx95-clock.h>
#include <dt-bindings/clock/nxp,imx94-clock.h>
#include <linux/clk-provider.h>

#include "clk.h"

/* Use 512 as base for lvds clock id, normally we won't have so many clocks */
#define IMX_LVDS_CSR_ID_BASE 512
#define IMX_HSIO_BLK_CTL_ID_BASE IMX_LVDS_CSR_ID_BASE + 32

static const char * const imx94_dispmix_clk_sels[] = {"disppix", "ldb_pll_div7"};

static int imx95_blkctrl_clk_xlate(struct clk *clk,
			struct ofnode_phandle_args *args)
{
	unsigned long id_base = IMX_LVDS_CSR_ID_BASE;
	debug("%s(clk=%p)\n", __func__, clk);

	if (args->args_count > 1) {
		debug("Invalid args_count: %d\n", args->args_count);
		return -EINVAL;
	}

	if (device_is_compatible(clk->dev, "fsl,imx94-dispmix-csr"))
		id_base += 1;
	else if (device_is_compatible(clk->dev, "nxp,imx95-hsio-blk-ctl"))
		id_base = IMX_HSIO_BLK_CTL_ID_BASE;

	if (args->args_count)
		clk->id = args->args[0] + id_base;
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

	if (device_is_compatible(dev, "fsl,imx94-dispmix-lvds-csr")) {
		clk_dm(IMX94_CLK_DISPMIX_LVDS_CLK_GATE + IMX_LVDS_CSR_ID_BASE,
			clk_register_gate(NULL, "lvds_ch_gate", "ldb_pll_div7", 0, addr + 0x0, 1,
				CLK_GATE_SET_TO_DISABLE, NULL));
	} else if (device_is_compatible(dev, "fsl,imx94-dispmix-csr")) {
		clk_dm(IMX94_CLK_DISPMIX_CLK_SEL + IMX_LVDS_CSR_ID_BASE + 1,
			clk_register_mux(NULL, "dispmix_clk_sel", imx94_dispmix_clk_sels,
				ARRAY_SIZE(imx94_dispmix_clk_sels), CLK_SET_RATE_PARENT | CLK_SET_RATE_NO_REPARENT,
				addr + 0x0, 1, 1, 0));
	} else if (device_is_compatible(dev, "nxp,imx95-hsio-blk-ctl")) {
		clk_dm(IMX_HSIO_BLK_CTL_ID_BASE,
			clk_register_gate(NULL, "hsio_blk_ctl_clk", "hsiopll", 0, addr + 0x0,
				6, 0, NULL));
	} else {
		clk_dm(IMX95_CLK_DISPMIX_LVDS_PHY_DIV + IMX_LVDS_CSR_ID_BASE,
			clk_register_fixed_factor(NULL, "ldb_phy_div", "ldbpll", 0, 1, 2));

		clk_dm(IMX95_CLK_DISPMIX_LVDS_CH0_GATE + IMX_LVDS_CSR_ID_BASE,
			clk_register_gate(NULL, "lvds_ch0_gate", "ldb_pll_div7", 0, addr + 0x0, 1,
				CLK_GATE_SET_TO_DISABLE, NULL));

		clk_dm(IMX95_CLK_DISPMIX_LVDS_CH1_GATE + IMX_LVDS_CSR_ID_BASE,
			clk_register_gate(NULL, "lvds_ch1_gate", "ldb_pll_div7", 0, addr + 0x0, 2,
				CLK_GATE_SET_TO_DISABLE, NULL));

		clk_dm(IMX95_CLK_DISPMIX_PIX_DI0_GATE + IMX_LVDS_CSR_ID_BASE,
			clk_register_gate(NULL, "lvds_di0_gate", "ldb_pll_div7", 0, addr + 0x0, 3,
				CLK_GATE_SET_TO_DISABLE, NULL));

		clk_dm(IMX95_CLK_DISPMIX_PIX_DI1_GATE + IMX_LVDS_CSR_ID_BASE,
			clk_register_gate(NULL, "lvds_di1_gate", "ldb_pll_div7", 0, addr + 0x0, 4,
				CLK_GATE_SET_TO_DISABLE, NULL));
	}

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
	{ .compatible = "fsl,imx94-dispmix-lvds-csr" },
	{ .compatible = "fsl,imx94-dispmix-csr" },
	{ .compatible = "nxp,imx95-hsio-blk-ctl" },
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
