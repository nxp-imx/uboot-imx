// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (C) 2024 The Android Open Source Project
 *
 * Copyright 2025 NXP
 */

#include <blk.h>
#include <efi.h>
#include <efi_gbl_boot_control_protocol.h>
#include <efi_loader.h>
#include <part.h>
#include <stdlib.h>

#include <android_bootloader_message.h>

#include <memalign.h>
#include <log.h>
#include <string.h>
#include <u-boot/crc.h>
#include <mmc.h>
#include <fb_fsl.h>
#include "../../drivers/fastboot/fb_fsl/fb_fsl_virtual_ab.h"
#include "../../drivers/fastboot/fb_fsl/fb_fsl_common.h"
#include "../../drivers/fastboot/fb_fsl/bcb.h"

#define SLOT_NUM 2
const efi_guid_t efi_gbl_boot_control_guid = EFI_GBL_BOOT_CONTROL_PROTOCOL_GUID;
static char slot_suffix[SLOT_NUM] = {'a', 'b'};

static efi_status_t EFIAPI get_slot_count(struct efi_gbl_boot_control_protocol *this,
					  uint8_t* slot_count) {
	EFI_ENTRY("%p, %p", this, slot_count);

	if (!this || !slot_count) {
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	}

	*slot_count = SLOT_NUM;

	return EFI_EXIT(EFI_SUCCESS);
}

static efi_status_t EFIAPI get_slot_info(struct efi_gbl_boot_control_protocol *this,
					 uint8_t idx, struct efi_gbl_slot_info *info)
{
	AvbIOResult result;
	struct bootloader_control ab_data = {};

	EFI_ENTRY("%p, %uc, %p", this, idx, info);
	if (!this || !info) {
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	}

	if (idx >= SLOT_NUM) {
		log_err("Invalid slot index!\n");
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	}

	/* Load slot metadata from device */
	result = fsl_avb_ab_ops.read_ab_metadata(&fsl_avb_ab_ops, &ab_data);
	if (result != AVB_IO_RESULT_OK) {
		log_err("Failed to load ab metadata!\n");
		return EFI_EXIT(EFI_DEVICE_ERROR);
	}

	info->suffix = slot_suffix[idx];
	info->priority = ab_data.slot_info[idx].priority;
	info->successful = ab_data.slot_info[idx].successful_boot;
	info->tries_remaining = ab_data.slot_info[idx].tries_remaining;
	info->unbootable_reason = 0;

	return EFI_EXIT(EFI_SUCCESS);
}

static efi_status_t EFIAPI get_current_slot(struct efi_gbl_boot_control_protocol *this,
					    struct efi_gbl_slot_info *info)
{
	EFI_ENTRY("%p, %p", this, info);
	if (!this || !info) {
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	}

	efi_status_t status;

	int ret = current_slot();
	if (ret == -1) {
		log_err("Failed to get current slot!\n");
		return EFI_EXIT(EFI_DEVICE_ERROR);
	}

	status = get_slot_info(this, ret, info);

	return EFI_EXIT(status);
}

static efi_status_t EFIAPI set_active_slot(struct efi_gbl_boot_control_protocol *this,
					   u8 idx)
{
	AvbIOResult result;
	EFI_ENTRY("%p, %uc", this, idx);

	if (!this) {
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	}

	if (idx >= SLOT_NUM) {
		log_err("Invalid slot index!\n");
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	}

#ifdef CONFIG_VIRTUAL_AB_SUPPORT
	if (virtual_ab_update_is_merging()) {
		log_err("Can not switch slot while snapshot merge is in progress!\n");
		return EFI_EXIT(EFI_ACCESS_DENIED);
	}

	/* Only output a warning when the image is snapshoted. */
	if (virtual_ab_update_is_snapshoted())
		log_info("Warning: changing the active slot with a snapshot applied may cancel the update!\n");
	else
		log_info("Warning: Virtual A/B is enabled, switch slot may make the system fail to boot. \n");
#endif

	/* Make the slot as active */
	result = fsl_avb_ab_mark_slot_active(&fsl_avb_ab_ops, idx);
	if (result != AVB_IO_RESULT_OK) {
		log_err("Failed to set slot (%d) as active!\n", idx);
		return EFI_EXIT(EFI_DEVICE_ERROR);
	}

	return EFI_EXIT(EFI_SUCCESS);
}

static efi_status_t EFIAPI get_one_shot_boot_mode(struct efi_gbl_boot_control_protocol *this,
						  efi_gbl_one_shot_boot_mode *mode) {
	EFI_ENTRY("%p, %p", this, mode);

	if (!this || !mode)
		return EFI_EXIT(EFI_INVALID_PARAMETER);

	if (is_recovery_key_pressing())
		*mode = EFI_GBL_ONE_SHOT_BOOT_MODE_RECOVERY;

	return EFI_EXIT(EFI_SUCCESS);
}

static efi_status_t EFIAPI handle_loaded_os(struct efi_gbl_boot_control_protocol *this,
					    const efi_gbl_loaded_os *os) {
	EFI_ENTRY("%p, %p", this, os);

	return EFI_EXIT(EFI_UNSUPPORTED);
}

static struct efi_gbl_boot_control_protocol efi_gbl_boot_control = {
	.version = EFI_GBL_BOOT_CONTROL_PROTOCOL_REVISION,
	.get_slot_count = get_slot_count,
	.get_slot_info = get_slot_info,
	.get_current_slot = get_current_slot,
	.set_active_slot = set_active_slot,
	.get_one_shot_boot_mode = get_one_shot_boot_mode,
	.handle_loaded_os = handle_loaded_os,
};

efi_status_t efi_gbl_boot_control_register(void)
{
	efi_status_t ret = efi_add_protocol(efi_root, &efi_gbl_boot_control_guid,
					    &efi_gbl_boot_control);
	if (ret != EFI_SUCCESS) {
		log_err("Failed to install EFI_GBL_BOOT_CONTROL_PROTOCOL: 0x%lx\n", ret);
	}

	return ret;
}
