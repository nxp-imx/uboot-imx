// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2016, NVIDIA CORPORATION.
 */

#define LOG_CATEGORY UCLASS_POWER_DOMAIN

#include <common.h>
#include <dm.h>
#include <log.h>
#include <malloc.h>
#include <power-domain.h>
#include <power-domain-uclass.h>
#include <dm/uclass-internal.h>
#include <dm/device-internal.h>

static inline struct power_domain_ops *power_domain_dev_ops(struct udevice *dev)
{
	return (struct power_domain_ops *)dev->driver->ops;
}

static int power_domain_of_xlate_default(struct power_domain *power_domain,
					 struct ofnode_phandle_args *args)
{
	debug("%s(power_domain=%p)\n", __func__, power_domain);

	if (args->args_count != 1) {
		debug("Invalid args_count: %d\n", args->args_count);
		return -EINVAL;
	}

	power_domain->id = args->args[0];

	return 0;
}

int power_domain_lookup_name(const char *name, struct power_domain *power_domain)
{
	struct udevice *dev;
	struct power_domain_ops *ops;
	int ret;

	debug("%s(power_domain=%p name=%s)\n", __func__, power_domain, name);

	ret = uclass_find_device_by_name(UCLASS_POWER_DOMAIN, name, &dev);
	if (!ret) {
		/* Probe the dev */
		ret = device_probe(dev);
		if (ret) {
			printf("Power domain probe device %s failed: %d\n", name, ret);
			return ret;
		}
		ops = power_domain_dev_ops(dev);

		power_domain->dev = dev;
		if (ops->of_xlate)
			ret = ops->of_xlate(power_domain, NULL);
		else
			ret = power_domain_of_xlate_default(power_domain, NULL);
		if (ret) {
			debug("of_xlate() failed: %d\n", ret);
			return ret;
		}

		ret = ops->request ? ops->request(power_domain) : 0;
		if (ret) {
			debug("ops->request() failed: %d\n", ret);
			return ret;
		}

		debug("%s ok: %s\n", __func__, dev->name);

		return 0;
	}

	printf("%s fail: %s, ret = %d\n", __func__, name, ret);
	return -EINVAL;
}

int power_domain_get_by_index(struct udevice *dev,
			      struct power_domain *power_domain, int index)
{
	struct ofnode_phandle_args args;
	int ret;
	struct udevice *dev_power_domain;
	struct power_domain_ops *ops;

	debug("%s(dev=%p, power_domain=%p)\n", __func__, dev, power_domain);

	ret = dev_read_phandle_with_args(dev, "power-domains",
					 "#power-domain-cells", 0, index,
					 &args);
	if (ret) {
		debug("%s: dev_read_phandle_with_args failed: %d\n",
		      __func__, ret);
		return ret;
	}

	ret = uclass_get_device_by_ofnode(UCLASS_POWER_DOMAIN, args.node,
					  &dev_power_domain);
	if (ret) {
		debug("%s: uclass_get_device_by_ofnode failed: %d\n",
		      __func__, ret);
		return ret;
	}
	ops = power_domain_dev_ops(dev_power_domain);

	power_domain->dev = dev_power_domain;
	if (ops->of_xlate)
		ret = ops->of_xlate(power_domain, &args);
	else
		ret = power_domain_of_xlate_default(power_domain, &args);
	if (ret) {
		debug("of_xlate() failed: %d\n", ret);
		return ret;
	}

	ret = ops->request ? ops->request(power_domain) : 0;
	if (ret) {
		debug("ops->request() failed: %d\n", ret);
		return ret;
	}

	return 0;
}

int power_domain_get_by_name(struct udevice *dev,
			     struct power_domain *power_domain, const char *name)
{
	int index;

	index = dev_read_stringlist_search(dev, "power-domain-names", name);
	if (index < 0) {
		debug("fdt_stringlist_search() failed: %d\n", index);
		return index;
	}

	return power_domain_get_by_index(dev, power_domain, index);
}

int power_domain_get(struct udevice *dev, struct power_domain *power_domain)
{
	return power_domain_get_by_index(dev, power_domain, 0);
}

int power_domain_free(struct power_domain *power_domain)
{
	struct power_domain_ops *ops = power_domain_dev_ops(power_domain->dev);

	debug("%s(power_domain=%p)\n", __func__, power_domain);

	return ops->rfree ? ops->rfree(power_domain) : 0;
}

int power_domain_on(struct power_domain *power_domain)
{
	struct power_domain_ops *ops = power_domain_dev_ops(power_domain->dev);

	debug("%s(power_domain=%p)\n", __func__, power_domain);

	return ops->on ? ops->on(power_domain) : 0;
}

int power_domain_off(struct power_domain *power_domain)
{
	struct power_domain_ops *ops = power_domain_dev_ops(power_domain->dev);

	debug("%s(power_domain=%p)\n", __func__, power_domain);

	return ops->off ? ops->off(power_domain) : 0;
}

#if CONFIG_IS_ENABLED(OF_REAL)
static int dev_power_domain_ctrl(struct udevice *dev, bool on)
{
	struct power_domain pd;
	int i, count, ret = 0;

	count = dev_count_phandle_with_args(dev, "power-domains",
					    "#power-domain-cells", 0);
	for (i = 0; i < count; i++) {
		ret = power_domain_get_by_index(dev, &pd, i);
		if (ret)
			return ret;
		if (on)
			ret = power_domain_on(&pd);
		else
			ret = power_domain_off(&pd);

		if (ret)
			return ret;

		if (count > 0 && !on && dev_get_parent(dev) == pd.dev)
			return ret;

		if (count > 0 && !on)
			device_remove(pd.dev, DM_REMOVE_NORMAL);
	}

	return ret;
}

int dev_power_domain_on(struct udevice *dev)
{
	return dev_power_domain_ctrl(dev, true);
}

int dev_power_domain_off(struct udevice *dev)
{
	return dev_power_domain_ctrl(dev, false);
}
#endif  /* OF_REAL */

UCLASS_DRIVER(power_domain) = {
	.id		= UCLASS_POWER_DOMAIN,
	.name		= "power_domain",
};
