/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2018 NXP
 */

#ifndef __ASM_ARCH_IMX8_CLOCK_H__
#define __ASM_ARCH_IMX8_CLOCK_H__

/* Mainly for compatible to imx common code. */
enum mxc_clock {
	MXC_ARM_CLK = 0,
	MXC_AHB_CLK,
	MXC_IPG_CLK,
	MXC_UART_CLK,
	MXC_CSPI_CLK,
	MXC_AXI_CLK,
	MXC_DDR_CLK,
	MXC_ESDHC_CLK,
	MXC_ESDHC2_CLK,
	MXC_ESDHC3_CLK,
	MXC_I2C_CLK,
	MXC_FEC_CLK,
};

u32 mxc_get_clock(enum mxc_clock clk);
u32 get_lpuart_clk(void);
int enable_i2c_clk(unsigned char enable, unsigned i2c_num);
u32 imx_get_i2cclk(unsigned i2c_num);
void enable_usboh3_clk(unsigned char enable);
u32 imx_get_fecclk(void);
void init_clk_usdhc(u32 index);
void init_clk_gpmi_nand(void);
void init_clk_usb3(int index);

#endif /* __ASM_ARCH_IMX8_CLOCK_H__ */
