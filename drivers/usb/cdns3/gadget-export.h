/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2019 NXP
 */

#ifndef __CDNS3_GADGET_EXPORT_H
#define __CDNS3_GADGET_EXPORT_H

#ifdef CONFIG_USB_CDNS3_GADGET

int cdns3_gadget_init(struct cdns3 *cdns);
void cdns3_gadget_remove(struct cdns3 *cdns);
#else

static inline int cdns3_gadget_init(struct cdns3 *cdns)
{
	return -ENXIO;
}

static inline void cdns3_gadget_remove(struct cdns3 *cdns)
{
}

#endif

#endif /* __CDNS3_GADGET_EXPORT_H */
