/*
 * Copyright (C) 2016 Freescale Semiconductor, Inc.
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */
#ifndef __UTILS_H__
#define __UTILS_H__

#include <common.h>

#define ALIGN_BYTES 64 /*mmc block read/write need 64 bytes aligned */

struct margin_pos {
	/* which blk the read/write starts */
	uint64_t blk_start;
	/* which blk the read/write ends */
	uint64_t blk_end;
	/* start position inside the start blk */
	unsigned long start;
	/* end position inside the end blk */
	unsigned long end;
	/* how many blks can be read/write one time */
	unsigned long multi;
};
typedef struct margin_pos margin_pos_t;

int get_margin_pos(long part_start, long part_end, long blksz,
		   margin_pos_t *margin, long offset, size_t num_bytes,
		   bool allow_partial);

int read_from_partition_in_bytes(struct blk_desc *fs_dev_desc,
				 disk_partition_t *info,
				 int64_t offset, size_t num_bytes,
		void* buffer, size_t* out_num_read);

int write_to_partition_in_bytes(struct blk_desc *fs_dev_desc,
				disk_partition_t *info, int64_t offset,
				size_t num_bytes, void* buffer,
				size_t *out_num_write);

#endif
