/*
 * Copyright (C) 2010-2014 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <div64.h>
#include <asm/io.h>
#include <asm/errno.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/crm_regs.h>
#include <asm/arch/clock.h>
#include <asm/arch/sys_proto.h>

enum pll_clocks {
	PLL_SYS,	/* System PLL */
	PLL_BUS,	/* System Bus PLL*/
	PLL_USBOTG,	/* OTG USB PLL */
	PLL_ENET,	/* ENET PLL */
};

struct mxc_ccm_reg *imx_ccm = (struct mxc_ccm_reg *)CCM_BASE_ADDR;

#ifdef CONFIG_MXC_OCOTP
void enable_ocotp_clk(unsigned char enable)
{
	u32 reg;

	reg = __raw_readl(&imx_ccm->CCGR2);
	if (enable)
		reg |= MXC_CCM_CCGR2_OCOTP_CTRL_MASK;
	else
		reg &= ~MXC_CCM_CCGR2_OCOTP_CTRL_MASK;
	__raw_writel(reg, &imx_ccm->CCGR2);
}
#endif

void enable_usboh3_clk(unsigned char enable)
{
	u32 reg;

	reg = __raw_readl(&imx_ccm->CCGR6);
	if (enable)
		reg |= MXC_CCM_CCGR6_USBOH3_MASK;
	else
		reg &= ~(MXC_CCM_CCGR6_USBOH3_MASK);
	__raw_writel(reg, &imx_ccm->CCGR6);

}

#ifdef CONFIG_SYS_I2C_MXC
/* i2c_num can be from 0 - 2 */
int enable_i2c_clk(unsigned char enable, unsigned i2c_num)
{
	u32 reg;
	u32 mask;

	if (i2c_num > 2)
		return -EINVAL;

	mask = MXC_CCM_CCGR_CG_MASK
		<< (MXC_CCM_CCGR2_I2C1_SERIAL_OFFSET + (i2c_num << 1));
	reg = __raw_readl(&imx_ccm->CCGR2);
	if (enable)
		reg |= mask;
	else
		reg &= ~mask;
	__raw_writel(reg, &imx_ccm->CCGR2);
	return 0;
}
#endif

static u32 decode_pll(enum pll_clocks pll, u32 infreq)
{
	u32 div;

	switch (pll) {
	case PLL_SYS:
		div = __raw_readl(&imx_ccm->analog_pll_sys);
		div &= BM_ANADIG_PLL_SYS_DIV_SELECT;

		return (infreq * div) >> 1;
	case PLL_BUS:
		div = __raw_readl(&imx_ccm->analog_pll_528);
		div &= BM_ANADIG_PLL_528_DIV_SELECT;

		return infreq * (20 + (div << 1));
	case PLL_USBOTG:
		div = __raw_readl(&imx_ccm->analog_usb1_pll_480_ctrl);
		div &= BM_ANADIG_USB1_PLL_480_CTRL_DIV_SELECT;

		return infreq * (20 + (div << 1));
	case PLL_ENET:
		div = __raw_readl(&imx_ccm->analog_pll_enet);
		div &= BM_ANADIG_PLL_ENET_DIV_SELECT;

		return 25000000 * (div + (div >> 1) + 1);
	default:
		return 0;
	}
	/* NOTREACHED */
}
static u32 mxc_get_pll_pfd(enum pll_clocks pll, int pfd_num)
{
	u32 div;
	u64 freq;

	switch (pll) {
	case PLL_BUS:
		if (pfd_num == 3) {
			/* No PFD3 on PPL2 */
			return 0;
		}
		div = __raw_readl(&imx_ccm->analog_pfd_528);
		freq = (u64)decode_pll(PLL_BUS, MXC_HCLK);
		break;
	case PLL_USBOTG:
		div = __raw_readl(&imx_ccm->analog_pfd_480);
		freq = (u64)decode_pll(PLL_USBOTG, MXC_HCLK);
		break;
	default:
		/* No PFD on other PLL					     */
		return 0;
	}

	return lldiv(freq * 18, (div & ANATOP_PFD_FRAC_MASK(pfd_num)) >>
			      ANATOP_PFD_FRAC_SHIFT(pfd_num));
}

static u32 get_mcu_main_clk(void)
{
	u32 reg, freq;

	reg = __raw_readl(&imx_ccm->cacrr);
	reg &= MXC_CCM_CACRR_ARM_PODF_MASK;
	reg >>= MXC_CCM_CACRR_ARM_PODF_OFFSET;
	freq = decode_pll(PLL_SYS, MXC_HCLK);

	return freq / (reg + 1);
}

