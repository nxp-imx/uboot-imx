// SPDX-License-Identifier: BSD-3-Clause
//
// Copyright 2025 NXP

#include <efi_loader.h>
#include <malloc.h>
#include <mmc.h>
#include <fb_fsl.h>
#include <config.h>

#define VENDOR_PATH_LENGTH 92
#define VENDOR_PATH_DATA_LEN 72

const efi_guid_t efi_gbl_vendor_media_device_guid = EFI_GBL_VENDOR_MEDIA_DEVICE_PATH_GUID;

struct efi_gbl_vendor_part_device_path {
	struct efi_device_path_vendor path;
	char vendor_defined_data[VENDOR_PATH_DATA_LEN];
	struct efi_device_path end;
};

struct efi_gbl_vendor_part {
	struct efi_object header;
	struct efi_gbl_vendor_part_device_path dp;
	struct efi_block_io_media media;
	struct efi_block_io ops;
	int hwpart;
	int start_blk;
};

struct efi_gbl_vendor_disk {
	struct mmc *mmc;
	int dev;
	/* Three possible partitions are supported:
	 * 1. bootloader0: bootloader image.
	 * 2. mcu_os: partition used for download mcu image.
	 */
	struct efi_gbl_vendor_part parts[2];
};

static struct efi_gbl_vendor_disk disk = {0};

static efi_status_t EFIAPI rw_blocks(struct efi_block_io *this,
				     uint32_t media_id, uint64_t lba,
				     size_t buffer_size, void *buffer,
				     bool is_write)
{
	struct efi_gbl_vendor_part *part = NULL;
	uint8_t original_part = 0;
	struct blk_desc *dev_desc = NULL;
	int blocks = 0, n = 0;

	if (!this || !buffer)
		EFI_EXIT(EFI_INVALID_PARAMETER);

	if (lba * this->media->block_size + buffer_size >
	    (this->media->last_block + 1) * this->media->block_size)
		return EFI_EXIT(EFI_INVALID_PARAMETER);

	if (!disk.mmc) {
		log_err("\nThe mmc device is not initialized!\n");
		return EFI_EXIT(EFI_DEVICE_ERROR);
	}
	dev_desc = mmc_get_blk_desc(disk.mmc);
	if (dev_desc == NULL) {
		log_err("Block device not supported!\n");
		return EFI_EXIT(EFI_DEVICE_ERROR);
	}

	/* Get the efi_gbl_vendor_part object */
	part = container_of(this, struct efi_gbl_vendor_part, ops);

	/* Check current hwpart, switch to the target hwpart if not desired.
	 * For SD card, ignore the hwpart switch.
	 */
	if (disk.mmc->part_config != MMCPART_NOAVAILABLE) {
		original_part = dev_desc->hwpart;
		if (original_part != part->hwpart) {
			if (mmc_switch_part(disk.mmc, part->hwpart) != 0) {
				log_err("Failed to switch to mmc part: %d", part->hwpart);
				return EFI_EXIT(EFI_DEVICE_ERROR);
			}
			dev_desc->hwpart = part->hwpart;
		}
	}

	/* Read/write the blocks. */
	blocks = buffer_size / this->media->block_size;
	if (is_write) {
		n = blk_dwrite(dev_desc, lba + part->start_blk, blocks, buffer);
	} else {
		n = blk_dread(dev_desc, lba + part->start_blk, blocks, buffer);
	}
	if (n != blocks) {
		log_err("Failed to read/write blocks! ret: %d\n", n);
		return EFI_EXIT(EFI_DEVICE_ERROR);
	}

	/* Restore the hwpart if necessary */
	if (disk.mmc->part_config != MMCPART_NOAVAILABLE) {
		if (original_part != dev_desc->hwpart) {
			if (mmc_switch_part(disk.mmc, original_part) != 0) {
				log_err("Failed to switch to mmc part: %d", original_part);
				return EFI_EXIT(EFI_DEVICE_ERROR);
			}
			dev_desc->hwpart = original_part;
		}
	}

	return EFI_EXIT(EFI_SUCCESS);
}

static efi_status_t EFIAPI read_blocks(struct efi_block_io *this, u32 media_id,
				       u64 lba, efi_uintn_t buffer_size,
				       void *buffer)
{
	EFI_ENTRY("%p, %x, %llx, %zx, %p", this, media_id, lba,
		  buffer_size, buffer);

	return rw_blocks(this, media_id, lba, buffer_size, buffer, false);
}

static efi_status_t EFIAPI write_blocks(struct efi_block_io *this, u32 media_id,
					u64 lba, efi_uintn_t buffer_size,
					void *buffer)
{
	EFI_ENTRY("%p, %x, %llx, %zx, %p", this, media_id, lba,
		  buffer_size, buffer);

	return rw_blocks(this, media_id, lba, buffer_size, buffer, true);
}

