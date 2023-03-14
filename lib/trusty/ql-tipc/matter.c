/*
 * Copyright 2022 NXP
 */

#include <trusty/matter.h>
#include <trusty/trusty_ipc.h>
#include <trusty/util.h>

static struct trusty_ipc_chan matter_chan;
static bool initialized = false;

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef NELEMS
#define NELEMS(x) (sizeof(x) / sizeof((x)[0]))
#endif

static int matter_send_request(uint32_t cmd, const void *req, size_t req_len)
{
    struct matter_message header = { .cmd = cmd };
    int num_iovecs = req ? 2 : 1;

    struct trusty_ipc_iovec req_iovs[2] = {
        { .base = &header, .len = sizeof(header) },
        { .base = (void*)req, .len = req_len },
    };

    return trusty_ipc_send(&matter_chan, req_iovs, num_iovecs, true);
}

/* Checks that the command opcode in |header| matches |expected_cmd|. Checks
 * that |tipc_result| is a valid response size. Returns negative on error.
 */
static int check_response_error(uint32_t expected_cmd,
                                struct matter_message header,
                                int32_t tipc_result)
{
    if (tipc_result < 0) {
        trusty_error("failed (%d) to recv response\n", tipc_result);
        return tipc_result;
    }
    if ((size_t) tipc_result < sizeof(struct matter_message)) {
        trusty_error("invalid response size (%d)\n", tipc_result);
        return TRUSTY_ERR_GENERIC;
    }
    if ((header.cmd & ~(MATTER_STOP_BIT)) !=
        (expected_cmd | MATTER_RESP_BIT)) {
        trusty_error("malformed response\n");
        return TRUSTY_ERR_GENERIC;
    }
    return tipc_result;
}

/* Reads the raw response to |resp| up to a maximum size of |resp_len|. Format
 * of each message frame read from the secure side:
 *
 * command header : 4 bytes
 * opaque bytes   : MAX(MATTER_MAX_BUFFER_LENGTH, x) bytes
 *
 * The individual message frames from the secure side are reassembled
 * into |resp|, stripping each frame's command header. Returns the number
 * of bytes written to |resp| on success, negative on error.
 */
static int matter_read_raw_response(uint32_t cmd, void *resp, size_t resp_len)
{
    struct matter_message header = { .cmd = cmd };
    int rc = TRUSTY_ERR_GENERIC;
    size_t max_resp_len = resp_len;
    struct trusty_ipc_iovec resp_iovs[2] = {
        { .base = &header, .len = sizeof(header) },
        { .base = resp, .len = MIN(MATTER_MAX_BUFFER_LENGTH, max_resp_len) }
    };

    if (!resp) {
        return TRUSTY_ERR_GENERIC;
    }
    resp_len = 0;
    while (true) {
        resp_iovs[1].base = (uint8_t*)resp + resp_len;
        resp_iovs[1].len = MIN(MATTER_MAX_BUFFER_LENGTH,
                               (int)max_resp_len - (int)resp_len);

        rc = trusty_ipc_recv(&matter_chan, resp_iovs, NELEMS(resp_iovs), true);
        rc = check_response_error(cmd, header, rc);
        if (rc < 0) {
            return rc;
        }
        resp_len += ((size_t)rc - sizeof(struct matter_message));
        if (header.cmd & MATTER_STOP_BIT || resp_len >= max_resp_len) {
            break;
        }
    }

    return resp_len;
}

/* Reads a Matter Response message with a sized buffer. The format
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
static int matter_read_data_response(uint32_t cmd, int32_t *error,
                                 uint8_t* resp_data, uint32_t* resp_data_len)
{
    struct matter_message header = { .cmd = cmd };
    int rc = TRUSTY_ERR_GENERIC;
    size_t max_resp_len = *resp_data_len;
    uint32_t resp_data_bytes = 0;
    /* On the first read, recv the matter_message header, error code,
     * response data length, and response data. On subsequent iterations,
     * only recv the matter_message header and response data.
     */
    struct trusty_ipc_iovec resp_iovs[4] = {
        { .base = &header, .len = sizeof(header) },
        { .base = error, .len = sizeof(int32_t) },
        { .base = resp_data_len, .len = sizeof(uint32_t) },
        { .base = resp_data, .len = MIN(MATTER_MAX_BUFFER_LENGTH, max_resp_len) }
    };

    rc = trusty_ipc_recv(&matter_chan, resp_iovs, NELEMS(resp_iovs), true);
    rc = check_response_error(cmd, header, rc);
    if (rc < 0) {
        return rc;
    }
    /* resp_data_bytes does not include the error or response data length */
    resp_data_bytes += ((size_t)rc - sizeof(struct matter_message) -
                        2 * sizeof(uint32_t));
    if (header.cmd & MATTER_STOP_BIT) {
        return TRUSTY_ERR_NONE;
    }

    /* Read the remaining response data */
    uint8_t* resp_data_start = resp_data + resp_data_bytes;
    size_t resp_data_remaining = *resp_data_len - resp_data_bytes;
    rc = matter_read_raw_response(cmd, resp_data_start, resp_data_remaining);
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
static int matter_do_tipc(uint32_t cmd, void* req, uint32_t req_len,
                      void* resp_data, uint32_t* resp_data_len)
{
    int rc = TRUSTY_ERR_GENERIC;
    struct matter_no_response resp_header;

    rc = matter_send_request(cmd, req, req_len);
    if (rc < 0) {
        trusty_error("%s: failed (%d) to send matter request\n", __func__, rc);
        return rc;
    }

    if (!resp_data) {
        rc = matter_read_raw_response(cmd, &resp_header, sizeof(resp_header));
    } else {
        rc = matter_read_data_response(cmd, &resp_header.error, resp_data,
                                   resp_data_len);
    }

    if (rc < 0) {
        trusty_error("%s: failed (%d) to read matter response\n", __func__, rc);
        return rc;
    }
    if (resp_header.error != MATTER_ERROR_OK) {
        trusty_error("%s: matter returned error (%d)\n", __func__,
                     resp_header.error);
        return TRUSTY_ERR_GENERIC;
    }
    return TRUSTY_ERR_NONE;
}

