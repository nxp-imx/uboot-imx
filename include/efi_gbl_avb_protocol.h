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

#ifndef __EFI_GBL_AVB_PROTOCOL_H__
#define __EFI_GBL_AVB_PROTOCOL_H__

#include <efi_api.h>
#include <efi_gbl_protocol_utils.h>

static const uint64_t EFI_GBL_AVB_PROTOCOL_REVISION = \
	GBL_PROTOCOL_REVISION(0, 4);

typedef uint64_t efi_gbl_avb_device_status;
// Indicates device is unlocked.
static const efi_gbl_avb_device_status EFI_GBL_AVB_STATUS_UNLOCKED = 0x1 << 0;
// Indecated dm-verity error is occurred.
static const efi_gbl_avb_device_status EFI_GBL_AVB_STATUS_DM_VERITY_FAILED = 0x1 << 1;

// Indicates device is unlocked for critical operations.
static const efi_gbl_avb_device_status GBL_EFI_AVB_DEVICE_STATUS_UNLOCKED_CRITICAL = 0x1 << 2;

// Indicates the device bootloader can be unlocked.
static const efi_gbl_avb_device_status GBL_EFI_AVB_DEVICE_STATUS_UNLOCKABLE = 0x1 << 3;

// Os boot state color flags.
//
// https://source.android.com/docs/security/features/verifiedboot/boot-flow#communicating-verified-boot-state-to-users
typedef uint64_t efi_gbl_avb_boot_state_color;
static const efi_gbl_avb_boot_state_color EFI_GBL_AVB_BOOT_COLOR_RED = 0x1 << 0;
static const efi_gbl_avb_boot_state_color EFI_GBL_AVB_BOOT_COLOR_RED_EIO = 0x1 << 1;
static const efi_gbl_avb_boot_state_color EFI_GBL_AVB_BOOT_COLOR_ORANGE = 0x1 << 2;
static const efi_gbl_avb_boot_state_color EFI_GBL_AVB_BOOT_COLOR_YELLOW = 0x1 << 3;
static const efi_gbl_avb_boot_state_color EFI_GBL_AVB_BOOT_COLOR_GREEN = 0x1 << 4;

// Vbmeta key validation status.
//
// https://source.android.com/docs/security/features/verifiedboot/boot-flow#locked-devices-with-custom-root-of-trust
EFI_ENUM(efi_gbl_avb_key_validation_status, uint32_t,
	 EFI_GBL_AVB_KEY_VALIDATION_STATUS_INVALID,
         EFI_GBL_AVB_KEY_VALIDATION_STATUS_VALID_CUSTOM_KEY,
         EFI_GBL_AVB_KEY_VALIDATION_STATUS_VALID);

typedef uint64_t efi_gbl_avb_partition_flags;
static const efi_gbl_avb_partition_flags EFI_GBL_AVB_PARTITION_OPTIONAL = 0x1 << 0;

EFI_ENUM(efi_gbl_avb_lock_type, uint8_t,
	 EFI_GBL_AVB_LOCK_TYPE_DEVICE,
	 EFI_GBL_AVB_LOCK_TYPE_CRITICAL);

EFI_ENUM(efi_gbl_avb_lock_state, uint8_t,
	 EFI_GBL_AVB_LOCK_STATE_UNLOCKED,
	 EFI_GBL_AVB_LOCK_STATE_LOCKED);

typedef struct {
  // On input - `base_name` buffer size
  // On output - actual `base_name` length
  size_t base_name_len;
  char* base_name;
  efi_gbl_avb_partition_flags flags;
} efi_gbl_avb_partition;

typedef struct {
  // UTF-8, null terminated
  const char* base_name;
  size_t data_size;
  const uint8_t* data;
} efi_gbl_avb_loaded_partition;

typedef struct {
  // UTF-8, null terminated
  const char* base_partition_name;
  // UTF-8, null terminated
  const char* key;
  // Excluding null terminator
  size_t value_size;
  const uint8_t* value;
} efi_gbl_avb_property;

typedef struct {
	// efi_gbl_avb_boot_state_color
	efi_gbl_avb_boot_state_color color;
	// Pointer to nul-terminated ASCII hex digest calculated by libavb. May be
	// null in case of verification failed (RED boot state color).
	uint8_t* digest;
	size_t num_partitions;
	const efi_gbl_avb_loaded_partition *partitions;
	size_t num_properties;
	const efi_gbl_avb_property *properties;
	uint64_t reserved[8];
} efi_gbl_avb_verification_result;

typedef struct efi_gbl_avb_protocol {
	uint64_t revision;

	efi_status_t (EFIAPI *read_partitions_to_verify)(struct efi_gbl_avb_protocol *this,
							 /* in-out */ size_t *num_partitions,
							 /* in-out */ efi_gbl_avb_partition *partitions);

	efi_status_t (EFIAPI *read_device_status)(struct efi_gbl_avb_protocol *this,
						       /* out */ efi_gbl_avb_device_status *status_flags);

	efi_status_t (EFIAPI *validate_vbmeta_public_key)(struct efi_gbl_avb_protocol *this,
							    /* in */ size_t public_key_length,
							    /* in */ const uint8_t *public_key_data,
							    /* in */ size_t public_key_metadata_length,
							    /* in */ const uint8_t *public_key_metadata,
							    /* out efi_gbl_avb_key_validation_status*/
							    efi_gbl_avb_key_validation_status *validation_status);

	efi_status_t (EFIAPI *read_rollback_index)(struct efi_gbl_avb_protocol *this,
						   /* in */ size_t index_location,
						   /* out */ uint64_t *rollback_index);

	efi_status_t (EFIAPI *write_rollback_index)(struct efi_gbl_avb_protocol *this,
						    /* in */ size_t index_location,
						    /* in */ uint64_t rollback_index);

	efi_status_t (EFIAPI *read_persistent_value)(struct efi_gbl_avb_protocol *this,
						     /* in */ const uint8_t *name,
						     /* in-out */ size_t *value_size,
						     /* out */ uint8_t *value);

	efi_status_t (EFIAPI *write_persistent_value)(struct efi_gbl_avb_protocol *this,
						      /* in */ const uint8_t *name,
						      /* in */ size_t value_size,
						      /* in */ const uint8_t *value);

	efi_status_t (EFIAPI *handle_verification_result)(struct efi_gbl_avb_protocol *this,
							  /* in */ const efi_gbl_avb_verification_result *result);
	efi_status_t (EFIAPI *write_lock_state)(struct efi_gbl_avb_protocol *this, efi_gbl_avb_lock_type type,
						efi_gbl_avb_lock_state state);
} efi_gbl_avb_protocol;

efi_status_t efi_gbl_avb_register(void);

#endif  //__EFI_GBL_AVB_PROTOCOL_H__
