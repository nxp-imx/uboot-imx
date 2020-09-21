/*
 * Copyright (C) 2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <part.h>
#include <stdlib.h>

#include <fb_fsl.h>
#include "../../../drivers/fastboot/fb_fsl/fastboot_lock_unlock.h"

#include <fsl_avb.h>
#include "fsl_avbkey.h"
#include "utils.h"
#include "debug.h"
#include "trusty/avb.h"
#ifndef CONFIG_LOAD_KEY_FROM_RPMB
#include "fsl_public_key.h"
#endif
#include "fsl_atx_attributes.h"

#define FSL_AVB_DEV "mmc"
#define AVB_MAX_BUFFER_LENGTH 2048

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
		fastboot_flash_dump_ptn();
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
		if (blk_dread(fs_dev_desc, bs, 1, bdata) != 1) {
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
		fastboot_flash_dump_ptn();
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
		if (blk_dread(fs_dev_desc, bs, blk_num, dst) != blk_num) {
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
		fastboot_flash_dump_ptn();
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
			if (blk_dread(fs_dev_desc, bs, 1, bdata) != 1) {
				ret = AVB_IO_RESULT_ERROR_IO;
				goto fail;
			}
		}
		memcpy(bdata + s, in_buf, cnt); //change data
		VDEBUG("cur: bs=%ld, start=%ld, cnt=%ld bdata=0x%08x\n",
				bs, s, cnt, bdata);
		if (blk_dwrite(fs_dev_desc, bs, 1, bdata) != 1) {
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
		fastboot_flash_dump_ptn();
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
		fastboot_flash_dump_ptn();
		return AVB_IO_RESULT_ERROR_NO_SUCH_PARTITION;
	}
	*out_size_num_bytes = (uint64_t)(pte->length) * 512;
	return AVB_IO_RESULT_OK;
}

#ifdef CONFIG_AVB_ATX
/* Reads permanent |attributes| data. There are no restrictions on where this
 * data is stored. On success, returns AVB_IO_RESULT_OK and populates
 * |attributes|.
 */
AvbIOResult fsl_read_permanent_attributes(
    AvbAtxOps* atx_ops, AvbAtxPermanentAttributes* attributes) {
#ifdef CONFIG_IMX_TRUSTY_OS
	if (!trusty_read_permanent_attributes((uint8_t *)attributes,
			sizeof(AvbAtxPermanentAttributes))) {
		return AVB_IO_RESULT_OK;
	}
	ERR("No perm-attr fused. Will use hard code one.\n");
#endif /* CONFIG_IMX_TRUSTY_OS */

	/* use hard code permanent attributes due to limited fuse and RPMB */
	attributes->version = fsl_version;
	memcpy(attributes->product_root_public_key, fsl_product_root_public_key,
	       sizeof(fsl_product_root_public_key));
	memcpy(attributes->product_id, fsl_atx_product_id,
	       sizeof(fsl_atx_product_id));

	return AVB_IO_RESULT_OK;
}

/* Reads a |hash| of permanent attributes. This hash MUST be retrieved from a
 * permanently read-only location (e.g. fuses) when a device is LOCKED. On
 * success, returned AVB_IO_RESULT_OK and populates |hash|.
 */
AvbIOResult fsl_read_permanent_attributes_hash(
    AvbAtxOps* atx_ops, uint8_t hash[AVB_SHA256_DIGEST_SIZE]) {
#ifdef CONFIG_ARM64
	/* calculate sha256(permanent attributes) */
	if (permanent_attributes_sha256_hash(hash) != RESULT_OK) {
		return AVB_IO_RESULT_ERROR_IO;
	} else {
	    return AVB_IO_RESULT_OK;
	}
#else
	uint8_t sha256_hash_buf[AVB_SHA256_DIGEST_SIZE];
	uint32_t sha256_hash_fuse[ATX_FUSE_BANK_NUM];

	/* read first 112 bits of sha256(permanent attributes) from fuse */
	if (fsl_fuse_read(sha256_hash_fuse, ATX_FUSE_BANK_NUM,
			  PERMANENT_ATTRIBUTE_HASH_OFFSET)) {
		printf("ERROR - read permanent attributes hash from "
		       "fuse error\n");
		return AVB_IO_RESULT_ERROR_IO;
	}
	/* only take the lower 2 bytes of last bank */
	sha256_hash_fuse[ATX_FUSE_BANK_NUM - 1] &= ATX_FUSE_BANK_MASK;

	/* calculate sha256(permanent attributes) */
	if (permanent_attributes_sha256_hash(sha256_hash_buf) != RESULT_OK) {
		return AVB_IO_RESULT_ERROR_IO;
	}
	/* check if the sha256(permanent attributes) hash match the calculated one,
	 * if not match, just return all zeros hash.
	 */
	if (memcmp(sha256_hash_fuse, sha256_hash_buf, ATX_HASH_LENGTH)) {
		printf("ERROR - sha256(permanent attributes) does not match\n");
		memset(hash, 0, AVB_SHA256_DIGEST_SIZE);
	} else {
		memcpy(hash, sha256_hash_buf, AVB_SHA256_DIGEST_SIZE);
	}

	return AVB_IO_RESULT_OK;
#endif /* CONFIG_ARM64 */
}

 /* Generates |num_bytes| random bytes and stores them in |output|,
   * which must point to a buffer large enough to store the bytes.
   *
   * Returns AVB_IO_RESULT_OK on success, otherwise an error code.
   */
