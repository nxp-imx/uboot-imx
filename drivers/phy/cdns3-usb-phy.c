// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019 NXP
 *
 * Cadence3 USB PHY driver
 *
 * Author: Sherry Sun <sherry.sun@nxp.com>
 */

#include <common.h>
#include <clk.h>
#include <dm.h>
#include <generic-phy.h>
#include <asm/io.h>

/* PHY registers */
#define PHY_PMA_CMN_CTRL1			(0xC800 * 4)
#define TB_ADDR_CMN_DIAG_HSCLK_SEL		(0x01e0 * 4)
#define TB_ADDR_CMN_PLL0_VCOCAL_INIT_TMR	(0x0084 * 4)
#define TB_ADDR_CMN_PLL0_VCOCAL_ITER_TMR	(0x0085 * 4)
#define TB_ADDR_CMN_PLL0_INTDIV	                (0x0094 * 4)
#define TB_ADDR_CMN_PLL0_FRACDIV		(0x0095 * 4)
#define TB_ADDR_CMN_PLL0_HIGH_THR		(0x0096 * 4)
#define TB_ADDR_CMN_PLL0_SS_CTRL1		(0x0098 * 4)
#define TB_ADDR_CMN_PLL0_SS_CTRL2		(0x0099 * 4)
#define TB_ADDR_CMN_PLL0_DSM_DIAG		(0x0097 * 4)
#define TB_ADDR_CMN_DIAG_PLL0_OVRD		(0x01c2 * 4)
#define TB_ADDR_CMN_DIAG_PLL0_FBH_OVRD		(0x01c0 * 4)
#define TB_ADDR_CMN_DIAG_PLL0_FBL_OVRD		(0x01c1 * 4)
#define TB_ADDR_CMN_DIAG_PLL0_V2I_TUNE          (0x01C5 * 4)
#define TB_ADDR_CMN_DIAG_PLL0_CP_TUNE           (0x01C6 * 4)
#define TB_ADDR_CMN_DIAG_PLL0_LF_PROG           (0x01C7 * 4)
#define TB_ADDR_CMN_DIAG_PLL0_TEST_MODE		(0x01c4 * 4)
#define TB_ADDR_CMN_PSM_CLK_CTRL		(0x0061 * 4)
#define TB_ADDR_XCVR_DIAG_RX_LANE_CAL_RST_TMR	(0x40ea * 4)
#define TB_ADDR_XCVR_PSM_RCTRL	                (0x4001 * 4)
#define TB_ADDR_TX_PSC_A0		        (0x4100 * 4)
#define TB_ADDR_TX_PSC_A1		        (0x4101 * 4)
#define TB_ADDR_TX_PSC_A2		        (0x4102 * 4)
#define TB_ADDR_TX_PSC_A3		        (0x4103 * 4)
#define TB_ADDR_TX_DIAG_ECTRL_OVRD		(0x41f5 * 4)
#define TB_ADDR_TX_PSC_CAL		        (0x4106 * 4)
#define TB_ADDR_TX_PSC_RDY		        (0x4107 * 4)
#define TB_ADDR_RX_PSC_A0	                (0x8000 * 4)
#define TB_ADDR_RX_PSC_A1	                (0x8001 * 4)
#define TB_ADDR_RX_PSC_A2	                (0x8002 * 4)
#define TB_ADDR_RX_PSC_A3	                (0x8003 * 4)
#define TB_ADDR_RX_PSC_CAL	                (0x8006 * 4)
#define TB_ADDR_RX_PSC_RDY	                (0x8007 * 4)
#define TB_ADDR_TX_TXCC_MGNLS_MULT_000		(0x4058 * 4)
#define TB_ADDR_TX_DIAG_BGREF_PREDRV_DELAY	(0x41e7 * 4)
#define TB_ADDR_RX_SLC_CU_ITER_TMR		(0x80e3 * 4)
#define TB_ADDR_RX_SIGDET_HL_FILT_TMR		(0x8090 * 4)
#define TB_ADDR_RX_SAMP_DAC_CTRL		(0x8058 * 4)
#define TB_ADDR_RX_DIAG_SIGDET_TUNE		(0x81dc * 4)
#define TB_ADDR_RX_DIAG_LFPSDET_TUNE2		(0x81df * 4)
#define TB_ADDR_RX_DIAG_BS_TM	                (0x81f5 * 4)
#define TB_ADDR_RX_DIAG_DFE_CTRL1		(0x81d3 * 4)
#define TB_ADDR_RX_DIAG_ILL_IQE_TRIM4		(0x81c7 * 4)
#define TB_ADDR_RX_DIAG_ILL_E_TRIM0		(0x81c2 * 4)
#define TB_ADDR_RX_DIAG_ILL_IQ_TRIM0		(0x81c1 * 4)
#define TB_ADDR_RX_DIAG_ILL_IQE_TRIM6		(0x81c9 * 4)
#define TB_ADDR_RX_DIAG_RXFE_TM3		(0x81f8 * 4)
#define TB_ADDR_RX_DIAG_RXFE_TM4		(0x81f9 * 4)
#define TB_ADDR_RX_DIAG_LFPSDET_TUNE		(0x81dd * 4)
#define TB_ADDR_RX_DIAG_DFE_CTRL3		(0x81d5 * 4)
#define TB_ADDR_RX_DIAG_SC2C_DELAY		(0x81e1 * 4)
#define TB_ADDR_RX_REE_VGA_GAIN_NODFE		(0x81bf * 4)
#define TB_ADDR_XCVR_PSM_CAL_TMR		(0x4002 * 4)
#define TB_ADDR_XCVR_PSM_A0BYP_TMR		(0x4004 * 4)
#define TB_ADDR_XCVR_PSM_A0IN_TMR		(0x4003 * 4)
#define TB_ADDR_XCVR_PSM_A1IN_TMR		(0x4005 * 4)
#define TB_ADDR_XCVR_PSM_A2IN_TMR		(0x4006 * 4)
#define TB_ADDR_XCVR_PSM_A3IN_TMR		(0x4007 * 4)
#define TB_ADDR_XCVR_PSM_A4IN_TMR		(0x4008 * 4)
#define TB_ADDR_XCVR_PSM_A5IN_TMR		(0x4009 * 4)
#define TB_ADDR_XCVR_PSM_A0OUT_TMR		(0x400a * 4)
#define TB_ADDR_XCVR_PSM_A1OUT_TMR		(0x400b * 4)
#define TB_ADDR_XCVR_PSM_A2OUT_TMR		(0x400c * 4)
#define TB_ADDR_XCVR_PSM_A3OUT_TMR		(0x400d * 4)
#define TB_ADDR_XCVR_PSM_A4OUT_TMR		(0x400e * 4)
#define TB_ADDR_XCVR_PSM_A5OUT_TMR		(0x400f * 4)
#define TB_ADDR_TX_RCVDET_EN_TMR	        (0x4122 * 4)
#define TB_ADDR_TX_RCVDET_ST_TMR	        (0x4123 * 4)
#define TB_ADDR_XCVR_DIAG_LANE_FCM_EN_MGN_TMR	(0x40f2 * 4)

