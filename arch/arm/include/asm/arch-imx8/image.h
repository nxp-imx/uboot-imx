/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2018 NXP
 */

#define IV_MAX_LEN			32
#define HASH_MAX_LEN			64

#define CONTAINER_HDR_ALIGNMENT 0x400
#define CONTAINER_HDR_EMMC_OFFSET 0
#define CONTAINER_HDR_MMCSD_OFFSET SZ_32K
#define CONTAINER_HDR_QSPI_OFFSET SZ_4K
#define CONTAINER_HDR_NAND_OFFSET SZ_128M

 struct container_hdr{
	 uint8_t version;
	 uint8_t length_lsb;
	 uint8_t length_msb;
	 uint8_t tag;
	 uint32_t flags;
	 uint16_t sw_version;
	 uint8_t fuse_version;
	 uint8_t num_images;
	 uint16_t sig_blk_offset;
	 uint16_t reserved;
 }__attribute__((packed));

 struct boot_img_t{
	 uint32_t offset;
	 uint32_t size;
	 uint64_t dst;
	 uint64_t entry;
	 uint32_t hab_flags;
	 uint32_t meta;
	 uint8_t hash[HASH_MAX_LEN];
	 uint8_t iv[IV_MAX_LEN];
 }__attribute__((packed));

 struct signature_block_hdr{
	 uint8_t version;
	 uint8_t length_lsb;
	 uint8_t length_msb;
	 uint8_t tag;
	 uint16_t srk_table_offset;
	 uint16_t cert_offset;
	 uint16_t blob_offset;
	 uint16_t signature_offset;
	 uint32_t reserved;
 }__attribute__((packed));

struct generate_key_blob_hdr {
	uint8_t version;
	uint8_t length_lsb;
	uint8_t length_msb;
	uint8_t tag;
	uint8_t flags;
	uint8_t size;
	uint8_t algorithm;
	uint8_t mode;
} __attribute__((packed));