u32 get_periph_clk(void)
{
	u32 reg, freq = 0;

	reg = __raw_readl(&imx_ccm->cbcdr);
	if (reg & MXC_CCM_CBCDR_PERIPH_CLK_SEL) {
		reg = __raw_readl(&imx_ccm->cbcmr);
		reg &= MXC_CCM_CBCMR_PERIPH_CLK2_SEL_MASK;
		reg >>= MXC_CCM_CBCMR_PERIPH_CLK2_SEL_OFFSET;

		switch (reg) {
		case 0:
			freq = decode_pll(PLL_USBOTG, MXC_HCLK);
			break;
		case 1:
		case 2:
			freq = MXC_HCLK;
			break;
		default:
			break;
		}
	} else {
		reg = __raw_readl(&imx_ccm->cbcmr);
		reg &= MXC_CCM_CBCMR_PRE_PERIPH_CLK_SEL_MASK;
		reg >>= MXC_CCM_CBCMR_PRE_PERIPH_CLK_SEL_OFFSET;

		switch (reg) {
		case 0:
			freq = decode_pll(PLL_BUS, MXC_HCLK);
			break;
		case 1:
			freq = mxc_get_pll_pfd(PLL_BUS, 2);
			break;
		case 2:
			freq = mxc_get_pll_pfd(PLL_BUS, 0);
			break;
		case 3:
			/* static / 2 divider */
			freq = mxc_get_pll_pfd(PLL_BUS, 2) / 2;
			break;
		default:
			break;
		}
	}

	return freq;
}

static u32 get_ipg_clk(void)
{
	u32 reg, ipg_podf;

	reg = __raw_readl(&imx_ccm->cbcdr);
	reg &= MXC_CCM_CBCDR_IPG_PODF_MASK;
	ipg_podf = reg >> MXC_CCM_CBCDR_IPG_PODF_OFFSET;

	return get_ahb_clk() / (ipg_podf + 1);
}

static u32 get_ipg_per_clk(void)
{
	u32 reg, perclk_podf;

	reg = __raw_readl(&imx_ccm->cscmr1);
#if (defined(CONFIG_MX6SL) || defined(CONFIG_MX6SX))
	if (reg & MXC_CCM_CSCMR1_PER_CLK_SEL_MASK)
		return MXC_HCLK; /* OSC 24Mhz */
#endif
	perclk_podf = reg & MXC_CCM_CSCMR1_PERCLK_PODF_MASK;

	return get_ipg_clk() / (perclk_podf + 1);
}

static u32 get_uart_clk(void)
{
	u32 reg, uart_podf;
	u32 freq = decode_pll(PLL_USBOTG, MXC_HCLK) / 6; /* static divider */
	reg = __raw_readl(&imx_ccm->cscdr1);
#if (defined(CONFIG_MX6SL) || defined(CONFIG_MX6SX))
	if (reg & MXC_CCM_CSCDR1_UART_CLK_SEL)
		freq = MXC_HCLK;
#endif
	reg &= MXC_CCM_CSCDR1_UART_CLK_PODF_MASK;
	uart_podf = reg >> MXC_CCM_CSCDR1_UART_CLK_PODF_OFFSET;

	return freq / (uart_podf + 1);
}

static u32 get_cspi_clk(void)
{
	u32 reg, cspi_podf;

	reg = __raw_readl(&imx_ccm->cscdr2);
	reg &= MXC_CCM_CSCDR2_ECSPI_CLK_PODF_MASK;
	cspi_podf = reg >> MXC_CCM_CSCDR2_ECSPI_CLK_PODF_OFFSET;

	return	decode_pll(PLL_USBOTG, MXC_HCLK) / (8 * (cspi_podf + 1));
}

static u32 get_axi_clk(void)
{
	u32 root_freq, axi_podf;
	u32 cbcdr =  __raw_readl(&imx_ccm->cbcdr);

	axi_podf = cbcdr & MXC_CCM_CBCDR_AXI_PODF_MASK;
	axi_podf >>= MXC_CCM_CBCDR_AXI_PODF_OFFSET;

	if (cbcdr & MXC_CCM_CBCDR_AXI_SEL) {
		if (cbcdr & MXC_CCM_CBCDR_AXI_ALT_SEL)
			root_freq = mxc_get_pll_pfd(PLL_BUS, 2);
		else
			root_freq = mxc_get_pll_pfd(PLL_USBOTG, 1);
	} else
		root_freq = get_periph_clk();

	return  root_freq / (axi_podf + 1);
}

static u32 get_emi_slow_clk(void)
{
	u32 emi_clk_sel, emi_slow_podf, cscmr1, root_freq = 0;

	cscmr1 =  __raw_readl(&imx_ccm->cscmr1);
	emi_clk_sel = cscmr1 & MXC_CCM_CSCMR1_ACLK_EMI_SLOW_MASK;
	emi_clk_sel >>= MXC_CCM_CSCMR1_ACLK_EMI_SLOW_OFFSET;
	emi_slow_podf = cscmr1 & MXC_CCM_CSCMR1_ACLK_EMI_SLOW_PODF_MASK;
	emi_slow_podf >>= MXC_CCM_CSCMR1_ACLK_EMI_SLOW_PODF_OFFSET;

	switch (emi_clk_sel) {
	case 0:
		root_freq = get_axi_clk();
		break;
	case 1:
		root_freq = decode_pll(PLL_USBOTG, MXC_HCLK);
		break;
	case 2:
		root_freq =  mxc_get_pll_pfd(PLL_BUS, 2);
		break;
	case 3:
		root_freq =  mxc_get_pll_pfd(PLL_BUS, 0);
		break;
	}

	return root_freq / (emi_slow_podf + 1);
}

