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

#ifndef __EFI_GBL_OS_CONFIG_H__
#define __EFI_GBL_OS_CONFIG_H__

#include <efi_api.h>
#include <efi_gbl_protocol_utils.h>

static const uint64_t EFI_GBL_OS_CONFIGURATION_PROTOCOL_REVISION = \
			GBL_PROTOCOL_REVISION(0, 2);

enum GBL_EFI_DEVICE_TREE_TYPE {
	// HLOS device tree.
	DEVICE_TREE,
	// HLOS device tree overlay.
	OVERLAY,
	// pVM device assignment overlay.
	PVM_DA_OVERLAY,
};

enum GBL_EFI_DEVICE_TREE_SOURCE {
	// Device tree loaded from boot partition.
	BOOT,
	// Device tree loaded from vendor_boot partition.
	VENDOR_BOOT,
	// Device tree loaded from dtbo partition.
	DTBO,
	// Device tree loaded from dtb partition.
	DTB,
};

struct efi_gbl_device_tree_metadata {
	// GblDeviceTreeSource
	u32 source;
	// GblDeviceTreeType
	u32 type;
	// Values are zeroed and must not be used in case of BOOT / VENDOR_BOOT source
	u32 id;
	u32 rev;
	u32 custom[4];
};

struct efi_gbl_verified_device_tree {
	struct efi_gbl_device_tree_metadata metadata;
	// Base device tree / overlay buffer (guaranteed to be 8-bytes aligned),
	// cannot be NULL. Device tree size can be identified by the header totalsize
	// field
	const void *device_tree;
	// Indicates whether this device tree (or overlay) must be included in the
	// final device tree. Set to true by a FW if this component must be used
	u8 selected;
};

struct efi_gbl_os_configuration_protocol {
	u64 revision;

	// Generates fixups for the bootconfig built by GBL.
	efi_status_t(EFIAPI *fixup_bootconfig)(
		struct efi_gbl_os_configuration_protocol *this,
		size_t size, /* in */
		const char *bootconfig, /* in */
		size_t *fixup_buffer_size, /* in-out */
		char *fixup /* out */
	);

	// Selects which device trees and overlays to use from those loaded by GBL.
	efi_status_t(EFIAPI *select_device_trees)(
		struct efi_gbl_os_configuration_protocol *this,
		size_t num_device_trees, /* in */
		struct efi_gbl_verified_device_tree *device_trees /* in-out */
	);

	// Selects FIT configuration to be used.
	efi_status_t(EFIAPI *select_fit_configuration)(
		struct efi_gbl_os_configuration_protocol *this,
		size_t fit_size, const uint8_t* fit,
		size_t metadata_size, const uint8_t* metadata,
		size_t* selected_configuration_offset);
};

extern const efi_guid_t efi_gbl_os_config_guid;

efi_status_t efi_gbl_os_config_register(void);

#endif /* __EFI_GBL_OS_CONFIG_H__ */
