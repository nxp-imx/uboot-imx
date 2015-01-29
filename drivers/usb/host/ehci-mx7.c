/*
 * Copyright (c) 2009 Daniel Mack <daniel@caiaq.de>
 * Copyright (C) 2010-2015 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <usb.h>
#include <errno.h>
#include <linux/compiler.h>
#include <usb/ehci-fsl.h>
#include <asm/io.h>
#include <asm/arch/crm_regs.h>
#include <asm/arch/clock.h>
#include <asm/imx-common/iomux-v3.h>

#include "ehci.h"

#define USB_NC_OFFSET	0x200

#define UCTRL_PM		(1 << 9)	/* OTG Power Mask */
#define UCTRL_OVER_CUR_POL	(1 << 8) /* OTG Polarity of Overcurrent */
#define UCTRL_OVER_CUR_DIS	(1 << 7) /* Disable OTG Overcurrent Detection */

/* USBCMD */
#define UCMD_RUN_STOP           (1 << 0) /* controller run/stop */
#define UCMD_RESET		(1 << 1) /* controller reset */

/* Base address for this IP block is 0x02184800 */
struct usbnc_regs {
	u32	ctrl1;
	u32 ctrl2;
	u32 reserve1[11];
	u32 phy_ctrl2;
	u32 reserve2[6];
	u32 adp_cfg1;
	u32 reserve3;
	u32 adp_status;
};

static void usb_oc_config(int index)
{
	struct usbnc_regs *usbnc = (struct usbnc_regs *)(USB_BASE_ADDR +
			(0x10000 * index) + USB_NC_OFFSET);
	void __iomem *ctrl = (void __iomem *)(&usbnc->ctrl1);
	u32 val;

	val = __raw_readl(ctrl);
	val |= UCTRL_OVER_CUR_POL;
	__raw_writel(val, ctrl);

	val = __raw_readl(ctrl);
	val |= (UCTRL_OVER_CUR_DIS | UCTRL_PM);
	__raw_writel(val, ctrl);
}

int __weak board_ehci_hcd_init(int port)
{
	return 0;
}

int __weak board_ehci_power(int port, int on)
{
	return 0;
}

int ehci_hcd_init(int index, enum usb_init_type init,
		struct ehci_hccr **hccr, struct ehci_hcor **hcor)
{
	struct usb_ehci *ehci = (struct usb_ehci *)(USB_BASE_ADDR +
		(0x10000 * index));

	if (index > 3)
		return -EINVAL;
	enable_usboh3_clk(1);
	mdelay(1);

	/* Do board specific initialization */
	board_ehci_hcd_init(index);

	usb_oc_config(index);

	*hccr = (struct ehci_hccr *)((uint32_t)&ehci->caplength);
	*hcor = (struct ehci_hcor *)((uint32_t)*hccr +
			HC_LENGTH(ehci_readl(&(*hccr)->cr_capbase)));

	board_ehci_power(index, (init == USB_INIT_DEVICE) ? 0 : 1);
	if (init == USB_INIT_DEVICE)
		return 0;
	setbits_le32(&ehci->usbmode, CM_HOST);
	__raw_writel(CONFIG_MXC_USB_PORTSC, &ehci->portsc);
	setbits_le32(&ehci->portsc, USB_EN);

	mdelay(10);

	return 0;
}

int ehci_hcd_stop(int index)
{
	return 0;
}
