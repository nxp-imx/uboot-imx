/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0
 *
 */

#ifndef __CDNS3_LINUX_COMPAT__
#define __CDNS3_LINUX_COMPAT__

#define dev_WARN(dev, format, arg...)	debug(format, ##arg)

static inline void *devm_kzalloc(struct device *dev, unsigned int size,
				 unsigned int flags)
{
	return kzalloc(size, flags);
}
#endif
