/*
+ * Copyright (C) 2016 Freescale Semiconductor, Inc.
+ * Copyright 2017 NXP
+ *
+ * SPDX-License-Identifier:     GPL-2.0+
+ */
#include <common.h>
#include <stdlib.h>

#include "debug.h"
#include "utils.h"

/* get margin_pos struct from offset [to the partition start/end] and num_bytes to read/write */
int get_margin_pos(uint64_t part_start, uint64_t part_end, unsigned long blksz,
				margin_pos_t *margin, int64_t offset, size_t num_bytes, bool allow_partial) {
	long off;
	assert(margin != NULL);
	if (blksz == 0 || part_start > part_end)
		return -1;

	if (offset < 0) {
		margin->blk_start = (offset + 1) / (uint64_t)blksz + part_end;
		margin->start = (off = offset % (uint64_t)blksz) == 0 ? 0 : blksz + off; // offset == -1 means the last byte?, or start need -1
		if (offset + num_bytes - 1 >= 0) {
			if (!allow_partial)
				return -1;
			margin->blk_end = part_end;
			margin->end = blksz - 1;
		} else {
			margin->blk_end = (num_bytes + offset) / (uint64_t)blksz + part_end; // which blk the last byte is in
			margin->end = (off = (num_bytes + offset - 1) % (uint64_t)blksz) == 0 ?
					0 : blksz + off; // last byte
		}
	} else {
		margin->blk_start = offset / (uint64_t)blksz + part_start;
		margin->start = offset % (uint64_t)blksz;
		margin->blk_end = (offset + num_bytes - 1) / (uint64_t)blksz + part_start ;
		margin->end = (offset + num_bytes - 1) % (uint64_t)blksz;
		if (margin->blk_end > part_end) {
			if (!allow_partial)
				return -1;
			margin->blk_end = part_end;
			margin->end = blksz - 1;
		}
	}
	VDEBUG("bs=%ld, be=%ld, s=%ld, e=%ld\n",
			margin->blk_start, margin->blk_end, margin->start, margin->end);

	if (margin->blk_start > part_end || margin->blk_start < part_start)
		return -1;
	long multi = margin->blk_end - margin->blk_start - 1 +
		    (margin->start == 0) + (margin->end == blksz -1);
	margin->multi = multi > 0 ? multi : 0;
	VDEBUG("bm=%ld\n", margin->multi);
	return 0;
}
