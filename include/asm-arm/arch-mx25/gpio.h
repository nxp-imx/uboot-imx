/*
 * Copyright 2009 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */
#ifndef __MACH_MX25_GPIO_H__
#define __MACH_MX25_GPIO_H__

#include <asm/arch/mx25.h>

static void _set_gpio_direction(u32 port, u32 index, int is_input);

void mxc_set_gpio_direction(iomux_pin_name_t pin, int is_input);

static void _set_gpio_dataout(u32 port, u32 index, u32 data);

void mxc_set_gpio_dataout(iomux_pin_name_t pin, u32 data);

/*!
 * @file mach-mx25/gpio.h
 *
 * @brief Simple GPIO definitions and functions
 *
 * @ingroup GPIO_MX25
 */
#endif
