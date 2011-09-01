/*
 * Copyright (C) 2010-2011 Freescale Semiconductor, Inc.
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
#include <asm/arch/mx6.h>
#include <asm/arch/regs-anadig.h>
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
	CPU_PLL1,	/* System PLL */
	BUS_PLL2,	/* System Bus PLL*/
	USBOTG_PLL3,    /* OTG USB PLL */
	AUD_PLL4,	/* Audio PLL */
	VID_PLL5,	/* Video PLL */
	MLB_PLL6,	/* MLB PLL */
	USBHOST_PLL7,   /* Host USB PLL */
	ENET_PLL8,      /* ENET PLL */
};

#define SZ_DEC_1M       1000000

/* Out-of-reset PFDs and clock source definitions */
#define PLL2_PFD0_FREQ	352000000
#define PLL2_PFD1_FREQ	594000000
#define PLL2_PFD2_FREQ	400000000
#define PLL2_PFD2_DIV_FREQ	200000000
#define PLL3_PFD0_FREQ	720000000
#define PLL3_PFD1_FREQ	540000000
#define PLL3_PFD2_FREQ	508200000
#define PLL3_PFD3_FREQ	454700000
#define PLL3_80M	80000000
#define PLL3_60M	60000000

#define AHB_CLK_ROOT 132000000
#define IPG_CLK_ROOT 66000000
#define ENET_FREQ_0	25000000
#define ENET_FREQ_1	50000000
#define ENET_FREQ_2	100000000
#define ENET_FREQ_3	125000000

#ifdef CONFIG_CMD_CLOCK
#define PLL1_FREQ_MAX	1300000000
#define PLL1_FREQ_MIN	650000000
#define PLL2_FREQ_MAX	528000000
#define PLL2_FREQ_MIN	480000000
#define MAX_DDR_CLK     PLL2_FREQ_MAX
#define AHB_CLK_MAX     132000000
#define IPG_CLK_MAX     (AHB_CLK_MAX >> 1)
#define NFC_CLK_MAX     PLL2_FREQ_MAX
#endif

static u32 __decode_pll(enum pll_clocks pll, u32 infreq)
{
	u32 div;

	switch (pll) {
	case CPU_PLL1:
		div = REG_RD(ANATOP_BASE_ADDR, HW_ANADIG_PLL_SYS) &
			BM_ANADIG_PLL_SYS_DIV_SELECT;
		return infreq * (div >> 1);
	case BUS_PLL2:
		div = REG_RD(ANATOP_BASE_ADDR, HW_ANADIG_PLL_528) &
			BM_ANADIG_PLL_528_DIV_SELECT;
		return infreq * (20 + (div << 1));
	case USBOTG_PLL3:
		div = REG_RD(ANATOP_BASE_ADDR, HW_ANADIG_USB2_PLL_480_CTRL) &
			BM_ANADIG_USB2_PLL_480_CTRL_DIV_SELECT;
		return infreq * (20 + (div << 1));
	case ENET_PLL8:
		div = REG_RD(ANATOP_BASE_ADDR, HW_ANADIG_PLL_ENET) &
			BM_ANADIG_PLL_ENET_DIV_SELECT;
		switch (div) {
		default:
		case 0:
			return ENET_FREQ_0;
		case 1:
			return ENET_FREQ_1;
		case 2:
			return ENET_FREQ_2;
		case 3:
			return ENET_FREQ_3;
		}
	case AUD_PLL4:
	case VID_PLL5:
	case MLB_PLL6:
	case USBHOST_PLL7:
	default:
		return 0;
	}
}

static u32 __get_mcu_main_clk(void)
{
	u32 reg, freq;
	reg = (__REG(MXC_CCM_CACRR) & MXC_CCM_CACRR_ARM_PODF_MASK) >>
	    MXC_CCM_CACRR_ARM_PODF_OFFSET;
	freq = __decode_pll(CPU_PLL1, CONFIG_MX6_HCLK_FREQ);
	return freq / (reg + 1);
}

