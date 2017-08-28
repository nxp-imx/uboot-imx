/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include <trusty/keymaster.h>
#include <trusty/rpmb.h>
#include <trusty/trusty_ipc.h>
#include <trusty/util.h>

#define LOCAL_LOG 0

static struct trusty_ipc_chan km_chan;
static bool initialized;
static int trusty_km_version = 2;

static int km_send_request(struct keymaster_message *msg, void *req,
                           size_t req_len)
{
    int num_iovecs = req ? 2 : 1;

    struct trusty_ipc_iovec req_iovs[2] = {
        { .base = msg, .len = sizeof(*msg) },
        { .base = req, .len = req_len },
    };

    return trusty_ipc_send(&km_chan, req_iovs, num_iovecs, true);
}

static int km_read_response(struct keymaster_message *msg, uint32_t cmd,
                            void *resp, size_t resp_len)
{
    int rc = TRUSTY_ERR_GENERIC;
    struct trusty_ipc_iovec resp_iovs[2] = {
        { .base = msg, .len = sizeof(*msg) },
        { .base = resp, .len = resp_len },
    };

    rc = trusty_ipc_recv(&km_chan, resp_iovs, resp ? 2 : 1, true);
    if (rc < 0) {
        trusty_error("failed (%d) to recv response\n", rc);
        return rc;
    }
    if ((msg->cmd & ~(KEYMASTER_STOP_BIT)) != (cmd | KEYMASTER_RESP_BIT)) {
        trusty_error("malformed response\n");
        return TRUSTY_ERR_GENERIC;
    }

    return rc;
}

static int km_do_tipc(uint32_t cmd, void *req, uint32_t req_len,
                      bool handle_rpmb)
{
    int rc = TRUSTY_ERR_GENERIC;
    struct keymaster_message msg = { .cmd = cmd };
    struct km_no_response resp;

    rc = km_send_request(&msg, req, req_len);
    if (rc < 0) {
        trusty_error("%s: failed (%d) to send km request\n", __func__, rc);
        return rc;
    }

    if (handle_rpmb) {
        /* handle any incoming RPMB requests */
        rc = rpmb_storage_proxy_poll();
        if (rc < 0) {
            trusty_error("%s: failed (%d) to get RPMB requests\n", __func__,
                         rc);
            return rc;
        }
    }

    rc = km_read_response(&msg, cmd, &resp, sizeof(resp));
    if (rc < 0) {
        trusty_error("%s: failed (%d) to read km response\n", __func__, rc);
        return rc;
    }
    return resp.error;
}

static int32_t MessageVersion(uint8_t major_ver, uint8_t minor_ver,
                              uint8_t subminor_ver) {
    int32_t message_version = -1;
    switch (major_ver) {
    case 0:
        message_version = 0;
        break;
    case 1:
        switch (minor_ver) {
        case 0:
            message_version = 1;
            break;
        case 1:
            message_version = 2;
            break;
        }
        break;
    case 2:
        message_version = 3;
        break;
    }
    return message_version;
}

static int km_get_version(int32_t *version)
{
    int rc = TRUSTY_ERR_GENERIC;
    struct keymaster_message msg = { .cmd = KM_GET_VERSION };
    struct km_get_version_resp resp;

    rc = km_send_request(&msg, NULL, 0);
    if (rc < 0) {
        trusty_error("failed to send km version request", rc);
        return rc;
    }

    rc = km_read_response(&msg, KM_GET_VERSION, &resp, sizeof(resp));
    if (rc < 0) {
        trusty_error("%s: failed (%d) to read km response\n", __func__, rc);
        return rc;
    }

    *version = MessageVersion(resp.major_ver, resp.minor_ver,
                              resp.subminor_ver);
    return rc;
}

int km_tipc_init(struct trusty_ipc_dev *dev)
{
    int rc = TRUSTY_ERR_GENERIC;

    trusty_assert(dev);

    trusty_ipc_chan_init(&km_chan, dev);
    trusty_debug("Connecting to Keymaster service\n");

    /* connect to km service and wait for connect to complete */
    rc = trusty_ipc_connect(&km_chan, KEYMASTER_PORT, true);
    if (rc < 0) {
        trusty_error("failed (%d) to connect to '%s'\n", rc, KEYMASTER_PORT);
        return rc;
    }

    int32_t version = -1;
    rc = km_get_version(&version);
    if (rc < 0) {
        trusty_error("failed (%d) to get keymaster version\n", rc);
        return rc;
    }
    if (version < trusty_km_version) {
        trusty_error("keymaster version mismatch. Expected %d, received %d\n",
                     trusty_km_version, version);
        return TRUSTY_ERR_GENERIC;
    }

    return TRUSTY_ERR_NONE;
}

void km_tipc_shutdown(struct trusty_ipc_dev *dev)
{
    if (!initialized)
        return;
    /* close channel */
    trusty_ipc_close(&km_chan);

    initialized = false;
}

/**
 * Appends |data_len| bytes at |data| to |buf|. Performs no bounds checking,
 * assumes sufficient memory allocated at |buf|. Returns |buf| + |data_len|.
 */
static uint8_t *append_to_buf(uint8_t *buf, const void *data, size_t data_len)
{
    if (data && data_len) {
        trusty_memcpy(buf, data, data_len);
    }
    return buf + data_len;
}

/**
 * Appends |val| to |buf|. Performs no bounds checking. Returns |buf| +
 * sizeof(uint32_t).
 */
