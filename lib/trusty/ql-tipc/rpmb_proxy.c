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

#include <trusty/rpmb.h>
#include <trusty/trusty_ipc.h>
#include <trusty/util.h>
#include <interface/storage/storage.h>

#define LOCAL_LOG 0

static bool initialized;
/* Address of rpmb device */
static void *proxy_rpmb;
struct trusty_ipc_chan proxy_chan;

struct storage_msg req_msg;
static uint8_t req_buf[4096];
static uint8_t read_buf[4096];

/*
 * Read RPMB request from storage service. Writes message to @msg
 * and @req.
 *
 * @chan:    proxy ipc channel
 * @msg:     address of storage message header
 * @req:     address of storage message request
 * @req_len: length of req in bytes
 */
static int proxy_read_request(struct trusty_ipc_chan *chan,
                              struct storage_msg *msg, void *req,
                              size_t req_len)
{
    int rc;

    struct trusty_ipc_iovec req_iovs[2] = {
        { .base = msg, .len = sizeof(*msg) },
        { .base = req, .len = req_len },
    };
    rc = trusty_ipc_recv(chan, req_iovs, 2, false);
    if (rc < 0) {
        /* recv message failed */
        trusty_error("%s: failed (%d) to recv request\n", __func__, rc);
        return rc;
    }

    if ((size_t)rc < sizeof(*msg)) {
        /* malformed message */
        trusty_error("%s: malformed request (%zu)\n", __func__, (size_t)rc);
        return TRUSTY_ERR_GENERIC;
    }

    return rc - sizeof(*msg); /* return payload size */
}

/*
 * Send RPMB response to storage service
 *
 * @chan:     proxy ipc channel
 * @msg:      address of storage message header
 * @resp:     address of storage message response
 * @resp_len: length of resp in bytes
 */
static int proxy_send_response(struct trusty_ipc_chan *chan,
                               struct storage_msg *msg, void *resp,
                               size_t resp_len)
{
    struct trusty_ipc_iovec resp_iovs[2] = {
        { .base = msg, .len = sizeof(*msg) },
        { .base = resp, .len = resp_len }
    };

    msg->cmd |= STORAGE_RESP_BIT;
    return trusty_ipc_send(chan, resp_iovs, resp ? 2 : 1, false);
}

/*
 * Executes the RPMB request at @r, sends response to storage service.
 *
 * @chan:    proxy ipc channel
 * @msg:     address of storage message header
 * @r:       address of storage message request
 * @req_len: length of resp in bytes
 */
static int proxy_handle_rpmb(struct trusty_ipc_chan *chan,
                             struct storage_msg *msg, const void *r,
                             size_t req_len)
{
    int rc;
    size_t exp_len;
    const void *write_data = NULL;
    const void *rel_write_data = NULL;
    const struct storage_rpmb_send_req *req = r;

    if (req_len < sizeof(req)) {
        msg->result = STORAGE_ERR_NOT_VALID;
        goto err_response;
    }

    exp_len = sizeof(*req) + req->reliable_write_size + req->write_size;
    if (req_len != exp_len) {
        trusty_error(
            "%s: malformed rpmb request: invalid length (%zu != %zu)\n",
            __func__, req_len, exp_len);
        msg->result = STORAGE_ERR_NOT_VALID;
        goto err_response;
    }

    if (req->reliable_write_size) {
        if ((req->reliable_write_size % MMC_BLOCK_SIZE) != 0) {
            trusty_error("%s: invalid reliable write size %u\n", __func__,
                         req->reliable_write_size);
            msg->result = STORAGE_ERR_NOT_VALID;
            goto err_response;
        }
        rel_write_data = req->payload;
    }

    if (req->write_size) {
        if ((req->write_size % MMC_BLOCK_SIZE) != 0) {
            trusty_error("%s: invalid write size %u\n", __func__,
                         req->write_size);
            msg->result = STORAGE_ERR_NOT_VALID;
            goto err_response;
        }
        write_data = req->payload + req->reliable_write_size;
    }

    if (req->read_size) {
        if (req->read_size % MMC_BLOCK_SIZE != 0 ||
            req->read_size > sizeof(read_buf)) {
            trusty_error("%s: invalid read size %u\n", __func__,
                         req->read_size);
            msg->result = STORAGE_ERR_NOT_VALID;
            goto err_response;
        }
    }

    /* execute rpmb command */
    rc = rpmb_storage_send(proxy_rpmb,
                           rel_write_data, req->reliable_write_size,
                           write_data, req->write_size,
                           read_buf, req->read_size);
    if (rc) {
        trusty_error("%s: rpmb_storage_send failed: %d\n", __func__, rc);
        msg->result = STORAGE_ERR_GENERIC;
        goto err_response;
    }

    if (msg->flags & STORAGE_MSG_FLAG_POST_COMMIT) {
        /*
         * Nothing todo for post msg commit request as MMC_IOC_MULTI_CMD
         * is fully synchronous in this implementation.
         */
    }

    msg->result = STORAGE_NO_ERROR;
    return proxy_send_response(chan, msg, read_buf, req->read_size);

err_response:
    return proxy_send_response(chan, msg, NULL, 0);
}

