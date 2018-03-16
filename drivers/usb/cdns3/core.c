/*
 * Copyright (C) 2016 Cadence Design Systems - https://www.cadence.com/
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier:     GPL-2.0
 */
#include <common.h>
#include <malloc.h>
#include <asm/dma-mapping.h>
#include <asm/io.h>
#include <linux/bug.h>
#include <linux/list.h>
#include <linux/compat.h>
#include <cdns3-uboot.h>

#include "linux-compat.h"
#include "cdns3-nxp-reg-def.h"
#include "core.h"
#include "gadget-export.h"

static LIST_HEAD(cdns3_list);

/* Need SoC level to implement the clock */
__weak int cdns3_enable_clks(int index)
{
	return 0;
}

__weak int cdns3_disable_clks(int index)
{
	return 0;
}

static void cdns3_usb_phy_init(void __iomem *regs)
{
	u32 value;

	pr_debug("begin of %s\n", __func__);

	writel(0x0830, regs + PHY_PMA_CMN_CTRL1);
	writel(0x10, regs + TB_ADDR_CMN_DIAG_HSCLK_SEL);
	writel(0x00F0, regs + TB_ADDR_CMN_PLL0_VCOCAL_INIT_TMR);
	writel(0x0018, regs + TB_ADDR_CMN_PLL0_VCOCAL_ITER_TMR);
	writel(0x00D0, regs + TB_ADDR_CMN_PLL0_INTDIV);
	writel(0x4aaa, regs + TB_ADDR_CMN_PLL0_FRACDIV);
	writel(0x0034, regs + TB_ADDR_CMN_PLL0_HIGH_THR);
	writel(0x1ee, regs + TB_ADDR_CMN_PLL0_SS_CTRL1);
	writel(0x7F03, regs + TB_ADDR_CMN_PLL0_SS_CTRL2);
	writel(0x0020, regs + TB_ADDR_CMN_PLL0_DSM_DIAG);
	writel(0x0000, regs + TB_ADDR_CMN_DIAG_PLL0_OVRD);
	writel(0x0000, regs + TB_ADDR_CMN_DIAG_PLL0_FBH_OVRD);
	writel(0x0000, regs + TB_ADDR_CMN_DIAG_PLL0_FBL_OVRD);
	writel(0x0007, regs + TB_ADDR_CMN_DIAG_PLL0_V2I_TUNE);
	writel(0x0027, regs + TB_ADDR_CMN_DIAG_PLL0_CP_TUNE);
	writel(0x0008, regs + TB_ADDR_CMN_DIAG_PLL0_LF_PROG);
	writel(0x0022, regs + TB_ADDR_CMN_DIAG_PLL0_TEST_MODE);
	writel(0x000a, regs + TB_ADDR_CMN_PSM_CLK_CTRL);
	writel(0x139, regs + TB_ADDR_XCVR_DIAG_RX_LANE_CAL_RST_TMR);
	writel(0xbefc, regs + TB_ADDR_XCVR_PSM_RCTRL);

	writel(0x7799, regs + TB_ADDR_TX_PSC_A0);
	writel(0x7798, regs + TB_ADDR_TX_PSC_A1);
	writel(0x509b, regs + TB_ADDR_TX_PSC_A2);
	writel(0x3, regs + TB_ADDR_TX_DIAG_ECTRL_OVRD);
	writel(0x509b, regs + TB_ADDR_TX_PSC_A3);
	writel(0x2090, regs + TB_ADDR_TX_PSC_CAL);
	writel(0x2090, regs + TB_ADDR_TX_PSC_RDY);

	writel(0xA6FD, regs + TB_ADDR_RX_PSC_A0);
	writel(0xA6FD, regs + TB_ADDR_RX_PSC_A1);
	writel(0xA410, regs + TB_ADDR_RX_PSC_A2);
	writel(0x2410, regs + TB_ADDR_RX_PSC_A3);

	writel(0x23FF, regs + TB_ADDR_RX_PSC_CAL);
	writel(0x2010, regs + TB_ADDR_RX_PSC_RDY);

	writel(0x0020, regs + TB_ADDR_TX_TXCC_MGNLS_MULT_000);
	writel(0x00ff, regs + TB_ADDR_TX_DIAG_BGREF_PREDRV_DELAY);
	writel(0x0002, regs + TB_ADDR_RX_SLC_CU_ITER_TMR);
	writel(0x0013, regs + TB_ADDR_RX_SIGDET_HL_FILT_TMR);
	writel(0x0000, regs + TB_ADDR_RX_SAMP_DAC_CTRL);
	writel(0x1004, regs + TB_ADDR_RX_DIAG_SIGDET_TUNE);
	writel(0x4041, regs + TB_ADDR_RX_DIAG_LFPSDET_TUNE2);
	writel(0x0480, regs + TB_ADDR_RX_DIAG_BS_TM);
	writel(0x8006, regs + TB_ADDR_RX_DIAG_DFE_CTRL1);
	writel(0x003f, regs + TB_ADDR_RX_DIAG_ILL_IQE_TRIM4);
	writel(0x543f, regs + TB_ADDR_RX_DIAG_ILL_E_TRIM0);
	writel(0x543f, regs + TB_ADDR_RX_DIAG_ILL_IQ_TRIM0);
	writel(0x0000, regs + TB_ADDR_RX_DIAG_ILL_IQE_TRIM6);
	writel(0x8000, regs + TB_ADDR_RX_DIAG_RXFE_TM3);
	writel(0x0003, regs + TB_ADDR_RX_DIAG_RXFE_TM4);
	writel(0x2408, regs + TB_ADDR_RX_DIAG_LFPSDET_TUNE);
	writel(0x05ca, regs + TB_ADDR_RX_DIAG_DFE_CTRL3);
	writel(0x0258, regs + TB_ADDR_RX_DIAG_SC2C_DELAY);
	writel(0x1fff, regs + TB_ADDR_RX_REE_VGA_GAIN_NODFE);

	writel(0x02c6, regs + TB_ADDR_XCVR_PSM_CAL_TMR);
	writel(0x0002, regs + TB_ADDR_XCVR_PSM_A0BYP_TMR);
	writel(0x02c6, regs + TB_ADDR_XCVR_PSM_A0IN_TMR);
	writel(0x0010, regs + TB_ADDR_XCVR_PSM_A1IN_TMR);
	writel(0x0010, regs + TB_ADDR_XCVR_PSM_A2IN_TMR);
	writel(0x0010, regs + TB_ADDR_XCVR_PSM_A3IN_TMR);
	writel(0x0010, regs + TB_ADDR_XCVR_PSM_A4IN_TMR);
	writel(0x0010, regs + TB_ADDR_XCVR_PSM_A5IN_TMR);

	writel(0x0002, regs + TB_ADDR_XCVR_PSM_A0OUT_TMR);
	writel(0x0002, regs + TB_ADDR_XCVR_PSM_A1OUT_TMR);
	writel(0x0002, regs + TB_ADDR_XCVR_PSM_A2OUT_TMR);
	writel(0x0002, regs + TB_ADDR_XCVR_PSM_A3OUT_TMR);
	writel(0x0002, regs + TB_ADDR_XCVR_PSM_A4OUT_TMR);
	writel(0x0002, regs + TB_ADDR_XCVR_PSM_A5OUT_TMR);

	/* Change rx detect parameter */
	writel(0x960, regs + TB_ADDR_TX_RCVDET_EN_TMR);
	writel(0x01e0, regs + TB_ADDR_TX_RCVDET_ST_TMR);
	writel(0x0090, regs + TB_ADDR_XCVR_DIAG_LANE_FCM_EN_MGN_TMR);

	/* RXDET_IN_P3_32KHZ, Receiver detect slow clock enable */
	value = readl(regs + TB_ADDR_TX_RCVDETSC_CTRL);
	value |= RXDET_IN_P3_32KHZ;
	writel(value, regs + TB_ADDR_TX_RCVDETSC_CTRL);

	udelay(10);

	pr_debug("end of %s\n", __func__);
}

