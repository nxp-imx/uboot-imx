/*
 * Copyright (C) 2010-2013 Freescale Semiconductor, Inc.
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
#ifdef CONFIG_GET_FEC_MAC_ADDR_FROM_IIM
#include <asm/arch/regs-ocotp.h>
#endif

#ifdef CONFIG_IMX_UDC
#include <usb/imx_udc.h>
#endif

#ifdef CONFIG_ANDROID_RECOVERY
#include <recovery.h>
#endif

#include <usb/regs-usbphy-mx6.h>

#if defined(CONFIG_SECURE_BOOT)
#include <asm/arch/mx6_secure.h>
#endif

#ifdef CONFIG_SERIAL_TAG
#include <imx_otp.h>
#endif

enum pll_clocks {
	CPU_PLL1,	/* System PLL */
	BUS_PLL2,	/* System Bus PLL*/
	USBOTG_PLL3,    /* OTG USB PLL */
	AUD_PLL4,	/* Audio PLL */
	VID_PLL5,	/* Video PLL */
#ifndef CONFIG_MX6SL
	MLB_PLL6,	/* MLB PLL */
	USBHOST_PLL7,   /* Host USB PLL */
#endif
	ENET_PLL8,      /* ENET PLL */
};

#define SZ_DEC_1M       1000000

/* Out-of-reset PFDs and clock source definitions */
#define PLL2_PFD0_FREQ	352000000
#define PLL2_PFD1_FREQ	594000000
#define PLL2_PFD2_FREQ	396000000
#define PLL2_PFD2_DIV_FREQ	198000000
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

#define GPC_PGC_GPU_PGCR_OFFSET 0x260
#define GPC_CNTR_OFFSET         0x0

#define OCOTP_THERMAL_OFFSET	0x4E0
#define TEMPERATURE_MIN			-40
#define TEMPERATURE_HOT			80
#define TEMPERATURE_MAX			125
#define REG_VALUE_TO_CEL(ratio, raw) ((raw_n40c - raw) * 100 / ratio - 40)
#define FACTOR1	15976
#define FACTOR2	4297157

static int cpu_tmp;
static unsigned int fuse;

#define SNVS_LPGPR_OFFSET	0x68
#define IOMUXC_GPR1_OFFSET	0x4

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
#ifndef CONFIG_MX6SL
	case MLB_PLL6:
	case USBHOST_PLL7:
#endif
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
	u32 podf, reg;
	u32 clk_root = __get_ipg_clk();

	reg = __REG(MXC_CCM_CSCMR1);
#ifdef CONFIG_MX6SL
	if (reg & 0x40) /* PERCLK from 24M OSC */
		clk_root = CONFIG_MX6_HCLK_FREQ;
#endif
	podf = reg & MXC_CCM_CSCMR1_PERCLK_PODF_MASK;
	return clk_root / (podf + 1);
}

static u32 __get_uart_clk(void)
{
	u32 freq = PLL3_80M, reg, podf;

	reg = __REG(MXC_CCM_CSCDR1);
#ifdef CONFIG_MX6SL
	if (reg & 0x40) /* UART clock from 24M OSC */
		freq = CONFIG_MX6_HCLK_FREQ;
#endif
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
			MXC_CCM_CSCMR1_ACLK_EMI_SLOW_PODF_OFFSET;

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
	u32 cs2cdr = __REG(MXC_CCM_CS2CDR);
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

	return  clkroot / (pred + 1) / (podf + 1);
}

#ifdef CONFIG_MX6SL
static u32 __get_ddr_clk(void)
{
	u32 cbcmr = __REG(MXC_CCM_CBCMR);
	u32 cbcdr = __REG(MXC_CCM_CBCDR);
	u32 freq, podf;

	podf = (cbcdr & MXC_CCM_CBCDR_MMDC_CH1_PODF_MASK) \
			>> MXC_CCM_CBCDR_MMDC_CH1_PODF_OFFSET;

	switch ((cbcmr & MXC_CCM_CBCMR_PRE_PERIPH2_CLK_SEL_MASK) >>
		MXC_CCM_CBCMR_PRE_PERIPH2_CLK_SEL_OFFSET) {
	case 0:
		freq = __decode_pll(BUS_PLL2, CONFIG_MX6_HCLK_FREQ);
		break;
	case 1:
		freq = PLL2_PFD2_FREQ;
		break;
	case 2:
		freq = PLL2_PFD0_FREQ;
		break;
	case 3:
		freq = PLL2_PFD2_DIV_FREQ;
	}

	return freq / (podf + 1);

}
#else
static u32 __get_ddr_clk(void)
{
	u32 cbcdr = __REG(MXC_CCM_CBCDR);
	u32 podf = (cbcdr & MXC_CCM_CBCDR_MMDC_CH0_PODF_MASK) >>
		MXC_CCM_CBCDR_MMDC_CH0_PODF_OFFSET;

	return __get_periph_clk() / (podf + 1);
}
#endif

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
	case MXC_GPMI_CLK:
	case MXC_BCH_CLK:
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
#ifndef CONFIG_MX6SL
	printf("nfc clock     : %dHz\n", mxc_get_clock(MXC_NFC_CLK));
