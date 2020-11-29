/*
 * Copyright (C) 2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:     GPL-2.0+
 *
 * Copyright 2020 NXP
 *
 */

#include <common.h>
#include <stdlib.h>
#include <linux/string.h>
#include <mmc.h>
#include <spl.h>
#include <part.h>
#include "utils.h"
#include <fb_fsl.h>
#include <fsl_avb.h>
#include <image.h>
#include <hang.h>
#include "fsl_caam.h"
#include "fsl_avbkey.h"
#include "hang.h"
#include "fsl_bootctrl.h"

/* Maximum values for slot data */
#define AVB_AB_MAX_PRIORITY 15
#define AVB_AB_MAX_TRIES_REMAINING 7
#define AVB_AB_SLOT_NUM 2
#ifndef MAX_PTN
#define MAX_PTN 32
#endif

/* The bootloader_control struct is stored 2048 bytes into the 'misc' partition
 * following the 'struct bootloader_message' field. The struct is compatible with
 * the guidelines in
 * hardware/interfaces/boot/1.1/default/boot_control/include/libboot_control/libboot_control.h
 */
#define FSL_AB_METADATA_MISC_PARTITION_OFFSET 2048
extern AvbABOps fsl_avb_ab_ops;

static char *slot_suffix[AVB_AB_SLOT_NUM] = {"_a", "_b"};

static int strcmp_l1(const char *s1, const char *s2) {
	if (!s1 || !s2)
		return -1;
	return strncmp(s1, s2, strlen(s1));
}

int get_curr_slot(struct bootloader_control *ab_data) {
	if (fsl_slot_is_bootable(&ab_data->slot_info[0]) &&
		fsl_slot_is_bootable(&ab_data->slot_info[1])) {
		if (ab_data->slot_info[1].priority > ab_data->slot_info[0].priority)
			return 1;
		else
			return 0;
	} else if (fsl_slot_is_bootable(&ab_data->slot_info[0]))
		return 0;
	else if (fsl_slot_is_bootable(&ab_data->slot_info[1]))
		return 1;
	else
		return -1;
}

/* Return current slot without passing 'bootloader_control' struct */
int current_slot(void) {
	struct bootloader_control ab_data;

	/* Load A/B metadata and decide which slot we are going to load */
	if (fsl_avb_ab_ops.read_ab_metadata(&fsl_avb_ab_ops, &ab_data) !=
					    AVB_IO_RESULT_OK) {
		printf("Error loading AB metadata from misc!\n");
		return -1;
	}
	return get_curr_slot(&ab_data);
}

int slotidx_from_suffix(char *suffix) {
	int slot = -1;

	if (!strcmp(suffix, "_a") ||
			!strcmp(suffix, "a"))
		slot = 0;
	else if (!strcmp(suffix, "_b") ||
			!strcmp(suffix, "b"))
		slot = 1;

	return slot;
}

bool is_slotvar_avb(char *cmd) {

	assert(cmd != NULL);
	if (!strcmp_l1("has-slot:", cmd) ||
		!strcmp_l1("slot-successful:", cmd) ||
		!strcmp_l1("slot-count", cmd) ||
		!strcmp_l1("slot-suffixes", cmd) ||
		!strcmp_l1("current-slot", cmd) ||
		!strcmp_l1("slot-unbootable:", cmd) ||
		!strcmp_l1("slot-retry-count:", cmd))
		return true;
	return false;
}

extern struct fastboot_ptentry g_ptable[MAX_PTN];
extern unsigned int g_pcount;

static bool has_slot(char *cmd) {
	unsigned int n;
	char *ptr;

	for (n = 0; n < g_pcount; n++) {
		ptr = strstr(g_ptable[n].name, cmd);
		if (ptr != NULL) {
			ptr += strlen(cmd);
			if (!strcmp(ptr, "_a") || !strcmp(ptr, "_b"))
				return true;
		}
	}
	return false;
}

int get_slotvar_avb(AvbABOps *ab_ops, char *cmd, char *buffer, size_t size) {

	struct bootloader_control ab_data;
	struct slot_metadata *slot_data;
	int slot;

	if ((ab_ops == NULL) || (cmd == NULL) || (buffer == NULL))
		return -1;

	char *str = cmd;
	if (!strcmp_l1("has-slot:", cmd)) {
		str += strlen("has-slot:");
		if (has_slot(str))
			strlcpy(buffer, "yes", size);
		else
			strlcpy(buffer, "no", size);
		return 0;

	} else if (!strcmp_l1("slot-suffixes", cmd)) {
		strlcpy(buffer, "_a,_b", size);
		return 0 ;

	} else if (!strcmp_l1("slot-count", cmd)) {
		strlcpy(buffer, "2", size);
		return 0 ;
	}

	/* load ab meta */
	if (ab_ops->read_ab_metadata == NULL ||
			ab_ops->read_ab_metadata(ab_ops, &ab_data) != AVB_IO_RESULT_OK) {
		strlcpy(buffer, "ab data read error", size);
		return -1 ;
	}

	if (!strcmp_l1("current-slot", cmd)) {
		int curr = get_curr_slot(&ab_data);
		if (curr >= 0 && curr < AVB_AB_SLOT_NUM)
			strlcpy(buffer, slot_suffix[curr] + sizeof(unsigned char), size);
		else {
			strlcpy(buffer, "no bootable slot", size);
			return -1;
		}

	} else if (!strcmp_l1("slot-successful:", cmd)) {
		str += strlen("slot-successful:");
		slot = slotidx_from_suffix(str);
		if (slot < 0) {
			strlcpy(buffer, "no such slot", size);
			return -1;
		} else {
			slot_data = &ab_data.slot_info[slot];
			bool succ = (slot_data->successful_boot != 0);
			strlcpy(buffer, succ ? "yes" : "no", size);
		}

	} else if (!strcmp_l1("slot-unbootable:", cmd)) {
		str += strlen("slot-unbootable:");
		slot = slotidx_from_suffix(str);
		if (slot < 0) {
			strlcpy(buffer, "no such slot", size);
			return -1;
		} else {
			slot_data = &ab_data.slot_info[slot];
			bool bootable = fsl_slot_is_bootable(slot_data);
			strlcpy(buffer, bootable ? "no" : "yes", size);
		}

	} else if (!strcmp_l1("slot-retry-count:", cmd)) {
		str += strlen("slot-retry-count:");
		slot = slotidx_from_suffix(str);
		if (slot < 0) {
			strlcpy(buffer, "no such slot", size);
			return -1;
		}
		else {
			slot_data = &ab_data.slot_info[slot];
			char var[7];
			sprintf(var, "%d",
				slot_data->tries_remaining);
			strlcpy(buffer, var, size);
		}

	} else {
		strlcpy(buffer, "no such slot command", size);
		return -1;
	}

	return 0;
}

