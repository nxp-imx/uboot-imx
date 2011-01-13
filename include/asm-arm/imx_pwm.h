/*
 * Copyright (C) 2011 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __IMX_PWM_H__
#define __IMX_PWM_H__

struct pwm_device {
	unsigned long mmio_base;
	unsigned int pwm_id;
	int pwmo_invert;
	void (*enable_pwm_pad)(void);
	void (*disable_pwm_pad)(void);
	void (*enable_pwm_clk)(void);
	void (*disable_pwm_clk)(void);
};

int imx_pwm_config(struct pwm_device pwm, int duty_ns, int period_ns);
int imx_pwm_enable(struct pwm_device pwm);
int imx_pwm_disable(struct pwm_device pwm);

#endif
