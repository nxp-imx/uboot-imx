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
 *
 * Auto Generate file, please don't edit it
 *
 */

#ifndef __MACH_MX6_GPIO_H__
#define __MACH_MX6_GPIO_H__

#include <asm/arch/mx6.h>
#include <asm/arch/iomux.h>

#define IMX_GPIO_NR(bank, nr)		(((bank) - 1) * 32 + (nr))

void mxc_set_gpio_direction(iomux_pin_name_t pin, int is_input);

void mxc_set_gpio_dataout(iomux_pin_name_t pin, u32 data);

/*!
 * @file mach-mx6/gpio.h
 *
 * @brief Simple GPIO definitions and functions
 *
 * @ingroup GPIO_MX6
 */
#endif
