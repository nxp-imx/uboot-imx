/*
 * This is from the Android Project,
 * Repository: https://android.googlesource.com/platform/bootable/bootloader/legacy
 * File: include/boot/bootimg.h
 * Commit: 4205b865141ff2e255fe1d3bd16de18e217ef06a
 *
 * Copyright (C) 2008 The Android Open Source Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef _ANDROID_IMAGE_H_
#define _ANDROID_IMAGE_H_

#define ANDR_BOOT_MAGIC "ANDROID!"
#define ANDR_BOOT_MAGIC_SIZE 8
#define ANDR_BOOT_NAME_SIZE 16
#define ANDR_BOOT_ARGS_SIZE 512
#define ANDR_BOOT_EXTRA_ARGS_SIZE 1024

/* Boot metric variables (in millisecond) */
struct boot_metric
{
	u32 bll_1;	/* 1th bootloader load duration */
	u32 ble_1;	/* 1th bootloader exec duration */
	u32 kl;		/* kernel image load duration */
	u32 kd;		/* kernel image decompress duration */
	u32 avb;	/* avb verify boot.img duration */
	u32 odt;	/* overlay device tree duration */
	u32 sw;		/* system wait for UI interaction duration*/
};
typedef struct boot_metric boot_metric;

struct andr_img_hdr {
	char magic[ANDR_BOOT_MAGIC_SIZE];

	u32 kernel_size;	/* size in bytes */
	u32 kernel_addr;	/* physical load addr */

	u32 ramdisk_size;	/* size in bytes */
	u32 ramdisk_addr;	/* physical load addr */

	u32 second_size;	/* size in bytes */
	u32 second_addr;	/* physical load addr */

	u32 tags_addr;		/* physical addr for kernel tags */
	u32 page_size;		/* flash page size we assume */
	u32 unused;		/* reserved for future expansion: MUST be 0 */

	/* operating system version and security patch level; for
	 * version "A.B.C" and patch level "Y-M-D":
	 * ver = A << 14 | B << 7 | C         (7 bits for each of A, B, C)
	 * lvl = ((Y - 2000) & 127) << 4 | M  (7 bits for Y, 4 bits for M)
	 * os_version = ver << 11 | lvl */
	u32 os_version;

	char name[ANDR_BOOT_NAME_SIZE]; /* asciiz product name */

	char cmdline[ANDR_BOOT_ARGS_SIZE];

	u32 id[8]; /* timestamp / checksum / sha1 / etc */

	/* Supplemental command line data; kept here to maintain
	 * binary compatibility with older versions of mkbootimg */
	char extra_cmdline[ANDR_BOOT_EXTRA_ARGS_SIZE];
} __attribute__((packed));

/*
 * +-----------------+
 * | boot header     | 1 page
 * +-----------------+
 * | kernel          | n pages
 * +-----------------+
 * | ramdisk         | m pages
 * +-----------------+
 * | second stage    | o pages
 * +-----------------+
 *
 * n = (kernel_size + page_size - 1) / page_size
 * m = (ramdisk_size + page_size - 1) / page_size
 * o = (second_size + page_size - 1) / page_size
 *
 * 0. all entities are page_size aligned in flash
 * 1. kernel and ramdisk are required (size != 0)
 * 2. second is optional (second_size == 0 -> no second)
 * 3. load each element (kernel, ramdisk, second) at
 *    the specified physical address (kernel_addr, etc)
 * 4. prepare tags at tag_addr.  kernel_args[] is
 *    appended to the kernel commandline in the tags.
 * 5. r0 = 0, r1 = MACHINE_TYPE, r2 = tags_addr
 * 6. if second_size != 0: jump to second_addr
 *    else: jump to kernel_addr
 */
struct header_image {
	uint32_t	code0;		/* Executable code */
	uint32_t	code1;		/* Executable code */
	uint64_t	text_offset;	/* Image load offset, LE */
	uint64_t	image_size;	/* Effective Image size, LE */
	uint64_t	res1;		/* reserved */
	uint64_t	res2;		/* reserved */
	uint64_t	res3;		/* reserved */
	uint64_t	res4;		/* reserved */
	uint32_t	magic;		/* Magic number */
	uint32_t	res5;
};
#endif
