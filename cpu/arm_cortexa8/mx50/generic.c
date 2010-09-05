/*
 * Copyright (C) 2010 Freescale Semiconductor, Inc. All Rights Reserved.
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
#include <asm/arch/mx50.h>
#include <asm/errno.h>
#include <asm/io.h>
#include "crm_regs.h"
#ifdef CONFIG_CMD_CLOCK
#include <asm/clock.h>
#endif
#include <div64.h>
#ifdef CONFIG_ARCH_CPU_INIT
#include <asm/cache-cp15.h>
#endif

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

#define AHB_CLK_ROOT 133333333
#define IPG_CLK_ROOT 66666666
#define IPG_PER_CLK_ROOT 40000000

#ifdef CONFIG_CMD_CLOCK
#define SZ_DEC_1M       1000000
#define PLL_PD_MAX      16      /* Actual pd+1 */
#define PLL_MFI_MAX     15
#define PLL_MFI_MIN     5
#define ARM_DIV_MAX     8
#define IPG_DIV_MAX     4
#define AHB_DIV_MAX     8
#define EMI_DIV_MAX     8
#define NFC_DIV_MAX     8

struct fixed_pll_mfd {
    u32 ref_clk_hz;
    u32 mfd;
};

const struct fixed_pll_mfd fixed_mfd[4] = {
    {0,                   0},      /* reserved */
    {0,                   0},      /* reserved */
    {CONFIG_MX50_HCLK_FREQ, 24 * 16},    /* 384 */
    {0,                   0},      /* reserved */
};

struct pll_param {
    u32 pd;
    u32 mfi;
    u32 mfn;
    u32 mfd;
};

#define PLL_FREQ_MAX(_ref_clk_) \
		(4 * _ref_clk_ * PLL_MFI_MAX)
#define PLL_FREQ_MIN(_ref_clk_) \
		((2 * _ref_clk_ * (PLL_MFI_MIN - 1)) / PLL_PD_MAX)
#define MAX_DDR_CLK     420000000
#define AHB_CLK_MAX     133333333
#define IPG_CLK_MAX     (AHB_CLK_MAX / 2)
#define NFC_CLK_MAX     25000000
#define HSP_CLK_MAX     133333333
#endif

static u32 __decode_pll(enum pll_clocks pll, u32 infreq)
{
	long mfi, mfn, mfd, pdf, ref_clk, mfn_abs;
	unsigned long dp_op, dp_mfd, dp_mfn, dp_ctl, pll_hfsm, dbl;
	s64 temp;

	dp_ctl = __REG(pll + MXC_PLL_DP_CTL);
	pll_hfsm = dp_ctl & MXC_PLL_DP_CTL_HFSM;
	dbl = dp_ctl & MXC_PLL_DP_CTL_DPDCK0_2_EN;

	if (pll_hfsm == 0) {
		dp_op = __REG(pll + MXC_PLL_DP_OP);
		dp_mfd = __REG(pll + MXC_PLL_DP_MFD);
		dp_mfn = __REG(pll + MXC_PLL_DP_MFN);
	} else {
		dp_op = __REG(pll + MXC_PLL_DP_HFS_OP);
		dp_mfd = __REG(pll + MXC_PLL_DP_HFS_MFD);
		dp_mfn = __REG(pll + MXC_PLL_DP_HFS_MFN);
	}
	pdf = dp_op & MXC_PLL_DP_OP_PDF_MASK;
	mfi = (dp_op & MXC_PLL_DP_OP_MFI_MASK) >> MXC_PLL_DP_OP_MFI_OFFSET;
	mfi = (mfi <= 5) ? 5 : mfi;
	mfd = dp_mfd & MXC_PLL_DP_MFD_MASK;
	mfn = mfn_abs = dp_mfn & MXC_PLL_DP_MFN_MASK;
	/* Sign extend to 32-bits */
	if (mfn >= 0x04000000) {
		mfn |= 0xFC000000;
		mfn_abs = -mfn;
	}

	ref_clk = 2 * infreq;
	if (dbl != 0)
		ref_clk *= 2;

	ref_clk /= (pdf + 1);
	temp = (u64) ref_clk * mfn_abs;
	do_div(temp, mfd + 1);
	if (mfn < 0)
		temp = -temp;
	temp = (ref_clk * mfi) + temp;

	return temp;
}

static u32 __get_mcu_main_clk(void)
{
	u32 reg, freq;
	reg = (__REG(MXC_CCM_CACRR) & MXC_CCM_CACRR_ARM_PODF_MASK) >>
	    MXC_CCM_CACRR_ARM_PODF_OFFSET;
	freq = __decode_pll(PLL1_CLK, CONFIG_MX50_HCLK_FREQ);
	return freq / (reg + 1);
}

