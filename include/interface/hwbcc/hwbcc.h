/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Copyright 2025 NXP
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

#include <trusty/sysdeps.h>

#define HWBCC_PORT "com.android.trusty.hwbcc"

/**
 * enum hwbcc_cmd - BCC service commands.
 * @HWBCC_CMD_REQ_SHIFT: Bitshift of the command index.
 * @HWBCC_CMD_RESP_BIT:  Bit indicating that this is a response.
 * @HWBCC_CMD_GET_DICE_ARTIFACTS: Get the DICE artifacts derived for a
 * child node of Trusty in the DICE chain in non-secure world (e.g. ABL).
 * @HWBCC_CMD_NS_DEPRIVILEGE: Deprivilege hwbcc from serving calls
 * to non-secure clients.
 * @HWBCC_CMD_COMMIT_CODE_HASH: Commit the code hash to the secure world.
 */
enum hwbcc_cmd {
    HWBCC_CMD_REQ_SHIFT = 1,
    HWBCC_CMD_RESP_BIT = 1,
    HWBCC_CMD_GET_DICE_ARTIFACTS = 3 << HWBCC_CMD_REQ_SHIFT,
    HWBCC_CMD_NS_DEPRIVILEGE = 4 << HWBCC_CMD_REQ_SHIFT,
    HWBCC_CMD_COMMIT_CODE_HASH = 8 << HWBCC_CMD_REQ_SHIFT,
};

/**
 * struct hwbcc_req_hdr - Generic header for all hwbcc requests.
 * @cmd:       The command to be run. Commands are described in hwbcc_cmd.
 * @test_mode: Whether or not RKP is making a test request.
 * @context:   Device specific context information passed in by the client.
 *             This is opaque to the generic Trusty code. This is required
 *             to make decisions about device specific behavior in the
 *             implementations of certain hwbcc interface methods. For e.g.
 *             w.r.t get_dice_artifacts, context can supply information
 *             about which secure/non-secure DICE child node is requesting
 *             the dice_artifacts and the implementations can use such
 *             information to derive dice artifacts specific to the
 *             particular child node.
 */
struct hwbcc_req_hdr {
    uint32_t cmd;
    uint32_t test_mode;
    uint64_t context;
};

#define MIN_CODE_HASH 32
#define MAX_CODE_HASH 64
struct hwbcc_req_commit_hash {
    uint32_t hash_size;
    uint8_t code_hash[MAX_CODE_HASH];
};

/**
 * struct hwbcc_resp_hdr - Generic header for all hwbcc requests.
 * @cmd:          Command identifier - %HWBCC_CMD_RSP_BIT or'ed with the command
 *                identifier of the corresponding request.
 * @status:       Whether or not the cmd succeeded, or how it failed.
 * @payload_size: Size of response payload that follows this struct.
 */
struct hwbcc_resp_hdr {
    uint32_t cmd;
    int32_t status;
    uint32_t payload_size;
};

#define HWBCC_MAX_RESP_PAYLOAD_SIZE 1024
