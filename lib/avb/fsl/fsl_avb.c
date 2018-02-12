/*
 * Copyright (C) 2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <part.h>
#include <stdlib.h>

#include <fsl_fastboot.h>
#include "../../../drivers/usb/gadget/fastboot_lock_unlock.h"

#include <fsl_avb.h>
#include "fsl_avbkey.h"
#include "utils.h"
#include "debug.h"

#define FSL_AVB_DEV "mmc"


static struct blk_desc *fs_dev_desc = NULL;
static struct blk_desc *get_mmc_desc(void) {
	extern int mmc_get_env_dev(void);
	int dev_no = mmc_get_env_dev();
	return blk_get_dev(FSL_AVB_DEV, dev_no);
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

	if ((fs_dev_desc = get_mmc_desc()) == NULL) {
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

	if(get_margin_pos((uint64_t)part_start, (uint64_t)part_end, blksz,
				&margin, offset, num_bytes, true))
		return AVB_IO_RESULT_ERROR_RANGE_OUTSIDE_PARTITION;

	bs = (lbaint_t)margin.blk_start;
	be = (lbaint_t)margin.blk_end;
	s = margin.start;

	// alloc a blksz mem
	bdata = (unsigned char *)memalign(ALIGN_BYTES, blksz);
	if (bdata == NULL)
		return AVB_IO_RESULT_ERROR_OOM;

	// one block a time
	while (bs <= be) {
		memset(bdata, 0, blksz);
		if (!fs_dev_desc->block_read(fs_dev_desc, bs, 1, bdata)) {
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

	if ((fs_dev_desc = get_mmc_desc()) == NULL) {
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

	if(get_margin_pos((uint64_t)part_start, (uint64_t)part_end, blksz,
				&margin, offset, num_bytes, true))
		return AVB_IO_RESULT_ERROR_RANGE_OUTSIDE_PARTITION;

	bs = (lbaint_t)margin.blk_start;
	be = (lbaint_t)margin.blk_end;
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
		if (!fs_dev_desc->block_read(fs_dev_desc, bs, blk_num, dst)) {
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

	if ((fs_dev_desc = get_mmc_desc()) == NULL) {
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

	if(get_margin_pos((uint64_t)part_start, (uint64_t)part_end, blksz,
				&margin, offset, num_bytes, false))
		return AVB_IO_RESULT_ERROR_RANGE_OUTSIDE_PARTITION;

	bs = (lbaint_t)margin.blk_start;
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
			if (!fs_dev_desc->block_read(fs_dev_desc, bs, 1, bdata)) {
				ret = AVB_IO_RESULT_ERROR_IO;
				goto fail;
			}
		}
		memcpy(bdata + s, in_buf, cnt); //change data
		VDEBUG("cur: bs=%ld, start=%ld, cnt=%ld bdata=0x%08x\n",
				bs, s, cnt, bdata);
		if (!fs_dev_desc->block_write(fs_dev_desc, bs, 1, bdata)) {
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
AvbIOResult fsl_read_ab_metadata(AvbABOps* ab_ops, struct AvbABData* data)
{
	return avb_ab_data_read(ab_ops, data);
}

/* Writes A/B metadata to persistent storage. This will byteswap and
 * update the CRC as needed. Returns AVB_IO_RESULT_OK on success,
 * error code otherwise.
 *
 * Implementations will typically want to use avb_ab_data_write()
 * here to use the 'misc' partition for persistent storage.
 */
AvbIOResult fsl_write_ab_metadata(AvbABOps* ab_ops, const struct AvbABData* data)
{
	return avb_ab_data_write(ab_ops, data);
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
/* Gets the size of a partition with the name in |partition|
 * (NUL-terminated UTF-8 string). Returns the value in
 * |out_size_num_bytes|.
 * Returns AVB_IO_RESULT_OK on success, otherwise an error code.
 */
AvbIOResult fsl_get_size_of_partition(AvbOps* ops,
		const char* partition,
		uint64_t* out_size_num_bytes)
{
	struct fastboot_ptentry *pte;
	pte = fastboot_flash_find_ptn(partition);
	if (!pte) {
		ERR("no %s partition\n", partition);
		return AVB_IO_RESULT_ERROR_NO_SUCH_PARTITION;
	}
	*out_size_num_bytes = (uint64_t)(pte->length * 512);
	return AVB_IO_RESULT_OK;
}
