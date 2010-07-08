/*
 * (C) Copyright 2008-2010 Freescale Semiconductor, Inc.
 *
 * Copyright 2007, Freescale Semiconductor, Inc
 * Andy Fleming
 *
 * Based vaguely on the pxa mmc code:
 * (C) Copyright 2003
 * Kyle Harris, Nexus Technologies, Inc. kharris@nexus-tech.net
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
#include <asm/io.h>
#include <command.h>
#include <common.h>
#include <asm/imx_iim.h>

int do_iimops(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int bank = 0,
		row = 0,
		val = 0;

	if (argc < 3 || argc > 5)
		goto err_rtn;

	if (strcmp(argv[1], "read") == 0) {
		if (strcmp(argv[2], "fecmac") == 0) {
			if (3 == argc)
				iim_blow_func(argv[2], NULL);
			else
				goto err_rtn;
		} else {
			if (4 == argc) {
				bank = simple_strtoul(argv[2], NULL, 16);
				row = simple_strtoul(argv[3], NULL, 16);

				iim_read(bank, row);
			} else
				goto err_rtn;
		}
	} else if (strcmp(argv[1], "blow") == 0) {
		if (strcmp(argv[2], "fecmac") == 0) {
			if (4 == argc)
				iim_blow_func(argv[2], argv[3]);
			else
				goto err_rtn;
		} else {
			if (5 == argc) {
				bank = simple_strtoul(argv[2], NULL, 16);
				row = simple_strtoul(argv[3], NULL, 16);
				val = simple_strtoul(argv[4], NULL, 16);

				iim_blow(bank, row, val);
			} else
				goto err_rtn;
		}
	} else
		goto err_rtn;

	return 0;
err_rtn:
	printf("Invalid parameters!\n");
	printf("It is too dangeous for you to use iim command.\n");
	return 1;
}

U_BOOT_CMD(
	iim, 5, 1, do_iimops,
	"IIM sub system",
	"Warning: all numbers in parameter are in hex format!\n"
	"iim read <bank> <row>	- Read some fuses\n"
	"iim read fecmac	- Read FEC Mac address\n"
	"iim blow <bank> <row> <value>	- Blow some fuses\n"
	"iim blow fecmac <0x##:0x##:0x##:0x##:0x##:0x##>"
	"- Blow FEC Mac address");

