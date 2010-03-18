/*
 * Copyright (C) 2010 Freescale Semiconductor, Inc.
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
#include <asm/arch/mx53.h>
#include <asm/errno.h>
#include "crm_regs.h"
#ifdef CONFIG_ARCH_CPU_INIT
#include <asm/cache-cp15.h>
#endif

enum pll_clocks {
	PLL1_CLK = MXC_DPLL1_BASE,
	PLL2_CLK = MXC_DPLL2_BASE,
	PLL3_CLK = MXC_DPLL3_BASE,
	PLL4_CLK = MXC_DPLL4_BASE,
};

enum pll_sw_clocks {
	PLL1_SW_CLK,
	PLL2_SW_CLK,
	PLL3_SW_CLK,
	PLL4_SW_CLK,
};

static u32 __decode_pll(enum pll_clocks pll, u32 infreq)
{
	u32 mfi, mfn, mfd, pd;

	mfn = __REG(pll + MXC_PLL_DP_MFN);
	mfd = __REG(pll + MXC_PLL_DP_MFD) + 1;
	mfi = __REG(pll + MXC_PLL_DP_OP);
	pd = (mfi  & 0xF) + 1;
	mfi = (mfi >> 4) & 0xF;
	mfi = (mfi >= 5) ? mfi : 5;

	return ((4 * (infreq / 1000) * (mfi * mfd + mfn)) / (mfd * pd)) * 1000;
}

static u32 __get_mcu_main_clk(void)
{
	u32 reg, freq;
	reg = (__REG(MXC_CCM_CACRR) & MXC_CCM_CACRR_ARM_PODF_MASK) >>
	    MXC_CCM_CACRR_ARM_PODF_OFFSET;
	freq = __decode_pll(PLL1_CLK, CONFIG_MX53_HCLK_FREQ);
	return freq / (reg + 1);
}

static u32 __get_periph_clk(void)
{
	u32 reg;
	reg = __REG(MXC_CCM_CBCDR);
	if (reg & MXC_CCM_CBCDR_PERIPH_CLK_SEL) {
		reg = __REG(MXC_CCM_CBCMR);
		switch ((reg & MXC_CCM_CBCMR_PERIPH_CLK_SEL_MASK) >>
			MXC_CCM_CBCMR_PERIPH_CLK_SEL_OFFSET) {
		case 0:
			return __decode_pll(PLL1_CLK, CONFIG_MX53_HCLK_FREQ);
		case 1:
			return __decode_pll(PLL3_CLK, CONFIG_MX53_HCLK_FREQ);
		default:
			return 0;
		}
	}
	return __decode_pll(PLL2_CLK, CONFIG_MX53_HCLK_FREQ);
}

static u32 __get_ipg_clk(void)
{
	u32 ahb_podf, ipg_podf;

	ahb_podf = __REG(MXC_CCM_CBCDR);
	ipg_podf = (ahb_podf & MXC_CCM_CBCDR_IPG_PODF_MASK) >>
			MXC_CCM_CBCDR_IPG_PODF_OFFSET;
	ahb_podf = (ahb_podf & MXC_CCM_CBCDR_AHB_PODF_MASK) >>
			MXC_CCM_CBCDR_AHB_PODF_OFFSET;
	return __get_periph_clk() / ((ahb_podf + 1) * (ipg_podf + 1));
}

static u32 __get_ipg_per_clk(void)
{
	u32 pred1, pred2, podf;
	if (__REG(MXC_CCM_CBCMR) & MXC_CCM_CBCMR_PERCLK_IPG_CLK_SEL)
		return __get_ipg_clk();
	/* Fixme: not handle what about lpm*/
	podf = __REG(MXC_CCM_CBCDR);
	pred1 = (podf & MXC_CCM_CBCDR_PERCLK_PRED1_MASK) >>
		MXC_CCM_CBCDR_PERCLK_PRED1_OFFSET;
	pred2 = (podf & MXC_CCM_CBCDR_PERCLK_PRED2_MASK) >>
		MXC_CCM_CBCDR_PERCLK_PRED2_OFFSET;
	podf = (podf & MXC_CCM_CBCDR_PERCLK_PODF_MASK) >>
		MXC_CCM_CBCDR_PERCLK_PODF_OFFSET;

	return __get_periph_clk() / ((pred1 + 1) * (pred2 + 1) * (podf + 1));
}

/*!
 * This function returns the low power audio clock.
 */
static u32 get_lp_apm(void)
{
	u32 ret_val = 0;
	u32 ccsr = __REG(MXC_CCM_CCSR);

	if (((ccsr >> MXC_CCM_CCSR_LP_APM_SEL_OFFSET) & 1) == 0)
		ret_val = CONFIG_MX53_HCLK_FREQ;
	else
		ret_val = ((32768 * 1024));

	return ret_val;
}

