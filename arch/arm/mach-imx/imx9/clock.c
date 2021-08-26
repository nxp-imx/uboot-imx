// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2022 NXP
 *
 * Peng Fan <peng.fan@nxp.com>
 */

#include <common.h>
#include <command.h>
#include <asm/arch/clock.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/ccm_regs.h>
#include <asm/arch/sys_proto.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <div64.h>
#include <errno.h>
#include <linux/bitops.h>
#include <linux/iopoll.h>
#include <linux/delay.h>
#include <log.h>

DECLARE_GLOBAL_DATA_PTR;

u32 get_clk_src_freq(u32 src_clk)
{
	switch(src_clk) {
	case OSC_24M_CLK:
		return MHZ(24);
	case SYS_PLL_PFD0:
		return MHZ(1000);
	case SYS_PLL_PFD0_DIV2:
		return MHZ(500);
	case SYS_PLL_PFD1:
		return MHZ(800);
	case SYS_PLL_PFD1_DIV2:
		return MHZ(400);
	case SYS_PLL_PFD2:
		return MHZ(625);
	case SYS_PLL_PFD2_DIV2:
		return 312500000;
	/* TODO: Add ARM/AUDIO/VIDEO */
	default:
		return 0;
	}
}

int get_clk_root_freq(u32 root_clk)
{
	void __iomem *base = (void __iomem *)CCM_BASE_ADDR + root_clk * 0x80;
	u32 status = readl(base + CLK_ROOT_STATUS0_OFF);
	u32 src_freq;
	u32 mux, div;

	if (status & CLK_ROOT_STATUS_OFF)
		return 0;

	div = status & CLK_ROOT_DIV_MASK;
	mux = (status & CLK_ROOT_MUX_MASK) >> CLK_ROOT_MUX_SHIFT;

	src_freq = get_clk_src_freq(clk_root_src[root_clk][mux]);

	return src_freq / (div + 1);
}

int get_clk_ccgr_freq(u32 lpcg_clk)
{
	void __iomem *base = (void __iomem *)CCM_CCGR_BASE_ADDR + lpcg_clk * 0x80;
}

int ccm_cfg_clk_root(u32 blk, u32 mux, u32 div)
{
	void __iomem *base = (void __iomem *)CCM_BASE_ADDR + blk * 0x80;
	u32 status;
	int ret;

	writel((mux << 8) | div, base + CLK_ROOT_CONTROL_OFF);

	ret = readl_poll_timeout(base + CLK_ROOT_STATUS0_OFF, status,
				 !(status & CLK_ROOT_STATUS_CHANGING), 200000);
	if (ret)
		log_err("%s: failed, status: 0x%x, base: %p\n", __func__,
			readl(base + CLK_ROOT_STATUS0_OFF), base);

	return ret;
};

int ccm_cfg_clk_ccgr(u32 lpcg, u32 val)
{
	void __iomem *base = (void __iomem *)CCM_CCGR_BASE_ADDR + lpcg * 0x80;

	writel(val, base);

	return 0;
}

int clock_init(void)
{
	return 0;
};

u32 get_lpuart_clk(void)
{
	return 24000000;
}

void init_uart_clk(u32 index)
{
	switch(index) {
	case LPUART1_CLK_ROOT:
		/* 24M */
		ccm_cfg_clk_root(LPUART1_CLK_ROOT, 0, 0);
		break;
	default:
		break;
	}
}

void init_clk_usdhc(u32 index)
{
	switch (index) {
	case 0:
		ccm_cfg_clk_ccgr(CCGR_USDHC1, 0);
		ccm_cfg_clk_root(51, 2, 1);
		ccm_cfg_clk_ccgr(CCGR_USDHC1, 1);
		break;
	case 1:
		ccm_cfg_clk_ccgr(CCGR_USDHC2, 0);
		ccm_cfg_clk_root(52, 2, 1);
		ccm_cfg_clk_ccgr(CCGR_USDHC2, 0);
		break;
	case 2:
		ccm_cfg_clk_ccgr(CCGR_USDHC3, 0);
		ccm_cfg_clk_root(53, 2, 1);
		ccm_cfg_clk_ccgr(CCGR_USDHC3, 0);
		break;
	default:
		return;
	};
};

unsigned int mxc_get_clock(enum mxc_clock clk)
{
	switch (clk) {
	case MXC_ESDHC_CLK:
		return 400000000;
	default:
		return -1;
	};

	return -1;
};