static void cdns3_reset_core(struct cdns3 *cdns)
{
	u32 value;

	value = readl(cdns->none_core_regs + USB3_CORE_CTRL1);
	value |= ALL_SW_RESET;
	writel(value, cdns->none_core_regs + USB3_CORE_CTRL1);
	udelay(1);
}

static void cdns3_set_role(struct cdns3 *cdns, enum cdns3_roles role)
{
	u32 value;
	int timeout_us = 100000;

	if (role == CDNS3_ROLE_END)
		return;

	/* Wait clk value */
	value = readl(cdns->none_core_regs + USB3_SSPHY_STATUS);
	writel(value, cdns->none_core_regs + USB3_SSPHY_STATUS);
	udelay(1);
	value = readl(cdns->none_core_regs + USB3_SSPHY_STATUS);
	while ((value & 0xf0000000) != 0xf0000000 && timeout_us-- > 0) {
		value = readl(cdns->none_core_regs + USB3_SSPHY_STATUS);
		dev_dbg(cdns->dev, "clkvld:0x%x\n", value);
		udelay(1);
	}

	if (timeout_us <= 0)
		dev_err(cdns->dev, "wait clkvld timeout\n");

	/* Set all Reset bits */
	value = readl(cdns->none_core_regs + USB3_CORE_CTRL1);
	value |= ALL_SW_RESET;
	writel(value, cdns->none_core_regs + USB3_CORE_CTRL1);
	udelay(1);

	if (role == CDNS3_ROLE_HOST) {
		value = readl(cdns->none_core_regs + USB3_CORE_CTRL1);
		value = (value & ~MODE_STRAP_MASK) | HOST_MODE | OC_DISABLE;
		writel(value, cdns->none_core_regs + USB3_CORE_CTRL1);
		value &= ~PHYAHB_SW_RESET;
		writel(value, cdns->none_core_regs + USB3_CORE_CTRL1);
		mdelay(1);
		cdns3_usb_phy_init(cdns->phy_regs);
		/* Force B Session Valid as 1 */
		writel(0x0060, cdns->phy_regs + 0x380a4);
		mdelay(1);

		value = readl(cdns->none_core_regs + USB3_INT_REG);
		value |= HOST_INT1_EN;
		writel(value, cdns->none_core_regs + USB3_INT_REG);

		value = readl(cdns->none_core_regs + USB3_CORE_CTRL1);
		value &= ~ALL_SW_RESET;
		writel(value, cdns->none_core_regs + USB3_CORE_CTRL1);

		dev_dbg(cdns->dev, "wait xhci_power_on_ready\n");

		value = readl(cdns->none_core_regs + USB3_CORE_STATUS);
		timeout_us = 100000;
		while (!(value & HOST_POWER_ON_READY) && timeout_us-- > 0) {
			value = readl(cdns->none_core_regs + USB3_CORE_STATUS);
			udelay(1);
		}

		if (timeout_us <= 0)
			dev_err(cdns->dev, "wait xhci_power_on_ready timeout\n");

		mdelay(1);

		dev_dbg(cdns->dev, "switch to host role successfully\n");
	} else { /* gadget mode */
		value = readl(cdns->none_core_regs + USB3_CORE_CTRL1);
		value = (value & ~MODE_STRAP_MASK) | DEV_MODE;
		writel(value, cdns->none_core_regs + USB3_CORE_CTRL1);
		value &= ~PHYAHB_SW_RESET;
		writel(value, cdns->none_core_regs + USB3_CORE_CTRL1);

		cdns3_usb_phy_init(cdns->phy_regs);
		/* Force B Session Valid as 1 */
		writel(0x0060, cdns->phy_regs + 0x380a4);
		value = readl(cdns->none_core_regs + USB3_INT_REG);
		value |= DEV_INT_EN;
		writel(value, cdns->none_core_regs + USB3_INT_REG);

		value = readl(cdns->none_core_regs + USB3_CORE_CTRL1);
		value &= ~ALL_SW_RESET;
		writel(value, cdns->none_core_regs + USB3_CORE_CTRL1);

		dev_dbg(cdns->dev, "wait gadget_power_on_ready\n");

		value = readl(cdns->none_core_regs + USB3_CORE_STATUS);
		timeout_us = 100000;
		while (!(value & DEV_POWER_ON_READY) && timeout_us-- > 0) {
			value = readl(cdns->none_core_regs + USB3_CORE_STATUS);
			udelay(1);
		}

		if (timeout_us <= 0)
			dev_err(cdns->dev,
				"wait gadget_power_on_ready timeout\n");

		mdelay(1);

		dev_dbg(cdns->dev, "switch to gadget role successfully\n");
	}
}

