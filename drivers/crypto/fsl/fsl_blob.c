// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2014 Freescale Semiconductor, Inc.
 *
 */

#include <common.h>
#include <cpu_func.h>
#include <log.h>
#include <malloc.h>
#include <memalign.h>
#include <fsl_sec.h>
#include <asm/cache.h>
#include <linux/errno.h>
#include "jobdesc.h"
#include "desc.h"
#include "jr.h"

/**
 * blob_decap() - Decapsulate the data from a blob
 * @key_mod:    - Key modifier address
 * @src:        - Source address (blob)
 * @dst:        - Destination address (data)
 * @len:        - Size of decapsulated data
 * @keycolor    - Determines if the source data is covered (black key) or
 *                plaintext.
 *
 * Note: Start and end of the key_mod, src and dst buffers have to be aligned to
 * the cache line size (ARCH_DMA_MINALIGN) for the CAAM operation to succeed.
 *
 * Returns zero on success, negative on error.
 */
int blob_decap(u8 *key_mod, u8 *src, u8 *dst, u32 len, u8 keycolor)
{
	int ret, size, i = 0;
	u32 *desc;

	if (!IS_ALIGNED((uintptr_t)key_mod, ARCH_DMA_MINALIGN) ||
	    !IS_ALIGNED((uintptr_t)src, ARCH_DMA_MINALIGN) ||
	    !IS_ALIGNED((uintptr_t)dst, ARCH_DMA_MINALIGN)) {
		puts("Error: blob_decap: Address arguments are not aligned!\n");
		return -EINVAL;
	}

	debug("\nDecapsulating blob to get data\n");
	desc = malloc_cache_aligned(sizeof(int) * MAX_CAAM_DESCSIZE);
	if (!desc) {
		debug("Not enough memory for descriptor allocation\n");
		return -ENOMEM;
	}

	size = ALIGN(16, ARCH_DMA_MINALIGN);
	flush_dcache_range((unsigned long)key_mod,
			   (unsigned long)key_mod + size);

	size = ALIGN(BLOB_SIZE(len), ARCH_DMA_MINALIGN);
	flush_dcache_range((unsigned long)src,
			   (unsigned long)src + size);

	inline_cnstr_jobdesc_blob_decap(desc, key_mod, src, dst, len, keycolor);

	debug("Descriptor dump:\n");
	for (i = 0; i < 14; i++)
		debug("Word[%d]: %08x\n", i, *(desc + i));

	size = ALIGN(sizeof(int) * MAX_CAAM_DESCSIZE, ARCH_DMA_MINALIGN);
	flush_dcache_range((unsigned long)desc,
			   (unsigned long)desc + size);

	size = ALIGN(len, ARCH_DMA_MINALIGN);
	invalidate_dcache_range((unsigned long)dst,
				(unsigned long)dst + size);

	ret = run_descriptor_jr(desc);

	if (ret) {
		/* clear the blob data output buffer */
		memset(dst, 0x00, len);
		size = ALIGN(len, ARCH_DMA_MINALIGN);
		flush_dcache_range((unsigned long)dst, (unsigned long)dst + size);
		printf("Error in blob decapsulation: %d\n", ret);
	} else {
		size = ALIGN(len, ARCH_DMA_MINALIGN);
		invalidate_dcache_range((unsigned long)dst,
					(unsigned long)dst + size);

		debug("Blob decapsulation successful.\n");
	}

	free(desc);
	return ret;
}

/**
 * blob_encap() - Encapsulate the data as a blob
 * @key_mod:    - Key modifier address
 * @src:        - Source address (data)
 * @dst:        - Destination address (blob)
 * @len:        - Size of data to be encapsulated
 * @keycolor    - Determines if the source data is covered (black key) or
 *                plaintext.
 *
 * Note: Start and end of the key_mod, src and dst buffers have to be aligned to
 * the cache line size (ARCH_DMA_MINALIGN) for the CAAM operation to succeed.
 *
 * Returns zero on success, negative on error.
 */
int blob_encap(u8 *key_mod, u8 *src, u8 *dst, u32 len, u8 keycolor)
{
	int ret, size, i = 0;
	u32 *desc;

	if (!IS_ALIGNED((uintptr_t)key_mod, ARCH_DMA_MINALIGN) ||
	    !IS_ALIGNED((uintptr_t)src, ARCH_DMA_MINALIGN) ||
	    !IS_ALIGNED((uintptr_t)dst, ARCH_DMA_MINALIGN)) {
		puts("Error: blob_encap: Address arguments are not aligned!\n");
		return -EINVAL;
	}

	debug("\nEncapsulating data to form blob\n");
	desc = malloc_cache_aligned(sizeof(int) * MAX_CAAM_DESCSIZE);
	if (!desc) {
		debug("Not enough memory for descriptor allocation\n");
		return -ENOMEM;
	}

	size = ALIGN(16, ARCH_DMA_MINALIGN);
	flush_dcache_range((unsigned long)key_mod,
			   (unsigned long)key_mod + size);

	size = ALIGN(len, ARCH_DMA_MINALIGN);
	flush_dcache_range((unsigned long)src,
			   (unsigned long)src + size);

	inline_cnstr_jobdesc_blob_encap(desc, key_mod, src, dst, len, keycolor);

	debug("Descriptor dump:\n");
	for (i = 0; i < 14; i++)
		debug("Word[%d]: %08x\n", i, *(desc + i));

	size = ALIGN(sizeof(int) * MAX_CAAM_DESCSIZE, ARCH_DMA_MINALIGN);
	flush_dcache_range((unsigned long)desc,
			   (unsigned long)desc + size);

	size = ALIGN(BLOB_SIZE(len), ARCH_DMA_MINALIGN);
	invalidate_dcache_range((unsigned long)dst,
				(unsigned long)dst + size);

	ret = run_descriptor_jr(desc);

	if (ret) {
		printf("Error in blob encapsulation: %d\n", ret);
	} else {
		size = ALIGN(BLOB_SIZE(len), ARCH_DMA_MINALIGN);
		invalidate_dcache_range((unsigned long)dst,
					(unsigned long)dst + size);

		debug("Blob encapsulation successful.\n");
	}

	free(desc);
	return ret;
}