/*
 * This function returns the low power audio clock.
 */
u32 __get_lp_apm(void)
{
	u32 ret_val = 0;
	u32 cbcmr = __REG(MXC_CCM_CBCMR);

	if (((cbcmr >> MXC_CCM_CBCMR_LP_APM_SEL_OFFSET) & 0x1) == 0)
		ret_val = CONFIG_MX50_HCLK_FREQ;
	else
		ret_val = ((32768 * 1024));

	return ret_val;
}

static u32 __get_periph_clk(void)
{
	u32 reg;
	reg = __REG(MXC_CCM_CBCDR);

	switch ((reg & MXC_CCM_CBCDR_PERIPH_CLK_SEL_MASK) >>
		MXC_CCM_CBCDR_PERIPH_CLK_SEL_OFFSET) {
	case 0:
		return __decode_pll(PLL1_CLK, CONFIG_MX50_HCLK_FREQ);
	case 1:
		return __decode_pll(PLL2_CLK, CONFIG_MX50_HCLK_FREQ);
	case 2:
		return __decode_pll(PLL3_CLK, CONFIG_MX50_HCLK_FREQ);
	default:
		return __get_lp_apm();
	}
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
	u32 pred1, pred2, podf, clk;
	if (__REG(MXC_CCM_CBCMR) & MXC_CCM_CBCMR_PERCLK_IPG_CLK_SEL)
		return __get_ipg_clk();

	clk = __REG(MXC_CCM_CBCMR) & MXC_CCM_CBCMR_PERCLK_LP_APM_CLK_SEL ?
		__get_lp_apm() : __get_periph_clk();

	podf = __REG(MXC_CCM_CBCDR);
	pred1 = (podf & MXC_CCM_CBCDR_PERCLK_PRED1_MASK) >>
		MXC_CCM_CBCDR_PERCLK_PRED1_OFFSET;
	pred2 = (podf & MXC_CCM_CBCDR_PERCLK_PRED2_MASK) >>
		MXC_CCM_CBCDR_PERCLK_PRED2_OFFSET;
	podf = (podf & MXC_CCM_CBCDR_PERCLK_PODF_MASK) >>
		MXC_CCM_CBCDR_PERCLK_PODF_OFFSET;

	return clk / ((pred1 + 1) * (pred2 + 1) * (podf + 1));
}

