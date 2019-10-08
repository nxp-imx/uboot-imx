/*
 * Copyright (C) 2016 The Android Open Source Project
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

#include <trusty/avb.h>
#include <trusty/rpmb.h>
#include <trusty/trusty_ipc.h>
#include <trusty/util.h>

#define LOCAL_LOG 0

static bool initialized;
static int avb_tipc_version = 1;
static struct trusty_ipc_chan avb_chan;

static int avb_send_request(struct avb_message *msg, void *req, size_t req_len)
{
    struct trusty_ipc_iovec req_iovs[2] = {
        { .base = msg, .len = sizeof(*msg) },
        { .base = req, .len = req_len },
    };

    return trusty_ipc_send(&avb_chan, req_iovs, req ? 2 : 1, true);
}

static int avb_read_response(struct avb_message *msg, uint32_t cmd, void *resp,
                             size_t resp_len)
{
    int rc;
    struct trusty_ipc_iovec resp_iovs[2] = {
        { .base = msg, .len = sizeof(*msg) },
        { .base = resp, .len = resp_len },
    };

    rc = trusty_ipc_recv(&avb_chan, resp_iovs, resp ? 2 : 1, true);
    if (rc < 0) {
        trusty_error("failed (%d) to recv response\n", rc);
        return rc;
    }
    if (msg->cmd != (cmd | AVB_RESP_BIT)) {
        trusty_error("malformed response\n");
        return TRUSTY_ERR_GENERIC;
    }
    /* return payload size */
    return rc - sizeof(*msg);
}

/*
 * Convenience function to send a request to the AVB service and read the
 * response.
 *
 * @cmd: the command
 * @req: the request buffer
 * @req_size: size of the request buffer
 * @resp: the response buffer
 * @resp_size_p: pointer to the size of the response buffer. changed to the
                 actual size of the response read from the secure side
 */
static int avb_do_tipc(uint32_t cmd, void *req, uint32_t req_size, void *resp,
                       uint32_t *resp_size_p)
{
    int rc;
    struct avb_message msg = { .cmd = cmd };

    if (!initialized && cmd != AVB_GET_VERSION) {
        trusty_error("%s: AVB TIPC client not initialized\n", __func__);
        return TRUSTY_ERR_GENERIC;
    }

    rc = avb_send_request(&msg, req, req_size);
    if (rc < 0) {
        trusty_error("%s: failed (%d) to send AVB request\n", __func__, rc);
        return rc;
    }

    uint32_t resp_size = resp_size_p ? *resp_size_p : 0;
    rc = avb_read_response(&msg, cmd, resp, resp_size);
    if (rc < 0) {
        trusty_error("%s: failed (%d) to read AVB response\n", __func__, rc);
        return rc;
    }
    /* change response size to actual response size */
    if (resp_size_p && rc != *resp_size_p) {
        *resp_size_p = rc;
    }
    if (msg.result != AVB_ERROR_NONE) {
        trusty_error("%s: AVB service returned error (%d)\n", __func__,
                     msg.result);
        return TRUSTY_ERR_GENERIC;
    }
    return TRUSTY_ERR_NONE;
}

static int avb_get_version(uint32_t *version)
{
    int rc;
    struct avb_get_version_resp resp;
    uint32_t resp_size = sizeof(resp);

    rc = avb_do_tipc(AVB_GET_VERSION, NULL, 0, &resp, &resp_size);

    *version = resp.version;
    return rc;
}


int avb_tipc_init(struct trusty_ipc_dev *dev)
{
    int rc;
    uint32_t version = 0;

    trusty_assert(dev);
    trusty_assert(!initialized);

    trusty_ipc_chan_init(&avb_chan, dev);
    trusty_debug("Connecting to AVB service\n");

    /* connect to AVB service and wait for connect to complete */
    rc = trusty_ipc_connect(&avb_chan, AVB_PORT, true);
    if (rc < 0) {
        trusty_error("failed (%d) to connect to '%s'\n", rc, AVB_PORT);
        return rc;
    }

    /* check for version mismatch */
    rc = avb_get_version(&version);
    if (rc != 0) {
        trusty_error("Error getting version");
        return TRUSTY_ERR_GENERIC;
    }
    if (version != avb_tipc_version) {
        trusty_error("AVB TIPC version mismatch. Expected %u, received %u\n",
                     avb_tipc_version, version);
        return TRUSTY_ERR_GENERIC;
    }

    /* mark as initialized */
    initialized = true;

    return TRUSTY_ERR_NONE;
}

