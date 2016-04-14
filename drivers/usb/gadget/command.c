/*
 * Copyright (C) 2015-2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <g_dnl.h>
#include "bcb.h"

#ifndef CONFIG_FASTBOOT_STORAGE_NAND
static char command[32];
static int read_command(char *command)
{
	int ret = 0;
	char *p_block = NULL;
	uint offset_in_block = 0;
	uint blk_size = 0;

	if (command == NULL)
		return -1;

	ret = rw_block(true, &p_block, &blk_size, NULL, MISC_COMMAND, 32);
	if (ret) {
		printf("read_bootctl, rw_block read failed\n");
		return -1;
	}

	offset_in_block = MISC_COMMAND%blk_size;
	memcpy(command, p_block + offset_in_block, 32);

	return 0;
}
static int write_command(char *bcb_command)
{
	int ret = 0;
	char *p_block = NULL;
	uint offset_in_block = 0;
	uint blk_size = 0;

	if (bcb_command == NULL)
		return -1;


	ret = rw_block(true, &p_block, &blk_size, NULL,  MISC_COMMAND, 32);
	if (ret) {
		printf("write_bootctl, rw_block read failed\n");
		return -1;
	}

	offset_in_block = MISC_COMMAND%blk_size;
	memcpy(p_block + offset_in_block, bcb_command, 32);

	ret = rw_block(false, NULL, NULL, p_block, MISC_COMMAND, 32);
	if (ret) {
		free(p_block);
		printf("write_bootctl, rw_block write failed\n");
		return -1;
	}

	free(p_block);
	return 0;
}

int recovery_check_and_clean_command(void)
{
	int ret;
	ret = read_command(command);
	if (ret < 0) {
		printf("read command failed\n");
		return 0;
	}
	if (!strcmp(command, "boot-recovery")) {
		memset(command, 0, 32);
		write_command(command);
		return 1;
	}
	return 0;
}
int fastboot_check_and_clean_command(void)
{
	int ret;
	ret = read_command(command);
	if (ret < 0) {
		printf("read command failed\n");
		return 0;
	}
	if (!strcmp(command, "boot-bootloader")) {
		memset(command, 0, 32);
		write_command(command);
		return 1;
	}

	return 0;
}
#else
int recovery_check_and_clean_command(void)
{
	return 0;
}
int fastboot_check_and_clean_command(void)
{
	return 0;
}
#endif
