/*
 * Copyright (C) 2016 Freescale Semiconductor, Inc.
 * Copyright 2017-2019 NXP
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
#include <hang.h>
#include <cpu_func.h>

#include <fsl_avb.h>
#include "trusty/avb.h"
#ifdef CONFIG_IMX_TRUSTY_OS
#include <trusty/libtipc.h>
#endif
#include "fsl_avbkey.h"
#include "utils.h"
#include "debug.h"
#include <memalign.h>
#include "trusty/hwcrypto.h"
#include "fsl_atx_attributes.h"
#include <asm/mach-imx/hab.h>
#include <asm/arch/sys_proto.h>
#ifdef CONFIG_ARCH_IMX8
#include <asm/arch/sci/sci.h>
#endif
#ifdef CONFIG_SPL_BUILD
#include <spl.h>
#endif

#define INITFLAG_FUSE_OFFSET 0
#define INITFLAG_FUSE_MASK 0x00000001
#define INITFLAG_FUSE 0x00000001

#define RPMB_BLKSZ 256
#define RPMBKEY_LENGTH 32
#define RPMBKEY_BLOB_LEN ((RPMBKEY_LENGTH) + (CAAM_PAD))

extern int mmc_switch(struct mmc *mmc, u8 set, u8 index, u8 value);

#if defined(CONFIG_SPL_BUILD) && defined(CONFIG_SPL_MMC_SUPPORT)
int spl_get_mmc_dev(void)
{
	u32 dev_no = spl_boot_device();
	switch (dev_no) {
	case BOOT_DEVICE_MMC1:
		return 0;
	case BOOT_DEVICE_MMC2:
	case BOOT_DEVICE_MMC2_2:
		return 1;
	}

#ifdef CONFIG_SPL_LIBCOMMON_SUPPORT
	printf("spl: unsupported mmc boot device.\n");
#endif

	return -ENODEV;
}
#endif

#ifdef AVB_RPMB
struct mmc *get_mmc(void) {
	int mmc_dev_no;
	struct mmc *mmc;
#if defined(CONFIG_SPL_BUILD) && defined(CONFIG_SPL_MMC_SUPPORT)
	mmc_dev_no = spl_get_mmc_dev();
#else
	mmc_dev_no = mmc_get_env_dev();
#endif
	mmc = find_mmc_device(mmc_dev_no);
	if (!mmc || mmc_init(mmc))
		return NULL;
	return mmc;
}

void fill_secure_keyslot_package(struct keyslot_package *kp) {

	memcpy((void*)CAAM_ARB_BASE_ADDR, kp, sizeof(struct keyslot_package));

	/* invalidate the cache to make sure no critical information left in it */
	memset(kp, 0, sizeof(struct keyslot_package));
	invalidate_dcache_range(((ulong)kp) & 0xffffffc0,(((((ulong)kp) +
				sizeof(struct keyslot_package)) & 0xffffff00) +
				0x100));
}

int read_keyslot_package(struct keyslot_package* kp) {
	char original_part;
	int blksz;
	unsigned char* fill = NULL;
	int ret = 0;
	/* load tee from boot1 of eMMC. */
#if defined(CONFIG_SPL_BUILD) && defined(CONFIG_SPL_MMC_SUPPORT)
	int mmcc = spl_get_mmc_dev();
#else
	int mmcc = mmc_get_env_dev();
#endif
	struct blk_desc *dev_desc = NULL;

	struct mmc *mmc;
#ifdef CONFIG_IMX8_TRUSTY_XEN
	mmcc = 0;
#endif
	mmc = find_mmc_device(mmcc);
	if (!mmc) {
		printf("boota: cannot find '%d' mmc device\n", mmcc);
		return -1;
	}
#if !CONFIG_IS_ENABLED(BLK)
	original_part = mmc->block_dev.hwpart;
	dev_desc = blk_get_dev("mmc", mmcc);
#else
	dev_desc = mmc_get_blk_desc(mmc);
#endif
	if (NULL == dev_desc) {
		printf("** Block device MMC %d not supported\n", mmcc);
		return -1;
	}
#if CONFIG_IS_ENABLED(BLK)
	original_part = dev_desc->hwpart;
#endif

	blksz = dev_desc->blksz;
	fill = (unsigned char *)memalign(ALIGN_BYTES, blksz);

	/* below was i.MX mmc operation code */
	if (mmc_init(mmc)) {
		printf("mmc%d init failed\n", mmcc);
		ret = -1;
		goto fail;;
	}

	if (mmc_switch_part(mmc, KEYSLOT_HWPARTITION_ID) != 0) {
		ret = -1;
		goto fail;
	}
#if !CONFIG_IS_ENABLED(BLK)
	mmc->block_dev.hwpart = KEYSLOT_HWPARTITION_ID;
#else
	dev_desc->hwpart = KEYSLOT_HWPARTITION_ID;
#endif
	if (blk_dread(dev_desc, KEYSLOT_BLKS,
		    1, fill) != 1) {
		printf("Failed to read rpmbkeyblob.");
		ret = -1;
		goto fail;
	} else {
		memcpy(kp, fill, sizeof(struct keyslot_package));
	}

fail:
	/* Free allocated memory. */
	if (fill != NULL)
		free(fill);
	/* Return to original partition */
#if !CONFIG_IS_ENABLED(BLK)
	if (mmc->block_dev.hwpart != original_part) {
		if (mmc_switch_part(mmc, original_part) != 0)
			return -1;
		mmc->block_dev.hwpart = original_part;
	}
#else
	if (dev_desc->hwpart != original_part) {
		if (mmc_switch_part(mmc, original_part) != 0)
			return -1;
		dev_desc->hwpart = original_part;
	}
#endif
	return ret;
}