static u32 __get_periph_clk(void)
{
	u32 reg;
	reg = __REG(MXC_CCM_CBCDR);
	if (reg & MXC_CCM_CBCDR_PERIPH_CLK_SEL) {
		reg = __REG(MXC_CCM_CBCMR);
		switch ((reg & MXC_CCM_CBCMR_PERIPH_CLK2_SEL_MASK) >>
			MXC_CCM_CBCMR_PERIPH_CLK2_SEL_OFFSET) {
		case 0:
			return __decode_pll(USBOTG_PLL3, CONFIG_MX6_HCLK_FREQ);
		case 1:
		case 2:
			return CONFIG_MX6_HCLK_FREQ;
		default:
			return 0;
		}
	} else {
		reg = __REG(MXC_CCM_CBCMR);
		switch ((reg & MXC_CCM_CBCMR_PRE_PERIPH_CLK_SEL_MASK) >>
			MXC_CCM_CBCMR_PRE_PERIPH_CLK_SEL_OFFSET) {
		default:
		case 0:
			return __decode_pll(BUS_PLL2, CONFIG_MX6_HCLK_FREQ);
		case 1:
			return PLL2_PFD2_FREQ;
		case 2:
			return PLL2_PFD0_FREQ;
		case 3:
			return PLL2_PFD2_DIV_FREQ;
		}
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
	u32 podf;
	u32 clk_root = __get_ipg_clk();

	podf = __REG(MXC_CCM_CSCMR1) & MXC_CCM_CSCMR1_PERCLK_PODF_MASK;
	return clk_root / (podf + 1);
}

static u32 __get_uart_clk(void)
{
	u32 freq = PLL3_80M, reg, podf;

	reg = __REG(MXC_CCM_CSCDR1);
	podf = (reg & MXC_CCM_CSCDR1_UART_CLK_PODF_MASK) >>
		MXC_CCM_CSCDR1_UART_CLK_PODF_OFFSET;
	freq /= (podf + 1);

	return freq;
}


static u32 __get_cspi_clk(void)
{
	u32 freq = PLL3_60M, reg, podf;

	reg = __REG(MXC_CCM_CSCDR2);
	podf = (reg & MXC_CCM_CSCDR2_ECSPI_CLK_PODF_MASK) >>
		MXC_CCM_CSCDR2_ECSPI_CLK_PODF_OFFSET;
	freq /= (podf + 1);

	return freq;
}

static u32 __get_axi_clk(void)
{
	u32 clkroot;
	u32 cbcdr =  __REG(MXC_CCM_CBCDR);
	u32 podf = (cbcdr & MXC_CCM_CBCDR_AXI_PODF_MASK) >>
		MXC_CCM_CBCDR_AXI_PODF_OFFSET;

	if (cbcdr & MXC_CCM_CBCDR_AXI_SEL) {
		if (cbcdr & MXC_CCM_CBCDR_AXI_ALT_SEL)
			clkroot = PLL2_PFD2_FREQ;
		else
			clkroot = PLL3_PFD1_FREQ;;
	} else
		clkroot = __get_periph_clk();

	return  clkroot / (podf + 1);
}

static u32 __get_ahb_clk(void)
{
	u32 cbcdr =  __REG(MXC_CCM_CBCDR);
	u32 podf = (cbcdr & MXC_CCM_CBCDR_AHB_PODF_MASK) \
			>> MXC_CCM_CBCDR_AHB_PODF_OFFSET;

	return  __get_periph_clk() / (podf + 1);
}

static u32 __get_emi_slow_clk(void)
{
	u32 cscmr1 =  __REG(MXC_CCM_CSCMR1);
	u32 emi_clk_sel = (cscmr1 & MXC_CCM_CSCMR1_ACLK_EMI_SLOW_MASK) >>
		MXC_CCM_CSCMR1_ACLK_EMI_SLOW_OFFSET;
	u32 podf = (cscmr1 & MXC_CCM_CSCMR1_ACLK_EMI_SLOW_PODF_MASK) >>
			MXC_CCM_CSCMR1_ACLK_EMI_PODF_OFFSET;

	switch (emi_clk_sel) {
	default:
	case 0:
		return __get_axi_clk() / (podf + 1);
	case 1:
		return __decode_pll(USBOTG_PLL3, CONFIG_MX6_HCLK_FREQ) /
			(podf + 1);
	case 2:
		return PLL2_PFD2_FREQ / (podf + 1);
	case 3:
		return PLL2_PFD0_FREQ / (podf + 1);
	}
}

static u32 __get_nfc_clk(void)
{
	u32 clkroot;
	u32 cs2cdr =  __REG(MXC_CCM_CS2CDR);
	u32 podf = (cs2cdr & MXC_CCM_CS2CDR_ENFC_CLK_PODF_MASK) \
			>> MXC_CCM_CS2CDR_ENFC_CLK_PODF_OFFSET;
	u32 pred = (cs2cdr & MXC_CCM_CS2CDR_ENFC_CLK_PRED_MASK) \
			>> MXC_CCM_CS2CDR_ENFC_CLK_PRED_OFFSET;

	switch ((cs2cdr & MXC_CCM_CS2CDR_ENFC_CLK_SEL_MASK) >>
		MXC_CCM_CS2CDR_ENFC_CLK_SEL_OFFSET) {
		default:
		case 0:
			clkroot = PLL2_PFD0_FREQ;
			break;
		case 1:
			clkroot = __decode_pll(BUS_PLL2, CONFIG_MX6_HCLK_FREQ);
			break;
		case 2:
			clkroot = __decode_pll(USBOTG_PLL3, CONFIG_MX6_HCLK_FREQ);
			break;
		case 3:
			clkroot = PLL2_PFD2_FREQ;
			break;
	}

	return  clkroot / pred / podf;
}

static u32 __get_ddr_clk(void)
{
	u32 cbcdr = __REG(MXC_CCM_CBCDR);
	u32 podf = (cbcdr & MXC_CCM_CBCDR_MMDC_CH0_PODF_MASK) >>
		MXC_CCM_CBCDR_MMDC_CH0_PODF_OFFSET;

	return __get_periph_clk() / (podf + 1);
}

static u32 __get_usdhc1_clk(void)
{
	u32 clkroot;
	u32 cscmr1 = __REG(MXC_CCM_CSCMR1);
	u32 cscdr1 = __REG(MXC_CCM_CSCDR1);
	u32 podf = (cscdr1 & MXC_CCM_CSCDR1_USDHC1_PODF_MASK) >>
		MXC_CCM_CSCDR1_USDHC1_PODF_OFFSET;

	if (cscmr1 & MXC_CCM_CSCMR1_USDHC1_CLK_SEL)
		clkroot = PLL2_PFD0_FREQ;
	else
		clkroot = PLL2_PFD2_FREQ;

	return clkroot / (podf + 1);
}

static u32 __get_usdhc2_clk(void)
{
	u32 clkroot;
	u32 cscmr1 = __REG(MXC_CCM_CSCMR1);
	u32 cscdr1 = __REG(MXC_CCM_CSCDR1);
	u32 podf = (cscdr1 & MXC_CCM_CSCDR1_USDHC2_PODF_MASK) >>
		MXC_CCM_CSCDR1_USDHC2_PODF_OFFSET;

	if (cscmr1 & MXC_CCM_CSCMR1_USDHC2_CLK_SEL)
		clkroot = PLL2_PFD0_FREQ;
	else
		clkroot = PLL2_PFD2_FREQ;

	return clkroot / (podf + 1);
}

static u32 __get_usdhc3_clk(void)
{
	u32 clkroot;
	u32 cscmr1 = __REG(MXC_CCM_CSCMR1);
	u32 cscdr1 = __REG(MXC_CCM_CSCDR1);
	u32 podf = (cscdr1 & MXC_CCM_CSCDR1_USDHC3_PODF_MASK) >>
		MXC_CCM_CSCDR1_USDHC3_PODF_OFFSET;

	if (cscmr1 & MXC_CCM_CSCMR1_USDHC3_CLK_SEL)
		clkroot = PLL2_PFD0_FREQ;
	else
		clkroot = PLL2_PFD2_FREQ;

	return clkroot / (podf + 1);
}

static u32 __get_usdhc4_clk(void)
{
	u32 clkroot;
	u32 cscmr1 = __REG(MXC_CCM_CSCMR1);
	u32 cscdr1 = __REG(MXC_CCM_CSCDR1);
	u32 podf = (cscdr1 & MXC_CCM_CSCDR1_USDHC4_PODF_MASK) >>
		MXC_CCM_CSCDR1_USDHC4_PODF_OFFSET;

	if (cscmr1 & MXC_CCM_CSCMR1_USDHC4_CLK_SEL)
		clkroot = PLL2_PFD0_FREQ;
	else
		clkroot = PLL2_PFD2_FREQ;

	return clkroot / (podf + 1);
}

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
	case MXC_CSPI_CLK:
		return __get_cspi_clk();
	case MXC_AXI_CLK:
		return __get_axi_clk();
	case MXC_EMI_SLOW_CLK:
		return __get_emi_slow_clk();
	case MXC_DDR_CLK:
		return __get_ddr_clk();
	case MXC_ESDHC_CLK:
		return __get_usdhc1_clk();
	case MXC_ESDHC2_CLK:
		return __get_usdhc2_clk();
	case MXC_ESDHC3_CLK:
		return __get_usdhc3_clk();
	case MXC_ESDHC4_CLK:
		return __get_usdhc4_clk();
	case MXC_SATA_CLK:
		return __get_ahb_clk();
	case MXC_NFC_CLK:
	  return __get_nfc_clk();
	default:
		break;
	}
	return -1;
}