void avb_tipc_shutdown(struct trusty_ipc_dev *dev)
{
    if (!initialized)
        return; /* nothing to do */

    /* close channel */
    trusty_ipc_close(&avb_chan);

    initialized = false;
}

int trusty_read_rollback_index(uint32_t slot, uint64_t *value)
{
    int rc;
    struct avb_rollback_req req = { .slot = slot, .value = 0 };
    struct avb_rollback_resp resp;
    uint32_t resp_size = sizeof(resp);

    rc = avb_do_tipc(READ_ROLLBACK_INDEX, &req, sizeof(req), &resp,
                     &resp_size);

    *value = resp.value;
    return rc;
}

int trusty_write_rollback_index(uint32_t slot, uint64_t value)
{
    int rc;
    struct avb_rollback_req req = { .slot = slot, .value = value };
    struct avb_rollback_resp resp;
    uint32_t resp_size = sizeof(resp);

    rc = avb_do_tipc(WRITE_ROLLBACK_INDEX, &req, sizeof(req), &resp,
                     &resp_size);
    return rc;
}

int trusty_read_permanent_attributes(uint8_t *attributes, uint32_t size)
{
    uint8_t resp_buf[AVB_MAX_BUFFER_LENGTH];
    uint32_t resp_size = AVB_MAX_BUFFER_LENGTH;
    int rc = avb_do_tipc(READ_PERMANENT_ATTRIBUTES, NULL, 0, resp_buf,
                         &resp_size);
    if (rc != 0) {
        return rc;
    }
    /* ensure caller passed size matches size returned by Trusty */
    if (size != resp_size) {
        return TRUSTY_ERR_INVALID_ARGS;
    }
    trusty_memcpy(attributes, resp_buf, resp_size);
    return rc;
}

int trusty_write_permanent_attributes(uint8_t *attributes, uint32_t size)
{
    return avb_do_tipc(WRITE_PERMANENT_ATTRIBUTES, attributes, size, NULL,
                       NULL);
}

int trusty_read_vbmeta_public_key(uint8_t *publickey, uint32_t size)
{
    uint8_t resp_buf[AVB_MAX_BUFFER_LENGTH];
    uint32_t resp_size = AVB_MAX_BUFFER_LENGTH;
    int rc = avb_do_tipc(READ_VBMETA_PUBLIC_KEY, NULL, 0, resp_buf,
                         &resp_size);
    if (rc != 0) {
        return rc;
    }
    /* ensure caller passed size matches size returned by Trusty */
    if (size < resp_size) {
        return TRUSTY_ERR_INVALID_ARGS;
    }
    trusty_memcpy(publickey, resp_buf, resp_size);
    return rc;
}

int trusty_write_vbmeta_public_key(uint8_t *publickey, uint32_t size)
{
    return avb_do_tipc(WRITE_VBMETA_PUBLIC_KEY, publickey, size, NULL,
                       NULL);
}

int trusty_read_lock_state(uint8_t *lock_state)
{
    uint32_t resp_size = sizeof(*lock_state);
    return avb_do_tipc(READ_LOCK_STATE, NULL, 0, lock_state,
                       &resp_size);
}

int trusty_write_lock_state(uint8_t lock_state)
{
    return avb_do_tipc(WRITE_LOCK_STATE, &lock_state, sizeof(lock_state), NULL,
                       NULL);
}

int trusty_lock_boot_state(void)
{
    return avb_do_tipc(LOCK_BOOT_STATE, NULL, 0, NULL, NULL);
}

int trusty_read_oem_unlock_device_permission(uint8_t *oem_device_unlock)
{
    uint32_t resp_size = sizeof(*oem_device_unlock);
    return avb_do_tipc(READ_OEM_UNLOCK_DEVICE_PERMISSION, NULL, 0, oem_device_unlock,
                       &resp_size);
}
