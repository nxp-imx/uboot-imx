/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2015-2016, Freescale Semiconductor, Inc.
 */

#ifndef __ASM_ARCH_CLOCK_H
#define __ASM_ARCH_CLOCK_H

#ifdef __KERNEL__
#include <common.h>
#endif

enum mxc_clock {
	MXC_ARM_CLK = 0,
	MXC_BUS_CLK,
	MXC_PERIPHERALS_CLK,
	MXC_UART_CLK,
	MXC_USDHC_CLK,
	MXC_FEC_CLK,
	MXC_I2C_CLK,
	MXC_QSPI_CLK,
};
enum pll_type {
	ARM_PLL = 0,
	PERIPH_PLL,
	ENET_PLL,
	DDR_PLL,
	VIDEO_PLL,
};

#ifdef __KERNEL__
unsigned int mxc_get_clock(enum mxc_clock clk);
void clock_init(void);
void entry_to_target_mode(u32 mode);

#define imx_get_fecclk() mxc_get_clock(MXC_FEC_CLK)
#endif

#endif /* __ASM_ARCH_CLOCK_H */