static u32 __get_uart_clk(void)
{
	u32 freq = 0, reg, pred, podf;
	reg = __REG(MXC_CCM_CSCMR1);
	switch ((reg & MXC_CCM_CSCMR1_UART_CLK_SEL_MASK) >>
		MXC_CCM_CSCMR1_UART_CLK_SEL_OFFSET) {
	case 0x0:
		freq = __decode_pll(PLL1_CLK, CONFIG_MX53_HCLK_FREQ);
		break;
	case 0x1:
		freq = __decode_pll(PLL2_CLK, CONFIG_MX53_HCLK_FREQ);
		break;
	case 0x2:
		freq = __decode_pll(PLL3_CLK, CONFIG_MX53_HCLK_FREQ);
		break;
	case 0x4:
		freq = get_lp_apm();
		break;
	default:
		break;
	}

	reg = __REG(MXC_CCM_CSCDR1);

	pred = (reg & MXC_CCM_CSCDR1_UART_CLK_PRED_MASK) >>
		MXC_CCM_CSCDR1_UART_CLK_PRED_OFFSET;

	podf = (reg & MXC_CCM_CSCDR1_UART_CLK_PODF_MASK) >>
		MXC_CCM_CSCDR1_UART_CLK_PODF_OFFSET;
	freq /= (pred + 1) * (podf + 1);

	return freq;
}


static u32 __get_cspi_clk(void)
{
	u32 ret_val = 0, pdf, pre_pdf, clk_sel, div;
	u32 cscmr1 = __REG(MXC_CCM_CSCMR1);
	u32 cscdr2 = __REG(MXC_CCM_CSCDR2);

	pre_pdf = (cscdr2 & MXC_CCM_CSCDR2_CSPI_CLK_PRED_MASK) \
			>> MXC_CCM_CSCDR2_CSPI_CLK_PRED_OFFSET;
	pdf = (cscdr2 & MXC_CCM_CSCDR2_CSPI_CLK_PODF_MASK) \
			>> MXC_CCM_CSCDR2_CSPI_CLK_PODF_OFFSET;
	clk_sel = (cscmr1 & MXC_CCM_CSCMR1_CSPI_CLK_SEL_MASK) \
			>> MXC_CCM_CSCMR1_CSPI_CLK_SEL_OFFSET;

	div = (pre_pdf + 1) * (pdf + 1);

	switch (clk_sel) {
	case 0:
		ret_val = __decode_pll(PLL1_CLK, CONFIG_MX53_HCLK_FREQ) / div;
		break;
	case 1:
		ret_val = __decode_pll(PLL2_CLK, CONFIG_MX53_HCLK_FREQ) / div;
		break;
	case 2:
		ret_val = __decode_pll(PLL3_CLK, CONFIG_MX53_HCLK_FREQ) / div;
		break;
	default:
		ret_val = get_lp_apm() / div;
		break;
	}

	return ret_val;
}

static u32 __get_axi_a_clk(void)
{
	u32 cbcdr =  __REG(MXC_CCM_CBCDR);
	u32 pdf = (cbcdr & MXC_CCM_CBCDR_AXI_A_PODF_MASK) \
			>> MXC_CCM_CBCDR_AXI_A_PODF_OFFSET;

	return  __get_periph_clk() / (pdf + 1);
}

static u32 __get_axi_b_clk(void)
{
	u32 cbcdr =  __REG(MXC_CCM_CBCDR);
	u32 pdf = (cbcdr & MXC_CCM_CBCDR_AXI_B_PODF_MASK) \
			>> MXC_CCM_CBCDR_AXI_B_PODF_OFFSET;

	return  __get_periph_clk() / (pdf + 1);
}

static u32 __get_ahb_clk(void)
{
	u32 cbcdr =  __REG(MXC_CCM_CBCDR);
	u32 pdf = (cbcdr & MXC_CCM_CBCDR_AHB_PODF_MASK) \
			>> MXC_CCM_CBCDR_AHB_PODF_OFFSET;

	return  __get_periph_clk() / (pdf + 1);
}


static u32 __get_emi_slow_clk(void)
{
	u32 cbcdr =  __REG(MXC_CCM_CBCDR);
	u32 emi_clk_sel = cbcdr & MXC_CCM_CBCDR_EMI_CLK_SEL;
	u32 pdf = (cbcdr & MXC_CCM_CBCDR_EMI_PODF_MASK) \
			>> MXC_CCM_CBCDR_EMI_PODF_OFFSET;

	if (emi_clk_sel)
		return  __get_ahb_clk() / (pdf + 1);

	return  __get_periph_clk() / (pdf + 1);
}

