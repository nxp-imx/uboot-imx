/*
 * Copyright 2024 The Android Open Source Project
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
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
