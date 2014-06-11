/*
 * Copyright (C) 2014 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <command.h>

/* Allow for arch specific config before we boot */
static int __arch_auxiliary_core_up(u32 core_id, u32 boot_private_data)
{
	/* please define platform specific arch_auxiliary_core_up() */
	return CMD_RET_FAILURE;
}
int arch_auxiliary_core_up(u32 core_id, u32 boot_private_data)
	__attribute__((weak, alias("__arch_auxiliary_core_up")));

int do_bootaux(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	ulong addr;
	int ret;

	if (argc < 2)
		return CMD_RET_USAGE;

	addr = simple_strtoul(argv[1], NULL, 16);

	printf("## Starting auxiliary core at 0x%08lX ...\n", addr);

	ret = arch_auxiliary_core_up(0, addr);
	if (ret)
		return CMD_RET_FAILURE;

	return CMD_RET_SUCCESS;
}

U_BOOT_CMD(
	bootaux, CONFIG_SYS_MAXARGS, 1,	do_bootaux,
	"Start auxiliary core",
	""
);