char *select_slot(AvbABOps *ab_ops) {
	struct bootloader_control ab_data;
	int curr;

	if (ab_ops == NULL) {
		return NULL;
	}

	/* load ab meta */
	if (ab_ops->read_ab_metadata == NULL ||
			ab_ops->read_ab_metadata(ab_ops, &ab_data) != AVB_IO_RESULT_OK) {
		return NULL;
	}
	curr = get_curr_slot(&ab_data);
	if (curr >= 0 && curr < AVB_AB_SLOT_NUM)
		return slot_suffix[curr];
	else
		return NULL;
}

bool fsl_avb_ab_data_verify_and_byteswap(const struct bootloader_control* src,
					 struct bootloader_control* dest) {
	/* Ensure magic is correct. */
	if (src->magic != BOOT_CTRL_MAGIC) {
		printf("Magic is incorrect.\n");
		return false;
	}

	memcpy(dest, src, sizeof(struct bootloader_control));

	/* Ensure we don't attempt to access any fields if the bootctrl version
	* is not supported.
	*/
	if (dest->version > BOOT_CTRL_VERSION) {
		printf("No support for given bootctrl version.\n");
		return false;
	}

	/* Fail if CRC32 doesn't match. */
	if (dest->crc32_le !=
		avb_crc32((const uint8_t*)dest, sizeof(struct bootloader_control) - sizeof(uint32_t))) {
		printf("CRC32 does not match.\n");
		return false;
	}

	return true;
}

void fsl_avb_ab_data_update_crc_and_byteswap(const struct bootloader_control* src,
					     struct bootloader_control* dest) {
	memcpy(dest, src, sizeof(struct bootloader_control));
	dest->crc32_le = avb_crc32((const uint8_t*)dest,
				    sizeof(struct bootloader_control) - sizeof(uint32_t));
}

void fsl_avb_ab_data_init(struct bootloader_control* data) {
	memset(data, '\0', sizeof(struct bootloader_control));
	data->magic = BOOT_CTRL_MAGIC;
	data->version = BOOT_CTRL_VERSION;
	// this bootctrl can support up to 4 slots but here we only support 2
	data->nb_slot = AVB_AB_SLOT_NUM;
	data->slot_info[0].priority = AVB_AB_MAX_PRIORITY;
	data->slot_info[0].tries_remaining = AVB_AB_MAX_TRIES_REMAINING;
	data->slot_info[0].successful_boot = 0;
	data->slot_info[0].verity_corrupted = 0;
#ifdef CONFIG_DUAL_BOOTLOADER
	data->slot_info[0].bootloader_verified = 0;
#endif
	data->slot_info[1].priority = AVB_AB_MAX_PRIORITY;
	data->slot_info[1].tries_remaining = AVB_AB_MAX_TRIES_REMAINING;
	data->slot_info[1].successful_boot = 0;
	data->slot_info[1].verity_corrupted = 0;
#ifdef CONFIG_DUAL_BOOTLOADER
	data->slot_info[1].bootloader_verified = 0;
#endif
}

AvbIOResult fsl_avb_ab_data_read(AvbABOps* ab_ops, struct bootloader_control* data) {
	AvbOps* ops = ab_ops->ops;
	struct bootloader_control serialized;
	AvbIOResult io_ret;
	size_t num_bytes_read;

	io_ret = ops->read_from_partition(ops,
					  FASTBOOT_PARTITION_MISC,
					  FSL_AB_METADATA_MISC_PARTITION_OFFSET,
					  sizeof(struct bootloader_control),
					  &serialized,
					  &num_bytes_read);
	if (io_ret == AVB_IO_RESULT_ERROR_OOM) {
		return AVB_IO_RESULT_ERROR_OOM;
	} else if (io_ret != AVB_IO_RESULT_OK ||
		num_bytes_read != sizeof(struct bootloader_control)) {
		printf("Error reading A/B metadata.\n");
		return AVB_IO_RESULT_ERROR_IO;
	}

	if (!fsl_avb_ab_data_verify_and_byteswap(&serialized, data)) {
		printf(
			"Error validating A/B metadata from disk. "
			"Resetting and writing new A/B metadata to disk.\n");
		fsl_avb_ab_data_init(data);
		return fsl_avb_ab_data_write(ab_ops, data);
	}

	return AVB_IO_RESULT_OK;
}

AvbIOResult fsl_avb_ab_data_write(AvbABOps* ab_ops, const struct bootloader_control* data) {
	AvbOps* ops = ab_ops->ops;
	struct bootloader_control serialized;
	AvbIOResult io_ret;

	fsl_avb_ab_data_update_crc_and_byteswap(data, &serialized);
	io_ret = ops->write_to_partition(ops,
					 FASTBOOT_PARTITION_MISC,
					 FSL_AB_METADATA_MISC_PARTITION_OFFSET,
					 sizeof(struct bootloader_control),
					 &serialized);
	if (io_ret == AVB_IO_RESULT_ERROR_OOM) {
		return AVB_IO_RESULT_ERROR_OOM;
	} else if (io_ret != AVB_IO_RESULT_OK) {
		printf("Error writing A/B metadata.\n");
		return AVB_IO_RESULT_ERROR_IO;
	}
	return AVB_IO_RESULT_OK;
}

bool fsl_slot_is_bootable(struct slot_metadata* slot) {
#ifdef CONFIG_DUAL_BOOTLOADER
	/* The 'bootloader_verified' will be set when the slot has only one chance
	 * left, which means the slot is bootable even tries_remaining is 0.
	 */
	return slot->priority > 0 &&
		(slot->successful_boot || (slot->tries_remaining > 0)||
		(slot->bootloader_verified == 1));
#else
	return slot->priority > 0 &&
		(slot->successful_boot || (slot->tries_remaining > 0));
#endif
}

static void fsl_slot_set_unbootable(struct slot_metadata* slot) {
	slot->priority = 0;
	slot->tries_remaining = 0;
	slot->successful_boot = 0;
#ifdef CONFIG_DUAL_BOOTLOADER
	slot->bootloader_verified = 0;
#endif
}

/* Ensure all unbootable and/or illegal states are marked as the
 * canonical 'unbootable' state, e.g. priority=0, tries_remaining=0,
 * and successful_boot=0.
 */
static void fsl_slot_normalize(struct slot_metadata* slot) {
	if (slot->priority > 0) {
#if defined(CONFIG_DUAL_BOOTLOADER) && !defined(CONFIG_SPL_BUILD)
		if ((slot->tries_remaining == 0)
			&& (slot->bootloader_verified != 1)) {
			/* We've exhausted all tries -> unbootable. */
			fsl_slot_set_unbootable(slot);
		}
#else
		if (slot->tries_remaining == 0) {
			/* We've exhausted all tries -> unbootable. */
			fsl_slot_set_unbootable(slot);
		}
#endif
	} else {
		fsl_slot_set_unbootable(slot);
	}
}

