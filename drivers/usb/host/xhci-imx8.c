/*
 * Copyright 2017 NXP
 *
 * NXP i.MX8 USB HOST xHCI Controller (Cadence IP)
 *
 * Author: Peter Chen <peter.chen@nxp.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <usb.h>
#include <malloc.h>
#include <linux/compat.h>
#include "xhci.h"
#include <usb/imx8_usb3_reg_def.h>
#include <dm.h>
#include <power-domain.h>
#include <asm/arch/clock.h>

/* Declare global data pointer */
DECLARE_GLOBAL_DATA_PTR;

/* According to UG CH 3.1.1 Bring-up Sequence */
static void imx_usb3_phy_init(void)
{
	writel(0x0830, PHY_PMA_CMN_CTRL1);
	writel(0x10, TB_ADDR_CMN_DIAG_HSCLK_SEL);
	writel(0x00F0, TB_ADDR_CMN_PLL0_VCOCAL_INIT_TMR);
	writel(0x0018, TB_ADDR_CMN_PLL0_VCOCAL_ITER_TMR);
	writel(0x00D0, TB_ADDR_CMN_PLL0_INTDIV);
	writel(0x4aaa, TB_ADDR_CMN_PLL0_FRACDIV);
	writel(0x0034, TB_ADDR_CMN_PLL0_HIGH_THR);
	writel(0x1ee, TB_ADDR_CMN_PLL0_SS_CTRL1);
	writel(0x7F03, TB_ADDR_CMN_PLL0_SS_CTRL2);
	writel(0x0020, TB_ADDR_CMN_PLL0_DSM_DIAG);
	writel(0x0000, TB_ADDR_CMN_DIAG_PLL0_OVRD);
	writel(0x0000, TB_ADDR_CMN_DIAG_PLL0_FBH_OVRD);
	writel(0x0000, TB_ADDR_CMN_DIAG_PLL0_FBL_OVRD);
	writel(0x0007, TB_ADDR_CMN_DIAG_PLL0_V2I_TUNE);
	writel(0x0027, TB_ADDR_CMN_DIAG_PLL0_CP_TUNE);
	writel(0x0008, TB_ADDR_CMN_DIAG_PLL0_LF_PROG);
	writel(0x0022, TB_ADDR_CMN_DIAG_PLL0_TEST_MODE);
	writel(0x000a, TB_ADDR_CMN_PSM_CLK_CTRL);
	writel(0x139, TB_ADDR_XCVR_DIAG_RX_LANE_CAL_RST_TMR);
	writel(0xbefc, TB_ADDR_XCVR_PSM_RCTRL);

	writel(0x7799, TB_ADDR_TX_PSC_A0);
	writel(0x7798, TB_ADDR_TX_PSC_A1);
	writel(0x509b, TB_ADDR_TX_PSC_A2);
	writel(0x3, TB_ADDR_TX_DIAG_ECTRL_OVRD);
	writel(0x5098, TB_ADDR_TX_PSC_A3);
	writel(0x2090, TB_ADDR_TX_PSC_CAL);
	writel(0x2090, TB_ADDR_TX_PSC_RDY);

	writel(0xA6FD, TB_ADDR_RX_PSC_A0);
	writel(0xA6FD, TB_ADDR_RX_PSC_A1);
	writel(0xA410, TB_ADDR_RX_PSC_A2);
	writel(0x2410, TB_ADDR_RX_PSC_A3);

	writel(0x23FF, TB_ADDR_RX_PSC_CAL);
	writel(0x2010, TB_ADDR_RX_PSC_RDY);

	writel(0x0020, TB_ADDR_TX_TXCC_MGNLS_MULT_000);
	writel(0x00ff, TB_ADDR_TX_DIAG_BGREF_PREDRV_DELAY);
	writel(0x0002, TB_ADDR_RX_SLC_CU_ITER_TMR);
	writel(0x0013, TB_ADDR_RX_SIGDET_HL_FILT_TMR);
	writel(0x0000, TB_ADDR_RX_SAMP_DAC_CTRL);
	writel(0x1004, TB_ADDR_RX_DIAG_SIGDET_TUNE);
	writel(0x4041, TB_ADDR_RX_DIAG_LFPSDET_TUNE2);
	writel(0x0480, TB_ADDR_RX_DIAG_BS_TM);
	writel(0x8006, TB_ADDR_RX_DIAG_DFE_CTRL1);
	writel(0x003f, TB_ADDR_RX_DIAG_ILL_IQE_TRIM4);
	writel(0x543f, TB_ADDR_RX_DIAG_ILL_E_TRIM0);
	writel(0x543f, TB_ADDR_RX_DIAG_ILL_IQ_TRIM0);
	writel(0x0000, TB_ADDR_RX_DIAG_ILL_IQE_TRIM6);
	writel(0x8000, TB_ADDR_RX_DIAG_RXFE_TM3);
	writel(0x0003, TB_ADDR_RX_DIAG_RXFE_TM4);
	writel(0x2408, TB_ADDR_RX_DIAG_LFPSDET_TUNE);
	writel(0x05ca, TB_ADDR_RX_DIAG_DFE_CTRL3);
	writel(0x0258, TB_ADDR_RX_DIAG_SC2C_DELAY);
	writel(0x1fff, TB_ADDR_RX_REE_VGA_GAIN_NODFE);

	writel(0x02c6, TB_ADDR_XCVR_PSM_CAL_TMR);
	writel(0x0002, TB_ADDR_XCVR_PSM_A0BYP_TMR);
	writel(0x02c6, TB_ADDR_XCVR_PSM_A0IN_TMR);
	writel(0x0010, TB_ADDR_XCVR_PSM_A1IN_TMR);
	writel(0x0010, TB_ADDR_XCVR_PSM_A2IN_TMR);
	writel(0x0010, TB_ADDR_XCVR_PSM_A3IN_TMR);
	writel(0x0010, TB_ADDR_XCVR_PSM_A4IN_TMR);
	writel(0x0010, TB_ADDR_XCVR_PSM_A5IN_TMR);

	writel(0x0002, TB_ADDR_XCVR_PSM_A0OUT_TMR);
	writel(0x0002, TB_ADDR_XCVR_PSM_A1OUT_TMR);
	writel(0x0002, TB_ADDR_XCVR_PSM_A2OUT_TMR);
	writel(0x0002, TB_ADDR_XCVR_PSM_A3OUT_TMR);
	writel(0x0002, TB_ADDR_XCVR_PSM_A4OUT_TMR);
	writel(0x0002, TB_ADDR_XCVR_PSM_A5OUT_TMR);

	/* Change rx detect parameter */
	writel(0x960, TB_ADDR_TX_RCVDET_EN_TMR);
	writel(0x01e0, TB_ADDR_TX_RCVDET_ST_TMR);
	writel(0x0090, TB_ADDR_XCVR_DIAG_LANE_FCM_EN_MGN_TMR);

	udelay(10);

	/* force rx detect */
/*	writel(0xc000, TB_ADDR_TX_RCVDET_OVRD); */
}

