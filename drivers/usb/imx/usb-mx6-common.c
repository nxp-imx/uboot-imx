// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2009 Daniel Mack <daniel@caiaq.de>
 * Copyright (C) 2010 Freescale Semiconductor, Inc.
 * Copyright 2017-2019 NXP
 *
 */

#include <common.h>
#include <usb.h>
#include <errno.h>
#include <wait_bit.h>
#include <linux/compiler.h>
#include <usb/ehci-ci.h>
#include <asm/io.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/clock.h>
#include <asm/mach-imx/iomux-v3.h>
#include <asm/mach-imx/regs-usbphy.h>
#include <asm/mach-imx/sys_proto.h>
#include <asm/mach-types.h>
#include <asm/arch/sys_proto.h>

DECLARE_GLOBAL_DATA_PTR;

#define USB_OTGREGS_OFFSET	0x000
#define USB_H1REGS_OFFSET	0x200
#define USB_H2REGS_OFFSET	0x400
#define USB_H3REGS_OFFSET	0x600
#define USB_OTHERREGS_OFFSET	0x800

#define USB_H1_CTRL_OFFSET	0x04

#define ANADIG_USB2_CHRG_DETECT_EN_B		0x00100000
#define ANADIG_USB2_CHRG_DETECT_CHK_CHRG_B	0x00080000

#define ANADIG_USB2_PLL_480_CTRL_BYPASS		0x00010000
#define ANADIG_USB2_PLL_480_CTRL_ENABLE		0x00002000
#define ANADIG_USB2_PLL_480_CTRL_POWER		0x00001000
#define ANADIG_USB2_PLL_480_CTRL_EN_USB_CLKS	0x00000040

#define USBNC_OFFSET		0x200
#define USBNC_PHYCFG2_ACAENB	(1 << 4) /* otg_id detection enable */
#define UCTRL_PWR_POL		(1 << 9) /* OTG Polarity of Power Pin */
#define UCTRL_OVER_CUR_POL	(1 << 8) /* OTG Polarity of Overcurrent */
#define UCTRL_OVER_CUR_DIS	(1 << 7) /* Disable OTG Overcurrent Detection */

#define PLL_USB_EN_USB_CLKS_MASK	(0x01 << 6)
#define PLL_USB_PWR_MASK		(0x01 << 12)
#define PLL_USB_ENABLE_MASK		(0x01 << 13)
#define PLL_USB_BYPASS_MASK		(0x01 << 16)
#define PLL_USB_REG_ENABLE_MASK		(0x01 << 21)
#define PLL_USB_DIV_SEL_MASK		(0x07 << 22)
#define PLL_USB_LOCK_MASK		(0x01 << 31)

/* USBCMD */
#define UCMD_RUN_STOP           (1 << 0) /* controller run/stop */
#define UCMD_RESET		(1 << 1) /* controller reset */

#if defined(CONFIG_MX6) || defined(CONFIG_MX7ULP) || defined(CONFIG_IMX8)
static const ulong phy_bases[] = {
	USB_PHY0_BASE_ADDR,
#if defined(USB_PHY1_BASE_ADDR)
	USB_PHY1_BASE_ADDR,
#endif
};

static void usb_internal_phy_clock_gate(int index, int on)
{
	void __iomem *phy_reg;

	if (index >= ARRAY_SIZE(phy_bases))
		return;

	phy_reg = (void __iomem *)phy_bases[index];
	phy_reg += on ? USBPHY_CTRL_CLR : USBPHY_CTRL_SET;
	writel(USBPHY_CTRL_CLKGATE, phy_reg);
}