int derive_blob_kek(u8 *bkek_buf, u8 *key_mod, u32 key_sz)
{
	int ret, size;
	u32 *desc;

	if (!IS_ALIGNED((uintptr_t)bkek_buf, ARCH_DMA_MINALIGN) ||
	    !IS_ALIGNED((uintptr_t)key_mod, ARCH_DMA_MINALIGN)) {
		puts("Error: derive_bkek: Address arguments are not aligned!\n");
		return -EINVAL;
	}

	debug("\nBlob key encryption key(bkek)\n");
	desc = malloc_cache_aligned(sizeof(int) * MAX_CAAM_DESCSIZE);
	if (!desc) {
		printf("Not enough memory for descriptor allocation\n");
		return -ENOMEM;
	}

	size = ALIGN(key_sz, ARCH_DMA_MINALIGN);
	flush_dcache_range((unsigned long)key_mod, (unsigned long)key_mod + size);

	/* construct blob key encryption key(bkek) derive descriptor */
	inline_cnstr_jobdesc_derive_bkek(desc, bkek_buf, key_mod, key_sz);

	size = ALIGN(sizeof(int) * MAX_CAAM_DESCSIZE, ARCH_DMA_MINALIGN);
	flush_dcache_range((unsigned long)desc, (unsigned long)desc + size);
	size = ALIGN(BKEK_SIZE, ARCH_DMA_MINALIGN);
	flush_dcache_range((unsigned long)bkek_buf,
			   (unsigned long)bkek_buf + size);

	/* run descriptor */
	ret = run_descriptor_jr(desc);
	if (ret < 0) {
		printf("Error: derive_blob_kek failed 0x%x\n", ret);
	} else {
		invalidate_dcache_range((unsigned long)bkek_buf,
					(unsigned long)bkek_buf + size);
		debug("derive bkek successful.\n");
	}

	free(desc);
	return ret;
}

int hwrng_generate(u8 *dst, u32 len)
{
	int ret, size;
	u32 *desc;

	if (!IS_ALIGNED((uintptr_t)dst, ARCH_DMA_MINALIGN)) {
		puts("Error: caam_hwrng: Address arguments are not aligned!\n");
		return -EINVAL;
	}

	debug("\nRNG generate\n");
	desc = malloc_cache_aligned(sizeof(int) * MAX_CAAM_DESCSIZE);
	if (!desc) {
		printf("Not enough memory for descriptor allocation\n");
		return -ENOMEM;
	}

	inline_cnstr_jobdesc_rng(desc, dst ,len);

	size = ALIGN(sizeof(int) * MAX_CAAM_DESCSIZE, ARCH_DMA_MINALIGN);
	flush_dcache_range((unsigned long)desc, (unsigned long)desc + size);
	size = ALIGN(len, ARCH_DMA_MINALIGN);
	flush_dcache_range((unsigned long)dst, (unsigned long)dst + size);

	ret = run_descriptor_jr(desc);
	if (ret < 0) {
		printf("Error: RNG generate failed 0x%x\n", ret);
	} else {
		invalidate_dcache_range((unsigned long)dst,
					(unsigned long)dst + size);
		debug("RNG generation successful.\n");
	}

	free(desc);
	return ret;
}

#ifdef CONFIG_CMD_DEKBLOB
int blob_dek(const u8 *src, u8 *dst, u8 len)
{
	int ret, size, i = 0;
	u32 *desc;

	int out_sz =  WRP_HDR_SIZE + len + KEY_BLOB_SIZE + MAC_SIZE;

	puts("\nEncapsulating provided DEK to form blob\n");
	desc = memalign(ARCH_DMA_MINALIGN,
			sizeof(uint32_t) * DEK_BLOB_DESCSIZE);
	if (!desc) {
		debug("Not enough memory for descriptor allocation\n");
		return -ENOMEM;
	}

	ret = inline_cnstr_jobdesc_blob_dek(desc, src, dst, len);
	if (ret) {
		debug("Error in Job Descriptor Construction:  %d\n", ret);
	} else {
		size = roundup(sizeof(uint32_t) * DEK_BLOB_DESCSIZE,
			      ARCH_DMA_MINALIGN);
		flush_dcache_range((unsigned long)desc,
				   (unsigned long)desc + size);
		size = roundup(sizeof(uint8_t) * out_sz, ARCH_DMA_MINALIGN);
		flush_dcache_range((unsigned long)dst,
				   (unsigned long)dst + size);

		ret = run_descriptor_jr(desc);
	}

	if (ret) {
		debug("Error in Encapsulation %d\n", ret);
	   goto err;
	}

	size = roundup(out_sz, ARCH_DMA_MINALIGN);
	invalidate_dcache_range((unsigned long)dst, (unsigned long)dst+size);

	puts("DEK Blob\n");
	for (i = 0; i < out_sz; i++)
		printf("%02X", ((uint8_t *)dst)[i]);
	printf("\n");

err:
	free(desc);
	return ret;
}
#endif