/* Helper function to load metadata - returns AVB_IO_RESULT_OK on
 * success, error code otherwise.
 */
static AvbIOResult fsl_load_metadata(AvbABOps* ab_ops,
					struct bootloader_control* ab_data,
					struct bootloader_control* ab_data_orig) {
	AvbIOResult io_ret;

	io_ret = ab_ops->read_ab_metadata(ab_ops, ab_data);
	if (io_ret != AVB_IO_RESULT_OK) {
		printf("I/O error while loading A/B metadata.\n");
		return io_ret;
	}
	*ab_data_orig = *ab_data;

	/* Ensure data is normalized, e.g. illegal states will be marked as
	 * unbootable and all unbootable states are represented with
	 * (priority=0, tries_remaining=0, successful_boot=0).
	 */
	fsl_slot_normalize(&ab_data->slot_info[0]);
	fsl_slot_normalize(&ab_data->slot_info[1]);
	return AVB_IO_RESULT_OK;
}

/* Writes A/B metadata to disk only if it has been changed.
 */
static AvbIOResult fsl_save_metadata_if_changed(AvbABOps* ab_ops,
						struct bootloader_control* ab_data,
						struct bootloader_control* ab_data_orig) {
	if (avb_safe_memcmp(ab_data, ab_data_orig, sizeof(struct bootloader_control)) != 0) {
		printf("Writing A/B metadata to disk.\n");
		return ab_ops->write_ab_metadata(ab_ops, ab_data);
	}
	return AVB_IO_RESULT_OK;
}

AvbIOResult fsl_avb_ab_mark_slot_active(AvbABOps* ab_ops,
					unsigned int slot_number) {
	struct bootloader_control ab_data, ab_data_orig;
	unsigned int other_slot_number;
	AvbIOResult ret;

	avb_assert(slot_number < 2);

	ret = fsl_load_metadata(ab_ops, &ab_data, &ab_data_orig);
	if (ret != AVB_IO_RESULT_OK) {
		goto out;
	}

	/* Make requested slot top priority, unsuccessful, and with max tries. */
	ab_data.slot_info[slot_number].priority = AVB_AB_MAX_PRIORITY;
	ab_data.slot_info[slot_number].tries_remaining = AVB_AB_MAX_TRIES_REMAINING;
	ab_data.slot_info[slot_number].successful_boot = 0;
#ifdef CONFIG_DUAL_BOOTLOADER
	ab_data.slot_info[slot_number].bootloader_verified = 0;
#endif

  /* Ensure other slot doesn't have as high a priority. */
	other_slot_number = 1 - slot_number;
	if (ab_data.slot_info[other_slot_number].priority == AVB_AB_MAX_PRIORITY) {
 		ab_data.slot_info[other_slot_number].priority = AVB_AB_MAX_PRIORITY - 1;
	}

	ret = AVB_IO_RESULT_OK;

out:
	if (ret == AVB_IO_RESULT_OK) {
		ret = fsl_save_metadata_if_changed(ab_ops, &ab_data, &ab_data_orig);
	}
	return ret;
}


/* Below are the A/B AVB flow in spl and uboot proper. */
#if defined(CONFIG_DUAL_BOOTLOADER) && defined(CONFIG_SPL_BUILD)

#define PARTITION_NAME_LEN 13
#define PARTITION_BOOTLOADER "bootloader"
#ifdef CONFIG_ANDROID_AUTO_SUPPORT
/* This should always sync with the gpt */
#define PARTITION_MISC_ID 9
#endif

extern int mmc_switch(struct mmc *mmc, u8 set, u8 index, u8 value);

/* Pre-declaration of h_spl_load_read(), see detail implementation in
 * common/spl/spl_mmc.c.
 */
ulong h_spl_load_read(struct spl_load_info *load, ulong sector,
		      ulong count, void *buf);

/* Writes A/B metadata to disk only if it has changed.
 */
