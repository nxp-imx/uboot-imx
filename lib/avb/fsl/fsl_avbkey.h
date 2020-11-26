/*
 * Copyright (C) 2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef __FSL_AVBKEY_H__
#define __FSL_AVBKEY_H__

#include <mmc.h>

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

#if defined(CONFIG_AVB_ATX) && defined(CONFIG_DUAL_BOOTLOADER)
#define BL_RBINDEX_MAGIC "BL_RBINDEX" 
#define BL_RBINDEX_MAGIC_LEN 11
struct bl_rbindex_package {
	char magic[BL_RBINDEX_MAGIC_LEN];
	uint32_t rbindex;
};
#endif

#ifndef CONFIG_AVB_ATX
#define RPMB_KEY_MAGIC "RPMB"
#endif

#ifdef CONFIG_AVB_ATX
#define ATX_FUSE_BANK_NUM  4
#define ATX_FUSE_BANK_MASK 0xFFFF
#define ATX_HASH_LENGTH    14
#endif

#define RESULT_ERROR -1
#define RESULT_OK     0

struct kblb_tag {
	uint32_t flag;
	uint32_t offset;
	uint32_t len;
};
typedef struct kblb_tag kblb_tag_t;

struct kblb_hdr {
	/* avbkey partition magic */
	char magic[AVB_KBLB_MAGIC_LEN];
	/* Rollback index for bootloader is managed by SPL and
	 * will be stored in RPMB.
	 */
#if defined(CONFIG_DUAL_BOOTLOADER) && defined(CONFIG_SPL_BUILD)
	kblb_tag_t bootloader_rbk_tags;
#endif
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

#define RPMBKEY_LEN (32 + CAAM_PAD)
#define KEYPACK_MAGIC "!KS"
#define KEYPACK_PAD_LENGTH (512 - 4 * sizeof(char) - sizeof(unsigned int) - RPMBKEY_LEN * sizeof(unsigned char))

struct keyslot_package
{
    char magic[4];
    unsigned int rpmb_keyblob_len;
    unsigned char rpmb_keyblob[RPMBKEY_LEN];
    // padding keyslot_package to 1 block size
    unsigned char pad[KEYPACK_PAD_LENGTH];
};

int gen_rpmb_key(struct keyslot_package *kp);
int read_keyslot_package(struct keyslot_package* kp);
void fill_secure_keyslot_package(struct keyslot_package *kp);
int rpmb_init(void);
int rpmb_read(struct mmc *mmc, uint8_t *buffer,
		size_t num_bytes,int64_t offset);
int rpmb_write(struct mmc *mmc, uint8_t *buffer, size_t num_bytes,
		int64_t offset);

int check_rpmb_blob(struct mmc *mmc);
bool rpmbkey_is_set(void);
int fsl_fuse_write(const uint32_t *buffer, uint32_t length, uint32_t offset);
int fsl_fuse_read(uint32_t *buffer, uint32_t length, uint32_t offset);
int permanent_attributes_sha256_hash(unsigned char* output);
struct mmc *get_mmc(void);
#endif
