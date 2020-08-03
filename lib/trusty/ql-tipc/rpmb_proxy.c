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
bool proxy_resp_flag = false;
static int req_len = 0;

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
 * Read RPMB response from storage service. Writes message to @msg
 * and @req.
 *
 * @chan:     proxy ipc channel
 * @msg:      address of storage message header
 * @cmd:      cmd corresponding to the request
 * @resp:     address of storage message response
 * @resp_len: length of resp in bytes
 */
static int proxy_read_response(struct trusty_ipc_chan *chan, struct storage_msg *msg,
                               uint32_t cmd, void *resp, size_t resp_len)
{
    int rc;

    struct trusty_ipc_iovec resp_iovs[2] = {
        { .base = msg, .len = sizeof(*msg) },
        { .base = resp, .len = resp_len },
    };
    rc = trusty_ipc_recv(chan, resp_iovs, resp ? 2 : 1, true);
    if (rc < 0) {
        /* recv message failed */
        trusty_error("%s: failed (%d) to recv response\n", __func__, rc);
        return rc;
    }
    if (proxy_resp_flag) {
        memcpy(msg, &req_msg, sizeof(struct storage_msg));
        if (resp)
            memcpy(resp, req_buf, req_len);
        rc = req_len + sizeof(struct storage_msg);
        proxy_resp_flag = false;
    }

    if ((size_t)rc < sizeof(*msg)) {
        /* malformed message */
        trusty_error("%s: malformed request (%zu)\n", __func__, (size_t)rc);
        return TRUSTY_ERR_GENERIC;
    }

    if (msg->cmd != (cmd | STORAGE_RESP_BIT)) {
        trusty_error("malformed response, cmd: 0x%x\n", msg->cmd);
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
 * Send RPMB request to storage service
 *
 * @chan:     proxy ipc channel
 * @msg:      address of storage message header
 * @req:      address of storage message request
 * @req_len:  length of request in bytes
 */
static int proxy_send_request(struct trusty_ipc_chan *chan,
                              struct storage_msg *msg, void *req,
                              size_t req_len)
{
    struct trusty_ipc_iovec req_iovs[2] = {
        { .base = msg, .len = sizeof(*msg) },
        { .base = req, .len = req_len }
    };

    return trusty_ipc_send(chan, req_iovs, req ? 2 : 1, false);
}

/*
 * Convenience function to send a request to the storage service and read the
 * response.
 *
 * @cmd: the command
 * @req: the request buffer
 * @req_size: size of the request buffer
 * @resp: the response buffer
 * @resp_size_p: pointer to the size of the response buffer. changed to the
                 actual size of the response read from the secure side
 */
static int storage_do_tipc(uint32_t cmd, void *req, uint32_t req_size, void *resp,
                       uint32_t *resp_size_p)
{
    int rc;
    struct storage_msg msg = { .cmd = cmd };

    if (!initialized) {
        trusty_error("%s: Secure storage TIPC client not initialized\n", __func__);
        return TRUSTY_ERR_GENERIC;
    }
    rc = proxy_send_request(&proxy_chan, &msg, req, req_size);
    if (rc < 0) {
        trusty_error("%s: failed (%d) to send storage request\n", __func__, rc);
        return rc;
    }

    uint32_t resp_size = resp_size_p ? *resp_size_p : 0;
    rc = proxy_read_response(&proxy_chan, &msg, cmd, resp, resp_size);
    if (rc < 0) {
        trusty_error("%s: failed (%d) to read secure storage response\n", __func__, rc);
        return rc;
    }
    /* change response size to actual response size */
    if (resp_size_p && rc != *resp_size_p) {
        *resp_size_p = rc;
    }
    if (msg.result != STORAGE_NO_ERROR) {
        trusty_error("%s: secure storage service returned error (%d)\n", __func__,
                     msg.result);
        return TRUSTY_ERR_GENERIC;
    }
    return TRUSTY_ERR_NONE;
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

    /**
     * The response of proxy will also be routed to here but we should
     * not handle them, just return and set "proxy_resp_flag" to indicate
     * func trusty_ipc_dev_recv() not try to receive the msg again.
     */
    if (req_msg.cmd & STORAGE_RESP_BIT) {
        chan->complete = 1;
        proxy_resp_flag = 1;
        req_len = rc;
        return TRUSTY_EVENT_HANDLED;
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

int storage_set_rpmb_key(void)
{
    uint32_t size = 0;
    return storage_do_tipc(STORAGE_RPMB_KEY_SET, NULL, 0, NULL, &size);
}

int storage_erase_rpmb(void)
{
    uint32_t size = 0;
    return storage_do_tipc(STORAGE_RPMB_ERASE_ALL, NULL, 0, NULL, &size);
}
