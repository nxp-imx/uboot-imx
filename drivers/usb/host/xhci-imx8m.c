/*
 * Copyright 2017-2023 NXP
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
#include <linux/bitfield.h>
#include <linux/math64.h>
#include <asm/arch/sys_proto.h>
#include <dm.h>
#include <usb/xhci.h>
#include <clk.h>
#include <generic-phy.h>
#include <dwc3-uboot.h>

/* Declare global data pointer */
DECLARE_GLOBAL_DATA_PTR;

#define NSEC_PER_SEC	1000000000L

struct xhci_imx8m_plat {
	struct clk_bulk clks;
	struct phy_bulk phys;
	struct clk *ref_clk;
	bool gfladj_refclk_lpm_sel;
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

static void imx8m_xhci_ref_clk_period(struct dwc3 *dwc3_reg,
	struct xhci_imx8m_plat *plat)
{
	unsigned long period;
	unsigned long fladj;
	unsigned long decr;
	unsigned long rate;
	u32 reg;

	if (plat->ref_clk) {
		rate = clk_get_rate(plat->ref_clk);
		if (!rate)
			return;
		period = NSEC_PER_SEC / rate;
	} else {
		return;
	}

	reg = readl(&dwc3_reg->g_uctl);
	reg &= ~DWC3_GUCTL_REFCLKPER_MASK;
	reg |=  FIELD_PREP(DWC3_GUCTL_REFCLKPER_MASK, period);
	writel(reg, &dwc3_reg->g_uctl);


	/*
	 * The calculation below is
	 *
	 * 125000 * (NSEC_PER_SEC / (rate * period) - 1)
	 *
	 * but rearranged for fixed-point arithmetic. The division must be
	 * 64-bit because 125000 * NSEC_PER_SEC doesn't fit in 32 bits (and
	 * neither does rate * period).
	 *
	 * Note that rate * period ~= NSEC_PER_SECOND, minus the number of
	 * nanoseconds of error caused by the truncation which happened during
	 * the division when calculating rate or period (whichever one was
	 * derived from the other). We first calculate the relative error, then
	 * scale it to units of 8 ppm.
	 */
	fladj = div64_u64(125000ULL * NSEC_PER_SEC, (u64)rate * period);
	fladj -= 125000;

	/*
	 * The documented 240MHz constant is scaled by 2 to get PLS1 as well.
	 */
	decr = 480000000 / rate;

	reg = readl(&dwc3_reg->g_fladj);
	reg &= ~DWC3_GFLADJ_REFCLK_FLADJ_MASK
	    &  ~DWC3_GFLADJ_240MHZDECR
	    &  ~DWC3_GFLADJ_240MHZDECR_PLS1;
	reg |= FIELD_PREP(DWC3_GFLADJ_REFCLK_FLADJ_MASK, fladj)
	    |  FIELD_PREP(DWC3_GFLADJ_240MHZDECR, decr >> 1)
	    |  FIELD_PREP(DWC3_GFLADJ_240MHZDECR_PLS1, decr & 1);

	if (plat->gfladj_refclk_lpm_sel)
		reg |= DWC3_GFLADJ_REFCLK_LPM_SEL;

	writel(reg, &dwc3_reg->g_fladj);
}

static int imx8m_xhci_core_init(struct dwc3 *dwc3_reg,
	struct xhci_imx8m_plat *plat)
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

	/* Adjust Reference Clock Period */
	imx8m_xhci_ref_clk_period(dwc3_reg, plat);

	return ret;
}

static int xhci_imx8m_clk_init(struct udevice *dev,
			      struct xhci_imx8m_plat *plat)
{
	int ret, index;

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

	index = ofnode_stringlist_search(dev_ofnode(dev), "clock-names", "ref");
	if (index >= 0)
		plat->ref_clk = &plat->clks.clks[index];

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

	plat->gfladj_refclk_lpm_sel = dev_read_bool(dev,
				"snps,gfladj-refclk-lpm-sel-quirk");

	hccr = (struct xhci_hccr *)((uintptr_t)dev_remap_addr(dev));
	if (!hccr)
		return -EINVAL;

	hcor = (struct xhci_hcor *)((uintptr_t)hccr +
			HC_LENGTH(xhci_readl(&(hccr)->cr_capbase)));

	ret = dwc3_setup_phy(dev, &plat->phys);
	if (ret && (ret != -ENOTSUPP))
		return ret;

	dwc3_reg = (struct dwc3 *)((char *)(hccr) + DWC3_REG_OFFSET);

	ret = imx8m_xhci_core_init(dwc3_reg, plat);
	if (ret < 0) {
		puts("Failed to initialize imx8m xhci\n");
		return ret;
	}

	ret = board_usb_init(dev_seq(dev), USB_INIT_HOST);
	if (ret != 0) {
		puts("Failed to initialize board for imx8m USB\n");
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