#endif
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

#define calc_pred_n_podf(target_clk, src_clk, p_pred, p_podf, pred_limit, podf_limit) {	\
		u32 div = calc_div(target_clk, src_clk,	\
				pred_limit * podf_limit);	\
		u32 tmp = 0, tmp_i = 0, tmp_j = 0;	\
		if ((div + 1) > (pred_limit * podf_limit))	{\
			tmp_i = pred_limit;	\
			tmp_j = podf_limit;	\
		}	\
		for (tmp_i = 1; tmp_i <= podf_limit; ++tmp_i) {	\
			for (tmp_j = 1; tmp_j <= pred_limit; ++tmp_j) {	\
				if ((div + 1) == (tmp_i * tmp_j)) {	\
					tmp = 1;	\
					break;	\
				}	\
			}	\
			if (1 == tmp)	\
				break;	\
		}	\
		*p_pred = tmp_j - 1;	\
		*p_podf = tmp_i - 1;	\
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
	u32 clkroot;
	u32 cs2cdr = __REG(MXC_CCM_CS2CDR);
	u32 podf = 0, pred = 0;

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
			clkroot = __decode_pll(USBOTG_PLL3,
					CONFIG_MX6_HCLK_FREQ);
			break;
		case 3:
			clkroot = PLL2_PFD2_FREQ;
			break;
	}

	calc_pred_n_podf(nfc_clk, clkroot, &pred, &podf, 8, 64);

	cs2cdr &= ~(MXC_CCM_CS2CDR_ENFC_CLK_PRED_MASK |
			MXC_CCM_CS2CDR_ENFC_CLK_PODF_MASK);
	cs2cdr |= (pred << MXC_CCM_CS2CDR_ENFC_CLK_PRED_OFFSET);
	cs2cdr |= (podf << MXC_CCM_CS2CDR_ENFC_CLK_PODF_OFFSET);

	writel(cs2cdr, MXC_CCM_CS2CDR);

	return  0;
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

	cbcdr &= ~(MXC_CCM_CBCDR_MMDC_CH0_PODF_MASK |
			MXC_CCM_CBCDR_MMDC_CH1_PODF_MASK);
	cbcdr |= (podf << MXC_CCM_CBCDR_MMDC_CH0_PODF_OFFSET) |
			(podf << MXC_CCM_CBCDR_MMDC_CH1_PODF_OFFSET);
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

