/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2016 Cadence Design Systems - https://www.cadence.com/
 * Copyright 2019 NXP
 */

#ifndef __DRIVERS_USB_CDNS_IO_H
#define __DRIVERS_USB_CDNS_IO_H

#include <linux/io.h>

static inline u32 cdns_readl(u32 __iomem *reg)
{
	return readl(reg);
}

static inline void cdns_writel(u32 __iomem *reg, u32 value)
{
	writel(value, reg);
}

static inline void cdns_flush_cache(uintptr_t addr, int length)
{
	flush_dcache_range(addr, addr + length);
}

#endif /* __DRIVERS_USB_CDNS_IO_H */
