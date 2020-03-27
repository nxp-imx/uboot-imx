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

bool ci_udc_check_bus_active(ulong ehci_addr, ulong phy_addr, int index);

#endif /* __CI_UDC_H__ */
