/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2014 Freescale Semiconductor, Inc.
 * Copyright 2021 NXP
 *
 */

#ifndef __JOBDESC_H
#define __JOBDESC_H

#include <common.h>
#include <asm/io.h>
#include "rsa_caam.h"

#define KEY_IDNFR_SZ_BYTES		16

/* Encrypted key */
#define BLACK_KEY	1

#ifdef CONFIG_CMD_DEKBLOB
/* inline_cnstr_jobdesc_blob_dek:
 * Intializes and constructs the job descriptor for DEK encapsulation
 * using the given parameters.
 * @desc: reference to the job descriptor
 * @plain_txt: reference to the DEK
 * @enc_blob: reference where to store the blob
 * @in_sz: size in bytes of the DEK
 * @return: 0 on success, ECONSTRJDESC otherwise
 */
int inline_cnstr_jobdesc_blob_dek(uint32_t *desc, const uint8_t *plain_txt,
				uint8_t *enc_blob, uint32_t in_sz);
#endif

void inline_cnstr_jobdesc_hash(uint32_t *desc,
			  const uint8_t *msg, uint32_t msgsz, uint8_t *digest,
			  u32 alg_type, uint32_t alg_size, int sg_tbl);

void inline_cnstr_jobdesc_blob_encap(uint32_t *desc, uint8_t *key_idnfr,
				     uint8_t *plain_txt, uint8_t *enc_blob,
				     uint32_t in_sz, uint8_t keycolor);

void inline_cnstr_jobdesc_blob_decap(uint32_t *desc, uint8_t *key_idnfr,
				     uint8_t *enc_blob, uint8_t *plain_txt,
				     uint32_t out_sz, uint8_t keycolor);

void inline_cnstr_jobdesc_rng_instantiation(u32 *desc, int handle, int do_sk);

void inline_cnstr_jobdesc_rng_deinstantiation(u32 *desc, int handle);

void inline_cnstr_jobdesc_rng(u32 *desc, void *data_out, u32 size);

void inline_cnstr_jobdesc_pkha_rsaexp(uint32_t *desc,
				      struct pk_in_params *pkin, uint8_t *out,
				      uint32_t out_siz);

void inline_cnstr_jobdesc_derive_bkek(uint32_t *desc, void *bkek_out,
				      void *key_mod, uint32_t key_sz);

void inline_cnstr_jobdesc_aes_ecb_decrypt(uint32_t *desc, uint8_t *key,
					  uint32_t key_len, uint8_t *src,
					  uint8_t *dst, uint32_t len);
#endif