static u32 __get_ddr_clk(void)
{
	u32 ret_val = 0;
	u32 cbcmr =  __REG(MXC_CCM_CBCMR);
	u32 ddr_clk_sel = (cbcmr & MXC_CCM_CBCMR_DDR_CLK_SEL_MASK) \
				>> MXC_CCM_CBCMR_DDR_CLK_SEL_OFFSET;

	switch (ddr_clk_sel) {
	case 0:
		ret_val =  __get_axi_a_clk();
		break;
	case 1:
		ret_val =  __get_axi_b_clk();
		break;
	case 2:
		ret_val =  __get_emi_slow_clk();
		break;
	case 3:
		ret_val =  __get_ahb_clk();
		break;
	default:
		break;
	}

	return ret_val;
}


unsigned int mxc_get_clock(enum mxc_clock clk)
{
	switch (clk) {
	case MXC_ARM_CLK:
		return __get_mcu_main_clk();
	case MXC_AHB_CLK:
		return __get_ahb_clk();
	case MXC_IPG_CLK:
		return __get_ipg_clk();
	case MXC_IPG_PERCLK:
		return __get_ipg_per_clk();
	case MXC_UART_CLK:
		return __get_uart_clk();
	case MXC_CSPI_CLK:
		return __get_cspi_clk();
	case MXC_AXI_A_CLK:
		return __get_axi_a_clk();
	case MXC_AXI_B_CLK:
		return __get_axi_b_clk();
	case MXC_EMI_SLOW_CLK:
		return __get_emi_slow_clk();
	case MXC_DDR_CLK:
		return __get_ddr_clk();
	case MXC_ESDHC_CLK:
		return __decode_pll(PLL3_CLK, CONFIG_MX53_HCLK_FREQ);
	default:
		break;
	}
	return -1;
}

void mxc_dump_clocks(void)
{
	u32 freq;
	freq = __decode_pll(PLL1_CLK, CONFIG_MX53_HCLK_FREQ);
	printf("mx53 pll1: %dMHz\n", freq / 1000000);
	freq = __decode_pll(PLL2_CLK, CONFIG_MX53_HCLK_FREQ);
	printf("mx53 pll2: %dMHz\n", freq / 1000000);
	freq = __decode_pll(PLL3_CLK, CONFIG_MX53_HCLK_FREQ);
	printf("mx53 pll3: %dMHz\n", freq / 1000000);
	printf("ipg clock     : %dHz\n", mxc_get_clock(MXC_IPG_CLK));
	printf("ipg per clock : %dHz\n", mxc_get_clock(MXC_IPG_PERCLK));
	printf("uart clock    : %dHz\n", mxc_get_clock(MXC_UART_CLK));
	printf("cspi clock    : %dHz\n", mxc_get_clock(MXC_CSPI_CLK));
	printf("ahb clock     : %dHz\n", mxc_get_clock(MXC_AHB_CLK));
	printf("axi_a clock   : %dHz\n", mxc_get_clock(MXC_AXI_A_CLK));
	printf("axi_b clock   : %dHz\n", mxc_get_clock(MXC_AXI_B_CLK));
	printf("emi_slow clock: %dHz\n", mxc_get_clock(MXC_EMI_SLOW_CLK));
	printf("ddr clock     : %dHz\n", mxc_get_clock(MXC_DDR_CLK));
}

#if defined(CONFIG_DISPLAY_CPUINFO)
int print_cpuinfo(void)
{
	printf("CPU:   Freescale i.MX53 family %d.%dV at %d MHz\n",
	       (get_board_rev() & 0xFF) >> 4,
	       (get_board_rev() & 0xF),
		__get_mcu_main_clk() / 1000000);
	mxc_dump_clocks();
	return 0;
}
#endif

#if defined(CONFIG_MXC_FEC)
extern int mxc_fec_initialize(bd_t *bis);
extern void mxc_fec_set_mac_from_env(char *mac_addr);
#endif

int cpu_eth_init(bd_t *bis)
{
	int rc = -ENODEV;
#if defined(CONFIG_MXC_FEC)
	char *env = NULL;

	rc = mxc_fec_initialize(bis);

	env = getenv("fec_addr");
	if (env)
		mxc_fec_set_mac_from_env(env);
#endif
	return rc;
}

#if defined(CONFIG_ARCH_CPU_INIT)
int arch_cpu_init(void)
{
	icache_enable();
	dcache_enable();

#ifdef CONFIG_L2_OFF
	l2_cache_disable();
#else
	l2_cache_enable();
#endif
	return 0;
}
#endif

