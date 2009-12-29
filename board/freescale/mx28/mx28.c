/*
 *
 * (c) 2008 Embedded Alley Solutions, Inc.
 *
 * (C) Copyright 2009 Freescale Semiconductor, Inc.
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
#include <asm/arch/clkctrl.h>
#include <asm/arch/pinmux.h>
#include <asm/arch/spi.h>

DECLARE_GLOBAL_DATA_PTR;

#define KHz	1000
#define MHz	(1000 * KHz)

static void set_pinmux(void)
{
}

#define IO_DIVIDER	18
static void set_clocks(void)
{
}

int dram_init(void)
{
	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;

	return 0;
}

int board_init(void)
{
	/* arch number of Freescale STMP 378x development board */
	/* gd->bd->bi_arch_number = MACH_TYPE_MX28_EVK; */

	/* adress of boot parameters */
	gd->bd->bi_boot_params = LINUX_BOOT_PARAM_ADDR;

	set_clocks();

	set_pinmux();

	/* Configure SPI on SSP1 or SSP2 */
	spi_init();

	return 0;
}

int misc_init_r(void)
{
	return 0;
}

int checkboard(void)
{
	printf("Board: MX28 EVK \n");
	return 0;
}
