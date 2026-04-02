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
 *
 */

#ifndef __EFI_GBL_FASTBOOT_TRANSPORT_H__
#define __EFI_GBL_FASTBOOT_TRANSPORT_H__

#include <efi_api.h>
#include <efi_gbl_protocol_utils.h>

typedef enum EFI_GBL_FASTBOOT_RX_MODE {
  SINGLE_PACKET = 0,
  FIXED_LENGTH,
} EFI_GBL_FASTBOOT_RX_MODE;

static const uint64_t EFI_GBL_FASTBOOT_TRANSPORT_PROTOCOL_REVISION =
    GBL_PROTOCOL_REVISION(0, 1);

typedef struct efi_gbl_fastboot_transport_protocol {
  uint64_t revision;
  const char* description;
  efi_status_t (EFIAPI *start)(struct efi_gbl_fastboot_transport_protocol* this);
  efi_status_t (EFIAPI *stop)(struct efi_gbl_fastboot_transport_protocol* this);
  efi_status_t (EFIAPI *receive)(struct efi_gbl_fastboot_transport_protocol* this,
                                 size_t* buffer_size, void* buffer,
                                 EFI_GBL_FASTBOOT_RX_MODE mode);
  efi_status_t (EFIAPI *send)(struct efi_gbl_fastboot_transport_protocol* this,
                              size_t* buffer_size, const void* buffer);
  efi_status_t (EFIAPI *flush)(struct efi_gbl_fastboot_transport_protocol* this);
} efi_gbl_fastboot_transport_protocol;

efi_status_t efi_gbl_fastboot_transport_register(void);

#endif  //__EFI_GBL_FASTBOOT_TRANSPORT_H__
