/*
 * Copyright (C) 2016 Cadence Design Systems - https://www.cadence.com/
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier:     GPL-2.0
 */

#ifndef __DRIVERS_USB_CDNS_IO_H
#define __DRIVERS_USB_CDNS_IO_H

#include <linux/io.h>

static inline u32 cdns_readl(uint32_t __iomem *reg)
{
	u32 value = 0;

	value = readl(reg);
	return value;
}

static inline void cdns_writel(uint32_t __iomem *reg, u32 value)
{
	writel(value, reg);
}

static inline void cdns_flush_cache(uintptr_t addr, int length)
{
	flush_dcache_range(addr, addr + ROUND(length, ARCH_DMA_MINALIGN));
}

#endif /* __DRIVERS_USB_CDNS_IO_H */