static void usb_power_config(int index)
{
	if (index >= ARRAY_SIZE(phy_bases))
                return;

#if defined(CONFIG_MX7ULP)
	struct usbphy_regs __iomem *usbphy =
		(struct usbphy_regs __iomem *)USB_PHY0_BASE_ADDR;

	writel(ANADIG_USB2_CHRG_DETECT_EN_B |
		   ANADIG_USB2_CHRG_DETECT_CHK_CHRG_B,
		   &usbphy->usb1_chrg_detect);

	scg_enable_usb_pll(true);

#elif defined(CONFIG_IMX8)


	struct usbphy_regs __iomem *usbphy = (struct usbphy_regs __iomem *)phy_bases[index];

	int timeout = 1000000;

	writel(ANADIG_USB2_CHRG_DETECT_EN_B |
		   ANADIG_USB2_CHRG_DETECT_CHK_CHRG_B,
		   &usbphy->usb1_chrg_detect);

	if (!(readl(&usbphy->usb1_pll_480_ctrl) & PLL_USB_LOCK_MASK)) {

		/* Enable the regulator first */
		writel(PLL_USB_REG_ENABLE_MASK,
		       &usbphy->usb1_pll_480_ctrl_set);

		/* Wait at least 25us */
		udelay(25);

		/* Enable the power */
		writel(PLL_USB_PWR_MASK, &usbphy->usb1_pll_480_ctrl_set);

		/* Wait lock */
		while (timeout--) {
			if (readl(&usbphy->usb1_pll_480_ctrl) &
			    PLL_USB_LOCK_MASK)
				break;
			udelay(10);
		}

		if (timeout <= 0) {
			/* If timeout, we power down the pll */
			writel(PLL_USB_PWR_MASK,
			       &usbphy->usb1_pll_480_ctrl_clr);
			return;
		}
	}

	/* Clear the bypass */
	writel(PLL_USB_BYPASS_MASK, &usbphy->usb1_pll_480_ctrl_clr);

	/* Enable the PLL clock out to USB */
	writel((PLL_USB_EN_USB_CLKS_MASK | PLL_USB_ENABLE_MASK),
	       &usbphy->usb1_pll_480_ctrl_set);

#else
	struct anatop_regs __iomem *anatop =
		(struct anatop_regs __iomem *)ANATOP_BASE_ADDR;
	void __iomem *chrg_detect;
	void __iomem *pll_480_ctrl_clr;
	void __iomem *pll_480_ctrl_set;

	switch (index) {
	case 0:
		chrg_detect = &anatop->usb1_chrg_detect;
		pll_480_ctrl_clr = &anatop->usb1_pll_480_ctrl_clr;
		pll_480_ctrl_set = &anatop->usb1_pll_480_ctrl_set;
		break;
	case 1:
		chrg_detect = &anatop->usb2_chrg_detect;
		pll_480_ctrl_clr = &anatop->usb2_pll_480_ctrl_clr;
		pll_480_ctrl_set = &anatop->usb2_pll_480_ctrl_set;
		break;
	default:
		return;
	}
	/*
	 * Some phy and power's special controls
	 * 1. The external charger detector needs to be disabled
	 * or the signal at DP will be poor
	 * 2. The PLL's power and output to usb
	 * is totally controlled by IC, so the Software only needs
	 * to enable them at initializtion.
	 */
	writel(ANADIG_USB2_CHRG_DETECT_EN_B |
		     ANADIG_USB2_CHRG_DETECT_CHK_CHRG_B,
		     chrg_detect);

	writel(ANADIG_USB2_PLL_480_CTRL_BYPASS,
		     pll_480_ctrl_clr);

	writel(ANADIG_USB2_PLL_480_CTRL_ENABLE |
		     ANADIG_USB2_PLL_480_CTRL_POWER |
		     ANADIG_USB2_PLL_480_CTRL_EN_USB_CLKS,
		     pll_480_ctrl_set);

#endif
}

/* Return 0 : host node, <>0 : device mode */
static int usb_phy_enable(int index, struct usb_ehci *ehci)
{
	void __iomem *phy_reg;
	void __iomem *phy_ctrl;
	void __iomem *usb_cmd;
	int ret;

	if (index >= ARRAY_SIZE(phy_bases))
		return 0;

	phy_reg = (void __iomem *)phy_bases[index];
	phy_ctrl = (void __iomem *)(phy_reg + USBPHY_CTRL);
	usb_cmd = (void __iomem *)&ehci->usbcmd;

	/* Stop then Reset */
	clrbits_le32(usb_cmd, UCMD_RUN_STOP);
	ret = wait_for_bit_le32(usb_cmd, UCMD_RUN_STOP, false, 10000, false);
	if (ret)
		return ret;

	setbits_le32(usb_cmd, UCMD_RESET);
	ret = wait_for_bit_le32(usb_cmd, UCMD_RESET, false, 10000, false);
	if (ret)
		return ret;

	/* Reset USBPHY module */
	setbits_le32(phy_ctrl, USBPHY_CTRL_SFTRST);
	udelay(10);

	/* Remove CLKGATE and SFTRST */
	clrbits_le32(phy_ctrl, USBPHY_CTRL_CLKGATE | USBPHY_CTRL_SFTRST);
	udelay(10);

	/* Power up the PHY */
	writel(0, phy_reg + USBPHY_PWD);
	/* enable FS/LS device */
	setbits_le32(phy_ctrl, USBPHY_CTRL_ENUTMILEVEL2 |
			USBPHY_CTRL_ENUTMILEVEL3);

	return 0;
}

int usb_phy_mode(int port)
{
	void __iomem *phy_reg;
	void __iomem *phy_ctrl;
	u32 val;

	if (port >= ARRAY_SIZE(phy_bases))
		return USB_INIT_HOST;

	phy_reg = (void __iomem *)phy_bases[port];
	phy_ctrl = (void __iomem *)(phy_reg + USBPHY_CTRL);

	val = readl(phy_ctrl);

	if (val & USBPHY_CTRL_OTG_ID)
		return USB_INIT_DEVICE;
	else
		return USB_INIT_HOST;
}