#if (defined(CONFIG_MX6SL) || defined(CONFIG_MX6SX))
static u32 get_mmdc_ch0_clk(void)
{
	u32 cbcmr = __raw_readl(&imx_ccm->cbcmr);
	u32 cbcdr = __raw_readl(&imx_ccm->cbcdr);
	u32 freq, podf;

	podf = (cbcdr & MXC_CCM_CBCDR_MMDC_CH1_PODF_MASK) \
			>> MXC_CCM_CBCDR_MMDC_CH1_PODF_OFFSET;

	switch ((cbcmr & MXC_CCM_CBCMR_PRE_PERIPH2_CLK_SEL_MASK) >>
		MXC_CCM_CBCMR_PRE_PERIPH2_CLK_SEL_OFFSET) {
	case 0:
		freq = decode_pll(PLL_BUS, MXC_HCLK);
		break;
	case 1:
		freq = mxc_get_pll_pfd(PLL_BUS, 2);
		break;
	case 2:
		freq = mxc_get_pll_pfd(PLL_BUS, 0);
		break;
	case 3:
		/* static / 2 divider */
		freq =  mxc_get_pll_pfd(PLL_BUS, 2) / 2;
	}

	return freq / (podf + 1);

}
#else
static u32 get_mmdc_ch0_clk(void)
{
	u32 cbcdr = __raw_readl(&imx_ccm->cbcdr);
	u32 mmdc_ch0_podf = (cbcdr & MXC_CCM_CBCDR_MMDC_CH0_PODF_MASK) >>
				MXC_CCM_CBCDR_MMDC_CH0_PODF_OFFSET;

	return get_periph_clk() / (mmdc_ch0_podf + 1);
}
#endif

#ifdef CONFIG_MX6SX
void enable_lvds(uint32_t lcdif_base)
{
	u32 reg = 0;
	struct iomuxc *iomux = (struct iomuxc *)IOMUXC_GPR_BASE_ADDR;

	/* Turn on LDB DI0 clocks */
	reg = readl(&imx_ccm->CCGR3);
	reg |=  MXC_CCM_CCGR3_LDB_DI0_MASK;
	writel(reg, &imx_ccm->CCGR3);

	/* set LDB DI0 clk select to 011 PLL2 PFD3 200M*/
	reg = readl(&imx_ccm->cs2cdr);
	reg &= ~MXC_CCM_CS2CDR_LDB_DI0_CLK_SEL_MASK;
	reg |= (3 << MXC_CCM_CS2CDR_LDB_DI0_CLK_SEL_OFFSET);
	writel(reg, &imx_ccm->cs2cdr);

	reg = readl(&imx_ccm->cscmr2);
	reg |= MXC_CCM_CSCMR2_LDB_DI0_IPU_DIV;
	writel(reg, &imx_ccm->cscmr2);

	/* set LDB DI0 clock for LCDIF PIX clock */
	reg = readl(&imx_ccm->cscdr2);
	if (lcdif_base == LCDIF1_BASE_ADDR) {
		reg &= ~MXC_CCM_CSCDR2_LCDIF1_CLK_SEL_MASK;
		reg |= (0x3 << MXC_CCM_CSCDR2_LCDIF1_CLK_SEL_OFFSET);
	} else {
		reg &= ~MXC_CCM_CSCDR2_LCDIF2_CLK_SEL_MASK;
		reg |= (0x3 << MXC_CCM_CSCDR2_LCDIF2_CLK_SEL_OFFSET);
	}
	writel(reg, &imx_ccm->cscdr2);

	reg = IOMUXC_GPR2_DI0_VS_POLARITY_ACTIVE_LOW
		| IOMUXC_GPR2_BIT_MAPPING_CH0_SPWG
		| IOMUXC_GPR2_DATA_WIDTH_CH0_18BIT
		| IOMUXC_GPR2_LVDS_CH0_MODE_ENABLED_DI0;
	writel(reg, &iomux->gpr[6]);

	reg = readl(&iomux->gpr[5]);
	if (lcdif_base == LCDIF1_BASE_ADDR)
		reg &= ~0x8;  /* MUX LVDS to LCDIF1 */
	else
		reg |= 0x8; /* MUX LVDS to LCDIF2 */
	writel(reg, &iomux->gpr[5]);
}