bool rpmbkey_is_set(void)
{
	int mmcc;
	bool ret;
	uint8_t *buf;
	struct mmc *mmc;
	char original_part;
	struct blk_desc *desc = NULL;

	/* Get current mmc device. */
#if defined(CONFIG_SPL_BUILD) && defined(CONFIG_SPL_MMC_SUPPORT)
	mmcc = spl_get_mmc_dev();
#else
	mmcc = mmc_get_env_dev();
#endif
	mmc = find_mmc_device(mmcc);
	if (!mmc) {
		printf("error - cannot find '%d' mmc device\n", mmcc);
		return false;
	}

#if !CONFIG_IS_ENABLED(BLK)
	original_part = mmc->block_dev.hwpart;
	desc = blk_get_dev("mmc", mmcc);
#else
	desc = mmc_get_blk_desc(mmc);
	original_part = desc->hwpart;
#endif

	/* Switch to the RPMB partition */
#if !CONFIG_IS_ENABLED(BLK)
	if (mmc->block_dev.hwpart != MMC_PART_RPMB) {
#else
	if (desc->hwpart != MMC_PART_RPMB) {
#endif
		if (mmc_switch_part(mmc, MMC_PART_RPMB) != 0) {
			printf("ERROR - can't switch to rpmb partition \n");
			return false;
		}
#if !CONFIG_IS_ENABLED(BLK)
		mmc->block_dev.hwpart = MMC_PART_RPMB;
#else
		desc->hwpart = MMC_PART_RPMB;
#endif
	}

	/* Try to read the first one block, return count '1' means the rpmb
	 * key has been set, otherwise means the key hasn't been set.
	 */
	buf = (uint8_t *)memalign(ALIGN_BYTES, desc->blksz);
	if (mmc_rpmb_read(mmc, buf, 0, 1, NULL) != 1)
		ret = false;
	else
		ret = true;

	/* return to original partition. */
#if !CONFIG_IS_ENABLED(BLK)
	if (mmc->block_dev.hwpart != original_part) {
#else
	if (desc->hwpart != original_part) {
#endif
		if (mmc_switch_part(mmc, original_part) != 0)
			ret = false;
#if !CONFIG_IS_ENABLED(BLK)
		mmc->block_dev.hwpart = original_part;
#else
		desc->hwpart = original_part;
#endif
	}
	/* remember to free the buffer */
	if (buf != NULL)
		free(buf);

	return ret;
}

#ifdef CONFIG_FSL_CAAM_KB
int rpmb_read(struct mmc *mmc, uint8_t *buffer, size_t num_bytes, int64_t offset) {

	unsigned char *bdata = NULL;
	unsigned char *out_buf = (unsigned char *)buffer;
	unsigned long s, cnt;
	unsigned long blksz;
	size_t num_read = 0;
	unsigned short part_start, part_length, part_end, bs, be;
	margin_pos_t margin;
	char original_part;
	uint8_t *blob = NULL;
	struct blk_desc *desc = mmc_get_blk_desc(mmc);
	ALLOC_CACHE_ALIGN_BUFFER(uint8_t, extract_key, RPMBKEY_LENGTH);

	struct keyslot_package kp;
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
	original_part = desc->hwpart;
	if (desc->hwpart != MMC_PART_RPMB) {
		if (mmc_switch_part(mmc, MMC_PART_RPMB) != 0)
			return -1;
		desc->hwpart = MMC_PART_RPMB;
	}

	/* get rpmb key */
	blob = (uint8_t *)memalign(ARCH_DMA_MINALIGN, RPMBKEY_BLOB_LEN);
	if (read_keyslot_package(&kp)) {
		ERR("read rpmb key error\n");
		ret = -1;
		goto fail;
	}
	/* copy rpmb key to blob */
	memcpy(blob, kp.rpmb_keyblob, RPMBKEY_BLOB_LEN);
	caam_open();
	if (caam_decap_blob((ulong)extract_key, (ulong)blob,
				RPMBKEY_LENGTH)) {
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
	if (desc->hwpart != original_part) {
		if (mmc_switch_part(mmc, original_part) != 0)
			ret = -1;
		else
			desc->hwpart = original_part;
	}
	if (blob != NULL)
		free(blob);
	if (bdata != NULL)
		free(bdata);
	return ret;

}

int rpmb_write(struct mmc *mmc, uint8_t *buffer, size_t num_bytes, int64_t offset) {

	unsigned char *bdata = NULL;
	unsigned char *in_buf = (unsigned char *)buffer;
	unsigned long s, cnt;
	unsigned long blksz;
	size_t num_write = 0;
	unsigned short part_start, part_length, part_end, bs;
	margin_pos_t margin;
	char original_part;
	uint8_t *blob = NULL;
	struct blk_desc *desc = mmc_get_blk_desc(mmc);
	ALLOC_CACHE_ALIGN_BUFFER(uint8_t, extract_key, RPMBKEY_LENGTH);

	struct keyslot_package kp;
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
	original_part = desc->hwpart;
	if (desc->hwpart != MMC_PART_RPMB) {
		if (mmc_switch_part(mmc, MMC_PART_RPMB) != 0)
			return -1;
		desc->hwpart = MMC_PART_RPMB;
	}

	/* get rpmb key */
	blob = (uint8_t *)memalign(ARCH_DMA_MINALIGN, RPMBKEY_BLOB_LEN);
	if (read_keyslot_package(&kp)) {
		ERR("read rpmb key error\n");
		ret = -1;
		goto fail;
	}
	/* copy rpmb key to blob */
	memcpy(blob, kp.rpmb_keyblob, RPMBKEY_BLOB_LEN);
	caam_open();
	if (caam_decap_blob((ulong)extract_key, (ulong)blob,
				RPMBKEY_LENGTH)) {
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
				ERR("mmc_rpmb_read err, mmc= 0x%08x\n", (uint32_t)(ulong)mmc);
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
	if (desc->hwpart != original_part) {
		if (mmc_switch_part(mmc, original_part) != 0)
			ret = -1;
		else
			desc->hwpart = original_part;
	}
	if (blob != NULL)
		free(blob);
	if (bdata != NULL)
		free(bdata);

	return ret;

}

int rpmb_init(void) {
#if !defined(CONFIG_SPL_BUILD) || !defined(CONFIG_DUAL_BOOTLOADER)
	int i;
#endif
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
	/* The bootloader rollback index is stored in the last 8k bytes of
	 * RPMB which is different from the rollback index for vbmeta and
	 * ATX key versions.
	 */
#if defined(CONFIG_SPL_BUILD) && defined(CONFIG_DUAL_BOOTLOADER)
	if (rpmb_read(mmc_dev, (uint8_t *)&hdr, sizeof(hdr),
			BOOTLOADER_RBIDX_OFFSET) != 0) {
#else
	if (rpmb_read(mmc_dev, (uint8_t *)&hdr, sizeof(hdr), 0) != 0) {
#endif
		ERR("read RPMB error\n");
		return -1;
	}
	if (!memcmp(hdr.magic, AVB_KBLB_MAGIC, AVB_KBLB_MAGIC_LEN))
		return 0;
	else
		printf("initialize rollback index...\n");
	/* init rollback index */
#if defined(CONFIG_SPL_BUILD) && defined(CONFIG_DUAL_BOOTLOADER)
	offset = BOOTLOADER_RBIDX_START;
	rbidx_len = BOOTLOADER_RBIDX_LEN;
	rbidx = malloc(rbidx_len);
	if (rbidx == NULL) {
		ERR("failed to allocate memory!\n");
		return -1;
	}
	memset(rbidx, 0, rbidx_len);
	*(uint64_t *)rbidx = BOOTLOADER_RBIDX_INITVAL;
	tag = &hdr.bootloader_rbk_tags;
	tag->offset = offset;
	tag->len = rbidx_len;
	if (rpmb_write(mmc_dev, rbidx, tag->len, tag->offset) != 0) {
		ERR("write RBKIDX RPMB error\n");
		free(rbidx);
		return -1;
	}
	if (rbidx != NULL)
		free(rbidx);
#else /* CONFIG_SPL_BUILD && CONFIG_DUAL_BOOTLOADER */
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
	if (rbidx != NULL)
		free(rbidx);
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
	if (rbidx != NULL)
		free(rbidx);
#endif
#endif /* CONFIG_SPL_BUILD && CONFIG_DUAL_BOOTLOADER */

	/* init hdr */
	memcpy(hdr.magic, AVB_KBLB_MAGIC, AVB_KBLB_MAGIC_LEN);
#if defined(CONFIG_SPL_BUILD) && defined(CONFIG_DUAL_BOOTLOADER)
	if (rpmb_write(mmc_dev, (uint8_t *)&hdr, sizeof(hdr),
			BOOTLOADER_RBIDX_OFFSET) != 0) {
#else
	if (rpmb_write(mmc_dev, (uint8_t *)&hdr, sizeof(hdr), 0) != 0) {
#endif
		ERR("write RPMB hdr error\n");
		return -1;
	}

	return 0;
}

int gen_rpmb_key(struct keyslot_package *kp) {
	char original_part;
	unsigned char* fill = NULL;
	int blksz;
	ALLOC_CACHE_ALIGN_BUFFER(uint8_t, plain_key, RPMBKEY_LENGTH);

	kp->rpmb_keyblob_len = RPMBKEY_LEN;
	strcpy(kp->magic, KEYPACK_MAGIC);

	int ret = -1;
	/* load tee from boot1 of eMMC. */
#if defined(CONFIG_SPL_BUILD) && defined(CONFIG_SPL_MMC_SUPPORT)
	int mmcc = spl_get_mmc_dev();
#else
	int mmcc = mmc_get_env_dev();
#endif
	struct blk_desc *dev_desc = NULL;

	struct mmc *mmc;
	mmc = find_mmc_device(mmcc);
	if (!mmc) {
		printf("boota: cannot find '%d' mmc device\n", mmcc);
		return -1;
	}
#if !CONFIG_IS_ENABLED(BLK)
	original_part = mmc->block_dev.hwpart;
	dev_desc = blk_get_dev("mmc", mmcc);
#else
	dev_desc = mmc_get_blk_desc(mmc);
	original_part = dev_desc->hwpart;
#endif
	if (NULL == dev_desc) {
		printf("** Block device MMC %d not supported\n", mmcc);
		goto fail;
	}

	blksz = dev_desc->blksz;
	fill = (unsigned char *)memalign(ALIGN_BYTES, blksz);

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
		goto fail;
	}
#else
	memset(plain_key, 0, RPMBKEY_LENGTH);
#endif

	/* generate keyblob and program to boot1 partition */
	if (caam_gen_blob((ulong)plain_key, (ulong)(kp->rpmb_keyblob),
				RPMBKEY_LENGTH)) {
		ERR("gen rpmb key blb error\n");
		goto fail;
	}
	memcpy(fill, kp, sizeof(struct keyslot_package));

	if (mmc_switch_part(mmc, KEYSLOT_HWPARTITION_ID) != 0) {
		ret = -1;
		goto fail;
	}

	if (blk_dwrite(dev_desc, KEYSLOT_BLKS,
		    1, (void *)fill) != 1) {
		printf("Failed to write rpmbkeyblob.");
		goto fail;
	}

	/* program key to mmc */
#if !CONFIG_IS_ENABLED(BLK)
	if (mmc->block_dev.hwpart != MMC_PART_RPMB) {
		if (mmc_switch_part(mmc, MMC_PART_RPMB) != 0) {
			ret = -1;
			goto fail;
		} else
			mmc->block_dev.hwpart = MMC_PART_RPMB;
	}
#else
	if (dev_desc->hwpart != MMC_PART_RPMB) {
		if (mmc_switch_part(mmc, MMC_PART_RPMB) != 0) {
			ret = -1;
			goto fail;
		} else
			dev_desc->hwpart = MMC_PART_RPMB;
	}
#endif
	if (mmc_rpmb_set_key(mmc, plain_key)) {
		ERR("Key already programmed ?\n");
		goto fail;
	}

	ret = 0;

fail:
	/* Return to original partition */
#if !CONFIG_IS_ENABLED(BLK)
	if (mmc->block_dev.hwpart != original_part) {
		if (mmc_switch_part(mmc, original_part) != 0)
			ret = -1;
		else
			mmc->block_dev.hwpart = original_part;
	}
#else
	if (dev_desc->hwpart != original_part) {
		if (mmc_switch_part(mmc, original_part) != 0)
			ret = -1;
		else
			dev_desc->hwpart = original_part;
	}
#endif
	if (fill != NULL)
		free(fill);

	return ret;

}

int init_avbkey(void) {
	struct keyslot_package kp;
	read_keyslot_package(&kp);
	if (strcmp(kp.magic, KEYPACK_MAGIC)) {
		printf("keyslot package magic error. Will generate new one\n");
		memset((void *)&kp, 0, sizeof(struct keyslot_package));
		gen_rpmb_key(&kp);
	}
#ifndef CONFIG_IMX_TRUSTY_OS
	if (rpmb_init())
		return RESULT_ERROR;
#endif
#if defined(CONFIG_AVB_ATX) && !defined(CONFIG_IMX_TRUSTY_OS)
	if (init_permanent_attributes_fuse())
		return RESULT_ERROR;
#endif
	fill_secure_keyslot_package(&kp);
	return RESULT_OK;
}

#ifndef CONFIG_IMX_TRUSTY_OS
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
#endif /* CONFIG_IMX_TRUSTY_OS */
#endif /* CONFIG_FSL_CAAM_KB */
#else /* AVB_RPMB */
int rbkidx_erase(void) {
	return 0;
}
#endif /* AVB_RPMB */

#ifdef CONFIG_SPL_BUILD
#if defined (CONFIG_IMX8_TRUSTY_XEN) || \
	(defined(CONFIG_IMX_TRUSTY_OS) && !defined(CONFIG_AVB_ATX))
int check_rpmb_blob(struct mmc *mmc)
{
	int ret = 0;
	char original_part;
	struct keyslot_package kp;
#if CONFIG_IS_ENABLED(BLK)
	struct blk_desc *dev_desc = NULL;
#endif

	read_keyslot_package(&kp);
	if (strcmp(kp.magic, KEYPACK_MAGIC)) {
		if (rpmbkey_is_set()) {
			printf("\nFATAL - RPMB key was destroyed!\n");
			hang();
		} else {
			printf("keyslot package magic error, do nothing here!\n");
			return 0;
		}
	}
	/* If keyslot package valid, copy it to secure memory */
	fill_secure_keyslot_package(&kp);

	/* switch to boot1 partition. */
#if !CONFIG_IS_ENABLED(BLK)
	original_part = mmc->block_dev.hwpart;
#else
	dev_desc = mmc_get_blk_desc(mmc);
	original_part = dev_desc->hwpart;
#endif
	if (mmc_switch_part(mmc, KEYSLOT_HWPARTITION_ID) != 0) {
		printf("ERROR - can't switch to boot1 partition! \n");
		ret = -1;
		goto fail;
	} else
#if !CONFIG_IS_ENABLED(BLK)
		mmc->block_dev.hwpart = KEYSLOT_HWPARTITION_ID;
#else
		dev_desc->hwpart = KEYSLOT_HWPARTITION_ID;
#endif
	/* write power-on write protection for boot1 partition. */
	if (mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL,
			EXT_CSD_BOOT_WP, BOOT1_PWR_WP)) {
		printf("ERROR - unable to set power-on write protection!\n");
		ret = -1;
		goto fail;
	}
fail:
	/* return to original partition. */
#if !CONFIG_IS_ENABLED(BLK)
	if (mmc->block_dev.hwpart != original_part) {
		if (mmc_switch_part(mmc, original_part) != 0)
			return -1;
		mmc->block_dev.hwpart = original_part;
	}
#else
	if (dev_desc->hwpart != original_part) {
		if (mmc_switch_part(mmc, original_part) != 0)
			return -1;
		dev_desc->hwpart = original_part;
	}
#endif

	return ret;
}
#endif /* CONFIG_IMX_TRUSTY_OS && !defined(CONFIG_AVB_ATX) */
#else /* CONFIG_SPL_BUILD */
#ifdef CONFIG_AVB_ATX
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

int fsl_fuse_read(uint32_t *buffer, uint32_t length, uint32_t offset) {

	return fsl_fuse_ops(
		buffer,
		length,
		offset,
		1
		);
}

int fsl_fuse_write(const uint32_t *buffer, uint32_t length, uint32_t offset) {

	return fsl_fuse_ops(
		(uint32_t *)buffer,
		length,
		offset,
		0
		);
}

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

int permanent_attributes_sha256_hash(unsigned char* output) {
	AvbAtxPermanentAttributes attributes;

#ifdef CONFIG_IMX_TRUSTY_OS
	if(!trusty_read_permanent_attributes((uint8_t *)(&attributes),
		sizeof(AvbAtxPermanentAttributes))) {
		goto calc_sha256;
	} else {
		ERR("No perm-attr fused. Will use hard code one.\n");
	}
#endif
	/* get permanent attributes */
	attributes.version = fsl_version;
	memcpy(attributes.product_root_public_key, fsl_product_root_public_key,
			sizeof(fsl_product_root_public_key));
	memcpy(attributes.product_id, fsl_atx_product_id,
			sizeof(fsl_atx_product_id));
#ifdef CONFIG_IMX_TRUSTY_OS
calc_sha256:
#endif
	/* calculate sha256(permanent attributes) hash */
	if (sha256((unsigned char *)&attributes, sizeof(AvbAtxPermanentAttributes),
			output) == RESULT_ERROR) {
		printf("ERROR - calculate permanent attributes hash error");
		return RESULT_ERROR;
	}

	return RESULT_OK;
}

static int init_permanent_attributes_fuse(void) {

#ifdef CONFIG_ARM64
       return RESULT_OK;
#else
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
		printf("ERROR - calculating permanent attributes SHA256 error!\n");
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
#endif /* CONFIG_ARM64 */
}

int avb_atx_fuse_perm_attr(uint8_t *staged_buffer, uint32_t size) {

	if (staged_buffer == NULL) {
		ERR("Error. Get null staged_buffer\n");
		return -1;
	}
	if (size != sizeof(AvbAtxPermanentAttributes)) {
		ERR("Error. expect perm_attr length %u, but get %u.\n",
		(uint32_t)sizeof(AvbAtxPermanentAttributes), size);
		return -1;
	}
#ifdef CONFIG_IMX_TRUSTY_OS
	if (trusty_write_permanent_attributes(staged_buffer, size)) {
		ERR("Error. Failed to write permanent attributes into secure storage\n");
		return -1;
	}
	else
		return init_permanent_attributes_fuse();
#else
	/*
	 * TODO:
	 * Need to handle this when no Trusty OS support.
	 * But now every Android Things will have Trusty OS support.
	 */
	ERR("No Trusty OS enabled in bootloader.\n");
	return 0;
#endif
}

int avb_atx_get_unlock_challenge(struct AvbAtxOps* atx_ops,
				uint8_t *upload_buffer, uint32_t *upload_size)
{
	struct AvbAtxUnlockChallenge *buf = NULL;
	int ret, size;

	size = sizeof(struct AvbAtxUnlockChallenge);
	buf = (struct AvbAtxUnlockChallenge *)malloc(size);
	if (buf == NULL) {
		ERR("unable to alloc memory!\n");
		return -1;
	}

	if (avb_atx_generate_unlock_challenge(atx_ops, buf) !=
			AVB_IO_RESULT_OK) {
		ERR("generate unlock challenge fail!\n");
		ret = -1;
		goto fail;
	}
	/* Current avbtool only accept 16 bytes random numbers as unlock
	 * challenge, need to return the whole 'AvbAtxUnlockChallenge'
	 * when avbtool is ready.
	 */
	memcpy(upload_buffer, buf->challenge, AVB_ATX_UNLOCK_CHALLENGE_SIZE);
	*upload_size = AVB_ATX_UNLOCK_CHALLENGE_SIZE;
	ret = 0;
fail:
	if (buf != NULL)
		free(buf);
	return ret;
}

int avb_atx_verify_unlock_credential(struct AvbAtxOps* atx_ops,
					uint8_t *staged_buffer)
{
	bool out_is_trusted;
	AvbIOResult ret;
	const AvbAtxUnlockCredential* buf = NULL;

	buf = (const AvbAtxUnlockCredential*)staged_buffer;
	ret = avb_atx_validate_unlock_credential(atx_ops, buf, &out_is_trusted);
	if ((ret != AVB_IO_RESULT_OK) || (out_is_trusted != true)) {
		ERR("validate unlock credential fail!\n");
		return -1;
	} else
		return 0;
}

bool perm_attr_are_fused(void)
{
#ifdef CONFIG_IMX_TRUSTY_OS
	AvbAtxPermanentAttributes attributes;
	if(!trusty_read_permanent_attributes((uint8_t *)(&attributes),
		sizeof(AvbAtxPermanentAttributes))) {
		return true;
	} else {
		ERR("No perm-attr fused, please fuse your perm-attr first!.\n");
		return false;
	}
#else
	/* We hard code the perm-attr if trusty is not enabled. */
	return true;
#endif
}

bool at_unlock_vboot_is_disabled(void)
{
	uint32_t unlock_vboot_status;

	if (fsl_fuse_read(&unlock_vboot_status, 1,
				UNLOCK_VBOOT_STATUS_OFFSET_IN_WORD)) {
		printf("Read at unlock vboot status error!\n");
		return false;
	}

	if (unlock_vboot_status & (1 << UNLOCK_VBOOT_STATUS_OFFSET_IN_BIT))
		return true;
	else
		return false;
}

int at_disable_vboot_unlock(void)
{
	uint32_t unlock_vboot_status = 0;

	/* Read the status first */
	if (fsl_fuse_read(&unlock_vboot_status, 1,
				UNLOCK_VBOOT_STATUS_OFFSET_IN_WORD)) {
		ERR("Read unlock vboot status error!\n");
		return -1;
	}

	/* Set the disable unlock vboot bit */
	unlock_vboot_status |= (1 << UNLOCK_VBOOT_STATUS_OFFSET_IN_BIT);

	/* Write disable unlock vboot bit to fuse */
	if (fsl_fuse_write(&unlock_vboot_status, 1,
				UNLOCK_VBOOT_STATUS_OFFSET_IN_WORD)) {
		ERR("Write unlock vboot status fail!\n");
		return -1;
	}

	return 0;
}
#endif /* CONFIG_AVB_ATX */

#if defined(CONFIG_IMX_TRUSTY_OS) && !defined(CONFIG_AVB_ATX)

DECLARE_GLOBAL_DATA_PTR;
extern struct imx_sec_config_fuse_t const imx_sec_config_fuse;
#define HAB_ENABLED_BIT (is_soc_type(MXC_SOC_IMX8M)? 0x2000000 : 0x2)

/* Check hab status, this is basically copied from imx_hab_is_enabled() */
bool hab_is_enabled(void)
{
#ifdef CONFIG_ARCH_IMX8
	sc_err_t err;
	uint16_t lc;

	err = sc_seco_chip_info(-1, &lc, NULL, NULL, NULL);
	if (err != SC_ERR_NONE) {
		printf("Error in get lifecycle\n");
		return false;
	}

	if (lc != 0x80)
#else
	struct imx_sec_config_fuse_t *fuse =
		(struct imx_sec_config_fuse_t *)&imx_sec_config_fuse;
	uint32_t reg;
	int ret;

	ret = fuse_read(fuse->bank, fuse->word, &reg);
	if (ret) {
		puts("\nSecure boot fuse read error!\n");
		return false;
	}

	if (!((reg & HAB_ENABLED_BIT) == HAB_ENABLED_BIT))
#endif
		return false;
	else
		return true;
}

int do_rpmb_key_set(uint8_t *key, uint32_t key_size)
{
	int ret = 0;
	int mmcc;
	struct mmc *mmc;
	char original_part;
	struct keyslot_package kp;
	struct blk_desc *desc = NULL;
	ALLOC_CACHE_ALIGN_BUFFER(uint8_t, rpmb_key, RPMBKEY_LENGTH);
	ALLOC_CACHE_ALIGN_BUFFER(uint8_t, blob,
                                 RPMBKEY_LENGTH + CAAM_PAD);

	/* copy rpmb key to cache aligned buffer. */
	memset(rpmb_key, 0, RPMBKEY_LENGTH);
	memcpy(rpmb_key, key, RPMBKEY_LENGTH);

	/* Get current mmc device. */
#if defined(CONFIG_SPL_BUILD) && defined(CONFIG_SPL_MMC_SUPPORT)
	mmcc = spl_get_mmc_dev();
#else
	mmcc = mmc_get_env_dev();
#endif
	mmc = find_mmc_device(mmcc);
	if (!mmc) {
		printf("error - cannot find '%d' mmc device\n", mmcc);
		return -1;
	}
	desc = mmc_get_blk_desc(mmc);
	original_part = desc->hwpart;

	/* Switch to the RPMB partition */
	if (desc->hwpart != MMC_PART_RPMB) {
		if (mmc_switch_part(mmc, MMC_PART_RPMB) != 0) {
			printf("ERROR - can't switch to rpmb partition \n");
			return -1;
		}
		desc->hwpart = MMC_PART_RPMB;
	}

	if (mmc_rpmb_set_key(mmc, rpmb_key)) {
		printf("ERROR - Key already programmed ?\n");
		ret = -1;
		goto fail;
	} else
		printf("RPMB key programed successfully!\n");

	/* Generate keyblob with CAAM. */
	memset((void *)&kp, 0, sizeof(struct keyslot_package));
	kp.rpmb_keyblob_len = RPMBKEY_LENGTH + CAAM_PAD;
	strcpy(kp.magic, KEYPACK_MAGIC);
	if (hwcrypto_gen_blob((uint32_t)(ulong)rpmb_key, RPMBKEY_LENGTH,
				(uint32_t)(ulong)blob) != 0) {
		printf("ERROR - generate rpmb key blob error!\n");
		ret = -1;
		goto fail;
	} else
		printf("RPMB key blob generated!\n");

	memcpy(kp.rpmb_keyblob, blob, kp.rpmb_keyblob_len);

	/* Reset key after use */
	memset(rpmb_key, 0, RPMBKEY_LENGTH);
	memset(key, 0, RPMBKEY_LENGTH);

	/* Store the rpmb key blob to last block of boot1 partition. */
	if (mmc_switch_part(mmc, KEYSLOT_HWPARTITION_ID) != 0) {
		printf("ERROR - can't switch to boot1 partition! \n");
		ret = -1;
		goto fail;
	} else
		desc->hwpart = KEYSLOT_HWPARTITION_ID;
	if (blk_dwrite(desc, KEYSLOT_BLKS, 1, (void *)&kp) != 1) {
		printf("ERROR - failed to write rpmbkeyblob!");
		ret = -1;
		goto fail;
	}
	/* Set power-on write protection to boot1 partition. */
	if (mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL,
			EXT_CSD_BOOT_WP, BOOT1_PWR_WP)) {
		printf("ERROR - unable to set power-on write protection!\n");
		ret = -1;
		goto fail;
	}

fail:
	/* Return to original partition */
	if (desc->hwpart != original_part) {
		if (mmc_switch_part(mmc, original_part) != 0)
			return -1;
		desc->hwpart = original_part;
	}

	return ret;
}

int fastboot_set_rpmb_key(uint8_t *staged_buf, uint32_t key_size)
{

	if (memcmp(staged_buf, RPMB_KEY_MAGIC, strlen(RPMB_KEY_MAGIC))) {
		printf("ERROR - rpmb magic doesn't match!\n");
		return -1;
	}

	return do_rpmb_key_set(staged_buf + strlen(RPMB_KEY_MAGIC),
				RPMBKEY_LENGTH);
}

int fastboot_set_rpmb_random_key(void)
{
	ALLOC_CACHE_ALIGN_BUFFER(uint8_t, rpmb_key, RPMBKEY_LENGTH);

	if (hwcrypto_gen_rng((ulong)rpmb_key, RPMBKEY_LENGTH)) {
		printf("error - can't generate random key!\n");
		return -1;
	}

	return do_rpmb_key_set(rpmb_key, RPMBKEY_LENGTH);
}

int avb_set_public_key(uint8_t *staged_buffer, uint32_t size) {

	if ((staged_buffer == NULL) || (size <= 0)) {
		ERR("Error. Get null staged_buffer\n");
		return -1;
	}
	if (trusty_write_vbmeta_public_key(staged_buffer, size)) {
		ERR("Error. Failed to write vbmeta public key into secure storage\n");
		return -1;
	} else
		printf("Set vbmeta public key successfully!\n");

	return 0;
}

int fastboot_get_mppubk(uint8_t *staged_buffer, uint32_t *size) {

	if (!hab_is_enabled()) {
		ERR("Error. This command can only be used when hab is closed!!\n");
		return -1;
	}

	if ((staged_buffer == NULL) || (size == NULL)) {
		ERR("Error. Get null staged_buffer!\n");
		return -1;
	}
	if (trusty_get_mppubk(staged_buffer, size)) {
		ERR("Error. Failed to get mppubk!\n");
		return -1;
	}

	return 0;
}
#endif /* CONFIG_IMX_TRUSTY_OS && !defind(CONFIG_AVB_ATX) */
#endif /* CONFIG_SPL_BUILD */