void mxc_dump_clocks(void)
{
	u32 freq;
	freq = __decode_pll(CPU_PLL1, CONFIG_MX6_HCLK_FREQ);
	printf("mx6q pll1: %dMHz\n", freq / SZ_DEC_1M);
	freq = __decode_pll(BUS_PLL2, CONFIG_MX6_HCLK_FREQ);
	printf("mx6q pll2: %dMHz\n", freq / SZ_DEC_1M);
	freq = __decode_pll(USBOTG_PLL3, CONFIG_MX6_HCLK_FREQ);
	printf("mx6q pll3: %dMHz\n", freq / SZ_DEC_1M);
	freq = __decode_pll(ENET_PLL8, CONFIG_MX6_HCLK_FREQ);
	printf("mx6q pll8: %dMHz\n", freq / SZ_DEC_1M);
	printf("ipg clock     : %dHz\n", mxc_get_clock(MXC_IPG_CLK));
	printf("ipg per clock : %dHz\n", mxc_get_clock(MXC_IPG_PERCLK));
	printf("uart clock    : %dHz\n", mxc_get_clock(MXC_UART_CLK));
	printf("cspi clock    : %dHz\n", mxc_get_clock(MXC_CSPI_CLK));
	printf("ahb clock     : %dHz\n", mxc_get_clock(MXC_AHB_CLK));
	printf("axi clock   : %dHz\n", mxc_get_clock(MXC_AXI_CLK));
	printf("emi_slow clock: %dHz\n", mxc_get_clock(MXC_EMI_SLOW_CLK));
	printf("ddr clock     : %dHz\n", mxc_get_clock(MXC_DDR_CLK));
	printf("usdhc1 clock  : %dHz\n", mxc_get_clock(MXC_ESDHC_CLK));
	printf("usdhc2 clock  : %dHz\n", mxc_get_clock(MXC_ESDHC2_CLK));
	printf("usdhc3 clock  : %dHz\n", mxc_get_clock(MXC_ESDHC3_CLK));
	printf("usdhc4 clock  : %dHz\n", mxc_get_clock(MXC_ESDHC4_CLK));
	printf("nfc clock     : %dHz\n", mxc_get_clock(MXC_NFC_CLK));
}

