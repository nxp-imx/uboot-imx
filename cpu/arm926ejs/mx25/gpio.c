/*
 * (c) Copyright 2009 Freescale Semiconductors
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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

#include <common.h>
#include <asm/arch/mx25.h>
#include <asm/arch/mx25_pins.h>
#include <asm/arch/gpio.h>

enum gpio_reg {
	DR = 0x00,
	GDIR = 0x04,
	PSR = 0x08,
	ICR1 = 0x0C,
	ICR2 = 0x10,
	IMR = 0x14,
	ISR = 0x18,
};

struct gpio_port_addr {
	int num;
	int base;
};

struct gpio_port_addr gpio_port[4] = {
				{0, GPIO1_BASE},
				{1, GPIO2_BASE},
				{2, GPIO3_BASE},
				{3, GPIO4_BASE}
				};

/*
 * Set a GPIO pin's direction
 * @param port		pointer to a gpio_port
 * @param index		gpio pin index value (0~31)
 * @param is_input	0 for output; non-zero for input
 */
static void _set_gpio_direction(u32 port, u32 index, int is_input)
{
	u32 reg = gpio_port[port].base + GDIR;
	u32 l;

	l = __REG(reg);
	if (is_input)
		l &= ~(1 << index);
	else
		l |= 1 << index;
	__REG(reg) = l;
}


/*!
 * Exported function to set a GPIO pin's direction
 * @param pin		a name defined by \b iomux_pin_name_t
 * @param is_input	1 (or non-zero) for input; 0 for output
 */
void mxc_set_gpio_direction(iomux_pin_name_t pin, int is_input)
{
	u32 port;
	u32 gpio = IOMUX_TO_GPIO(pin);

	port = GPIO_TO_PORT(gpio);
	_set_gpio_direction(port, GPIO_TO_INDEX(gpio), is_input);
}

/*
 * Set a GPIO pin's data output
 * @param port		number of gpio port
 * @param index		gpio pin index value (0~31)
 * @param data		value to be set (only 0 or 1 is valid)
 */
static void _set_gpio_dataout(u32 port, u32 index, u32 data)
{
	u32 reg = gpio_port[port].base + DR;
	u32 l = 0;

	l = (__REG(reg) & (~(1 << index))) | (data << index);
	__REG(reg) = l;
}

/*!
 * Exported function to set a GPIO pin's data output
 * @param pin		a name defined by \b iomux_pin_name_t
 * @param data		value to be set (only 0 or 1 is valid)
 */

void mxc_set_gpio_dataout(iomux_pin_name_t pin, u32 data)
{
	u32 port;
	u32 gpio = IOMUX_TO_GPIO(pin);

	port = GPIO_TO_PORT(gpio);
	_set_gpio_dataout(port, GPIO_TO_INDEX(gpio), (data == 0) ? 0 : 1);
}

