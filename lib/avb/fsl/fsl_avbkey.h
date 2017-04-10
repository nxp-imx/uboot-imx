/*
 * Copyright (C) 2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef __FSL_AVBKEY_H__
#define __FSL_AVBKEY_H__


#define CAAM_PAD 48

#define AVB_PUBKY_FLAG 0xABAB
#define AVB_PUBKY_OFFSET 0x1000

#define AVB_RBIDX_FLAG 0xCDCD
#define AVB_RBIDX_START 0x2000
#define AVB_RBIDX_ALIGN 0x1000
#define AVB_RBIDX_LEN 0x08
#define AVB_RBIDX_INITVAL 0


#define AVB_KBLB_MAGIC "\0KBLB!"
#define AVB_KBLB_MAGIC_LEN 6


struct kblb_tag {
	uint32_t flag;
	uint32_t offset;
	uint32_t len;
};
typedef struct kblb_tag kblb_tag_t;

struct kblb_hdr {
	/* avbkey partition magic */
	char magic[AVB_KBLB_MAGIC_LEN];
	/* public key keyblb tag */
	kblb_tag_t pubk_tag;
	/* rollback index keyblb tag */
	kblb_tag_t rbk_tags[AVB_MAX_NUMBER_OF_ROLLBACK_INDEX_LOCATIONS];
};
typedef struct kblb_hdr kblb_hdr_t;

#endif
