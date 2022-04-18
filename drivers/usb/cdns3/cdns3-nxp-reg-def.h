/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2019 NXP
 */

#ifndef __DRIVERS_USB_CDNS3_NXP_H
#define __DRIVERS_USB_CDNS3_NXP_H

#define USB3_CORE_CTRL1    0x00
#define USB3_CORE_CTRL2    0x04
#define USB3_INT_REG       0x08
#define USB3_CORE_STATUS   0x0c
#define XHCI_DEBUG_LINK_ST 0x10
#define XHCI_DEBUG_BUS     0x14
#define USB3_SSPHY_CTRL1   0x40
#define USB3_SSPHY_CTRL2   0x44
#define USB3_SSPHY_STATUS  0x4c
#define USB2_PHY_CTRL1     0x50
#define USB2_PHY_CTRL2     0x54
#define USB2_PHY_STATUS    0x5c

/* Register bits definition */

/* USB3_CORE_CTRL1 */
#define SW_RESET_MASK	(0x3f << 26)
#define PWR_SW_RESET	BIT(31)
#define APB_SW_RESET	BIT(30)
#define AXI_SW_RESET	BIT(29)
#define RW_SW_RESET	BIT(28)
#define PHY_SW_RESET	BIT(27)
#define PHYAHB_SW_RESET	BIT(26)
#define ALL_SW_RESET	(PWR_SW_RESET | APB_SW_RESET | AXI_SW_RESET | \
		RW_SW_RESET | PHY_SW_RESET | PHYAHB_SW_RESET)
#define OC_DISABLE	BIT(9)
#define MDCTRL_CLK_SEL	BIT(7)
#define MODE_STRAP_MASK	(0x7)
#define DEV_MODE	BIT(2)
#define HOST_MODE	BIT(1)
#define OTG_MODE	BIT(0)

/* USB3_INT_REG */
#define CLK_125_REQ	BIT(29)
#define LPM_CLK_REQ	BIT(28)
#define DEVU3_WAEKUP_EN	BIT(14)
#define OTG_WAKEUP_EN	BIT(12)
#define DEV_INT_EN (3 << 8) /* DEV INT b9:8 */
#define HOST_INT1_EN BIT(0) /* HOST INT b7:0 */

/* USB3_CORE_STATUS */
#define MDCTRL_CLK_STATUS	BIT(15)
#define DEV_POWER_ON_READY	BIT(13)
#define HOST_POWER_ON_READY	BIT(12)

/* USB3_SSPHY_STATUS */
#define PHY_REFCLK_REQ		BIT(0)
#define CLK_VLD	0xf0000000

/* PHY register definition */
#define TB_ADDR_TX_RCVDETSC_CTRL	        (0x4124 * 4)
#define CDNS3_USB2_PHY_BASE			(0x38000)
#define USB2_PHY_AFE_BC_REG4			(CDNS3_USB2_PHY_BASE + 0x29 * 4)

/* USB2_PHY_AFE_BC_REG4 */
#define SET_FORCE_B_SESS_VALID			0x60

/* TB_ADDR_TX_RCVDETSC_CTRL */
#define RXDET_IN_P3_32KHZ			BIT(0)

/* OTG registers definition */
#define OTGSTS		0x4
#define OTGREFCLK	0xc

/* Register bits definition */
/* OTGSTS */
#define OTG_NRDY	BIT(11)
/* OTGREFCLK */
#define OTG_STB_CLK_SWITCH_EN	BIT(31)

/* xHCI registers definition  */
#define XECP_PORT_CAP_REG	0x8000
#define XECP_PM_PMCSR		0x8018
#define XECP_AUX_CTRL_REG1	0x8120

/* Register bits definition */
/* XECP_PORT_CAP_REG */
#define LPM_2_STB_SWITCH_EN	BIT(25)

/* XECP_AUX_CTRL_REG1 */
#define CFG_RXDET_P3_EN		BIT(15)

/* XECP_PM_PMCSR */
#define PS_D0			BIT(0)
#endif /* __DRIVERS_USB_CDNS3_NXP_H */
