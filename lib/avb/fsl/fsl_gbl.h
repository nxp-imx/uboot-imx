/*
 * SPDX-License-Identifier:     GPL-2.0+
 *
 * Copyright 2025 NXP
 *
 */

#ifndef __FSL_GBL_H__
#define __FSL_GBL_H__

// Check gbl_signtool.py for the signed GBL image layout

/* Magic values for validation */
#define GBL0_MAGIC "GBL0\0"
#define GBLF_MAGIC "GBLf\0"

/* Fixed sizes and offsets */
#define RSA4096_SIG_LEN 512
#define FOOTER_OFFSET_FROM_END 512
#define ROLLBACK_INDEX_SLOT_MAX (32)

typedef struct gbl_metadata {
	/* GBL0_MAGIC */
	char magic[8];
	/* Size of original GBL image */
	uint32_t original_gbl_size;
	/* Rollback index location */
	uint32_t rollback_index_location;
	/* Rollback index of the GBL image */
	uint32_t rollback_index;
} gbl_metadata;

typedef struct gbl_footer {
	/* GBLF_MAGIC */
	char magic[8];
	/* Offset of the metadata struct */
	uint32_t metadata_offset;
	/* Size of signed GBL image including the signature */
	uint32_t image_size;
} gbl_footer;

int verify_gbl_footer(struct gbl_footer *footer);
int verify_gbl_metadata(struct gbl_metadata *metadata,
			struct gbl_footer *footer);
int verify_gbl_signature(uint8_t *gbl, struct gbl_footer *footer);
int verify_gbl(uint8_t *gbl, struct gbl_footer *footer);
#endif
