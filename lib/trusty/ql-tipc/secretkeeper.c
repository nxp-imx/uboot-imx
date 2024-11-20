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

#include <trusty/secretkeeper.h>
#include <trusty/trusty_ipc.h>
#include <trusty/util.h>
#include <linux/libfdt.h>
#include <fdt_support.h>

static struct trusty_ipc_chan secretkeeper_chan;
static bool initialized;

int secretkeeper_tipc_init(struct trusty_ipc_dev* dev) {
    trusty_assert(dev);
    trusty_assert(!initialized);
    trusty_ipc_chan_init(&secretkeeper_chan, dev);

    trusty_debug(
            "In secretkeeper_tipc_init: connecting to secretkeeper bootloader service.\n");
    int rc = trusty_ipc_connect(&secretkeeper_chan, SECRETKEEPER_BL_PORT,
                                true /*wait*/);
    if (rc < 0) {
        trusty_error(
                "In secretkeeper_tipc_init:: failed (%d) to connect to '%s'.\n",
                rc, SECRETKEEPER_BL_PORT);
        return rc;
    }
    initialized = true;
    return TRUSTY_ERR_NONE;
}

void secretkeeper_tipc_shutdown(void) {
    if (!initialized) {
        return;
    }
    trusty_ipc_close(&secretkeeper_chan);
    initialized = false;
}

static int send_header_only_request(struct secretkeeper_req_hdr* hdr,
                                    size_t hdr_size) {
    int num_iovec = 1;

    if (!initialized) {
        trusty_error("%s: SecretKeeper client not initialized\n", __func__);
        return TRUSTY_ERR_GENERIC;
    }

    struct trusty_ipc_iovec req_iov = {.base = hdr, .len = hdr_size};
    return trusty_ipc_send(&secretkeeper_chan, &req_iov, num_iovec, true);
}

static int read_response_with_data(struct secretkeeper_req_hdr* hdr,
                                   uint8_t* buf,
                                   size_t buf_size,
                                   size_t* out_size) {
    struct secretkeeper_resp_hdr resp_hdr = {};

    trusty_assert(buf);
    trusty_assert(out_size);

    int num_iovec = 2;
    struct trusty_ipc_iovec resp_iovecs[2] = {
            {.base = &resp_hdr, .len = sizeof(resp_hdr)},
            {.base = buf, .len = buf_size},
    };

    int rc = trusty_ipc_recv(&secretkeeper_chan, resp_iovecs, num_iovec, true);
    if (rc < 0) {
        trusty_error("Secretkeeper: Failure on receiving response: %d\n", rc);
        return rc;
    }

    size_t bytes = rc;
    if (bytes < sizeof(resp_hdr)) {
        trusty_error("Secretkeeper: Invalid response size (%d).\n", rc);
        return TRUSTY_ERR_GENERIC;
    }

    if (resp_hdr.cmd != (hdr->cmd | htonl(SECRETKEEPER_RESPONSE_MARKER))) {
        trusty_error("Secretkeeper: Unknown response cmd: %x\n",
                     ntohl(resp_hdr.cmd));
        return TRUSTY_ERR_GENERIC;
    }

    if (resp_hdr.error_code != 0) {
        trusty_error("Secretkeeper: Error code (%d) is not zero.\n",
                     ntohl(resp_hdr.error_code));
        return TRUSTY_ERR_GENERIC;
    }

    *out_size = bytes - sizeof(resp_hdr);
    return rc;
}

int secretkeeper_get_identity(size_t identity_buf_size,
                              uint8_t identity_buf[],
                              size_t* identity_size) {
    trusty_assert(dice_artifacts);
    trusty_assert(dice_artifacts_size);

    struct secretkeeper_req_hdr hdr;
    hdr.cmd = htonl(SECRETKEEPER_CMD_GET_IDENTITY);

    int rc = send_header_only_request(&hdr, sizeof(hdr));

    if (rc < 0) {
        trusty_error(
                "In secretkeeper_get_identity: failed (%d) to send request to Secretkeeper.\n",
                rc);
        return rc;
    }

    rc = read_response_with_data(&hdr, identity_buf, identity_buf_size,
                                 identity_size);

    if (rc < 0) {
        trusty_error(
                "In secretkeeper_get_identity: failed (%d) to read the response.\n",
                rc);
        return rc;
    }

    return TRUSTY_ERR_NONE;
}

#define SK_KEY_SIZE (128)
int trusty_populate_sk_key(void *fdt_addr) {
    uint8_t sk_key[SK_KEY_SIZE] = {0};
    size_t out_key_size = 0;
    int node_offset = 0;
    int ret = 0;

    /* Retrive the key from secure os */
    ret = secretkeeper_get_identity(sizeof(sk_key), sk_key, &out_key_size);
    if (ret != TRUSTY_ERR_NONE || out_key_size > sizeof(sk_key)) {
        trusty_error("Failed to get secretkeeper key (err = %d)!\n", ret);
        return ret;
    }

    /* Now populate the key to device-tree */
    ret = fdt_increase_size(fdt_addr, SK_KEY_SIZE);
    if (ret != 0) {
        trusty_error("Failed to increase the fdt size! ret: %d\n", ret);
        return ret;
    }

    node_offset = fdt_path_offset(fdt_addr, "/avf/reference/avf");
    if (node_offset < 0) {
        trusty_error("Failed to find avf fdt path!\n");
        return node_offset;
    }

    ret = fdt_setprop(fdt_addr, node_offset, "secretkeeper_public_key", sk_key, out_key_size);
    if (ret != 0) {
        trusty_error("Failed to set avf property!, err: %d\n", ret);
    }

    return ret;
}
