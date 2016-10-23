/*
 * Copyright (C) 2015-2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <g_dnl.h>
#ifdef CONFIG_FASTBOOT_STORAGE_NAND
#include <nand.h>
#endif
#include "bcb.h"

static char command[32];
#ifndef CONFIG_FASTBOOT_STORAGE_NAND
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

#else
#define ALIGN_BYTES 64
#define MISC_PAGES 3
static int read_command(char *command)
{
	char read_cmd[128];
	char *addr_str;
	char *nand_str;
	ulong misc_info_size;
	nand_info_t *nand = &nand_info[0];
	if (command == NULL)
		return -1;
	memset(read_cmd, 0, 128);
	misc_info_size = MISC_PAGES * nand->writesize;
	nand_str = (char *)memalign(ALIGN_BYTES, misc_info_size);
	sprintf(read_cmd, "nand read 0x%x ${misc_nand_offset} \
			0x%x", nand_str, misc_info_size);
	run_command(read_cmd, 0);
	/* The offset of bootloader_message is 1 PAGE.
	   The offset of bootloader_message and the size of misc info
	   need align with user space and recovery.
	*/
	addr_str = nand_str + nand->writesize;
	memcpy(command, (char *)addr_str, 32);
	free(nand_str);
	return 0;
}
static int write_command(char *command)
{
	char cmd[128];
	char *addr_str;
	char *nand_str;
	ulong misc_info_size;
	nand_info_t *nand = &nand_info[0];
	if (command == NULL)
		return -1;
	memset(cmd, 0, 128);
	misc_info_size = MISC_PAGES * nand->writesize;
	nand_str = (char *)memalign(ALIGN_BYTES, misc_info_size);
	sprintf(cmd, "nand read 0x%x ${misc_nand_offset} \
			0x%x", nand_str, misc_info_size);
	run_command(cmd, 0);
	/* the offset of bootloader_message is 1 PAGE*/
	addr_str = nand_str +  nand->writesize;
	memcpy((char *)addr_str, command, 32);
	/* erase 3 pages which hold BCB struct.*/
	sprintf(cmd, "nand erase ${misc_nand_offset} 0x%x",nand->erasesize);
	run_command(cmd, 0);
	sprintf(cmd, "nand write 0x%x ${misc_nand_offset} 0x%x",nand_str, misc_info_size);
	run_command(cmd, 0);
	free(nand_str);
	return 0;
}
#endif
bool recovery_check_and_clean_command(void)
{
	int ret;
	ret = read_command(command);
	if (ret < 0) {
		printf("read command failed\n");
		return false;
	}
	if (!strcmp(command, "boot-recovery")) {
		memset(command, 0, 32);
		write_command(command);
		return true;
	}
	return false;
}
bool fastboot_check_and_clean_command(void)
{
	int ret;
	ret = read_command(command);
	if (ret < 0) {
		printf("read command failed\n");
		return false;
	}
	if (!strcmp(command, "boot-bootloader")) {
		memset(command, 0, 32);
		write_command(command);
		return true;
	}

	return false;
}
void fastboot_enable_command(void)
{
	char fastboot_command[32];
	memcpy(fastboot_command,"boot-bootloader",32);
	write_command(fastboot_command);
}
