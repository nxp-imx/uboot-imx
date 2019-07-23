// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2009 Daniel Mack <daniel@caiaq.de>
 * Copyright (C) 2010 Freescale Semiconductor, Inc.
 * Copyright 2017 NXP
 *
 */

#include <common.h>
#include <clk.h>
#include <log.h>
#include <usb.h>
#include <errno.h>
#include <wait_bit.h>
#include <asm/global_data.h>
#include <linux/compiler.h>
#include <linux/delay.h>
#include <usb/ehci-ci.h>
#include <asm/io.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/clock.h>
#include <asm/mach-imx/iomux-v3.h>
#include <asm/mach-imx/regs-usbphy.h>
#include <asm/mach-imx/sys_proto.h>
#include <dm.h>
#include <asm/mach-types.h>
#include <power/regulator.h>
#include <linux/usb/otg.h>
#include <linux/usb/phy.h>

#include "ehci.h"
#if CONFIG_IS_ENABLED(POWER_DOMAIN)
#include <power-domain.h>
#endif
#include <clk.h>

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

/* If this is not defined, assume MX6/MX7/MX8M SoC default */
#ifndef CONFIG_MXC_USB_PORTSC
#define CONFIG_MXC_USB_PORTSC	(PORT_PTS_UTMI | PORT_PTS_PTW)
#endif

/* Base address for this IP block is 0x02184800 */
struct usbnc_regs {
	u32 ctrl[4]; /* otg/host1-3 */
	u32 uh2_hsic_ctrl;
	u32 uh3_hsic_ctrl;
	u32 otg_phy_ctrl_0;
	u32 uh1_phy_ctrl_0;
	u32 reserve1[4];
	u32 phy_cfg1;
	u32 phy_cfg2;
	u32 reserve2;
	u32 phy_status;
	u32 reserve3[4];
	u32 adp_cfg1;
	u32 adp_cfg2;
	u32 adp_status;
};

struct ehci_mx6_phy_data {
	void __iomem *phy_addr;
	void __iomem *misc_addr;
	void __iomem *anatop_addr;
};

