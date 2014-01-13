/*
 * Copyright (C) 2014 Freescale Semiconductor, Inc.
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
 */

#ifndef __GPIO_EXP_H
#define __GPIO_EXP_H

#ifndef CONFIG_IOEXP_DEV_PINS_NUM
#define CONFIG_IOEXP_DEV_PINS_NUM 8
#endif

#ifndef CONFIG_IOEXP_DEVICES_NUM
#define CONFIG_IOEXP_DEVICES_NUM 1
#endif

/*Define for building ioexp gpio, port starts from 1, index starts from 0*/
#define IOEXP_GPIO_NR(port, index) \
	((((port)-1)*CONFIG_IOEXP_DEV_PINS_NUM)+ \
	 ((index)&(CONFIG_IOEXP_DEV_PINS_NUM-1)))

/*Get the device number from a ioexp gpio*/
#define IOEXP_GPIO_TO_DEVICE(gpio_nr) \
	(gpio_nr / CONFIG_IOEXP_DEV_PINS_NUM)

/*Get the port number from a ioexp gpio*/
#define IOEXP_GPIO_TO_PORT(gpio_nr) \
	((gpio_nr / CONFIG_IOEXP_DEV_PINS_NUM) + 1)

/*Get the pin number from a ioexp gpio*/
#define IOEXP_GPIO_TO_PIN(gpio_nr) \
	(gpio_nr % CONFIG_IOEXP_DEV_PINS_NUM)

/**
 * Make a GPIO an input.
 *
 * @param gpio	GPIO number
 * @return 0 if ok, -1 on error
 */
int gpio_exp_direction_input(unsigned gpio);

/**
 * Make a GPIO an output, and set its value.
 *
 * @param gpio	GPIO number
 * @param value	GPIO value (0 for low or 1 for high)
 * @return 0 if ok, -1 on error
 */
int gpio_exp_direction_output(unsigned gpio, int value);

/**
 * Setup the io expander's port.
 *
 * @param port	IO expander port , starts from 1
 * @param i2c_bus_id	i2c bus index, starts from 0
 * @param i2c_slave_addr	i2c address of the io expander port
 * @return 0 if ok, -1 on error
 */
int gpio_exp_setup_port(int port, int i2c_bus_id, int i2c_slave_addr);

#endif	/*__GPIO_EXP_H*/
