// SPDX-License-Identifier:     GPL-2.0+
/*
 * Copyright (C) 2015-2016 Freescale Semiconductor, Inc.
 */

#include <common.h>
#include <g_dnl.h>
#include "bcb.h"

int bcb_read_command(char *command)
{
	int ret = 0;
	char *p_block = NULL;
	uint offset_in_block = 0;
	uint blk_size = 0;

	if (command == NULL)
		return -1;

	ret = bcb_rw_block(true, &p_block, &blk_size, NULL, MISC_COMMAND, 32);
	if (ret) {
		printf("read_bootctl, bcb_rw_block read failed\n");
		return -1;
	}

	offset_in_block = MISC_COMMAND%blk_size;
	memcpy(command, p_block + offset_in_block, 32);
	free(p_block);

	return 0;
}
int bcb_write_command(char *bcb_command)
{
	int ret = 0;
	char *p_block = NULL;
	uint offset_in_block = 0;
	uint blk_size = 0;

	if (bcb_command == NULL)
		return -1;


	ret = bcb_rw_block(true, &p_block, &blk_size, NULL,  MISC_COMMAND, 32);
	if (ret) {
		printf("write_bootctl, bcb_rw_block read failed\n");
		return -1;
	}

	offset_in_block = MISC_COMMAND%blk_size;
	memcpy(p_block + offset_in_block, bcb_command, 32);

	ret = bcb_rw_block(false, NULL, NULL, p_block, MISC_COMMAND, 32);
	if (ret) {
		free(p_block);
		printf("write_bootctl, bcb_rw_block write failed\n");
		return -1;
	}

	free(p_block);
	return 0;
}

#ifdef CONFIG_ANDROID_RECOVERY
int bcb_write_recovery_opt(char *opts)
{
	int ret = 0;
	char *p_block = NULL;
	uint offset_in_block = 0;
	uint blk_size = 0;

	if (opts == NULL)
		return -1;


	ret = bcb_rw_block(true, &p_block, &blk_size, NULL, RECOVERY_OPTIONS, 32);
	if (ret) {
		printf("write_bootctl, bcb_rw_block read failed\n");
		return -1;
	}

	offset_in_block = RECOVERY_OPTIONS%blk_size;
	memcpy(p_block + offset_in_block, opts, 32);

	ret = bcb_rw_block(false, NULL, NULL, p_block, RECOVERY_OPTIONS, 32);
	if (ret) {
		free(p_block);
		printf("write_bootctl, bcb_rw_block write failed\n");
		return -1;
	}

	free(p_block);
	return 0;
}
#endif