AvbIOResult fsl_get_random(AvbAtxOps* atx_ops,
				size_t num_bytes,
				uint8_t* output)
{
	uint32_t num = 0;
	uint32_t i;

	if (output == NULL) {
		ERR("Output buffer is NULL!\n");
		return AVB_IO_RESULT_ERROR_INSUFFICIENT_SPACE;
	}

	/* set the seed as device boot time. */
	srand((uint32_t)get_timer(0));
	for (i = 0; i < num_bytes; i++) {
		num = rand() % 256;
		output[i] = (uint8_t)num;
	}

	return AVB_IO_RESULT_OK;
}
/* Provides the key version of a key used during verification. This may be
 * useful for managing the minimum key version.
 */
void fsl_set_key_version(AvbAtxOps* atx_ops,
			 size_t rollback_index_location,
			 uint64_t key_version) {
	kblb_hdr_t hdr;
	kblb_tag_t *rbk;
	uint64_t *plain_idx = NULL;
	struct mmc *mmc_dev;
	static const uint32_t kTypeMask = 0xF000;

	DEBUGAVB("[rpmb] write to rollback slot: (%zu, %" PRIu64 ")\n",
		 rollback_index_location, key_version);

	assert(atx_ops != NULL);

	if ((mmc_dev = get_mmc()) == NULL) {
		ERR("err get mmc device\n");
	}
	/* read the kblb header */
	if (rpmb_read(mmc_dev, (uint8_t *)&hdr, sizeof(hdr), 0) != 0) {
		ERR("read RPMB error\n");
	}

	if (memcmp(hdr.magic, AVB_KBLB_MAGIC, AVB_KBLB_MAGIC_LEN) != 0) {
		ERR("magic not match\n");
	}

	/* rollback index for Android Things key versions */
	rbk = &hdr.atx_rbk_tags[rollback_index_location & ~kTypeMask];

	plain_idx = malloc(rbk->len);
	if (plain_idx == NULL)
		printf("\nError! allocate memory fail!\n");
	memset(plain_idx, 0, rbk->len);
	*plain_idx = key_version;

	/* write rollback_index keyblob */
	if (rpmb_write(mmc_dev, (uint8_t *)plain_idx, rbk->len, rbk->offset) !=
	    0) {
		ERR("write rollback index error\n");
		goto fail;
	}
fail:
	if (plain_idx != NULL)
		free(plain_idx);
}
#endif /* CONFIG_AVB_ATX */

#ifdef AVB_RPMB
/* Checks if the given public key used to sign the 'vbmeta'
 * partition is trusted. Boot loaders typically compare this with
 * embedded key material generated with 'avbtool
 * extract_public_key'.
 *
 * If AVB_IO_RESULT_OK is returned then |out_is_trusted| is set -
 * true if trusted or false if untrusted.
 */
