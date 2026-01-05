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

#ifndef __EFI_GBL_IMAGE_LOADING_H__
#define __EFI_GBL_IMAGE_LOADING_H__

#include <efi_api.h>
#include <part_efi.h>
#include <stddef.h>
#include <efi_gbl_protocol_utils.h>

static const uint64_t EFI_GBL_IMAGE_LOADING_PROTOCOL_REVISION = \
	GBL_PROTOCOL_REVISION(0, 1);

#define PARTITION_NAME_LEN_U16 36

//******************************************************
// GBL reserved image types
//******************************************************
// Buffer for loading, verifying and fixing up OS images.
#define GBL_IMAGE_TYPE_OS_LOAD u"os_load"
// Buffer for use as finalized kernel load buffer.
#define GBL_IMAGE_TYPE_KERNEL_LOAD u"kernel_load"
// Buffer for use as finalized ramdisk load buffer.
#define GBL_IMAGE_TYPE_RAMDISK_LOAD u"ramdisk_load"
// Buffer for use as finalized fdt load buffer.
#define GBL_IMAGE_TYPE_FDT_LOAD u"fdt_load"
// Buffer for use as fastboot download buffer.
#define GBL_IMAGE_TYPE_FASTBOOT u"fastboot"
// Buffer reserved for pvmfw binary and configuration (must be 4KiB-aligned).
#define GBL_IMAGE_TYPE_PVMFW_DATA u"pvmfw_data"

extern const efi_guid_t efi_gbl_image_loading_protocol_guid;

typedef struct gbl_image_info {
	efi_char16_t image_type[PARTITION_NAME_LEN_U16];
	size_t size_bytes;
} gbl_image_info;

typedef struct gbl_image_buffer {
	void *memory;
	size_t size_bytes;
} gbl_image_buffer;

typedef struct efi_image_loading_protocol {
	// Currently must contain 0x00010000
	u64 revision;

	efi_status_t(EFIAPI *get_buffer)(struct efi_image_loading_protocol *,
					 const gbl_image_info * /* in param */,
					 gbl_image_buffer * /* in-out param */);
} efi_image_loading_protocol;

efi_status_t efi_gbl_image_loading_register(void);

#endif /* __EFI_GBL_IMAGE_LOADING_H__ */