void enable_lcdif_clock(uint32_t base_addr)
{
	u32 reg = 0;

	/* Set to pre-mux clock at default */
	reg = readl(&imx_ccm->cscdr2);
	if (base_addr == LCDIF1_BASE_ADDR)
		reg &= ~MXC_CCM_CSCDR2_LCDIF1_CLK_SEL_MASK;
	else
		reg &= ~MXC_CCM_CSCDR2_LCDIF2_CLK_SEL_MASK;
	writel(reg, &imx_ccm->cscdr2);

	/* Enable the LCDIF pix clock, axi clock, disp axi clock */
	reg = readl(&imx_ccm->CCGR3);
	if (base_addr == LCDIF1_BASE_ADDR)
		reg |= (MXC_CCM_CCGR3_LCDIF1_PIX_MASK | MXC_CCM_CCGR3_DISP_AXI_MASK);
	else
		reg |= (MXC_CCM_CCGR3_LCDIF2_PIX_MASK | MXC_CCM_CCGR3_DISP_AXI_MASK);
	writel(reg, &imx_ccm->CCGR3);

	reg = readl(&imx_ccm->CCGR2);
	reg |= (MXC_CCM_CCGR2_LCD_MASK);
	writel(reg, &imx_ccm->CCGR2);
}

static int enable_pll_video(u32 pll_div, u32 pll_num, u32 pll_denom)
{
	u32 reg = 0;
	ulong start;

	debug("pll5 div = %d, num = %d, denom = %d\n",
		pll_div, pll_num, pll_denom);

	/* Power up PLL5 video */
	writel(BM_ANADIG_PLL_VIDEO_POWERDOWN | BM_ANADIG_PLL_VIDEO_BYPASS |
		BM_ANADIG_PLL_VIDEO_DIV_SELECT | BM_ANADIG_PLL_VIDEO_TEST_DIV_SELECT,
		&imx_ccm->analog_pll_video_clr);

	/* Set div, num and denom */
	writel(BF_ANADIG_PLL_VIDEO_DIV_SELECT(pll_div) |
		BF_ANADIG_PLL_VIDEO_TEST_DIV_SELECT(0x2),
		&imx_ccm->analog_pll_video_set);

	writel(BF_ANADIG_PLL_VIDEO_NUM_A(pll_num),
		&imx_ccm->analog_pll_video_num);

	writel(BF_ANADIG_PLL_VIDEO_DENOM_B(pll_denom),
		&imx_ccm->analog_pll_video_denon);

	/* Wait PLL5 lock */
	start = get_timer(0);	/* Get current timestamp */

	do {
		reg = readl(&imx_ccm->analog_pll_video);
		if (reg & BM_ANADIG_PLL_VIDEO_LOCK) {
			/* Enable PLL out */
			writel(BM_ANADIG_PLL_VIDEO_ENABLE,
					&imx_ccm->analog_pll_video_set);
			return 0;
		}
	} while (get_timer(0) < (start + 10)); /* Wait 10ms */

	printf("Lock PLL5 timeout\n");
	return 1;

}

void mxs_set_lcdclk(uint32_t base_addr, uint32_t freq)
{
	u32 reg = 0;
	u32 hck = MXC_HCLK/1000;
	u32 min = hck * 27;
	u32 max = hck * 54;
	u32 temp, best = 0;
	u32 i, j, pred = 1, postd = 1;
	u32 pll_div, pll_num, pll_denom;

	debug("mxs_set_lcdclk, freq = %d\n", freq);

	if (base_addr == LCDIF1_BASE_ADDR) {
		reg = readl(&imx_ccm->cscdr2);
		if ((reg & MXC_CCM_CSCDR2_LCDIF1_CLK_SEL_MASK) != 0)
			return; /*Can't change clocks when clock not from pre-mux */
	} else {
		reg = readl(&imx_ccm->cscdr2);
		if ((reg & MXC_CCM_CSCDR2_LCDIF2_CLK_SEL_MASK) != 0)
			return; /*Can't change clocks when clock not from pre-mux */
	}

	for (i = 1; i <= 8; i++) {
		for (j = 1; j <= 8; j++) {
			temp = freq * i * j;
			if (temp > max || temp < min)
				continue;

			if (best == 0 || temp < best) {
				best = temp;
				pred = i;
				postd = j;
			}
		}
	}

	if (best == 0) {
		printf("Fail to set rate to %dkhz", freq);
		return;
	}

	debug("best %d, pred = %d, postd = %d\n", best, pred, postd);

	pll_div = best / hck;
	pll_denom = 1000000;
	pll_num = (best - hck * pll_div) * pll_denom / hck;

	if (base_addr == LCDIF1_BASE_ADDR) {
		if (enable_pll_video(pll_div, pll_num, pll_denom))
			return;

		/* Select pre-lcd clock to PLL5 */
		reg = readl(&imx_ccm->cscdr2);
		reg &= ~MXC_CCM_CSCDR2_LCDIF1_PRED_SEL_MASK;
		reg |= (0x2 << MXC_CCM_CSCDR2_LCDIF1_PRED_SEL_OFFSET);
		/* Set the pre divider */
		reg &= ~MXC_CCM_CSCDR2_LCDIF1_PRE_DIV_MASK;
		reg |= ((pred - 1) << MXC_CCM_CSCDR2_LCDIF1_PRE_DIV_OFFSET);
		writel(reg, &imx_ccm->cscdr2);

		/* Set the post divider */
		reg = readl(&imx_ccm->cbcmr);
		reg &= ~MXC_CCM_CBCMR_LCDIF1_PODF_MASK;
		reg |= ((postd - 1) << MXC_CCM_CBCMR_LCDIF1_PODF_OFFSET);
		writel(reg, &imx_ccm->cbcmr);

	} else {
		if (enable_pll_video(pll_div, pll_num, pll_denom))
			return;

		/* Select pre-lcd clock to PLL5 */
		reg = readl(&imx_ccm->cscdr2);
		reg &= ~MXC_CCM_CSCDR2_LCDIF2_PRED_SEL_MASK;
		reg |= (0x2 << MXC_CCM_CSCDR2_LCDIF2_PRED_SEL_OFFSET);
		/* Set the pre divider */
		reg &= ~MXC_CCM_CSCDR2_LCDIF2_PRE_DIV_MASK;
		reg |= ((pred - 1) << MXC_CCM_CSCDR2_LCDIF2_PRE_DIV_OFFSET);
		writel(reg, &imx_ccm->cscdr2);

		/* Set the post divider */
		reg = readl(&imx_ccm->cscmr1);
		reg &= ~MXC_CCM_CSCMR1_LCDIF2_PODF_MASK;
		reg |= ((postd - 1) << MXC_CCM_CSCMR1_LCDIF2_PODF_OFFSET);
		writel(reg, &imx_ccm->cscmr1);
	}
}

