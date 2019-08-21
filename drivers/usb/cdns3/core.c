// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2016 Cadence Design Systems - https://www.cadence.com/
 * Copyright 2019 NXP
 */
#include <common.h>
#include <malloc.h>
#include <wait_bit.h>
#include <asm/dma-mapping.h>
#include <asm/io.h>
#include <linux/bug.h>
#include <linux/list.h>
#include <linux/compat.h>
#include <linux/usb/gadget.h>
#include <linux/usb/otg.h>

#include "cdns3-nxp-reg-def.h"
#include "core.h"
#include "gadget-export.h"
#include "gadget.h"

static void cdns3_reset_core(struct cdns3 *cdns)
{
	/* Set all Reset bits */
	setbits_le32(cdns->none_core_regs + USB3_CORE_CTRL1, ALL_SW_RESET);
	udelay(1);
}

static int cdns3_host_role_set(struct cdns3 *cdns)
{
	int ret;

	struct cdns3_generic_peripheral *priv = container_of(cdns,
			struct cdns3_generic_peripheral, cdns3);

	clrsetbits_le32(cdns->none_core_regs + USB3_CORE_CTRL1,
			MODE_STRAP_MASK, HOST_MODE | OC_DISABLE);
	clrbits_le32(cdns->none_core_regs + USB3_CORE_CTRL1,
		     PHYAHB_SW_RESET);
	mdelay(1);
	generic_phy_init(&priv->phy);
	setbits_le32(cdns->phy_regs + TB_ADDR_TX_RCVDETSC_CTRL,
		     RXDET_IN_P3_32KHZ);
	udelay(10);
	/* Force B Session Valid as 1 */
	writel(SET_FORCE_B_SESS_VALID, cdns->phy_regs + USB2_PHY_AFE_BC_REG4);
	mdelay(1);

	setbits_le32(cdns->none_core_regs + USB3_INT_REG, HOST_INT1_EN);

	clrbits_le32(cdns->none_core_regs + USB3_CORE_CTRL1,
		     ALL_SW_RESET);

	dev_dbg(cdns->dev, "wait xhci_power_on_ready\n");
	ret = wait_for_bit_le32(cdns->none_core_regs + USB3_CORE_STATUS,
				HOST_POWER_ON_READY, true, 100, false);
	if (ret) {
		dev_err(cdns->dev, "wait xhci_power_on_ready timeout\n");
		return ret;
	}

	return 0;
}

static int cdns3_gadget_role_set(struct cdns3 *cdns)
{
	int ret;

	struct cdns3_generic_peripheral *priv = container_of(cdns,
			struct cdns3_generic_peripheral, cdns3);

	clrsetbits_le32(cdns->none_core_regs + USB3_CORE_CTRL1,
			MODE_STRAP_MASK, DEV_MODE);
	clrbits_le32(cdns->none_core_regs + USB3_CORE_CTRL1,
		     PHYAHB_SW_RESET);

	generic_phy_init(&priv->phy);
	setbits_le32(cdns->phy_regs + TB_ADDR_TX_RCVDETSC_CTRL,
		     RXDET_IN_P3_32KHZ);
	udelay(10);
	/* Force B Session Valid as 1 */
	writel(SET_FORCE_B_SESS_VALID, cdns->phy_regs + USB2_PHY_AFE_BC_REG4);
	setbits_le32(cdns->none_core_regs + USB3_INT_REG, DEV_INT_EN);

	clrbits_le32(cdns->none_core_regs + USB3_CORE_CTRL1,
		     ALL_SW_RESET);

	dev_dbg(cdns->dev, "wait gadget_power_on_ready\n");
	ret = wait_for_bit_le32(cdns->none_core_regs + USB3_CORE_STATUS,
				DEV_POWER_ON_READY, true, 100, false);
	if (ret) {
		dev_err(cdns->dev, "wait gadget_power_on_ready timeout\n");
		return ret;
	}

	return 0;
}

static int cdns3_set_role(struct cdns3 *cdns, enum cdns3_roles role)
{
	int ret;

	if (role == CDNS3_ROLE_END)
		return -EPERM;

	/* Wait clk value */
	writel(CLK_VLD, cdns->none_core_regs + USB3_SSPHY_STATUS);
	ret = wait_for_bit_le32(cdns->none_core_regs + USB3_SSPHY_STATUS,
				CLK_VLD, true, 100, false);
	if (ret) {
		dev_err(cdns->dev, "wait clkvld timeout\n");
		return ret;
	}

	cdns3_reset_core(cdns);

	if (role == CDNS3_ROLE_HOST) {
		cdns3_host_role_set(cdns);
		dev_dbg(cdns->dev, "switch to host role successfully\n");
	} else { /* gadget mode */
		cdns3_gadget_role_set(cdns);
		dev_dbg(cdns->dev, "switch to gadget role successfully\n");
	}

	return 0;
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
 * @dr_mode: Role mode of device
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

int cdns3_init(struct cdns3 *cdns)
{
	int ret;

	ret = cdns3_core_init_role(cdns, USB_DR_MODE_PERIPHERAL);

	cdns->role = cdns3_get_role(cdns);
	dev_dbg(dev, "the init role is %d\n", cdns->role);
	cdns3_set_role(cdns, cdns->role);
	ret = cdns3_role_start(cdns, cdns->role);
	if (ret) {
		dev_err(dev, "can't start %s role\n", cdns3_role(cdns)->name);
		goto err;
	}

	dev_dbg(dev, "Cadence USB3 core: probe succeed\n");

	return 0;

err:
	cdns3_remove_roles(cdns);

	return ret;
}

void cdns3_exit(struct cdns3 *cdns)
{
	cdns3_role_stop(cdns);
	cdns3_remove_roles(cdns);
	cdns3_reset_core(cdns);
}