#if defined(CONFIG_MX6) && !CONFIG_IS_ENABLED(PHY)
static void usb_power_config_mx6(void __iomem *anatop_base,
				 int anatop_bits_index)
{
	struct anatop_regs __iomem *anatop = (struct anatop_regs __iomem *)anatop_base;
	void __iomem *chrg_detect;
	void __iomem *pll_480_ctrl_clr;
	void __iomem *pll_480_ctrl_set;

	if (!is_mx6())
		return;

	switch (anatop_bits_index) {
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
}
#else
static void __maybe_unused
usb_power_config_mx6(void __iomem *anatop_base, int anatop_bits_index) { }
#endif

#if defined(CONFIG_USB_EHCI_MX7) && !CONFIG_IS_ENABLED(PHY)
static void usb_power_config_mx7(void __iomem *usbnc_base)
{
	struct usbnc_regs __iomem *usbnc = (struct usbnc_regs __iomem *)usbnc_base;
	void __iomem *phy_cfg2 = (void __iomem *)(&usbnc->phy_cfg2);

	if (!is_mx7())
		return;

	/*
	 * Clear the ACAENB to enable usb_otg_id detection,
	 * otherwise it is the ACA detection enabled.
	 */
	clrbits_le32(phy_cfg2, USBNC_PHYCFG2_ACAENB);
}
#else
static void __maybe_unused
usb_power_config_mx7(void __iomem *usbnc_base) { }
#endif

#if defined(CONFIG_MX7ULP) && !CONFIG_IS_ENABLED(PHY)
static void usb_power_config_mx7ulp(void __iomem *usbphy_base)
{
	struct usbphy_regs __iomem *usbphy = (struct usbphy_regs __iomem *)usbphy_base;

	if (!is_mx7ulp())
		return;

	writel(ANADIG_USB2_CHRG_DETECT_EN_B |
	       ANADIG_USB2_CHRG_DETECT_CHK_CHRG_B,
	       &usbphy->usb1_chrg_detect);

	scg_enable_usb_pll(true);
}
#else
static void __maybe_unused
usb_power_config_mx7ulp(void __iomem *usbphy_base) { }
#endif

#if defined(CONFIG_IMX8)
static void usb_power_config_imx8(void __iomem *usbphy_base)
{
	struct usbphy_regs __iomem *usbphy = (struct usbphy_regs __iomem *)usbphy_base;

	if (!is_imx8())
		return;

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
}
#else
static void __maybe_unused
usb_power_config_imx8(void __iomem *usbphy_base) { }
#endif


#if defined(CONFIG_MX6) || defined(CONFIG_MX7ULP) || defined(CONFIG_IMXRT) || defined(CONFIG_IMX8)
static const ulong phy_bases[] = {
	USB_PHY0_BASE_ADDR,
#if defined(USB_PHY1_BASE_ADDR)
	USB_PHY1_BASE_ADDR,
#endif
};

#if !CONFIG_IS_ENABLED(PHY)
static void usb_internal_phy_clock_gate(void __iomem *phy_reg, int on)
{
	if (phy_reg == NULL)
		return;

	phy_reg += on ? USBPHY_CTRL_CLR : USBPHY_CTRL_SET;
	writel(USBPHY_CTRL_CLKGATE, phy_reg);
}

/* Return 0 : host node, <>0 : device mode */
static int usb_phy_enable(struct usb_ehci *ehci, void __iomem *phy_reg)
{
	void __iomem *phy_ctrl;
	void __iomem *usb_cmd;
	int ret;

	if (phy_reg == NULL)
		return -EINVAL;

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
#endif

int usb_phy_mode(int port)
{
	void __iomem *phy_reg;
	void __iomem *phy_ctrl;
	u32 val;

	phy_reg = (void __iomem *)phy_bases[port];
	phy_ctrl = (void __iomem *)(phy_reg + USBPHY_CTRL);

	val = readl(phy_ctrl);

	if (val & USBPHY_CTRL_OTG_ID)
		return USB_INIT_DEVICE;
	else
		return USB_INIT_HOST;
}

#elif defined(CONFIG_USB_EHCI_MX7)
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

static void ehci_mx6_powerup_fixup(struct ehci_ctrl *ctrl, uint32_t *status_reg,
				   uint32_t *reg)
{
	uint32_t result;
	int usec = 2000;

	mdelay(50);

	do {
		result = ehci_readl(status_reg);
		udelay(5);
		if (!(result & EHCI_PS_PR))
			break;
		usec--;
	} while (usec > 0);

	*reg = ehci_readl(status_reg);
}

/* Should be done in the MXS PHY driver */
static void usb_oc_config(void __iomem *usbnc_base, int index)
{
	struct usbnc_regs __iomem *usbnc = (struct usbnc_regs __iomem *)usbnc_base;
#if defined(CONFIG_MX6)
	void __iomem *ctrl = (void __iomem *)(&usbnc->ctrl[index]);
#elif defined(CONFIG_USB_EHCI_MX7) || defined(CONFIG_MX7ULP) || defined(CONFIG_IMX8)
	void __iomem *ctrl = (void __iomem *)(&usbnc->ctrl[0]);
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

void ehci_mx6_phy_init(struct usb_ehci *ehci, struct ehci_mx6_phy_data *phy_data, int index)
{
	u32 portsc;

	portsc = readl(&ehci->portsc);
	if (portsc & PORT_PTS_PHCD) {
		debug("suspended: portsc %x, enabled it.\n", portsc);
		clrbits_le32(&ehci->portsc, PORT_PTS_PHCD);
	}

	usb_power_config_mx6(phy_data->anatop_addr, index);
	usb_power_config_mx7(phy_data->misc_addr);
	usb_power_config_mx7ulp(phy_data->phy_addr);
	usb_power_config_imx8(phy_data->phy_addr);

	usb_oc_config(phy_data->misc_addr, index);

#if defined(CONFIG_MX6) || defined(CONFIG_MX7ULP) || defined(CONFIG_IMXRT) || defined(CONFIG_IMX8)
	usb_internal_phy_clock_gate(phy_data->phy_addr, 1);
	usb_phy_enable(ehci, phy_data->phy_addr);
#endif
}

#if !CONFIG_IS_ENABLED(DM_USB)
/**
 * board_usb_phy_mode - override usb phy mode
 * @port:	usb host/otg port
 *
 * Target board specific, override usb_phy_mode.
 * When usb-otg is used as usb host port, iomux pad usb_otg_id can be
 * left disconnected in this case usb_phy_mode will not be able to identify
 * the phy mode that usb port is used.
 * Machine file overrides board_usb_phy_mode.
 *
 * Return: USB_INIT_DEVICE or USB_INIT_HOST
 */
int __weak board_usb_phy_mode(int port)
{
	return usb_phy_mode(port);
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

/**
 * board_ehci_power - enables/disables usb vbus voltage
 * @port:      usb otg port
 * @on:        on/off vbus voltage
 *
 * Enables/disables supply vbus voltage for usb otg port.
 * Machine board file overrides board_ehci_power
 *
 * Return: 0 Success
 */
int __weak board_ehci_power(int port, int on)
{
	return 0;
}

static const struct ehci_ops mx6_ehci_ops = {
	.powerup_fixup		= ehci_mx6_powerup_fixup,
};

int ehci_hcd_init(int index, enum usb_init_type init,
		struct ehci_hccr **hccr, struct ehci_hcor **hcor)
{
	enum usb_init_type type;
	struct ehci_mx6_phy_data phy_data;
	memset(&phy_data, 0, sizeof(phy_data));

#if defined(CONFIG_MX6) || defined(CONFIG_IMXRT)
	if (index > 3)
			return -EINVAL;

	u32 controller_spacing = 0x200;
	phy_data.anatop_addr = (void __iomem *)ANATOP_BASE_ADDR;
	phy_data.misc_addr = (void __iomem *)(USB_BASE_ADDR +
			USB_OTHERREGS_OFFSET);
	if (index < ARRAY_SIZE(phy_bases))
		phy_data.phy_addr = (void __iomem *)(ulong)phy_bases[index];

#elif defined(CONFIG_USB_EHCI_MX7)
	if (index > 1)
		return -EINVAL;

	u32 controller_spacing = 0x10000;
	phy_data.misc_addr = (void __iomem *)(ulong)(USB_BASE_ADDR +
			(0x10000 * index) + USBNC_OFFSET);
#elif defined(CONFIG_MX7ULP) || defined(CONFIG_IMX8)
	if (index >= ARRAY_SIZE(phy_bases))
		return -EINVAL;

	u32 controller_spacing = 0x10000;
	phy_data.phy_addr = (void __iomem *)(ulong)phy_bases[index];
	phy_data.misc_addr = (void __iomem *)(ulong)(USB_BASE_ADDR +
			(0x10000 * index) + USBNC_OFFSET);
#endif
	struct usb_ehci *ehci = (struct usb_ehci *)(ulong)(USB_BASE_ADDR +
		(controller_spacing * index));
	int ret;

	if (CONFIG_IS_ENABLED(IMX_MODULE_FUSE)) {
		if (usb_fused((ulong)ehci)) {
			printf("SoC fuse indicates USB@0x%lx is unavailable.\n",
			       (ulong)ehci);
			return	-ENODEV;
		}
	}

	enable_usboh3_clk(1);
	mdelay(1);

	/* Do board specific initialization */
	ret = board_ehci_hcd_init(index);
	if (ret) {
		enable_usboh3_clk(0);
		return ret;
	}

	ehci_mx6_phy_init(ehci, &phy_data, index);

	ehci_set_controller_priv(index, NULL, &mx6_ehci_ops);

	type = board_usb_phy_mode(index);

	if (hccr && hcor) {
		*hccr = (struct ehci_hccr *)((uintptr_t)&ehci->caplength);
		*hcor = (struct ehci_hcor *)((uintptr_t)*hccr +
				HC_LENGTH(ehci_readl(&(*hccr)->cr_capbase)));
	}

	if ((type == init) || (type == USB_INIT_DEVICE))
		board_ehci_power(index, (type == USB_INIT_DEVICE) ? 0 : 1);
	if (type != init)
		return -ENODEV;

	if (is_mx6dqp() || is_mx6dq() || is_mx6sdl() ||
		((is_mx6sl() || is_mx6sx()) && type == USB_INIT_HOST))
		setbits_le32(&ehci->usbmode, SDIS);

	if (type == USB_INIT_DEVICE)
		return 0;

	setbits_le32(&ehci->usbmode, CM_HOST);
	writel(CONFIG_MXC_USB_PORTSC, &ehci->portsc);
	setbits_le32(&ehci->portsc, USB_EN);

	mdelay(10);

	return 0;
}

int ehci_hcd_stop(int index)
{
	return 0;
}
#else
struct ehci_mx6_priv_data {
	struct ehci_ctrl ctrl;
	struct usb_ehci *ehci;
	struct udevice *vbus_supply;
	struct clk clk;
	struct phy phy;
	enum usb_init_type init_type;
	enum usb_phy_interface phy_type;
	int portnr;
	int phy_node_off;
#if !CONFIG_IS_ENABLED(PHY) || defined(CONFIG_IMX8)
	struct udevice phy_dev;
	struct ehci_mx6_phy_data phy_data;
#if CONFIG_IS_ENABLED(POWER_DOMAIN)
	struct power_domain phy_pd;
#endif
#if CONFIG_IS_ENABLED(CLK)
	struct clk phy_clk;
#endif
#endif
};

static u32 mx6_portsc(enum usb_phy_interface phy_type)
{
	switch (phy_type) {
	case USBPHY_INTERFACE_MODE_UTMI:
		return PORT_PTS_UTMI;
	case USBPHY_INTERFACE_MODE_UTMIW:
		return PORT_PTS_UTMI | PORT_PTS_PTW;
	case USBPHY_INTERFACE_MODE_ULPI:
		return PORT_PTS_ULPI;
	case USBPHY_INTERFACE_MODE_SERIAL:
		return PORT_PTS_SERIAL;
	case USBPHY_INTERFACE_MODE_HSIC:
		return PORT_PTS_HSIC;
	default:
		return CONFIG_MXC_USB_PORTSC;
	}
}

static int mx6_init_after_reset(struct ehci_ctrl *dev)
{
	struct ehci_mx6_priv_data *priv = dev->priv;
	enum usb_init_type type = priv->init_type;
	struct usb_ehci *ehci = priv->ehci;
	int ret;

	ret = board_usb_init(priv->portnr, priv->init_type);
	if (ret) {
		printf("Failed to initialize board for USB\n");
		return ret;
	}

#if !CONFIG_IS_ENABLED(PHY) || defined(CONFIG_IMX8)
	ehci_mx6_phy_init(ehci, &priv->phy_data, priv->portnr);
#endif

#if CONFIG_IS_ENABLED(DM_REGULATOR)
	if (priv->vbus_supply) {
		int ret;
		ret = regulator_set_enable(priv->vbus_supply,
					   (type == USB_INIT_DEVICE) ?
					   false : true);
		if (ret && ret != -ENOSYS) {
			printf("Error enabling VBUS supply (ret=%i)\n", ret);
			return ret;
		}
	}
#endif

	if (is_mx6dqp() || is_mx6dq() || is_mx6sdl() ||
		((is_mx6sl() || is_mx6sx()) && type == USB_INIT_HOST))
		setbits_le32(&ehci->usbmode, SDIS);

	if (type == USB_INIT_DEVICE)
		return 0;

	setbits_le32(&ehci->usbmode, CM_HOST);
	writel(mx6_portsc(priv->phy_type), &ehci->portsc);
	setbits_le32(&ehci->portsc, USB_EN);

	mdelay(10);

	return 0;
}

static const struct ehci_ops mx6_ehci_ops = {
	.powerup_fixup		= ehci_mx6_powerup_fixup,
	.init_after_reset 	= mx6_init_after_reset
};

/**
 * board_ehci_usb_phy_mode - override usb phy mode
 * @port:	usb host/otg port
 *
 * Target board specific, override usb_phy_mode.
 * When usb-otg is used as usb host port, iomux pad usb_otg_id can be
 * left disconnected in this case usb_phy_mode will not be able to identify
 * the phy mode that usb port is used.
 * Machine file overrides board_usb_phy_mode.
 * When the extcon property is set in DTB, machine must provide this function, otherwise
 * it will default return HOST.
 *
 * Return: USB_INIT_DEVICE or USB_INIT_HOST
 */
int __weak board_ehci_usb_phy_mode(struct udevice *dev)
{
	return USB_INIT_HOST;
}

static int ehci_usb_phy_mode(struct udevice *dev)
{
	struct ehci_mx6_priv_data *priv = dev_get_priv(dev);
	void *__iomem addr = (void *__iomem)devfdt_get_addr(dev);
	void *__iomem phy_ctrl, *__iomem phy_status;
	const void *blob = gd->fdt_blob;
	u32 val;

	/*
	 * About fsl,usbphy, Refer to
	 * Documentation/devicetree/bindings/usb/ci-hdrc-usb2.txt.
	 */
	if (is_mx6() || is_mx7ulp() || is_imxrt() || is_imx8()) {
		addr = (void __iomem *)fdtdec_get_addr(blob, priv->phy_node_off,
						       "reg");
		if ((fdt_addr_t)addr == FDT_ADDR_T_NONE)
			return -EINVAL;

		phy_ctrl = (void __iomem *)(addr + USBPHY_CTRL);
		val = readl(phy_ctrl);

		if (val & USBPHY_CTRL_OTG_ID)
			priv->init_type = USB_INIT_DEVICE;
		else
			priv->init_type = USB_INIT_HOST;
	} else if (is_mx7() || is_imx8mm() || is_imx8mn()) {
		phy_status = (void __iomem *)(addr +
					      USBNC_PHY_STATUS_OFFSET);
		val = readl(phy_status);

		if (val & USBNC_PHYSTATUS_ID_DIG)
			priv->init_type = USB_INIT_DEVICE;
		else
			priv->init_type = USB_INIT_HOST;
	} else {
		return -EINVAL;
	}

	return 0;
}

static int ehci_usb_of_to_plat(struct udevice *dev)
{
	struct usb_plat *plat = dev_get_plat(dev);
	struct ehci_mx6_priv_data *priv = dev_get_priv(dev);
	enum usb_dr_mode dr_mode;
	const struct fdt_property *extcon;

	extcon = fdt_get_property(gd->fdt_blob, dev_of_offset(dev),
			"extcon", NULL);
	if (extcon) {
		priv->init_type = board_ehci_usb_phy_mode(dev);
		goto check_type;
	}

	dr_mode = usb_get_dr_mode(dev_ofnode(dev));

	switch (dr_mode) {
	case USB_DR_MODE_HOST:
		priv->init_type = USB_INIT_HOST;
		break;
	case USB_DR_MODE_PERIPHERAL:
		priv->init_type = USB_INIT_DEVICE;
		break;
	default:
		priv->init_type = USB_INIT_UNKNOWN;
	};

check_type:
	if (priv->init_type != USB_INIT_UNKNOWN && priv->init_type != plat->init_type) {
		debug("Request USB type is %u, board forced type is %u\n",
			plat->init_type, priv->init_type);
		return -ENODEV;
	}

	return 0;
}

static int mx6_parse_dt_addrs(struct udevice *dev)
{
	struct ehci_mx6_priv_data *priv = dev_get_priv(dev);
	int phy_off, misc_off;
	const void *blob = gd->fdt_blob;
	int offset = dev_of_offset(dev);

	phy_off = fdtdec_lookup_phandle(blob, offset, "fsl,usbphy");
	if (phy_off < 0) {
		phy_off = fdtdec_lookup_phandle(blob, offset, "phys");
		if (phy_off < 0)
			return -EINVAL;
	}

	misc_off = fdtdec_lookup_phandle(blob, offset, "fsl,usbmisc");
	if (misc_off < 0)
		return -EINVAL;

	priv->portnr = dev_seq(dev);
	priv->phy_node_off = phy_off;

#if !CONFIG_IS_ENABLED(PHY) || defined(CONFIG_IMX8)
	void *__iomem addr;
	struct ehci_mx6_phy_data *phy_data = &priv->phy_data;
	addr = (void __iomem *)fdtdec_get_addr(blob, phy_off, "reg");
	if ((fdt_addr_t)addr == FDT_ADDR_T_NONE)
		addr = NULL;

	phy_data->phy_addr = addr;

	addr = (void __iomem *)fdtdec_get_addr(blob, misc_off, "reg");
	if ((fdt_addr_t)addr == FDT_ADDR_T_NONE)
		return -EINVAL;

	phy_data->misc_addr = addr;

#if defined(CONFIG_MX6)
	int anatop_off;

	/* Resolve ANATOP offset through USB PHY node */
	anatop_off = fdtdec_lookup_phandle(blob, phy_off, "fsl,anatop");
	if (anatop_off < 0)
		return -EINVAL;

	addr = (void __iomem *)fdtdec_get_addr(blob, anatop_off, "reg");
	if ((fdt_addr_t)addr == FDT_ADDR_T_NONE)
		return -EINVAL;

	phy_data->anatop_addr = addr;
#endif
#endif
	return 0;
}

#if !CONFIG_IS_ENABLED(PHY) || defined(CONFIG_IMX8)
static int ehci_mx6_phy_prepare(struct ehci_mx6_priv_data *priv)
{
	dev_set_ofnode(&priv->phy_dev, offset_to_ofnode(priv->phy_node_off));

#if CONFIG_IS_ENABLED(POWER_DOMAIN)
	/* Need to power on the PHY before access it */
	if (!power_domain_get(&priv->phy_dev, &priv->phy_pd)) {
		if (power_domain_on(&priv->phy_pd))
			return -EINVAL;
	}
#endif

#if CONFIG_IS_ENABLED(CLK)
	int ret;

	ret = clk_get_by_index(&priv->phy_dev, 0, &priv->phy_clk);
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

	return 0;
}

static int ehci_mx6_phy_remove(struct ehci_mx6_priv_data *priv)
{
	int ret = 0;

#if CONFIG_IS_ENABLED(CLK)
	if (priv->phy_clk.dev) {
		ret = clk_disable(&priv->phy_clk);
		if (ret)
			return ret;

		ret = clk_free(&priv->phy_clk);
		if (ret)
			return ret;
	}
#endif

#if CONFIG_IS_ENABLED(POWER_DOMAIN)
	if (priv->phy_pd.dev) {
		ret = power_domain_off(&priv->phy_pd);
		if (ret)
			printf("Power down USB PHY failed! (error = %d)\n", ret);
	}
#endif

	return ret;
}
#endif

static int ehci_usb_probe(struct udevice *dev)
{
	struct usb_plat *plat = dev_get_plat(dev);
	struct usb_ehci *ehci = dev_read_addr_ptr(dev);
	struct ehci_mx6_priv_data *priv = dev_get_priv(dev);
	enum usb_init_type type = plat->init_type;
	struct ehci_hccr *hccr;
	struct ehci_hcor *hcor;
	int ret;

	if (CONFIG_IS_ENABLED(IMX_MODULE_FUSE)) {
		if (usb_fused((ulong)ehci)) {
			printf("SoC fuse indicates USB@0x%lx is unavailable.\n",
			       (ulong)ehci);
			return -ENODEV;
		}
	}

	ret = mx6_parse_dt_addrs(dev);
	if (ret)
		return ret;

	priv->ehci = ehci;
	priv->phy_type = usb_get_phy_mode(dev_ofnode(dev));

	/* Init usb board level according to the requested init type */
	ret = board_usb_init(priv->portnr, type);
	if (ret) {
		printf("Failed to initialize board for USB\n");
		return ret;
	}

#if !CONFIG_IS_ENABLED(PHY) || defined(CONFIG_IMX8)
	ret = ehci_mx6_phy_prepare(priv);
	if (ret) {
		printf("Failed to prepare USB phy\n");
		return ret;
	}
#endif

#if CONFIG_IS_ENABLED(CLK)
	ret = clk_get_by_index(dev, 0, &priv->clk);
	if (ret < 0)
		return ret;

	ret = clk_enable(&priv->clk);
	if (ret)
		return ret;
#else
	/* Compatibility with DM_USB and !CLK */
	enable_usboh3_clk(1);
	mdelay(1);
#endif

#if CONFIG_IS_ENABLED(DM_REGULATOR)
	ret = device_get_supply_regulator(dev, "vbus-supply",
					  &priv->vbus_supply);
	if (ret)
		debug("%s: No vbus supply\n", dev->name);
#endif

#if !CONFIG_IS_ENABLED(PHY) || defined(CONFIG_IMX8)
	ehci_mx6_phy_init(ehci, &priv->phy_data, priv->portnr);
#else
	ret = ehci_setup_phy(dev, &priv->phy, priv->portnr);
	if (ret)
		goto err_clk;
#endif

	/* If the init_type is unknown due to it is not forced in DTB, we use USB ID to detect */
	if (priv->init_type == USB_INIT_UNKNOWN) {
		ret = ehci_usb_phy_mode(dev);
		if (ret)
			goto err_clk;
		if (priv->init_type != type) {
			ret = -ENODEV;
			goto err_clk;
		}
	}

#if CONFIG_IS_ENABLED(DM_REGULATOR)
	if (priv->vbus_supply) {
		ret = regulator_set_enable(priv->vbus_supply,
					   (priv->init_type == USB_INIT_DEVICE) ?
					   false : true);
		if (ret && ret != -ENOSYS) {
			printf("Error enabling VBUS supply (ret=%i)\n", ret);
			goto err_phy;
		}
	}
#endif

	if (priv->init_type == USB_INIT_HOST) {
		setbits_le32(&ehci->usbmode, CM_HOST);
		writel(mx6_portsc(priv->phy_type), &ehci->portsc);
		setbits_le32(&ehci->portsc, USB_EN);
	}

	mdelay(10);

	hccr = (struct ehci_hccr *)((uintptr_t)&ehci->caplength);
	hcor = (struct ehci_hcor *)((uintptr_t)hccr +
			HC_LENGTH(ehci_readl(&(hccr)->cr_capbase)));

	ret = ehci_register(dev, hccr, hcor, &mx6_ehci_ops, 0, priv->init_type);
	if (ret)
		goto err_regulator;

	return ret;


err_regulator:
#if CONFIG_IS_ENABLED(DM_REGULATOR)
	if (priv->vbus_supply)
		regulator_set_enable(priv->vbus_supply, false);
#endif
err_phy:
#if CONFIG_IS_ENABLED(PHY) && !defined(CONFIG_IMX8)
	ehci_shutdown_phy(dev, &priv->phy);
#endif
err_clk:
#if CONFIG_IS_ENABLED(CLK)
	clk_disable(&priv->clk);
#endif
#if !CONFIG_IS_ENABLED(PHY) || defined(CONFIG_IMX8)
	ehci_mx6_phy_remove(priv);
#endif

	return ret;
}

int ehci_usb_remove(struct udevice *dev)
{
	struct ehci_mx6_priv_data *priv = dev_get_priv(dev);
	struct usb_plat *plat = dev_get_plat(dev);

	ehci_deregister(dev);

#if CONFIG_IS_ENABLED(PHY) && !defined(CONFIG_IMX8)
	ehci_shutdown_phy(dev, &priv->phy);
#endif

#if CONFIG_IS_ENABLED(DM_REGULATOR)
	if (priv->vbus_supply)
		regulator_set_enable(priv->vbus_supply, false);
#endif

#if CONFIG_IS_ENABLED(CLK)
	clk_disable(&priv->clk);
#endif

#if !CONFIG_IS_ENABLED(PHY) || defined(CONFIG_IMX8)
	ehci_mx6_phy_remove(priv);
#endif

	plat->init_type = 0; /* Clean the requested usb type to host mode */

	return board_usb_cleanup(dev_seq(dev), priv->init_type);
}

static const struct udevice_id mx6_usb_ids[] = {
	{ .compatible = "fsl,imx27-usb" },
	{ .compatible = "fsl,imx7d-usb" },
	{ .compatible = "fsl,imxrt-usb" },
	{ }
};

U_BOOT_DRIVER(usb_mx6) = {
	.name	= "ehci_mx6",
	.id	= UCLASS_USB,
	.of_match = mx6_usb_ids,
	.of_to_plat = ehci_usb_of_to_plat,
	.probe	= ehci_usb_probe,
	.remove = ehci_usb_remove,
	.ops	= &ehci_usb_ops,
	.plat_auto	= sizeof(struct usb_plat),
	.priv_auto	= sizeof(struct ehci_mx6_priv_data),
	.flags	= DM_FLAG_ALLOC_PRIV_DMA,
};
#endif