/* qspi_num can be from 0 - 1 */
void enable_qspi_clk(int qspi_num)
{
    u32 reg = 0;

    /* Enable QuadSPI clock */
	switch (qspi_num) {
	case 0:
		/*disable the clock gate*/
		clrbits_le32(&imx_ccm->CCGR3, MXC_CCM_CCGR3_QSPI1_MASK);

		/* set 50M  : (50 = 396 / 2 / 4) */
		reg = readl(&imx_ccm->cscmr1);
		reg &= ~(MXC_CCM_CSCMR1_QSPI1_PODF_MASK |
			MXC_CCM_CSCMR1_QSPI1_CLK_SEL_MASK);
		reg |= ((1 << MXC_CCM_CSCMR1_QSPI1_PODF_OFFSET) |
				(2 << MXC_CCM_CSCMR1_QSPI1_CLK_SEL_OFFSET));
		writel(reg, &imx_ccm->cscmr1);

		/*enable the clock gate*/
		setbits_le32(&imx_ccm->CCGR3, MXC_CCM_CCGR3_QSPI1_MASK);
		break;
	case 1:
		/*disable the clock gatei*/
		/*QSPI2 and GPMI_BCH_INPUT_GPMI_IO share the same clock gate, disable both of them*/
		clrbits_le32(&imx_ccm->CCGR4, MXC_CCM_CCGR4_QSPI2_ENFC_MASK
				| MXC_CCM_CCGR4_RAWNAND_U_GPMI_BCH_INPUT_GPMI_IO_MASK);

		/* set 50M  : (50 = 396 / 2 / 4) */
		reg = readl(&imx_ccm->cs2cdr);
		reg &= ~(MXC_CCM_CS2CDR_QSPI2_CLK_PODF_MASK |
				MXC_CCM_CS2CDR_QSPI2_CLK_PRED_MASK |
				MXC_CCM_CS2CDR_QSPI2_CLK_SEL_MASK);
		reg |= (MXC_CCM_CS2CDR_QSPI2_CLK_PRED(0x1) |
				MXC_CCM_CS2CDR_QSPI2_CLK_SEL(0x3));
		writel(reg, &imx_ccm->cs2cdr);

		/*enable the clock gate*/
		setbits_le32(&imx_ccm->CCGR4, MXC_CCM_CCGR4_QSPI2_ENFC_MASK
				| MXC_CCM_CCGR4_RAWNAND_U_GPMI_BCH_INPUT_GPMI_IO_MASK);
		break;
	default:
		break;
	}
}

void enable_enet_clock(void)
{
	u32 reg = 0;

	/* set enet ahb clock 200Mhz
	 * pll2_pfd2_396m-> ENET_PODF-> ENET_AHB
	 */
	reg = __raw_readl(&imx_ccm->chsccdr);
	reg &= ~(MXC_CCM_CHSCCDR_ENET_PRE_CLK_SEL_MASK
		| MXC_CCM_CHSCCDR_ENET_PODF_MASK | MXC_CCM_CHSCCDR_ENET_CLK_SEL_MASK);
	/* PLL2 PFD2 */
	reg |= (4 << MXC_CCM_CHSCCDR_ENET_PRE_CLK_SEL_OFFSET);
	/* Div = 2*/
	reg |= (1 << MXC_CCM_CHSCCDR_ENET_PODF_OFFSET);
	reg |= (0 << MXC_CCM_CHSCCDR_ENET_CLK_SEL_OFFSET);
	writel(reg, &imx_ccm->chsccdr);

	/* Enable enet system clock */
	reg = readl(&imx_ccm->CCGR3);
	reg |= MXC_CCM_CCGR3_ENET_MASK;
	writel(reg, &imx_ccm->CCGR3);
}

