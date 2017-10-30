/*
 * Copyright (C) 2016 Freescale Semiconductor, Inc.
 * Copyright 2017 NXP
 * SPDX-License-Identifier:     GPL-2.0+
 *
 */

#include <common.h>
#include <stdlib.h>
#ifdef CONFIG_FSL_CAAM_KB
#include <fsl_caam.h>
#endif
#include <fuse.h>
#include <mmc.h>
#include <hash.h>
#include <mapmem.h>

#include <fsl_avb.h>
#include "fsl_avbkey.h"
#include "fsl_public_key.h"
#include "fsl_atx_attributes.h"
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
#define RPMBKEY_BLOB_LEN ((RPMBKEY_LENGTH) + (CAAM_PAD))

#ifdef CONFIG_AVB_ATX
#define ATX_FUSE_BANK_NUM  4
#define ATX_FUSE_BANK_MASK 0xFFFF
#define ATX_HASH_LENGTH    14
#endif

#define RESULT_ERROR -1
#define RESULT_OK     0

#ifndef CONFIG_FSL_CAAM_KB
/* ARM64 won't avbkey and rollback index in this stage directly. */
int avbkey_init(uint8_t *plainkey, uint32_t keylen) {
	return 0;
}

int rbkidx_erase(void) {
	return 0;
}

/*
 * In no security enhanced ARM64, we cannot protect public key.
 * So that we choose to trust the key from vbmeta image
 */
