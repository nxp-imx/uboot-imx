// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * NXP PF5300 regulator driver
 * Copyright 2023 NXP
 */

#include <common.h>
#include <dm.h>
#include <log.h>
#include <linux/bitops.h>
#include <power/pf5300.h>
#include <power/pmic.h>
#include <power/regulator.h>

/**
 * struct pf5300_vrange - describe linear range of voltages
 *
 * @min_volt:	smallest voltage in range
 * @step:	how much voltage changes at each selector step
 * @min_sel:	smallest selector in the range
 * @max_sel:	maximum selector in the range
 */
struct pf5300_vrange {
	unsigned int	min_volt;
	u8		min_sel;
	u8		max_sel;
	unsigned int	step;
};

/**
 * struct pf5300_plat - describe regulator control registers
 *
 * @name:	name of the regulator. Used for matching the dt-entry
 * @enable_reg:	register address used to enable/disable regulator
 * @enablemask:	register mask used to enable/disable regulator
 * @volt_reg:	register address used to configure regulator voltage
 * @volt_mask:	register mask used to configure regulator voltage
 * @ranges:	pointer to ranges of regulator voltages and matching register
 *		values
 * @numranges:	number of voltage ranges pointed by ranges
 */
struct pf5300_plat {
	const char		*name;
	u8			enable_reg;
	u8			enablemask;
	u8			volt_reg;
	u8			volt_mask;
	struct pf5300_vrange	*ranges;
	unsigned int		numranges;
};

#define PCA_RANGE(_min, _sel_low, _sel_hi, _vstep) \
{ \
	.min_volt = (_min), .min_sel = (_sel_low), \
	.max_sel = (_sel_hi), .step = (_vstep),\
}

#define PCA_DATA(_name, enreg, enmask, vreg, vmask, _range) \
{ \
	.name = (_name), .enable_reg = (enreg), .enablemask = (enmask), \
	.volt_reg = (vreg), .volt_mask = (vmask), .ranges = (_range), \
	.numranges = ARRAY_SIZE(_range) \
}

static struct pf5300_vrange pf5300_sw1_vranges[] = {
	PCA_RANGE(500000,  0x00, 0x8C, 5000),
	PCA_RANGE(0,       0x8D, 0xFF, 0),
};

static struct pf5300_plat pf5301_reg_data[] = {
	PCA_DATA("SW1_2a", PF5300_REG_SW1_CTRL1, SW_MODE_PWM,
		 PF5300_REG_SW1_VOLT, SW1_VOLT_MASK,
		 pf5300_sw1_vranges),
};

static struct pf5300_plat pf5302_reg_data[] = {
	PCA_DATA("SW1_29", PF5300_REG_SW1_CTRL1, SW_MODE_PWM,
		 PF5300_REG_SW1_VOLT, SW1_VOLT_MASK,
		 pf5300_sw1_vranges),
};

static int vrange_find_value(struct pf5300_vrange *r, unsigned int sel,
			     unsigned int *val)
{
	if (!val || sel < r->min_sel || sel > r->max_sel)
		return -EINVAL;

	*val = r->min_volt + r->step * (sel - r->min_sel);
	return 0;
}

static int vrange_find_selector(struct pf5300_vrange *r, int val,
				unsigned int *sel)
{
	int ret = -EINVAL;
	int num_vals = r->max_sel - r->min_sel + 1;

	if (val >= r->min_volt &&
	    val <= r->min_volt + r->step * (num_vals - 1)) {
		if (r->step) {
			*sel = r->min_sel + ((val - r->min_volt) / r->step);
			ret = 0;
		} else {
			*sel = r->min_sel;
			ret = 0;
		}
	}
	return ret;
}

static int pf5300_get_enable(struct udevice *dev)
{
	struct pf5300_plat *plat = dev_get_plat(dev);
	int val;

	val = pmic_reg_read(dev->parent, plat->enable_reg);
	if (val < 0)
		return val;

	return (val & plat->enablemask);
}

