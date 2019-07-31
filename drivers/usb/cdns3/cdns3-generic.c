// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2019 NXP
 */

#include <common.h>
#include <clk.h>
#include <dm.h>
#include <generic-phy.h>
#include <linux/usb/gadget.h>
#include <dm/device-internal.h>
#include "core.h"
#include "gadget.h"

static int cdns3_generic_peripheral_clk_init(struct udevice *dev,
					     struct cdns3_generic_peripheral
					     *priv)
{
#if CONFIG_IS_ENABLED(CLK)
	int ret;

	ret = clk_get_bulk(dev, &priv->clks);
	if (ret == -ENOSYS)
		return 0;
	if (ret)
		return ret;

	ret = clk_enable_bulk(&priv->clks);
	if (ret) {
		clk_release_bulk(&priv->clks);
		return ret;
	}
#endif

	return 0;
}

static int cdns3_generic_handle_interrupts(struct udevice *dev)
{
	struct cdns3_generic_peripheral *priv = dev_get_priv(dev);
	struct cdns3 *cdns3 = &priv->cdns3;

	cdns3_role_irq_handler(cdns3);

	return 0;
}

static int cdns3_generic_peripheral_probe(struct udevice *dev)
{
	int ret;
	struct cdns3_generic_peripheral *priv = dev_get_priv(dev);
	struct cdns3 *cdns3 = &priv->cdns3;

	cdns3->dev = dev;

	ret = generic_phy_get_by_index(dev, 0, &priv->phy);
	if (ret && ret != -ENOENT) {
		printf("Failed to get USB PHY for %s\n", dev->name);
		return ret;
	}

	ret = cdns3_generic_peripheral_clk_init(dev, priv);
	if (ret)
		return ret;

	ret = cdns3_init(cdns3);

	return 0;
}

static int cdns3_generic_peripheral_remove(struct udevice *dev)
{
	struct cdns3_generic_peripheral *priv = dev_get_priv(dev);
	struct cdns3 *cdns3 = &priv->cdns3;

	cdns3_exit(cdns3);

	clk_release_bulk(&priv->clks);

	if (generic_phy_valid(&priv->phy))
		device_remove(priv->phy.dev, DM_REMOVE_NORMAL);

	return 0;
}

static int cdns3_generic_peripheral_ofdata_to_platdata(struct udevice *dev)
{
	struct cdns3_generic_peripheral *priv = dev_get_priv(dev);
	struct cdns3 *cdns3 = &priv->cdns3;

	cdns3->none_core_regs = (void __iomem *)devfdt_get_addr_name(dev,
								     "none-core");
	cdns3->xhci_regs = (void __iomem *)devfdt_get_addr_name(dev, "xhci");
	cdns3->dev_regs = (void __iomem *)devfdt_get_addr_name(dev, "dev");
	cdns3->phy_regs = (void __iomem *)devfdt_get_addr_name(dev, "phy");
	cdns3->otg_regs = (void __iomem *)devfdt_get_addr_name(dev, "otg");

	return 0;
}

static const struct udevice_id cdns3_generic_peripheral_ids[] = {
	{ .compatible = "Cadence,usb3" },
	{},
};

U_BOOT_DRIVER(cdns3_generic_peripheral) = {
	.name	= "cdns3-generic-peripheral",
	.id	= UCLASS_USB_GADGET_GENERIC,
	.of_match = cdns3_generic_peripheral_ids,
	.ofdata_to_platdata = cdns3_generic_peripheral_ofdata_to_platdata,
	.probe = cdns3_generic_peripheral_probe,
	.remove = cdns3_generic_peripheral_remove,
	.handle_interrupts = cdns3_generic_handle_interrupts,
	.priv_auto_alloc_size = sizeof(struct cdns3_generic_peripheral),
};
