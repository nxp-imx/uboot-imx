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

/* Declare global data pointer */
DECLARE_GLOBAL_DATA_PTR;

#define USBMIX_PHY_OFFSET		0xF0040

#define PHY_CTRL0_REF_SSP_EN		BIT(2)

#define PHY_CTRL1_RESET			BIT(0)
#define PHY_CTRL1_ATERESET		BIT(3)
#define PHY_CTRL1_VDATSRCENB0		BIT(19)
#define PHY_CTRL1_VDATDETENB0		BIT(20)

#define PHY_CTRL2_TXENABLEN0		BIT(8)

struct imx8m_usbmix {
	u32 phy_ctrl0;
	u32 phy_ctrl1;
	u32 phy_ctrl2;
	u32 phy_ctrl3;
	u32 phy_ctrl4;
	u32 phy_ctrl5;
	u32 phy_ctrl6;
};

struct imx8m_xhci {
	struct xhci_hccr *hcd;
	struct dwc3 *dwc3_reg;
	struct imx8m_usbmix *usbmix_reg;
};

struct imx8m_usbctrl_data {
	u32 usb_id;
	unsigned long ctr_addr;
};
static struct imx8m_xhci imx8m_xhci;
static struct imx8m_usbctrl_data ctr_data[] = {
	{0, USB1_BASE_ADDR},
	{1, USB2_BASE_ADDR},
};

#ifdef CONFIG_IMX8MP
#define HSIO_GPR_BASE                               (0x32F10000U)
#define HSIO_GPR_REG_0                              (HSIO_GPR_BASE)
#define HSIO_GPR_REG_0_USB_CLOCK_MODULE_EN_SHIFT    (1)
#define HSIO_GPR_REG_0_USB_CLOCK_MODULE_EN          (0x1U << HSIO_GPR_REG_0_USB_CLOCK_MODULE_EN_SHIFT)
#endif

static void imx8m_usb_phy_init(struct imx8m_usbmix *usbmix_reg)
{
	u32 reg;

#ifdef CONFIG_IMX8MP
	/* enable usb clock via hsio gpr */
	reg = readl(HSIO_GPR_REG_0);
	reg |= HSIO_GPR_REG_0_USB_CLOCK_MODULE_EN;
	writel(reg, HSIO_GPR_REG_0);

	/* USB3.0 PHY signal fsel for 24M ref */
	reg = readl(&usbmix_reg->phy_ctrl0);
	reg = (reg & 0xfffff81f) | (0x2a<<5);
	writel(reg, &usbmix_reg->phy_ctrl0);

	reg = readl(&usbmix_reg->phy_ctrl6);
	reg &=~0x1;
	writel(reg, &usbmix_reg->phy_ctrl6);
#endif

	reg = readl(&usbmix_reg->phy_ctrl1);
	reg &= ~(PHY_CTRL1_VDATSRCENB0 | PHY_CTRL1_VDATDETENB0);
	reg |= PHY_CTRL1_RESET | PHY_CTRL1_ATERESET;
	writel(reg, &usbmix_reg->phy_ctrl1);

	reg = readl(&usbmix_reg->phy_ctrl0);
	reg |= PHY_CTRL0_REF_SSP_EN;
	writel(reg, &usbmix_reg->phy_ctrl0);

	reg = readl(&usbmix_reg->phy_ctrl2);
	reg |= PHY_CTRL2_TXENABLEN0;
	writel(reg, &usbmix_reg->phy_ctrl2);

	reg = readl(&usbmix_reg->phy_ctrl1);
	reg &= ~(PHY_CTRL1_RESET | PHY_CTRL1_ATERESET);
	writel(reg, &usbmix_reg->phy_ctrl1);
}

static void imx8m_xhci_set_suspend_clk(struct dwc3 *dwc3_reg)
{
	u32 reg;

	/* Set suspend_clk to be 32KHz */
	reg = readl(&dwc3_reg->g_ctl);
	reg &= ~(DWC3_GCTL_PWRDNSCALE_MASK);
	reg |= DWC3_GCTL_PWRDNSCALE(2);

	writel(reg, &dwc3_reg->g_ctl);
}

