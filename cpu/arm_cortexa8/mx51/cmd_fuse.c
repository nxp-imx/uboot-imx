/*
 * (C) Copyright 2008-2010 Freescale Semiconductor, Inc.
 * Terry Lv
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
#include <asm/arch/mx51.h>
#include <command.h>
#include <common.h>
#include <asm/arch/imx_fuse.h>

int do_fuseops(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int rc = 0;
	int bank = 0,
		row = 0,
		val = 0;

	switch (argc) {
	case 4:
		if (strcmp(argv[1], "read") == 0) {
			bank = simple_strtoul(argv[2], NULL, 16);
			row = simple_strtoul(argv[3], NULL, 16);

			fuse_read(bank, row);
		} else if (strcmp(argv[1], "blow") == 0) {
			fuse_blow_func(argv[2], argv[3]);
		} else {
			printf("It is too dangeous for you to use fuse command.\n");
			printf("Usage:\n%s\n", cmdtp->usage);
			rc = 1;
		}
		break;
	case 5:
		if (strcmp(argv[1], "blow") == 0) {
			bank = simple_strtoul(argv[2], NULL, 16);
			row = simple_strtoul(argv[3], NULL, 16);
			val = simple_strtoul(argv[4], NULL, 16);

			fuse_blow(bank, row, val);
		} else {
			printf("It is too dangeous for you to use fuse command.\n");
			printf("Usage:\n%s\n", cmdtp->usage);
			rc = 1;
		}
		break;
	default:
		rc = 1;
		printf("It is too dangeous for you to use fuse command.\n");
		printf("Usage:\n%s\n", cmdtp->usage);
		break;
	}

	return rc;
}

U_BOOT_CMD(
	fuse, 5, 1, do_fuseops,
	"Fuse sub system",
	"read <bank> <row>	- Read some fuses\n"
	"blow <bank> <row> <value>	- Blow some fuses\n"
	"blow scc <value>	- Blow scc value\n"
	"blow srk <value>	- Blow srk value\n"
	"blow fecmac <0x##:0x##:0x##:0x##:0x##:0x##>"
	"- Blow FEC Mac address\n");

