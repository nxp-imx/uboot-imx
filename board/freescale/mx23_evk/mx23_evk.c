/*
 *
 * (c) 2008 Embedded Alley Solutions, Inc.
 *
 * Copyright (C) 2009-2011 Freescale Semiconductor, Inc.
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
#include <asm/arch/mx23.h>
#include <asm/arch/clkctrl.h>
#include <asm/arch/pinmux.h>
#include <asm/arch/spi.h>

DECLARE_GLOBAL_DATA_PTR;

#define KHz	1000
#define MHz	(1000 * KHz)

static void set_pinmux(void)
{

#if defined(CONFIG_SPI_SSP1)

	/* Configure SSP1 pins for ENC28j60: 8maA */
	REG_CLR(PINCTRL_BASE + PINCTRL_MUXSEL(4), 0x00003fff);

	REG_CLR(PINCTRL_BASE + PINCTRL_DRIVE(8), 0X03333333);
	REG_SET(PINCTRL_BASE + PINCTRL_DRIVE(8), 0x01111111);

	REG_CLR(PINCTRL_BASE + PINCTRL_PULL(2), 0x0000003f);
#endif

#if defined(CONFIG_SPI_SSP2)

	/* Configure SSP2 pins for ENC28j60: 8maA */
	REG_CLR(PINCTRL_BASE + PINCTRL_MUXSEL(0), 0x00000fc3);
	REG_SET(PINCTRL_BASE + PINCTRL_MUXSEL(0), 0x00000a82);

	REG_CLR(PINCTRL_BASE + PINCTRL_MUXSEL(1), 0x00030300);
	REG_SET(PINCTRL_BASE + PINCTRL_MUXSEL(1), 0x00020200);

	REG_CLR(PINCTRL_BASE + PINCTRL_DRIVE(0), 0X00333003);
	REG_SET(PINCTRL_BASE + PINCTRL_DRIVE(0), 0x00111001);

	REG_CLR(PINCTRL_BASE + PINCTRL_DRIVE(2), 0x00030000);
	REG_SET(PINCTRL_BASE + PINCTRL_DRIVE(2), 0x00010000);

	REG_CLR(PINCTRL_BASE + PINCTRL_DRIVE(3), 0x00000003);
	REG_SET(PINCTRL_BASE + PINCTRL_DRIVE(3), 0x00000001);

	REG_CLR(PINCTRL_BASE + PINCTRL_PULL(0), 0x00100039);
#endif

}

#define IO_DIVIDER	18
static void set_clocks(void)
{
	u32 ssp_source_clk, ssp_clk;
	u32 ssp_div = 1;
	u32 val = 0;

	/*
	 * Configure 480Mhz IO clock
	 */

	/* Ungate IO_CLK and set divider */
	REG_CLR(CLKCTRL_BASE + CLKCTRL_FRAC, FRAC_CLKGATEIO);
	REG_CLR(CLKCTRL_BASE + CLKCTRL_FRAC, 0x3f << FRAC_IOFRAC);
	REG_SET(CLKCTRL_BASE + CLKCTRL_FRAC, IO_DIVIDER << FRAC_IOFRAC);

	/*
	 * Set SSP CLK to desired value
	 */

	/* Calculate SSP_CLK divider relatively to 480Mhz IO_CLK*/
	ssp_source_clk = 480 * MHz;
	ssp_clk = CONFIG_SSP_CLK;
	ssp_div = (ssp_source_clk + ssp_clk - 1) / ssp_clk;

	/* Enable SSP clock */
	val = REG_RD(CLKCTRL_BASE + CLKCTRL_SSP);
	val &= ~SSP_CLKGATE;
	REG_WR(CLKCTRL_BASE + CLKCTRL_SSP, val);

	/* Wait while clock is gated */
	while (REG_RD(CLKCTRL_BASE + CLKCTRL_SSP) & SSP_CLKGATE)
		;

	/* Set SSP clock divider */
	val &= ~(0x1ff << SSP_DIV);
	val |= ssp_div << SSP_DIV;
	REG_WR(CLKCTRL_BASE + CLKCTRL_SSP, val);

	/* Wait until new divider value is set */
	while (REG_RD(CLKCTRL_BASE + CLKCTRL_SSP) & SSP_BUSY)
		;

	/* Set SSP clock source to IO_CLK */
	REG_SET(CLKCTRL_BASE + CLKCTRL_CLKSEQ, CLKSEQ_BYPASS_SSP);
	REG_CLR(CLKCTRL_BASE + CLKCTRL_CLKSEQ, CLKSEQ_BYPASS_SSP);
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
	gd->bd->bi_arch_number = MACH_TYPE_MX23EVK;

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
	printf("Board: MX23 EVK. \n");
	return 0;
}
