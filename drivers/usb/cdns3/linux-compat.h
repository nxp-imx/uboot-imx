/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0
 *
 */

#ifndef __CDNS3_LINUX_COMPAT__
#define __CDNS3_LINUX_COMPAT__

#define pr_debug(format, arg...)                debug(format, ##arg)
#define WARN(val, format, arg...)	debug(format, ##arg)
#define dev_WARN(dev, format, arg...)	debug(format, ##arg)
#define WARN_ON_ONCE(val)		debug("Error %d\n", val)

static inline void *devm_kzalloc(struct device *dev, unsigned int size,
				 unsigned int flags)
{
	return kzalloc(size, flags);
}
#endif