static inline int read_cpu_temperature(void)
{
	unsigned int reg, tmp, i;
	unsigned int raw_25c, raw_hot, hot_temp, raw_n40c, ratio;
	unsigned int ccm_ccgr2;
	int temperature;

	ccm_ccgr2 = readl(MXC_CCM_CCGR2);
	writel(readl(MXC_CCM_CCGR2) | (0x3 << MXC_CCM_CCGR2_CG6_OFFSET),
			MXC_CCM_CCGR2);
	fuse = readl(OCOTP_BASE_ADDR + OCOTP_THERMAL_OFFSET);
	writel(ccm_ccgr2, MXC_CCM_CCGR2);
	if (fuse == 0 || fuse == 0xffffffff || (fuse & 0xfff00000) == 0)
		return TEMPERATURE_MIN;

	/* Fuse data layout:
	 * [31:20] sensor value @ 25C
	 * [19:8] sensor value of hot
	 * [7:0] hot temperature value */
	raw_25c = fuse >> 20;
	raw_hot = (fuse & 0xfff00) >> 8;
	hot_temp = fuse & 0xff;

	/*
	 * Only when high temperature calibration
	 * data not used, we use universal equation to get thermal
	 * sensor's ratio.
	 */
#ifndef USE_CALIBRATION
	/*
	 * The universal equation for thermal sensor
	 * is slope = 0.4297157 - (0.0015976 * 25C fuse),
	 * here we convert them to integer to make them
	 * easy for counting, FACTOR1 is 15976,
	 * FACTOR2 is 4297157. Our ratio = -100 * slope.
	 */
	ratio = ((FACTOR1 * raw_25c - FACTOR2) + 50000) / 100000;
#else
	ratio = ((raw_25c - raw_hot) * 100) / (hot_temp - 25);
#endif

	printf("Thermal sensor with ratio = %d\n", ratio);
	raw_n40c = raw_25c + (13 * ratio) / 20;

	/* now we only using single measure, every time we measure
	the temperature, we will power on/down the anadig module*/
	writel(BM_ANADIG_TEMPSENSE0_POWER_DOWN,
			ANATOP_BASE_ADDR + HW_ANADIG_TEMPSENSE0_CLR);
	writel(BM_ANADIG_ANA_MISC0_REFTOP_SELBIASOFF,
			ANATOP_BASE_ADDR + HW_ANADIG_ANA_MISC0_SET);

	/* write measure freq */
	reg = readl(ANATOP_BASE_ADDR + HW_ANADIG_TEMPSENSE1);
	reg &= ~BM_ANADIG_TEMPSENSE1_MEASURE_FREQ;
	reg |= 327;
	writel(reg, ANATOP_BASE_ADDR + HW_ANADIG_TEMPSENSE1);

	writel(BM_ANADIG_TEMPSENSE0_MEASURE_TEMP,
		ANATOP_BASE_ADDR + HW_ANADIG_TEMPSENSE0_CLR);
	writel(BM_ANADIG_TEMPSENSE0_FINISHED,
		ANATOP_BASE_ADDR + HW_ANADIG_TEMPSENSE0_CLR);
	writel(BM_ANADIG_TEMPSENSE0_MEASURE_TEMP,
		ANATOP_BASE_ADDR + HW_ANADIG_TEMPSENSE0_SET);

	tmp = 0;
	/* read five times of temperature values to get average*/
	for (i = 0; i < 5; i++) {
		while ((readl(ANATOP_BASE_ADDR + HW_ANADIG_TEMPSENSE0)
			& BM_ANADIG_TEMPSENSE0_FINISHED) == 0)
			udelay(10000);
		reg = readl(ANATOP_BASE_ADDR + HW_ANADIG_TEMPSENSE0);
		tmp += (reg & BM_ANADIG_TEMPSENSE0_TEMP_VALUE)
				>> BP_ANADIG_TEMPSENSE0_TEMP_VALUE;
		writel(BM_ANADIG_TEMPSENSE0_FINISHED,
			ANATOP_BASE_ADDR + HW_ANADIG_TEMPSENSE0_CLR);
	}

	tmp = tmp / 5;
	if (tmp <= raw_n40c)
		temperature = REG_VALUE_TO_CEL(ratio, tmp);
	else
		temperature = TEMPERATURE_MIN;
	/* power down anatop thermal sensor */
	writel(BM_ANADIG_TEMPSENSE0_POWER_DOWN,
			ANATOP_BASE_ADDR + HW_ANADIG_TEMPSENSE0_SET);
	writel(BM_ANADIG_ANA_MISC0_REFTOP_SELBIASOFF,
			ANATOP_BASE_ADDR + HW_ANADIG_ANA_MISC0_CLR);

	return temperature;

}

static void check_cpu_temperature(void)
{
	cpu_tmp = read_cpu_temperature();
	while (cpu_tmp > TEMPERATURE_MIN && cpu_tmp < TEMPERATURE_MAX) {
		if (cpu_tmp >= TEMPERATURE_HOT) {
			printf("CPU is %d C, too hot to boot, waiting...\n",
				cpu_tmp);
			udelay(5000000);
			cpu_tmp = read_cpu_temperature();
		} else
			break;
	}
	if (cpu_tmp > TEMPERATURE_MIN && cpu_tmp < TEMPERATURE_MAX)
		printf("Temperature:   %d C, calibration data 0x%x\n",
			cpu_tmp, fuse);
	else
		printf("Temperature:   can't get valid data!\n");
}

#if defined(CONFIG_DISPLAY_CPUINFO)
int print_cpuinfo(void)
{
	printf("CPU: Freescale i.MX6 family TO%d.%d at %d MHz\n",
	       (get_board_rev() & 0xFF) >> 4,
	       (get_board_rev() & 0xF),
		__get_mcu_main_clk() / SZ_DEC_1M);

	check_cpu_temperature();

	mxc_dump_clocks();
	return 0;
}
#endif