AvbIOResult fsl_validate_vbmeta_public_key_rpmb(AvbOps* ops,
					   const uint8_t* public_key_data,
					   size_t public_key_length,
					   const uint8_t* public_key_metadata,
					   size_t public_key_metadata_length,
					   bool* out_is_trusted) {
	AvbIOResult ret;
	assert(ops != NULL && out_is_trusted != NULL);
	*out_is_trusted = false;

#ifdef CONFIG_LOAD_KEY_FROM_RPMB
	uint8_t public_key_buf[AVB_MAX_BUFFER_LENGTH];
	if (trusty_read_vbmeta_public_key(public_key_buf,
						public_key_length) != 0) {
		ERR("Read public key error\n");
		/* We're not going to return error code here because it will
		 * abort the following avb verify process even we allow the
		 * verification error. Return AVB_IO_RESULT_OK and keep the
		 * 'out_is_trusted' as false, avb will handle the error
		 * depends on the 'allow_verification_error' flag.
		 */
		return AVB_IO_RESULT_OK;
	}

	if (memcmp(public_key_buf, public_key_data, public_key_length)) {
#else
	/* match given public key */
	if (memcmp(fsl_public_key, public_key_data, public_key_length)) {
#endif
		ERR("public key not match\n");
		return AVB_IO_RESULT_OK;
	}

	*out_is_trusted = true;
	ret = AVB_IO_RESULT_OK;

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
AvbIOResult fsl_write_rollback_index_rpmb(AvbOps* ops, size_t rollback_index_slot,
				     uint64_t rollback_index) {
	AvbIOResult ret;
#ifdef CONFIG_IMX_TRUSTY_OS
	if (trusty_write_rollback_index(rollback_index_slot, rollback_index)) {
		ERR("write rollback from Trusty error!\n");
#ifndef CONFIG_AVB_ATX
		/* Read/write rollback index from rpmb will fail if the rpmb
		 * key hasn't been set, return AVB_IO_RESULT_OK in this case.
		 */
		if (!rpmbkey_is_set())
			ret = AVB_IO_RESULT_OK;
		else
#endif
			ret = AVB_IO_RESULT_ERROR_IO;
	} else {
		ret = AVB_IO_RESULT_OK;
	}
	return ret;
#else
	kblb_hdr_t hdr;
	kblb_tag_t *rbk;
	uint64_t *plain_idx = NULL;
	struct mmc *mmc_dev;
#ifdef CONFIG_AVB_ATX
	static const uint32_t kTypeMask = 0xF000;
	static const unsigned int kTypeShift = 12;
#endif

	DEBUGAVB("[rpmb] write to rollback slot: (%zu, %" PRIu64 ")\n",
			rollback_index_slot, rollback_index);

	assert(ops != NULL);
	/* check if the rollback index location exceed the limit */
#ifdef CONFIG_AVB_ATX
	if ((rollback_index_slot & ~kTypeMask) >= AVB_MAX_NUMBER_OF_ROLLBACK_INDEX_LOCATIONS)
#else
	if (rollback_index_slot >= AVB_MAX_NUMBER_OF_ROLLBACK_INDEX_LOCATIONS)
#endif /* CONFIG_AVB_ATX */
		return AVB_IO_RESULT_ERROR_IO;

	if ((mmc_dev = get_mmc()) == NULL) {
		ERR("err get mmc device\n");
		return AVB_IO_RESULT_ERROR_IO;
	}
	/* read the kblb header */
	if (rpmb_read(mmc_dev, (uint8_t *)&hdr, sizeof(hdr), 0) != 0) {
		ERR("read RPMB error\n");
		return AVB_IO_RESULT_ERROR_IO;
	}

	if (memcmp(hdr.magic, AVB_KBLB_MAGIC, AVB_KBLB_MAGIC_LEN) != 0) {
		ERR("magic not match\n");
		return AVB_IO_RESULT_ERROR_IO;
	}
	/* choose rollback index type */
#ifdef CONFIG_AVB_ATX
	if ((rollback_index_slot & kTypeMask) >> kTypeShift) {
		/* rollback index for Android Things key versions */
		rbk = &hdr.atx_rbk_tags[rollback_index_slot & ~kTypeMask];
	} else {
		/* rollback index for vbmeta */
		rbk = &hdr.rbk_tags[rollback_index_slot & ~kTypeMask];
	}
#else
	rbk = &hdr.rbk_tags[rollback_index_slot];
#endif /* CONFIG_AVB_ATX */
	plain_idx = malloc(rbk->len);
	if (plain_idx == NULL)
		return AVB_IO_RESULT_ERROR_OOM;
	memset(plain_idx, 0, rbk->len);
	*plain_idx = rollback_index;

	/* write rollback_index keyblob */
	if (rpmb_write(mmc_dev, (uint8_t *)plain_idx, rbk->len, rbk->offset) !=
	    0) {
		ERR("write rollback index error\n");
		ret = AVB_IO_RESULT_ERROR_IO;
		goto fail;
	}
	ret = AVB_IO_RESULT_OK;
fail:
	if (plain_idx != NULL)
		free(plain_idx);
	return ret;
#endif /* CONFIG_IMX_TRUSTY_OS */
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
AvbIOResult fsl_read_rollback_index_rpmb(AvbOps* ops, size_t rollback_index_slot,
				    uint64_t* out_rollback_index) {
	AvbIOResult ret;
#ifdef CONFIG_IMX_TRUSTY_OS
	if (trusty_read_rollback_index(rollback_index_slot, out_rollback_index)) {
		ERR("read rollback from Trusty error!\n");
#ifndef CONFIG_AVB_ATX
		if (!rpmbkey_is_set()) {
			*out_rollback_index = 0;
			ret = AVB_IO_RESULT_OK;
		} else
#endif
			ret = AVB_IO_RESULT_ERROR_IO;
	} else {
		ret = AVB_IO_RESULT_OK;
	}
	return ret;
#else
	kblb_hdr_t hdr;
	kblb_tag_t *rbk;
	uint64_t *extract_idx = NULL;
	struct mmc *mmc_dev;
#ifdef CONFIG_AVB_ATX
	static const uint32_t kTypeMask = 0xF000;
	static const unsigned int kTypeShift = 12;
#endif

	assert(ops != NULL && out_rollback_index != NULL);
	*out_rollback_index = ~0;

	DEBUGAVB("[rpmb] read rollback slot: %zu\n", rollback_index_slot);

	/* check if the rollback index location exceed the limit */
#ifdef CONFIG_AVB_ATX
	if ((rollback_index_slot & ~kTypeMask) >= AVB_MAX_NUMBER_OF_ROLLBACK_INDEX_LOCATIONS)
#else
	if (rollback_index_slot >= AVB_MAX_NUMBER_OF_ROLLBACK_INDEX_LOCATIONS)
#endif
		return AVB_IO_RESULT_ERROR_IO;

	if ((mmc_dev = get_mmc()) == NULL) {
		ERR("err get mmc device\n");
		return AVB_IO_RESULT_ERROR_IO;
	}
	/* read the kblb header */
	if (rpmb_read(mmc_dev, (uint8_t *)&hdr, sizeof(hdr), 0) != 0) {
		ERR("read RPMB error\n");
		return AVB_IO_RESULT_ERROR_IO;
	}

	if (memcmp(hdr.magic, AVB_KBLB_MAGIC, AVB_KBLB_MAGIC_LEN) != 0) {
		ERR("magic not match\n");
		return AVB_IO_RESULT_ERROR_IO;
	}
	/* choose rollback index type */
#ifdef CONFIG_AVB_ATX
	if ((rollback_index_slot & kTypeMask) >> kTypeShift) {
		/* rollback index for Android Things key versions */
		rbk = &hdr.atx_rbk_tags[rollback_index_slot & ~kTypeMask];
	} else {
		/* rollback index for vbmeta */
		rbk = &hdr.rbk_tags[rollback_index_slot & ~kTypeMask];
	}
#else
	rbk = &hdr.rbk_tags[rollback_index_slot];
#endif /* CONFIG_AVB_ATX */
	extract_idx = malloc(rbk->len);
	if (extract_idx == NULL)
		return AVB_IO_RESULT_ERROR_OOM;

	/* read rollback_index keyblob */
	if (rpmb_read(mmc_dev, (uint8_t *)extract_idx, rbk->len, rbk->offset) != 0) {
		ERR("read rollback index error\n");
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
	return ret;
#endif /* CONFIG_IMX_TRUSTY_OS */
}
#else /* AVB_RPMB */
AvbIOResult fsl_validate_vbmeta_public_key_rpmb(AvbOps* ops,
					   const uint8_t* public_key_data,
					   size_t public_key_length,
					   const uint8_t* public_key_metadata,
					   size_t public_key_metadata_length,
					   bool* out_is_trusted) {
	assert(ops != NULL && out_is_trusted != NULL);

	/* match given public key */
	if (memcmp(fsl_public_key, public_key_data, public_key_length)) {
		ERR("public key not match\n");
		*out_is_trusted = false;
	} else
		*out_is_trusted = true;

	/* We're not going to return error code when public key
	 * verify fail because it will abort the following avb
	 * verify process even we allow the verification error.
	 * Return AVB_IO_RESULT_OK and keep the 'out_is_trusted'
	 * as false, avb will handle the error depends on the
	 * 'allow_verification_error' flag.
	 */
	return AVB_IO_RESULT_OK;
}

/* In no security enhanced ARM64, rollback index has no protection so no use it */
AvbIOResult fsl_write_rollback_index_rpmb(AvbOps* ops, size_t rollback_index_slot,
				     uint64_t rollback_index) {
	return AVB_IO_RESULT_OK;

}
AvbIOResult fsl_read_rollback_index_rpmb(AvbOps* ops, size_t rollback_index_slot,
				    uint64_t* out_rollback_index) {
	*out_rollback_index = 0;
	return AVB_IO_RESULT_OK;
}
#endif /* AVB_RPMB */
