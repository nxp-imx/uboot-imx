/*
 * Copyright 2024 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <interface/secretkeeper/secretkeeper.h>
#include <trusty/trusty_ipc.h>

/*
 * Initialize SECRETKEEPER TIPC client. Returns one of trusty_err.
 *
 * @dev: trusty_ipc_dev
 */
int secretkeeper_tipc_init(struct trusty_ipc_dev* dev);

/*
 * Shutdown SECRETKEEPER TIPC client.
 */
void secretkeeper_tipc_shutdown(void);

/**
 * Retrieves the identity (public key) of the Secretkeeper implementation.
 * The key is represented as a CBOR-encoded COSE_key, as one of as a
 * PubKeyEd25519 / PubKeyECDSA256 / PubKeyECDSA384. See
 * https://cs.android.com/android/platform/superproject/main/+/main:hardware/interfaces/security/rkp/aidl/android/hardware/security/keymint/generateCertificateRequestV2.cddl
 * @identity_buf_size:          Size of the buffer pointed to by @identity_buf.
 * @identity_buf:               Pointer to a buffer to store the CBOR-encoded
 *                              public key.
 * @identity_size:              On return the actual size of the public key.
 */
int secretkeeper_get_identity(size_t identity_buf_size,
                              uint8_t identity_buf[],
                              size_t* identity_size);

/**
 * Retrive the secretkeeper from the secure os and populate it to the host DT.
 * @ fdt_addr: point to the start of Host DT.
 */
int trusty_populate_sk_key(void *fdt_addr);