#if defined(CONFIG_MXC_FEC)
extern int mxc_fec_initialize(bd_t *bis);
extern void mxc_fec_set_mac_from_env(char *mac_addr);
void enet_board_init(void);
#ifdef CONFIG_GET_FEC_MAC_ADDR_FROM_IIM
int fec_get_mac_addr(unsigned char *mac)
{
	unsigned int value;

	value = readl(OCOTP_BASE_ADDR + HW_OCOTP_MACn(0));
	mac[5] = value & 0xff;
	mac[4] = (value >> 8) & 0xff;
	mac[3] = (value >> 16) & 0xff;
	mac[2] = (value >> 24) & 0xff;
	value = readl(OCOTP_BASE_ADDR + HW_OCOTP_MACn(1));
	mac[1] = value & 0xff;
	mac[0] = (value >> 8) & 0xff;

	return 0;
}
#endif

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
	int val;

	/* Clear MMDC channel mask */
	writel(0, CCM_BASE_ADDR + 0x4);

	/* Due to hardware limitation, on MX6Q we need to gate/ungate all PFDs
	 * to make sure PFD is working right, otherwise, PFDs may
	 * not output clock after reset, MX6DL and MX6SL have added 396M pfd
	 * workaround in ROM code, as bus clock need it
	 */
	writel(BM_ANADIG_PFD_480_PFD3_CLKGATE |
		BM_ANADIG_PFD_480_PFD2_CLKGATE |
		BM_ANADIG_PFD_480_PFD1_CLKGATE |
		BM_ANADIG_PFD_480_PFD0_CLKGATE,
		ANATOP_BASE_ADDR + HW_ANADIG_PFD_480_SET);
	writel(BM_ANADIG_PFD_528_PFD3_CLKGATE |
#ifdef CONFIG_MX6Q
		BM_ANADIG_PFD_528_PFD2_CLKGATE |
#endif
		BM_ANADIG_PFD_528_PFD1_CLKGATE |
		BM_ANADIG_PFD_528_PFD0_CLKGATE,
		ANATOP_BASE_ADDR + HW_ANADIG_PFD_528_SET);

	writel(BM_ANADIG_PFD_480_PFD3_CLKGATE |
		BM_ANADIG_PFD_480_PFD2_CLKGATE |
		BM_ANADIG_PFD_480_PFD1_CLKGATE |
		BM_ANADIG_PFD_480_PFD0_CLKGATE,
		ANATOP_BASE_ADDR + HW_ANADIG_PFD_480_CLR);
	writel(BM_ANADIG_PFD_528_PFD3_CLKGATE |
#ifdef CONFIG_MX6Q
		BM_ANADIG_PFD_528_PFD2_CLKGATE |
#endif
		BM_ANADIG_PFD_528_PFD1_CLKGATE |
		BM_ANADIG_PFD_528_PFD0_CLKGATE,
		ANATOP_BASE_ADDR + HW_ANADIG_PFD_528_CLR);

	icache_enable();
	dcache_enable();

#ifndef CONFIG_L2_OFF
	l2_cache_enable();
#endif
	/* Need to power down xPU in GPC before turn off PU LDO */
	val = readl(GPC_BASE_ADDR + GPC_PGC_GPU_PGCR_OFFSET);
	writel(val | 0x1, GPC_BASE_ADDR + GPC_PGC_GPU_PGCR_OFFSET);

	val = readl(GPC_BASE_ADDR + GPC_CNTR_OFFSET);
	writel(val | 0x1, GPC_BASE_ADDR + GPC_CNTR_OFFSET);
	while (readl(GPC_BASE_ADDR + GPC_CNTR_OFFSET) & 0x1)
		;

	/* Increase the VDDSOC to 1.2V and disable VDDPU */
	val = REG_RD(ANATOP_BASE_ADDR, HW_ANADIG_REG_CORE);
	val &= ~BM_ANADIG_REG_CORE_REG2_TRG;
	val &= ~BM_ANADIG_REG_CORE_REG1_TRG;
	val |= BF_ANADIG_REG_CORE_REG2_TRG(0x14);
	REG_WR(ANATOP_BASE_ADDR, HW_ANADIG_REG_CORE, val);

	/* Need to power down PCIe */
	val = REG_RD(IOMUXC_BASE_ADDR, IOMUXC_GPR1_OFFSET);
	val |= (0x1 << 18);
	REG_WR(IOMUXC_BASE_ADDR, IOMUXC_GPR1_OFFSET, val);

	/*clear PowerDown Enable bit of WDOGx_WMCR*/
	writew(0, WDOG1_BASE_ADDR + 0x08);
	writew(0, WDOG2_BASE_ADDR + 0x08);
	return 0;
}
#endif

#ifdef CONFIG_VIDEO_MX5
void ipu_clk_enable(void)
{
}

void ipu_clk_disable(void)
{
}
#endif

#ifdef CONFIG_CMD_IMXOTP
int otp_clk_enable(void)
{
	u32 reg = 0;

	reg = readl(CCM_BASE_ADDR + CLKCTL_CCGR2);
	if (!(reg & 0x3000))
		reg |= 0x3000;
    writel(reg, CCM_BASE_ADDR + CLKCTL_CCGR2);
	return 0;
}

