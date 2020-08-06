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
#include <trusty/keymaster_serializable.h>
#include <trusty/rpmb.h>
#include <trusty/trusty_ipc.h>
#include <trusty/util.h>

#define LOCAL_LOG 0

static struct trusty_ipc_chan km_chan;
static bool initialized = false;
static int trusty_km_version = 2;
static const size_t kMaxCaRequestSize = 10000;
static const size_t kMaxSendSize = 4000;
static const size_t kUuidSize = 32;

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef NELEMS
#define NELEMS(x) (sizeof(x) / sizeof((x)[0]))
#endif

static int km_send_request(uint32_t cmd, const void *req, size_t req_len)
{
    struct keymaster_message header = { .cmd = cmd };
    int num_iovecs = req ? 2 : 1;

    struct trusty_ipc_iovec req_iovs[2] = {
        { .base = &header, .len = sizeof(header) },
        { .base = (void*)req, .len = req_len },
    };

    return trusty_ipc_send(&km_chan, req_iovs, num_iovecs, true);
}

/* Checks that the command opcode in |header| matches |ex-ected_cmd|. Checks
 * that |tipc_result| is a valid response size. Returns negative on error.
 */
static int check_response_error(uint32_t expected_cmd,
                                struct keymaster_message header,
                                int32_t tipc_result)
{
    if (tipc_result < 0) {
        trusty_error("failed (%d) to recv response\n", tipc_result);
        return tipc_result;
    }
    if ((size_t) tipc_result < sizeof(struct keymaster_message)) {
        trusty_error("invalid response size (%d)\n", tipc_result);
        return TRUSTY_ERR_GENERIC;
    }
    if ((header.cmd & ~(KEYMASTER_STOP_BIT)) !=
        (expected_cmd | KEYMASTER_RESP_BIT)) {
        trusty_error("malformed response\n");
        return TRUSTY_ERR_GENERIC;
    }
    return tipc_result;
}

/* Reads the raw response to |resp| up to a maximum size of |resp_len|. Format
 * of each message frame read from the secure side:
 *
 * command header : 4 bytes
 * opaque bytes   : MAX(KEYMASTER_MAX_BUFFER_LENGTH, x) bytes
 *
 * The individual message frames from the secure side are reassembled
 * into |resp|, stripping each frame's command header. Returns the number
 * of bytes written to |resp| on success, negative on error.
 */
static int km_read_raw_response(uint32_t cmd, void *resp, size_t resp_len)
{
    struct keymaster_message header = { .cmd = cmd };
    int rc = TRUSTY_ERR_GENERIC;
    size_t max_resp_len = resp_len;
    struct trusty_ipc_iovec resp_iovs[2] = {
        { .base = &header, .len = sizeof(header) },
        { .base = resp, .len = MIN(KEYMASTER_MAX_BUFFER_LENGTH, max_resp_len) }
    };

    if (!resp) {
        return TRUSTY_ERR_GENERIC;
    }
    resp_len = 0;
    while (true) {
        resp_iovs[1].base = (uint8_t*)resp + resp_len;
        resp_iovs[1].len = MIN(KEYMASTER_MAX_BUFFER_LENGTH,
                               (int)max_resp_len - (int)resp_len);

        rc = trusty_ipc_recv(&km_chan, resp_iovs, NELEMS(resp_iovs), true);
        rc = check_response_error(cmd, header, rc);
        if (rc < 0) {
            return rc;
        }
        resp_len += ((size_t)rc - sizeof(struct keymaster_message));
        if (header.cmd & KEYMASTER_STOP_BIT || resp_len >= max_resp_len) {
            break;
        }
    }

    return resp_len;
}

/* Reads a Keymaster Response message with a sized buffer. The format
 * of the response is as follows:
 *
 * command header : 4 bytes
 * error          : 4 bytes
 * data length    : 4 bytes
 * data           : |data length| bytes
 *
 * On success, |error|, |resp_data|, and |resp_data_len| are filled
 * successfully. Returns a trusty_err.
 */
