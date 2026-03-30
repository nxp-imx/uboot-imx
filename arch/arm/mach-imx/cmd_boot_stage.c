// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2024 NXP
 *
 * Command to query and display the boot stage from ROM API
 */

#include <command.h>
#include <asm/arch/sys_proto.h>

static const char *boot_stage_name(u32 stage)
{
	switch (stage) {
	case BT_STAGE_PRIMARY:
		return "primary";
	case BT_STAGE_SECONDARY:
		return "secondary";
	case BT_STAGE_RECOVERY:
		return "recovery";
	case BT_STAGE_USB:
		return "usb";
	default:
		return "unknown";
	}
}

static int do_boot_stage(struct cmd_tbl *cmdtp, int flag, int argc,
			 char *const argv[])
{
	int ret;
	u32 bstage;

	ret = rom_api_query_boot_infor(QUERY_BT_STAGE, &bstage);
	if (ret != ROM_API_OKAY) {
		printf("Failed to query boot stage (error: 0x%x)\n", ret);
		return CMD_RET_FAILURE;
	}

	printf("Boot Stage: %s (0x%x)\n", boot_stage_name(bstage), bstage);
	
	/* Return boot stage mapping: 0=primary, 1=secondary, 2=recovery, 3=other */
	switch (bstage) {
	case BT_STAGE_PRIMARY:
		return 0;
	case BT_STAGE_SECONDARY:
		return 1;
	case BT_STAGE_RECOVERY:
		return 2;
	default:
		return 3;
	}
}

U_BOOT_CMD(
	boot_stage, 1, 0, do_boot_stage,
	"Query and display the current boot stage",
	""
);