int otp_clk_disable(void)
{
	u32 reg = 0;

	reg = readl(CCM_BASE_ADDR + CLKCTL_CCGR2);
	if ((reg & 0x3000) == 0x3000)
		reg &= ~(0x3000);
    writel(reg, CCM_BASE_ADDR + CLKCTL_CCGR2);
	return 0;
}
#endif

#ifdef CONFIG_IMX_UDC
void enable_usboh3_clk(unsigned char enable)
{
	unsigned int reg;
	reg = readl(MXC_CCM_CCGR6);
	if (enable)
		reg |= 1 << MXC_CCM_CCGR6_CG0_OFFSET;
	else
		reg &= ~(1 << MXC_CCM_CCGR6_CG0_OFFSET);
	writel(reg, MXC_CCM_CCGR6);
}

void enable_usb_phy1_clk(unsigned char enable)
{
	if (enable) {
		writel(BM_USBPHY_CTRL_CLKGATE, USB_PHY0_BASE_ADDR + HW_USBPHY_CTRL_CLR);
	} else {
		writel(BM_USBPHY_CTRL_CLKGATE, USB_PHY0_BASE_ADDR + HW_USBPHY_CTRL_SET);
	}
}

void reset_usb_phy1(void)
{
	/* Reset USBPHY module */
	u32 temp;
	temp = readl(USB_PHY0_BASE_ADDR + HW_USBPHY_CTRL);
	temp |= BM_USBPHY_CTRL_SFTRST;
	writel(temp, USB_PHY0_BASE_ADDR + HW_USBPHY_CTRL);
	udelay(10);

	/* Remove CLKGATE and SFTRST */
	temp = readl(USB_PHY0_BASE_ADDR + HW_USBPHY_CTRL);
	temp &= ~(BM_USBPHY_CTRL_CLKGATE | BM_USBPHY_CTRL_SFTRST);
	writel(temp, USB_PHY0_BASE_ADDR + HW_USBPHY_CTRL);
	udelay(10);

	/* Power up the PHY */
	writel(0, USB_PHY0_BASE_ADDR + HW_USBPHY_PWD);
}

void set_usb_phy1_clk(void)
{
	/* make sure pll3 is enable here */
	REG_SET(ANATOP_BASE_ADDR, HW_ANADIG_USB1_CHRG_DETECT,
		BM_ANADIG_USB1_CHRG_DETECT_EN_B | BM_ANADIG_USB1_CHRG_DETECT_CHK_CHRG_B);
	REG_SET(ANATOP_BASE_ADDR, HW_ANADIG_USB1_PLL_480_CTRL,
		BM_ANADIG_USB1_PLL_480_CTRL_EN_USB_CLKS);
}

void set_usboh3_clk(void)
{
	udc_pins_setting();
}
#endif

#ifdef CONFIG_ANDROID_RECOVERY
#define ANDROID_RECOVERY_BOOT  (1 << 7)
/* check if the recovery bit is set by kernel, it can be set by kernel
 * issue a command '# reboot recovery' */
int check_and_clean_recovery_flag(void)
{
	int flag_set = 0;
	u32 reg;
	reg = readl(SNVS_BASE_ADDR + SNVS_LPGPR);

	flag_set = !!(reg & ANDROID_RECOVERY_BOOT);

	/* clean it in case looping infinite here.... */
	if (flag_set) {
		reg &= ~ANDROID_RECOVERY_BOOT;
		writel(reg, SNVS_BASE_ADDR + SNVS_LPGPR);
	}

	return flag_set;
}
#endif

#ifdef CONFIG_FASTBOOT
#define ANDROID_FASTBOOT_BOOT  (1 << 8)
/* check if the recovery bit is set by kernel, it can be set by kernel
 * issue a command '# reboot fastboot' */
int fastboot_check_and_clean_flag(void)
{
	int flag_set = 0;
	u32 reg;

	reg = readl(SNVS_BASE_ADDR + SNVS_LPGPR);

	flag_set = !!(reg & ANDROID_FASTBOOT_BOOT);

	/* clean it in case looping infinite here.... */
	if (flag_set) {
		reg &= ~ANDROID_FASTBOOT_BOOT;
		writel(reg, SNVS_BASE_ADDR + SNVS_LPGPR);
	}

	return flag_set;
}
#endif

#ifdef CONFIG_CMD_IMX_DOWNLOAD_MODE
#define PERSIST_WATCHDOG_RESET_BOOT		(0x10000000)
/*BOOT_CFG1[7..4] = 0x3 Boot from Serial ROM (I2C/SPI)*/
#define BOOT_MODE_SERIAL_ROM			(0x00000030)

extern int do_reset(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);

/* this function should call before enter linux, otherwise, you
 * watchdog reset will enter mfg download mode again, clear this bit
 * to prevent this behavior */
