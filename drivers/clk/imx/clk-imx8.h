/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2018 NXP
 * Peng Fan <peng.fan@nxp.com>
 */

#define CLK_IMX8_MAX_MUX_SEL 5

#define FLAG_CLK_IMX8_IMX8QM	BIT(0)
#define FLAG_CLK_IMX8_IMX8QXP	BIT(1)

struct imx8_clk_header {
	ulong id;
#if CONFIG_IS_ENABLED(CMD_CLK)
	const char *name;
#endif
};

struct imx8_clks {
	struct imx8_clk_header hdr;
	u16 rsrc;
	sc_pm_clk_t pm_clk;
};

struct imx8_fixed_clks {
	struct imx8_clk_header hdr;
	ulong rate;
};

struct imx8_gpr_clks {
	struct imx8_clk_header hdr;
	u16 rsrc;
	sc_ctrl_t gpr_id;
	ulong parent_id;
};

struct imx8_lpcg_clks {
	struct imx8_clk_header hdr;
	u8 bit_idx;
	u32 lpcg;
	ulong parent_id;
};

struct imx8_mux_clks {
	struct imx8_clk_header hdr;
	ulong slice_clk_id;
	ulong parent_clks[CLK_IMX8_MAX_MUX_SEL];
};

enum imx8_clk_type {
	IMX8_CLK_SLICE 		= 0,
	IMX8_CLK_FIXED 	= 1,
	IMX8_CLK_GPR 		= 2,
	IMX8_CLK_LPCG 		= 3,
	IMX8_CLK_MUX 		= 4,
	IMX8_CLK_END		= 5,
};

struct imx8_clk_pair {
	void *type_clks;
	u32 num;
};

struct imx8_clks_collect {
	struct imx8_clk_pair clks[IMX8_CLK_END];
	ulong match_flag;
};

#if CONFIG_IS_ENABLED(CMD_CLK)
#define CLK_3(ID, NAME, MEM2) \
	{ { ID, NAME, }, MEM2, }
#define CLK_4(ID, NAME, MEM2, MEM3) \
	{ { ID, NAME, }, MEM2, MEM3, }
#define CLK_5(ID, NAME, MEM2, MEM3, MEM4) \
	{ { ID, NAME, }, MEM2, MEM3, MEM4, }
#define CLK_MUX(ID, NAME, MEM2, MUX0, MUX1, MUX2, MUX3, MUX4) \
	{ { ID, NAME, }, MEM2, { MUX0, MUX1, MUX2, MUX3, MUX4} }
#else
#define CLK_3(ID, NAME, MEM2) \
	{ { ID, }, MEM2, }
#define CLK_4(ID, NAME, MEM2, MEM3) \
	{ { ID, }, MEM2, MEM3, }
#define CLK_5(ID, NAME, MEM2, MEM3, MEM4) \
	{ { ID, }, MEM2, MEM3, MEM4, }
#define CLK_MUX(ID, NAME, MEM2, MUX0, MUX1, MUX2, MUX3, MUX4) \
	{ { ID, }, MEM2, { MUX0, MUX1, MUX2, MUX3, MUX4} }
#endif

extern struct imx8_clks_collect imx8qxp_clk_collect;
extern struct imx8_clks_collect imx8qm_clk_collect;