static int km_read_data_response(uint32_t cmd, int32_t *error,
                                 uint8_t* resp_data, uint32_t* resp_data_len)
{
    struct keymaster_message header = { .cmd = cmd };
    int rc = TRUSTY_ERR_GENERIC;
    size_t max_resp_len = *resp_data_len;
    uint32_t resp_data_bytes = 0;
    /* On the first read, recv the keymaster_message header, error code,
     * response data length, and response data. On subsequent iterations,
     * only recv the keymaster_message header and response data.
     */
    struct trusty_ipc_iovec resp_iovs[4] = {
        { .base = &header, .len = sizeof(header) },
        { .base = error, .len = sizeof(int32_t) },
        { .base = resp_data_len, .len = sizeof(uint32_t) },
        { .base = resp_data, .len = MIN(KEYMASTER_MAX_BUFFER_LENGTH, max_resp_len) }
    };

    rc = trusty_ipc_recv(&km_chan, resp_iovs, NELEMS(resp_iovs), true);
    rc = check_response_error(cmd, header, rc);
    if (rc < 0) {
        return rc;
    }
    /* resp_data_bytes does not include the error or response data length */
    resp_data_bytes += ((size_t)rc - sizeof(struct keymaster_message) -
                        2 * sizeof(uint32_t));
    if (header.cmd & KEYMASTER_STOP_BIT) {
        return TRUSTY_ERR_NONE;
    }

    /* Read the remaining response data */
    uint8_t* resp_data_start = resp_data + resp_data_bytes;
    size_t resp_data_remaining = *resp_data_len - resp_data_bytes;
    rc = km_read_raw_response(cmd, resp_data_start, resp_data_remaining);
    if (rc < 0) {
        return rc;
    }
    resp_data_bytes += rc;
    if (*resp_data_len != resp_data_bytes) {
        return TRUSTY_ERR_GENERIC;
    }
    return TRUSTY_ERR_NONE;
}

/**
 * Convenience method to send a request to the secure side, handle rpmb
 * operations, and receive the response. If |resp_data| is not NULL, the
 * caller expects an additional data buffer to be returned from the secure
 * side.
 */