static u32 __get_uart_clk(void)
{
	u32 freq = 0, reg, pred, podf;
	reg = __REG(MXC_CCM_CSCMR1);
	switch ((reg & MXC_CCM_CSCMR1_UART_CLK_SEL_MASK) >>
		MXC_CCM_CSCMR1_UART_CLK_SEL_OFFSET) {
	case 0x0:
		freq = __decode_pll(PLL1_CLK, CONFIG_MX50_HCLK_FREQ);
		break;
	case 0x1:
		freq = __decode_pll(PLL2_CLK, CONFIG_MX50_HCLK_FREQ);
		break;
	case 0x2:
		freq = __decode_pll(PLL3_CLK, CONFIG_MX50_HCLK_FREQ);
		break;
	case 0x3:
		freq = __get_lp_apm();
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
		ret_val = __decode_pll(PLL1_CLK, CONFIG_MX50_HCLK_FREQ) / div;
		break;
	case 1:
		ret_val = __decode_pll(PLL2_CLK, CONFIG_MX50_HCLK_FREQ) / div;
		break;
	case 2:
		ret_val = __decode_pll(PLL3_CLK, CONFIG_MX50_HCLK_FREQ) / div;
		break;
	default:
		ret_val = __get_lp_apm() / div;
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
	u32 emi_clk_sel = cbcdr & MXC_CCM_CBCDR_WEIM_CLK_SEL;
	u32 pdf = (cbcdr & MXC_CCM_CBCDR_WEIM_PODF_MASK) \
			>> MXC_CCM_CBCDR_WEIM_PODF_OFFSET;

	if (emi_clk_sel)
		return  __get_ahb_clk() / (pdf + 1);

	return  __get_periph_clk() / (pdf + 1);
}

static u32 __get_sys_clk(void)
{
	u32 ret_val = 0, clk, sys_pll_div;
	u32 clkseq_bypass = __REG(MXC_CCM_CLKSEQ_BYPASS);

	/* Fixme Not handle OSC and PFD1 mux */
	if ((clkseq_bypass & MXC_CCM_CLKSEQ_BYPASS_SYS_CLK_1) &&
	    clkseq_bypass & MXC_CCM_CLKSEQ_BYPASS_SYS_CLK_0) {
		sys_pll_div = __REG(MXC_CCM_CLK_SYS) \
				& MXC_CCM_CLK_SYS_DIV_PLL_MASK;
		clk = __decode_pll(PLL1_CLK, CONFIG_MX50_HCLK_FREQ);
		if (sys_pll_div)
			clk /= sys_pll_div;
		ret_val = clk;
	} else if ((clkseq_bypass & MXC_CCM_CLKSEQ_BYPASS_SYS_CLK_0) == 0) {
		ret_val = CONFIG_MX50_HCLK_FREQ; /* OSC */
	}  else {

		printf("Warning, Fixme Not handle PFD1 mux\n");
	}

	return ret_val;

}
static u32 __get_ddr_clk(void)
{
	u32 ret_val = 0, clk, ddr_pll_div;
	u32 clk_ddr = __REG(MXC_CCM_CLK_DDR);
	u32 ddr_clk_sel = clk_ddr & MXC_CCM_CLK_DDR_DDR_PFD_SEL;

	if (!ddr_clk_sel) {
		ddr_pll_div = clk_ddr & \
			MXC_CCM_CLK_DDR_DDR_DIV_PLL_MASK;
		clk = __decode_pll(PLL1_CLK, CONFIG_MX50_HCLK_FREQ);
		if (ddr_pll_div)
			clk /= ddr_pll_div;
		ret_val = clk;
	} else {

		printf("Warning, Fixme Not handle PFD1 mux\n");
	}

	return ret_val;
}

#ifdef CONFIG_CMD_MMC
static u32 __get_esdhc1_clk(void)
{
	u32 ret_val = 0, div, pre_pdf, pdf;
	u32 cscmr1 = __REG(MXC_CCM_CSCMR1);
	u32 cscdr1 = __REG(MXC_CCM_CSCDR1);
	u32 esdh1_clk_sel;

	esdh1_clk_sel = (cscmr1 & MXC_CCM_CSCMR1_ESDHC1_CLK_SEL_MASK) \
				>> MXC_CCM_CSCMR1_ESDHC1_CLK_SEL_OFFSET;
	pre_pdf = (cscdr1 & MXC_CCM_CSCDR1_ESDHC1_CLK_PRED_MASK) \
			>> MXC_CCM_CSCDR1_ESDHC1_CLK_PRED_OFFSET;
	pdf = (cscdr1 & MXC_CCM_CSCDR1_ESDHC1_CLK_PODF_MASK) \
			>> MXC_CCM_CSCDR1_ESDHC1_CLK_PODF_OFFSET ;

	div = (pre_pdf + 1) * (pdf + 1);

	switch (esdh1_clk_sel) {
	case 0:
		ret_val = __decode_pll(PLL1_CLK, CONFIG_MX50_HCLK_FREQ);
		break;
	case 1:
		ret_val = __decode_pll(PLL2_CLK, CONFIG_MX50_HCLK_FREQ);
		break;
	case 2:
		ret_val = __decode_pll(PLL3_CLK, CONFIG_MX50_HCLK_FREQ);
		break;
	case 3:
		ret_val = __get_lp_apm();
		break;
	default:
		break;
	}

	ret_val /= div;

	return ret_val;
}

static u32 __get_esdhc3_clk(void)
{
	u32 ret_val = 0, div, pre_pdf, pdf;
	u32 esdh3_clk_sel;
	u32 cscmr1 = __REG(MXC_CCM_CSCMR1);
	u32 cscdr1 = __REG(MXC_CCM_CSCDR1);
	esdh3_clk_sel = (cscmr1 & MXC_CCM_CSCMR1_ESDHC3_CLK_SEL_MASK) \
				>> MXC_CCM_CSCMR1_ESDHC3_CLK_SEL_OFFSET;
	pre_pdf = (cscdr1 & MXC_CCM_CSCDR1_ESDHC3_CLK_PRED_MASK) \
			>> MXC_CCM_CSCDR1_ESDHC3_CLK_PRED_OFFSET;
	pdf = (cscdr1 & MXC_CCM_CSCDR1_ESDHC3_CLK_PODF_MASK) \
			>> MXC_CCM_CSCDR1_ESDHC3_CLK_PODF_OFFSET ;

	div = (pre_pdf + 1) * (pdf + 1);

	switch (esdh3_clk_sel) {
	case 0:
		ret_val = __decode_pll(PLL1_CLK, CONFIG_MX50_HCLK_FREQ);
		break;
	case 1:
		ret_val = __decode_pll(PLL2_CLK, CONFIG_MX50_HCLK_FREQ);
		break;
	case 2:
		ret_val = __decode_pll(PLL3_CLK, CONFIG_MX50_HCLK_FREQ);
		break;
	case 3:
		ret_val = __get_lp_apm();
		break;
	case 5 ... 8:
		puts("Warning, Fixme,not handle PFD mux\n");

		break;
	default:
		break;
	}

	ret_val /= div;

	return ret_val;
}

static u32 __get_esdhc2_clk(void)
{
	u32 cscmr1 = __REG(MXC_CCM_CSCMR1);
	u32 esdh2_clk_sel = cscmr1 & MXC_CCM_CSCMR1_ESDHC2_CLK_SEL;
	if (esdh2_clk_sel)
		return __get_esdhc3_clk();

	return __get_esdhc1_clk();
}

static u32 __get_esdhc4_clk(void)
{
	u32 cscmr1 = __REG(MXC_CCM_CSCMR1);
	u32 esdh4_clk_sel = cscmr1 & MXC_CCM_CSCMR1_ESDHC4_CLK_SEL;
	if (esdh4_clk_sel)
		return __get_esdhc3_clk();

	return __get_esdhc1_clk();
}
#endif

#ifdef CONFIG_CMD_NAND
static inline void __enable_gpmi_clk(void)
{
	u32 reg1 = __REG(MXC_CCM_GPMI);

	reg1 |= 0x80000000;

	writel(reg1, MXC_CCM_GPMI);
}

static inline void __enable_bch_clk(void)
{
	u32 reg1 = __REG(MXC_CCM_BCH);

	reg1 |= 0x80000000;

	writel(reg1, MXC_CCM_BCH);
}


static u32 __get_gpmi_clk(void)
{
	u32 ret_val = 0;
	u32 clkseq_bypass = __REG(MXC_CCM_CLKSEQ_BYPASS);
	u32 clk_sel = (clkseq_bypass & MXC_CCM_CLKSEQ_BYPASS_GPMI_MASK)
			>> MXC_CCM_CLKSEQ_BYPASS_GPMI_OFFSET;
	u32 div = __REG(MXC_CCM_GPMI) & 0x3f;

	__enable_gpmi_clk();

	switch (clk_sel) {
	case 0:
		/* 24MHz xtal */
		ret_val = CONFIG_MX50_HCLK_FREQ;
		break;
	case 1:
		/* PFD4 */
		puts("Warning, Fixme,not handle PFD mux\n");
		break;
	case 2:
		/* PLL1 */
		ret_val = __decode_pll(PLL1_CLK, CONFIG_MX50_HCLK_FREQ);
		break;
	default:
		break;
	}

	return (div > 0) ? (ret_val / div) : 0;
}

static u32 __get_bch_clk(void)
{
	u32 ret_val = 0;
	u32 clkseq_bypass = __REG(MXC_CCM_CLKSEQ_BYPASS);
	u32 clk_sel = (clkseq_bypass & MXC_CCM_CLKSEQ_BYPASS_BCH_MASK)
			>> MXC_CCM_CLKSEQ_BYPASS_BCH_OFFSET;
	u32 div = __REG(MXC_CCM_BCH) & 0x3f;

	__enable_bch_clk();

	switch (clk_sel) {
	case 0:
		/* 24MHz xtal */
		ret_val = CONFIG_MX50_HCLK_FREQ;
		break;
	case 1:
		/* PFD4 */
		puts("Warning, Fixme,not handle PFD mux\n");
		break;
	case 2:
		/* PLL1 */
		ret_val = __decode_pll(PLL1_CLK, CONFIG_MX50_HCLK_FREQ);
		break;
	default:
		break;
	}

	return (div > 0) ? (ret_val / div) : 0;
}

#endif

unsigned int mxc_get_clock(enum mxc_clock clk)
{
	switch (clk) {
	case MXC_ARM_CLK:
		return __get_mcu_main_clk();
	case MXC_PER_CLK:
		return __get_periph_clk();
	case MXC_AHB_CLK:
		return __get_ahb_clk();
	case MXC_IPG_CLK:
		return __get_ipg_clk();
	case MXC_IPG_PERCLK:
		return __get_ipg_per_clk();
	case MXC_UART_CLK:
		return __get_uart_clk();
#ifdef CONFIG_IMX_CSPI
	case MXC_CSPI_CLK:
		return __get_cspi_clk();
#endif
	case MXC_AXI_A_CLK:
		return __get_axi_a_clk();
	case MXC_AXI_B_CLK:
		return __get_axi_b_clk();
	case MXC_EMI_SLOW_CLK:
		return __get_emi_slow_clk();
	case MXC_DDR_CLK:
		return __get_ddr_clk();
#ifdef CONFIG_CMD_MMC
	case MXC_ESDHC_CLK:
		return __get_esdhc1_clk();
	case MXC_ESDHC2_CLK:
		return __get_esdhc2_clk();
	case MXC_ESDHC3_CLK:
		return __get_esdhc3_clk();
	case MXC_ESDHC4_CLK:
		return __get_esdhc4_clk();
#endif
#ifdef CONFIG_CMD_NAND
	case MXC_GPMI_CLK:
		return __get_gpmi_clk();
	case MXC_BCH_CLK:
		return __get_bch_clk();
#endif
	default:
		break;
	}
	return -1;
}

void mxc_dump_clocks(void)
{
	u32 freq;
	freq = __decode_pll(PLL1_CLK, CONFIG_MX50_HCLK_FREQ);
	printf("mx50 pll1: %dMHz\n", freq / 1000000);
	freq = __decode_pll(PLL2_CLK, CONFIG_MX50_HCLK_FREQ);
	printf("mx50 pll2: %dMHz\n", freq / 1000000);
	freq = __decode_pll(PLL3_CLK, CONFIG_MX50_HCLK_FREQ);
	printf("mx50 pll3: %dMHz\n", freq / 1000000);
	printf("ipg clock     : %dHz\n", mxc_get_clock(MXC_IPG_CLK));
	printf("ipg per clock : %dHz\n", mxc_get_clock(MXC_IPG_PERCLK));
	printf("uart clock    : %dHz\n", mxc_get_clock(MXC_UART_CLK));
#ifdef CONFIG_IMX_ECSPI
	printf("cspi clock    : %dHz\n", mxc_get_clock(MXC_CSPI_CLK));
#endif
	printf("ahb clock     : %dHz\n", mxc_get_clock(MXC_AHB_CLK));
	printf("axi_a clock   : %dHz\n", mxc_get_clock(MXC_AXI_A_CLK));
	printf("axi_b clock   : %dHz\n", mxc_get_clock(MXC_AXI_B_CLK));
	printf("weim_clock    : %dHz\n", mxc_get_clock(MXC_EMI_SLOW_CLK));
	printf("ddr clock     : %dHz\n", mxc_get_clock(MXC_DDR_CLK));
#ifdef CONFIG_CMD_MMC
	printf("esdhc1 clock  : %dHz\n", mxc_get_clock(MXC_ESDHC_CLK));
	printf("esdhc2 clock  : %dHz\n", mxc_get_clock(MXC_ESDHC2_CLK));
	printf("esdhc3 clock  : %dHz\n", mxc_get_clock(MXC_ESDHC3_CLK));
	printf("esdhc4 clock  : %dHz\n", mxc_get_clock(MXC_ESDHC4_CLK));
#endif
#ifdef CONFIG_CMD_NAND
	printf("GPMI clock    : %dHz\n", mxc_get_clock(MXC_GPMI_CLK));
	printf("BCH clock     : %dHz\n", mxc_get_clock(MXC_BCH_CLK));
#endif
}

#ifdef CONFIG_CMD_CLOCK
/* precondition: m>0 and n>0.  Let g=gcd(m,n). */
static int gcd(int m, int n)
{
	int t;
	while (m > 0) {
		if (n > m) {
			t = m;
			m = n;
			n = t;
		} /* swap */
		m -= n;
	}
	return n;
}

/*!
 * This is to calculate various parameters based on reference clock and
 * targeted clock based on the equation:
 *      t_clk = 2*ref_freq*(mfi + mfn/(mfd+1))/(pd+1)
 * This calculation is based on a fixed MFD value for simplicity.
 *
 * @param ref       reference clock freq in Hz
 * @param target    targeted clock in Hz
 * @param pll		pll_param structure.
 *
 * @return          0 if successful; non-zero otherwise.
 */
static int calc_pll_params(u32 ref, u32 target, struct pll_param *pll)
{
	u64 pd, mfi = 1, mfn, mfd, t1;
	u32 n_target = target;
	u32 n_ref = ref, i;

	/*
	 * Make sure targeted freq is in the valid range.
	 * Otherwise the following calculation might be wrong!!!
	 */
	if (n_target < PLL_FREQ_MIN(ref) ||
		n_target > PLL_FREQ_MAX(ref)) {
		printf("Targeted peripheral clock should be"
			"within [%d - %d]\n",
			PLL_FREQ_MIN(ref) / SZ_DEC_1M,
			PLL_FREQ_MAX(ref) / SZ_DEC_1M);
		return -1;
	}

	for (i = 0; i < ARRAY_SIZE(fixed_mfd); i++) {
		if (fixed_mfd[i].ref_clk_hz == ref) {
			mfd = fixed_mfd[i].mfd;
			break;
		}
	}

	if (i == ARRAY_SIZE(fixed_mfd))
		return -1;

	/* Use n_target and n_ref to avoid overflow */
	for (pd = 1; pd <= PLL_PD_MAX; pd++) {
		t1 = n_target * pd;
		do_div(t1, (4 * n_ref));
		mfi = t1;
		if (mfi > PLL_MFI_MAX)
			return -1;
		else if (mfi < 5)
			continue;
		break;
	}
	/* Now got pd and mfi already */
	/*
	mfn = (((n_target * pd) / 4 - n_ref * mfi) * mfd) / n_ref;
	*/
	t1 = n_target * pd;
	do_div(t1, 4);
	t1 -= n_ref * mfi;
	t1 *= mfd;
	do_div(t1, n_ref);
	mfn = t1;
#ifdef CMD_CLOCK_DEBUG
	printf("%d: ref=%d, target=%d, pd=%d,"
			"mfi=%d,mfn=%d, mfd=%d\n",
			__LINE__, ref, (u32)n_target,
			(u32)pd, (u32)mfi, (u32)mfn,
			(u32)mfd);
#endif
	i = 1;
	if (mfn != 0)
		i = gcd(mfd, mfn);
	pll->pd = (u32)pd;
	pll->mfi = (u32)mfi;
	do_div(mfn, i);
	pll->mfn = (u32)mfn;
	do_div(mfd, i);
	pll->mfd = (u32)mfd;

	return 0;
}

int clk_info(u32 clk_type)
{
	switch (clk_type) {
	case CPU_CLK:
		printf("CPU Clock: %dHz\n",
			mxc_get_clock(MXC_ARM_CLK));
		break;
	case PERIPH_CLK:
		printf("Peripheral Clock: %dHz\n",
			mxc_get_clock(MXC_PER_CLK));
		break;
	case AHB_CLK:
		printf("AHB Clock: %dHz\n",
			mxc_get_clock(MXC_AHB_CLK));
		break;
	case IPG_CLK:
		printf("IPG Clock: %dHz\n",
			mxc_get_clock(MXC_IPG_CLK));
		break;
	case IPG_PERCLK:
		printf("IPG_PER Clock: %dHz\n",
			mxc_get_clock(MXC_IPG_PERCLK));
		break;
	case UART_CLK:
		printf("UART Clock: %dHz\n",
			mxc_get_clock(MXC_UART_CLK));
		break;
	case CSPI_CLK:
		printf("CSPI Clock: %dHz\n",
			mxc_get_clock(MXC_CSPI_CLK));
		break;
	case DDR_CLK:
		printf("DDR Clock: %dHz\n",
			mxc_get_clock(MXC_DDR_CLK));
		break;
	case ALL_CLK:
		printf("cpu clock: %dMHz\n",
			mxc_get_clock(MXC_ARM_CLK) / SZ_DEC_1M);
		mxc_dump_clocks();
		break;
	default:
		printf("Unsupported clock type! :(\n");
	}

	return 0;
}

#define calc_div(target_clk, src_clk, limit) ({	\
		u32 tmp = 0;	\
		if ((src_clk % target_clk) <= 100)	\
			tmp = src_clk / target_clk;	\
		else	\
			tmp = (src_clk / target_clk) + 1;	\
		if (tmp > limit)	\
			tmp = limit;	\
		(tmp - 1);	\
	})