struct cdns3_usb_phy {
	struct clk phy_clk;
	void __iomem *phy_regs;
};

static int cdns3_usb_phy_init(struct phy *phy)
{
	struct udevice *dev = phy->dev;
	struct cdns3_usb_phy *priv = dev_get_priv(dev);
	void __iomem *regs = priv->phy_regs;

	writel(0x0830, regs + PHY_PMA_CMN_CTRL1);
	writel(0x10, regs + TB_ADDR_CMN_DIAG_HSCLK_SEL);
	writel(0x00F0, regs + TB_ADDR_CMN_PLL0_VCOCAL_INIT_TMR);
	writel(0x0018, regs + TB_ADDR_CMN_PLL0_VCOCAL_ITER_TMR);
	writel(0x00D0, regs + TB_ADDR_CMN_PLL0_INTDIV);
	writel(0x4aaa, regs + TB_ADDR_CMN_PLL0_FRACDIV);
	writel(0x0034, regs + TB_ADDR_CMN_PLL0_HIGH_THR);
	writel(0x1ee, regs + TB_ADDR_CMN_PLL0_SS_CTRL1);
	writel(0x7F03, regs + TB_ADDR_CMN_PLL0_SS_CTRL2);
	writel(0x0020, regs + TB_ADDR_CMN_PLL0_DSM_DIAG);
	writel(0x0000, regs + TB_ADDR_CMN_DIAG_PLL0_OVRD);
	writel(0x0000, regs + TB_ADDR_CMN_DIAG_PLL0_FBH_OVRD);
	writel(0x0000, regs + TB_ADDR_CMN_DIAG_PLL0_FBL_OVRD);
	writel(0x0007, regs + TB_ADDR_CMN_DIAG_PLL0_V2I_TUNE);
	writel(0x0027, regs + TB_ADDR_CMN_DIAG_PLL0_CP_TUNE);
	writel(0x0008, regs + TB_ADDR_CMN_DIAG_PLL0_LF_PROG);
	writel(0x0022, regs + TB_ADDR_CMN_DIAG_PLL0_TEST_MODE);
	writel(0x000a, regs + TB_ADDR_CMN_PSM_CLK_CTRL);
	writel(0x139, regs + TB_ADDR_XCVR_DIAG_RX_LANE_CAL_RST_TMR);
	writel(0xbefc, regs + TB_ADDR_XCVR_PSM_RCTRL);

	writel(0x7799, regs + TB_ADDR_TX_PSC_A0);
	writel(0x7798, regs + TB_ADDR_TX_PSC_A1);
	writel(0x509b, regs + TB_ADDR_TX_PSC_A2);
	writel(0x3, regs + TB_ADDR_TX_DIAG_ECTRL_OVRD);
	writel(0x509b, regs + TB_ADDR_TX_PSC_A3);
	writel(0x2090, regs + TB_ADDR_TX_PSC_CAL);
	writel(0x2090, regs + TB_ADDR_TX_PSC_RDY);

	writel(0xA6FD, regs + TB_ADDR_RX_PSC_A0);
	writel(0xA6FD, regs + TB_ADDR_RX_PSC_A1);
	writel(0xA410, regs + TB_ADDR_RX_PSC_A2);
	writel(0x2410, regs + TB_ADDR_RX_PSC_A3);

	writel(0x23FF, regs + TB_ADDR_RX_PSC_CAL);
	writel(0x2010, regs + TB_ADDR_RX_PSC_RDY);

	writel(0x0020, regs + TB_ADDR_TX_TXCC_MGNLS_MULT_000);
	writel(0x00ff, regs + TB_ADDR_TX_DIAG_BGREF_PREDRV_DELAY);
	writel(0x0002, regs + TB_ADDR_RX_SLC_CU_ITER_TMR);
	writel(0x0013, regs + TB_ADDR_RX_SIGDET_HL_FILT_TMR);
	writel(0x0000, regs + TB_ADDR_RX_SAMP_DAC_CTRL);
	writel(0x1004, regs + TB_ADDR_RX_DIAG_SIGDET_TUNE);
	writel(0x4041, regs + TB_ADDR_RX_DIAG_LFPSDET_TUNE2);
	writel(0x0480, regs + TB_ADDR_RX_DIAG_BS_TM);
	writel(0x8006, regs + TB_ADDR_RX_DIAG_DFE_CTRL1);
	writel(0x003f, regs + TB_ADDR_RX_DIAG_ILL_IQE_TRIM4);
	writel(0x543f, regs + TB_ADDR_RX_DIAG_ILL_E_TRIM0);
	writel(0x543f, regs + TB_ADDR_RX_DIAG_ILL_IQ_TRIM0);
	writel(0x0000, regs + TB_ADDR_RX_DIAG_ILL_IQE_TRIM6);
	writel(0x8000, regs + TB_ADDR_RX_DIAG_RXFE_TM3);
	writel(0x0003, regs + TB_ADDR_RX_DIAG_RXFE_TM4);
	writel(0x2408, regs + TB_ADDR_RX_DIAG_LFPSDET_TUNE);
	writel(0x05ca, regs + TB_ADDR_RX_DIAG_DFE_CTRL3);
	writel(0x0258, regs + TB_ADDR_RX_DIAG_SC2C_DELAY);
	writel(0x1fff, regs + TB_ADDR_RX_REE_VGA_GAIN_NODFE);

	writel(0x02c6, regs + TB_ADDR_XCVR_PSM_CAL_TMR);
	writel(0x0002, regs + TB_ADDR_XCVR_PSM_A0BYP_TMR);
	writel(0x02c6, regs + TB_ADDR_XCVR_PSM_A0IN_TMR);
	writel(0x0010, regs + TB_ADDR_XCVR_PSM_A1IN_TMR);
	writel(0x0010, regs + TB_ADDR_XCVR_PSM_A2IN_TMR);
	writel(0x0010, regs + TB_ADDR_XCVR_PSM_A3IN_TMR);
	writel(0x0010, regs + TB_ADDR_XCVR_PSM_A4IN_TMR);
	writel(0x0010, regs + TB_ADDR_XCVR_PSM_A5IN_TMR);

	writel(0x0002, regs + TB_ADDR_XCVR_PSM_A0OUT_TMR);
	writel(0x0002, regs + TB_ADDR_XCVR_PSM_A1OUT_TMR);
	writel(0x0002, regs + TB_ADDR_XCVR_PSM_A2OUT_TMR);
	writel(0x0002, regs + TB_ADDR_XCVR_PSM_A3OUT_TMR);
	writel(0x0002, regs + TB_ADDR_XCVR_PSM_A4OUT_TMR);
	writel(0x0002, regs + TB_ADDR_XCVR_PSM_A5OUT_TMR);

	/* Change rx detect parameter */
	writel(0x960, regs + TB_ADDR_TX_RCVDET_EN_TMR);
	writel(0x01e0, regs + TB_ADDR_TX_RCVDET_ST_TMR);
	writel(0x0090, regs + TB_ADDR_XCVR_DIAG_LANE_FCM_EN_MGN_TMR);

	udelay(10);
	return 0;
}

