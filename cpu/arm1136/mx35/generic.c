/*
 * (C) Copyright 2007
 * Sascha Hauer, Pengutronix
 *
 * (C) Copyright 2008-2009 Freescale Semiconductor, Inc.
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

#include <common.h>
#include <asm/errno.h>
#include <asm/arch/mx35.h>
#include "crm_regs.h"

#define CLK_CODE(arm, ahb, sel) (((arm) << 16) + ((ahb) << 8) + (sel))
#define CLK_CODE_ARM(c)		(((c) >> 16) & 0xFF)
#define CLK_CODE_AHB(c) 	(((c) >>  8) & 0xFF)
#define CLK_CODE_PATH(c) 	((c) & 0xFF)

#define CCM_GET_DIVIDER(x, m, o) (((x) & (m)) >> (o))

static int g_clk_mux_auto[8] = {
	CLK_CODE(1, 3, 0), CLK_CODE(1, 2, 1), CLK_CODE(2, 1, 1), -1,
	CLK_CODE(1, 6, 0), CLK_CODE(1, 4, 1), CLK_CODE(2, 2, 1), -1,
};

static int g_clk_mux_consumer[16] = {
	CLK_CODE(1, 4, 0), CLK_CODE(1, 3, 1), CLK_CODE(1, 3, 1), -1,
	-1, -1, CLK_CODE(4, 1, 0), CLK_CODE(1, 5, 0),
	CLK_CODE(1, 8, 1), CLK_CODE(1, 6, 1), CLK_CODE(2, 4, 0), -1,
	-1, -1, CLK_CODE(4, 2, 0), -1,
};

static u32 __get_arm_div(u32 pdr0, u32 *fi, u32 *fd)
{
	int *pclk_mux;
	if (pdr0 & MXC_CCM_PDR0_AUTO_CON) {
		pclk_mux = g_clk_mux_consumer +
		    ((pdr0 & MXC_CCM_PDR0_CON_MUX_DIV_MASK) >>
		     MXC_CCM_PDR0_CON_MUX_DIV_OFFSET);
	} else {
		pclk_mux = g_clk_mux_auto +
		    ((pdr0 & MXC_CCM_PDR0_AUTO_MUX_DIV_MASK) >>
		     MXC_CCM_PDR0_AUTO_MUX_DIV_OFFSET);
	}

	if ((*pclk_mux) == -1)
		return -1;

	if (fi && fd) {
		if (!CLK_CODE_PATH(*pclk_mux)) {
			*fi = *fd = 1;
			return CLK_CODE_ARM(*pclk_mux);
		}
		if (pdr0 & MXC_CCM_PDR0_AUTO_CON) {
			*fi = 3;
			*fd = 4;
		} else {
			*fi = 2;
			*fd = 3;
		}
	}
	return CLK_CODE_ARM(*pclk_mux);
}

static int __get_ahb_div(u32 pdr0)
{
	int *pclk_mux;
	if (pdr0 & MXC_CCM_PDR0_AUTO_CON) {
		pclk_mux = g_clk_mux_consumer +
		    ((pdr0 & MXC_CCM_PDR0_CON_MUX_DIV_MASK) >>
		     MXC_CCM_PDR0_CON_MUX_DIV_OFFSET);
	} else {
		pclk_mux = g_clk_mux_auto +
		    ((pdr0 & MXC_CCM_PDR0_AUTO_MUX_DIV_MASK) >>
		     MXC_CCM_PDR0_AUTO_MUX_DIV_OFFSET);
	}

	if ((*pclk_mux) == -1)
		return -1;

	return CLK_CODE_AHB(*pclk_mux);
}

static u32 __decode_pll(u32 reg, u32 infreq)
{
	u32 mfi = (reg >> 10) & 0xf;
	u32 mfn = reg & 0x3f;
	u32 mfd = (reg >> 16) & 0x3f;
	u32 pd = (reg >> 26) & 0xf;

	mfi = mfi <= 5 ? 5 : mfi;
	mfd += 1;
	pd += 1;

	return ((2 * (infreq / 1000) * (mfi * mfd + mfn)) / (mfd * pd)) * 1000;
}

static u32 __get_mcu_main_clk(void)
{
	u32 arm_div, fi, fd;
	arm_div = __get_arm_div(__REG(CCM_BASE_ADDR + CLKCTL_PDR0), &fi, &fd);
	fi *=
	    __decode_pll(__REG(CCM_BASE_ADDR + CLKCTL_MPCTL),
			 CONFIG_MX35_HCLK_FREQ);
	return fi / (arm_div * fd);
}

static u32 __get_ipg_clk(void)
{
	u32 freq = __get_mcu_main_clk();
	u32 pdr0 = __REG(CCM_BASE_ADDR + CLKCTL_PDR0);

	return freq / (__get_ahb_div(pdr0) * 2);
}

static u32 __get_ipg_per_clk(void)
{
	u32 freq = __get_mcu_main_clk();
	u32 pdr0 = __REG(CCM_BASE_ADDR + CLKCTL_PDR0);
	u32 pdr4 = __REG(CCM_BASE_ADDR + CLKCTL_PDR4);
	u32 div;
	if (pdr0 & MXC_CCM_PDR0_PER_SEL) {
		div = (CCM_GET_DIVIDER(pdr4,
				       MXC_CCM_PDR4_PER0_PRDF_MASK,
				       MXC_CCM_PDR4_PER0_PODF_OFFSET) + 1) *
		    (CCM_GET_DIVIDER(pdr4,
				     MXC_CCM_PDR4_PER0_PODF_MASK,
				     MXC_CCM_PDR4_PER0_PODF_OFFSET) + 1);
	} else {
		div = CCM_GET_DIVIDER(pdr0,
				      MXC_CCM_PDR0_PER_PODF_MASK,
				      MXC_CCM_PDR0_PER_PODF_OFFSET) + 1;
		freq /= __get_ahb_div(pdr0);
	}
	return freq / div;
}

static u32 __get_uart_clk(void)
{
	u32 freq;
	u32 pdr4 = __REG(CCM_BASE_ADDR + CLKCTL_PDR4);

	if (__REG(CCM_BASE_ADDR + CLKCTL_PDR3) & MXC_CCM_PDR3_UART_M_U)
		freq = __get_mcu_main_clk();
	else
		freq = __decode_pll(__REG(CCM_BASE_ADDR + CLKCTL_PPCTL),
				    CONFIG_MX35_HCLK_FREQ);
	freq /= ((CCM_GET_DIVIDER(pdr4,
				  MXC_CCM_PDR4_UART_PRDF_MASK,
				  MXC_CCM_PDR4_UART_PRDF_OFFSET) + 1) *
		 (CCM_GET_DIVIDER(pdr4,
				  MXC_CCM_PDR4_UART_PODF_MASK,
				  MXC_CCM_PDR4_UART_PODF_OFFSET) + 1));
	return freq;
}

unsigned int mxc_get_clock(enum mxc_clock clk)
{
	switch (clk) {
	case MXC_ARM_CLK:
		return __get_mcu_main_clk();
	case MXC_AHB_CLK:
		break;
	case MXC_IPG_CLK:
		return __get_ipg_clk();
	case MXC_IPG_PERCLK:
		return __get_ipg_per_clk();
	case MXC_UART_CLK:
		return __get_uart_clk();
	}
	return -1;
}

void mxc_dump_clocks(void)
{
	u32 cpufreq = __get_mcu_main_clk();
	printf("mx35 cpu clock: %dMHz\n", cpufreq / 1000000);
	printf("ipg clock     : %dHz\n", __get_ipg_clk());
	printf("uart clock     : %dHz\n", mxc_get_clock(MXC_UART_CLK));
}

#if defined(CONFIG_DISPLAY_CPUINFO)
int print_cpuinfo(void)
{
	printf("CPU:   Freescale i.MX35 at %d MHz\n",
	       __get_mcu_main_clk() / 1000000);
	return 0;
}
#endif

/*
 * Initializes on-chip ethernet controllers.
 * to override, implement board_eth_init()
 */
int cpu_eth_init(bd_t *bis)
{
	int rc = -ENODEV;
#if defined(CONFIG_MXC_FEC)
	rc = mxc_fec_initialize(bis);
#endif
	return rc;
}