static int imx8m_xhci_core_init(struct imx8m_xhci *imx8m_xhci)
{
	int ret = 0;

	imx8m_usb_phy_init(imx8m_xhci->usbmix_reg);

	ret = dwc3_core_init(imx8m_xhci->dwc3_reg);
	if (ret) {
		debug("%s:failed to initialize core\n", __func__);
		return ret;
	}

	imx8m_xhci_set_suspend_clk(imx8m_xhci->dwc3_reg);

	/* We are hard-coding DWC3 core to Host Mode */
	dwc3_set_mode(imx8m_xhci->dwc3_reg, DWC3_GCTL_PRTCAP_HOST);

	/* Set GFLADJ_30MHZ as 20h as per XHCI spec default value */
	dwc3_set_fladj(imx8m_xhci->dwc3_reg, GFLADJ_30MHZ_DEFAULT);

	return ret;
}

#ifdef CONFIG_DM_USB
static int xhci_imx8m_probe(struct udevice *dev)
{
	struct xhci_hccr *hccr;
	struct xhci_hcor *hcor;
	struct imx8m_xhci *ctx = &imx8m_xhci;
	int ret = 0;

	ctx->hcd = (struct xhci_hccr *)(ctr_data[dev->seq].ctr_addr);
	ctx->dwc3_reg = (struct dwc3 *)((char *)(ctx->hcd) + DWC3_REG_OFFSET);
	ctx->usbmix_reg = (struct imx8m_usbmix *)((char *)(ctx->hcd) +
							USBMIX_PHY_OFFSET);

	ret = board_usb_init(ctr_data[dev->seq].usb_id, USB_INIT_HOST);
	if (ret != 0) {
		imx8m_usb_power(ctr_data[dev->seq].usb_id, false);
		puts("Failed to initialize board for imx8m USB\n");
		return ret;
	}

	ret = imx8m_xhci_core_init(ctx);
	if (ret < 0) {
		puts("Failed to initialize imx8m xhci\n");
		return ret;
	}

	hccr = (struct xhci_hccr *)ctx->hcd;
	hcor = (struct xhci_hcor *)((uintptr_t) hccr
				+ HC_LENGTH(xhci_readl(&(hccr)->cr_capbase)));

	debug("imx8m-xhci: init hccr %lx and hcor %lx hc_length %lx\n",
	      (uintptr_t)hccr, (uintptr_t)hcor,
	      (uintptr_t)HC_LENGTH(xhci_readl(&(hccr)->cr_capbase)));

	return xhci_register(dev, hccr, hcor);
}

static int xhci_imx8m_remove(struct udevice *dev)
{
	int ret = xhci_deregister(dev);

	board_usb_cleanup(dev->seq, USB_INIT_HOST);

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
	.platdata_auto_alloc_size = sizeof(struct usb_platdata),
	.priv_auto_alloc_size = sizeof(struct xhci_ctrl),
	.flags	= DM_FLAG_ALLOC_PRIV_DMA,
};

#else
int xhci_hcd_init(int index, struct xhci_hccr **hccr, struct xhci_hcor **hcor)
{
	struct imx8m_xhci *ctx = &imx8m_xhci;
	int ret = 0;

	ctx->hcd = (struct xhci_hccr *)(ctr_data[index].ctr_addr);
	ctx->dwc3_reg = (struct dwc3 *)((char *)(ctx->hcd) + DWC3_REG_OFFSET);
	ctx->usbmix_reg = (struct imx8m_usbmix *)((char *)(ctx->hcd) +
							USBMIX_PHY_OFFSET);

	ret = board_usb_init(ctr_data[index].usb_id, USB_INIT_HOST);
	if (ret != 0) {
		imx8m_usb_power(ctr_data[index].usb_id, false);
		puts("Failed to initialize board for imx8m USB\n");
		return ret;
	}

	ret = imx8m_xhci_core_init(ctx);
	if (ret < 0) {
		puts("Failed to initialize imx8m xhci\n");
		return ret;
	}

	*hccr = (struct xhci_hccr *)ctx->hcd;
	*hcor = (struct xhci_hcor *)((uintptr_t) *hccr
				+ HC_LENGTH(xhci_readl(&(*hccr)->cr_capbase)));

	debug("imx8m-xhci: init hccr %lx and hcor %lx hc_length %lx\n",
	      (uintptr_t)*hccr, (uintptr_t)*hcor,
	      (uintptr_t)HC_LENGTH(xhci_readl(&(*hccr)->cr_capbase)));

	return ret;
}

void xhci_hcd_stop(int index)
{
	board_usb_cleanup(ctr_data[index].usb_id, USB_INIT_HOST);
}
#endif