struct phy_ops cdns3_usb_phy_ops = {
	.init = cdns3_usb_phy_init,
};

static int cdns3_usb_phy_remove(struct udevice *dev)
{
#if CONFIG_IS_ENABLED(CLK)
	struct cdns3_usb_phy *priv = dev_get_priv(dev);
	int ret;

	if (priv->phy_clk.dev) {
		ret = clk_disable(&priv->phy_clk);
		if (ret)
			return ret;

		ret = clk_free(&priv->phy_clk);
		if (ret)
			return ret;
	}
#endif

	return 0;
}

static int cdns3_usb_phy_probe(struct udevice *dev)
{
	struct cdns3_usb_phy *priv = dev_get_priv(dev);

#if CONFIG_IS_ENABLED(CLK)
	int ret;

	ret = clk_get_by_name(dev, "main_clk", &priv->phy_clk);
	if (ret) {
		printf("Failed to get phy_clk\n");
		return ret;
	}

	ret = clk_enable(&priv->phy_clk);
	if (ret) {
		printf("Failed to enable phy_clk\n");
		return ret;
	}
#endif
	priv->phy_regs = (void *__iomem)devfdt_get_addr(dev);

	return 0;
}

static const struct udevice_id cdns3_usb_phy_ids[] = {
	{ .compatible = "cdns,usb3-phy" },
	{ }
};

U_BOOT_DRIVER(cdns3_usb_phy) = {
	.name = "cdns3_usb_phy",
	.id = UCLASS_PHY,
	.of_match = cdns3_usb_phy_ids,
	.probe = cdns3_usb_phy_probe,
	.remove = cdns3_usb_phy_remove,
	.ops = &cdns3_usb_phy_ops,
	.priv_auto_alloc_size = sizeof(struct cdns3_usb_phy),
};