void clear_mfgmode_mem(void)
{
	u32 reg;
	reg = readl(SRC_BASE_ADDR + SRC_GPR9);

	reg &= ~BOOT_MODE_SERIAL_ROM;
	writel(reg, SRC_BASE_ADDR + SRC_GPR9);

	reg = readl(SRC_BASE_ADDR + SRC_GPR10);
	reg &= ~PERSIST_WATCHDOG_RESET_BOOT;
	reg = writel(reg, SRC_BASE_ADDR + SRC_GPR10);
}

int do_switch_mfgmode(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	u32 reg;

	/*
	 * During reset, if GPR10[28] is 1, ROM will copy GPR9[25:0]
	 * to SBMR1, which will determine what is the boot device.
	 * Here SERIAL_ROM mode is selected
	 */
	reg = readl(SRC_BASE_ADDR + SRC_GPR9);
	reg |= BOOT_MODE_SERIAL_ROM;
	writel(reg, SRC_BASE_ADDR + SRC_GPR9);

	reg = readl(SRC_BASE_ADDR + SRC_GPR10);
	reg |= PERSIST_WATCHDOG_RESET_BOOT;
	writel(reg, SRC_BASE_ADDR + SRC_GPR10);

	/*
	 * this watchdog reset will let chip enter mfgtool download
	 * mode.
	 */
	do_reset(NULL, 0, 0, NULL);

	return 0;
}

U_BOOT_CMD(
	download_mode, 1, 1, do_switch_mfgmode,
	"download_mode - enter i.MX serial/usb download mode",
	"");
#endif

#ifdef CONFIG_SECURE_BOOT
/* -------- start of HAB API updates ------------*/
#define hab_rvt_report_event_p						\
(									\
	(mx6_chip_is_dq() && (mx6_chip_rev() >= CHIP_REV_1_5)) ?	\
	((hab_rvt_report_event_t *)HAB_RVT_REPORT_EVENT_NEW) :		\
	(mx6_chip_is_dl() && (mx6_chip_rev() >= CHIP_REV_1_2)) ?	\
	((hab_rvt_report_event_t *)HAB_RVT_REPORT_EVENT_NEW) :		\
	((hab_rvt_report_event_t *)HAB_RVT_REPORT_EVENT)		\
)

#define hab_rvt_report_status_p						\
(									\
	(mx6_chip_is_dq() && (mx6_chip_rev() >= CHIP_REV_1_5)) ?	\
	((hab_rvt_report_status_t *)HAB_RVT_REPORT_STATUS_NEW) :	\
	(mx6_chip_is_dl() && (mx6_chip_rev() >= CHIP_REV_1_2)) ?	\
	((hab_rvt_report_status_t *)HAB_RVT_REPORT_STATUS_NEW) :	\
	((hab_rvt_report_status_t *)HAB_RVT_REPORT_STATUS)		\
)

#define hab_rvt_authenticate_image_p					\
(									\
	(mx6_chip_is_dq() && (mx6_chip_rev() >= CHIP_REV_1_5)) ?	\
	((hab_rvt_authenticate_image_t *)HAB_RVT_AUTHENTICATE_IMAGE_NEW) :\
	(mx6_chip_is_dl() && (mx6_chip_rev() >= CHIP_REV_1_2)) ?	\
	((hab_rvt_authenticate_image_t *)HAB_RVT_AUTHENTICATE_IMAGE_NEW) :\
	((hab_rvt_authenticate_image_t *)HAB_RVT_AUTHENTICATE_IMAGE)	\
)

#define hab_rvt_entry_p							\
(									\
	(mx6_chip_is_dq() && (mx6_chip_rev() >= CHIP_REV_1_5)) ?	\
	((hab_rvt_entry_t *)HAB_RVT_ENTRY_NEW) :			\
	(mx6_chip_is_dl() && (mx6_chip_rev() >= CHIP_REV_1_2)) ?	\
	((hab_rvt_entry_t *)HAB_RVT_ENTRY_NEW) :			\
	((hab_rvt_entry_t *)HAB_RVT_ENTRY)				\
)

#define hab_rvt_exit_p							\
(									\
	(mx6_chip_is_dq() && (mx6_chip_rev() >= CHIP_REV_1_5)) ?	\
	((hab_rvt_exit_t *)HAB_RVT_EXIT_NEW) :				\
	(mx6_chip_is_dl() && (mx6_chip_rev() >= CHIP_REV_1_2)) ?	\
	((hab_rvt_exit_t *)HAB_RVT_EXIT_NEW) :				\
	((hab_rvt_exit_t *)HAB_RVT_EXIT)				\
)