void imx8_xhci_init(void)
{
	u32 tmp_data;

	tmp_data = readl(USB3_SSPHY_STATUS);
	writel(tmp_data, USB3_SSPHY_STATUS);
	tmp_data = readl(USB3_SSPHY_STATUS);
	while ((tmp_data & 0xf0000000) != 0xf0000000) {
		printf("clkvld is incorrect = 0x%x\n", tmp_data);
		udelay(10);
		tmp_data = readl(USB3_SSPHY_STATUS);
	}

	tmp_data = readl(USB3_CORE_CTRL1);
	tmp_data = (tmp_data & 0xfffffff8) | 0x202;
	writel(tmp_data, USB3_CORE_CTRL1);
	tmp_data &= ~0x04000000; /* clear PHY apb reset */
	writel(tmp_data, USB3_CORE_CTRL1);
	imx_usb3_phy_init();

	tmp_data = readl(USB3_CORE_CTRL1);
	tmp_data &= ~0xfc000000; /* clear all sw_rst */
	writel(tmp_data, USB3_CORE_CTRL1);

	debug("wait xhci_power_on_ready\n");
	tmp_data = readl(USB3_CORE_STATUS);
	while (!(tmp_data & 0x1000))
		tmp_data = readl(USB3_CORE_STATUS);

	debug("xhci_power_on_ready\n");

	tmp_data = readl(USBSTS);
	debug("waiting CNR 0x%x\n", tmp_data);
	while (tmp_data & 0x800)
		tmp_data = readl(USBSTS);

	debug("check CNR has finished\n");
}

