/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2011, Marvell Semiconductor Inc.
 * Lei Wen <leiwen@marvell.com>
 */


#ifndef __CI_UDC_H__
#define __CI_UDC_H__
#include <usb/ehci-ci.h>

#define EP_MAX_PACKET_SIZE	0x200
#define EP0_MAX_PACKET_SIZE	64

struct ehci_mx6_phy_data {
	void __iomem *phy_addr;
	void __iomem *misc_addr;
	void __iomem *anatop_addr;
};

void ehci_mx6_phy_init(struct usb_ehci *ehci, struct ehci_mx6_phy_data *phy_data, int index);
#endif /* __CI_UDC_H__ */
