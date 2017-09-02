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

#ifdef CONFIG_AVB_ATX
#define ATX_RBIDX_FLAG 0xEFEF
#define ATX_RBIDX_START 0x22000
#define ATX_RBIDX_ALIGN 0x1000
#define ATX_RBIDX_LEN 0x08
#define ATX_RBIDX_INITVAL 0
#endif

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
	/* vbmeta rollback index keyblb tag */
	kblb_tag_t rbk_tags[AVB_MAX_NUMBER_OF_ROLLBACK_INDEX_LOCATIONS];
#ifdef CONFIG_AVB_ATX
	/* Android Things key versions rollback index keyblb tag */
	kblb_tag_t atx_rbk_tags[AVB_MAX_NUMBER_OF_ROLLBACK_INDEX_LOCATIONS];
#endif
};
typedef struct kblb_hdr kblb_hdr_t;

#ifdef AVB_RPMB
#define RPMBKEY_LEN (32 + CAAM_PAD)
#define KEYPACK_MAGIC "!KS"

struct keyslot_package
{
    char magic[4];
    unsigned int rpmb_keyblob_len;
    unsigned char rpmb_keyblob[RPMBKEY_LEN];
};
#endif

#endif
