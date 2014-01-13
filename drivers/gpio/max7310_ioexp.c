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

#include <common.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <errno.h>
#include <i2c.h>
#include <gpio_exp.h>

#define MAX7310_REG_INPUT_PORT          0x00
#define MAX7310_REG_OUTPUT_PORT         0x01
#define MAX7310_REG_POLARITY            0x02
#define MAX7310_REG_CONFIGURATION       0x03
#define MAX7310_REG_TIMEOUT             0x04

enum max7310_gpio_direction {
	MAX7310_GPIO_DIRECTION_IN,
	MAX7310_GPIO_DIRECTION_OUT,
};

struct max7310_config_struct {
	int i2c_slave_addr;
	int i2c_bus_id;
};

static struct max7310_config_struct max7310_configs[CONFIG_IOEXP_DEVICES_NUM];


static int max7310_gpio_direction(unsigned int gpio,
	enum max7310_gpio_direction direction)
{
	unsigned int dev = IOEXP_GPIO_TO_DEVICE(gpio);
	unsigned int pin = IOEXP_GPIO_TO_PIN(gpio);
	unsigned char value, val2, val3;
	unsigned char chip;

	if (dev >= CONFIG_IOEXP_DEVICES_NUM)
		return -EPERM;

	chip = max7310_configs[dev].i2c_slave_addr;

	i2c_set_bus_num(max7310_configs[dev].i2c_bus_id);
	if (i2c_probe(chip))
		return -ENXIO;

	i2c_read(chip, MAX7310_REG_CONFIGURATION, 1, &value, 1);

	switch (direction) {
	case MAX7310_GPIO_DIRECTION_OUT:
		value &= ~(1 << pin);
		i2c_write(chip, MAX7310_REG_CONFIGURATION, 1, &value, 1);
		break;
	case MAX7310_GPIO_DIRECTION_IN:

		i2c_read(chip, MAX7310_REG_POLARITY, 1, &val2, 1);
		i2c_read(chip, MAX7310_REG_OUTPUT_PORT, 1, &val3, 1);

		value |= (1 << pin);
		val2 &= ~(1 << pin);
		val3 &= ~(1 << pin);

		i2c_write(chip, MAX7310_REG_POLARITY, 1, &val2, 1);
		i2c_write(chip, MAX7310_REG_CONFIGURATION, 1, &value, 1);
		i2c_write(chip, MAX7310_REG_OUTPUT_PORT, 1, &val3, 1);

		break;
	}

	return 0;
}

static int max7310_gpio_set_value(unsigned gpio, int value)
{
	unsigned int dev = IOEXP_GPIO_TO_DEVICE(gpio);
	unsigned int pin = IOEXP_GPIO_TO_PIN(gpio);
	unsigned char reg_val;
	unsigned char chip;

	if (dev >= CONFIG_IOEXP_DEVICES_NUM)
		return -EPERM;

	chip = max7310_configs[dev].i2c_slave_addr;

	i2c_set_bus_num(max7310_configs[dev].i2c_bus_id);
	if (i2c_probe(chip))
		return -ENXIO;

	i2c_read(chip, MAX7310_REG_OUTPUT_PORT, 1, &reg_val, 1);

	if (value)
		reg_val |= 1 << pin;
	else
		reg_val &= ~(1 << pin);

	i2c_write(chip, MAX7310_REG_OUTPUT_PORT, 1, &reg_val, 1);

	return 0;
}

static int max7310_gpio_get_value(unsigned gpio)
{
	unsigned int dev = IOEXP_GPIO_TO_DEVICE(gpio);
	unsigned int pin = IOEXP_GPIO_TO_PIN(gpio);
	unsigned char reg_val;
	unsigned char chip;

	if (dev >= CONFIG_IOEXP_DEVICES_NUM)
		return -EPERM;

	chip = max7310_configs[dev].i2c_slave_addr;

	i2c_set_bus_num(max7310_configs[dev].i2c_bus_id);
	if (i2c_probe(chip))
		return -ENXIO;

	i2c_read(chip, MAX7310_REG_INPUT_PORT, 1, &reg_val, 1);

	reg_val = (reg_val >> pin) & 0x01;

	return reg_val;
}

int gpio_exp_direction_input(unsigned gpio)
{
	int ret = max7310_gpio_direction(gpio, MAX7310_GPIO_DIRECTION_IN);

	if (ret < 0)
		return ret;

	return max7310_gpio_get_value(gpio);
}

int gpio_exp_direction_output(unsigned gpio, int value)
{
	int ret = max7310_gpio_direction(gpio, MAX7310_GPIO_DIRECTION_OUT);

	if (ret < 0)
		return ret;

	return max7310_gpio_set_value(gpio, value);
}

int gpio_exp_setup_port(int port, int i2c_bus_id, int i2c_slave_addr)
{
	if (port > CONFIG_IOEXP_DEVICES_NUM || port <= 0)
		return -EPERM;

	max7310_configs[port-1].i2c_bus_id = i2c_bus_id;
	max7310_configs[port-1].i2c_slave_addr = i2c_slave_addr;

	return 0;
}