void mxs_set_vadcclk()
{
	u32 reg = 0;

	reg = readl(&imx_ccm->cscmr2);
	reg &= ~MXC_CCM_CSCMR2_VID_CLK_SEL_MASK;
	reg |= 0x19 << MXC_CCM_CSCMR2_VID_CLK_SEL_OFFSET;
	writel(reg, &imx_ccm->cscmr2);
}
#endif

#ifdef CONFIG_FEC_MXC
int enable_fec_anatop_clock(int fec_id, enum enet_freq freq)
{
	u32 reg = 0;
	s32 timeout = 100000;

	if (freq < ENET_25MHz || freq > ENET_125MHz)
		return -EINVAL;

	reg = readl(&imx_ccm->analog_pll_enet);
	reg &= ~BM_ANADIG_PLL_ENET_DIV_SELECT;

	if (0 == fec_id) {
		reg &= ~BM_ANADIG_PLL_ENET_DIV_SELECT;
		reg |= BF_ANADIG_PLL_ENET_DIV_SELECT(freq);
	} else {
		reg &= ~BM_ANADIG_PLL_ENET2_DIV_SELECT;
		reg |= BF_ANADIG_PLL_ENET2_DIV_SELECT(freq);
	}

	if ((reg & BM_ANADIG_PLL_ENET_POWERDOWN) ||
	    (!(reg & BM_ANADIG_PLL_ENET_LOCK))) {
		reg &= ~BM_ANADIG_PLL_ENET_POWERDOWN;
		writel(reg, &imx_ccm->analog_pll_enet);
		while (timeout--) {
			if (readl(&imx_ccm->analog_pll_enet) & BM_ANADIG_PLL_ENET_LOCK)
				break;
		}
		if (timeout < 0)
			return -ETIMEDOUT;
	}

	/* Enable FEC clock */
	if (0 == fec_id)
		reg |= BM_ANADIG_PLL_ENET_ENABLE;
	else
		reg |= BM_ANADIG_PLL_ENET2_ENABLE;
	reg &= ~BM_ANADIG_PLL_ENET_BYPASS;
#ifdef CONFIG_FEC_MXC_25M_REF_CLK
	reg |= BM_ANADIG_PLL_ENET_REF_25M_ENABLE;
#endif
	writel(reg, &imx_ccm->analog_pll_enet);

	return 0;
}
#endif

static u32 get_usdhc_clk(u32 port)
{
	u32 root_freq = 0, usdhc_podf = 0, clk_sel = 0;
	u32 cscmr1 = __raw_readl(&imx_ccm->cscmr1);
	u32 cscdr1 = __raw_readl(&imx_ccm->cscdr1);

	switch (port) {
	case 0:
		usdhc_podf = (cscdr1 & MXC_CCM_CSCDR1_USDHC1_PODF_MASK) >>
					MXC_CCM_CSCDR1_USDHC1_PODF_OFFSET;
		clk_sel = cscmr1 & MXC_CCM_CSCMR1_USDHC1_CLK_SEL;

		break;
	case 1:
		usdhc_podf = (cscdr1 & MXC_CCM_CSCDR1_USDHC2_PODF_MASK) >>
					MXC_CCM_CSCDR1_USDHC2_PODF_OFFSET;
		clk_sel = cscmr1 & MXC_CCM_CSCMR1_USDHC2_CLK_SEL;

		break;
	case 2:
		usdhc_podf = (cscdr1 & MXC_CCM_CSCDR1_USDHC3_PODF_MASK) >>
					MXC_CCM_CSCDR1_USDHC3_PODF_OFFSET;
		clk_sel = cscmr1 & MXC_CCM_CSCMR1_USDHC3_CLK_SEL;

		break;
	case 3:
		usdhc_podf = (cscdr1 & MXC_CCM_CSCDR1_USDHC4_PODF_MASK) >>
					MXC_CCM_CSCDR1_USDHC4_PODF_OFFSET;
		clk_sel = cscmr1 & MXC_CCM_CSCMR1_USDHC4_CLK_SEL;

		break;
	default:
		break;
	}

	if (clk_sel)
		root_freq = mxc_get_pll_pfd(PLL_BUS, 0);
	else
		root_freq = mxc_get_pll_pfd(PLL_BUS, 2);

	return root_freq / (usdhc_podf + 1);
}

u32 imx_get_uartclk(void)
{
	return get_uart_clk();
}

u32 imx_get_fecclk(void)
{
	return mxc_get_clock(MXC_IPG_CLK);
}

static int enable_enet_pll(uint32_t en)
{
	struct mxc_ccm_reg *const imx_ccm
		= (struct mxc_ccm_reg *) CCM_BASE_ADDR;
	s32 timeout = 100000;
	u32 reg = 0;

	/* Enable PLLs */
	reg = readl(&imx_ccm->analog_pll_enet);
	reg &= ~BM_ANADIG_PLL_SYS_POWERDOWN;
	writel(reg, &imx_ccm->analog_pll_enet);
	reg |= BM_ANADIG_PLL_SYS_ENABLE;
	while (timeout--) {
		if (readl(&imx_ccm->analog_pll_enet) & BM_ANADIG_PLL_SYS_LOCK)
			break;
	}
	if (timeout <= 0)
		return -EIO;
	reg &= ~BM_ANADIG_PLL_SYS_BYPASS;
	writel(reg, &imx_ccm->analog_pll_enet);
	reg |= en;
	writel(reg, &imx_ccm->analog_pll_enet);
	return 0;
}