#if defined(CONFIG_MX7ULP)
struct usbnc_regs {
	u32 ctrl1;
	u32 ctrl2;
	u32 reserve0[2];
	u32 hsic_ctrl;
};
#elif defined(CONFIG_IMX8)
struct usbnc_regs {
	u32 ctrl1;
	u32 ctrl2;
	u32 reserve1[10];
	u32 phy_cfg1;
	u32 phy_cfg2;
	u32 reserve2;
	u32 phy_status;
	u32 reserve3[4];
	u32 adp_cfg1;
	u32 adp_cfg2;
	u32 adp_status;
};
#else
/* Base address for this IP block is 0x02184800 */
struct usbnc_regs {
	u32	ctrl[4];	/* otg/host1-3 */
	u32	uh2_hsic_ctrl;
	u32	uh3_hsic_ctrl;
	u32	otg_phy_ctrl_0;
	u32	uh1_phy_ctrl_0;
};
#endif

#elif defined(CONFIG_USB_EHCI_MX7)
struct usbnc_regs {
	u32 ctrl1;
	u32 ctrl2;
	u32 reserve1[10];
	u32 phy_cfg1;
	u32 phy_cfg2;
	u32 reserve2;
	u32 phy_status;
	u32 reserve3[4];
	u32 adp_cfg1;
	u32 adp_cfg2;
	u32 adp_status;
};

static void usb_power_config(int index)
{
	struct usbnc_regs *usbnc = (struct usbnc_regs *)(ulong)(USB_BASE_ADDR +
			(0x10000 * index) + USBNC_OFFSET);
	void __iomem *phy_cfg2 = (void __iomem *)(&usbnc->phy_cfg2);

	/*
	 * Clear the ACAENB to enable usb_otg_id detection,
	 * otherwise it is the ACA detection enabled.
	 */
	clrbits_le32(phy_cfg2, USBNC_PHYCFG2_ACAENB);
}

int usb_phy_mode(int port)
{
	struct usbnc_regs *usbnc = (struct usbnc_regs *)(ulong)(USB_BASE_ADDR +
			(0x10000 * port) + USBNC_OFFSET);
	void __iomem *status = (void __iomem *)(&usbnc->phy_status);
	u32 val;

	val = readl(status);

	if (val & USBNC_PHYSTATUS_ID_DIG)
		return USB_INIT_DEVICE;
	else
		return USB_INIT_HOST;
}
#endif

static void usb_oc_config(int index)
{
#if defined(CONFIG_MX6)
	struct usbnc_regs *usbnc = (struct usbnc_regs *)(USB_BASE_ADDR +
			USB_OTHERREGS_OFFSET);
	void __iomem *ctrl = (void __iomem *)(&usbnc->ctrl[index]);
#elif defined(CONFIG_USB_EHCI_MX7) || defined(CONFIG_MX7ULP) || defined(CONFIG_IMX8)
	struct usbnc_regs *usbnc = (struct usbnc_regs *)(ulong)(USB_BASE_ADDR +
			(0x10000 * index) + USBNC_OFFSET);
	void __iomem *ctrl = (void __iomem *)(&usbnc->ctrl1);
#endif

#if CONFIG_MACH_TYPE == MACH_TYPE_MX6Q_ARM2
	/* mx6qarm2 seems to required a different setting*/
	clrbits_le32(ctrl, UCTRL_OVER_CUR_POL);
#else
	setbits_le32(ctrl, UCTRL_OVER_CUR_POL);
#endif

	setbits_le32(ctrl, UCTRL_OVER_CUR_DIS);

	/* Set power polarity to high active */
#ifdef CONFIG_MXC_USB_OTG_HACTIVE
	setbits_le32(ctrl, UCTRL_PWR_POL);
#else
	clrbits_le32(ctrl, UCTRL_PWR_POL);
#endif
}

/**
 * board_ehci_hcd_init - set usb vbus voltage
 * @port:      usb otg port
 *
 * Target board specific, setup iomux pad to setup supply vbus voltage
 * for usb otg port. Machine board file overrides board_ehci_hcd_init
 *
 * Return: 0 Success
 */
int __weak board_ehci_hcd_init(int port)
{
	return 0;
}

int ehci_mx6_common_init(struct usb_ehci *ehci, int index)
{
	int ret;
	u32 portsc;

	enable_usboh3_clk(1);
	mdelay(1);

	portsc = readl(&ehci->portsc);
	if (portsc & PORT_PTS_PHCD) {
		debug("suspended: portsc %x, enabled it.\n", portsc);
		clrbits_le32(&ehci->portsc, PORT_PTS_PHCD);
	}

	/* Do board specific initialization */
	ret = board_ehci_hcd_init(index);
	if (ret)
		return ret;

	usb_power_config(index);
	usb_oc_config(index);

#if defined(CONFIG_MX6) || defined(CONFIG_MX7ULP) || defined(CONFIG_IMX8)
	usb_internal_phy_clock_gate(index, 1);
	usb_phy_enable(index, ehci);
#endif

	return 0;
}
