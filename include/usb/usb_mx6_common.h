// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019 NXP
 *
 */

#ifndef __USB_MX6_COMMON_H__
#define __USB_MX6_COMMON_H__
#include <usb/ehci-ci.h>

struct ehci_mx6_phy_data {
	void __iomem *phy_addr;
	void __iomem *misc_addr;
	void __iomem *anatop_addr;
};

void ehci_mx6_phy_init(struct usb_ehci *ehci, struct ehci_mx6_phy_data *phy_data, int index);
#endif /* __USB_MX6_COMMON_H__ */
