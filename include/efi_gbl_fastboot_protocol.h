/*
 * Copyright (C) 2024 The Android Open Source Project
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

#ifndef __EFI_GBL_FASTBOOT_PROTOCOL_H__
#define __EFI_GBL_FASTBOOT_PROTOCOL_H__

#include <efi_api.h>
#include <efi_gbl_protocol_utils.h>

#define GBL_EFI_FASTBOOT_SERIAL_NUMBER_MAX_LEN_UTF8 32

// Callback function pointer passed to efi_gbl_fastboot_protocol.get_var_all.
//
// context: Caller specific context.
// args: An array of NULL-terminated strings that contains the variable name
//       followed by additional arguments if any.
// val: A NULL-terminated string representing the value.
typedef void (*get_var_all_callback)(void* context, const char* const* args,
                                     size_t num_args, const char* val);

EFI_ENUM(efi_gbl_fastboot_message_type, uint32_t,
	 EFI_GBL_FASTBOOT_MESSAGE_TYPE_OKAY,
	 EFI_GBL_FASTBOOT_MESSAGE_TYPE_FAIL,
	 EFI_GBL_FASTBOOT_MESSAGE_TYPE_INFO);

typedef efi_status_t (*fastboot_message_sender)(void* context,
                                                efi_gbl_fastboot_message_type msg_type,
                                                const char* msg, size_t msg_len);

static const uint64_t EFI_GBL_FASTBOOT_PROTOCOL_REVISION =
    GBL_PROTOCOL_REVISION(0, 7);

static const size_t GBL_EFI_FASTBOOT_PARTITION_TYPE_BUF_LEN = 56;

EFI_ENUM(efi_gbl_fastboot_erase_action, uint32_t,
	 // Treats the partition as a physical on disk partition and erases it.
	 EFI_GBL_FASTBOOT_ERASE_ACTION_ERASE_AS_PHYSICAL_PARTITION,
	 // Ignores the partition.
	 EFI_GBL_FASTBOOT_ERASE_ACTION_NOOP,);

EFI_ENUM(efi_gbl_fastboot_cmd_exec_result, uint32_t,
	 EFI_GBL_FASTBOOT_COMMAND_EXEC_RESULT_PROHIBITED,
	 EFI_GBL_FASTBOOT_COMMAND_EXEC_RESULT_DEFAULT_IMPL,
	 EFI_GBL_FASTBOOT_COMMAND_EXEC_RESULT_CUSTOM_IMPL);

typedef struct efi_gbl_fastboot_protocol {
  uint64_t revision;
  // Null-terminated UTF-8 encoded string
  char serial_number[GBL_EFI_FASTBOOT_SERIAL_NUMBER_MAX_LEN_UTF8];

  // Fastboot variable methods
  efi_status_t (EFIAPI *get_var)(struct efi_gbl_fastboot_protocol* this,
                                 const char* const* args, size_t num_args, uint8_t* out,
                                 size_t* out_size);
  efi_status_t (EFIAPI *get_var_all)(struct efi_gbl_fastboot_protocol *this, void* ctx,
                                     get_var_all_callback cb);

  // Fastboot get_staged backend
  efi_status_t (EFIAPI *get_staged)(struct efi_gbl_fastboot_protocol* this, uint8_t* out,
                                    size_t* out_size, size_t* out_remain);

  // Misc methods
  efi_status_t (EFIAPI *vendor_erase)(struct efi_gbl_fastboot_protocol* this,
                                      const char* part_name, efi_gbl_fastboot_erase_action* action);
  efi_status_t (EFIAPI *command_exec) (struct efi_gbl_fastboot_protocol* this,
				       size_t num_args, const char* const* args,
				       size_t download_data_used_len,
				       uint8_t* download_data,
				       size_t download_data_full_size,
				       efi_gbl_fastboot_cmd_exec_result *implementation,
				       fastboot_message_sender sender, void* ctx);

  efi_status_t (EFIAPI *get_partition_type)(struct efi_gbl_fastboot_protocol* this,
					    const uint8_t* part_name, uint8_t* part_type,
					    size_t* part_type_len);
} efi_gbl_fastboot_protocol;

efi_status_t efi_gbl_fastboot_register(void);

#endif  // __EFI_GBL_FASTBOOT_PROTOCOL_H__