int fsl_save_metadata_if_changed_dual_uboot(struct blk_desc *dev_desc,
					    struct bootloader_control* ab_data,
					    struct bootloader_control* ab_data_orig) {
	struct bootloader_control serialized;
	size_t num_bytes;
	disk_partition_t info;

	/* Save metadata if changed. */
	if (memcmp(ab_data, ab_data_orig, sizeof(struct bootloader_control)) != 0) {
		/* Get misc partition info */
#ifdef CONFIG_ANDROID_AUTO_SUPPORT
		if (part_get_info(dev_desc, PARTITION_MISC_ID, &info) == -1) {
#else
		if (part_get_info_by_name(dev_desc, FASTBOOT_PARTITION_MISC, &info) == -1) {
#endif
			printf("Can't get partition info of partition: misc\n");
			return -1;
		}

		/* Writing A/B metadata to disk. */
		fsl_avb_ab_data_update_crc_and_byteswap(ab_data, &serialized);
		if (write_to_partition_in_bytes(dev_desc, &info,
						FSL_AB_METADATA_MISC_PARTITION_OFFSET,
						sizeof(struct bootloader_control),
						(void *)&serialized, &num_bytes) ||
						(num_bytes != sizeof(struct bootloader_control))) {
			printf("Error--write metadata fail!\n");
			return -1;
		}
	}
	return 0;
}

/* Load metadate from misc partition.
 */
int fsl_load_metadata_dual_uboot(struct blk_desc *dev_desc,
				 struct bootloader_control* ab_data,
				 struct bootloader_control* ab_data_orig) {
	disk_partition_t info;
	struct bootloader_control serialized;
	size_t num_bytes;

#ifdef CONFIG_ANDROID_AUTO_SUPPORT
	if (part_get_info(dev_desc, PARTITION_MISC_ID, &info) == -1) {
#else
	if (part_get_info_by_name(dev_desc, FASTBOOT_PARTITION_MISC, &info) == -1) {
#endif
		printf("Can't get partition info of partition: misc\n");
		return -1;
	} else {
		read_from_partition_in_bytes(dev_desc, &info,
						FSL_AB_METADATA_MISC_PARTITION_OFFSET,
						sizeof(struct bootloader_control),
						(void *)ab_data, &num_bytes );
		if (num_bytes != sizeof(struct bootloader_control)) {
			printf("Error--read metadata fail!\n");
			return -1;
		} else {
			if (!fsl_avb_ab_data_verify_and_byteswap(ab_data, &serialized)) {
				printf("Error validating A/B metadata from disk.\n");
				printf("Resetting and writing new A/B metadata to disk.\n");
				fsl_avb_ab_data_init(ab_data);
				fsl_avb_ab_data_update_crc_and_byteswap(ab_data, &serialized);
				num_bytes = 0;
				if (write_to_partition_in_bytes(dev_desc, &info, FSL_AB_METADATA_MISC_PARTITION_OFFSET,
								  sizeof(struct bootloader_control),
								  (void *)&serialized, &num_bytes) ||
								(num_bytes != sizeof(struct bootloader_control))) {
					printf("Error--write metadata fail!\n");
					return -1;
				} else
					return 0;
			} else {
				memcpy(ab_data_orig, ab_data, sizeof(struct bootloader_control));
				/* Ensure data is normalized, e.g. illegal states will be marked as
				 * unbootable and all unbootable states are represented with
				 * (priority=0, tries_remaining=0, successful_boot=0).
				 */
				fsl_slot_normalize(&ab_data->slot_info[0]);
				fsl_slot_normalize(&ab_data->slot_info[1]);
				return 0;
			}
		}
	}
}

#if !defined(CONFIG_XEN) && defined(CONFIG_IMX_TRUSTY_OS)
static int spl_verify_rbidx(struct mmc *mmc, struct slot_metadata *slot,
			struct spl_image_info *spl_image)
{
	kblb_hdr_t hdr;
	kblb_tag_t *rbk;
	uint64_t extract_idx;
#ifdef CONFIG_AVB_ATX
	struct bl_rbindex_package *bl_rbindex;
#endif

	/* Make sure rollback index has been initialized before verify */
	if (rpmb_init()) {
		printf("RPMB init failed!\n");
		return -1;
	}

	/* Read bootloader rollback index header first. */
	if (rpmb_read(mmc, (uint8_t *)&hdr, sizeof(hdr),
			BOOTLOADER_RBIDX_OFFSET) != 0) {
		printf("Read RPMB error!\n");
		return -1;
	}

	/* Read bootloader rollback index. */
	rbk = &(hdr.bootloader_rbk_tags);
	if (rpmb_read(mmc, (uint8_t *)&extract_idx, rbk->len, rbk->offset) != 0) {
		printf("Read rollback index error!\n");
		return -1;
	}

	/* Verify bootloader rollback index. */
	if (spl_image->rbindex >= extract_idx) {
		/* Rollback index verify pass, update it only when current slot
		 * has been marked as successful.
		 */
		if ((slot->successful_boot != 0) && (spl_image->rbindex != extract_idx) &&
				rpmb_write(mmc, (uint8_t *)(&(spl_image->rbindex)),
				rbk->len, rbk->offset)) {
			printf("Update bootloader rollback index failed!\n");
			return -1;
		}

#ifdef CONFIG_AVB_ATX
		/* Pass bootloader rbindex to u-boot here. */
		bl_rbindex = (struct bl_rbindex_package *)BL_RBINDEX_LOAD_ADDR;
		memcpy(bl_rbindex->magic, BL_RBINDEX_MAGIC, BL_RBINDEX_MAGIC_LEN);
		if (slot->successful_boot != 0)
			bl_rbindex->rbindex = spl_image->rbindex;
		else
			bl_rbindex->rbindex = extract_idx;
#endif

		return 0;
	} else {
		printf("Rollback index verify rejected!\n");
		return -1;
	}

}
#endif /* !CONFIG_XEN && CONFIG_IMX_TRUSTY_OS */

int mmc_load_image_raw_sector_dual_uboot(struct spl_image_info *spl_image,
					 struct mmc *mmc)
{
	unsigned long count;
	disk_partition_t info;
	int ret = 0, n = 0;
	char partition_name[PARTITION_NAME_LEN];
	struct blk_desc *dev_desc;
	struct image_header *header;
	struct spl_load_info load;
	struct bootloader_control ab_data, ab_data_orig;
	size_t slot_index_to_boot, target_slot;
#if !defined(CONFIG_XEN) && defined(CONFIG_IMX_TRUSTY_OS)
	struct keyslot_package kp;
#endif

	/* Check if gpt is valid */
	dev_desc = mmc_get_blk_desc(mmc);
	if (dev_desc) {
		if (part_get_info(dev_desc, 1, &info)) {
			printf("GPT is invalid, please flash correct GPT!\n");
			return -1;
		}
	} else {
		printf("Get block desc fail!\n");
		return -1;
	}

#if !defined(CONFIG_XEN) && defined(CONFIG_IMX_TRUSTY_OS)
	read_keyslot_package(&kp);
	if (strcmp(kp.magic, KEYPACK_MAGIC)) {
		if (rpmbkey_is_set()) {
			printf("\nFATAL - RPMB key was destroyed!\n");
			hang();
		} else
			printf("keyslot package magic error, do nothing here!\n");
	} else {
		/* Set power-on write protection to boot1 partition. */
		if (mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_BOOT_WP, BOOT1_PWR_WP)) {
			printf("Unable to set power-on write protection to boot1!\n");
			return -1;
		}
	}
#endif

	/* Load AB metadata from misc partition */
	if (fsl_load_metadata_dual_uboot(dev_desc, &ab_data,
					&ab_data_orig)) {
		return -1;
	}

	slot_index_to_boot = 2;  // Means not 0 or 1
	target_slot =
	    (ab_data.slot_info[1].priority > ab_data.slot_info[0].priority) ? 1 : 0;

	for (n = 0; n < 2; n++) {
		if (!fsl_slot_is_bootable(&ab_data.slot_info[target_slot])) {
			target_slot = (target_slot == 1 ? 0 : 1);
			continue;
		}
		/* Choose slot to load. */
		snprintf(partition_name, PARTITION_NAME_LEN,
			 PARTITION_BOOTLOADER"%s",
			 slot_suffix[target_slot]);

		/* Read part info from gpt */
		if (part_get_info_by_name(dev_desc, partition_name, &info) == -1) {
			printf("Can't get partition info of partition bootloader%s\n",
				slot_suffix[target_slot]);
			ret = -1;
			goto end;
		} else {
			header = (struct image_header *)(CONFIG_SYS_TEXT_BASE -
				 sizeof(struct image_header));

			/* read image header to find the image size & load address */
			count = blk_dread(dev_desc, info.start, 1, header);
			if (count == 0) {
				ret = -1;
				goto end;
			}

			/* Load fit/container and check HAB */
			load.dev = mmc;
			load.priv = NULL;
			load.filename = NULL;
			load.bl_len = mmc->read_bl_len;
			load.read = h_spl_load_read;
			if (IS_ENABLED(CONFIG_SPL_LOAD_FIT) &&
					image_get_magic(header) == FDT_MAGIC) {
				/* Fit */
				ret = spl_load_simple_fit(spl_image, &load,
							  info.start, header);
			} else if (IS_ENABLED(CONFIG_SPL_LOAD_IMX_CONTAINER)) {
				/* container */
				ret = spl_load_imx_container(spl_image, &load, info.start);
			} else
				ret = -1;

#if !defined(CONFIG_XEN) && defined(CONFIG_IMX_TRUSTY_OS)
			/* Image loaded successfully, go to verify rollback index */
			if (rpmbkey_is_set()) {
				if (!ret)
					ret = spl_verify_rbidx(mmc, &ab_data.slot_info[target_slot], spl_image);

				/* Copy rpmb keyslot to secure memory. */
				if (!ret)
					fill_secure_keyslot_package(&kp);
			}
#endif
		}

		/* Set current slot to unbootable if load/verify fail. */
		if (ret != 0) {
			/* Reboot if current slot has booted succefully before, this prevents
			 * slot been marked as "unbootable" due to some random failures (like
			 * eMMC/DRAM access error at some critical temperature).
			 */
			if (ab_data.slot_info[target_slot].successful_boot)
				do_reset(NULL, 0, 0, NULL);
			else {
				printf("Load or verify bootloader%s fail, setting unbootable..\n",
				       slot_suffix[target_slot]);
				fsl_slot_set_unbootable(&ab_data.slot_info[target_slot]);
				/* Switch to another slot. */
				target_slot = (target_slot == 1 ? 0 : 1);
			}
		} else {
			slot_index_to_boot = target_slot;
			n = 2;
		}
	}

	if (slot_index_to_boot == 2) {
		/* No bootable slots, try to boot into recovery! */
		printf("No bootable slots found, try to boot into recovery mode...\n");

		ab_data.spl_recovery = true;
		if ((ab_data.last_boot != 0) && (ab_data.last_boot != 1))
			slot_index_to_boot = 0;
		else
			slot_index_to_boot = ab_data.last_boot;

		snprintf(partition_name, PARTITION_NAME_LEN,
			 PARTITION_BOOTLOADER"%s",
			 slot_suffix[target_slot]);

		/* Read part info from gpt */
		if (part_get_info_by_name(dev_desc, partition_name, &info) == -1) {
			printf("Can't get partition info of partition bootloader%s\n",
				slot_suffix[target_slot]);
			ret = -1;
			goto end;
		} else {
			header = (struct image_header *)(CONFIG_SYS_TEXT_BASE -
				 sizeof(struct image_header));

			/* read image header to find the image size & load address */
			count = blk_dread(dev_desc, info.start, 1, header);
			if (count == 0) {
				ret = -1;
				goto end;
			}

			/* Load fit/container and check HAB */
			load.dev = mmc;
			load.priv = NULL;
			load.filename = NULL;
			load.bl_len = mmc->read_bl_len;
			load.read = h_spl_load_read;
			if (IS_ENABLED(CONFIG_SPL_LOAD_FIT) &&
					image_get_magic(header) == FDT_MAGIC) {
				/* Fit */
				ret = spl_load_simple_fit(spl_image, &load,
							  info.start, header);
			} else if (IS_ENABLED(CONFIG_SPL_LOAD_IMX_CONTAINER)) {
				/* container */
				ret = spl_load_imx_container(spl_image, &load, info.start);
			} else
				ret = -1;

#if !defined(CONFIG_XEN) && defined(CONFIG_IMX_TRUSTY_OS)
			/* Image loaded successfully, go to verify rollback index */
			if (rpmbkey_is_set()) {
				if (!ret)
					ret = spl_verify_rbidx(mmc, &ab_data.slot_info[target_slot], spl_image);

				/* Copy rpmb keyslot to secure memory. */
				if (!ret)
					fill_secure_keyslot_package(&kp);
			}
#endif
		}

		if (ret)
			goto end;
	} else if (!ab_data.slot_info[slot_index_to_boot].successful_boot &&
		   (ab_data.slot_info[slot_index_to_boot].tries_remaining > 0)) {
		/* Set the bootloader_verified flag as if current slot only has one chance. */
		if (ab_data.slot_info[slot_index_to_boot].tries_remaining == 1)
			ab_data.slot_info[slot_index_to_boot].bootloader_verified = 1;
		ab_data.slot_info[slot_index_to_boot].tries_remaining -= 1;

		ab_data.last_boot = slot_index_to_boot;
	}
	printf("Booting from bootloader%s...\n", slot_suffix[slot_index_to_boot]);

end:
	/* Save metadata if changed. */
	if (fsl_save_metadata_if_changed_dual_uboot(dev_desc, &ab_data, &ab_data_orig)) {
		ret = -1;
	}

	if (ret)
		return -1;
	else
		return 0;
}

/*
 * spl_fit_get_rbindex(): Get rollback index of the bootloader.
 * @fit:	Pointer to the FDT blob.
 * @images:	Offset of the /images subnode.
 *
 * Return:	the rollback index value of bootloader or a negative
 * 		error number.
 */
int spl_fit_get_rbindex(const void *fit, int images)
{
	const char *str;
	uint64_t index;
	int conf_node;
	int len;

	conf_node = fit_find_config_node(fit);
	if (conf_node < 0) {
		return conf_node;
	}

	str = fdt_getprop(fit, conf_node, "rbindex", &len);
	if (!str) {
		debug("cannot find property 'rbindex'\n");
		return -EINVAL;
	}

	index = simple_strtoul(str, NULL, 10);

	return index;
}

/* For normal build */
#elif !defined(CONFIG_SPL_BUILD)

#ifdef CONFIG_DUAL_BOOTLOADER
// dual bootloader flow in uboot proper
AvbABFlowResult avb_flow_dual_uboot(AvbABOps* ab_ops,
				    const char* const* requested_partitions,
				    AvbSlotVerifyFlags flags,
				    AvbHashtreeErrorMode hashtree_error_mode,
				    AvbSlotVerifyData** out_data) {
	AvbOps* ops = ab_ops->ops;
	AvbSlotVerifyData* slot_data = NULL;
	AvbSlotVerifyData* data = NULL;
	AvbABFlowResult ret;
	struct bootloader_control ab_data, ab_data_orig;
	AvbIOResult io_ret;
	bool saw_and_allowed_verification_error = false;
	AvbSlotVerifyResult verify_result;
	bool set_slot_unbootable = false;
	int target_slot, n;
	uint64_t rollback_index_value = 0;
	uint64_t current_rollback_index_value = 0;

	io_ret = fsl_load_metadata(ab_ops, &ab_data, &ab_data_orig);
	if (io_ret == AVB_IO_RESULT_ERROR_OOM) {
		ret = AVB_AB_FLOW_RESULT_ERROR_OOM;
		goto out;
	} else if (io_ret != AVB_IO_RESULT_OK) {
		ret = AVB_AB_FLOW_RESULT_ERROR_IO;
		goto out;
	}

	/* Choose the target slot, it should be the same with the one in SPL. */
	target_slot = get_curr_slot(&ab_data);
	if (target_slot == -1) {
		ret = AVB_AB_FLOW_RESULT_ERROR_NO_BOOTABLE_SLOTS;
		printf("No bootable slot found!\n");
		goto out;
	}
	/* Clear the bootloader_verified flag. */
	ab_data.slot_info[target_slot].bootloader_verified = 0;

	printf("Verifying slot %s ...\n", slot_suffix[target_slot]);
	verify_result = avb_slot_verify(ops,
					requested_partitions,
					slot_suffix[target_slot],
					flags,
					hashtree_error_mode,
					&slot_data);

	switch (verify_result) {
		case AVB_SLOT_VERIFY_RESULT_ERROR_OOM:
			ret = AVB_AB_FLOW_RESULT_ERROR_OOM;
			goto out;

		case AVB_SLOT_VERIFY_RESULT_ERROR_IO:
			ret = AVB_AB_FLOW_RESULT_ERROR_IO;
			goto out;

		case AVB_SLOT_VERIFY_RESULT_OK:
			ret = AVB_AB_FLOW_RESULT_OK;
			break;

		case AVB_SLOT_VERIFY_RESULT_ERROR_INVALID_METADATA:
		case AVB_SLOT_VERIFY_RESULT_ERROR_UNSUPPORTED_VERSION:
			/* Even with AVB_SLOT_VERIFY_FLAGS_ALLOW_VERIFICATION_ERROR
			 * these mean game over.
			 */
			set_slot_unbootable = true;
			break;

		case AVB_SLOT_VERIFY_RESULT_ERROR_VERIFICATION:
		case AVB_SLOT_VERIFY_RESULT_ERROR_ROLLBACK_INDEX:
		case AVB_SLOT_VERIFY_RESULT_ERROR_PUBLIC_KEY_REJECTED:
			if (flags & AVB_SLOT_VERIFY_FLAGS_ALLOW_VERIFICATION_ERROR) {
				/* Do nothing since we allow this. */
				avb_debugv("Allowing slot ",
					   slot_suffix[target_slot],
					   " which verified "
					   "with result ",
					   avb_slot_verify_result_to_string(verify_result),
					   " because "
					   "AVB_SLOT_VERIFY_FLAGS_ALLOW_VERIFICATION_ERROR "
					   "is set.\n",
					   NULL);
				saw_and_allowed_verification_error =
					 true;
			} else {
				set_slot_unbootable = true;
			}
			break;

		case AVB_SLOT_VERIFY_RESULT_ERROR_INVALID_ARGUMENT:
			ret = AVB_AB_FLOW_RESULT_ERROR_INVALID_ARGUMENT;
			goto out;
			/* Do not add a 'default:' case here because
			 * of -Wswitch.
			 */
	}

	if (set_slot_unbootable) {
		/* Reboot if current slot has booted succefully before, this prevents
		 * slot been marked as "unbootable" due to some random failures (like
		 * eMMC/DRAM access error at some critical temperature).
		 */
		if (ab_data.slot_info[target_slot].successful_boot)
			do_reset(NULL, 0, 0, NULL);
		else {
			avb_errorv("Error verifying slot ",
				   slot_suffix[target_slot],
				   " with result ",
				   avb_slot_verify_result_to_string(verify_result),
				   " - setting unbootable.\n",
				   NULL);
			fsl_slot_set_unbootable(&ab_data.slot_info[target_slot]);

			/* Only the slot chosen by SPL will be verified here so we
			 * return AVB_AB_FLOW_RESULT_ERROR_NO_BOOTABLE_SLOTS if the
			 * slot should be set unbootable.
			 */
			ret = AVB_AB_FLOW_RESULT_ERROR_NO_BOOTABLE_SLOTS;
			goto out;
		}
	}

	/* Update stored rollback index only when the slot has been marked
	 * as successful. Do this for every rollback index location.
	*/
	if ((ret == AVB_AB_FLOW_RESULT_OK) &&
		(ab_data.slot_info[target_slot].successful_boot != 0)) {
		for (n = 0; n < AVB_MAX_NUMBER_OF_ROLLBACK_INDEX_LOCATIONS; n++) {

			rollback_index_value = slot_data->rollback_indexes[n];

			if (rollback_index_value != 0) {
				io_ret = ops->read_rollback_index(
						ops, n, &current_rollback_index_value);
				if (io_ret == AVB_IO_RESULT_ERROR_OOM) {
					ret = AVB_AB_FLOW_RESULT_ERROR_OOM;
					goto out;
				} else if (io_ret != AVB_IO_RESULT_OK) {
					printf("Error getting rollback index for slot.\n");
					ret = AVB_AB_FLOW_RESULT_ERROR_IO;
					goto out;
				}
				if (current_rollback_index_value != rollback_index_value) {
					io_ret = ops->write_rollback_index(
							ops, n, rollback_index_value);
					if (io_ret == AVB_IO_RESULT_ERROR_OOM) {
						ret = AVB_AB_FLOW_RESULT_ERROR_OOM;
						goto out;
					} else if (io_ret != AVB_IO_RESULT_OK) {
						printf("Error setting stored rollback index.\n");
						ret = AVB_AB_FLOW_RESULT_ERROR_IO;
						goto out;
					}
				}
			}
		}
	}

	/* Finally, select this slot. */
	avb_assert(slot_data != NULL);
	data = slot_data;
	slot_data = NULL;
	if (saw_and_allowed_verification_error) {
		avb_assert(
			flags & AVB_SLOT_VERIFY_FLAGS_ALLOW_VERIFICATION_ERROR);
		ret = AVB_AB_FLOW_RESULT_OK_WITH_VERIFICATION_ERROR;
	} else {
		ret = AVB_AB_FLOW_RESULT_OK;
	}

out:
	io_ret = fsl_save_metadata_if_changed(ab_ops, &ab_data, &ab_data_orig);
	if (io_ret != AVB_IO_RESULT_OK) {
		if (io_ret == AVB_IO_RESULT_ERROR_OOM) {
			ret = AVB_AB_FLOW_RESULT_ERROR_OOM;
		} else {
			ret = AVB_AB_FLOW_RESULT_ERROR_IO;
		}
		if (data != NULL) {
			avb_slot_verify_data_free(data);
			data = NULL;
		}
	}

	if (slot_data != NULL)
		avb_slot_verify_data_free(slot_data);

	if (out_data != NULL) {
		*out_data = data;
	} else {
		if (data != NULL) {
			avb_slot_verify_data_free(data);
		}
	}

	return ret;
}

static bool spl_recovery_flag = false;
bool is_spl_recovery(void)
{
	return spl_recovery_flag;
}
void check_spl_recovery(void)
{
	struct bootloader_control ab_data, ab_data_orig;
	AvbIOResult io_ret;

	io_ret = fsl_load_metadata(&fsl_avb_ab_ops, &ab_data, &ab_data_orig);
	if (io_ret != AVB_IO_RESULT_OK) {
		printf("Load metadata fail, go to fail!\n");
		hang();
	}

	spl_recovery_flag = ab_data.spl_recovery;
	/* Clear spl recovery flag. */
	ab_data.spl_recovery = false;
	fsl_save_metadata_if_changed(&fsl_avb_ab_ops, &ab_data, &ab_data_orig);

	if (spl_recovery_flag) {
		printf("Enter spl recovery mode, only fastboot commands are supported!\n");

		while (1) {
			run_command("fastboot 0", 0);
		}
	}
}

#else /* CONFIG_DUAL_BOOTLOADER */
/* For legacy i.mx6/7, we won't enable A/B due to the limitation of
 * storage capacity, but we still want to verify boot/recovery with
 * AVB. */
AvbABFlowResult avb_single_flow(AvbABOps* ab_ops,
                            const char* const* requested_partitions,
                            AvbSlotVerifyFlags flags,
                            AvbHashtreeErrorMode hashtree_error_mode,
                            AvbSlotVerifyData** out_data) {
  AvbOps* ops = ab_ops->ops;
  AvbSlotVerifyData* slot_data = NULL;
  AvbSlotVerifyData* data = NULL;
  AvbABFlowResult ret;
  bool saw_and_allowed_verification_error = false;

  /* Validate boot/recovery. */
  AvbSlotVerifyResult verify_result;

  verify_result = avb_slot_verify(ops,
                    requested_partitions,
                    "",
                    flags,
                    hashtree_error_mode,
                    &slot_data);
  switch (verify_result) {
      case AVB_SLOT_VERIFY_RESULT_ERROR_OOM:
        ret = AVB_AB_FLOW_RESULT_ERROR_OOM;
        goto out;

      case AVB_SLOT_VERIFY_RESULT_ERROR_IO:
        ret = AVB_AB_FLOW_RESULT_ERROR_IO;
        goto out;

      case AVB_SLOT_VERIFY_RESULT_OK:
        break;

      case AVB_SLOT_VERIFY_RESULT_ERROR_INVALID_METADATA:
      case AVB_SLOT_VERIFY_RESULT_ERROR_UNSUPPORTED_VERSION:
          /* Even with AVB_SLOT_VERIFY_FLAGS_ALLOW_VERIFICATION_ERROR
           * these mean game over.
           */
        ret = AVB_AB_FLOW_RESULT_ERROR_NO_BOOTABLE_SLOTS;
        goto out;

      /* explicit fallthrough. */
      case AVB_SLOT_VERIFY_RESULT_ERROR_VERIFICATION:
      case AVB_SLOT_VERIFY_RESULT_ERROR_ROLLBACK_INDEX:
      case AVB_SLOT_VERIFY_RESULT_ERROR_PUBLIC_KEY_REJECTED:
        if (flags & AVB_SLOT_VERIFY_FLAGS_ALLOW_VERIFICATION_ERROR) {
          /* Do nothing since we allow this. */
          avb_debugv("Allowing slot ",
                     slot_suffix[n],
                     " which verified "
                     "with result ",
                     avb_slot_verify_result_to_string(verify_result),
                     " because "
                     "AVB_SLOT_VERIFY_FLAGS_ALLOW_VERIFICATION_ERROR "
                     "is set.\n",
                     NULL);
          saw_and_allowed_verification_error = true;
        } else {
            ret = AVB_AB_FLOW_RESULT_ERROR_NO_BOOTABLE_SLOTS;
            goto out;
        }
        break;

      case AVB_SLOT_VERIFY_RESULT_ERROR_INVALID_ARGUMENT:
        ret = AVB_AB_FLOW_RESULT_ERROR_INVALID_ARGUMENT;
        goto out;
        /* Do not add a 'default:' case here because of -Wswitch. */
      }

  avb_assert(slot_data != NULL);
  data = slot_data;
  slot_data = NULL;
  if (saw_and_allowed_verification_error) {
    avb_assert(flags & AVB_SLOT_VERIFY_FLAGS_ALLOW_VERIFICATION_ERROR);
    ret = AVB_AB_FLOW_RESULT_OK_WITH_VERIFICATION_ERROR;
  } else {
    ret = AVB_AB_FLOW_RESULT_OK;
  }

out:
  if (slot_data != NULL) {
    avb_slot_verify_data_free(slot_data);
  }

  if (out_data != NULL) {
    *out_data = data;
  } else {
    if (data != NULL) {
      avb_slot_verify_data_free(data);
    }
  }

  return ret;
}

AvbABFlowResult avb_ab_flow_fast(AvbABOps* ab_ops,
				 const char* const* requested_partitions,
				 AvbSlotVerifyFlags flags,
				 AvbHashtreeErrorMode hashtree_error_mode,
				 AvbSlotVerifyData** out_data) {
	AvbOps* ops = ab_ops->ops;
	AvbSlotVerifyData* slot_data[2] = {NULL, NULL};
	AvbSlotVerifyData* data = NULL;
	AvbABFlowResult ret;
	struct bootloader_control ab_data, ab_data_orig;
	size_t slot_index_to_boot, n;
	AvbIOResult io_ret;
	bool saw_and_allowed_verification_error = false;
	size_t target_slot;
	AvbSlotVerifyResult verify_result;
	bool set_slot_unbootable = false;
	uint64_t rollback_index_value = 0;
	uint64_t current_rollback_index_value = 0;

	io_ret = fsl_load_metadata(ab_ops, &ab_data, &ab_data_orig);
	if (io_ret == AVB_IO_RESULT_ERROR_OOM) {
		ret = AVB_AB_FLOW_RESULT_ERROR_OOM;
		goto out;
	} else if (io_ret != AVB_IO_RESULT_OK) {
		ret = AVB_AB_FLOW_RESULT_ERROR_IO;
		goto out;
	}

	slot_index_to_boot = 2;  // Means not 0 or 1
	target_slot =
	    (ab_data.slot_info[1].priority > ab_data.slot_info[0].priority) ? 1 : 0;

	for (n = 0; n < 2; n++) {
		if (!fsl_slot_is_bootable(&ab_data.slot_info[target_slot])) {
			target_slot = (target_slot == 1 ? 0 : 1);
			continue;
		}
		verify_result = avb_slot_verify(ops,
						requested_partitions,
						slot_suffix[target_slot],
						flags,
						hashtree_error_mode,
						&slot_data[target_slot]);
		switch (verify_result) {
			case AVB_SLOT_VERIFY_RESULT_ERROR_OOM:
				ret = AVB_AB_FLOW_RESULT_ERROR_OOM;
				goto out;

			case AVB_SLOT_VERIFY_RESULT_ERROR_IO:
				ret = AVB_AB_FLOW_RESULT_ERROR_IO;
				goto out;

			case AVB_SLOT_VERIFY_RESULT_OK:
				slot_index_to_boot = target_slot;
				ret = AVB_AB_FLOW_RESULT_OK;
				n = 2;
				break;

			case AVB_SLOT_VERIFY_RESULT_ERROR_INVALID_METADATA:
			case AVB_SLOT_VERIFY_RESULT_ERROR_UNSUPPORTED_VERSION:
				/* Even with AVB_SLOT_VERIFY_FLAGS_ALLOW_VERIFICATION_ERROR
				 * these mean game over.
				 */
				set_slot_unbootable = true;
				break;

			/* explicit fallthrough. */
			case AVB_SLOT_VERIFY_RESULT_ERROR_VERIFICATION:
			case AVB_SLOT_VERIFY_RESULT_ERROR_ROLLBACK_INDEX:
			case AVB_SLOT_VERIFY_RESULT_ERROR_PUBLIC_KEY_REJECTED:
				if (flags & AVB_SLOT_VERIFY_FLAGS_ALLOW_VERIFICATION_ERROR) {
					/* Do nothing since we allow this. */
					avb_debugv("Allowing slot ",
						   slot_suffix[target_slot],
						   " which verified "
						   "with result ",
						   avb_slot_verify_result_to_string(verify_result),
						   " because "
						   "AVB_SLOT_VERIFY_FLAGS_ALLOW_VERIFICATION_ERROR "
						   "is set.\n",
						   NULL);
					saw_and_allowed_verification_error =
						 true;
					slot_index_to_boot = target_slot;
					n = 2;
				} else {
					set_slot_unbootable = true;
				}
				break;

			case AVB_SLOT_VERIFY_RESULT_ERROR_INVALID_ARGUMENT:
				ret = AVB_AB_FLOW_RESULT_ERROR_INVALID_ARGUMENT;
				goto out;
				/* Do not add a 'default:' case here because
				 * of -Wswitch.
				 */
		}

		if (set_slot_unbootable) {
			/* Reboot if current slot has booted succefully before, this prevents
			 * slot been marked as "unbootable" due to some random failures (like
			 * eMMC/DRAM access error at some critical temperature).
			 */
			if (ab_data.slot_info[target_slot].successful_boot)
				do_reset(NULL, 0, 0, NULL);
			else {
				avb_errorv("Error verifying slot ",
					   slot_suffix[target_slot],
					   " with result ",
					   avb_slot_verify_result_to_string(verify_result),
					   " - setting unbootable.\n",
					   NULL);
				fsl_slot_set_unbootable(&ab_data.slot_info[target_slot]);
				set_slot_unbootable = false;
			}
			if (slot_data[target_slot] != NULL) {
				avb_slot_verify_data_free(slot_data[target_slot]);
				slot_data[target_slot] = NULL;
			}
		}
		/* switch to another slot */
		target_slot = (target_slot == 1 ? 0 : 1);
	}

	if (slot_index_to_boot == 2) {
		/* No bootable slots! */
		printf("No bootable slots found.\n");
		ret = AVB_AB_FLOW_RESULT_ERROR_NO_BOOTABLE_SLOTS;
		goto out;
	}

	/* Update stored rollback index only when the slot has been marked
	 * as successful. Do this for every rollback index location.
	*/
	if ((ret == AVB_AB_FLOW_RESULT_OK) &&
		(ab_data.slot_info[slot_index_to_boot].successful_boot != 0)) {
		for (n = 0; n < AVB_MAX_NUMBER_OF_ROLLBACK_INDEX_LOCATIONS; n++) {

			rollback_index_value = slot_data[slot_index_to_boot]->rollback_indexes[n];

			if (rollback_index_value != 0) {
				io_ret = ops->read_rollback_index(
						ops, n, &current_rollback_index_value);
				if (io_ret == AVB_IO_RESULT_ERROR_OOM) {
					ret = AVB_AB_FLOW_RESULT_ERROR_OOM;
					goto out;
				} else if (io_ret != AVB_IO_RESULT_OK) {
					printf("Error getting rollback index for slot.\n");
					ret = AVB_AB_FLOW_RESULT_ERROR_IO;
					goto out;
				}
				if (current_rollback_index_value != rollback_index_value) {
					io_ret = ops->write_rollback_index(
							ops, n, rollback_index_value);
					if (io_ret == AVB_IO_RESULT_ERROR_OOM) {
						ret = AVB_AB_FLOW_RESULT_ERROR_OOM;
						goto out;
					} else if (io_ret != AVB_IO_RESULT_OK) {
						printf("Error setting stored rollback index.\n");
						ret = AVB_AB_FLOW_RESULT_ERROR_IO;
						goto out;
					}
				}
			}
		}
	}

	/* Finally, select this slot. */
	avb_assert(slot_data[slot_index_to_boot] != NULL);
	data = slot_data[slot_index_to_boot];
	slot_data[slot_index_to_boot] = NULL;
	if (saw_and_allowed_verification_error) {
		avb_assert(
			flags & AVB_SLOT_VERIFY_FLAGS_ALLOW_VERIFICATION_ERROR);
		ret = AVB_AB_FLOW_RESULT_OK_WITH_VERIFICATION_ERROR;
	} else {
		ret = AVB_AB_FLOW_RESULT_OK;
	}

	/* ... and decrement tries remaining, if applicable. */
	if (!ab_data.slot_info[slot_index_to_boot].successful_boot &&
	    (ab_data.slot_info[slot_index_to_boot].tries_remaining > 0)) {
		ab_data.slot_info[slot_index_to_boot].tries_remaining -= 1;
	}

out:
	io_ret = fsl_save_metadata_if_changed(ab_ops, &ab_data, &ab_data_orig);
	if (io_ret != AVB_IO_RESULT_OK) {
		if (io_ret == AVB_IO_RESULT_ERROR_OOM) {
			ret = AVB_AB_FLOW_RESULT_ERROR_OOM;
		} else {
			ret = AVB_AB_FLOW_RESULT_ERROR_IO;
		}
		if (data != NULL) {
			avb_slot_verify_data_free(data);
			data = NULL;
		}
	}

	for (n = 0; n < 2; n++) {
		if (slot_data[n] != NULL) {
			avb_slot_verify_data_free(slot_data[n]);
		}
	}

	if (out_data != NULL) {
		*out_data = data;
	} else {
		if (data != NULL) {
			avb_slot_verify_data_free(data);
		}
	}

	return ret;
}
#endif /* CONFIG_DUAL_BOOTLOADER */
#endif /* CONFIG_DUAL_BOOTLOADER && CONFIG_SPL_BUILD */