static enum cdns3_roles cdns3_get_role(struct cdns3 *cdns)
{
	return cdns->roles[CDNS3_ROLE_HOST]
		? CDNS3_ROLE_HOST
		: CDNS3_ROLE_GADGET;
}

/**
 * cdns3_core_init_role - initialize role of operation
 * @cdns: Pointer to cdns3 structure
 *
 * Returns 0 on success otherwise negative errno
 */
static int cdns3_core_init_role(struct cdns3 *cdns, enum usb_dr_mode dr_mode)
{
	cdns->role = CDNS3_ROLE_END;
	if (dr_mode == USB_DR_MODE_UNKNOWN)
		dr_mode = USB_DR_MODE_OTG;

	/* Currently, only support gadget mode */
	if (dr_mode == USB_DR_MODE_OTG || dr_mode == USB_DR_MODE_HOST) {
		dev_err(cdns->dev, "doesn't support host and OTG, only for gadget\n");
		return -EPERM;
	}

	if (dr_mode == USB_DR_MODE_PERIPHERAL) {
		if (cdns3_gadget_init(cdns))
			dev_info(cdns->dev, "doesn't support gadget\n");
	}

	if (!cdns->roles[CDNS3_ROLE_HOST] && !cdns->roles[CDNS3_ROLE_GADGET]) {
		dev_err(cdns->dev, "no supported roles\n");
		return -ENODEV;
	}

	return 0;
}

