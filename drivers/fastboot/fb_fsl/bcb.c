// SPDX-License-Identifier:     GPL-2.0+
/*
 * Copyright (C) 2015-2016 Freescale Semiconductor, Inc.
 * Copyright 2017 NXP
 */

#include <fb_fsl.h>
#include <linux/stat.h>
#include <linux/types.h>
#include <common.h>
#include <g_dnl.h>
#include <mmc.h>
#include "bcb.h"
#define ALIGN_BYTES 64 /*armv7 cache line need 64 bytes aligned */

static ulong get_block_size(char *ifname, int dev)
{
	struct blk_desc *dev_desc = NULL;

	dev_desc = blk_get_dev(ifname, dev);
	if (dev_desc == NULL) {
		printf("Block device %s %d not supported\n", ifname, dev);
		return 0;
	}

	return dev_desc->blksz;
}

static int do_write(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char *ep;
	struct blk_desc *dev_desc = NULL;
	int dev;
	int part = 0;
	disk_partition_t part_info;
	ulong offset = 0u;
	ulong limit = 0u;
	void *addr;
	uint blk;
	uint cnt;

	if (argc != 6) {
		cmd_usage(cmdtp);
		return 1;
	}

	dev = (int)simple_strtoul(argv[2], &ep, 16);
	if (*ep) {
		if (*ep != ':') {
			printf("Invalid block device %s\n", argv[2]);
			return 1;
		}
		part = (int)simple_strtoul(++ep, NULL, 16);
	}

	dev_desc = blk_get_dev(argv[1], dev);
	if (dev_desc == NULL) {
		printf("Block device %s %d not supported\n", argv[1], dev);
		return 1;
	}

	addr = (void *)simple_strtoul(argv[3], NULL, 16);
	blk = simple_strtoul(argv[4], NULL, 16);
	cnt = simple_strtoul(argv[5], NULL, 16);

	if (part != 0) {
		if (part_get_info(dev_desc, part, &part_info)) {
			printf("Cannot find partition %d\n", part);
			return 1;
		}
		offset = part_info.start;
		limit = part_info.size;
	} else {
		/* Largest address not available in block_dev_desc_t. */
		limit = ~0;
	}

	if (cnt + blk > limit) {
		printf("Write out of range\n");
		return 1;
	}

	if (blk_dwrite(dev_desc, offset + blk, cnt, addr) != cnt) {
		printf("Error writing blocks\n");
		return 1;
	}

	return 0;
}

U_BOOT_CMD(
	write,	6,	0,	do_write,
	"write binary data to a partition",
	"<interface> <dev[:part]> addr blk# cnt"
);

int bcb_rw_block(bool bread, char **ppblock,
		uint *pblksize, char *pblock_write, uint offset, uint size)
{
	int ret;
	char *argv[6];
	char addr_str[20];
	char cnt_str[8];
	char devpart_str[8];
	char block_begin_str[8];
	ulong blk_size = 0;
	uint blk_begin = 0;
	uint blk_end = 0;
	uint block_cnt = 0;
	char *p_block = NULL;
	unsigned int mmc_id;

	if (bread && ((ppblock == NULL) || (pblksize == NULL)))
		return -1;

	if (!bread && (pblock_write == NULL))
		return -1;

	mmc_id = mmc_get_env_dev();
	blk_size = get_block_size("mmc", mmc_id);
	if (blk_size == 0) {
		printf("bcb_rw_block, get_block_size return 0\n");
		return -1;
	}

	blk_begin = offset/blk_size;
	blk_end = (offset + size)/blk_size;
	block_cnt = 1 + (blk_end - blk_begin);

	sprintf(devpart_str, "0x%x:0x%x", mmc_id,
			fastboot_flash_find_index(FASTBOOT_PARTITION_MISC));
	sprintf(block_begin_str, "0x%x", blk_begin);
	sprintf(cnt_str, "0x%x", block_cnt);

	argv[0] = "rw"; /* not care */
	argv[1] = "mmc";
	argv[2] = devpart_str;
	argv[3] = addr_str;
	argv[4] = block_begin_str;
	argv[5] = cnt_str;

	if (bread) {
		p_block = (char *)memalign(ALIGN_BYTES, blk_size * block_cnt);
		if (NULL == p_block) {
			printf("bcb_rw_block, memalign %d bytes failed\n",
			(int)(blk_size * block_cnt));
			return -1;
		}
		sprintf(addr_str, "0x%x", (unsigned int)(uintptr_t)p_block);
		ret = do_raw_read(NULL, 0, 6, argv);
		if (ret) {
			free(p_block);
			printf("do_raw_read failed, ret %d\n", ret);
			return -1;
		}

		*ppblock = p_block;
		*pblksize = (uint)blk_size;
	} else {
		sprintf(addr_str, "0x%x", (unsigned int)(uintptr_t)pblock_write);
		ret = do_write(NULL, 0, 6, argv);
		if (ret) {
			printf("do_write failed, ret %d\n", ret);
			return -1;
		}
	}
	return 0;
}