u32 calc_per_cbcdr_val(u32 per_clk, u32 cbcmr)
{
	u32 cbcdr = __REG(MXC_CCM_CBCDR);
	u32 tmp_clk = 0, div = 0, clk_sel = 0;

	cbcdr &= ~MXC_CCM_CBCDR_PERIPH_CLK_SEL;

	/* emi_slow_podf divider */
	tmp_clk = __get_emi_slow_clk();
	clk_sel = cbcdr & MXC_CCM_CBCDR_EMI_CLK_SEL;
	if (clk_sel) {
		div = calc_div(tmp_clk, per_clk, 8);
		cbcdr &= ~MXC_CCM_CBCDR_EMI_PODF_MASK;
		cbcdr |= (div << MXC_CCM_CBCDR_EMI_PODF_OFFSET);
	}

	/* axi_b_podf divider */
	tmp_clk = __get_axi_b_clk();
	div = calc_div(tmp_clk, per_clk, 8);
	cbcdr &= ~MXC_CCM_CBCDR_AXI_B_PODF_MASK;
	cbcdr |= (div << MXC_CCM_CBCDR_AXI_B_PODF_OFFSET);

	/* axi_b_podf divider */
	tmp_clk = __get_axi_a_clk();
	div = calc_div(tmp_clk, per_clk, 8);
	cbcdr &= ~MXC_CCM_CBCDR_AXI_A_PODF_MASK;
	cbcdr |= (div << MXC_CCM_CBCDR_AXI_A_PODF_OFFSET);

	/* ahb podf divider */
	tmp_clk = AHB_CLK_ROOT;
	div = calc_div(tmp_clk, per_clk, 8);
	cbcdr &= ~MXC_CCM_CBCDR_AHB_PODF_MASK;
	cbcdr |= (div << MXC_CCM_CBCDR_AHB_PODF_OFFSET);

	return cbcdr;
}

