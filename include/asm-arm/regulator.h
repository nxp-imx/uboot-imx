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

struct anatop_regulator_data {
	char name[80];
	char *parent_name;
	int (*reg_register)(struct anatop_regulator_data *sreg);
	int (*set_voltage)(struct anatop_regulator_data *sreg, int uv);
	int (*get_voltage)(struct anatop_regulator_data *sreg);
	int (*set_current)(struct anatop_regulator_data *sreg, int uA);
	int (*get_current)(struct anatop_regulator_data *sreg);
	int (*enable)(struct anatop_regulator_data *sreg);
	int (*disable)(struct anatop_regulator_data *sreg);
	int (*is_enabled)(struct anatop_regulator_data *sreg);
	int (*set_mode)(struct anatop_regulator_data *sreg, int mode);
	int (*get_mode)(struct anatop_regulator_data *sreg);
	int (*get_optimum_mode)(struct anatop_regulator_data *sreg,
	int input_uV, int output_uV, int load_uA);
	int control_reg;
	int vol_bit_shift;
	int vol_bit_mask;
	int min_bit_val;
	int min_voltage;
	int max_voltage;
	int max_current;
};

int regul_list(int show_val);
int regul_set(char *vdd_name, int uv);
int regul_get(char *vdd_name);
int regul_set_core(int uv);
int regul_get_core(void);
int regul_set_periph(int uv);
int regul_get_periph(void);
