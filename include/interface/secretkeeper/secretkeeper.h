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
