// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2019 NXP
 */

#include <common.h>
#include <asm-generic/io.h>
#include <dm.h>
#include <dm/device-internal.h>
#include <dm/lists.h>
#include <linux/usb/gadget.h>
#include <malloc.h>
#include <usb.h>
#include "core.h"
#include "gadget.h"
#include <clk.h>
#include <power-domain.h>

#if CONFIG_IS_ENABLED(DM_USB_GADGET)

static int cdns3_setup_phy(struct udevice *dev,
			   struct cdns3_generic_peripheral *priv)
{
	int ret = 0;

#if CONFIG_IS_ENABLED(POWER_DOMAIN)
	struct udevice phy_dev;
	int phy_off;

	phy_off = fdtdec_lookup_phandle(gd->fdt_blob, dev_of_offset(dev),
					"cdns3,usbphy");
	if (phy_off < 0)
		return -EINVAL;

	phy_dev.node = offset_to_ofnode(phy_off);

	if (!power_domain_get(&phy_dev, &priv->phy_pd)) {
		if (power_domain_on(&priv->phy_pd))
			return -EINVAL;
	}
#endif

#if CONFIG_IS_ENABLED(CLK)
	ret = clk_get_by_name(&phy_dev, "main_clk", &priv->phy_clk);
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
	return ret;
}

static int cdns3_shutdown_phy(struct udevice *dev,
			      struct cdns3_generic_peripheral *priv)
{
	int ret;

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

	ret = power_domain_off(&priv->phy_pd);
	if (ret)
		printf("conn_usb2_phy Power down failed! (error = %d)\n", ret);

	return ret;
}

static int cdns3_generic_peripheral_clk_init(struct udevice *dev,
					     struct cdns3_generic_peripheral
					     *priv)
{
	int ret;

	ret = clk_get_bulk(dev, &priv->clks);
	if (ret)
		return ret;

#if CONFIG_IS_ENABLED(CLK)
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

	cdns3_setup_phy(dev, priv);
	ret = cdns3_generic_peripheral_clk_init(dev, priv);
	if (ret)
		return ret;

	ret = board_usb_init(dev->seq, USB_INIT_DEVICE);
	ret = cdns3_init(cdns3);
	printf("cdns3_uboot_initmode %d\n",  ret);

	return 0;
}

static int cdns3_generic_peripheral_remove(struct udevice *dev)
{
	struct cdns3_generic_peripheral *priv = dev_get_priv(dev);
	struct cdns3 *cdns3 = &priv->cdns3;
	struct power_domain pd;
	int ret = 0;

	cdns3_exit(cdns3);

	clk_release_bulk(&priv->clks);
	cdns3_shutdown_phy(dev, priv);

	if (!power_domain_get(dev, &pd)) {
		ret = power_domain_off(&pd);
		if (ret)
			printf("conn_usb2 power down failed!(error = %d)\n",
			       ret);
	}

	return ret;
}

static int cdns3_generic_peripheral_ofdata_to_platdata(struct udevice *dev)
{
	struct cdns3_generic_peripheral *priv = dev_get_priv(dev);
	struct cdns3 *cdns3 = &priv->cdns3;

	cdns3->none_core_regs = (void __iomem *)devfdt_get_addr_index(dev, 0);
	cdns3->xhci_regs = (void __iomem *)devfdt_get_addr_index(dev, 1);
	cdns3->dev_regs = (void __iomem *)devfdt_get_addr_index(dev, 2);
	cdns3->phy_regs = (void __iomem *)devfdt_get_addr_index(dev, 3);
	cdns3->otg_regs = (void __iomem *)devfdt_get_addr_index(dev, 4);

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
#endif

