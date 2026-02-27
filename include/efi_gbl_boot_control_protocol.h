/*
 * Copyright (C) 2024 The Android Open Source Project
 *
 * Copyright 2025 NXP
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0 OR BSD-2-Clause-Patent
 *
 * You may choose to use or redistribute this file under
 *  (a) the Apache License, Version 2.0, or
 *  (b) the BSD 2-Clause Patent license.
 *
 * Unless you expressly elect the BSD-2-Clause-Patent terms, the Apache-2.0
 * terms apply by default.
 *
 * This project elects to use the BSD-2-Clause-Patent License.
 */

#ifndef __EFI_GBL_BOOT_CONTROL_H__
#define __EFI_GBL_BOOT_CONTROL_H__

#include <efi_api.h>
#include <efi_gbl_protocol_utils.h>

static const uint64_t EFI_GBL_BOOT_CONTROL_PROTOCOL_REVISION = \
			GBL_PROTOCOL_REVISION(0, 3);
extern const efi_guid_t efi_gbl_boot_control_guid;

EFI_ENUM(efi_gbl_unbootable_reason, uint8_t,
	 EFI_GBL_UNBOOTABLE_REASON_UNKNOWN_REASON,
	 EFI_GBL_UNBOOTABLE_REASON_NO_MORE_TRIES,
	 EFI_GBL_UNBOOTABLE_REASON_SYSTEM_UPDATE,
	 EFI_GBL_UNBOOTABLE_REASON_USER_REQUESTED,
	 EFI_GBL_UNBOOTABLE_REASON_VERIFICATION_FAILURE);

EFI_ENUM(efi_gbl_one_shot_boot_mode, uint32_t,
	 EFI_GBL_ONE_SHOT_BOOT_MODE_NONE,
	 EFI_GBL_ONE_SHOT_BOOT_MODE_BOOTLOADER,
	 EFI_GBL_ONE_SHOT_BOOT_MODE_RECOVERY);

typedef struct efi_gbl_slot_info {
	// One UTF-8 encoded single character
	uint32_t suffix;
	// Any value other than those explicitly enumerated in efi_gbl_unbootable_reason
	// will be interpreted as UNKNOWN_REASON.
	efi_gbl_unbootable_reason unbootable_reason;
	uint8_t priority;
	uint8_t tries_remaining;
	// Value of 1 if slot has successfully booted.
	uint8_t successful;
} efi_gbl_slot_info;

typedef struct efi_gbl_loaded_os {
	size_t kernel_size;
	uint64_t kernel;
	size_t ramdisk_size;
	uint64_t ramdisk;
	size_t device_tree_size;
	uint64_t device_tree;
	uint64_t reserved[8];
} efi_gbl_loaded_os;

typedef struct efi_memory_descriptor {
	uint32_t memory_type;
	uint32_t padding;
	uint64_t physical_start;
	uint64_t virtual_start;
	uint64_t number_of_pages;
	uint64_t attributes;
} efi_memory_descriptor;

struct efi_gbl_boot_control_protocol {
	uint64_t version;
	// Slot metadata query methods
	efi_status_t(EFIAPI * get_slot_count) (struct efi_gbl_boot_control_protocol *this, uint8_t *slot_count);
	efi_status_t(EFIAPI * get_slot_info) (struct efi_gbl_boot_control_protocol *this,
					      uint8_t index,
					      struct efi_gbl_slot_info *info);
	efi_status_t(EFIAPI * get_current_slot) (struct efi_gbl_boot_control_protocol *this,
						 struct efi_gbl_slot_info *info);
	// Slot metadata manipulation methods
	efi_status_t(EFIAPI * set_active_slot) (struct efi_gbl_boot_control_protocol *this,
						uint8_t index);
	// Boot control methods
	efi_status_t(EFIAPI *get_one_shot_boot_mode) (struct efi_gbl_boot_control_protocol *this,
							efi_gbl_one_shot_boot_mode *mode);
	efi_status_t(EFIAPI *handle_loaded_os) (struct efi_gbl_boot_control_protocol *this,
						const efi_gbl_loaded_os *os);
};

efi_status_t efi_gbl_boot_control_register(void);

#endif /* __EFI_GBL_BOOT_CONTROL_H__ */