#define OCOTP_CFG5_OFFSET	0x460
#define IVT_SIZE		0x20
#define ALIGN_SIZE		0x1000
#define CSF_PAD_SIZE		0x2000

/*
 * +------------+  0x0 (DDR_UIMAGE_START) -
 * |   Header   |                          |
 * +------------+  0x40                    |
 * |            |                          |
 * |            |                          |
 * |            |                          |
 * |            |                          |
 * | Image Data |                          |
 * .            |                          |
 * .            |                           > Stuff to be authenticated ----+
 * .            |                          |                                |
 * |            |                          |                                |
 * |            |                          |                                |
 * +------------+                          |                                |
 * |            |                          |                                |
 * | Fill Data  |                          |                                |
 * |            |                          |                                |
 * +------------+ Align to ALIGN_SIZE      |                                |
 * |    IVT     |                          |                                |
 * +------------+ + IVT_SIZE              -                                 |
 * |            |                                                           |
 * |  CSF DATA  | <---------------------------------------------------------+
 * |            |
 * +------------+
 * |            |
 * | Fill Data  |
 * |            |
 * +------------+ + CSF_PAD_SIZE
 */

int check_hab_enable(void)
{
	u32 reg = 0;
	int result = 0;

	reg = readl(IMX_OTP_BASE + OCOTP_CFG5_OFFSET);
	if ((reg & 0x2) == 0x2)
		result = 1;

	return result;
}

void display_event(uint8_t *event_data, size_t bytes)
{
	uint32_t i;
	if ((event_data) && (bytes > 0)) {
		for (i = 0; i < bytes; i++) {
			if (i == 0)
				printf("\t0x%02x", event_data[i]);
			else if ((i % 8) == 0)
				printf("\n\t0x%02x", event_data[i]);
			else
				printf(" 0x%02x", event_data[i]);
		}
	}
}

int get_hab_status(void)
{
	uint32_t index = 0; /* Loop index */
	uint8_t event_data[128]; /* Event data buffer */
	size_t bytes = sizeof(event_data); /* Event size in bytes */
	hab_config_t config = 0;
	hab_state_t state = 0;
	hab_rvt_report_event_t *hab_rvt_report_event;
	hab_rvt_report_status_t *hab_rvt_report_status;

	hab_rvt_report_event = hab_rvt_report_event_p;
	hab_rvt_report_status = hab_rvt_report_status_p;

	/* Check HAB status */
	if (hab_rvt_report_status(&config, &state) != HAB_SUCCESS) {
		printf("\nHAB Configuration: 0x%02x, HAB State: 0x%02x\n",
			config, state);

		/* Display HAB Error events */
		while (hab_rvt_report_event(HAB_FAILURE, index, event_data,
				&bytes) == HAB_SUCCESS) {
			printf("\n");
			printf("--------- HAB Event %d -----------------\n",
					index + 1);
			printf("event data:\n");
			display_event(event_data, bytes);
			printf("\n");
			bytes = sizeof(event_data);
			index++;
		}
	}
	/* Display message if no HAB events are found */
	else {
		printf("\nHAB Configuration: 0x%02x, HAB State: 0x%02x\n",
			config, state);
		printf("No HAB Events Found!\n\n");
	}
	return 0;
}

void hab_caam_clock_enable(void)
{
	u32 reg = 0;

	reg = readl(CCM_BASE_ADDR + CLKCTL_CCGR0); /* CCGR0 */
	reg |= 0x3F00; /*CG4 ~ CG6, enable CAAM clocks*/
	writel(reg, CCM_BASE_ADDR + CLKCTL_CCGR0);
}


void hab_caam_clock_disable(void)
{
	u32 reg = 0;

	reg = readl(CCM_BASE_ADDR + CLKCTL_CCGR0); /* CCGR0 */
	reg &= ~0x3F00; /*CG4 ~ CG6, disable CAAM clocks*/
	writel(reg, CCM_BASE_ADDR + CLKCTL_CCGR0);
}

#ifdef DEBUG_AUTHENTICATE_IMAGE
void dump_mem(uint32_t addr, int size)
{
	int i;

	for (i = 0; i < size; i += 4) {
		if (i != 0) {
			if (i % 16 == 0)
				printf("\n");
			else
				printf(" ");
		}

		printf("0x%08x", *(uint32_t *)addr);
		addr += 4;
	}

	printf("\n");

	return;
}
#endif

