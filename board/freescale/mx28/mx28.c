/*
 * (C) Copyright 2009-2010 Freescale Semiconductor, Inc.
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
#include <asm/arch/mx28.h>
#include <asm/arch/regs-pinctrl.h>
#include <asm/arch/pinctrl.h>

/* This should be removed after it's added into mach-types.h */
#ifndef MACH_TYPE_MX28EVK
#define MACH_TYPE_MX28EVK	2531
#endif

DECLARE_GLOBAL_DATA_PTR;

/* MMC pins */
static struct pin_desc mmc_pins_desc[] = {
	{ PINID_SSP0_DATA0, PIN_FUN1, PAD_12MA, PAD_3V3, 1 },
	{ PINID_SSP0_DATA1, PIN_FUN1, PAD_12MA, PAD_3V3, 1 },
	{ PINID_SSP0_DATA2, PIN_FUN1, PAD_12MA, PAD_3V3, 1 },
	{ PINID_SSP0_DATA3, PIN_FUN1, PAD_12MA, PAD_3V3, 1 },
	{ PINID_SSP0_DATA4, PIN_FUN1, PAD_12MA, PAD_3V3, 1 },
	{ PINID_SSP0_DATA5, PIN_FUN1, PAD_12MA, PAD_3V3, 1 },
	{ PINID_SSP0_DATA6, PIN_FUN1, PAD_12MA, PAD_3V3, 1 },
	{ PINID_SSP0_DATA7, PIN_FUN1, PAD_12MA, PAD_3V3, 1 },
	{ PINID_SSP0_CMD, PIN_FUN1, PAD_12MA, PAD_3V3, 1 },
	{ PINID_SSP0_DETECT, PIN_FUN1, PAD_12MA, PAD_3V3, 1 },
	{ PINID_SSP0_SCK, PIN_FUN1, PAD_12MA, PAD_3V3, 1 }
};

static struct pin_group mmc_pins = {
	.pins		= mmc_pins_desc,
	.nr_pins	= ARRAY_SIZE(mmc_pins_desc)
};

/* ENET pins */
static struct pin_desc enet_pins_desc[] = {
	{ PINID_ENET0_MDC, PIN_FUN1, PAD_8MA, PAD_3V3, 1 },
	{ PINID_ENET0_MDIO, PIN_FUN1, PAD_8MA, PAD_3V3, 1 },
	{ PINID_ENET0_RX_EN, PIN_FUN1, PAD_8MA, PAD_3V3, 1 },
	{ PINID_ENET0_RXD0, PIN_FUN1, PAD_8MA, PAD_3V3, 1 },
	{ PINID_ENET0_RXD1, PIN_FUN1, PAD_8MA, PAD_3V3, 1 },
	{ PINID_ENET0_TX_EN, PIN_FUN1, PAD_8MA, PAD_3V3, 1 },
	{ PINID_ENET0_TXD0, PIN_FUN1, PAD_8MA, PAD_3V3, 1 },
	{ PINID_ENET0_TXD1, PIN_FUN1, PAD_8MA, PAD_3V3, 1 },
	{ PINID_ENET_CLK, PIN_FUN1, PAD_8MA, PAD_3V3, 1 }
};

static struct pin_group enet_pins = {
	.pins		= enet_pins_desc,
	.nr_pins	= ARRAY_SIZE(enet_pins_desc)
};

/*
 * Functions
 */
int board_init(void)
{
	/* Will change it for MX28 EVK later */
	gd->bd->bi_arch_number = MACH_TYPE_MX28EVK;
	/* Adress of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM_1 + 0x100;

	return 0;
}

int dram_init(void)
{
	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;

	return 0;
}

u32 ssp_mmc_is_wp(void)
{
	return pin_gpio_get(PINID_SSP1_SCK);
}

void ssp_mmc_board_init(void)
{
	/* Set up MMC pins */
	pin_set_group(&mmc_pins);

	/* Power on the card slot */
	pin_set_type(PINID_PWM3, PIN_GPIO);
	pin_gpio_direction(PINID_PWM3, 1);
	pin_gpio_set(PINID_PWM3, 0);

	/* Wait 10 ms for card ramping up */
	udelay(10000);

	/* Set up WP pin */
	pin_set_type(PINID_SSP1_SCK, PIN_GPIO);
	pin_gpio_direction(PINID_SSP1_SCK, 0);
}

void enet_board_init(void)
{
	/* Set up ENET pins */
	pin_set_group(&enet_pins);

	/* Power on the external phy */
	pin_set_type(PINID_SSP1_DATA3, PIN_GPIO);
	pin_gpio_direction(PINID_SSP1_DATA3, 1);
	pin_gpio_set(PINID_SSP1_DATA3, 0);

	/* Reset the external phy */
	pin_set_type(PINID_ENET0_RX_CLK, PIN_GPIO);
	pin_gpio_direction(PINID_ENET0_RX_CLK, 1);
	pin_gpio_set(PINID_ENET0_RX_CLK, 0);
	udelay(200);
	pin_gpio_set(PINID_ENET0_RX_CLK, 1);
}