static efi_status_t EFIAPI reset(struct efi_block_io *this,
				 char extended_verification)
{
	EFI_ENTRY("%p, %x", this, extended_verification);
	return EFI_EXIT(EFI_SUCCESS);
}

static efi_status_t EFIAPI flush_blocks(struct efi_block_io *this)
{
	EFI_ENTRY("%p", this);
	return EFI_EXIT(EFI_SUCCESS);
}

static int install_block_io_part(struct efi_gbl_vendor_part *part, uint32_t part_start, uint64_t part_blks,
				 char *part_name, uint32_t hwpart) {
	efi_status_t ret;

	if (!part || !part_name || hwpart > 2) {
		return -1;
	}

	/* Hook up to the device list */
	efi_add_handle(&part->header);

	part->hwpart = hwpart;
	part->start_blk = part_start;

	/* add gbl vendor defined device path */
	part->dp.path.dp.type = DEVICE_PATH_TYPE_MEDIA_DEVICE;
	part->dp.path.dp.sub_type = DEVICE_PATH_SUB_TYPE_VENDOR_PATH;
	part->dp.path.dp.length = VENDOR_PATH_LENGTH;
	part->dp.path.guid = efi_gbl_vendor_media_device_guid;
	memset(part->dp.vendor_defined_data, 0, sizeof(part->dp.vendor_defined_data));
	memcpy(part->dp.vendor_defined_data, part_name, strlen(part_name));
	part->dp.end.type = DEVICE_PATH_TYPE_END;
	part->dp.end.sub_type = DEVICE_PATH_SUB_TYPE_END;

	/* Populates the EFI_BLOCK_IO_MEDIA */
	part->media.media_id = 1;
	part->media.removable_media = 0;
	part->media.media_present = 1;
	part->media.logical_partition = 0;
	part->media.read_only = 0;
	part->media.write_caching = 0;
	part->media.block_size = disk.mmc->write_bl_len;
	part->media.io_align = disk.mmc->write_bl_len;
	part->media.last_block = part_blks;

	/* Populates EFI_BLOCK_IO_PROTOCOL. */
	part->ops.revision = EFI_BLOCK_IO_PROTOCOL_REVISION3; //make the GBL happy.
	part->ops.media = &part->media;
	part->ops.reset = reset;
	part->ops.read_blocks = read_blocks;
	part->ops.write_blocks = write_blocks;
	part->ops.flush_blocks = flush_blocks;

	/* Installs protocols */
	struct efi_object *handle = &part->header;
	ret = efi_install_multiple_protocol_interfaces(
		&handle, &efi_guid_device_path,
		&part->dp.path.dp, &efi_block_io_guid,
		&part->ops, NULL);
	if (ret != EFI_SUCCESS) {
		efi_delete_handle(&part->header);
		return -1;
	}

	return 0;
}

extern ulong bootloader_mmc_offset(void);
efi_status_t efi_gbl_vendor_part_register(void)
{
	int mmc_dev_no, ret = 0;
	struct mmc *mmc;
	uint64_t total_blocks = 0, part_start = 0, hwpart = 0;

	/* Find the mmc device. */
	mmc_dev_no = mmc_get_env_dev();
	mmc = find_mmc_device(mmc_dev_no);
	if (!mmc || mmc_init(mmc)) {
		log_err("\nFailed to get mmc device!\n");
		return EFI_DEVICE_ERROR;
	}

	disk.mmc = mmc;
	disk.dev = mmc_dev_no;

	/* Register partition "bootloader0". This partition would be in boot
	 * part 0 for eMMC device and in user part for SD card device.
	 */
	part_start = bootloader_mmc_offset() / mmc->write_bl_len;
	if (mmc->part_config != MMCPART_NOAVAILABLE) {
		total_blocks = mmc->capacity_boot;
		hwpart = 1;
	} else {
		total_blocks = ANDROID_BOOTLOADER_SIZE;
		hwpart = 0;
	}
	total_blocks /= mmc->write_bl_len;
	total_blocks = total_blocks - 1;
	ret = install_block_io_part(&disk.parts[0], part_start, total_blocks, "bootloader0", hwpart);
	if (ret != 0) {
		log_err("Failed to register partition 'bootloader0'.\n");
		return EFI_DEVICE_ERROR;
	}

#ifdef CONFIG_FLASH_MCUFIRMWARE_SUPPORT
	part_start = ANDROID_MCU_FIRMWARE_START / mmc->write_bl_len;
	total_blocks = ANDROID_MCU_OS_PARTITION_SIZE / mmc->write_bl_len;
	total_blocks = total_blocks - 1;
	hwpart = 0;
	ret = install_block_io_part(&disk.parts[1], part_start, total_blocks, "mcu_os", hwpart);
	if (ret != 0) {
		log_err("Failed to register partition 'mcu_os'.\n");
		return EFI_DEVICE_ERROR;
	}
#endif

	return EFI_SUCCESS;
}