static int km_do_tipc(uint32_t cmd, void* req, uint32_t req_len,
                      void* resp_data, uint32_t* resp_data_len)
{
    int rc = TRUSTY_ERR_GENERIC;
    struct km_no_response resp_header;

    rc = km_send_request(cmd, req, req_len);
    if (rc < 0) {
        trusty_error("%s: failed (%d) to send km request\n", __func__, rc);
        return rc;
    }

    if (!resp_data) {
        rc = km_read_raw_response(cmd, &resp_header, sizeof(resp_header));
    } else {
        rc = km_read_data_response(cmd, &resp_header.error, resp_data,
                                   resp_data_len);
    }

    if (rc < 0) {
        trusty_error("%s: failed (%d) to read km response\n", __func__, rc);
        return rc;
    }
    if (resp_header.error != KM_ERROR_OK) {
        trusty_error("%s: keymaster returned error (%d)\n", __func__,
                     resp_header.error);
        return TRUSTY_ERR_GENERIC;
    }
    return TRUSTY_ERR_NONE;
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
    struct km_get_version_resp resp;

    rc = km_send_request(KM_GET_VERSION, NULL, 0);
    if (rc < 0) {
        trusty_error("failed (%d) to send km version request", rc);
        return rc;
    }

    rc = km_read_raw_response(KM_GET_VERSION, &resp, sizeof(resp));
    if (rc < 0) {
        trusty_error("%s: failed (%d) to read km response\n", __func__, rc);
        return rc;
    }

    *version = MessageVersion(resp.major_ver, resp.minor_ver,
                              resp.subminor_ver);
    return TRUSTY_ERR_NONE;
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

    /* mark as initialized */
    initialized = true;

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

int trusty_set_boot_params(uint32_t os_version, uint32_t os_patchlevel,
                           keymaster_verified_boot_t verified_boot_state,
                           bool device_locked,
                           const uint8_t *verified_boot_key_hash,
                           uint32_t verified_boot_key_hash_size,
                           const uint8_t *verified_boot_hash,
                           uint32_t verified_boot_hash_size)
{
    if (!initialized) {
	trusty_error("Keymaster TIPC client not initialized!\n");
	return -1;
    }
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
    rc = km_do_tipc(KM_SET_BOOT_PARAMS, req, req_size, NULL, NULL);

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
    rc = km_do_tipc(cmd, req, req_size, NULL, NULL);

end:
    if (req) {
        trusty_free(req);
    }
    return rc;
}

static int trusty_send_raw_buffer(uint32_t cmd, const uint8_t *req_data,
                                  uint32_t req_data_size, uint8_t *resp_data,
                                  uint32_t *resp_data_size)
{
    struct km_raw_buffer buf = {
        .data_size = req_data_size,
        .data = req_data,
    };
    uint8_t *req = NULL;
    uint32_t req_size = 0;
    int rc = km_raw_buffer_serialize(&buf, &req, &req_size);
    if (rc < 0) {
        trusty_error("failed (%d) to serialize request\n", rc);
        goto end;
    }
    rc = km_do_tipc(cmd, req, req_size, resp_data, resp_data_size);

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

int trusty_set_attestation_key_enc(const uint8_t *key, uint32_t key_size,
                               keymaster_algorithm_t algorithm)
{
    return trusty_send_attestation_data(KM_SET_ATTESTATION_KEY_ENC, key, key_size,
                                        algorithm);
}

int trusty_append_attestation_cert_chain_enc(const uint8_t *cert,
                                         uint32_t cert_size,
                                         keymaster_algorithm_t algorithm)
{
    return trusty_send_attestation_data(KM_APPEND_ATTESTATION_CERT_CHAIN_ENC,
                                        cert, cert_size, algorithm);
}

int trusty_atap_get_ca_request(const uint8_t *operation_start,
                               uint32_t operation_start_size,
                               uint8_t **ca_request_p,
                               uint32_t *ca_request_size_p)
{
    *ca_request_p = trusty_calloc(1, kMaxCaRequestSize);
    if (!*ca_request_p) {
        return TRUSTY_ERR_NO_MEMORY;
    }
    *ca_request_size_p = kMaxCaRequestSize;
    int rc = trusty_send_raw_buffer(KM_ATAP_GET_CA_REQUEST, operation_start,
                                    operation_start_size, *ca_request_p,
                                    ca_request_size_p);
    if (rc != TRUSTY_ERR_NONE) {
        trusty_free(*ca_request_p);
    }
    return rc;
}

int trusty_atap_set_ca_response(const uint8_t *ca_response,
                                uint32_t ca_response_size)
{
    struct km_set_ca_response_begin_req begin_req;
    int rc = TRUSTY_ERR_GENERIC;
    uint32_t bytes_sent = 0, send_size = 0;

    /* Tell the Trusty Keymaster TA the size of CA Response message */
    begin_req.ca_response_size = ca_response_size;
    rc = km_do_tipc(KM_ATAP_SET_CA_RESPONSE_BEGIN, &begin_req,
                    sizeof(begin_req), NULL, NULL);
    if (rc != TRUSTY_ERR_NONE) {
        return rc;
    }

    /* Send the CA Response message in chunks */
    while (bytes_sent < ca_response_size) {
        send_size = MIN(kMaxSendSize, ca_response_size - bytes_sent);
        rc = trusty_send_raw_buffer(KM_ATAP_SET_CA_RESPONSE_UPDATE,
                                    ca_response + bytes_sent, send_size,
                                    NULL, NULL);
        if (rc != TRUSTY_ERR_NONE) {
            return rc;
        }
        bytes_sent += send_size;
    }

    /* Tell Trusty Keymaster to parse the CA Response message */
    return km_do_tipc(KM_ATAP_SET_CA_RESPONSE_FINISH, NULL, 0, NULL, NULL);
}


int trusty_atap_read_uuid_str(char **uuid_p)
{
    *uuid_p = (char*) trusty_calloc(1, kUuidSize);

    uint32_t response_size = kUuidSize;
    int rc = km_do_tipc(KM_ATAP_READ_UUID, NULL, 0, *uuid_p,
                        &response_size);
    if (rc < 0) {
        trusty_error("failed to read uuid: %d\n", rc);
        trusty_free(*uuid_p);
        return rc;
    }
    if (response_size != kUuidSize) {
        trusty_error("keymaster returned wrong uuid size: %d\n", response_size);
        trusty_free(*uuid_p);
        rc = TRUSTY_ERR_GENERIC;
    }
    return rc;
}

int trusty_get_mppubk(uint8_t *mppubk, uint32_t *size)
{
    int rc = TRUSTY_ERR_GENERIC;
    struct km_get_mppubk_resp resp;

    rc = km_send_request(KM_GET_MPPUBK, NULL, 0);
    if (rc < 0) {
        trusty_error("%s: failed (%d) to send km mppubk request\n", __func__, rc);
        return rc;
    }

    rc = km_read_raw_response(KM_GET_MPPUBK, &resp, sizeof(resp));
    if (rc < 0) {
        trusty_error("%s: failed (%d) to read km mppubk response\n", __func__, rc);
        return rc;
    }

    if (resp.data_size != 64) {
        trusty_error("%s: Wrong mppubk size!\n", __func__);
        return TRUSTY_ERR_GENERIC;
    } else {
        *size = resp.data_size;
    }

    memcpy(mppubk, resp.data, resp.data_size);
    return TRUSTY_ERR_NONE;
}

int trusty_verify_secure_unlock(uint8_t *unlock_credential,
                                uint32_t credential_size,
                                uint8_t *serial, uint32_t serial_size)
{
    int rc = TRUSTY_ERR_GENERIC;
    uint8_t *req = NULL;
    uint32_t req_size = 0;

    struct km_secure_unlock_data secure_unlock_data = {
        .serial_size = serial_size,
        .serial_data = serial,
        .credential_size = credential_size,
        .credential_data = unlock_credential,
    };

    rc = km_secure_unlock_data_serialize(&secure_unlock_data,
                                             &req, &req_size);

    if (rc < 0) {
        trusty_error("failed (%d) to serialize request\n", rc);
        goto end;
    }
    rc = km_do_tipc(KM_VERIFY_SECURE_UNLOCK, req, req_size, NULL, NULL);

end:
    if (req) {
        trusty_free(req);
    }
    return rc;
}
