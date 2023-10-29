// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * NXP PF0900 regulator driver
 * Copyright 2023 NXP
 *
 */

#include <common.h>
#include <dm.h>
#include <log.h>
#include <linux/bitops.h>
#include <power/pf0900.h>
#include <power/pmic.h>
#include <power/regulator.h>

/**
 * struct pf0900_vrange - describe linear range of voltages
 *
 * @min_volt:	smallest voltage in range
 * @min_sel:	smallest selector in the range
 * @max_sel:	maximum selector in the range
 * @step:	how much voltage changes at each selector step
 */
struct pf0900_vrange {
	unsigned int	min_volt;
	u8		min_sel;
	u8		max_sel;
	unsigned int	step;
};

/**
 * struct pf0900_plat - describe regulator control registers
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
struct pf0900_plat {
	const char		*name;
	u8			enable_reg;
	u8			enablemask;
	u8			volt_reg;
	u8			volt_mask;
	struct pf0900_vrange	*ranges;
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

static struct pf0900_vrange pf0900_vaon_vranges[] = {
	PCA_RANGE(0,       0x00, 0x01, 0x00),
	PCA_RANGE(1800000, 0x00, 0x01, 0x00),
	PCA_RANGE(3000000, 0x00, 0x01, 0x00),
	PCA_RANGE(3300000, 0x00, 0x01, 0x00),
};

static struct pf0900_vrange pf0900_sw1_vranges[] = {
	PCA_RANGE(0,        0x00, 0x08, 0),
	PCA_RANGE(500000,   0x09, 0x91, 6250),
	PCA_RANGE(0,        0x92, 0x9E, 0),
	PCA_RANGE(1500000,  0x9F, 0x9F, 0),
	PCA_RANGE(1800000,  0xA0, 0xD8, 12500),
	PCA_RANGE(0,        0xD9, 0xDF, 0),
	PCA_RANGE(2800000,  0xE0, 0xF4, 25000),
	PCA_RANGE(0,        0xF5, 0xFF, 0),
};

static struct pf0900_vrange pf0900_sw2345_vranges[] = {
	PCA_RANGE(300000,   0x00, 0x00, 0),
	PCA_RANGE(450000,   0x01, 0x91, 6250),
	PCA_RANGE(0,        0x92, 0x9E, 0),
	PCA_RANGE(1500000,  0x9F, 0x9F, 0),
	PCA_RANGE(1800000,  0xA0, 0xD8, 12500),
	PCA_RANGE(0,        0xD9, 0xDF, 0),
	PCA_RANGE(2800000,  0xE0, 0xF4, 25000),
	PCA_RANGE(0,        0xF5, 0xFF, 0),
};

static struct pf0900_vrange pf0900_ldo1_vranges[] = {
	PCA_RANGE(750000,   0x00, 0x0F, 50000),
	PCA_RANGE(1800000,  0x10, 0x1F, 100000),
};

static struct pf0900_vrange pf0900_ldo23_vranges[] = {
	PCA_RANGE(650000,   0x00, 0x0D, 50000),
	PCA_RANGE(1400000,  0x0E, 0x0F, 100000),
	PCA_RANGE(1800000,  0x10, 0x1F, 100000),
};

static struct pf0900_plat pf0900_reg_data[] = {
	PCA_DATA("VAON", PF0900_REG_VAON_CFG1, VAON_1P8V,
		 PF0900_REG_VAON_CFG1, VAON_MASK,
		 pf0900_vaon_vranges),
	PCA_DATA("SW1", PF0900_REG_SW1_MODE, SW_RUN_MODE_PWM,
		 PF0900_REG_SW1_VRUN, SW_VRUN_MASK,
		 pf0900_sw1_vranges),
	PCA_DATA("SW2", PF0900_REG_SW2_MODE, SW_RUN_MODE_PWM,
		 PF0900_REG_SW2_VRUN, SW_VRUN_MASK,
		 pf0900_sw2345_vranges),
	PCA_DATA("SW3", PF0900_REG_SW3_MODE, SW_RUN_MODE_PWM,
		 PF0900_REG_SW3_VRUN, SW_VRUN_MASK,
		 pf0900_sw2345_vranges),
	PCA_DATA("SW4", PF0900_REG_SW4_MODE, SW_RUN_MODE_PWM,
		 PF0900_REG_SW4_VRUN, SW_VRUN_MASK,
		 pf0900_sw2345_vranges),
	PCA_DATA("SW5", PF0900_REG_SW5_MODE, SW_RUN_MODE_PWM,
		 PF0900_REG_SW5_VRUN, SW_VRUN_MASK,
		 pf0900_sw2345_vranges),
	PCA_DATA("LDO1", PF0900_REG_LDO1_RUN, LDO1_RUN_EN_MASK,
		 PF0900_REG_LDO1_RUN, VLDO1_RUN_MASK,
		 pf0900_ldo1_vranges),
	PCA_DATA("LDO2", PF0900_REG_LDO2_RUN, LDO2_RUN_EN_MASK,
		 PF0900_REG_LDO2_RUN, VLDO2_RUN_MASK,
		 pf0900_ldo23_vranges),
	PCA_DATA("LDO3", PF0900_REG_LDO3_RUN, LDO3_RUN_EN_MASK,
		 PF0900_REG_LDO3_RUN, VLDO3_RUN_MASK,
		 pf0900_ldo23_vranges),
};

static int vrange_find_value(struct pf0900_vrange *r, unsigned int sel,
			     unsigned int *val)
{
	if (!val || sel < r->min_sel || sel > r->max_sel)
		return -EINVAL;

	*val = r->min_volt + r->step * (sel - r->min_sel);
	return 0;
}

static int vrange_find_selector(struct pf0900_vrange *r, int val,
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

static int pf0900_get_enable(struct udevice *dev)
{
	struct pf0900_plat *plat = dev_get_plat(dev);
	int val;

	val = pmic_reg_read(dev->parent, plat->enable_reg);
	if (val < 0)
		return val;

	return (val & plat->enablemask);
}

static int pf0900_set_enable(struct udevice *dev, bool enable)
{
	int val = 0;
	int tmp = 0;
	struct pf0900_plat *plat = dev_get_plat(dev);

	if (enable)
		tmp = plat->enablemask;
	val = pmic_reg_read(dev->parent, plat->enable_reg);
	if (val < 0)
		return val;
	val = (tmp & plat->enablemask) | (val & (~plat->enablemask));

	return pmic_reg_write(dev->parent, plat->enable_reg, val);
}

static int pf0900_get_value(struct udevice *dev)
{
	struct pf0900_plat *plat = dev_get_plat(dev);
	unsigned int reg, tmp;
	int i, ret;

	ret = pmic_reg_read(dev->parent, plat->volt_reg);
	if (ret < 0)
		return ret;

	reg = ret;
	reg &= plat->volt_mask;

	if (!strcmp(plat->name, "VAON"))
		return plat->ranges[reg].min_volt;

	for (i = 0; i < plat->numranges; i++) {
		struct pf0900_vrange *r = &plat->ranges[i];

		if (!vrange_find_value(r, reg, &tmp))
			return tmp;
	}

	pr_err("Unknown voltage value read from pmic\n");

	return -EINVAL;
}

static int pf0900_set_value(struct udevice *dev, int uvolt)
{
	struct pf0900_plat *plat = dev_get_plat(dev);
	unsigned int sel;
	int i, val, found = 0;

	for (i = 0; i < plat->numranges; i++) {
		struct pf0900_vrange *r = &plat->ranges[i];

		if (!strcmp(plat->name, "VAON")) {
			if (r->min_volt == uvolt) {
				sel = i;
				found = 1;
				break;
			} else {
				continue;
			}
		}

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

static int pf0900_regulator_probe(struct udevice *dev)
{
	struct pf0900_plat *plat = dev_get_plat(dev);
	int i, type;

	type = dev_get_driver_data(dev_get_parent(dev));

	if (type != PF0900_TYPE_PF0900) {
		debug("Unknown PMIC type\n");
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(pf0900_reg_data); i++) {
		if (strcmp(dev->name, pf0900_reg_data[i].name))
			continue;

		*plat = pf0900_reg_data[i];

		return 0;
	}

	pr_err("Unknown regulator '%s'\n", dev->name);

	return -ENOENT;
}

static const struct dm_regulator_ops pf0900_regulator_ops = {
	.get_value	= pf0900_get_value,
	.set_value	= pf0900_set_value,
	.get_enable	= pf0900_get_enable,
	.set_enable	= pf0900_set_enable,
};

U_BOOT_DRIVER(pf0900_regulator) = {
	.name		= PF0900_REGULATOR_DRIVER,
	.id		= UCLASS_REGULATOR,
	.ops		= &pf0900_regulator_ops,
	.probe		= pf0900_regulator_probe,
	.plat_auto	= sizeof(struct pf0900_plat),
};
