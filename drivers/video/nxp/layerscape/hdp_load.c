/*
 * Copyright 2017-2018 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <command.h>
#include <asm/global_data.h>

#include "API_General.h"

DECLARE_GLOBAL_DATA_PTR;

int do_hdp(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	if (argc < 2)
		return 0;

	if (strncmp(argv[1], "load", 4) == 0) {
		unsigned long address = 0;
		unsigned long offset  = 0x2000;
		const int iram_size   = 0x10000;
		const int dram_size   = 0x8000;

		if (argc > 2) {
			address = simple_strtoul(argv[2], NULL, 0);
			if (argc > 3)
				offset = simple_strtoul(argv[3], NULL, 0);
		} else {
			printf("Missing address\n");
		}

		printf("Loading hdp firmware from 0x%016lx offset 0x%016lx\n",
		       address, offset);
		cdn_api_loadfirmware((unsigned char *)(address + offset),
				     iram_size,
				     (unsigned char *)(address + offset +
						       iram_size),
				     dram_size);
		printf("Loading hdp firmware Complete\n");
		/* do not turn off hdmi power or firmware load will be lost */
	} else {
		printf("test error argc %d\n", argc);
	}

	return 0;
}

/***************************************************/

U_BOOT_CMD(
	hdp,  CONFIG_SYS_MAXARGS, 1, do_hdp,
	"load hdmi firmware ",
	"[<command>] ...\n"
	"hdpload [address] [<offset>]\n"
	"        address - address where the binary image starts\n"
	"        <offset> - IRAM offset in the binary image (8192 default)\n"
	);
