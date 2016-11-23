/*
 * Copyright (C) 2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <stdlib.h>
#include <fsl_caam.h>
#include <fuse.h>

#include <fsl_avb.h>
#include "fsl_avbkey.h"
#include "debug.h"

/* bank 15, GP7, 0xc80[31:0] */
#define AVBKEY_FUSE_BANK 15
#define AVBKEY_FUSE_WORD 0
#define AVBKEY_FUSE_MASK 0xffffffff
#define AVBKEY_FUSE_INIT 0x4156424b /* 'avbk' */


static int encrypt_write(uint8_t *plain, uint32_t len, const char * part, size_t offset) {

	uint8_t *blb;
	uint32_t blbsize;
	int ret;

	blbsize = len + AVB_CAAM_PAD;
	blb = (uint8_t *)malloc(blbsize);
	if (blb == NULL)
		return -1;

	caam_open();
	if (caam_gen_blob((uint32_t)plain, (uint32_t)blb, len)) {
		ret = -1;
		goto fail;
	}

	if (fsl_write_to_partition(NULL, part, offset, blbsize,
			(void *)blb) != AVB_IO_RESULT_OK) {
		ret = -1;
		goto fail;
	}
	ret = 0;

fail:
	free(blb);
	return ret;
}

int rbkidx_erase(const char * kblb_part) {
	int i;
	size_t num_read;
	kblb_hdr_t hdr;
	kblb_tag_t *tag;
	/* read the kblb header */
	if (fsl_read_from_partition(NULL, kblb_part, 0, sizeof(hdr),
			(void *)&hdr, &num_read) != AVB_IO_RESULT_OK) {
		ERR("read partition avbkey error\n");
		return -1;
	}
	if (num_read != sizeof(hdr) ||
			memcmp(hdr.magic, AVB_KBLB_MAGIC, AVB_KBLB_MAGIC_LEN) != 0) {
		ERR("avbkey partition magic not match\n");
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
	for (i = 0; i < AVB_MAX_NUMBER_OF_ROLLBACK_INDEX_SLOTS; i++) {
		tag = &hdr.rbk_tags[i];
		tag->flag = AVB_RBIDX_FLAG;
		tag->offset = offset;
		tag->len = rbidx_len;
		/* caam encrypt and write */
		if (encrypt_write(rbidx, tag->len, kblb_part, tag->offset) != 0) {
			ERR("write rollback index keyblob error\n");
			free(rbidx);
			return -1;
		}
		offset += AVB_RBIDX_ALIGN;
	}
	free(rbidx);
	/* write hdr */
	if (fsl_write_to_partition(NULL, kblb_part, 0,
				sizeof(hdr), (void *)&hdr) != AVB_IO_RESULT_OK) {
		ERR("write avbkey hdr error\n");
		return -1;
	}
	return 0;
}

int avbkeyblb_init(uint8_t *plainkey, uint32_t keylen, const char * kblb_part) {
	int i;
	kblb_hdr_t hdr;
	kblb_tag_t *tag;
	uint32_t fuse_val;

	/* read fuse to check enable init */
	/* fuse_read read the shadow reg of fuse
	 * use fuse_sense to real read fuse */
#ifdef CONFIG_AVB_FUSE
	if (fuse_sense(AVBKEY_FUSE_BANK, AVBKEY_FUSE_WORD, &fuse_val)) {
#else
	if (fuse_read(AVBKEY_FUSE_BANK, AVBKEY_FUSE_WORD, &fuse_val)) {
#endif
		ERR("read fuse error\n");
		return -1;
	}
	if ((fuse_val & AVBKEY_FUSE_MASK) == AVBKEY_FUSE_INIT) {
		ERR("key already init\n");
		return -1;
	}
	fuse_val = AVBKEY_FUSE_MASK & AVBKEY_FUSE_INIT;

	/* write fuse to prevent init again */
	/* fuse_override write the shadow reg of fuse
	 * use fuse_prog to PERMANENT write fuse */
#ifdef CONFIG_AVB_FUSE
	if (fuse_prog(AVBKEY_FUSE_BANK, AVBKEY_FUSE_WORD, fuse_val)) {
#else
	if (fuse_override(AVBKEY_FUSE_BANK, AVBKEY_FUSE_WORD, fuse_val)) {
#endif
		ERR("write fuse error\n");
		return -1;
	}

	assert(plainkey != NULL);

	/* check overflow */
	if (keylen > AVB_RBIDX_START - AVB_PUBKY_OFFSET) {
		ERR("key len overflow\n");
		return -1;
	}

	/* init pubkey */
	tag = &hdr.pubk_tag;
	tag->flag = AVB_PUBKY_FLAG;
	tag->offset = AVB_PUBKY_OFFSET;
	tag->len = keylen;
	/* caam encrypt and write */
	if (encrypt_write(plainkey, tag->len, kblb_part, tag->offset) != 0) {
		ERR("write pubkey keyblob error\n");
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
	for (i = 0; i < AVB_MAX_NUMBER_OF_ROLLBACK_INDEX_SLOTS; i++) {
		tag = &hdr.rbk_tags[i];
		tag->flag = AVB_RBIDX_FLAG;
		tag->offset = offset;
		tag->len = rbidx_len;
		/* caam encrypt and write */
		if (encrypt_write(rbidx, tag->len, kblb_part, tag->offset) != 0) {
			ERR("write rollback index keyblob error\n");
			free(rbidx);
			return -1;
		}
		offset += AVB_RBIDX_ALIGN;
	}
	free(rbidx);

	/* init hdr */
	memcpy(hdr.magic, AVB_KBLB_MAGIC, AVB_KBLB_MAGIC_LEN);
	if (fsl_write_to_partition(NULL, kblb_part, 0,
				sizeof(hdr), (void *)&hdr) != AVB_IO_RESULT_OK) {
		ERR("write avbkey hdr error\n");
		return -1;
	}
	return 0;
}
