// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright 2021 NXP
 *
 */

#include <common.h>
#include <cpu_func.h>
#include <malloc.h>
#include <memalign.h>
#include <fsl_sec.h>
#include <linux/errno.h>
#include "jobdesc.h"
#include "desc.h"
#include "desc_constr.h"
#include "jr.h"

/**
 * aesecb_decrypt() - Decrypt data using AES-256-ECB
 * @key_mod:    - Key address
 * @key_len:    - Key length
 * @src:        - Source address (Encrypted key)
 * @dst:        - Destination address (decrypted key in black)
 * @len:        - Size of data to be decrypted
 *
 * Note: Start and end of the key, src and dst buffers have to be aligned to
 * the cache line size (ARCH_DMA_MINALIGN) for the CAAM operation to succeed.
 *
 * Returns zero on success, negative on error.
 */
int aesecb_decrypt(u8 *key, u32 key_len, u8 *src, u8 *dst, u32 len)
{
	int ret, size, i = 0;
	u32 *desc;
	u8 len_desc;

	if (!IS_ALIGNED((uintptr_t)key, ARCH_DMA_MINALIGN) ||
	    !IS_ALIGNED((uintptr_t)src, ARCH_DMA_MINALIGN) ||
	    !IS_ALIGNED((uintptr_t)dst, ARCH_DMA_MINALIGN)) {
		printf("%s: Address arguments are not aligned!\n", __func__);
		return -EINVAL;
	}

	debug("\nAES ECB decryption Operation\n");
	desc = malloc_cache_aligned(sizeof(int) * MAX_CAAM_DESCSIZE);
	if (!desc) {
		debug("Not enough memory for descriptor allocation\n");
		return -ENOMEM;
	}

	size = ALIGN(key_len, ARCH_DMA_MINALIGN);
	flush_dcache_range((unsigned long)key, (unsigned long)key + size);
	size = ALIGN(len, ARCH_DMA_MINALIGN);
	flush_dcache_range((unsigned long)src, (unsigned long)src + size);

	inline_cnstr_jobdesc_aes_ecb_decrypt(desc, key, key_len, src, dst, len);

	debug("Descriptor dump:\n");
	len_desc = desc_len(desc);
	for (i = 0; i < len_desc; i++)
		debug("Word[%d]: %08x\n", i, *(desc + i));

	size = ALIGN(sizeof(int) * MAX_CAAM_DESCSIZE, ARCH_DMA_MINALIGN);
	flush_dcache_range((unsigned long)desc, (unsigned long)desc + size);
	size = ALIGN(len, ARCH_DMA_MINALIGN);
	invalidate_dcache_range((unsigned long)dst, (unsigned long)dst + size);

	ret = run_descriptor_jr(desc);

	if (ret) {
		printf("%s: error: %d\n", __func__, ret);
	} else {
		invalidate_dcache_range((unsigned long)dst, (unsigned long)dst + size);
		debug("%s: success.\n", __func__);
	}

	free(desc);
	return ret;
}
