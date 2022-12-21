/*
 * Copyright 2017 NXP
 *
 * FSL i.MX8M USB HOST xHCI Controller
 *
 * Author: Jun Li <jun.li@nxp.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <usb.h>
#include <linux/errno.h>
#include <linux/compat.h>
#include <linux/usb/dwc3.h>
#include <asm/arch/sys_proto.h>
#include <dm.h>
#include <usb/xhci.h>
#include <clk.h>
#include <generic-phy.h>
#include <dwc3-uboot.h>

/* Declare global data pointer */
DECLARE_GLOBAL_DATA_PTR;

struct xhci_imx8m_plat {
	struct clk_bulk clks;
	struct phy_bulk phys;
};

static void imx8m_xhci_set_suspend_clk(struct dwc3 *dwc3_reg)
{
	u32 reg;

	/* Set suspend_clk to be 32KHz */
	reg = readl(&dwc3_reg->g_ctl);
	reg &= ~(DWC3_GCTL_PWRDNSCALE_MASK);
	reg |= DWC3_GCTL_PWRDNSCALE(2);

	writel(reg, &dwc3_reg->g_ctl);
}

static int imx8m_xhci_core_init(struct dwc3 *dwc3_reg)
{
	int ret = 0;

	ret = dwc3_core_init(dwc3_reg);
	if (ret) {
		debug("%s:failed to initialize core\n", __func__);
		return ret;
	}

	imx8m_xhci_set_suspend_clk(dwc3_reg);

	/* We are hard-coding DWC3 core to Host Mode */
	dwc3_set_mode(dwc3_reg, DWC3_GCTL_PRTCAP_HOST);

	/* Set GFLADJ_30MHZ as 20h as per XHCI spec default value */
	dwc3_set_fladj(dwc3_reg, GFLADJ_30MHZ_DEFAULT);

	return ret;
}

static int xhci_imx8m_clk_init(struct udevice *dev,
			      struct xhci_imx8m_plat *plat)
{
	int ret;

	ret = clk_get_bulk(dev, &plat->clks);
	if (ret == -ENOSYS || ret == -ENOENT)
		return 0;
	if (ret)
		return ret;

	ret = clk_enable_bulk(&plat->clks);
	if (ret) {
		clk_release_bulk(&plat->clks);
		return ret;
	}

	return 0;
}

static int xhci_imx8m_probe(struct udevice *dev)
{
	struct xhci_hccr *hccr;
	struct xhci_hcor *hcor;
	struct dwc3 *dwc3_reg;
	struct xhci_imx8m_plat *plat = dev_get_plat(dev);
	int ret = 0;

	ret = xhci_imx8m_clk_init(dev, plat);
	if (ret)
		return ret;

	ret = board_usb_init(dev_seq(dev), USB_INIT_HOST);
	if (ret != 0) {
		puts("Failed to initialize board for imx8m USB\n");
		return ret;
	}

	hccr = (struct xhci_hccr *)((uintptr_t)dev_remap_addr(dev));
	if (!hccr)
		return -EINVAL;

	hcor = (struct xhci_hcor *)((uintptr_t)hccr +
			HC_LENGTH(xhci_readl(&(hccr)->cr_capbase)));

	ret = dwc3_setup_phy(dev, &plat->phys);
	if (ret && (ret != -ENOTSUPP))
		return ret;

	dwc3_reg = (struct dwc3 *)((char *)(hccr) + DWC3_REG_OFFSET);

	ret = imx8m_xhci_core_init(dwc3_reg);
	if (ret < 0) {
		puts("Failed to initialize imx8m xhci\n");
		return ret;
	}

	debug("imx8m-xhci: init hccr %lx and hcor %lx hc_length %lx\n",
	      (uintptr_t)hccr, (uintptr_t)hcor,
	      (uintptr_t)HC_LENGTH(xhci_readl(&(hccr)->cr_capbase)));

	return xhci_register(dev, hccr, hcor);
}

static int xhci_imx8m_remove(struct udevice *dev)
{
	int ret;
	struct xhci_imx8m_plat *plat = dev_get_plat(dev);

	dwc3_shutdown_phy(dev, &plat->phys);

	clk_release_bulk(&plat->clks);

	ret = xhci_deregister(dev);

	board_usb_cleanup(dev_seq(dev), USB_INIT_HOST);

	return ret;
}

static const struct udevice_id xhci_usb_ids[] = {
	{ .compatible = "fsl,imx8mq-dwc3", },
	{ }
};

U_BOOT_DRIVER(xhci_imx8m) = {
	.name	= "xhci_imx8m",
	.id	= UCLASS_USB,
	.of_match = xhci_usb_ids,
	.probe = xhci_imx8m_probe,
	.remove = xhci_imx8m_remove,
	.ops	= &xhci_usb_ops,
	.plat_auto = sizeof(struct xhci_imx8m_plat),
	.priv_auto = sizeof(struct xhci_ctrl),
	.flags	= DM_FLAG_ALLOC_PRIV_DMA,
};

static const struct udevice_id xhci_imx8mp_ids[] = {
	{ .compatible = "fsl,imx8mp-dwc3", },
	{ }
};

U_BOOT_DRIVER(xhci_imx8mp_misc) = {
	.name	= "xhci_imx8mp_misc",
	.id	= UCLASS_MISC,
	.of_match = of_match_ptr(xhci_imx8mp_ids),
};
