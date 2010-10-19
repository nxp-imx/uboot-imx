/*
 * (C) Copyright 2008-2010 Freescale Semiconductor, Inc.
 * Terry Lv
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

#include <linux/types.h>
#include <command.h>
#include <common.h>
#include <asm/clock.h>

int do_clkops(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int rc = 0;
	int freq = 0;

	switch (argc) {
	case 1:
		clk_info(ALL_CLK);
		break;
	case 2:
		if (strcmp(argv[1], "core") == 0)
			clk_info(CPU_CLK);
		else if (strcmp(argv[1], "periph") == 0)
			clk_info(PERIPH_CLK);
		else if (strcmp(argv[1], "ddr") == 0)
			clk_info(DDR_CLK);
		else if (strcmp(argv[1], "nfc") == 0)
			clk_info(NFC_CLK);
		else
			printf("Unsupported clock type!\n");
		break;
	case 3:
		freq = simple_strtoul(argv[2], NULL, 10);
		if (strcmp(argv[1], "core") == 0)
			clk_config(CONFIG_REF_CLK_FREQ, freq, CPU_CLK);
		else if (strcmp(argv[1], "periph") == 0)
			clk_config(CONFIG_REF_CLK_FREQ, freq, PERIPH_CLK);
		else if (strcmp(argv[1], "ddr") == 0)
			clk_config(CONFIG_REF_CLK_FREQ, freq, DDR_CLK);
		else if (strcmp(argv[1], "nfc") == 0)
			clk_config(CONFIG_REF_CLK_FREQ, freq, NFC_CLK);
		else
			printf("Unsupported clock type!\n");
		break;
	default:
		rc = 1;
		printf("Too much parameters.\n");
		printf("Usage:\n%s\n", cmdtp->usage);
		break;
	}

	return rc;
}

U_BOOT_CMD(
	clk, 3, 1, do_clkops,
	"Clock sub system",
	"Setup/Display clock\n"
	"clk - Display all clocks\n"
	"clk core <core clock in MHz> - Setup/Display core clock\n"
	"clk periph <peripheral clock in MHz> -"
	"Setup/Display peripheral clock\n"
	"clk ddr <DDR clock in MHz> - Setup/Display DDR clock\n"
	"clk nfc <NFC clk in MHz> - Setup/Display NFC clock\n"
	"Example:\n"
	"clk - Show various clocks\n"
	"clk core 665 - Set core clock to 665MHz\n"
	"clk periph 600 - Set peripheral clock to 600MHz\n"
	"clk ddr 166 - Set DDR clock to 166MHz");