static void cdns3_remove_roles(struct cdns3 *cdns)
{
	/* Only support gadget */
	cdns3_gadget_remove(cdns);
}

int cdns3_uboot_init(struct cdns3_device *cdns3_dev)
{
	struct device *dev = NULL;
	struct cdns3 *cdns;
	int ret;

	cdns = devm_kzalloc(dev, sizeof(*cdns), GFP_KERNEL);
	if (!cdns)
		return -ENOMEM;

	cdns->dev = dev;

	/*
	 * Request memory region
	 * region-0: nxp wrap registers
	 * region-1: xHCI
	 * region-2: Peripheral
	 * region-3: PHY registers
	 * region-4: OTG registers
	 */
	cdns->none_core_regs = (void __iomem *)cdns3_dev->none_core_base;
	cdns->xhci_regs = (void __iomem *)cdns3_dev->xhci_base;
	cdns->dev_regs = (void __iomem *)cdns3_dev->dev_base;
	cdns->phy_regs = (void __iomem *)cdns3_dev->phy_base;
	cdns->otg_regs = (void __iomem *)cdns3_dev->otg_base;
	cdns->index = cdns3_dev->index;

	ret = cdns3_enable_clks(cdns->index);
	if (ret)
		return ret;

	ret = cdns3_core_init_role(cdns, cdns3_dev->dr_mode);
	if (ret)
		goto err1;

	cdns->role = cdns3_get_role(cdns);
	dev_dbg(dev, "the init role is %d\n", cdns->role);
	cdns3_set_role(cdns, cdns->role);
	ret = cdns3_role_start(cdns, cdns->role);
	if (ret) {
		dev_err(dev, "can't start %s role\n", cdns3_role(cdns)->name);
		goto err2;
	}

	dev_dbg(dev, "Cadence USB3 core: probe succeed\n");

	list_add_tail(&cdns->list, &cdns3_list);

	return 0;

err2:
	cdns3_remove_roles(cdns);
err1:
	cdns3_disable_clks(cdns->index);
	return ret;
}

void cdns3_uboot_exit(int index)
{
	struct cdns3 *cdns;

	list_for_each_entry(cdns, &cdns3_list, list) {
		if (cdns->index != index)
			continue;

		cdns3_role_stop(cdns);
		cdns3_remove_roles(cdns);
		cdns3_reset_core(cdns);
		cdns3_disable_clks(index);

		list_del(&cdns->list);
		kfree(cdns);
		break;
	}
}

void cdns3_uboot_handle_interrupt(int index)
{
	struct cdns3 *cdns = NULL;

	list_for_each_entry(cdns, &cdns3_list, list) {
		if (cdns->index != index)
			continue;

		cdns3_role_irq_handler(cdns);
		break;
	}
}
