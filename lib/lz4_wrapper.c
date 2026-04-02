// SPDX-License-Identifier: GPL 2.0+ OR BSD-3-Clause
/*
 * Copyright 2015 Google Inc.
 * Copyright 2025 NXP
 */

#include <compiler.h>
#include <image.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <asm/unaligned.h>
#include <u-boot/lz4.h>

/* lz4.c is unaltered (except removing unrelated code) from github.com/Cyan4973/lz4. */
#include "lz4.c"	/* #include for inlining, do not link! */

#define LZ4F_BLOCKUNCOMPRESSED_FLAG 0x80000000U

__rcode int ulz4fn(const void *src, size_t srcn, void *dst, size_t *dstn)
{
	const void *end = dst + *dstn;
	const void *in = src;
	void *out = dst;
	int has_block_checksum;
	int ret;
	*dstn = 0;

	{ /* With in-place decompression the header may become invalid later. */
		u32 magic;
		u8 flags, version, independent_blocks, has_content_size;
		u8 block_desc;

		if (srcn < sizeof(u32) + 3*sizeof(u8))
			return -EINVAL;	/* input overrun */

		magic = get_unaligned_le32(in);
		in += sizeof(u32);
		flags = *(u8 *)in;
		in += sizeof(u8);
		block_desc = *(u8 *)in;
		in += sizeof(u8);

		version = (flags >> 6) & 0x3;
		independent_blocks = (flags >> 5) & 0x1;
		has_block_checksum = (flags >> 4) & 0x1;
		has_content_size = (flags >> 3) & 0x1;

		/* We assume there's always only a single, standard frame. */
		if (magic != LZ4F_MAGIC || version != 1)
			return -EPROTONOSUPPORT;	/* unknown format */
		if ((flags & 0x03) || (block_desc & 0x8f))
			return -EINVAL;	/* reserved bits must be zero */
		if (!independent_blocks)
			return -EPROTONOSUPPORT; /* we can't support this yet */

		if (has_content_size) {
			if (srcn < sizeof(u32) + 3*sizeof(u8) + sizeof(u64))
				return -EINVAL;	/* input overrun */
			in += sizeof(u64);
		}
		/* Header checksum byte */
		in += sizeof(u8);
	}

	while (1) {
		u32 block_header, block_size;

		block_header = get_unaligned_le32(in);
		in += sizeof(u32);
		block_size = block_header & ~LZ4F_BLOCKUNCOMPRESSED_FLAG;

		if (in - src + block_size > srcn) {
			ret = -EINVAL;		/* input overrun */
			break;
		}

		if (!block_size) {
			ret = 0;	/* decompression successful */
			break;
		}

		if (block_header & LZ4F_BLOCKUNCOMPRESSED_FLAG) {
			size_t size = min((ptrdiff_t)block_size, (ptrdiff_t)(end - out));
			memcpy(out, in, size);
			out += size;
			if (size < block_size) {
				ret = -ENOBUFS;	/* output overrun */
				break;
			}
		} else {
			/* constant folding essential, do not touch params! */
			ret = LZ4_decompress_generic(in, out, block_size,
					end - out, endOnInputSize,
					decode_full_block, noDict, out, NULL, 0);
			if (ret < 0) {
				ret = -EPROTO;	/* decompression error */
				break;
			}
			out += ret;
		}

		in += block_size;
		if (has_block_checksum)
			in += sizeof(u32);
	}

	*dstn = out - dst;
	return ret;
}

__rcode int ulz4fn_legacy(const void *src, size_t srcn, void *dst, size_t *dstn)
{
	const void *end = dst + *dstn;
	const void *in = src;
	void *out = dst;
	int ret = 0;
	*dstn = 0;
	u32 block_size;
	u32 magic;

	if (srcn < sizeof(u32) * 2) {
		printf("lz4: wrong input image size!\n");
		return -EINVAL;
	}

	magic = get_unaligned_le32(in);
	if (magic != LZ4L_MAGIC) {
		printf("lz4: expect legacy lz4 magic (0x%x), but get 0x%x\n", LZ4L_MAGIC, magic);
		return -EPROTONOSUPPORT;
	}
	in += sizeof(u32);

	while (1) {
		if (in >= src + srcn) {
			// success
			ret = 0;
			break;
		}

		block_size = get_unaligned_le32(in);
		in += sizeof(u32);
		if (block_size == 0) {
			continue;
		}

		if (in - src + block_size > srcn) {
			printf("lz4: input overrun\n");
			ret = -EINVAL;
			break;
		}

		ret = LZ4_decompress_safe(in, out, block_size, end - out);
		if (ret < 0) {
			ret = -EPROTO;
			printf("lz4: decompression error!");
			break;
		}

		out += ret;
		in += block_size;
	}

	*dstn = out - dst;
	return ret;
}

__rcode int ulz4fn_auto(const void *src, size_t srcn, void *dst, size_t *dstn)
{
	u32 magic;

	magic = get_unaligned_le32(src);
	if (magic == LZ4L_MAGIC) {
		// lz4 legacy format
		return ulz4fn_legacy(src, srcn, dst, dstn);
	} else if (magic == LZ4F_MAGIC) {
		// standard lz4 format
		return ulz4fn(src, srcn, dst, dstn);
	} else {
		return -EPROTONOSUPPORT;
	}
}