#define CHANGE_PLL_SETTINGS(base, pd, mfi, mfn, mfd) \
	{	\
		writel(0x1232, base + PLL_DP_CTL); \
		writel(0x2, base + PLL_DP_CONFIG);    \
		writel(((pd - 1) << 0) | (mfi << 4),	\
			base + PLL_DP_OP);	\
		writel(mfn, base + PLL_DP_MFN);	\
		writel(mfd - 1, base + PLL_DP_MFD);	\
		writel(((pd - 1) << 0) | (mfi << 4),	\
			base + PLL_DP_HFS_OP);	\
		writel(mfn, base + PLL_DP_HFS_MFN);	\
		writel(mfd - 1, base + PLL_DP_HFS_MFD);	\
		writel(0x1232, base + PLL_DP_CTL); \
		while (!readl(base + PLL_DP_CTL) & 0x1)  \
			; \
	}

int config_pll_clk(enum pll_clocks pll, struct pll_param *pll_param)
{
	u32 ccsr = readl(CCM_BASE_ADDR + CLKCTL_CCSR);
	u32 pll_base = pll;

	switch (pll) {
	case PLL1_CLK:
		/* Switch ARM to PLL2 clock */
		writel(ccsr | 0x4, CCM_BASE_ADDR + CLKCTL_CCSR);
		CHANGE_PLL_SETTINGS(pll_base, pll_param->pd,
					pll_param->mfi, pll_param->mfn,
					pll_param->mfd);
		/* Switch back */
		writel(ccsr & ~0x4, CCM_BASE_ADDR + CLKCTL_CCSR);
		break;
	case PLL2_CLK:
		/* Switch to pll2 bypass clock */
		writel(ccsr | 0x2, CCM_BASE_ADDR + CLKCTL_CCSR);
		CHANGE_PLL_SETTINGS(pll_base, pll_param->pd,
					pll_param->mfi, pll_param->mfn,
					pll_param->mfd);
		/* Switch back */
		writel(ccsr & ~0x2, CCM_BASE_ADDR + CLKCTL_CCSR);
		break;
	case PLL3_CLK:
		/* Switch to pll3 bypass clock */
		writel(ccsr | 0x1, CCM_BASE_ADDR + CLKCTL_CCSR);
		CHANGE_PLL_SETTINGS(pll_base, pll_param->pd,
					pll_param->mfi, pll_param->mfn,
					pll_param->mfd);
		/* Switch back */
		writel(ccsr & ~0x1, CCM_BASE_ADDR + CLKCTL_CCSR);
		break;
	default:
		return -1;
	}

	return 0;
}