uint32_t authenticate_image(uint32_t ddr_start, uint32_t image_size)
{
	uint32_t load_addr = 0;
	size_t bytes;
	ptrdiff_t ivt_offset = 0;
	int result = 0;
	ulong start;
	hab_rvt_authenticate_image_t *hab_rvt_authenticate_image;
	hab_rvt_entry_t *hab_rvt_entry;
	hab_rvt_exit_t *hab_rvt_exit;

	hab_rvt_authenticate_image = hab_rvt_authenticate_image_p;
	hab_rvt_entry = hab_rvt_entry_p;
	hab_rvt_exit = hab_rvt_exit_p;

	printf("\nAuthenticate uImage from DDR location 0x%lx...\n", ddr_start);

	hab_caam_clock_enable();

	if (hab_rvt_entry() == HAB_SUCCESS) {

		/* If not already aligned, Align to ALIGN_SIZE */
		if (image_size % ALIGN_SIZE)
			ivt_offset = image_size - image_size %
					ALIGN_SIZE + ALIGN_SIZE;
		else
			ivt_offset = image_size;

		start = ddr_start;
		bytes = ivt_offset + IVT_SIZE + CSF_PAD_SIZE;

#ifdef DEBUG_AUTHENTICATE_IMAGE
		printf("\nivt_offset = 0x%x, ivt addr = 0x%x\n",
		       ivt_offset, ddr_start + ivt_offset);
		printf("Dumping IVT\n");
		dump_mem(ddr_start + ivt_offset, 0x20);

		printf("Dumping CSF Header\n");
		dump_mem(ddr_start + ivt_offset + 0x20, 0x40);

		get_hab_status();

		printf("\nCalling authenticate_image in ROM\n");
		printf("\tivt_offset = 0x%x\n\tstart = 0x%08x"
		       "\n\tbytes = 0x%x\n", ivt_offset, start, bytes);
#endif

		load_addr = (uint32_t)hab_rvt_authenticate_image(
					HAB_CID_UBOOT,
					ivt_offset, (void **)&start,
					(size_t *)&bytes, NULL);
		if (hab_rvt_exit() != HAB_SUCCESS) {
			printf("hab exit function fail\n");
			load_addr = 0;
		}
	} else
		printf("hab entry function fail\n");

	hab_caam_clock_disable();

	get_hab_status();


	if (load_addr != 0)
		result = 1;

	return result;
}
/* ----------- end of HAB API updates ------------*/
#endif


#ifdef CONFIG_ANDROID_RECOVERY
#ifndef CONFIG_ANDROID_RECOVERY_BOOTCMD_SD
#define CONFIG_ANDROID_RECOVERY_BOOTCMD_SD CONFIG_ANDROID_RECOVERY_BOOTCMD_MMC
#endif
struct reco_envs supported_reco_envs[BOOT_DEV_NUM] = {
	{
		.cmd = CONFIG_ANDROID_RECOVERY_BOOTCMD_MMC,
		.args = CONFIG_ANDROID_RECOVERY_BOOTARGS_MMC,
	},
	{
		.cmd = NULL,
		.args = NULL,
	},
	{
		.cmd = NULL,
		.args = NULL,
	},
	{
		.cmd = NULL,
		.args = NULL,
	},
	{
		.cmd = CONFIG_ANDROID_RECOVERY_BOOTCMD_MMC,
		.args = CONFIG_ANDROID_RECOVERY_BOOTARGS_MMC,
	},
	{
		.cmd = CONFIG_ANDROID_RECOVERY_BOOTCMD_MMC,
		.args = CONFIG_ANDROID_RECOVERY_BOOTARGS_MMC,
	},
	{
		.cmd = CONFIG_ANDROID_RECOVERY_BOOTCMD_SD,
		.args = CONFIG_ANDROID_RECOVERY_BOOTARGS_MMC,
	},
	{
		.cmd = CONFIG_ANDROID_RECOVERY_BOOTCMD_MMC,
		.args = CONFIG_ANDROID_RECOVERY_BOOTARGS_MMC,
	},
#ifdef CONFIG_CMD_NAND
	{
		.cmd = CONFIG_ANDROID_RECOVERY_BOOTCMD_NAND,
		.args = CONFIG_ANDROID_RECOVERY_BOOTARGS_NAND,
	},
#else
	{
		.cmd = NULL,
		.args = NULL,
	},
#endif
};
#endif

#ifdef CONFIG_SERIAL_TAG
void get_board_serial(struct tag_serialnr *serialnr)
{
	imx_otp_read_one_u32(CPU_UID_LOW_FUSE_INDEX, &serialnr->low);
	imx_otp_read_one_u32(CPU_UID_HIGH_FUSE_INDEX, &serialnr->high);
}
#endif

#ifdef CONFIG_DYNAMIC_MMC_DEVNO
int get_mmc_env_devno(void)
{
	uint soc_sbmr = readl(SRC_BASE_ADDR + 0x4);

	/* BOOT_CFG2[3] and BOOT_CFG2[4] */
	return (soc_sbmr & 0x00001800) >> 11;
}
#endif