static uint8_t *append_uint32_to_buf(uint8_t *buf, uint32_t val)
{
    return append_to_buf(buf, &val, sizeof(val));
}

/**
 * Appends a sized buffer to |buf|. First appends |data_len| to |buf|, then
 * appends |data_len| bytes at |data| to |buf|. Performs no bounds checking.
 * Returns |buf| + sizeof(uint32_t) + |data_len|.
 */
static uint8_t *append_sized_buf_to_buf(uint8_t *buf, const uint8_t *data,
                                        uint32_t data_len)
{
    buf = append_uint32_to_buf(buf, data_len);
    return append_to_buf(buf, data, data_len);
}

int km_boot_params_serialize(const struct km_boot_params *params, uint8_t** out,
                             uint32_t *out_size)
{
    uint8_t *tmp;

    if (!out || !params || !out_size) {
        return TRUSTY_ERR_INVALID_ARGS;
    }
    *out_size = (sizeof(params->os_version) + sizeof(params->os_patchlevel) +
                 sizeof(params->device_locked) +
                 sizeof(params->verified_boot_state) +
                 sizeof(params->verified_boot_key_hash_size) +
                 sizeof(params->verified_boot_hash_size) +
                 params->verified_boot_key_hash_size +
                 params->verified_boot_hash_size);
    *out = trusty_calloc(*out_size, 1);
    if (!*out) {
        return TRUSTY_ERR_NO_MEMORY;
    }

    tmp = append_uint32_to_buf(*out, params->os_version);
    tmp = append_uint32_to_buf(tmp, params->os_patchlevel);
    tmp = append_uint32_to_buf(tmp, params->device_locked);
    tmp = append_uint32_to_buf(tmp, params->verified_boot_state);
    tmp = append_sized_buf_to_buf(tmp, params->verified_boot_key_hash,
                                  params->verified_boot_key_hash_size);
    tmp = append_sized_buf_to_buf(tmp, params->verified_boot_hash,
                                  params->verified_boot_hash_size);

    return TRUSTY_ERR_NONE;
}

int km_attestation_data_serialize(const struct km_attestation_data *data,
                                 uint8_t** out, uint32_t *out_size)
{
    uint8_t *tmp;

    if (!out || !data || !out_size) {
        return TRUSTY_ERR_INVALID_ARGS;
    }
    *out_size = (sizeof(data->algorithm) + sizeof(data->data_size) +
                 data->data_size);
    *out = trusty_calloc(*out_size, 1);
    if (!*out) {
        return TRUSTY_ERR_NO_MEMORY;
    }

    tmp = append_uint32_to_buf(*out, data->algorithm);
    tmp = append_sized_buf_to_buf(tmp, data->data, data->data_size);

    return TRUSTY_ERR_NONE;
}

int trusty_set_boot_params(uint32_t os_version, uint32_t os_patchlevel,
                           keymaster_verified_boot_t verified_boot_state,
                           bool device_locked,
                           const uint8_t *verified_boot_key_hash,
                           uint32_t verified_boot_key_hash_size,
                           const uint8_t *verified_boot_hash,
                           uint32_t verified_boot_hash_size)
{
    struct km_boot_params params = {
        .os_version = os_version,
        .os_patchlevel = os_patchlevel,
        .device_locked = (uint32_t)device_locked,
        .verified_boot_state = (uint32_t)verified_boot_state,
        .verified_boot_key_hash_size = verified_boot_key_hash_size,
        .verified_boot_key_hash = verified_boot_key_hash,
        .verified_boot_hash_size = verified_boot_hash_size,
        .verified_boot_hash = verified_boot_hash
    };
    uint8_t *req = NULL;
    uint32_t req_size = 0;
    int rc = km_boot_params_serialize(&params, &req, &req_size);

    if (rc < 0) {
        trusty_error("failed (%d) to serialize request\n", rc);
        goto end;
    }
    rc = km_do_tipc(KM_SET_BOOT_PARAMS, req, req_size, false);

end:
    if (req) {
        trusty_free(req);
    }
    return rc;
}

static int trusty_send_attestation_data(uint32_t cmd, const uint8_t *data,
                                        uint32_t data_size,
                                        keymaster_algorithm_t algorithm)
{
    struct km_attestation_data attestation_data = {
        .algorithm = (uint32_t)algorithm,
        .data_size = data_size,
        .data = data,
    };
    uint8_t *req = NULL;
    uint32_t req_size = 0;
    int rc = km_attestation_data_serialize(&attestation_data, &req, &req_size);

    if (rc < 0) {
        trusty_error("failed (%d) to serialize request\n", rc);
        goto end;
    }
    rc = km_do_tipc(cmd, req, req_size, true);

end:
    if (req) {
        trusty_free(req);
    }
    return rc;
}

int trusty_set_attestation_key(const uint8_t *key, uint32_t key_size,
                               keymaster_algorithm_t algorithm)
{
    return trusty_send_attestation_data(KM_SET_ATTESTATION_KEY, key, key_size,
                                        algorithm);
}

int trusty_append_attestation_cert_chain(const uint8_t *cert,
                                         uint32_t cert_size,
                                         keymaster_algorithm_t algorithm)
{
    return trusty_send_attestation_data(KM_APPEND_ATTESTATION_CERT_CHAIN,
                                        cert, cert_size, algorithm);
}