int config_core_clk(u32 ref, u32 freq)
{
	int ret = 0;
	u32 pll = 0;
	struct pll_param pll_param;

	memset(&pll_param, 0, sizeof(struct pll_param));

	/* The case that periph uses PLL1 is not considered here */
	pll = freq;
	ret = calc_pll_params(ref, pll, &pll_param);
	if (ret != 0) {
		printf("Can't find pll parameters: %d\n",
			ret);
		return ret;
	}

	return config_pll_clk(PLL1_CLK, &pll_param);
}

int config_periph_clk(u32 ref, u32 freq)
{
	int ret = 0;
	u32 pll = freq;
	struct pll_param pll_param;

	memset(&pll_param, 0, sizeof(struct pll_param));

	if (__REG(MXC_CCM_CBCDR) & MXC_CCM_CBCDR_PERIPH_CLK_SEL) {
		/* Actually this case is not considered here */
		ret = calc_pll_params(ref, pll, &pll_param);
		if (ret != 0) {
			printf("Can't find pll parameters: %d\n",
				ret);
			return ret;
		}
		switch ((__REG(MXC_CCM_CBCMR) & \
			MXC_CCM_CBCMR_PERIPH_CLK_SEL_MASK) >>
			MXC_CCM_CBCMR_PERIPH_CLK_SEL_OFFSET) {
		case 0:
			return config_pll_clk(PLL1_CLK, &pll_param);
			break;
		case 1:
			return config_pll_clk(PLL3_CLK, &pll_param);
			break;
		default:
			return -1;
		}
	} else {
		u32 old_cbcmr = readl(CCM_BASE_ADDR + CLKCTL_CBCMR);
		u32 new_cbcdr = calc_per_cbcdr_val(pll, old_cbcmr);

		/* Switch peripheral to PLL3 */
		writel(0x00015154, CCM_BASE_ADDR + CLKCTL_CBCMR);
		writel(0x02888945, CCM_BASE_ADDR + CLKCTL_CBCDR);

		/* Make sure change is effective */
		while (readl(CCM_BASE_ADDR + CLKCTL_CDHIPR) != 0)
			;

		/* Setup PLL2 */
		ret = calc_pll_params(ref, pll, &pll_param);
		if (ret != 0) {
			printf("Can't find pll parameters: %d\n",
				ret);
			return ret;
		}
		config_pll_clk(PLL2_CLK, &pll_param);

		/* Switch peripheral back */
		writel(new_cbcdr, CCM_BASE_ADDR + CLKCTL_CBCDR);
		writel(old_cbcmr, CCM_BASE_ADDR + CLKCTL_CBCMR);

		/* Make sure change is effective */
		while (readl(CCM_BASE_ADDR + CLKCTL_CDHIPR) != 0)
			;
		puts("\n");
	}

	return 0;
}

