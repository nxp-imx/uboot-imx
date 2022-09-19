/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Common values for SHA-2 algorithms
 *
 * Copyright 2022 NXP
 */

#ifndef _CRYPTO_SHA2_H
#define _CRYPTO_SHA2_H

#include <linux/types.h>

#define SHA256_DIGEST_SIZE      32
#define SHA256_BLOCK_SIZE       64

#define SHA256_H0	0x6a09e667UL
#define SHA256_H1	0xbb67ae85UL
#define SHA256_H2	0x3c6ef372UL
#define SHA256_H3	0xa54ff53aUL
#define SHA256_H4	0x510e527fUL
#define SHA256_H5	0x9b05688cUL
#define SHA256_H6	0x1f83d9abUL
#define SHA256_H7	0x5be0cd19UL

struct sha256_state {
	uint32_t state[SHA256_DIGEST_SIZE / 4];
	uint64_t count;
	uint8_t buf[SHA256_BLOCK_SIZE];
};

/*
 * Stand-alone implementation of the SHA256 algorithm.
 */

static inline void sha256_init(struct sha256_state *sctx)
{
	sctx->state[0] = SHA256_H0;
	sctx->state[1] = SHA256_H1;
	sctx->state[2] = SHA256_H2;
	sctx->state[3] = SHA256_H3;
	sctx->state[4] = SHA256_H4;
	sctx->state[5] = SHA256_H5;
	sctx->state[6] = SHA256_H6;
	sctx->state[7] = SHA256_H7;
	sctx->count = 0;
}

void sha256_ce(const unsigned char *data, unsigned int ilen, unsigned char *output);

#endif /* _CRYPTO_SHA2_H */
