/*
 * Copyright (C) 2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <part.h>
#include <stdlib.h>

#include <fsl_fastboot.h>
#include <fsl_caam.h>
#include "../../../drivers/usb/gadget/fastboot_lock_unlock.h"

#include "fsl_avb.h"
#include "fsl_avbkey.h"
#include "debug.h"

#define FSL_AVB_DEV "mmc"

#define ALIGN_BYTES 64 /*mmc block read/write need 64 bytes aligned */

struct margin_pos {
	/* which blk the read/write starts */
	lbaint_t blk_start;
	/* which blk the read/write ends */
	lbaint_t blk_end;
	/* start position inside the start blk */
	unsigned long start;
	/* end position inside the end blk */
	unsigned long end;
	/* how many blks can be read/write one time */
	unsigned long multi;
};
typedef struct margin_pos margin_pos_t;


static block_dev_desc_t *fs_dev_desc = NULL;
static block_dev_desc_t *get_mmc_desc(void) {
	int dev_no = mmc_get_env_devno();
	return get_dev(FSL_AVB_DEV, dev_no);
}

/* get margin_pos struct from offset [to the partition start/end] and num_bytes to read/write */
static int32_t get_margin_pos(lbaint_t part_start, lbaint_t part_end, unsigned long blksz,
				margin_pos_t *margin, int64_t offset, size_t num_bytes, bool allow_partial) {
	long off;
	if (offset < 0) {
		margin->blk_start = (offset + 1) / blksz + part_end;
		margin->start = (off = offset % blksz) == 0 ? 0 : blksz + off; // offset == -1 means the last byte?, or start need -1
		if (offset + num_bytes - 1 >= 0) {
			if (!allow_partial)
				return -1;
			margin->blk_end = part_end;
			margin->end = blksz - 1;
		} else {
			margin->blk_end = (num_bytes + offset) / blksz + part_end; // which blk the last byte is in
			margin->end = (off = (num_bytes + offset - 1) % blksz) == 0 ?
					0 : blksz + off; // last byte
		}
	} else {
		margin->blk_start = offset / blksz + part_start;
		margin->start = offset % blksz;
		margin->blk_end = (offset + num_bytes - 1) / blksz + part_start ;
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
 /* Reads |num_bytes| from offset |offset| from partition with name
  * |partition| (NUL-terminated UTF-8 string). If |offset| is
  * negative, its absolute value should be interpreted as the number
  * of bytes from the end of the partition.
  *
  * This function returns AVB_IO_RESULT_ERROR_NO_SUCH_PARTITION if
  * there is no partition with the given name,
  * AVB_IO_RESULT_ERROR_RANGE_OUTSIDE_PARTITION if the requested
  * |offset| is outside the partition, and AVB_IO_RESULT_ERROR_IO if
  * there was an I/O error from the underlying I/O subsystem.  If the
  * operation succeeds as requested AVB_IO_RESULT_OK is returned and
  * the data is available in |buffer|.
  *
  * The only time partial I/O may occur is if reading beyond the end
  * of the partition. In this case the value returned in
  * |out_num_read| may be smaller than |num_bytes|.
  */
 AvbIOResult fsl_read_from_partition(AvbOps* ops, const char* partition,
				     int64_t offset, size_t num_bytes,
				     void* buffer, size_t* out_num_read)
{
	struct fastboot_ptentry *pte;
	unsigned char *bdata;
	unsigned char *out_buf = (unsigned char *)buffer;
	unsigned long blksz;
	unsigned long s, cnt;
	size_t num_read = 0;
	lbaint_t part_start, part_end, bs, be;
	margin_pos_t margin;

	AvbIOResult ret;

	DEBUGAVB("[%s]: offset=%ld, num_bytes=%zu\n", partition, (long)offset, num_bytes);

	assert(buffer != NULL && out_num_read != NULL);

	if (!fs_dev_desc && (fs_dev_desc = get_mmc_desc()) == NULL) {
		ERR("mmc device not found\n");
		return AVB_IO_RESULT_ERROR_NO_SUCH_PARTITION;
	}

	pte = fastboot_flash_find_ptn(partition);
	if (!pte) {
		ERR("no %s partition\n", partition);
		return AVB_IO_RESULT_ERROR_NO_SUCH_PARTITION;
	}

	blksz = fs_dev_desc->blksz;
	part_start = pte->start;
	part_end = pte->start + pte->length - 1;
	VDEBUG("blksz: %ld, part_end: %ld, part_start: %ld:\n",
			blksz, part_end, part_start);

	if(get_margin_pos(part_start, part_end, blksz,
				&margin, offset, num_bytes, true))
		return AVB_IO_RESULT_ERROR_RANGE_OUTSIDE_PARTITION;

	bs = margin.blk_start;
	be = margin.blk_end;
	s = margin.start;

	// alloc a blksz mem
	bdata = (unsigned char *)memalign(ALIGN_BYTES, blksz);
	if (bdata == NULL)
		return AVB_IO_RESULT_ERROR_OOM;

	// one block a time
	while (bs <= be) {
		memset(bdata, 0, blksz);
		if (!fs_dev_desc->block_read(fs_dev_desc->dev, bs, 1, bdata)) {
			ret = AVB_IO_RESULT_ERROR_IO;
			goto fail;
		}
		cnt = blksz - s;
		if (num_read + cnt > num_bytes)
			cnt = num_bytes - num_read;
		VDEBUG("cur: bs=%ld, start=%ld, cnt=%ld bdata=0x%08x\n",
				bs, s, cnt, bdata);
		memcpy(out_buf, bdata + s, cnt);
		bs++;
		num_read += cnt;
		out_buf += cnt;
		s = 0;
	}
	*out_num_read = num_read;
	ret = AVB_IO_RESULT_OK;
#ifdef AVB_VVDEBUG
	printf("\nnum_read=%zu", num_read);
	printf("\n----dump---\n");
	print_buffer(0, buffer, HEXDUMP_WIDTH, num_read, 0);
	printf("--- end ---\n");
#endif

fail:
	free(bdata);
	return ret;
}

/* multi block read version of read_from_partition */
 AvbIOResult fsl_read_from_partition_multi(AvbOps* ops, const char* partition,
					   int64_t offset, size_t num_bytes,
					   void* buffer, size_t* out_num_read)
{
	struct fastboot_ptentry *pte;
	unsigned char *bdata;
	unsigned char *out_buf = (unsigned char *)buffer;
	unsigned char *dst, *dst64 = NULL;
	unsigned long blksz;
	unsigned long s, cnt;
	size_t num_read = 0;
	lbaint_t part_start, part_end, bs, be, bm, blk_num;
	margin_pos_t margin;

	AvbIOResult ret;

	DEBUGAVB("[%s]: offset=%ld, num_bytes=%zu\n", partition, (long)offset, num_bytes);

	assert(buffer != NULL && out_num_read != NULL);

	if (!fs_dev_desc && (fs_dev_desc = get_mmc_desc()) == NULL) {
		ERR("mmc device not found\n");
		return AVB_IO_RESULT_ERROR_NO_SUCH_PARTITION;
	}

	pte = fastboot_flash_find_ptn(partition);
	if (!pte) {
		ERR("no %s partition\n", partition);
		return AVB_IO_RESULT_ERROR_NO_SUCH_PARTITION;
	}

	blksz = fs_dev_desc->blksz;
	part_start = pte->start;
	part_end = pte->start + pte->length - 1;
	VDEBUG("blksz: %ld, part_end: %ld, part_start: %ld:\n",
			blksz, part_end, part_start);

	if(get_margin_pos(part_start, part_end, blksz,
				&margin, offset, num_bytes, true))
		return AVB_IO_RESULT_ERROR_RANGE_OUTSIDE_PARTITION;

	bs = margin.blk_start;
	be = margin.blk_end;
	s = margin.start;
	bm = margin.multi;

	// alloc a blksz mem
	bdata = (unsigned char *)memalign(ALIGN_BYTES, blksz);
	if (bdata == NULL)
		return AVB_IO_RESULT_ERROR_OOM;

	// support multi blk read
	while (bs <= be) {
		if (!s && bm > 1) {
			dst = out_buf;
			dst64 = PTR_ALIGN(out_buf, 64); //for mmc blk read alignment
			VDEBUG("cur: dst=0x%08x, dst64=0x%08x\n", dst, dst64);
			if (dst64 != dst) {
				dst = dst64;
				bm--;
			}
			blk_num = bm;
			cnt = bm * blksz;
			bm = 0; //no more multi blk
		} else {
			blk_num = 1;
			cnt = blksz - s;
			if (num_read + cnt > num_bytes)
				cnt = num_bytes - num_read;
			dst = bdata;
		}
		VDEBUG("cur: bs=%ld, num=%ld, start=%ld, cnt=%ld dst=0x%08x\n",
				bs, blk_num, s, cnt, dst);
		if (!fs_dev_desc->block_read(fs_dev_desc->dev, bs, blk_num, dst)) {
			ret = AVB_IO_RESULT_ERROR_IO;
			goto fail;
		}

		if (dst == bdata)
			memcpy(out_buf, bdata + s, cnt);
		else if (dst == dst64)
			memcpy(out_buf, dst, cnt); //internal copy

		s = 0;
		bs += blk_num;
		num_read += cnt;
		out_buf += cnt;
#ifdef AVB_VVDEBUG
		printf("\nnum_read=%ld", cnt);
		printf("\n----dump---\n");
		print_buffer(0, buffer, HEXDUMP_WIDTH, cnt, 0);
		printf("--- end ---\n");
#endif
	}
	*out_num_read = num_read;
	ret = AVB_IO_RESULT_OK;
#ifdef AVB_VVDEBUG
	printf("\nnum_read=%zu", num_read);
	printf("\n----dump---\n");
	print_buffer(0, buffer, HEXDUMP_WIDTH, num_read, 0);
	printf("--- end ---\n");
#endif

fail:
	free(bdata);
	return ret;
}

 /* Writes |num_bytes| from |bffer| at offset |offset| to partition
  * with name |partition| (NUL-terminated UTF-8 string). If |offset|
  * is negative, its absolute value should be interpreted as the
  * number of bytes from the end of the partition.
  *
  * This function returns AVB_IO_RESULT_ERROR_NO_SUCH_PARTITION if
  * there is no partition with the given name,
  * AVB_IO_RESULT_ERROR_RANGE_OUTSIDE_PARTITION if the requested
  * byterange goes outside the partition, and AVB_IO_RESULT_ERROR_IO
  * if there was an I/O error from the underlying I/O subsystem.  If
  * the operation succeeds as requested AVB_IO_RESULT_OK is
  * returned.
  *
  * This function never does any partial I/O, it either transfers all
  * of the requested bytes or returns an error.
  */
 AvbIOResult fsl_write_to_partition(AvbOps* ops, const char* partition,
				    int64_t offset, size_t num_bytes,
				    const void* buffer)
{
	struct fastboot_ptentry *pte;
	unsigned char *bdata;
	unsigned char *in_buf = (unsigned char *)buffer;
	unsigned long blksz;
	unsigned long s, cnt;
	size_t num_write = 0;
	lbaint_t part_start, part_end, bs;
	margin_pos_t margin;

	AvbIOResult ret;

	DEBUGAVB("[%s]: offset=%ld, num_bytes=%zu\n", partition, (long)offset, num_bytes);

	assert(buffer != NULL);

	if (!fs_dev_desc && (fs_dev_desc = get_mmc_desc()) == NULL) {
		ERR("mmc device not found\n");
		return AVB_IO_RESULT_ERROR_NO_SUCH_PARTITION;
	}

	pte = fastboot_flash_find_ptn(partition);
	if (!pte) {
		ERR("no %s partition\n", partition);
		return AVB_IO_RESULT_ERROR_NO_SUCH_PARTITION;
	}

	blksz = fs_dev_desc->blksz;
	part_start = pte->start;
	part_end = pte->start + pte->length - 1;
	VDEBUG("blksz: %ld, part_end: %ld, part_start: %ld:\n",
			blksz, part_end, part_start);

	if(get_margin_pos(part_start, part_end, blksz,
				&margin, offset, num_bytes, false))
		return AVB_IO_RESULT_ERROR_RANGE_OUTSIDE_PARTITION;

	bs = margin.blk_start;
	s = margin.start;

	// alloc a blksz mem
	bdata = (unsigned char *)memalign(ALIGN_BYTES, blksz);
	if (bdata == NULL)
		return AVB_IO_RESULT_ERROR_OOM;

	while (num_write < num_bytes) {
		memset(bdata, 0, blksz);
		cnt = blksz - s;
		if (num_write + cnt >  num_bytes)
			cnt = num_bytes - num_write;
		if (!s || cnt != blksz) { //read blk first
			if (!fs_dev_desc->block_read(fs_dev_desc->dev, bs, 1, bdata)) {
				ret = AVB_IO_RESULT_ERROR_IO;
				goto fail;
			}
		}
		memcpy(bdata + s, in_buf, cnt); //change data
		VDEBUG("cur: bs=%ld, start=%ld, cnt=%ld bdata=0x%08x\n",
				bs, s, cnt, bdata);
		if (!fs_dev_desc->block_write(fs_dev_desc->dev, bs, 1, bdata)) {
			ret = AVB_IO_RESULT_ERROR_IO;
			goto fail;
		}
		bs++;
		num_write += cnt;
		in_buf += cnt;
		if (s != 0)
			s = 0;
	}
	ret = AVB_IO_RESULT_OK;

fail:
	free(bdata);
	return ret;
}

/* Reads A/B metadata from persistent storage. Returned data is
 * properly byteswapped. Returns AVB_IO_RESULT_OK on success, error
 * code otherwise.
 *
 * If the data read is invalid (e.g. wrong magic or CRC checksum
 * failure), the metadata shoule be reset using avb_ab_data_init()
 * and then written to persistent storage.
 *
 * Implementations will typically want to use avb_ab_data_read()
 * here to use the 'misc' partition for persistent storage.
 */
AvbIOResult fsl_read_ab_metadata(AvbOps* ops, struct AvbABData* data)
{
	return avb_ab_data_read(ops, data);
}

/* Writes A/B metadata to persistent storage. This will byteswap and
 * update the CRC as needed. Returns AVB_IO_RESULT_OK on success,
 * error code otherwise.
 *
 * Implementations will typically want to use avb_ab_data_write()
 * here to use the 'misc' partition for persistent storage.
 */
AvbIOResult fsl_write_ab_metadata(AvbOps* ops, const struct AvbABData* data)
{
	return avb_ab_data_write(ops, data);
}

/* Checks if the given public key used to sign the 'vbmeta'
 * partition is trusted. Boot loaders typically compare this with
 * embedded key material generated with 'avbtool
 * extract_public_key'.
 *
 * If AVB_IO_RESULT_OK is returned then |out_is_trusted| is set -
 * true if trusted or false if untrusted.
 */
AvbIOResult fsl_validate_vbmeta_public_key(AvbOps* ops,
					   const uint8_t* public_key_data,
					   size_t public_key_length,
					   bool* out_is_trusted) {
	kblb_hdr_t hdr;
	kblb_tag_t *pubk;
	size_t num_read, blob_size;
	uint8_t *extract_key = NULL;
	uint8_t *read_keyblb = NULL;
	AvbIOResult ret;

	assert(ops != NULL && out_is_trusted != NULL);
	*out_is_trusted = false;
	/* read the kblb header */
	if (ops->read_from_partition(ops, "avbkey", 0, sizeof(hdr),
			(void *)&hdr, &num_read) != AVB_IO_RESULT_OK) {
		ERR("read partition avbkey error\n");
		return AVB_IO_RESULT_ERROR_IO;
	}

	if (num_read != sizeof(hdr) ||
			memcmp(hdr.magic, AVB_KBLB_MAGIC, AVB_KBLB_MAGIC_LEN) != 0) {
		ERR("avbkey partition magic not match\n");
		return AVB_IO_RESULT_ERROR_IO;
	}

	pubk = &hdr.pubk_tag;
	if (pubk->len != public_key_length){
		ERR("avbkey len not match\n");
		return AVB_IO_RESULT_ERROR_IO;
	}
	blob_size = pubk->len + AVB_CAAM_PAD;
	extract_key = malloc(pubk->len);
	read_keyblb = malloc(blob_size);
	if (extract_key == NULL || read_keyblb == NULL) {
		ret = AVB_IO_RESULT_ERROR_OOM;
		goto fail;
	}

	/* read public keyblob */
	if (ops->read_from_partition(ops, "avbkey", pubk->offset, blob_size,
			(void *)read_keyblb, &num_read) != AVB_IO_RESULT_OK) {
		ERR("read public keyblob error\n");
		ret = AVB_IO_RESULT_ERROR_IO;
		goto fail;
	}
	if (num_read != blob_size) {
		ERR("avbkey partition magic not match\n");
		ret = AVB_IO_RESULT_ERROR_IO;
		goto fail;
	}

	/* caam decrypt */
	caam_open();
	if (caam_decap_blob((uint32_t)extract_key, (uint32_t)read_keyblb, pubk->len)) {
		ret = AVB_IO_RESULT_ERROR_IO;
		goto fail;
	}
	/* match given public key */
	if (memcmp(extract_key, public_key_data, public_key_length)) {
		ret = AVB_IO_RESULT_OK;
		goto fail;
	}
#ifdef AVB_VDEBUG
	printf("\n----key dump: stored---\n");
	print_buffer(0, extract_key, HEXDUMP_WIDTH, pubk->len, 0);
	printf("\n----key dump: vbmeta---\n");
	print_buffer(0, public_key_data, HEXDUMP_WIDTH, public_key_length, 0);
	printf("--- end ---\n");
#endif

	*out_is_trusted = true;
	ret = AVB_IO_RESULT_OK;
fail:
	if (extract_key != NULL)
		free(extract_key);
	if (read_keyblb != NULL)
		free(read_keyblb);
	return ret;
}

/* Gets the rollback index corresponding to the slot given by
 * |rollback_index_slot|. The value is returned in
 * |out_rollback_index|. Returns AVB_IO_RESULT_OK if the rollback
 * index was retrieved, otherwise an error code.
 *
 * A device may have a limited amount of rollback index slots (say,
 * one or four) so may error out if |rollback_index_slot| exceeds
 * this number.
 */
AvbIOResult fsl_read_rollback_index(AvbOps* ops, size_t rollback_index_slot,
				    uint64_t* out_rollback_index) {
	kblb_hdr_t hdr;
	kblb_tag_t *rbk;
	size_t num_read, blob_size;
	uint64_t *extract_idx = NULL;
	uint64_t *read_keyblb = NULL;
	AvbIOResult ret;

	assert(ops != NULL && out_rollback_index != NULL);
	*out_rollback_index = ~0;

	DEBUGAVB("read rollback slot: %zu\n", rollback_index_slot);

	if (rollback_index_slot >= AVB_MAX_NUMBER_OF_ROLLBACK_INDEX_SLOTS)
		return AVB_IO_RESULT_ERROR_IO;

	/* read the kblb header */
	if (ops->read_from_partition(ops, "avbkey", 0, sizeof(hdr),
			(void *)&hdr, &num_read) != AVB_IO_RESULT_OK) {
		ERR("read partition avbkey error\n");
		return AVB_IO_RESULT_ERROR_IO;
	}

	if (num_read != sizeof(hdr) ||
			memcmp(hdr.magic, AVB_KBLB_MAGIC, AVB_KBLB_MAGIC_LEN) != 0) {
		ERR("avbkey partition magic not match\n");
		return AVB_IO_RESULT_ERROR_IO;
	}

	rbk = &hdr.rbk_tags[rollback_index_slot];
	blob_size = rbk->len + AVB_CAAM_PAD;
	extract_idx = malloc(rbk->len);
	read_keyblb = malloc(blob_size);
	if (extract_idx == NULL || read_keyblb == NULL) {
		ret = AVB_IO_RESULT_ERROR_OOM;
		goto fail;
	}

	/* read rollback_index keyblob */
	if (ops->read_from_partition(ops, "avbkey", rbk->offset, blob_size,
			(void *)read_keyblb, &num_read) != AVB_IO_RESULT_OK) {
		ERR("read public keyblob error\n");
		ret = AVB_IO_RESULT_ERROR_IO;
		goto fail;
	}
	if (num_read != blob_size) {
		ERR("avbkey read len not match\n");
		ret = AVB_IO_RESULT_ERROR_IO;
		goto fail;
	}

	/* caam decrypt */
	caam_open();
	if (caam_decap_blob((uint32_t)extract_idx, (uint32_t)read_keyblb, rbk->len)) {
		ret = AVB_IO_RESULT_ERROR_IO;
		goto fail;
	}
#ifdef AVB_VVDEBUG
	printf("\n----idx dump: ---\n");
	print_buffer(0, extract_idx, HEXDUMP_WIDTH, rbk->len, 0);
	printf("--- end ---\n");
#endif

	*out_rollback_index = *extract_idx;
	DEBUGAVB("rollback_index = %" PRIu64 "\n", *out_rollback_index);
	ret = AVB_IO_RESULT_OK;
fail:
	if (extract_idx != NULL)
		free(extract_idx);
	if (read_keyblb != NULL)
		free(read_keyblb);
	return ret;
}

/* Sets the rollback index corresponding to the slot given by
 * |rollback_index_slot| to |rollback_index|. Returns
 * AVB_IO_RESULT_OK if the rollback index was set, otherwise an
 * error code.
 *
 * A device may have a limited amount of rollback index slots (say,
 * one or four) so may error out if |rollback_index_slot| exceeds
 * this number.
 */
AvbIOResult fsl_write_rollback_index(AvbOps* ops, size_t rollback_index_slot,
				     uint64_t rollback_index) {
	kblb_hdr_t hdr;
	kblb_tag_t *rbk;
	size_t num_read, blob_size;
	uint64_t *plain_idx = NULL;
	uint64_t *write_keyblb = NULL;
	AvbIOResult ret;

	DEBUGAVB("write to rollback slot: (%zu, %" PRIu64 ")\n",
			rollback_index_slot, rollback_index);

	assert(ops != NULL);

	if (rollback_index_slot >= AVB_MAX_NUMBER_OF_ROLLBACK_INDEX_SLOTS)
		return AVB_IO_RESULT_ERROR_IO;

	/* read the kblb header */
	if (ops->read_from_partition(ops, "avbkey", 0, sizeof(hdr),
			(void *)&hdr, &num_read) != AVB_IO_RESULT_OK) {
		ERR("read partition avbkey error\n");
		return AVB_IO_RESULT_ERROR_IO;
	}

	if (num_read != sizeof(hdr) ||
			memcmp(hdr.magic, AVB_KBLB_MAGIC, AVB_KBLB_MAGIC_LEN) != 0) {
		ERR("avbkey partition magic not match\n");
		return AVB_IO_RESULT_ERROR_IO;
	}

	rbk = &hdr.rbk_tags[rollback_index_slot];
	blob_size = rbk->len + AVB_CAAM_PAD;
	plain_idx = malloc(rbk->len);
	write_keyblb = malloc(blob_size);
	if (plain_idx == NULL || write_keyblb == NULL) {
		ret = AVB_IO_RESULT_ERROR_OOM;
		goto fail;
	}
	memset(plain_idx, 0, rbk->len);
	*plain_idx = rollback_index;

	/* caam encrypt */
	caam_open();
	if (caam_gen_blob((uint32_t)plain_idx, (uint32_t)write_keyblb, rbk->len)) {
		ret = AVB_IO_RESULT_ERROR_IO;
		goto fail;
	}

	/* write rollback_index keyblob */
	if (ops->write_to_partition(ops, "avbkey", rbk->offset, blob_size,
			(void *)write_keyblb) != AVB_IO_RESULT_OK) {
		ERR("read public keyblob error\n");
		ret = AVB_IO_RESULT_ERROR_IO;
		goto fail;
	}
	ret = AVB_IO_RESULT_OK;
fail:
	if (plain_idx != NULL)
		free(plain_idx);
	if (write_keyblb != NULL)
		free(write_keyblb);
	return ret;
}

/* Gets whether the device is unlocked. The value is returned in
 * |out_is_unlocked| (true if unlocked, false otherwise). Returns
 * AVB_IO_RESULT_OK if the state was retrieved, otherwise an error
 * code.
 */
AvbIOResult fsl_read_is_device_unlocked(AvbOps* ops, bool* out_is_unlocked) {

	FbLockState status;

	assert(out_is_unlocked != NULL);
	*out_is_unlocked = false;

	status = fastboot_get_lock_stat();
	if (status != FASTBOOT_LOCK_ERROR) {
		if (status == FASTBOOT_LOCK)
			*out_is_unlocked = false;
		else
			*out_is_unlocked = true;
	} else
		return AVB_IO_RESULT_ERROR_IO;

	DEBUGAVB("is_unlocked=%d\n", *out_is_unlocked);
	return AVB_IO_RESULT_OK;
}

/* Gets the unique partition GUID for a partition with name in
 * |partition| (NUL-terminated UTF-8 string). The GUID is copied as
 * a string into |guid_buf| of size |guid_buf_size| and will be NUL
 * terminated. The string must be lower-case and properly
 * hyphenated. For example:
 *
 *  527c1c6d-6361-4593-8842-3c78fcd39219
 *
 * Returns AVB_IO_RESULT_OK on success, otherwise an error code.
 */
AvbIOResult fsl_get_unique_guid_for_partition(AvbOps* ops,
					      const char* partition,
					      char* guid_buf,
					      size_t guid_buf_size) {
	assert(guid_buf != NULL);
#ifdef CONFIG_PARTITION_UUIDS
	struct fastboot_ptentry *pte;
	pte = fastboot_flash_find_ptn(partition);
	if (!pte) {
		ERR("no %s partition\n", partition);
		return AVB_IO_RESULT_ERROR_NO_SUCH_PARTITION;
	}
	strncpy(guid_buf, (const char *)pte->uuid, guid_buf_size);
	guid_buf[guid_buf_size - 1] = '\0';
	DEBUGAVB("[%s]: GUID=%s\n", partition, guid_buf);
	return AVB_IO_RESULT_OK;
#else
	return AVB_IO_RESULT_ERROR_IO;
#endif
}