#ifdef CONFIG_CMD_CLOCK

/*!
 * This is to calculate divider based on reference clock and
 * targeted clock based on the equation for each PLL.
 *
 * @param pll		pll number
 * @param ref       reference clock freq in Hz
 * @param target    targeted clock in Hz
 *
 * @return          divider if successful; -1 otherwise.
 */
static int calc_pll_divider(enum pll_clocks pll, u32 ref, u32 target)
{
	int i, div;

	switch (pll) {
	case CPU_PLL1:
		if (target < PLL1_FREQ_MIN || target > PLL1_FREQ_MAX) {
			printf("PLL1 frequency should be"
			"within [%d - %d] MHz\n", PLL1_FREQ_MIN / SZ_DEC_1M,
				PLL1_FREQ_MAX / SZ_DEC_1M);
			return -1;
		}
		for (i = 54, div = i; i < 109; i++) {
			if ((ref * (i >> 1)) > target)
				break;
			div = i;
		}
		break;
	case BUS_PLL2:
		if (target < PLL2_FREQ_MIN || target > PLL2_FREQ_MAX) {
			printf("PLL2 frequency should be"
			"within [%d - %d] MHz\n", PLL2_FREQ_MIN / SZ_DEC_1M,
				PLL2_FREQ_MAX / SZ_DEC_1M);
			return -1;
		}
		for (i = 0, div = i; i < 2; i++) {
			if (ref * (20 + (i << 1)) > target)
				break;
			div = i;
		}
		break;
	default:
		printf("Changing this PLL not supported\n");
		return -1;
		break;
	}

	return div;
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
	case NFC_CLK:
		printf("NFC Clock: %dHz\n",
			 mxc_get_clock(MXC_NFC_CLK));
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



static int config_pll_clk(enum pll_clocks pll, u32 divider)
{
	u32 ccsr = readl(CCM_BASE_ADDR + CLKCTL_CCSR);

	switch (pll) {
	case CPU_PLL1:
		/* Switch ARM to PLL2 clock */
		writel(ccsr | 0x4, CCM_BASE_ADDR + CLKCTL_CCSR);

		REG_CLR(ANATOP_BASE_ADDR, HW_ANADIG_PLL_SYS,
			BM_ANADIG_PLL_SYS_DIV_SELECT);
		REG_SET(ANATOP_BASE_ADDR, HW_ANADIG_PLL_SYS,
			BF_ANADIG_PLL_SYS_DIV_SELECT(divider));
		/* Enable CPU PLL1 */
		REG_SET(ANATOP_BASE_ADDR, HW_ANADIG_PLL_SYS,
			BM_ANADIG_PLL_SYS_ENABLE);
		/* Wait for PLL lock */
		while (REG_RD(ANATOP_BASE_ADDR, HW_ANADIG_PLL_SYS) &
			BM_ANADIG_PLL_SYS_LOCK)
			udelay(10);
		/* Clear bypass bit */
		REG_CLR(ANATOP_BASE_ADDR, HW_ANADIG_PLL_SYS,
			BM_ANADIG_PLL_SYS_BYPASS);

		/* Switch back */
		writel(ccsr & ~0x4, CCM_BASE_ADDR + CLKCTL_CCSR);
		break;
	case BUS_PLL2:
		/* Switch to pll2 bypass clock */
		writel(ccsr | 0x2, CCM_BASE_ADDR + CLKCTL_CCSR);

		REG_CLR(ANATOP_BASE_ADDR, HW_ANADIG_PLL_528,
			BM_ANADIG_PLL_528_DIV_SELECT);
		REG_SET(ANATOP_BASE_ADDR, HW_ANADIG_PLL_528,
			divider);
		/* Enable BUS PLL2 */
		REG_SET(ANATOP_BASE_ADDR, HW_ANADIG_PLL_528,
			BM_ANADIG_PLL_528_ENABLE);
		/* Wait for PLL lock */
		while (REG_RD(ANATOP_BASE_ADDR, HW_ANADIG_PLL_528) &
			BM_ANADIG_PLL_528_LOCK)
			udelay(10);
		/* Clear bypass bit */
		REG_CLR(ANATOP_BASE_ADDR, HW_ANADIG_PLL_528,
			BM_ANADIG_PLL_528_BYPASS);

		/* Switch back */
		writel(ccsr & ~0x2, CCM_BASE_ADDR + CLKCTL_CCSR);
		break;
	default:
		return -1;
	}

	return 0;
}

static int config_core_clk(u32 ref, u32 freq)
{
	int div = calc_pll_divider(CPU_PLL1, ref, freq);
	if (div < 0) {
		printf("Can't find pll parameters\n");
		return div;
	}

	return config_pll_clk(CPU_PLL1, div);
}

static int config_nfc_clk(u32 nfc_clk)
{
	/* TBD */
	return -1;
}
static int config_periph_clk(u32 ref, u32 freq)
{
	u32 cbcdr = readl(CCM_BASE_ADDR + CLKCTL_CBCDR);
	u32 cbcmr = readl(CCM_BASE_ADDR + CLKCTL_CBCMR);
	u32 pll2_freq = __decode_pll(BUS_PLL2, CONFIG_MX6_HCLK_FREQ);
	u32 pll3_freq = __decode_pll(USBOTG_PLL3, CONFIG_MX6_HCLK_FREQ);

	if (freq >=  pll2_freq) {
		/* PLL2 */
		writel(cbcmr & ~MXC_CCM_CBCMR_PRE_PERIPH_CLK_SEL_MASK,
			CCM_BASE_ADDR + CLKCTL_CBCMR);
		writel(cbcdr & ~MXC_CCM_CBCDR_PERIPH_CLK_SEL,
			CCM_BASE_ADDR + CLKCTL_CBCDR);
	} else if (freq < pll2_freq && freq >= pll3_freq) {
		/* PLL3 */
		writel(cbcmr & ~MXC_CCM_CBCMR_PERIPH_CLK2_SEL_MASK,
			CCM_BASE_ADDR + CLKCTL_CBCMR);
		writel(cbcdr | MXC_CCM_CBCDR_PERIPH_CLK_SEL,
			CCM_BASE_ADDR + CLKCTL_CBCDR);
	} else if (freq < pll3_freq && freq >= PLL2_PFD2_FREQ) {
		/* 400M PLL2 PFD */
		cbcmr = (cbcmr & ~MXC_CCM_CBCMR_PRE_PERIPH_CLK_SEL_MASK) |
			(1 << MXC_CCM_CBCMR_PRE_PERIPH_CLK_SEL_OFFSET);
		writel(cbcmr, CCM_BASE_ADDR + CLKCTL_CBCMR);
		writel(cbcdr & ~MXC_CCM_CBCDR_PERIPH_CLK_SEL,
			CCM_BASE_ADDR + CLKCTL_CBCDR);
	} else if (freq < PLL2_PFD2_FREQ && freq >= PLL2_PFD0_FREQ) {
		/* 352M PLL2 PFD */
		cbcmr = (cbcmr & ~MXC_CCM_CBCMR_PRE_PERIPH_CLK_SEL_MASK) |
			(2 << MXC_CCM_CBCMR_PRE_PERIPH_CLK_SEL_OFFSET);
		writel(cbcmr, CCM_BASE_ADDR + CLKCTL_CBCMR);
		writel(cbcdr & ~MXC_CCM_CBCDR_PERIPH_CLK_SEL,
			CCM_BASE_ADDR + CLKCTL_CBCDR);
	} else if (freq == PLL2_PFD2_DIV_FREQ) {
		/* 200M PLL2 PFD */
		cbcmr = (cbcmr & ~MXC_CCM_CBCMR_PRE_PERIPH_CLK_SEL_MASK) |
			(3 << MXC_CCM_CBCMR_PRE_PERIPH_CLK_SEL_OFFSET);
		writel(cbcmr, CCM_BASE_ADDR + CLKCTL_CBCMR);
		writel(cbcdr & ~MXC_CCM_CBCDR_PERIPH_CLK_SEL,
			CCM_BASE_ADDR + CLKCTL_CBCDR);
	} else {
		printf("Frequency requested not within range [%d-%d] MHz\n",
			PLL2_PFD2_DIV_FREQ / SZ_DEC_1M, pll2_freq / SZ_DEC_1M);
		return -1;
	}
	puts("\n");

	return 0;
}

static int config_ddr_clk(u32 ddr_clk)
{
	u32 clk_src = __get_periph_clk();
	u32 i, podf;
	u32 cbcdr = readl(CCM_BASE_ADDR + CLKCTL_CBCDR);

	if (ddr_clk > MAX_DDR_CLK) {
		printf("DDR clock should be less than"
			"%d MHz, assuming max value\n",
			(MAX_DDR_CLK / SZ_DEC_1M));
		ddr_clk = MAX_DDR_CLK;
	}

	for (i = 1; i < 9; i++)
		if ((clk_src / i) <= ddr_clk)
			break;

	podf = i - 1;

	cbcdr &= ~MXC_CCM_CBCDR_MMDC_CH0_PODF_MASK;
	cbcdr |= podf << MXC_CCM_CBCDR_MMDC_CH0_PODF_OFFSET;
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
 * core clock (arm_podf=0) unless the core clock is less than PLL_FREQ_MIN.
 *
 * @param ref       pll input reference clock (24MHz)
 * @param freq		targeted freq in Hz
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
	case NFC_CLK:
		if (config_nfc_clk(freq))
			return -1;
		break;
	default:
		printf("Unsupported or invalid clock type! :(\n");
		return -1;
	}

	return 0;
}
#endif

#if defined(CONFIG_DISPLAY_CPUINFO)
int print_cpuinfo(void)
{
	printf("CPU:   Freescale i.MX 6 family %d.%dV at %d MHz\n",
	       (get_board_rev() & 0xFF) >> 4,
	       (get_board_rev() & 0xF),
		__get_mcu_main_clk() / SZ_DEC_1M);
	mxc_dump_clocks();
	return 0;
}
#endif

#if defined(CONFIG_MXC_FEC)
extern int mxc_fec_initialize(bd_t *bis);
extern void mxc_fec_set_mac_from_env(char *mac_addr);
void enet_board_init(void);
#endif

int cpu_eth_init(bd_t *bis)
{
	int rc = -ENODEV;
#if defined(CONFIG_MXC_FEC)
	rc = mxc_fec_initialize(bis);

	/* Board level init */
	enet_board_init();

#endif
	return rc;
}

#if defined(CONFIG_ARCH_CPU_INIT)
int arch_cpu_init(void)
{
	icache_enable();
	dcache_enable();

#ifndef CONFIG_L2_OFF
	l2_cache_enable();
#endif
	return 0;
}
#endif

void ipu_clk_enable(void)
{
}

void ipu_clk_disable(void)
{
}
