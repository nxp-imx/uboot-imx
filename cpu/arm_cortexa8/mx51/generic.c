/*
 * (C) Copyright 2007
 * Sascha Hauer, Pengutronix
 *
 * (C) Copyright 2009-2010 Freescale Semiconductor, Inc.
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
#include <asm/arch/mx51.h>
#include <asm/errno.h>
#include "crm_regs.h"

enum pll_clocks {
PLL1_CLK = MXC_DPLL1_BASE,
PLL2_CLK = MXC_DPLL2_BASE,
PLL3_CLK = MXC_DPLL3_BASE,
};

enum pll_sw_clocks {
PLL1_SW_CLK,
PLL2_SW_CLK,
PLL3_SW_CLK,
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
	freq = __decode_pll(PLL1_CLK, CONFIG_MX51_HCLK_FREQ);
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
			return __decode_pll(PLL1_CLK, CONFIG_MX51_HCLK_FREQ);
		case 1:
			return __decode_pll(PLL3_CLK, CONFIG_MX51_HCLK_FREQ);
		default:
			return 0;
		}
	}
	return __decode_pll(PLL2_CLK, CONFIG_MX51_HCLK_FREQ);
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

static u32 __get_uart_clk(void)
{
	unsigned int freq, reg, pred, podf;
	reg = __REG(MXC_CCM_CSCMR1);
	switch ((reg & MXC_CCM_CSCMR1_UART_CLK_SEL_MASK) >>
		MXC_CCM_CSCMR1_UART_CLK_SEL_OFFSET) {
	case 0x0:
		freq = __decode_pll(PLL1_CLK, CONFIG_MX51_HCLK_FREQ);
		break;
	case 0x1:
		freq = __decode_pll(PLL2_CLK, CONFIG_MX51_HCLK_FREQ);
		break;
	case 0x2:
		freq = __decode_pll(PLL3_CLK, CONFIG_MX51_HCLK_FREQ);
		break;
	default:
		return 66500000;
	}

	reg = __REG(MXC_CCM_CSCDR1);

	pred = (reg & MXC_CCM_CSCDR1_UART_CLK_PRED_MASK) >>
		MXC_CCM_CSCDR1_UART_CLK_PRED_OFFSET;

	podf = (reg & MXC_CCM_CSCDR1_UART_CLK_PODF_MASK) >>
		MXC_CCM_CSCDR1_UART_CLK_PODF_OFFSET;
	freq /= (pred + 1) * (podf + 1);

	return freq;
}

/*!
+ * This function returns the low power audio clock.
+ */
u32 get_lp_apm(void)
{
	u32 ret_val = 0;
	u32 ccsr = __REG(MXC_CCM_CCSR);

	if (((ccsr >> 9) & 1) == 0)
		ret_val = CONFIG_MX51_HCLK_FREQ;
	else
		ret_val = ((32768 * 1024));

	return ret_val;
}

static u32 __get_cspi_clk(void)
{
	u32 ret_val = 0, pdf, pre_pdf, clk_sel;
	u32 cscmr1 = __REG(MXC_CCM_CSCMR1);
	u32 cscdr2 = __REG(MXC_CCM_CSCDR2);

	pre_pdf = (cscdr2 & MXC_CCM_CSCDR2_CSPI_CLK_PRED_MASK) \
			>> MXC_CCM_CSCDR2_CSPI_CLK_PRED_OFFSET;
	pdf = (cscdr2 & MXC_CCM_CSCDR2_CSPI_CLK_PODF_MASK) \
			>> MXC_CCM_CSCDR2_CSPI_CLK_PODF_OFFSET;
	clk_sel = (cscmr1 & MXC_CCM_CSCMR1_CSPI_CLK_SEL_MASK) \
			>> MXC_CCM_CSCMR1_CSPI_CLK_SEL_OFFSET;

	switch (clk_sel) {
	case 0:
		ret_val = __decode_pll(PLL1_CLK, CONFIG_MX51_HCLK_FREQ) / ((pre_pdf + 1) * (pdf + 1));
		break;
	case 1:
		ret_val = __decode_pll(PLL2_CLK, CONFIG_MX51_HCLK_FREQ) / ((pre_pdf + 1) * (pdf + 1));
		break;
	case 2:
		ret_val = __decode_pll(PLL3_CLK, CONFIG_MX51_HCLK_FREQ) / ((pre_pdf + 1) * (pdf + 1));
		break;
	default:
		ret_val = get_lp_apm() / ((pre_pdf + 1) * (pdf + 1));
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
		break;
	case MXC_IPG_CLK:
		return __get_ipg_clk();
	case MXC_IPG_PERCLK:
		return __get_ipg_per_clk();
	case MXC_UART_CLK:
		return __get_uart_clk();
	case MXC_CSPI_CLK:
		return __get_cspi_clk();
	case MXC_FEC_CLK:
		return __decode_pll(PLL1_CLK, CONFIG_MX51_HCLK_FREQ);
	case MXC_ESDHC_CLK:
		return __decode_pll(PLL3_CLK, CONFIG_MX51_HCLK_FREQ);
	default:
		break;
	}
	return -1;
}

void mxc_dump_clocks(void)
{
	u32 freq;
	freq = __decode_pll(PLL1_CLK, CONFIG_MX51_HCLK_FREQ);
	printf("mx51 pll1: %dMHz\n", freq / 1000000);
	freq = __decode_pll(PLL2_CLK, CONFIG_MX51_HCLK_FREQ);
	printf("mx51 pll2: %dMHz\n", freq / 1000000);
	freq = __decode_pll(PLL3_CLK, CONFIG_MX51_HCLK_FREQ);
	printf("mx51 pll3: %dMHz\n", freq / 1000000);
	printf("ipg clock     : %dHz\n", mxc_get_clock(MXC_IPG_CLK));
	printf("ipg per clock : %dHz\n", mxc_get_clock(MXC_IPG_PERCLK));
	printf("uart clock    : %dHz\n", mxc_get_clock(MXC_UART_CLK));
	printf("cspi clock    : %dHz\n", mxc_get_clock(MXC_CSPI_CLK));
}

#if defined(CONFIG_DISPLAY_CPUINFO)
int print_cpuinfo(void)
{
	printf("CPU:   Freescale i.MX51 family %d.%dV at %d MHz\n",
	       (get_board_rev() & 0xFF) >> 4,
	       (get_board_rev() & 0xF),
		__get_mcu_main_clk() / 1000000);
	mxc_dump_clocks();
	return 0;
}
#endif

/*
 * Initializes on-chip ethernet controllers.
 * to override, implement board_eth_init()
 */
 #if defined(CONFIG_MXC_FEC)
 extern int mxc_fec_initialize(bd_t *bis);
 #endif

int cpu_eth_init(bd_t *bis)
{
	int rc = -ENODEV;
	char *env = NULL;

#if defined(CONFIG_MXC_FEC)
	rc = mxc_fec_initialize(bis);
#endif

	env = getenv("fec_addr");
	if (env)
		mxc_fec_set_mac_from_env(env);

	return rc;
}

#if defined(CONFIG_ARCH_CPU_INIT)
int arch_cpu_init(void)
{
	icache_enable();
	dcache_enable();
	return 0;
}
#endif
