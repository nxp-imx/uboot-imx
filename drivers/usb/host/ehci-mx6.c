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
#include <usb/usb_mx6_common.h>
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
#include <asm/arch/sys_proto.h>

#include "ehci.h"
#if CONFIG_IS_ENABLED(POWER_DOMAIN)
#include <power-domain.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

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

#if !CONFIG_IS_ENABLED(DM_USB)
static const struct ehci_ops mx6_ehci_ops = {
	.powerup_fixup		= ehci_mx6_powerup_fixup,
};

int ehci_hcd_init(int index, enum usb_init_type init,
		struct ehci_hccr **hccr, struct ehci_hcor **hcor)
{
	enum usb_init_type type;
#if defined(CONFIG_MX6)
	u32 controller_spacing = 0x200;
#elif defined(CONFIG_USB_EHCI_MX7) || defined(CONFIG_MX7ULP) || defined(CONFIG_IMX8)
	u32 controller_spacing = 0x10000;
#endif
	struct usb_ehci *ehci = (struct usb_ehci *)(ulong)(USB_BASE_ADDR +
		(controller_spacing * index));
	int ret;

	if (index > 3)
		return -EINVAL;

#if defined(CONFIG_MX6)
	if (mx6_usb_fused((u32)ehci)) {
		printf("USB@0x%x is fused, disable it\n", (u32)ehci);
		return -ENODEV;
	}
#endif

	ret = ehci_mx6_common_init(ehci, index);
	if (ret)
		return ret;

	ehci_set_controller_priv(index, NULL, &mx6_ehci_ops);

	type = board_usb_phy_mode(index);

