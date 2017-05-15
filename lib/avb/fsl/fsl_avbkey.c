/*
 * Copyright (C) 2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <stdlib.h>
#include <fsl_caam.h>
#include <fuse.h>
#include <mmc.h>

#include <fsl_avb.h>
#include "fsl_avbkey.h"
#include "utils.h"
#include "debug.h"

#define INITFLAG_FUSE_OFFSET 0
#define INITFLAG_FUSE_MASK 0x00000001
#define INITFLAG_FUSE 0x00000001

#define RPMB_BLKSZ 256
#define RPMBKEY_FUSE_OFFSET 1
#define RPMBKEY_LENGTH 32
#define RPMBKEY_FUSE_LEN ((RPMBKEY_LENGTH) + (CAAM_PAD))
#define RPMBKEY_FUSE_LENW (RPMBKEY_FUSE_LEN / 4)

static int mmc_dev_no = -1;

static struct mmc *get_mmc(void) {
	extern int mmc_get_env_devno(void);
	struct mmc *mmc;
	if (mmc_dev_no < 0 && (mmc_dev_no = mmc_get_env_dev()) < 0)
		return NULL;
	mmc = find_mmc_device(mmc_dev_no);
	if (!mmc || mmc_init(mmc))
		return NULL;
	return mmc;
}

static int fsl_fuse_ops(uint32_t *buffer, uint32_t length, uint32_t offset,
			const uint8_t read) {

	unsigned short bs, ws, bksz, cnt;
	unsigned short num_done = 0;
	margin_pos_t margin;
	int i;

	/* read from fuse */
	bksz = CONFIG_AVB_FUSE_BANK_SIZEW;
	if(get_margin_pos(CONFIG_AVB_FUSE_BANK_START, CONFIG_AVB_FUSE_BANK_END, bksz,
				&margin, offset, length, false))
		return -1;
	bs = (unsigned short)margin.blk_start;
	ws = (unsigned short)margin.start;

	while (num_done < length) {
		cnt = bksz - ws;
		if (num_done + cnt > length)
			cnt = length - num_done;
		for (i = 0; i < cnt; i++) {
			VDEBUG("cur: bank=%d, word=%d\n",bs, ws);
			if (read) {
#ifdef CONFIG_AVB_FUSE
				if (fuse_sense(bs, ws, buffer)) {
#else
				if (fuse_read(bs, ws, buffer)) {
#endif
					ERR("read fuse bank %d, word %d error\n", bs, ws);
					return -1;
				}
			} else {
#ifdef CONFIG_AVB_FUSE
				if (fuse_prog(bs, ws, *buffer)) {
#else
				if (fuse_override(bs, ws, *buffer)) {
#endif
					ERR("write fuse bank %d, word %d error\n", bs, ws);
					return -1;
				}
			}
			ws++;
			buffer++;
		}
		bs++;
		num_done += cnt;
		ws = 0;
	}
	return 0;
}

static int fsl_fuse_read(uint32_t *buffer, uint32_t length, uint32_t offset) {

	return fsl_fuse_ops(
		buffer,
		length,
		offset,
		1
		);
}

static int fsl_fuse_write(const uint32_t *buffer, uint32_t length, uint32_t offset) {

	return fsl_fuse_ops(
		(uint32_t *)buffer,
		length,
		offset,
		0
		);
}

static int rpmb_key(struct mmc *mmc) {

	char original_part;
	uint8_t blob[RPMBKEY_FUSE_LEN];
	uint8_t plain_key[RPMBKEY_LENGTH];

	int ret;

	DEBUGAVB("[rpmb]: set kley\n");

	/* Switch to the RPMB partition */
	original_part = mmc->block_dev.hwpart;
	if (mmc->block_dev.hwpart != MMC_PART_RPMB) {
		if (mmc_switch_part(mmc, MMC_PART_RPMB) != 0)
			return -1;
		mmc->block_dev.hwpart = MMC_PART_RPMB;
	}

	/* use caam hwrng to generate */
	caam_open();
	if (caam_hwrng(plain_key, RPMBKEY_LENGTH)) {
		ERR("ERROR - caam rng\n");
		ret = -1;
		goto fail;
	}

	/* generate keyblob and program to fuse */
	if (caam_gen_blob((uint32_t)plain_key, (uint32_t)blob, RPMBKEY_LENGTH)) {
		ERR("gen rpmb key blb error\n");
		ret = -1;
		goto fail;
	}

	if (fsl_fuse_write((uint32_t *)blob, RPMBKEY_FUSE_LENW, RPMBKEY_FUSE_OFFSET)){
		ERR("write rpmb key to fuse error\n");
		ret = -1;
		goto fail;
	}

#ifdef CONFIG_AVB_FUSE
	/* program key to mmc */
	if (mmc_rpmb_set_key(mmc, plain_key)) {
		ERR("Key already programmed ?\n");
		ret = -1;
		goto fail;
	}
#endif
	ret = 0;

#ifdef CONFIG_AVB_DEBUG
	/* debug */
	uint8_t ext_key[RPMBKEY_LENGTH];
	printf(" RPMB plain kay---\n");
	print_buffer(0, plain_key, HEXDUMP_WIDTH, RPMBKEY_LENGTH, 0);
	if (fsl_fuse_read((uint32_t *)blob, RPMBKEY_FUSE_LENW, RPMBKEY_FUSE_OFFSET)){
		ERR("read rpmb key to fuse error\n");
		ret = -1;
		goto fail;
	}
	printf(" RPMB blob---\n");
	print_buffer(0, blob, HEXDUMP_WIDTH, RPMBKEY_FUSE_LEN, 0);
	if (caam_decap_blob((uint32_t)ext_key, (uint32_t)blob, RPMBKEY_LENGTH)) {
		ret = -1;
		goto fail;
	}
	printf(" RPMB extract---\n");
	print_buffer(0, ext_key, HEXDUMP_WIDTH, RPMBKEY_LENGTH, 0);
	/* debug done */
#endif

fail:
	/* Return to original partition */
	if (mmc->block_dev.hwpart != original_part) {
		if (mmc_switch_part(mmc, original_part) != 0)
			return -1;
		mmc->block_dev.hwpart = original_part;
	}
	return ret;

}

static int rpmb_read(struct mmc *mmc, uint8_t *buffer, size_t num_bytes, int64_t offset) {

	unsigned char *bdata = NULL;
	unsigned char *out_buf = (unsigned char *)buffer;
	unsigned long s, cnt;
	unsigned long blksz;
	size_t num_read = 0;
	unsigned short part_start, part_length, part_end, bs, be;
	margin_pos_t margin;
	char original_part;
	uint8_t extract_key[RPMBKEY_LENGTH];
	uint8_t blob[RPMBKEY_FUSE_LEN];

	int ret;

	blksz = RPMB_BLKSZ;
	part_length = mmc->capacity_rpmb >> 8;
	part_start = 0;
	part_end = part_start + part_length - 1;

	DEBUGAVB("[rpmb]: offset=%ld, num_bytes=%zu\n", (long)offset, num_bytes);

	if(get_margin_pos(part_start, part_end, blksz,
				&margin, offset, num_bytes, false))
		return -1;

	bs = (unsigned short)margin.blk_start;
	be = (unsigned short)margin.blk_end;
	s = margin.start;

	/* Switch to the RPMB partition */
	original_part = mmc->block_dev.hwpart;
	if (mmc->block_dev.hwpart != MMC_PART_RPMB) {
		if (mmc_switch_part(mmc, MMC_PART_RPMB) != 0)
			return -1;
		mmc->block_dev.hwpart = MMC_PART_RPMB;
	}

	/* get rpmb key */
	if (fsl_fuse_read((uint32_t *)blob, RPMBKEY_FUSE_LENW, RPMBKEY_FUSE_OFFSET)){
		ERR("read rpmb key error\n");
		ret = -1;
		goto fail;
	}
	caam_open();
	if (caam_decap_blob((uint32_t)extract_key, (uint32_t)blob, RPMBKEY_LENGTH)) {
		ERR("decap rpmb key error\n");
		ret = -1;
		goto fail;
	}

	// alloc a blksz mem
	bdata = (unsigned char *)memalign(ALIGN_BYTES, blksz);
	if (bdata == NULL) {
		ret = -1;
		goto fail;
	}
	// one block a time
	while (bs <= be) {
		memset(bdata, 0, blksz);
		if (mmc_rpmb_read(mmc, bdata, bs, 1, extract_key) != 1) {
			ret = -1;
			goto fail;
		}
		cnt = blksz - s;
		if (num_read + cnt > num_bytes)
			cnt = num_bytes - num_read;
		VDEBUG("cur: bs=%d, start=%ld, cnt=%ld bdata=0x%p\n",
				bs, s, cnt, bdata);
		memcpy(out_buf, bdata + s, cnt);
		bs++;
		num_read += cnt;
		out_buf += cnt;
		s = 0;
	}
	ret = 0;

fail:
	/* Return to original partition */
	if (mmc->block_dev.hwpart != original_part) {
		if (mmc_switch_part(mmc, original_part) != 0)
			return -1;
		mmc->block_dev.hwpart = original_part;
	}
	if (bdata != NULL)
		free(bdata);
	return ret;

}
static int rpmb_write(struct mmc *mmc, uint8_t *buffer, size_t num_bytes, int64_t offset) {

	unsigned char *bdata = NULL;
	unsigned char *in_buf = (unsigned char *)buffer;
	unsigned long s, cnt;
	unsigned long blksz;
	size_t num_write = 0;
	unsigned short part_start, part_length, part_end, bs;
	margin_pos_t margin;
	char original_part;
	uint8_t extract_key[RPMBKEY_LENGTH];
	uint8_t blob[RPMBKEY_FUSE_LEN];

	int ret;

	blksz = RPMB_BLKSZ;
	part_length = mmc->capacity_rpmb >> 8;
	part_start = 0;
	part_end = part_start + part_length - 1;

	DEBUGAVB("[rpmb]: offset=%ld, num_bytes=%zu\n", (long)offset, num_bytes);

	if(get_margin_pos(part_start, part_end, blksz,
				&margin, offset, num_bytes, false)) {
		ERR("get_margin_pos err\n");
		return -1;
	}

	bs = (unsigned short)margin.blk_start;
	s = margin.start;

	/* Switch to the RPMB partition */
	original_part = mmc->block_dev.hwpart;
	if (mmc->block_dev.hwpart != MMC_PART_RPMB) {
		if (mmc_switch_part(mmc, MMC_PART_RPMB) != 0)
			return -1;
		mmc->block_dev.hwpart = MMC_PART_RPMB;
	}

	/* get rpmb key */
	if (fsl_fuse_read((uint32_t *)blob, RPMBKEY_FUSE_LENW, RPMBKEY_FUSE_OFFSET)){
		ERR("read rpmb key error\n");
		ret = -1;
		goto fail;
	}
	caam_open();
	if (caam_decap_blob((uint32_t)extract_key, (uint32_t)blob, RPMBKEY_LENGTH)) {
		ERR("decap rpmb key error\n");
		ret = -1;
		goto fail;
	}

	// alloc a blksz mem
	bdata = (unsigned char *)memalign(ALIGN_BYTES, blksz);
	if (bdata == NULL) {
		ret = -1;
		goto fail;
	}
	while (num_write < num_bytes) {
		memset(bdata, 0, blksz);
		cnt = blksz - s;
		if (num_write + cnt >  num_bytes)
			cnt = num_bytes - num_write;
		if (!s || cnt != blksz) { //read blk first
			if (mmc_rpmb_read(mmc, bdata, bs, 1, extract_key) != 1) {
				ERR("mmc_rpmb_read err, mmc= 0x%08x\n", (unsigned int)mmc);
				ret = -1;
				goto fail;
			}
		}
		memcpy(bdata + s, in_buf, cnt); //change data
		VDEBUG("cur: bs=%d, start=%ld, cnt=%ld\n",	bs, s, cnt);
		if (mmc_rpmb_write(mmc, bdata, bs, 1, extract_key) != 1) {
			ret = -1;
			goto fail;
		}
		bs++;
		num_write += cnt;
		in_buf += cnt;
		if (s != 0)
			s = 0;
	}
	ret = 0;

fail:
	/* Return to original partition */
	if (mmc->block_dev.hwpart != original_part) {
		if (mmc_switch_part(mmc, original_part) != 0)
			return -1;
		mmc->block_dev.hwpart = original_part;
	}
	if (bdata != NULL)
		free(bdata);
	return ret;

}

int rbkidx_erase(void) {
	int i;
	kblb_hdr_t hdr;
	kblb_tag_t *tag;
	struct mmc *mmc_dev;

	if ((mmc_dev = get_mmc()) == NULL) {
		ERR("err get mmc device\n");
		return -1;
	}

	/* read the kblb header */
	if (rpmb_read(mmc_dev, (uint8_t *)&hdr, sizeof(hdr), 0) != 0) {
		ERR("read RPMB error\n");
		return -1;
	}
	if (memcmp(hdr.magic, AVB_KBLB_MAGIC, AVB_KBLB_MAGIC_LEN) != 0) {
		ERR("magic not match\n");
		return -1;
	}

	/* reset rollback index */
	uint32_t offset = AVB_RBIDX_START;
	uint32_t rbidx_len = AVB_RBIDX_LEN;
	uint8_t *rbidx = malloc(rbidx_len);
	if (rbidx == NULL)
		return -1;
	memset(rbidx, 0, rbidx_len);
	*(uint64_t *)rbidx = AVB_RBIDX_INITVAL;
	for (i = 0; i < AVB_MAX_NUMBER_OF_ROLLBACK_INDEX_LOCATIONS; i++) {
		tag = &hdr.rbk_tags[i];
		tag->flag = AVB_RBIDX_FLAG;
		tag->offset = offset;
		tag->len = rbidx_len;
		/* write */
		if (rpmb_write(mmc_dev, rbidx, tag->len, tag->offset) != 0) {
			ERR("write RBKIDX RPMB error\n");
			free(rbidx);
			return -1;
		}
		offset += AVB_RBIDX_ALIGN;
	}
	free(rbidx);
	/* write back hdr */
	if (rpmb_write(mmc_dev, (uint8_t *)&hdr, sizeof(hdr), 0) != 0) {
		ERR("write RPMB hdr error\n");
		return -1;
	}
	return 0;
}


int avbkey_init(uint8_t *plainkey, uint32_t keylen) {
	int i;
	kblb_hdr_t hdr;
	kblb_tag_t *tag;
	struct mmc *mmc_dev;
	uint32_t init_flag;

	/* int ret; */

	assert(plainkey != NULL);

	/* check overflow */
	if (keylen > AVB_RBIDX_START - AVB_PUBKY_OFFSET) {
		ERR("key len overflow\n");
		return -1;
	}

	/* check init status */
	if (fsl_fuse_read(&init_flag, 1, INITFLAG_FUSE_OFFSET)) {
		ERR("ERROR - read fuse init flag error\n");
		return -1;
	}
	if ((init_flag & INITFLAG_FUSE_MASK) == INITFLAG_FUSE) {
		ERR("ERROR - already inited\n");
		return -1;
	}
	init_flag = INITFLAG_FUSE & INITFLAG_FUSE_MASK;

	/* generate and write key to mmc/fuse */
	if ((mmc_dev = get_mmc()) == NULL) {
		ERR("ERROR - get mmc device\n");
		return -1;
	}
	if (rpmb_key(mmc_dev)) {
		ERR("ERROR - write mmc rpmb key\n");
		return -1;
	}

	if (fsl_fuse_write(&init_flag, 1, INITFLAG_FUSE_OFFSET)){
		ERR("write fuse init error\n");
		return -1;
	}

	/* init pubkey */
	tag = &hdr.pubk_tag;
	tag->flag = AVB_PUBKY_FLAG;
	tag->offset = AVB_PUBKY_OFFSET;
	tag->len = keylen;

	if (rpmb_write(mmc_dev, plainkey, tag->len, tag->offset) != 0) {
		ERR("write RPMB error\n");
		return -1;
	}

	/* init rollback index */
	uint32_t offset = AVB_RBIDX_START;
	uint32_t rbidx_len = AVB_RBIDX_LEN;
	uint8_t *rbidx = malloc(rbidx_len);
	if (rbidx == NULL)
		return -1;
	memset(rbidx, 0, rbidx_len);
	*(uint64_t *)rbidx = AVB_RBIDX_INITVAL;
	for (i = 0; i < AVB_MAX_NUMBER_OF_ROLLBACK_INDEX_LOCATIONS; i++) {
		tag = &hdr.rbk_tags[i];
		tag->flag = AVB_RBIDX_FLAG;
		tag->offset = offset;
		tag->len = rbidx_len;
		if (rpmb_write(mmc_dev, rbidx, tag->len, tag->offset) != 0) {
			ERR("write RBKIDX RPMB error\n");
			free(rbidx);
			return -1;
		}
		offset += AVB_RBIDX_ALIGN;
	}
	free(rbidx);

	/* init hdr */
	memcpy(hdr.magic, AVB_KBLB_MAGIC, AVB_KBLB_MAGIC_LEN);
	if (rpmb_write(mmc_dev, (uint8_t *)&hdr, sizeof(hdr), 0) != 0) {
		ERR("write RPMB hdr error\n");
		return -1;
	}

	return 0;
}

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
	kblb_hdr_t hdr;
	kblb_tag_t *pubk;
	uint8_t *extract_key = NULL;
	struct mmc *mmc_dev;
	AvbIOResult ret;

	assert(ops != NULL && out_is_trusted != NULL);
	*out_is_trusted = false;

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

	/* read public key */
	pubk = &hdr.pubk_tag;
	if (pubk->len != public_key_length){
		ERR("avbkey len not match\n");
		return AVB_IO_RESULT_ERROR_IO;
	}
	extract_key = malloc(pubk->len);
	if (extract_key == NULL)
		return AVB_IO_RESULT_ERROR_OOM;

	if (rpmb_read(mmc_dev, extract_key, pubk->len, pubk->offset) != 0) {
		ERR("read public keyblob error\n");
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
AvbIOResult fsl_read_rollback_index_rpmb(AvbOps* ops, size_t rollback_index_slot,
				    uint64_t* out_rollback_index) {
	kblb_hdr_t hdr;
	kblb_tag_t *rbk;
	uint64_t *extract_idx = NULL;
	struct mmc *mmc_dev;
	AvbIOResult ret;

	assert(ops != NULL && out_rollback_index != NULL);
	*out_rollback_index = ~0;

	DEBUGAVB("[rpmb] read rollback slot: %zu\n", rollback_index_slot);

	if (rollback_index_slot >= AVB_MAX_NUMBER_OF_ROLLBACK_INDEX_LOCATIONS)
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

	rbk = &hdr.rbk_tags[rollback_index_slot];
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
	kblb_hdr_t hdr;
	kblb_tag_t *rbk;
	uint64_t *plain_idx = NULL;
	struct mmc *mmc_dev;
	AvbIOResult ret;

	DEBUGAVB("[rpmb] write to rollback slot: (%zu, %" PRIu64 ")\n",
			rollback_index_slot, rollback_index);

	assert(ops != NULL);

	if (rollback_index_slot >= AVB_MAX_NUMBER_OF_ROLLBACK_INDEX_LOCATIONS)
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

	rbk = &hdr.rbk_tags[rollback_index_slot];
	plain_idx = malloc(rbk->len);
	if (plain_idx == NULL)
		return AVB_IO_RESULT_ERROR_OOM;
	memset(plain_idx, 0, rbk->len);
	*plain_idx = rollback_index;

	/* write rollback_index keyblob */
	if (rpmb_write(mmc_dev, (uint8_t *)plain_idx, rbk->len, rbk->offset) != 0) {
		ERR("write rollback index error\n");
		ret = AVB_IO_RESULT_ERROR_IO;
		goto fail;
	}
	ret = AVB_IO_RESULT_OK;
fail:
	if (plain_idx != NULL)
		free(plain_idx);
	return ret;
}
