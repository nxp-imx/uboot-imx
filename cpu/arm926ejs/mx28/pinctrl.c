/*
 *
 * (C) Copyright 2009-2010 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <common.h>
#include <asm/arch/regs-pinctrl.h>
#include <asm/arch/pinctrl.h>

void pin_gpio_direction(u32 id, u32 output)
{
	u32 addr;
	u32 bank = PINID_2_BANK(id);
	u32 pin = PINID_2_PIN(id);

	addr = REGS_PINCTRL_BASE + HW_PINCTRL_DOE0;
	addr += 0x10 * bank;
	if (output)
		REG_SET_ADDR(addr, 1 << pin);
	else
		REG_CLR_ADDR(addr, 1 << pin);
}

u32 pin_gpio_get(u32 id)
{
	u32 addr, val;
	u32 bank = PINID_2_BANK(id);
	u32 pin = PINID_2_PIN(id);

	addr = REGS_PINCTRL_BASE + HW_PINCTRL_DIN0;
	addr += 0x10 * bank;
	val = REG_RD_ADDR(addr);

	return (val & (1 << pin)) >> pin;
}

void pin_gpio_set(u32 id, u32 val)
{
	u32 addr;
	u32 bank = PINID_2_BANK(id);
	u32 pin = PINID_2_PIN(id);

	addr = REGS_PINCTRL_BASE + HW_PINCTRL_DOUT0;
	addr += 0x10 * bank;
	if (val)
		REG_SET_ADDR(addr, 1 << pin);
	else
		REG_CLR_ADDR(addr, 1 << pin);
}

void pin_set_strength(u32 id, enum pad_strength strength)
{
	u32 addr;
	u32 bank = PINID_2_BANK(id);
	u32 pin = PINID_2_PIN(id);

	addr = REGS_PINCTRL_BASE + HW_PINCTRL_DRIVE0;
	addr += 0x40 * bank + 0x10 * (pin >> 3);
	pin &= 0x7;
	REG_CLR_ADDR(addr, 0x3 << (pin * 4));
	REG_SET_ADDR(addr, strength << (pin * 4));
}

void pin_set_voltage(u32 id, enum pad_voltage volt)
{
	u32 addr;
	u32 bank = PINID_2_BANK(id);
	u32 pin = PINID_2_PIN(id);

	addr = REGS_PINCTRL_BASE + HW_PINCTRL_DRIVE0;
	addr += 0x40 * bank + 0x10 * (pin >> 3);
	pin &= 0x7;
	if (volt == PAD_1V8)
		REG_CLR_ADDR(addr, 1 << (pin * 4 + 2));
	else
		REG_SET_ADDR(addr, 1 << (pin * 4 + 2));
}

void pin_set_pullup(u32 id, u32 pullup)
{
	u32 addr;
	u32 bank = PINID_2_BANK(id);
	u32 pin = PINID_2_PIN(id);

	addr = REGS_PINCTRL_BASE + HW_PINCTRL_PULL0;
	addr += 0x10 * bank;
	if (pullup)
		REG_SET_ADDR(addr, 1 << pin);
	else
		REG_CLR_ADDR(addr, 1 << pin);
}

void pin_set_type(u32 id, enum pin_fun cfg)
{
	u32 addr;
	u32 bank = PINID_2_BANK(id);
	u32 pin = PINID_2_PIN(id);

	addr = REGS_PINCTRL_BASE + HW_PINCTRL_MUXSEL0;
	addr += 0x20 * bank + 0x10 * (pin >> 4);
	pin &= 0xf;
	REG_CLR_ADDR(addr, 0x3 << (pin * 2));
	REG_SET_ADDR(addr, cfg << (pin * 2));
}

void pin_set_group(struct pin_group *pin_group)
{
	u32 p;
	struct pin_desc *pin;

	for (p = 0; p < pin_group->nr_pins; p++) {
		pin = &pin_group->pins[p];
		pin_set_type(pin->id, pin->fun);
		pin_set_strength(pin->id, pin->strength);
		pin_set_voltage(pin->id, pin->voltage);
		pin_set_pullup(pin->id, pin->pullup);
	}
}

