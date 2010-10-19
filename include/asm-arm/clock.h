/*
 * (C) Copyright 2008-2010 Freescale Semiconductor, Inc.
 * Terry Lv
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __ASM_ARCH_CLOCK_H__
#define __ASM_ARCH_CLOCK_H__
#include <linux/types.h>

enum {
	CPU_CLK = 0,
	PERIPH_CLK,
	AHB_CLK,
	IPG_CLK,
	IPG_PERCLK,
	UART_CLK,
	CSPI_CLK,
	DDR_CLK,
	NFC_CLK,
	ALL_CLK,
};

int clk_config(u32 ref, u32 freq, u32 clk_type);
int clk_info(u32 clk_type);

#endif