int config_ddr_clk(u32 emi_clk)
{
	u32 clk_src;
	s32 shift = 0, clk_sel, div = 1;
	u32 cbcmr = readl(CCM_BASE_ADDR + CLKCTL_CBCMR);
	u32 cbcdr = readl(CCM_BASE_ADDR + CLKCTL_CBCDR);

	if (emi_clk > MAX_DDR_CLK) {
		printf("DDR clock should be less than"
			"%d MHz, assuming max value \n",
			(MAX_DDR_CLK / SZ_DEC_1M));
		emi_clk = MAX_DDR_CLK;
	}

	clk_src = __get_periph_clk();
	/* Find DDR clock input */
	clk_sel = (cbcmr >> 10) & 0x3;
	switch (clk_sel) {
	case 0:
		shift = 16;
		break;
	case 1:
		shift = 19;
		break;
	case 2:
		shift = 22;
		break;
	case 3:
		shift = 10;
		break;
	default:
		return -1;
	}

	if ((clk_src % emi_clk) == 0)
		div = clk_src / emi_clk;
	else
		div = (clk_src / emi_clk) + 1;
	if (div > 8)
		div = 8;

	cbcdr = cbcdr & ~(0x7 << shift);
	cbcdr |= ((div - 1) << shift);
	writel(cbcdr, CCM_BASE_ADDR + CLKCTL_CBCDR);
	while (readl(CCM_BASE_ADDR + CLKCTL_CDHIPR) != 0)
		;
	writel(0x0, CCM_BASE_ADDR + CLKCTL_CCDR);

	return 0;
}

