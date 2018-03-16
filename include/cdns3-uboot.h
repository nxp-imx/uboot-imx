/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier:     GPL-2.0
 */

#ifndef __CDNS3_UBOOT_H_
#define __CDNS3_UBOOT_H_

#include <linux/usb/otg.h>

struct cdns3_device {
	unsigned long none_core_base;
	unsigned long xhci_base;
	unsigned long dev_base;
	unsigned long phy_base;
	unsigned long otg_base;
	enum usb_dr_mode dr_mode;
	int index;
};

int cdns3_uboot_init(struct cdns3_device *dev);
void cdns3_uboot_exit(int index);
void cdns3_uboot_handle_interrupt(int index);
#endif /* __CDNS3_UBOOT_H_ */
