/*
 * Copyright (C) 2025 The Android Open Source Project
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
 *
 */

#ifndef __EFI_GBL_BOOT_MEMORY_PROTOCOL_H__
#define __EFI_GBL_BOOT_MEMORY_PROTOCOL_H__

#include <efi_api.h>
#include <efi_gbl_protocol_utils.h>

static const uint64_t GBL_EFI_BOOT_MEMORY_PROTOCOL_REVISION =
    GBL_PROTOCOL_REVISION(0, 1);

typedef enum GBL_EFI_BOOT_BUFFER_TYPE {
  GENERAL_LOAD,
  KERNEL,
  RAMDISK,
  FDT,
  PVMFW_DATA,
  FASTBOOT_DOWNLOAD,
} GBL_EFI_BOOT_BUFFER_TYPE;

typedef enum GBL_EFI_PARTITION_BUFFER_FLAG {
  PRELOADED = 1 << 0,
} GBL_EFI_PARTITION_BUFFER_FLAG;

typedef struct efi_gbl_boot_memory_protocol {
  uint64_t revision;
  efi_status_t (EFIAPI *get_partition_buffer)(struct efi_gbl_boot_memory_protocol *this,
                                     /* in */ const char *base_name,
                                    /* out */ size_t* size,
                                    /* out */ void** addr,
                                    /* out */ GBL_EFI_PARTITION_BUFFER_FLAG *flag);
  efi_status_t (EFIAPI *sync_partition_buffer)(struct efi_gbl_boot_memory_protocol *this,
                                      /* in */ bool sync_preloaded);
  efi_status_t (EFIAPI *get_boot_buffer)(struct efi_gbl_boot_memory_protocol *this,
                                /* in */ GBL_EFI_BOOT_BUFFER_TYPE buf_type,
                               /* out */ size_t* size,
                               /* out */ void** addr);
} efi_gbl_boot_memory_protocol;

efi_status_t efi_gbl_boot_memory_register(void);

#endif  //__EFI_GBL_BOOT_MEMORY_PROTOCOL_H__