/*
 * Handles storage request.
 *
 * @chan:    proxy ipc channel
 * @msg:     address of storage message header
 * @req:     address of storage message request
 * @req_len: length of resp in bytes
 */
static int proxy_handle_req(struct trusty_ipc_chan *chan,
                            struct storage_msg *msg, const void *req,
                            size_t req_len)
{
    int rc;

    if (msg->flags & STORAGE_MSG_FLAG_PRE_COMMIT) {
        /* nothing to do */
    }

    switch (msg->cmd) {
    case STORAGE_RPMB_SEND:
        rc = proxy_handle_rpmb(chan, msg, req, req_len);
        break;

    case STORAGE_FILE_DELETE:
    case STORAGE_FILE_OPEN:
    case STORAGE_FILE_CLOSE:
    case STORAGE_FILE_WRITE:
    case STORAGE_FILE_READ:
    case STORAGE_FILE_GET_SIZE:
    case STORAGE_FILE_SET_SIZE:
        /* Bulk filesystem is not supported */
        msg->result = STORAGE_ERR_UNIMPLEMENTED;
        rc = proxy_send_response(chan, msg, NULL, 0);
        break;

    default:
        msg->result = STORAGE_ERR_UNIMPLEMENTED;
        rc = proxy_send_response(chan, msg, NULL, 0);
    }

    return rc;
}

/*
 * Invalidates @chan on hangup event
 *
 * @chan:    proxy ipc channel
 */
static int proxy_on_disconnect(struct trusty_ipc_chan *chan)
{
    trusty_assert(chan);

    trusty_debug("%s: closed by peer\n", __func__);
    chan->handle = INVALID_IPC_HANDLE;
    return TRUSTY_EVENT_HANDLED;
}

/*
 * Handles received storage message on message event
 *
 * @chan:    proxy ipc channel
 */
static int proxy_on_message(struct trusty_ipc_chan *chan)
{
    int rc;

    trusty_assert(chan);

    /* read request */
    rc = proxy_read_request(chan, &req_msg, req_buf, sizeof(req_buf));
    if (rc < 0) {
        trusty_error("%s: failed (%d) to read request\n", __func__, rc);
        trusty_ipc_close(chan);
        return rc;
    }

    /* handle it and send reply */
    rc = proxy_handle_req(chan, &req_msg, req_buf, rc);
    if (rc < 0) {
        trusty_error("%s: failed (%d) to handle request\n", __func__, rc);
        trusty_ipc_close(chan);
        return rc;
    }

    return TRUSTY_EVENT_HANDLED;
}

static struct trusty_ipc_ops proxy_ops = {
    .on_message = proxy_on_message,
    .on_disconnect = proxy_on_disconnect,
};

/*
 * Initialize RPMB storage proxy
 */
int rpmb_storage_proxy_init(struct trusty_ipc_dev *dev, void *rpmb_dev)
{
    int rc;

    trusty_assert(dev);
    trusty_assert(!initialized);

    /* attach rpmb device  */
    proxy_rpmb = rpmb_dev;

    /* init ipc channel */
    trusty_ipc_chan_init(&proxy_chan, dev);

    /* connect to proxy service and wait for connect to complete */
    rc = trusty_ipc_connect(&proxy_chan, STORAGE_DISK_PROXY_PORT, true);
    if (rc < 0) {
        trusty_error("%s: failed (%d) to connect to '%s'\n", __func__, rc,
                     STORAGE_DISK_PROXY_PORT);
        return rc;
    }

    /* override default ops */
    proxy_chan.ops = &proxy_ops;

    do {
        /* Check for RPMB events */
        rc = trusty_ipc_poll_for_event(proxy_chan.dev);
        if (rc < 0) {
            trusty_error("%s: failed (%d) to get rpmb event\n", __func__, rc);
            return rc;
        }

        if (proxy_chan.handle == INVALID_IPC_HANDLE) {
            trusty_error("%s: unexpected proxy channel close\n", __func__);
            return TRUSTY_ERR_CHANNEL_CLOSED;
        }
    }
    while (rc != TRUSTY_EVENT_NONE);

    /* mark as initialized */
    initialized = true;

    return TRUSTY_ERR_NONE;
}

void rpmb_storage_proxy_shutdown(struct trusty_ipc_dev *dev)
{
    trusty_assert(initialized);

    /* close channel */
    trusty_ipc_close(&proxy_chan);

    initialized = false;
}
