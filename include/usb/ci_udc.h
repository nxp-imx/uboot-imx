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

int ehci_mx6_common_init(struct usb_ehci *ehci, int index);
#endif /* __CI_UDC_H__ */
