/*
 * Copyright (C) 2012 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <asm/arch/mx6.h>
#include <asm/arch/regs-anadig.h>
#include <asm/regulator.h>
#include <asm/errno.h>
#include <asm/io.h>
#include <common.h>

static int get_voltage(struct anatop_regulator_data *rdata)
{
	int uv;

	if (rdata->control_reg) {
		u32 val = (readl(rdata->control_reg) >>
			   rdata->vol_bit_shift) & rdata->vol_bit_mask;
		uv = rdata->min_voltage + (val - rdata->min_bit_val) * 25000;
#ifdef DEBUG
		printf("vddio = %d, val=%u\n", uv, val);
#endif
		return uv;
	} else {
		printf("Regulator not supported.\n");
		return -EOPNOTSUPP;
	}
}

static int set_voltage(struct anatop_regulator_data *rdata, int uv)
{
	u32 val, reg;

#ifdef DEBUG
	printf("%s: uv %d, min %d, max %d\n", __func__,
		uv, rdata->min_voltage, rdata->max_voltage);
#endif

	if (uv < rdata->min_voltage || uv > rdata->max_voltage) {
		printf("Voltage not supported!\n");
		return -EINVAL;
	}

	if (rdata->control_reg) {
		val = rdata->min_bit_val +
		      (uv - rdata->min_voltage) / 25000;

		reg = (readl(rdata->control_reg) &
			~(rdata->vol_bit_mask <<
			rdata->vol_bit_shift));
#ifdef DEBUG
		printf("%s: calculated val %d\n", __func__, val);
#endif
		writel((val << rdata->vol_bit_shift) | reg,
			     rdata->control_reg);
		return 0;
	} else {
		printf("Regulator not supported!\n");
		return -EOPNOTSUPP;
	}
}

static int enable(struct anatop_regulator_data *sreg)
{
	return 0;
}

static int disable(struct anatop_regulator_data *sreg)
{
	return 0;
}

static int is_enabled(struct anatop_regulator_data *sreg)
{
	return 1;
}

#if defined(CONFIG_MX6Q) || defined(CONFIG_MX6DL)
static struct anatop_regulator_data vdd_data_set[] = {
	{
	.name		= "vddpu",
	.set_voltage	= set_voltage,
	.get_voltage	= get_voltage,
	.enable		= enable,
	.disable	= disable,
	.is_enabled	= is_enabled,
	.control_reg	= (u32)(ANATOP_BASE_ADDR + HW_ANADIG_REG_CORE),
	.vol_bit_shift	= 9,
	.vol_bit_mask	= 0x1F,
	.min_bit_val	= 1,
	.min_voltage	= 725000,
	.max_voltage	= 1300000,
	},
	{
	.name		= "vddcore",
	.set_voltage	= set_voltage,
	.get_voltage	= get_voltage,
	.enable		= enable,
	.disable	= disable,
	.is_enabled	= is_enabled,
	.control_reg	= (u32)(ANATOP_BASE_ADDR + HW_ANADIG_REG_CORE),
	.vol_bit_shift	= 0,
	.vol_bit_mask	= 0x1F,
	.min_bit_val	= 1,
	.min_voltage	= 725000,
	.max_voltage	= 1300000,
	},
	{
	.name		= "vddsoc",
	.set_voltage	= set_voltage,
	.get_voltage	= get_voltage,
	.enable		= enable,
	.disable	= disable,
	.is_enabled	= is_enabled,
	.control_reg	= (u32)(ANATOP_BASE_ADDR + HW_ANADIG_REG_CORE),
	.vol_bit_shift	= 18,
	.vol_bit_mask	= 0x1F,
	.min_bit_val	= 1,
	.min_voltage	= 725000,
	.max_voltage	= 1300000,
	},
	{
	.name		= "vdd2p5",
	.set_voltage	= set_voltage,
	.get_voltage	= get_voltage,
	.enable		= enable,
	.disable	= disable,
	.is_enabled	= is_enabled,
	.control_reg	= (u32)(ANATOP_BASE_ADDR + HW_ANADIG_REG_2P5),
	.vol_bit_shift	= 8,
	.vol_bit_mask	= 0x1F,
	.min_bit_val	= 0,
	.min_voltage	= 2000000,
	.max_voltage	= 2775000,
	},
	{
	.name		= "vdd1p1",
	.set_voltage	= set_voltage,
	.get_voltage	= get_voltage,
	.enable		= enable,
	.disable	= disable,
	.is_enabled	= is_enabled,
	.control_reg	= (u32)(ANATOP_BASE_ADDR + HW_ANADIG_REG_1P1),
	.vol_bit_shift	= 8,
	.vol_bit_mask	= 0x1F,
	.min_bit_val	= 4,
	.min_voltage	= 800000,
	.max_voltage	= 1400000,
	},
	{
	.name		= "vdd3p0",
	.set_voltage	= set_voltage,
	.get_voltage	= get_voltage,
	.enable		= enable,
	.disable	= disable,
	.is_enabled	= is_enabled,
	.control_reg	= (u32)(ANATOP_BASE_ADDR + HW_ANADIG_REG_3P0),
	.vol_bit_shift	= 8,
	.vol_bit_mask	= 0x1F,
	.min_bit_val	= 7,
	.min_voltage	= 2800000,
	.max_voltage	= 3150000,
	},
};
#endif

int regul_list(int show_val)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(vdd_data_set); ++i) {
		if (show_val) {
			int uv = 0;

			uv = vdd_data_set[i].get_voltage(&vdd_data_set[i]);
			printf("%s\t\t%d\n", vdd_data_set[i].name, uv);
		} else
			printf("%s\n", vdd_data_set[i].name);
	}

	return 0;
}

int regul_set(char *vdd_name, int uv)
{
	int i = 0;
	struct anatop_regulator_data *vdd_data;

	for (i = 0; i < ARRAY_SIZE(vdd_data_set); ++i) {
		if (!strcmp(vdd_name, vdd_data_set[i].name)) {
			vdd_data = &vdd_data_set[i];
			break;
		}
	}

	if (i == ARRAY_SIZE(vdd_data_set)) {
		printf("No regulator as %s!\n", vdd_name);
		return -1;
	}

	return vdd_data->set_voltage(vdd_data, uv);
}

int regul_get(char *vdd_name)
{
	int i = 0;
	struct anatop_regulator_data *vdd_data;

	for (i = 0; i < ARRAY_SIZE(vdd_data_set); ++i) {
		if (!strcmp(vdd_name, vdd_data_set[i].name)) {
			vdd_data = &vdd_data_set[i];
			break;
		}
	}

	if (i == ARRAY_SIZE(vdd_data_set)) {
		printf("No regulator as %s!\n", vdd_name);
		return -1;
	}

	return vdd_data->get_voltage(vdd_data);
}

int regul_set_core(int uv)
{
#ifdef CONFIG_CORE_REGULATOR_NAME
	return regul_set(CONFIG_CORE_REGULATOR_NAME, uv);
#else
	printf("error: CONFIG_CORE_REGULATOR_NAME not set!\n");
	return -1;
#endif
}

int regul_get_core(void)
{
#ifdef CONFIG_CORE_REGULATOR_NAME
	return regul_get(CONFIG_CORE_REGULATOR_NAME);
#else
	printf("error: CONFIG_CORE_REGULATOR_NAME not set!\n");
	return -1;
#endif
}

int regul_set_periph(int uv)
{
#ifdef CONFIG_PERIPH_REGULATOR_NAME
	return regul_set(CONFIG_PERIPH_REGULATOR_NAME, uv);
#else
	printf("error: CONFIG_PERIPH_REGULATOR_NAME not set!\n");
	return -1;
#endif
}

int regul_get_periph(void)
{
#ifdef CONFIG_PERIPH_REGULATOR_NAME
	return regul_get(CONFIG_PERIPH_REGULATOR_NAME);
#else
	printf("error: CONFIG_PERIPH_REGULATOR_NAME not set!\n");
	return -1;
#endif
}

