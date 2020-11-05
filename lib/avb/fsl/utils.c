/*
+ * Copyright (C) 2016 Freescale Semiconductor, Inc.
+ * Copyright 2018 NXP
+ *
+ * SPDX-License-Identifier:     GPL-2.0+
+ */
#include <common.h>
#include <stdlib.h>

#include "debug.h"
#include "utils.h"

/*
 * get margin_pos struct from offset [to the partition start/end] and
 * num_bytes to read/write
 */
int get_margin_pos(long part_start, long part_end, long blksz,
		   margin_pos_t *margin, long offset, size_t num_bytes,
		   bool allow_partial) {
	long off;
	if (margin == NULL)
		return -1;

	if (blksz == 0 || part_start > part_end)
		return -1;

	if (offset < 0) {
		margin->blk_start = (offset + 1) / blksz + part_end;
 		// offset == -1 means the last byte?, or start need -1
		margin->start = (off = offset % blksz) == 0 ?
			        0 : blksz + off;
		if (offset + num_bytes - 1 >= 0) {
			if (!allow_partial)
				return -1;
			margin->blk_end = part_end;
			margin->end = blksz - 1;
		} else {
 			// which blk the last byte is in
			margin->blk_end = (num_bytes + offset) /
					  blksz + part_end;
			margin->end = (off = (num_bytes + offset - 1) % blksz) == 0 ?
				      0 : blksz + off; // last byte
		}
	} else {
		margin->blk_start = offset / blksz + part_start;
		margin->start = offset % blksz;
		margin->blk_end = ((offset + num_bytes - 1) / blksz) +
				  part_start ;
		margin->end = (offset + num_bytes - 1) % blksz;
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

int read_from_partition_in_bytes(struct blk_desc *fs_dev_desc,
				 disk_partition_t *info, int64_t offset,
				 size_t num_bytes, void* buffer,
				 size_t* out_num_read)
{
	unsigned char *bdata;
	unsigned char *out_buf = (unsigned char *)buffer;
	unsigned char *dst, *dst64 = NULL;
	unsigned long blksz;
	unsigned long s, cnt;
	size_t num_read = 0;
	lbaint_t part_start, part_end, bs, be, bm, blk_num;
	margin_pos_t margin;
	int ret;

	if(buffer == NULL || out_num_read == NULL) {
		printf("NULL pointer error!\n");
		return -1;
	}

	blksz = fs_dev_desc->blksz;
	part_start = info->start;
	part_end = info->start + info->size - 1;

	if (get_margin_pos((uint64_t)part_start, (uint64_t)part_end, blksz,
			   &margin, offset, num_bytes, true))
		return -1;

	bs = (lbaint_t)margin.blk_start;
	be = (lbaint_t)margin.blk_end;
	s = margin.start;
	bm = margin.multi;

	/* alloc a blksz mem */
	bdata = (unsigned char *)memalign(ALIGN_BYTES, blksz);
	if (bdata == NULL) {
		printf("Failed to allocate memory!\n");
		return -1;
	}

	/* support multi blk read */
	while (bs <= be) {
		if (!s && bm > 1) {
			dst = out_buf;
 			/* for mmc blk read alignment */
			dst64 = PTR_ALIGN(out_buf, 64);
			if (dst64 != dst) {
				dst = dst64;
				bm--;
			}
			blk_num = bm;
			cnt = bm * blksz;
			bm = 0; /* no more multi blk */
		} else {
			blk_num = 1;
			cnt = blksz - s;
			if (num_read + cnt > num_bytes)
				cnt = num_bytes - num_read;
			dst = bdata;
		}
		if (!blk_dread(fs_dev_desc, bs, blk_num, dst)) {
			ret = -1;
			goto fail;
		}

		if (dst == bdata)
			memcpy(out_buf, bdata + s, cnt);
		else if (dst == dst64)
			memcpy(out_buf, dst, cnt); /* internal copy */

		s = 0;
		bs += blk_num;
		num_read += cnt;
		out_buf += cnt;
	}
	*out_num_read = num_read;
	ret = 0;

fail:
	free(bdata);
	return ret;
}

int write_to_partition_in_bytes(struct blk_desc *fs_dev_desc,
				disk_partition_t *info, int64_t offset,
				size_t num_bytes,
				void* buffer, size_t *out_num_write)
{
	unsigned char *bdata;
	unsigned char *in_buf = (unsigned char *)buffer;
	unsigned long blksz;
	unsigned long s, cnt;
	size_t num_write = 0;
	lbaint_t part_start, part_end, bs;
	margin_pos_t margin;
	int ret;

	if(buffer == NULL || out_num_write == NULL) {
		printf("NULL pointer error!\n");
		return -1;
	}

	blksz = fs_dev_desc->blksz;
	part_start = info->start;
	part_end = info->start + info->size - 1;

	if(get_margin_pos((uint64_t)part_start, (uint64_t)part_end, blksz,
			  &margin, offset, num_bytes, false))
		return -1;

	bs = (lbaint_t)margin.blk_start;
	s = margin.start;

	// alloc a blksz mem
	bdata = (unsigned char *)memalign(ALIGN_BYTES, blksz);
	if (bdata == NULL)
		return -1;

	while (num_write < num_bytes) {
		memset(bdata, 0, blksz);
		cnt = blksz - s;
		if (num_write + cnt >  num_bytes)
			cnt = num_bytes - num_write;
		if (!s || cnt != blksz) { //read blk first
			if (!blk_dread(fs_dev_desc, bs, 1,
						     bdata)) {
				ret = -1;
				goto fail;
			}
		}
		memcpy(bdata + s, in_buf, cnt); //change data
		if (!blk_dwrite(fs_dev_desc, bs, 1, bdata)) {
			ret = -1;
			goto fail;
		}
		bs++;
		num_write += cnt;
		in_buf += cnt;
		s = 0;
	}
	*out_num_write = num_write;
	ret = 0;

fail:
	free(bdata);
	return ret;
}
