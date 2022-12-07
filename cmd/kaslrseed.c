// SPDX-License-Identifier: GPL-2.0+
/*
 * The 'kaslrseed' command takes bytes from the hardware random number
 * generator and uses them to set the kaslr-seed value in the chosen node.
 *
 * Copyright (c) 2021, Chris Morgan <macromorgan@hotmail.com>
 * Copyright (c) 2022 NXP
 */

#include <common.h>
#include <command.h>
#include <dm.h>
#include <hexdump.h>
#include <malloc.h>
#include <rng.h>
#include <fdt_support.h>
#include <kaslr.h>

static int do_kaslr_seed(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	int ret = CMD_RET_SUCCESS;

	if (!working_fdt) {
		printf("No FDT memory address configured. Please configure\n"
		       "the FDT address via \"fdt addr <address>\" command.\n"
		       "Aborting!\n");
		return CMD_RET_FAILURE;
	}

	ret = fdt_check_header(working_fdt);
	if (ret < 0) {
		printf("fdt_chosen: %s\n", fdt_strerror(ret));
		return CMD_RET_FAILURE;
	}
	ret = do_generate_kaslr(working_fdt);
        if (ret < 0)
		return CMD_RET_FAILURE;
	return ret;
}

#ifdef CONFIG_SYS_LONGHELP
static char kaslrseed_help_text[] =
	"[n]\n"
	"  - append random bytes to chosen kaslr-seed node\n";
#endif

U_BOOT_CMD(
	kaslrseed, 1, 0, do_kaslr_seed,
	"feed bytes from the hardware random number generator to the kaslr-seed",
	kaslrseed_help_text
);
