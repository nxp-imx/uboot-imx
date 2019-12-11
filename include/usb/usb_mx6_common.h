// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019 NXP
 *
 */

#ifndef __USB_MX6_COMMON_H__
#define __USB_MX6_COMMON_H__
#include <usb/ehci-ci.h>

int ehci_mx6_common_init(struct usb_ehci *ehci, int index);
#endif /* __USB_MX6_COMMON_H__ */