#ifndef CONFIG_MX6SX
static void ungate_sata_clock(void)
{
	struct mxc_ccm_reg *const imx_ccm =
		(struct mxc_ccm_reg *)CCM_BASE_ADDR;

	/* Enable SATA clock. */
	setbits_le32(&imx_ccm->CCGR5, MXC_CCM_CCGR5_SATA_MASK);
}
#else
static void ungate_disp_axi_clock(void)
{
	struct mxc_ccm_reg *const imx_ccm =
		(struct mxc_ccm_reg *)CCM_BASE_ADDR;

	/* Enable display axi clock. */
	setbits_le32(&imx_ccm->CCGR3, MXC_CCM_CCGR3_DISP_AXI_MASK);
}
#endif

static void ungate_pcie_clock(void)
{
	struct mxc_ccm_reg *const imx_ccm =
		(struct mxc_ccm_reg *)CCM_BASE_ADDR;

	/* Enable PCIe clock. */
	setbits_le32(&imx_ccm->CCGR4, MXC_CCM_CCGR4_PCIE_MASK);
}

#ifndef CONFIG_MX6SX
int enable_sata_clock(void)
{
	ungate_sata_clock();
	return enable_enet_pll(BM_ANADIG_PLL_ENET_ENABLE_SATA);
}
#endif

int enable_pcie_clock(void)
{
	/* PCIe reference clock sourced from AXI. */
	clrbits_le32(&imx_ccm->cbcmr, MXC_CCM_CBCMR_PCIE_AXI_CLK_SEL);

	/*
	 * Here be dragons!
	 *
	 * The register ANATOP_MISC1 is not documented in the Freescale
	 * MX6RM. The register that is mapped in the ANATOP space and
	 * marked as ANATOP_MISC1 is actually documented in the PMU section
	 * of the datasheet as PMU_MISC1.
	 *
	 * Switch LVDS clock source to SATA (0xb), disable clock INPUT and
	 * enable clock OUTPUT. This is important for PCI express link that
	 * is clocked from the i.MX6.
	 */
#define ANADIG_ANA_MISC1_LVDSCLK1_IBEN		(1 << 12)
#define ANADIG_ANA_MISC1_LVDSCLK1_OBEN		(1 << 10)
#define ANADIG_ANA_MISC1_LVDS1_CLK_SEL_MASK	0x0000001F
#ifndef CONFIG_MX6SX
	/* lvds_clk1 is sourced from sata ref on imx6q/dl/solo */
	clrsetbits_le32(&imx_ccm->ana_misc1,
			ANADIG_ANA_MISC1_LVDSCLK1_IBEN |
			ANADIG_ANA_MISC1_LVDS1_CLK_SEL_MASK,
			ANADIG_ANA_MISC1_LVDSCLK1_OBEN | 0xb);

	/* Party time! Ungate the clock to the PCIe. */
	ungate_sata_clock();
	ungate_pcie_clock();

	return enable_enet_pll(BM_ANADIG_PLL_ENET_ENABLE_SATA |
			BM_ANADIG_PLL_ENET_ENABLE_PCIE);
#else
	/* lvds_clk1 is sourced from pcie ref on imx6sx */
	clrsetbits_le32(&imx_ccm->ana_misc1,
			ANADIG_ANA_MISC1_LVDSCLK1_IBEN |
			ANADIG_ANA_MISC1_LVDS1_CLK_SEL_MASK,
			ANADIG_ANA_MISC1_LVDSCLK1_OBEN | 0xa);

	ungate_disp_axi_clock();
	ungate_pcie_clock();
	return enable_enet_pll(BM_ANADIG_PLL_ENET_ENABLE_PCIE);
#endif
}

#ifdef CONFIG_SECURE_BOOT
void hab_caam_clock_enable(void)
{
	struct mxc_ccm_reg *ccm_regs = (struct mxc_ccm_reg *)CCM_BASE_ADDR;
	u32 reg = 0;

	/*CG4 ~ CG6, enable CAAM clocks*/
	reg = readl(&ccm_regs->CCGR0);
	reg |= (MXC_CCM_CCGR0_CAAM_WRAPPER_IPG_MASK |
			MXC_CCM_CCGR0_CAAM_WRAPPER_ACLK_MASK |
			MXC_CCM_CCGR0_CAAM_SECURE_MEM_MASK);
	writel(reg, &ccm_regs->CCGR0);

	/* Enable EMI slow clk */
	reg = readl(&ccm_regs->CCGR6);
	reg |= MXC_CCM_CCGR6_EMI_SLOW_MASK;
	writel(reg, &ccm_regs->CCGR6);
}

