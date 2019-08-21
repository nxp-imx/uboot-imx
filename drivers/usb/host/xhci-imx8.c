// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019 NXP
 *
 * NXP i.MX8 USB HOST xHCI Controller (Cadence IP)
 *
 * Author: Peter Chen <peter.chen@nxp.com>
 */

#include <common.h>
#include <clk.h>
#include <dm.h>
#include <generic-phy.h>
#include <power-domain.h>
#include <usb.h>
#include <wait_bit.h>
#include <dm/device-internal.h>
#include <usb/xhci.h>
#include <asm/arch/clock.h>

/* Declare global data pointer */
DECLARE_GLOBAL_DATA_PTR;

/* Host registers */
#define HCIVERSION_CAPLENGTH	0x10000
#define USBSTS			0x10084

/* None-core registers */
#define USB3_CORE_CTRL1		0x00
#define USB3_CORE_STATUS	0x0c
#define USB3_SSPHY_STATUS	0x4c

/* USB3_CORE_CTRL1 */
#define ALL_SW_RESET		0xfc000000
#define MODE_STRAP_MASK		0x7
#define PHYAHB_SW_RESET		BIT(26)
#define OC_DISABLE		BIT(9)
#define HOST_MODE		BIT(1)
#define OTG_MODE		BIT(0)

/* USB3_CORE_STATUS */
#define HOST_POWER_ON_READY	BIT(12)

/* USBSTS */
#define CONTROLLER_NOT_READY	BIT(11)

/* USB3_SSPHY_STATUS */
#define CLK_VLD	0xf0000000

struct xhci_imx8_data {
	void __iomem *usb3_ctrl_base;
	void __iomem *usb3_core_base;
	struct clk_bulk clks;
	struct phy phy;
};

static struct xhci_imx8_data imx8_data;

static int imx8_xhci_init(void)
{
	int ret;

	writel(CLK_VLD, imx8_data.usb3_ctrl_base + USB3_SSPHY_STATUS);
	ret = wait_for_bit_le32(imx8_data.usb3_ctrl_base + USB3_SSPHY_STATUS,
				CLK_VLD, true, 100, false);
	if (ret) {
		printf("clkvld is incorrect\n");
		return ret;
	}

	clrsetbits_le32(imx8_data.usb3_ctrl_base + USB3_CORE_CTRL1,
			MODE_STRAP_MASK,  HOST_MODE | OC_DISABLE);
	clrbits_le32(imx8_data.usb3_ctrl_base + USB3_CORE_CTRL1,
		     PHYAHB_SW_RESET);
	generic_phy_init(&imx8_data.phy);

	/* clear all sw_rst */
	clrbits_le32(imx8_data.usb3_ctrl_base + USB3_CORE_CTRL1, ALL_SW_RESET);

	debug("wait xhci_power_on_ready\n");
	ret = wait_for_bit_le32(imx8_data.usb3_ctrl_base + USB3_CORE_STATUS,
				HOST_POWER_ON_READY, true, 100, false);
	if (ret) {
		printf("wait xhci_power_on_ready timeout\n");
		return ret;
	}
	debug("xhci_power_on_ready\n");

	debug("waiting CNR\n");
	ret = wait_for_bit_le32(imx8_data.usb3_core_base + USBSTS,
				CONTROLLER_NOT_READY, false, 100, false);
	if (ret) {
		printf("wait CNR timeout\n");
		return ret;
	}
	debug("check CNR has finished\n");

	return 0;
}

static void imx8_xhci_reset(void)
{
	/* Set CORE ctrl to default value, that all rst are hold */
	writel(ALL_SW_RESET | OTG_MODE,
	       imx8_data.usb3_ctrl_base + USB3_CORE_CTRL1);
}

static int xhci_imx8_clk_init(struct udevice *dev)
{
	int ret;

	ret = clk_get_bulk(dev, &imx8_data.clks);
	if (ret)
		return ret;

	ret = clk_enable_bulk(&imx8_data.clks);
	if (ret)
		return ret;

	return 0;
}

static inline void xhci_imx8_get_reg_addr(struct udevice *dev)
{
	imx8_data.usb3_ctrl_base =
			(void __iomem *)devfdt_get_addr_name(dev, "none-core");
	imx8_data.usb3_core_base =
			(void __iomem *)devfdt_get_addr_name(dev, "otg");

}

static int xhci_imx8_probe(struct udevice *dev)
{
	struct xhci_hccr *hccr;
	struct xhci_hcor *hcor;
	struct udevice usbotg_dev;
	struct power_domain pd;
	int usbotg_off;
	int ret = 0;
	int len;

	usbotg_off = fdtdec_lookup_phandle(gd->fdt_blob,
					   dev_of_offset(dev),
					   "cdns3,usb");
	if (usbotg_off < 0)
		return -EINVAL;
	usbotg_dev.node = offset_to_ofnode(usbotg_off);
	usbotg_dev.parent = dev->parent;
	xhci_imx8_get_reg_addr(&usbotg_dev);

#if CONFIG_IS_ENABLED(POWER_DOMAIN)
	if (!power_domain_get(&usbotg_dev, &pd)) {
		ret = power_domain_on(&pd);
		if (ret)
			return ret;
	}
#endif

	ret = generic_phy_get_by_index(&usbotg_dev, 0, &imx8_data.phy);
	if (ret && ret != -ENOENT) {
		printf("Failed to get USB PHY for %s\n", dev->name);
		return ret;
	}

	ret = board_usb_init(dev->req_seq, USB_INIT_HOST);
	if (ret != 0) {
		printf("Failed to initialize board for USB\n");
		return ret;
	}

#if CONFIG_IS_ENABLED(CLK)
	xhci_imx8_clk_init(&usbotg_dev);
#else
	init_clk_usb3(dev->req_seq);
#endif

	imx8_xhci_init();

	hccr = (struct xhci_hccr *)(imx8_data.usb3_core_base +
				    HCIVERSION_CAPLENGTH);
	len = HC_LENGTH(xhci_readl(&hccr->cr_capbase));
	hcor = (struct xhci_hcor *)((uintptr_t)hccr + len);
	printf("XHCI-imx8 init hccr 0x%p and hcor 0x%p hc_length %d\n",
	       (uint32_t *)hccr, (uint32_t *)hcor, len);

	return xhci_register(dev, hccr, hcor);
}

static int xhci_imx8_remove(struct udevice *dev)
{
	int ret = xhci_deregister(dev);

	if (!ret)
		imx8_xhci_reset();

#if CONFIG_IS_ENABLED(CLK)
	clk_release_bulk(&imx8_data.clks);
#endif
	if (generic_phy_valid(&imx8_data.phy))
		device_remove(imx8_data.phy.dev, DM_REMOVE_NORMAL);

	board_usb_cleanup(dev->req_seq, USB_INIT_HOST);

	return ret;
}

static const struct udevice_id xhci_usb_ids[] = {
	{ .compatible = "Cadence,usb3-host", },
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
	.flags	= DM_FLAG_ALLOC_PRIV_DMA | DM_FLAG_OS_PREPARE,
};