static int pf5300_set_enable(struct udevice *dev, bool enable)
{
	int val = 0;
	int tmp = 0;
	struct pf5300_plat *plat = dev_get_plat(dev);

	if (enable)
		tmp = plat->enablemask;
	val = pmic_reg_read(dev->parent, plat->enable_reg);
	if (tmp < 0)
		return tmp;
	val = (tmp & plat->enablemask) | (val & (~plat->enablemask));

	return pmic_reg_write(dev->parent, plat->enable_reg, val);
}

static int pf5300_get_value(struct udevice *dev)
{
	struct pf5300_plat *plat = dev_get_plat(dev);
	unsigned int reg, tmp;
	int i, ret;

	ret = pmic_reg_read(dev->parent, plat->volt_reg);
	if (ret < 0)
		return ret;

	reg = ret;
	reg &= plat->volt_mask;

	for (i = 0; i < plat->numranges; i++) {
		struct pf5300_vrange *r = &plat->ranges[i];

		if (!vrange_find_value(r, reg, &tmp))
			return tmp;
	}

	pr_err("Unknown voltage value read from pmic\n");

	return -EINVAL;
}

static int pf5300_set_value(struct udevice *dev, int uvolt)
{
	struct pf5300_plat *plat = dev_get_plat(dev);
	unsigned int sel;
	int i, val, found = 0;

	for (i = 0; i < plat->numranges; i++) {
		struct pf5300_vrange *r = &plat->ranges[i];

		found = !vrange_find_selector(r, uvolt, &sel);
		if (found) {
			unsigned int tmp;

			/*
			 * We require exactly the requested value to be
			 * supported - this can be changed later if needed
			 */
			found = !vrange_find_value(r, sel, &tmp);
			if (found && tmp == uvolt)
				break;
			found = 0;
		}
	}

	if (!found)
		return -EINVAL;

	val = pmic_reg_read(dev->parent, plat->volt_reg);
	if (val < 0)
		return val;
	val = (sel & plat->volt_mask) | (val & (~plat->volt_mask));
	return pmic_reg_write(dev->parent, plat->volt_reg, val);
}

static int pf5300_regulator_probe(struct udevice *dev)
{
	struct pf5300_plat *plat = dev_get_plat(dev);
	int i, type;

	type = dev_get_driver_data(dev_get_parent(dev));

	if (type != PF5300_TYPE_PF5300 && type != PF5300_TYPE_PF5301 &&
	    type != PF5300_TYPE_PF5302) {
		debug("Unknown PMIC type\n");
		return -EINVAL;
	}

	if (type == PF5300_TYPE_PF5301) {
		for (i = 0; i < ARRAY_SIZE(pf5301_reg_data); i++) {
			if (strcmp(dev->name, pf5301_reg_data[i].name))
				continue;
			*plat = pf5301_reg_data[i];

			return 0;
		}
	} else if (type == PF5300_TYPE_PF5302) {
		for (i = 0; i < ARRAY_SIZE(pf5302_reg_data); i++) {
			if (strcmp(dev->name, pf5302_reg_data[i].name))
				continue;
			*plat = pf5301_reg_data[i];

			return 0;
		}
	}

	pr_err("Unknown regulator '%s'\n", dev->name);

	return -ENOENT;
}

static const struct dm_regulator_ops pf5300_regulator_ops = {
	.get_value	= pf5300_get_value,
	.set_value	= pf5300_set_value,
	.get_enable	= pf5300_get_enable,
	.set_enable	= pf5300_set_enable,
};

U_BOOT_DRIVER(pf5300_regulator) = {
	.name		= PF5300_REGULATOR_DRIVER,
	.id		= UCLASS_REGULATOR,
	.ops		= &pf5300_regulator_ops,
	.probe		= pf5300_regulator_probe,
	.plat_auto	= sizeof(struct pf5300_plat),
};