int matter_tipc_init(struct trusty_ipc_dev *dev)
{
    int rc = TRUSTY_ERR_GENERIC;

    trusty_assert(dev);

    trusty_ipc_chan_init(&matter_chan, dev);
    trusty_debug("Connecting to Matter service\n");

    /* connect to matter service and wait for connect to complete */
    rc = trusty_ipc_connect(&matter_chan, MATTER_PORT, true);
    if (rc < 0) {
        trusty_error("failed (%d) to connect to '%s'\n", rc, MATTER_PORT);
        return rc;
    }

    /* mark as initialized */
    initialized = true;

    return TRUSTY_ERR_NONE;
}

void matter_tipc_shutdown(struct trusty_ipc_dev *dev)
{
    if (!initialized)
        return;
    /* close channel */
    trusty_ipc_close(&matter_chan);

    initialized = false;
}

static uint8_t *append_to_buf(uint8_t *buf, const void *data, size_t data_len)
{
    if (data && data_len) {
        trusty_memcpy(buf, data, data_len);
    }
    return buf + data_len;
}

static uint8_t *append_uint32_to_buf(uint8_t *buf, uint32_t val)
{
    return append_to_buf(buf, &val, sizeof(val));
}

static uint8_t *append_sized_buf_to_buf(uint8_t *buf, const uint8_t *data,
                                 uint32_t data_len)
{
    buf = append_uint32_to_buf(buf, data_len);
    return append_to_buf(buf, data, data_len);
}

static int matter_cert_data_serialize(const struct matter_cert_data *data, uint8_t** out, uint32_t *out_size)
{
    uint8_t *tmp;

    if (!out || !data || !out_size) {
        return TRUSTY_ERR_INVALID_ARGS;
    }
    *out_size = (sizeof(data->data_size) + data->data_size);
    *out = trusty_calloc(*out_size, 1);
    if (!*out) {
        return TRUSTY_ERR_NO_MEMORY;
    }

    tmp = append_sized_buf_to_buf(*out, data->data, data->data_size);

    return TRUSTY_ERR_NONE;
}

static int trusty_send_cert_data(uint32_t cmd, const uint8_t *data, uint32_t data_size)
{
    struct matter_cert_data cert_data = {
        .data_size = data_size,
        .data = data,
    };
    uint8_t *req = NULL;
    uint32_t req_size = 0;
    int rc = matter_cert_data_serialize(&cert_data, &req, &req_size);

    if (rc < 0) {
        trusty_error("failed (%d) to serialize request\n", rc);
        goto end;
    }
    rc = matter_do_tipc(cmd, req, req_size, NULL, NULL);

end:
    if (req) {
        trusty_free(req);
    }
    return rc;
}

int trusty_set_dac_cert(const uint8_t *cert, uint32_t cert_size)
{
    if (!initialized) {
        trusty_error("Matter TIPC client not initialized!\n");
        return -1;
    }
    return trusty_send_cert_data(MATTER_IMPORT_DAC, cert, cert_size);
}

int trusty_set_pai_cert(const uint8_t *cert, uint32_t cert_size)
{
    if (!initialized) {
        trusty_error("Matter TIPC client not initialized!\n");
        return -1;
    }
    return trusty_send_cert_data(MATTER_IMPORT_PAI, cert, cert_size);
}

int trusty_set_cd_cert(const uint8_t *cert, uint32_t cert_size)
{
    if (!initialized) {
        trusty_error("Matter TIPC client not initialized!\n");
        return -1;
    }
    return trusty_send_cert_data(MATTER_IMPORT_CD, cert, cert_size);
}

int trusty_set_dac_prikey(const uint8_t *key, uint32_t key_size)
{
    if (!initialized) {
        trusty_error("Matter TIPC client not initialized!\n");
        return -1;
    }
    return trusty_send_cert_data(MATTER_IMPORT_DAC_PRIKEY, key, key_size);
}
