/*
 * Copyright (C) 2016 Freescale Semiconductor, Inc.
 * Copyright 2017 NXP
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

int get_margin_pos(uint64_t part_start, uint64_t part_end, unsigned long blksz,
				margin_pos_t *margin, int64_t offset, size_t num_bytes, bool allow_partial);

#endif
