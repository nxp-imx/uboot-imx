/*
 * SPDX-License-Identifier:     GPL-2.0+
 *
 * Copyright 2025 NXP
 *
 */

#include <image.h>
#include <log.h>
#include <string.h>
#include <u-boot/sha256.h>
#include <u-boot/rsa.h>
#include <trusty/libtipc.h>
#include "fsl_gbl.h"

int verify_gbl_footer(struct gbl_footer *footer) {
	assert(footer);

	if (memcmp(footer->magic, GBLF_MAGIC, sizeof(GBLF_MAGIC))) {
		printf("Invalid gbl footer magic!\n");
		return -1;
	}

	if (footer->metadata_offset == 0 || \
	    footer->image_size ==0 || \
	    footer->image_size <= footer->metadata_offset) {
		printf("Invalid gbl footer data!\n");
		return -1;
	}

	return 0;
}

#ifdef CONFIG_IMX_TRUSTY_OS
extern int is_current_slot_successful(bool *success);
int verify_gbl_metadata(struct gbl_metadata *metadata,
			struct gbl_footer *footer) {
	int ret = 0;
	uint64_t stored_rbindex = 0;

	assert(metadata);
	assert(footer);

	/* Check the magic */
	if (memcmp(metadata->magic, GBL0_MAGIC, sizeof(GBL0_MAGIC))) {
		printf("Invalid gbl metadata magic!\n");
		return -1;
	}

	/* Verify rollback index */
	if (metadata->rollback_index_location >= ROLLBACK_INDEX_SLOT_MAX) {
		printf("Invalid gbl rollback index location!\n");
		return -1;
	}
	ret = trusty_read_rollback_index(metadata->rollback_index_location,
					 &stored_rbindex);
	if (ret != 0) {
		printf("Failed to read gbl rollback index!\n");
		return -1;
	}
	if (metadata->rollback_index < stored_rbindex) {
		printf("GBL rollback index rejected!\n");
		return -1;
	}

	/* Update the stored rollback index if applied */
	if (metadata->rollback_index > stored_rbindex) {
		bool success_boot = false;
		if (is_current_slot_successful(&success_boot) < 0) {
			return -1;
		}
		/* Update the rollback index when current
		 * slot is successfully booted.
		 */
		if (success_boot) {
			ret = trusty_write_rollback_index(metadata->rollback_index_location,
							  metadata->rollback_index);
			if (ret != 0) {
				printf("Failed to write gbl rollback index!\n");
				return -1;
			}
		}
	}

	return 0;
}

int verify_gbl_signature(uint8_t *gbl, struct gbl_footer *footer) {
	uint8_t *signature = NULL;
	uint8_t hash[SHA256_SUM_LEN];
	uint8_t public_key_buf[2048];
	uint32_t public_key_sz = 0;
	struct image_sign_info info;
	char algo[64];
	int ret = 0;

	assert(gbl);
	assert(footer);

	/* Calculate the hash of signed data */
	sha256_csum_wd(gbl, footer->image_size - RSA4096_SIG_LEN, hash, CHUNKSZ_SHA256);

	/* Load GBL public key from secure storage */
	public_key_sz = sizeof(public_key_buf);
	if (trusty_read_gbl_public_key(public_key_buf,
				       &public_key_sz) != 0) {
		printf("Failed to read gbl public key!\n");
		return -1;
	}

	/* Verify the signature, only support SHA256_RSA4096 algorithm */
	memset(&info, '\0', sizeof(info));
	memset(algo, 0, sizeof(algo));

	info.padding = image_get_padding_algo("pkcs-1.5");
	memcpy(algo, "sha256,rsa4096", sizeof("sha256,rsa4096"));
	info.checksum = image_get_checksum_algo(algo);
	info.name = (const char *)algo;
	info.crypto = image_get_crypto_algo(info.name);
	if (!info.checksum || !info.crypto) {
		printf("<%s> not supported on image_get_(checksum|crypto)_algo()\n", algo);
		return -1;
	}

	info.key = public_key_buf;
	info.keylen = public_key_sz;
	signature = (uint8_t *)(gbl + footer->metadata_offset + sizeof(gbl_metadata));
	if (signature + RSA4096_SIG_LEN != gbl + footer->image_size) {
		printf("Wrong gbl signature size!\n");
		return -1;
	}

	ret = rsa_verify_with_pkey(&info, hash, signature, RSA4096_SIG_LEN);
	if (ret != 0) {
		printf("GBL signature verify failed! err: %d\n", ret);
		return -1;
	}

	return 0;
}

int verify_gbl(uint8_t *gbl, struct gbl_footer *footer) {
	gbl_metadata *metadata = NULL;

	assert(gbl);
	assert(footer);

	/* Verify the signature */
	if (verify_gbl_signature(gbl, footer))
		return -1;

	/* Verify the metadata */
	metadata = (gbl_metadata *)(gbl + footer->metadata_offset);
	if (verify_gbl_metadata(metadata, footer)) {
		return -1;
	}

	return 0;
}
#endif /* CONFIG_IMX_TRUSTY_OS */
