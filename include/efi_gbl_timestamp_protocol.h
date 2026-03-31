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
 */

#ifndef __EFI_GBL_TIMESTAMP_PROTOCOL_H__
#define __EFI_GBL_TIMESTAMP_PROTOCOL_H__

#include <efi_api.h>
#include <efi_gbl_protocol_utils.h>

typedef struct efi_timestamp_properties {
  uint64_t frequency;
  uint64_t end_value;
} efi_timestamp_properties_t;

typedef struct efi_gbl_timestamp_protocol {
  uint64_t (*get_timestamp)(void);
  efi_status_t (EFIAPI *get_properties)(efi_timestamp_properties_t *properties);
} efi_gbl_timestamp_protocol_t;

efi_status_t efi_gbl_timestamp_register(void);

#endif  //__EFI_GBL_TIMESTAMP_PROTOCOL_H__
