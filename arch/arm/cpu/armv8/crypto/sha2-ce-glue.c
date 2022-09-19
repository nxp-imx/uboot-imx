// SPDX-License-Identifier: GPL-2.0-only
/*
 * sha2-ce-glue.c - SHA-256 using ARMv8 Crypto Extensions
 *
 * Copyright (C) 2014 - 2017 Linaro Ltd <ard.biesheuvel@linaro.org>
 * Copyright 2022 NXP
 */

#include <linux/kernel.h>
#include <linux/linkage.h>
#include <crypto/sha256_base.h>

struct sha256_ce_state {
	struct sha256_state	sst;
	u32			finalize;
};

extern const u32 sha256_ce_offsetof_count;
extern const u32 sha256_ce_offsetof_finalize;

asmlinkage int sha2_ce_transform(struct sha256_ce_state *sst, u8 const *src,
				 int blocks);

static void __sha2_ce_transform(struct sha256_state *sst, u8 const *src,
				int blocks)
{
	while (blocks) {
		int rem;

		rem = sha2_ce_transform(container_of(sst, struct sha256_ce_state,
						     sst), src, blocks);
		src += (blocks - rem) * SHA256_BLOCK_SIZE;
		blocks = rem;
	}
}

const u32 sha256_ce_offsetof_count = offsetof(struct sha256_ce_state,
					      sst.count);
const u32 sha256_ce_offsetof_finalize = offsetof(struct sha256_ce_state,
						 finalize);

static void sha256_ce_update(struct sha256_ce_state *sctx, const u8 *data,
			     unsigned int len)
{
	sctx->finalize = 0;
	sha256_base_do_update(&sctx->sst, data, len, __sha2_ce_transform);
}

static void sha256_ce_final(struct sha256_ce_state *sctx, u8 *out)
{
	sctx->finalize = 0;
	sha256_base_do_finalize(&sctx->sst, __sha2_ce_transform);
	sha256_base_finish(&sctx->sst, out);
}

/*
 * Output = SHA-256( input buffer ).
 */
void sha256_ce(const unsigned char *input, unsigned int ilen, unsigned char *output)
{
	struct sha256_ce_state sctx;

	sha256_init(&sctx.sst);
	sha256_ce_update(&sctx, input, ilen);
	sha256_ce_final(&sctx, output);
}
