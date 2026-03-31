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

#include <stdint.h>
#include <trusty/sysdeps.h>

/* Note: The definitive source for the message interface here is is in
 * trusty/user/app/secretkeeper/lib.rs (TIPC port details) and
 * system/secretkeeper/core/src/ta/bootloader.rs (message format).
 * This is a manual translation into C.
 */

#define SECRETKEEPER_BL_PORT "com.android.trusty.secretkeeper.bootloader"

/**
 * enum secretkeeper_cmd - Secretkeeper commands.
 * @SECRETKEEPER_RESPONSE_MARKER: Bit indicating that this is a response.
 * @SECRETKEEPER_CMD_GET_IDENTITY: Get the per-boot identity (public key) of
 *                                 Secretkeeper.
 */
enum secretkeeper_cmd {
    SECRETKEEPER_RESPONSE_MARKER = 0x1 << 31,
    SECRETKEEPER_CMD_GET_IDENTITY = 1,
};

/**
 * struct secretkeeper_req_hdr - Generic header for all Secretkeeper requests.
 * Note that all fields are stored in network byte order (big endian).
 * @cmd:       The command to be run. Commands are described in
 *             enum secretkeeper_cmd.
 */
struct secretkeeper_req_hdr {
    uint32_t cmd;
};

/**
 * struct secretkeeper_resp_hdr - Generic header for all Secretkeeper responses.
 * Note that all fields are stored in network byte order (big endian).
 * Any response payload immediately follows this struct.
 * @cmd:          Command identifier - %SECRETKEEPER_RESPONSE_MARKER or'ed with
 *                the command identifier of the corresponding request.
 * @error_code:   0 if the request succeeded, or an indication of how it failed.
 */
struct secretkeeper_resp_hdr {
    uint32_t cmd;
    uint32_t error_code;
};
