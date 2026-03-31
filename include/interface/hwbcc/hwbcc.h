/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Copyright 2025 NXP
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