void imx8_xhci_reset(void)
{
	/* Set CORE ctrl to default value, that all rst are hold */
	writel(0xfc000001, USB3_CORE_CTRL1);
}


#ifdef CONFIG_DM_USB
static int xhci_imx8_probe(struct udevice *dev)
{
	struct xhci_hccr *hccr;
	struct xhci_hcor *hcor;

	int ret = 0;
	int len;

	/* Need to power on the PHY before access it */
#if CONFIG_IS_ENABLED(POWER_DOMAIN)
	struct udevice phy_dev;
	struct power_domain pd;
	const void *blob = gd->fdt_blob;
	int offset = dev_of_offset(dev), phy_off;

	phy_off = fdtdec_lookup_phandle(blob,
						offset,
						"fsl,usbphy");
	if (phy_off < 0)
		return -EINVAL;

	phy_dev.of_offset = phy_off;
	if (!power_domain_get(&phy_dev, &pd)) {
		if (power_domain_on(&pd))
			return -EINVAL;
	}
#endif

	ret = board_usb_init(dev->seq, USB_INIT_HOST);
	if (ret != 0) {
		printf("Failed to initialize board for USB\n");
		return ret;
	}

	init_clk_usb3(dev->seq);

	imx8_xhci_init();

	hccr = (struct xhci_hccr *)HCIVERSION_CAPLENGTH;
	len = HC_LENGTH(xhci_readl(&hccr->cr_capbase));
	hcor = (struct xhci_hcor *)((uintptr_t) hccr + len);

	printf("XHCI-imx8 init hccr 0x%p and hcor 0x%p hc_length %d\n",
	      (uint32_t *)hccr, (uint32_t *)hcor, len);

	return xhci_register(dev, hccr, hcor);
}

static int xhci_imx8_remove(struct udevice *dev)
{
	int ret = xhci_deregister(dev);
	if (!ret)
		imx8_xhci_reset();

	board_usb_cleanup(dev->seq, USB_INIT_HOST);

	return ret;
}

static const struct udevice_id xhci_usb_ids[] = {
	{ .compatible = "fsl,imx8-usb3", },
	{ }
};

U_BOOT_DRIVER(xhci_imx8) = {
	.name	= "xhci_imx8",
	.id	= UCLASS_USB,
	.of_match = xhci_usb_ids,
	.probe = xhci_imx8_probe,
	.remove = xhci_imx8_remove,
	.ops	= &xhci_usb_ops,
	.platdata_auto_alloc_size = sizeof(struct usb_platdata),
	.priv_auto_alloc_size = sizeof(struct xhci_ctrl),
	.flags	= DM_FLAG_ALLOC_PRIV_DMA,
};
#else
int xhci_hcd_init(int index, struct xhci_hccr **ret_hccr,
		  struct xhci_hcor **ret_hcor)
{
	struct xhci_hccr *hccr;
	struct xhci_hcor *hcor;
	int len, ret;

	ret = board_usb_init(index, USB_INIT_HOST);
	if (ret != 0) {
		printf("Failed to initialize board for USB\n");
		return ret;
	}

	init_clk_usb3(index);

	imx8_xhci_init();

	hccr = (struct xhci_hccr *)HCIVERSION_CAPLENGTH;
	len = HC_LENGTH(xhci_readl(&hccr->cr_capbase));
	hcor = (struct xhci_hcor *)((uintptr_t) hccr + len);

	printf("XHCI-imx8 init hccr 0x%p and hcor 0x%p hc_length %d\n",
	      (uint32_t *)hccr, (uint32_t *)hcor, len);

	*ret_hccr = hccr;
	*ret_hcor = hcor;

	return 0;
}

void xhci_hcd_stop(int index)
{
	imx8_xhci_reset();

	board_usb_cleanup(index, USB_INIT_HOST);
}
#endif
