/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * sha256_base.h - core logic for SHA-256 implementations
 *
 * Copyright (C) 2015 Linaro Ltd <ard.biesheuvel@linaro.org>
 * Copyright 2022 NXP
 */

#ifndef _CRYPTO_SHA256_BASE_H
#define _CRYPTO_SHA256_BASE_H

#include <asm/unaligned.h>
#include <linux/string.h>
#include <compiler.h>
#include <crypto/sha2.h>

typedef void (sha256_block_fn)(struct sha256_state *sst, u8 const *src,
			       int blocks);

static inline void sha256_base_do_update(struct sha256_state *sctx,
					 const u8 *data,
					 unsigned int len,
					 sha256_block_fn *block_fn)
{
	unsigned int partial = sctx->count % SHA256_BLOCK_SIZE;

	sctx->count += len;

	if ((partial + len) >= SHA256_BLOCK_SIZE) {
		int blocks;

		if (partial) {
			int p = SHA256_BLOCK_SIZE - partial;

			memcpy(sctx->buf + partial, data, p);
			data += p;
			len -= p;

			block_fn(sctx, sctx->buf, 1);
		}

		blocks = len / SHA256_BLOCK_SIZE;
		len %= SHA256_BLOCK_SIZE;

		if (blocks) {
			block_fn(sctx, data, blocks);
			data += blocks * SHA256_BLOCK_SIZE;
		}
		partial = 0;
	}
	if (len)
		memcpy(sctx->buf + partial, data, len);
}

static inline void sha256_base_do_finalize(struct sha256_state *sctx,
					   sha256_block_fn *block_fn)
{
	const int bit_offset = SHA256_BLOCK_SIZE - sizeof(__be64);
	__be64 *bits = (__be64 *)(sctx->buf + bit_offset);
	unsigned int partial = sctx->count % SHA256_BLOCK_SIZE;

	sctx->buf[partial++] = 0x80;
	if (partial > bit_offset) {
		memset(sctx->buf + partial, 0x0, SHA256_BLOCK_SIZE - partial);
		partial = 0;

		block_fn(sctx, sctx->buf, 1);
	}

	memset(sctx->buf + partial, 0x0, bit_offset - partial);
	*bits = cpu_to_be64(sctx->count << 3);
	block_fn(sctx, sctx->buf, 1);
}

static inline void sha256_base_finish(struct sha256_state *sctx, u8 *out)
{
	unsigned int digest_size = SHA256_DIGEST_SIZE;
	__be32 *digest = (__be32 *)out;
	int i;

	for (i = 0; digest_size > 0; i++, digest_size -= sizeof(__be32))
		put_unaligned_be32(sctx->state[i], digest++);

	memset(sctx, 0x0, sizeof(*sctx));
}

#endif /* _CRYPTO_SHA256_BASE_H */
