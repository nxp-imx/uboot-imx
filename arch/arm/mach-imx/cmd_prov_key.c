// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * @file - cmd_prov_key.c
 * @brief - NXP command support
 * Command for provisioning encrypted key as black blob,
 *
 * Copyright 2021 NXP
 *
 */

/*
 *Concepts:
 *
 *  - black key: secure encrypted key that can only be used by the CAAM HW
 *               module on the device generating this key.
 *  - black blob: black blob is an encapsulation of black data (key) that can
 *                only be decapsulated by the initiator device. The
 *                decapsulation will result in a new black data readable only
 *                by the CAAM HW.
 *
 *
 *Generation of the key black blob:
 *
 *     1) Compile the bootloader with configuration:
 *        CONFIG_IMX_HAB
 *        CONFIG_FSL_CAAM
 *        CONFIG_IMX_CAAM_MFG_PROT
 *        CONFIG_CMD_PROVISION_KEY
 *     2) Boot the bootloader on the board
 *     3) Bootloader will generate the MPPubK
 *     4) PKEK = hash(MPPUBK)
 *     5) Read the encrypted key from RAM
 *     6) Decrypt using PKEK
 *     7) Encapsulate the decrypted key in black blob
 *     8) Add the 20 bytes TAG to black blob
 *     9) Copy the black blob in a binary file.
 *        The file must have a size of 112 bytes (0x70 bytes).
 */

#include <common.h>
#include <cpu_func.h>
#include <command.h>
#include <malloc.h>
#include <memalign.h>
#include <asm/byteorder.h>
#include <linux/compiler.h>
#include <fsl_sec.h>
#include <hash.h>
#include <u-boot/sha256.h>
#include <asm/arch/clock.h>

/* Key modifier for CAAM blobs, used as a revision number */
static const char caam_key_modifier[16] = {
		'C', 'A', 'A', 'M', '_', 'K', 'E', 'Y',
		'_', 'T', 'Y', 'P', 'E', '_', 'V', '1',
};

/**
 * do_export_key_blob() - Handle the "export_key_blob" command-line command
 * @cmdtp:	Command data struct pointer
 * @flag:	Command flag
 * @argc:	Command-line argument count
 * @argv:	Array of command-line arguments
 *
 * Returns zero on success, CMD_RET_USAGE in case of misuse and negative
 * on error.
 */
static int do_export_key_blob(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	ulong src_addr, dst_addr;
	uint8_t *src_ptr, *dst_ptr;
	uint8_t *mppubk = NULL, *pkek = NULL, *black_key = NULL;
	size_t key_len = AES256_KEY_SZ, pkek_len = SHA256_SUM_LEN;
	size_t blob_len, blob_max_len;
	int size, ret = 0;

	ALLOC_CACHE_ALIGN_BUFFER(uint8_t, km_ptr, 16);

	if (argc != 3)
		return CMD_RET_USAGE;

	/* generate mppubk */
	mppubk = malloc_cache_aligned(FSL_CAAM_MP_PUBK_BYTES);
	if (!mppubk) {
		printf("Failed to allocate mem for mppubk\n");
		return -ENOMEM;
	}

	ret = gen_mppubk(mppubk);
	if (ret) {
		printf("Failed to generate MPPubK\n");
		goto free_m;
	}

	/* Derive PKEK = SHA256(MPPUBK) */
	pkek = malloc_cache_aligned(pkek_len);
	if (!pkek) {
		printf("Failed to allocate memory for pkek\n");
		ret = -ENOMEM;
		goto free_m;
	}

	ret = hash_block("sha256", mppubk, FSL_CAAM_MP_PUBK_BYTES, pkek, (int *)&pkek_len);
	if (ret)
		goto free_pkek;

	/* use pkek to decrypt src_addr which has enc key*/
	src_addr = simple_strtoul(argv[1], NULL, 16);
	src_ptr = (uint8_t *)(uintptr_t)src_addr;

	black_key = malloc_cache_aligned(key_len);
	if (!black_key) {
		printf("Failed to allocate memory for black_key\n");
		ret = -ENOMEM;
		goto free_pkek;
	}

	ret = aesecb_decrypt(pkek, pkek_len, src_ptr, black_key, key_len);
	if (ret)
		goto free_blk_key;

	/* create key black blob */
	dst_addr = simple_strtoul(argv[2], NULL, 16);
	dst_ptr = (uint8_t *)(uintptr_t)dst_addr;

	/* copy key modifier, must be same as used in kernel */
	memcpy(km_ptr, caam_key_modifier, 16);

	ret = blob_encap((uint8_t *)km_ptr, black_key, dst_ptr, key_len, 1);
	if (ret)
		goto free_blk_key;

	/* Tag the black blob so it can be passed to kernel */
	blob_len = BLOB_SIZE(key_len) + CCM_OVERHEAD;
	blob_max_len = MAX_BLOB_SIZE;
	ret = tag_black_obj(dst_ptr, blob_len, key_len, blob_max_len);
	if (ret)
		printf("Failed to tag black blob: %d\n", ret);

free_blk_key:
	free(black_key);
free_pkek:
	memset(pkek, 0, pkek_len);
	size = ALIGN(pkek_len, ARCH_DMA_MINALIGN);
	flush_dcache_range((unsigned long)pkek, (unsigned long)pkek + size);
	free(pkek);
free_m:
	memset(mppubk, 0, FSL_CAAM_MP_PUBK_BYTES);
	size = ALIGN(FSL_CAAM_MP_PUBK_BYTES, ARCH_DMA_MINALIGN);
	flush_dcache_range((unsigned long)mppubk, (unsigned long)mppubk + size);
	free(mppubk);

	return ret;
}

/***************************************************/

U_BOOT_CMD(
	export_key_blob, 3, 0, do_export_key_blob,
	"Provision encrypted key as black blob.",
	"src_addr dst_addr \n\n"
	" - src_addr: source addr which has encrypted key(32 byte) to provision.\n"
	"             must be 64 byte aligned.\n"
	" - dst_addr: destination addr which will have key black blob(112 byte).\n"
	"             must be 64 byte aligned.\n"
);