/*!
 * This function assumes the expected core clock has to be changed by
 * modifying the PLL. This is NOT true always but for most of the times,
 * it is. So it assumes the PLL output freq is the same as the expected
 * core clock (presc=1) unless the core clock is less than PLL_FREQ_MIN.
 * In the latter case, it will try to increase the presc value until
 * (presc*core_clk) is greater than PLL_FREQ_MIN. It then makes call to
 * calc_pll_params() and obtains the values of PD, MFI,MFN, MFD based
 * on the targeted PLL and reference input clock to the PLL. Lastly,
 * it sets the register based on these values along with the dividers.
 * Note 1) There is no value checking for the passed-in divider values
 *         so the caller has to make sure those values are sensible.
 *      2) Also adjust the NFC divider such that the NFC clock doesn't
 *         exceed NFC_CLK_MAX.
 *      3) IPU HSP clock is independent of AHB clock. Even it can go up to
 *         177MHz for higher voltage, this function fixes the max to 133MHz.
 *      4) This function should not have allowed diag_printf() calls since
 *         the serial driver has been stoped. But leave then here to allow
 *         easy debugging by NOT calling the cyg_hal_plf_serial_stop().
 *
 * @param ref       pll input reference clock (24MHz)
 * @param freq		core clock in Hz
 * @param clk_type  clock type, e.g CPU_CLK, DDR_CLK, etc.
 * @return          0 if successful; non-zero otherwise
 */
int clk_config(u32 ref, u32 freq, u32 clk_type)
{
	freq *= SZ_DEC_1M;

	switch (clk_type) {
	case CPU_CLK:
		if (config_core_clk(ref, freq))
			return -1;
		break;
	case PERIPH_CLK:
		if (config_periph_clk(ref, freq))
			return -1;
		break;
	case DDR_CLK:
		if (config_ddr_clk(freq))
			return -1;
		break;
	default:
		printf("Unsupported or invalid clock type! :(\n");
	}

	return 0;
}
#endif

#if defined(CONFIG_DISPLAY_CPUINFO)
int print_cpuinfo(void)
{
	printf("CPU:   Freescale i.MX50 family %d.%dV at %d MHz\n",
	       (get_board_rev() & 0xFF) >> 4,
	       (get_board_rev() & 0xF),
		__get_mcu_main_clk() / 1000000);
#ifndef CONFIG_CMD_CLOCK
		mxc_dump_clocks();
#endif
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
	rc = mxc_fec_initialize(bis);
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