	if (hccr && hcor) {
		*hccr = (struct ehci_hccr *)((ulong)&ehci->caplength);
		*hcor = (struct ehci_hcor *)((ulong)*hccr +
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
#define  USB_INIT_UNKNOWN (USB_INIT_DEVICE + 1)

struct ehci_mx6_priv_data {
	struct ehci_ctrl ctrl;
	struct usb_ehci *ehci;
	struct udevice *vbus_supply;
	enum usb_init_type init_type;
	void *__iomem phy_base;
	int portnr;
};

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

	ret = ehci_mx6_common_init(priv->ehci, priv->portnr);
	if (ret)
		return ret;

#if CONFIG_IS_ENABLED(DM_REGULATOR)
	if (priv->vbus_supply) {
		ret = regulator_set_enable(priv->vbus_supply,
					   (type == USB_INIT_DEVICE) ?
					   false : true);
		if (ret) {
			puts("Error enabling VBUS supply\n");
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
	writel(CONFIG_MXC_USB_PORTSC, &ehci->portsc);
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
	void *__iomem phy_ctrl, *__iomem phy_status;
	u32 val;

	if (is_mx6() || is_mx7ulp() || is_imx8()) {
		phy_ctrl = (void __iomem *)(priv->phy_base + USBPHY_CTRL);
		val = readl(phy_ctrl);

		if (val & USBPHY_CTRL_OTG_ID)
			priv->init_type = USB_INIT_DEVICE;
		else
			priv->init_type = USB_INIT_HOST;
	} else if (is_mx7() || is_imx8mm() || is_imx8mn()) {
		phy_status = (void __iomem *)(priv->phy_base +
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

static int ehci_get_usb_phy(struct udevice *dev)
{
	struct ehci_mx6_priv_data *priv = dev_get_priv(dev);
	void *__iomem addr = (void *__iomem)devfdt_get_addr(dev);
	const void *blob = gd->fdt_blob;
	int offset = dev_of_offset(dev), phy_off;

	/*
	 * About fsl,usbphy, Refer to
	 * Documentation/devicetree/bindings/usb/ci-hdrc-usb2.txt.
	 */
	if (is_mx6() || is_mx7ulp() || is_imx8()) {
		phy_off = fdtdec_lookup_phandle(blob,
						offset,
						"fsl,usbphy");
		if (phy_off < 0)
			return -EINVAL;

		addr = (void __iomem *)fdtdec_get_addr(blob, phy_off,
						       "reg");
		if ((fdt_addr_t)addr == FDT_ADDR_T_NONE)
			return -EINVAL;

		/* Need to power on the PHY before access it */
#if CONFIG_IS_ENABLED(POWER_DOMAIN)
		struct udevice phy_dev;
		struct power_domain pd;

		phy_dev.node = offset_to_ofnode(phy_off);
		if (!power_domain_get(&phy_dev, &pd)) {
			if (power_domain_on(&pd))
				return -EINVAL;
		}
#endif
		priv->phy_base = addr;
	} else if (is_mx7() || is_imx8mm() || is_imx8mn()) {
		priv->phy_base = addr;
	} else {
		return -EINVAL;
	}

	return 0;
}

static int ehci_usb_ofdata_to_platdata(struct udevice *dev)
{
	struct usb_platdata *plat = dev_get_platdata(dev);
	struct ehci_mx6_priv_data *priv = dev_get_priv(dev);
	enum usb_dr_mode dr_mode;
	const struct fdt_property *extcon;

	extcon = fdt_get_property(gd->fdt_blob, dev_of_offset(dev),
			"extcon", NULL);
	if (extcon) {
		priv->init_type = board_ehci_usb_phy_mode(dev);
		goto check_type;
	}

	dr_mode = usb_get_dr_mode(dev_of_offset(dev));

	switch (dr_mode) {
	case USB_DR_MODE_HOST:
		priv->init_type = USB_INIT_HOST;
		break;
	case USB_DR_MODE_PERIPHERAL:
		priv->init_type = USB_INIT_DEVICE;
		break;
	case USB_DR_MODE_OTG:
	case USB_DR_MODE_UNKNOWN:
		priv->init_type = USB_INIT_UNKNOWN;
		break;
	};

check_type:
	if (priv->init_type != USB_INIT_UNKNOWN && priv->init_type != plat->init_type) {
		debug("Request USB type is %u, board forced type is %u\n",
			plat->init_type, priv->init_type);
		return -ENODEV;
	}

	return 0;
}

static int ehci_usb_bind(struct udevice *dev)
{
	/*
	 * TODO:
	 * This driver is only partly converted to DT probing and still uses
	 * a tremendous amount of hard-coded addresses. To make things worse,
	 * the driver depends on specific sequential indexing of controllers,
	 * from which it derives offsets in the PHY and ANATOP register sets.
	 *
	 * Here we attempt to calculate these indexes from DT information as
	 * well as we can. The USB controllers on all existing iMX6 SoCs
	 * are placed next to each other, at addresses incremented by 0x200,
	 * and iMX7 their addresses are shifted by 0x10000.
	 * Thus, the index is derived from the multiple of 0x200 (0x10000 for
	 * iMX7) offset from the first controller address.
	 *
	 * However, to complete conversion of this driver to DT probing, the
	 * following has to be done:
	 * - DM clock framework support for iMX must be implemented
	 * - usb_power_config() has to be converted to clock framework
	 *   -> Thus, the ad-hoc "index" variable goes away.
	 * - USB PHY handling has to be factored out into separate driver
	 *   -> Thus, the ad-hoc "index" variable goes away from the PHY
	 *      code, the PHY driver must parse it's address from DT. This
	 *      USB driver must find the PHY driver via DT phandle.
	 *   -> usb_power_config() shall be moved to PHY driver
	 * With these changes in place, the ad-hoc indexing goes away and
	 * the driver is fully converted to DT probing.
	 */
	u32 controller_spacing;

	if (dev->req_seq == -1) {
		if (IS_ENABLED(CONFIG_MX6))
			controller_spacing = 0x200;
		else
			controller_spacing = 0x10000;
		fdt_addr_t addr = devfdt_get_addr_index(dev, 0);

		dev->req_seq = (addr - USB_BASE_ADDR) / controller_spacing;
	}

	return 0;
}

static int ehci_usb_probe(struct udevice *dev)
{
	struct usb_platdata *plat = dev_get_platdata(dev);
	struct usb_ehci *ehci = (struct usb_ehci *)devfdt_get_addr(dev);
	struct ehci_mx6_priv_data *priv = dev_get_priv(dev);
	enum usb_init_type type = plat->init_type;
	struct ehci_hccr *hccr;
	struct ehci_hcor *hcor;
	int ret;

#if defined(CONFIG_MX6)
	if (mx6_usb_fused((u32)ehci)) {
		printf("USB@0x%x is fused, disable it\n", (u32)ehci);
		return -ENODEV;
	}
#endif

	priv->ehci = ehci;
	priv->portnr = dev->req_seq;

	/* Init usb board level according to the requested init type */
	ret = board_usb_init(priv->portnr, type);
	if (ret) {
		printf("Failed to initialize board for USB\n");
		return ret;
	}

#if CONFIG_IS_ENABLED(DM_REGULATOR)
	ret = device_get_supply_regulator(dev, "vbus-supply",
					  &priv->vbus_supply);
	if (ret)
		debug("%s: No vbus supply\n", dev->name);
#endif

	ret = ehci_get_usb_phy(dev);
	if (ret) {
		debug("%s: fail to get USB PHY base\n", dev->name);
		return ret;
	}

	ret = ehci_mx6_common_init(ehci, priv->portnr);
	if (ret)
		return ret;

	/* If the init_type is unknown due to it is not forced in DTB, we use USB ID to detect */
	if (priv->init_type == USB_INIT_UNKNOWN) {
		ret = ehci_usb_phy_mode(dev);
		if (ret)
			return ret;
		if (priv->init_type != type)
			return -ENODEV;
	}

#if CONFIG_IS_ENABLED(DM_REGULATOR)
	if (priv->vbus_supply) {
		ret = regulator_set_enable(priv->vbus_supply,
					   (priv->init_type == USB_INIT_DEVICE) ?
					   false : true);
		if (ret) {
			puts("Error enabling VBUS supply\n");
			return ret;
		}
	}
#endif

	if (priv->init_type == USB_INIT_HOST) {
		setbits_le32(&ehci->usbmode, CM_HOST);
		writel(CONFIG_MXC_USB_PORTSC, &ehci->portsc);
		setbits_le32(&ehci->portsc, USB_EN);
	}

	mdelay(10);

	hccr = (struct ehci_hccr *)((ulong)&ehci->caplength);
	hcor = (struct ehci_hcor *)((ulong)hccr +
			HC_LENGTH(ehci_readl(&(hccr)->cr_capbase)));

	return ehci_register(dev, hccr, hcor, &mx6_ehci_ops, 0, priv->init_type);
}

int ehci_usb_remove(struct udevice *dev)
{
	struct ehci_mx6_priv_data *priv = dev_get_priv(dev);
	struct usb_platdata *plat = dev_get_platdata(dev);

	ehci_deregister(dev);

	plat->init_type = 0; /* Clean the requested usb type to host mode */

	return board_usb_cleanup(dev->req_seq, priv->init_type);
}

static const struct udevice_id mx6_usb_ids[] = {
	{ .compatible = "fsl,imx27-usb" },
	{ }
};

U_BOOT_DRIVER(usb_mx6) = {
	.name	= "ehci_mx6",
	.id	= UCLASS_USB,
	.of_match = mx6_usb_ids,
	.ofdata_to_platdata = ehci_usb_ofdata_to_platdata,
	.bind	= ehci_usb_bind,
	.probe	= ehci_usb_probe,
	.remove = ehci_usb_remove,
	.ops	= &ehci_usb_ops,
	.platdata_auto_alloc_size = sizeof(struct usb_platdata),
	.priv_auto_alloc_size = sizeof(struct ehci_mx6_priv_data),
	.flags	= DM_FLAG_ALLOC_PRIV_DMA,
};
#endif
