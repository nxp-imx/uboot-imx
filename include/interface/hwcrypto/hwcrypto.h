/*
 * Copyright (C) 2016 The Android Open Source Project
 * Copyright NXP 2018
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
 *
 */

#ifndef TRUSTY_INTERFACE_HWCRYPTO_H_
#define TRUSTY_INTERFACE_HWCRYPTO_H_

#include <trusty/sysdeps.h>

#define HWCRYPTO_PORT "com.android.trusty.hwcrypto"
#define HWCRYPTO_MAX_BUFFER_LENGTH 2048

enum hwcrypto_command {
    HWCRYPTO_REQ_SHIFT = 1,
    HWCRYPTO_RESP_BIT  = 1,

    HWCRYPTO_HASH            = (1 << HWCRYPTO_REQ_SHIFT),
    HWCRYPTO_ENCAP_BLOB      = (2 << HWCRYPTO_REQ_SHIFT),
    HWCRYPTO_GEN_RNG         = (3 << HWCRYPTO_REQ_SHIFT),
    HWCRYPTO_GEN_BKEK        = (4 << HWCRYPTO_REQ_SHIFT),
    HWCRYPTO_LOCK_BOOT_STATE = (5 << HWCRYPTO_REQ_SHIFT),
};

/**
 * enum hwcrypto_error - error codes for HWCRYPTO protocol
 * @HWCRYPTO_ERROR_NONE:         All OK
 * @HWCRYPTO_ERROR_INVALID:      Invalid input
 * @HWCRYPTO_ERROR_INTERNAL:     Error occurred during an operation in Trusty
 */
enum hwcrypto_error {
    HWCRYPTO_ERROR_NONE     = 0,
    HWCRYPTO_ERROR_INVALID  = 1,
    HWCRYPTO_ERROR_INTERNAL = 2,
};

enum hwcrypto_hash_algo {
    SHA1 = 0,
    SHA256
};
/**
 * hwcrypto_message - Serial header for communicating with hwcrypto server
 * @cmd:     the command. Payload must be a serialized buffer of the
 *           corresponding request object.
 * @result:  resulting error code for message, one of hwcrypto_error.
 * @payload: start of the serialized command specific payload
 */
struct hwcrypto_message {
    uint32_t cmd;
    uint32_t result;
    uint8_t payload[0];
};

/**
 * hwcrypto_hash_msg - Serial header for communicating with hwcrypto server
 * @in_addr:  start address of the input buf.
 * @in_len:   size of the input buf.
 * @out_addr: start addrss of the output buf.
 * @out_len:  size of the output buf.
 * @algo:     hash algorithm expect to use.
 */
typedef struct hwcrypto_hash_msg {
    uint32_t in_addr;
    uint32_t in_len;
    uint32_t out_addr;
    uint32_t out_len;
    enum hwcrypto_hash_algo algo;
} hwcrypto_hash_msg;

/**
 * @plain_pa:  physical start address of the plain blob buf.
 * @plain_size:   size of the plain blob.
 * @blob: physical start addrss of the output buf.
 */
typedef struct hwcrypto_blob_msg {
    uint32_t plain_pa;
    uint32_t plain_size;
    uint32_t blob_pa;
}hwcrypto_blob_msg;

/**
 * @buf:  physical start address of the output rng buf.
 * @len:  size of required rng.
 */
typedef struct hwcrypto_rng_msg {
    uint32_t buf;
    uint32_t len;
}hwcrypto_rng_msg;

/**
 * @buf:  physical start address of the output bkek buf.
 * @len:  size of required rng.
 */
typedef struct hwcrypto_bkek_msg {
    uint32_t buf;
    uint32_t len;
}hwcrypto_bkek_msg;
#endif /* TRUSTY_INTERFACE_HWCRYPTO_H_ */