AvbIOResult fsl_validate_vbmeta_public_key_rpmb(AvbOps* ops,
					   const uint8_t* public_key_data,
					   size_t public_key_length,
					   const uint8_t* public_key_metadata,
					   size_t public_key_metadata_length,
					   bool* out_is_trusted) {
	*out_is_trusted = true;
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
#else
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
				if (fuse_sense(bs, ws, buffer)) {
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

#if defined(AVB_RPMB) && defined(CONFIG_AVB_ATX)
static int sha256(unsigned char* data, int len, unsigned char* output) {
	struct hash_algo *algo;
	void *buf;

	if (hash_lookup_algo("sha256", &algo)) {
		printf("error in lookup sha256 algo!\n");
		return RESULT_ERROR;
	}
	buf = map_sysmem((ulong)data, len);
	algo->hash_func_ws(buf, len, output, algo->chunk_size);
	unmap_sysmem(buf);

	return algo->digest_size;
}

static int permanent_attributes_sha256_hash(unsigned char* output) {
	AvbAtxPermanentAttributes attributes;

	/* get permanent attributes */
	attributes.version = fsl_version;
	memcpy(attributes.product_root_public_key, fsl_product_root_public_key,
			sizeof(fsl_product_root_public_key));
	memcpy(attributes.product_id, fsl_atx_product_id,
			sizeof(fsl_atx_product_id));
	/* calculate sha256(permanent attributes) hash */
	if (sha256((unsigned char *)&attributes, sizeof(AvbAtxPermanentAttributes),
			output) == RESULT_ERROR) {
		printf("ERROR - calculate permanent attributes hash error");
		return RESULT_ERROR;
	}

	return RESULT_OK;
}

static int init_permanent_attributes_fuse(void) {
	uint8_t sha256_hash[AVB_SHA256_DIGEST_SIZE];
	uint32_t buffer[ATX_FUSE_BANK_NUM];
	int num = 0;

	/* read first 112 bits of sha256(permanent attributes) from fuse */
	if (fsl_fuse_read(buffer, ATX_FUSE_BANK_NUM, PERMANENT_ATTRIBUTE_HASH_OFFSET)) {
		printf("ERROR - read permanent attributes hash from fuse error\n");
		return RESULT_ERROR;
	}
	/* only take the lower 2 bytes of the last bank */
	buffer[ATX_FUSE_BANK_NUM - 1] &= ATX_FUSE_BANK_MASK;

	/* return RESULT_OK if fuse has been initialized before */
	for (num = 0; num < ATX_FUSE_BANK_NUM; num++) {
		if (buffer[num])
		    return RESULT_OK;
	}

	/* calculate sha256(permanent attributes) */
	if (permanent_attributes_sha256_hash(sha256_hash) != RESULT_OK) {
		return RESULT_ERROR;
	}

	/* write first 112 bits of sha256(permanent attributes) into fuse */
	memset(buffer, 0, sizeof(buffer));
	memcpy(buffer, sha256_hash, ATX_HASH_LENGTH);
	if (fsl_fuse_write(buffer, ATX_FUSE_BANK_NUM, PERMANENT_ATTRIBUTE_HASH_OFFSET)) {
		printf("ERROR - write permanent attributes hash to fuse error\n");
		return RESULT_ERROR;
	}

	return RESULT_OK;
}
#endif

#ifdef AVB_RPMB
static int rpmb_read(struct mmc *mmc, uint8_t *buffer, size_t num_bytes, int64_t offset);
static int rpmb_write(struct mmc *mmc, uint8_t *buffer, size_t num_bytes, int64_t offset);

static int rpmb_init(void) {
	int i;
	kblb_hdr_t hdr;
	kblb_tag_t *tag;
	struct mmc *mmc_dev;
	uint32_t offset;
	uint32_t rbidx_len;
	uint8_t *rbidx;

	/* check init status first */
	if ((mmc_dev = get_mmc()) == NULL) {
		ERR("ERROR - get mmc device\n");
		return -1;
	}
	if (rpmb_read(mmc_dev, (uint8_t *)&hdr, sizeof(hdr), 0) != 0) {
		ERR("read RPMB error\n");
		return -1;
	}
	if (!memcmp(hdr.magic, AVB_KBLB_MAGIC, AVB_KBLB_MAGIC_LEN))
		return 0;
	/* init RPMB if not inited before */
	/* init rollback index */
	offset = AVB_RBIDX_START;
	rbidx_len = AVB_RBIDX_LEN;
	rbidx = malloc(rbidx_len);
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
#ifdef CONFIG_AVB_ATX
	/* init rollback index for Android Things key versions */
	offset = ATX_RBIDX_START;
	rbidx_len = ATX_RBIDX_LEN;
	rbidx = malloc(rbidx_len);
	if (rbidx == NULL)
		return -1;
	memset(rbidx, 0, rbidx_len);
	*(uint64_t *)rbidx = ATX_RBIDX_INITVAL;
	for (i = 0; i < AVB_MAX_NUMBER_OF_ROLLBACK_INDEX_LOCATIONS; i++) {
		tag = &hdr.atx_rbk_tags[i];
		tag->flag = ATX_RBIDX_FLAG;
		tag->offset = offset;
		tag->len = rbidx_len;
		if (rpmb_write(mmc_dev, rbidx, tag->len, tag->offset) != 0) {
			ERR("write ATX_RBKIDX RPMB error\n");
			free(rbidx);
			return -1;
		}
		offset += ATX_RBIDX_ALIGN;
	}
#endif
	free(rbidx);

	/* init hdr */
	memcpy(hdr.magic, AVB_KBLB_MAGIC, AVB_KBLB_MAGIC_LEN);
	if (rpmb_write(mmc_dev, (uint8_t *)&hdr, sizeof(hdr), 0) != 0) {
		ERR("write RPMB hdr error\n");
		return -1;
	}

	return 0;
}

static void fill_secure_keyslot_package(struct keyslot_package *kp) {

	memcpy((void*)CAAM_ARB_BASE_ADDR, kp, sizeof(struct keyslot_package));

	/* invalidate the cache to make sure no critical information left in it */
	memset(kp, 0, sizeof(struct keyslot_package));
	invalidate_dcache_range(((uint32_t)kp) & 0xffffffc0,
		(((((uint32_t)kp) + sizeof(struct keyslot_package)) & 0xffffff00) + 0x100));
}

static int read_keyslot_package(struct keyslot_package* kp) {
	char original_part;
	int blksz;
	unsigned char* fill = NULL;
	int ret = 0;
	/* load tee from boot1 of eMMC. */
	int mmcc = mmc_get_env_dev();
	struct blk_desc *dev_desc = NULL;

	struct mmc *mmc;
	mmc = find_mmc_device(mmcc);
	if (!mmc) {
		printf("boota: cannot find '%d' mmc device\n", mmcc);
		return -1;
	}
	original_part = mmc->block_dev.hwpart;

	dev_desc = blk_get_dev("mmc", mmcc);
	if (NULL == dev_desc) {
		printf("** Block device MMC %d not supported\n", mmcc);
		return -1;
	}

	blksz = dev_desc->blksz;
	fill = (unsigned char *)memalign(ALIGN_BYTES, blksz);

	/* below was i.MX mmc operation code */
	if (mmc_init(mmc)) {
		printf("mmc%d init failed\n", mmcc);
		return -1;
	}

	mmc_switch_part(mmc, KEYSLOT_HWPARTITION_ID);
	if (mmc->block_dev.block_read(dev_desc, KEYSLOT_BLKS,
		    1, fill) != 1) {
		printf("Failed to read rpmbkeyblob.");
		ret = -1;
	} else {
		memcpy(kp, fill, sizeof(struct keyslot_package));
	}

	/* Return to original partition */
	if (mmc->block_dev.hwpart != original_part) {
		if (mmc_switch_part(mmc, original_part) != 0)
			return -1;
		mmc->block_dev.hwpart = original_part;
	}
	if (fill != NULL)
		free(fill);

	return ret;
}

static int gen_rpmb_key(struct keyslot_package *kp) {
	char original_part;
	uint8_t plain_key[RPMBKEY_LENGTH];
	int blksz;

	kp->rpmb_keyblob_len = RPMBKEY_LEN;
	strcpy(kp->magic, KEYPACK_MAGIC);

	int ret = 0;
	/* load tee from boot1 of eMMC. */
	int mmcc = mmc_get_env_dev();
	struct blk_desc *dev_desc = NULL;

	struct mmc *mmc;
	mmc = find_mmc_device(mmcc);
	if (!mmc) {
		printf("boota: cannot find '%d' mmc device\n", mmcc);
		return -1;
	}
	original_part = mmc->block_dev.hwpart;

	dev_desc = blk_get_dev("mmc", mmcc);
	if (NULL == dev_desc) {
		printf("** Block device MMC %d not supported\n", mmcc);
		goto fail;
	}

	blksz = dev_desc->blksz;
	unsigned char* fill = (unsigned char *)memalign(ALIGN_BYTES, blksz);

	/* below was i.MX mmc operation code */
	if (mmc_init(mmc)) {
		printf("mmc%d init failed\n", mmcc);
		goto fail;
	}

	/* Switch to the RPMB partition */

	/* use caam hwrng to generate */
	caam_open();

#ifdef TRUSTY_RPMB_RANDOM_KEY
	/* 
	 * Since boot1 is a bit easy to be erase during development
	 * so that before production stage use full 0 rpmb key
	 */
	if (caam_hwrng(plain_key, RPMBKEY_LENGTH)) {
		ERR("ERROR - caam rng\n");
		ret = -1;
		goto fail;
	}
#else
	memset(plain_key, 0, RPMBKEY_LENGTH);
#endif

	/* generate keyblob and program to fuse */
	if (caam_gen_blob((uint32_t)plain_key, (uint32_t)(kp->rpmb_keyblob), RPMBKEY_LENGTH)) {
		ERR("gen rpmb key blb error\n");
		ret = -1;
		goto fail;
	}
	memcpy(fill, kp, sizeof(struct keyslot_package));

	mmc_switch_part(mmc, KEYSLOT_HWPARTITION_ID);
	if (mmc->block_dev.block_write(dev_desc, KEYSLOT_BLKS,
		    1, (void *)fill) != 1) {
		printf("Failed to write rpmbkeyblob.");
	}

	/* program key to mmc */
	if (mmc->block_dev.hwpart != MMC_PART_RPMB) {
		if (mmc_switch_part(mmc, MMC_PART_RPMB) != 0)
			goto fail;
		mmc->block_dev.hwpart = MMC_PART_RPMB;
	}
	if (mmc_rpmb_set_key(mmc, plain_key)) {
		ERR("Key already programmed ?\n");
		ret = -1;
		goto fail;
	}

	ret = 0;

fail:
	/* Return to original partition */
	if (mmc->block_dev.hwpart != original_part) {
		if (mmc_switch_part(mmc, original_part) != 0)
			return -1;
		mmc->block_dev.hwpart = original_part;
	}
	return ret;

}

int init_avbkey(void) {
	struct keyslot_package kp;
	read_keyslot_package(&kp);
	if (strcmp(kp.magic, KEYPACK_MAGIC)) {
		printf("keyslot package magic error. Will generate new one\n");
		gen_rpmb_key(&kp);
	}
	if (rpmb_init())
		return RESULT_ERROR;
#ifdef CONFIG_AVB_ATX
	if (init_permanent_attributes_fuse())
		return RESULT_ERROR;
#endif
	fill_secure_keyslot_package(&kp);
	return RESULT_OK;
}

#endif

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
	uint8_t *blob = NULL;

#ifdef AVB_RPMB
	struct keyslot_package kp;
#endif

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
	blob = (uint8_t *)memalign(ALIGN_BYTES, RPMBKEY_BLOB_LEN);
#ifdef AVB_RPMB
	if (read_keyslot_package(&kp)) {
#else
	if (fsl_fuse_read((uint32_t *)blob, RPMBKEY_FUSE_LENW, RPMBKEY_FUSE_OFFSET)){
#endif
		ERR("read rpmb key error\n");
		ret = -1;
		goto fail;
	}
	/* copy rpmb key to blob */
#ifdef AVB_RPMB
	memcpy(blob, kp.rpmb_keyblob, RPMBKEY_BLOB_LEN);
#endif
	caam_open();
	if (caam_decap_blob((uint32_t)extract_key, (uint32_t)blob, RPMBKEY_LENGTH)) {
		ERR("decap rpmb key error\n");
		ret = -1;
		goto fail;
	}

	/* alloc a blksz mem */
	bdata = (unsigned char *)memalign(ALIGN_BYTES, blksz);
	if (bdata == NULL) {
		ret = -1;
		goto fail;
	}
	/* one block a time */
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
	if (blob != NULL)
		free(blob);
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
	uint8_t *blob = NULL;

#ifdef AVB_RPMB
	struct keyslot_package kp;
#endif

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
	blob = (uint8_t *)memalign(ALIGN_BYTES, RPMBKEY_BLOB_LEN);
#ifdef AVB_RPMB
	if (read_keyslot_package(&kp)) {
#else
	if (fsl_fuse_read((uint32_t *)blob, RPMBKEY_FUSE_LENW, RPMBKEY_FUSE_OFFSET)){
#endif
		ERR("read rpmb key error\n");
		ret = -1;
		goto fail;
	}
	/* copy rpmb key to blob */
#ifdef AVB_RPMB
	memcpy(blob, kp.rpmb_keyblob, RPMBKEY_BLOB_LEN);
#endif
	caam_open();
	if (caam_decap_blob((uint32_t)extract_key, (uint32_t)blob, RPMBKEY_LENGTH)) {
		ERR("decap rpmb key error\n");
		ret = -1;
		goto fail;
	}
	/* alloc a blksz mem */
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
		if (!s || cnt != blksz) { /* read blk first */
			if (mmc_rpmb_read(mmc, bdata, bs, 1, extract_key) != 1) {
				ERR("mmc_rpmb_read err, mmc= 0x%08x\n", (unsigned int)mmc);
				ret = -1;
				goto fail;
			}
		}
		memcpy(bdata + s, in_buf, cnt); /* change data */
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
	if (blob != NULL)
		free(blob);
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
	AvbIOResult ret;
	assert(ops != NULL && out_is_trusted != NULL);
	*out_is_trusted = false;

	/* match given public key */
	if (memcmp(fsl_public_key, public_key_data, public_key_length)) {
		ret = AVB_IO_RESULT_ERROR_IO;
		ERR("public key not match\n");
		return AVB_IO_RESULT_ERROR_IO;
	}

	*out_is_trusted = true;
	ret = AVB_IO_RESULT_OK;

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
#endif
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
#endif
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
#endif /* CONFIG_FSL_CAAM_KB */

#if defined(AVB_RPMB) && defined(CONFIG_AVB_ATX)
/* Reads permanent |attributes| data. There are no restrictions on where this
 * data is stored. On success, returns AVB_IO_RESULT_OK and populates
 * |attributes|.
 */
AvbIOResult fsl_read_permanent_attributes(
    AvbAtxOps* atx_ops, AvbAtxPermanentAttributes* attributes) {
	/* use hard code permanent attributes due to limited fuse and RPMB */
	attributes->version = fsl_version;
	memcpy(attributes->product_root_public_key, fsl_product_root_public_key,
		sizeof(fsl_product_root_public_key));
	memcpy(attributes->product_id, fsl_atx_product_id, sizeof(fsl_atx_product_id));

	return AVB_IO_RESULT_OK;
}

/* Reads a |hash| of permanent attributes. This hash MUST be retrieved from a
 * permanently read-only location (e.g. fuses) when a device is LOCKED. On
 * success, returned AVB_IO_RESULT_OK and populates |hash|.
 */
AvbIOResult fsl_read_permanent_attributes_hash(
    AvbAtxOps* atx_ops, uint8_t hash[AVB_SHA256_DIGEST_SIZE]) {
	uint8_t sha256_hash_buf[AVB_SHA256_DIGEST_SIZE];
	uint32_t sha256_hash_fuse[ATX_FUSE_BANK_NUM];

	/* read first 112 bits of sha256(permanent attributes) from fuse */
	if (fsl_fuse_read(sha256_hash_fuse, ATX_FUSE_BANK_NUM,
		    PERMANENT_ATTRIBUTE_HASH_OFFSET)) {
		printf("ERROR - read permanent attributes hash from fuse error\n");
		return AVB_IO_RESULT_ERROR_IO;
	}
	/* only take the lower 2 bytes of last bank */
	sha256_hash_fuse[ATX_FUSE_BANK_NUM - 1] &= ATX_FUSE_BANK_MASK;

	/* calculate sha256(permanent attributes) */
	if (permanent_attributes_sha256_hash(sha256_hash_buf) != RESULT_OK) {
		return AVB_IO_RESULT_ERROR_IO;
	}
	/* check if the sha256(permanent attributes) hash match */
	if (memcmp(sha256_hash_fuse, sha256_hash_buf, ATX_HASH_LENGTH)) {
		printf("ERROR - sha256(permanent attributes) does not match\n");
		return AVB_IO_RESULT_ERROR_IO;
	}

	memcpy(hash, sha256_hash_buf, AVB_SHA256_DIGEST_SIZE);
	return AVB_IO_RESULT_OK;
}
#endif