void hab_caam_clock_disable(void)
{
	struct mxc_ccm_reg *ccm_regs = (struct mxc_ccm_reg *)CCM_BASE_ADDR;
	u32 reg = 0;

	/*CG4 ~ CG6, disable CAAM clocks*/
	reg = readl(&ccm_regs->CCGR0);
	reg &= ~(MXC_CCM_CCGR0_CAAM_WRAPPER_IPG_MASK |
			MXC_CCM_CCGR0_CAAM_WRAPPER_ACLK_MASK |
			MXC_CCM_CCGR0_CAAM_SECURE_MEM_MASK);
	writel(reg, &ccm_regs->CCGR0);

	/* Disable EMI slow clk */
	reg = readl(&ccm_regs->CCGR6);
	reg &= ~MXC_CCM_CCGR6_EMI_SLOW_MASK;
	writel(reg, &ccm_regs->CCGR6);
}
#endif

unsigned int mxc_get_clock(enum mxc_clock clk)
{
	switch (clk) {
	case MXC_ARM_CLK:
		return get_mcu_main_clk();
	case MXC_PER_CLK:
		return get_periph_clk();
	case MXC_AHB_CLK:
		return get_ahb_clk();
	case MXC_IPG_CLK:
		return get_ipg_clk();
	case MXC_IPG_PERCLK:
	case MXC_I2C_CLK:
		return get_ipg_per_clk();
	case MXC_UART_CLK:
		return get_uart_clk();
	case MXC_CSPI_CLK:
		return get_cspi_clk();
	case MXC_AXI_CLK:
		return get_axi_clk();
	case MXC_EMI_SLOW_CLK:
		return get_emi_slow_clk();
	case MXC_DDR_CLK:
		return get_mmdc_ch0_clk();
	case MXC_ESDHC_CLK:
		return get_usdhc_clk(0);
	case MXC_ESDHC2_CLK:
		return get_usdhc_clk(1);
	case MXC_ESDHC3_CLK:
		return get_usdhc_clk(2);
	case MXC_ESDHC4_CLK:
		return get_usdhc_clk(3);
	case MXC_SATA_CLK:
		return get_ahb_clk();
	default:
		break;
	}

	return -1;
}

/*
 * Dump some core clockes.
 */
int do_mx6_showclocks(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	u32 freq;
	freq = decode_pll(PLL_SYS, MXC_HCLK);
	printf("PLL_SYS    %8d MHz\n", freq / 1000000);
	freq = decode_pll(PLL_BUS, MXC_HCLK);
	printf("PLL_BUS    %8d MHz\n", freq / 1000000);
	freq = decode_pll(PLL_USBOTG, MXC_HCLK);
	printf("PLL_OTG    %8d MHz\n", freq / 1000000);
	freq = decode_pll(PLL_ENET, MXC_HCLK);
	printf("PLL_NET    %8d MHz\n", freq / 1000000);

	printf("\n");
	printf("IPG        %8d kHz\n", mxc_get_clock(MXC_IPG_CLK) / 1000);
	printf("UART       %8d kHz\n", mxc_get_clock(MXC_UART_CLK) / 1000);
#ifdef CONFIG_MXC_SPI
	printf("CSPI       %8d kHz\n", mxc_get_clock(MXC_CSPI_CLK) / 1000);
#endif
	printf("AHB        %8d kHz\n", mxc_get_clock(MXC_AHB_CLK) / 1000);
	printf("AXI        %8d kHz\n", mxc_get_clock(MXC_AXI_CLK) / 1000);
	printf("DDR        %8d kHz\n", mxc_get_clock(MXC_DDR_CLK) / 1000);
	printf("USDHC1     %8d kHz\n", mxc_get_clock(MXC_ESDHC_CLK) / 1000);
	printf("USDHC2     %8d kHz\n", mxc_get_clock(MXC_ESDHC2_CLK) / 1000);
	printf("USDHC3     %8d kHz\n", mxc_get_clock(MXC_ESDHC3_CLK) / 1000);
	printf("USDHC4     %8d kHz\n", mxc_get_clock(MXC_ESDHC4_CLK) / 1000);
	printf("EMI SLOW   %8d kHz\n", mxc_get_clock(MXC_EMI_SLOW_CLK) / 1000);
	printf("IPG PERCLK %8d kHz\n", mxc_get_clock(MXC_IPG_PERCLK) / 1000);

	return 0;
}

#ifndef CONFIG_MX6SX
void enable_ipu_clock(void)
{
	struct mxc_ccm_reg *mxc_ccm = (struct mxc_ccm_reg *)CCM_BASE_ADDR;
	int reg;
	reg = readl(&mxc_ccm->CCGR3);
	reg |= MXC_CCM_CCGR3_IPU1_IPU_MASK;
	writel(reg, &mxc_ccm->CCGR3);
}
#endif
/***************************************************/

U_BOOT_CMD(
	clocks,	CONFIG_SYS_MAXARGS, 1, do_mx6_showclocks,
	"display clocks",
	""
);
