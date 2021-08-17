/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2022 NXP
 *
 * Peng Fan <peng.fan at nxp.com>
 */

#ifndef __CLOCK_IMX9__
#define __CLOCK_IMX9__

#include <linux/bitops.h>

#define MHZ(x)	((x) * 1000000UL)

enum {
	OSC_24M_CLK,
	SYS_PLL_PFD0,
	SYS_PLL_PFD0_DIV2,
	SYS_PLL_PFD1,
	SYS_PLL_PFD1_DIV2,
	SYS_PLL_PFD2,
	SYS_PLL_PFD2_DIV2,
	AUDIO_PLL_CLK,
	EXT_CLK,
	VIDEO_PLL_CLK,
	ARM_PLL_CLK,
	DRAM_PLL_CLK,
};

extern u32 clk_root_src[][4];

int clock_init(void);
int ccm_cfg_clk_root(u32 blk, u32 mux, u32 div);
u32 get_lpuart_clk(void);
void init_uart_clk(u32 index);
#endif
