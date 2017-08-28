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

#include <trusty/trusty_ipc.h>
#include <trusty/util.h>

#define LOCAL_LOG 0

static int sync_ipc_on_connect_complete(struct trusty_ipc_chan *chan)
{
    trusty_assert(chan);

    chan->complete = 1;
    return TRUSTY_EVENT_HANDLED;
}

static int sync_ipc_on_message(struct trusty_ipc_chan *chan)
{
    trusty_assert(chan);

    chan->complete = 1;
    return TRUSTY_EVENT_HANDLED;
}

static int sync_ipc_on_disconnect(struct trusty_ipc_chan *chan)
{
    trusty_assert(chan);

    chan->complete = TRUSTY_ERR_CHANNEL_CLOSED;
    return TRUSTY_EVENT_HANDLED;
}

static int wait_for_complete(struct trusty_ipc_chan *chan)
{
    int rc;

    chan->complete = 0;
    for (;;) {
        rc = trusty_ipc_poll_for_event(chan->dev);
        if (rc < 0)
            return rc;

        if (chan->complete)
            break;

        if (rc == TRUSTY_EVENT_NONE)
            trusty_ipc_dev_idle(chan->dev);
    }

    return chan->complete;
}

static int wait_for_connect(struct trusty_ipc_chan *chan)
{
    trusty_debug("%s: chan %x: waiting for connect\n", __func__,
                 (int)chan->handle);
    return wait_for_complete(chan);
}

static int wait_for_send(struct trusty_ipc_chan *chan)
{
    trusty_debug("%s: chan %d: waiting for send\n", __func__, chan->handle);
    return wait_for_complete(chan);
}

static int wait_for_reply(struct trusty_ipc_chan *chan)
{
    trusty_debug("%s: chan %d: waiting for reply\n", __func__, chan->handle);
    return wait_for_complete(chan);
}

static struct trusty_ipc_ops sync_ipc_ops = {
    .on_connect_complete = sync_ipc_on_connect_complete,
    .on_message = sync_ipc_on_message,
    .on_disconnect = sync_ipc_on_disconnect,
};

void trusty_ipc_chan_init(struct trusty_ipc_chan *chan,
                          struct trusty_ipc_dev *dev)
{
    trusty_assert(chan);
    trusty_assert(dev);

    trusty_memset(chan, 0, sizeof(*chan));

    chan->handle = INVALID_IPC_HANDLE;
    chan->dev = dev;
    chan->ops = &sync_ipc_ops;
    chan->ops_ctx = chan;
}

int trusty_ipc_connect(struct trusty_ipc_chan *chan, const char *port,
                       bool wait)
{
    int rc;

    trusty_assert(chan);
    trusty_assert(chan->dev);
    trusty_assert(chan->handle == INVALID_IPC_HANDLE);
    trusty_assert(port);

    rc = trusty_ipc_dev_connect(chan->dev, port, (uint64_t)(uintptr_t)chan);
    if (rc < 0) {
        trusty_error("%s: init connection failed (%d)\n", __func__, rc);
        return rc;
    }
    chan->handle = (handle_t)rc;
    trusty_debug("chan->handle: %x\n", (int)chan->handle);

    /* got valid channel */
    if (wait) {
        rc = wait_for_connect(chan);
        if (rc < 0) {
            trusty_error("%s: wait for connect failed (%d)\n", __func__, rc);
            trusty_ipc_close(chan);
        }
    }

    return rc;
}

int trusty_ipc_close(struct trusty_ipc_chan *chan)
{
    int rc;

    trusty_assert(chan);

    rc = trusty_ipc_dev_close(chan->dev, chan->handle);
    chan->handle = INVALID_IPC_HANDLE;

    return rc;
}

int trusty_ipc_send(struct trusty_ipc_chan *chan,
                    const struct trusty_ipc_iovec *iovs, size_t iovs_cnt,
                    bool wait)
{
    int rc;

    trusty_assert(chan);
    trusty_assert(chan->dev);
    trusty_assert(chan->handle);

Again:
    rc = trusty_ipc_dev_send(chan->dev, chan->handle, iovs, iovs_cnt);
    if (rc == TRUSTY_ERR_SEND_BLOCKED) {
        if (wait) {
            rc = wait_for_send(chan);
            if (rc < 0) {
                trusty_error("%s: wait to send failed (%d)\n", __func__, rc);
                return rc;
            }
            goto Again;
        }
    }
    return rc;
}

int trusty_ipc_recv(struct trusty_ipc_chan *chan,
                    const struct trusty_ipc_iovec *iovs, size_t iovs_cnt,
                    bool wait)
{
    int rc;
    trusty_assert(chan);
    trusty_assert(chan->dev);
    trusty_assert(chan->handle);

    if (wait) {
        rc = wait_for_reply(chan);
        if (rc < 0) {
            trusty_error("%s: wait to reply failed (%d)\n", __func__, rc);
            return rc;
        }
    }

    rc = trusty_ipc_dev_recv(chan->dev, chan->handle, iovs, iovs_cnt);
    if (rc < 0)
        trusty_error("%s: ipc recv failed (%d)\n", __func__, rc);

    return rc;
}

int trusty_ipc_poll_for_event(struct trusty_ipc_dev *ipc_dev)
{
    int rc;
    struct trusty_ipc_event evt;
    struct trusty_ipc_chan *chan;

    trusty_assert(dev);

    rc = trusty_ipc_dev_get_event(ipc_dev, 0, &evt);
    if (rc) {
        trusty_error("%s: get event failed (%d)\n", __func__, rc);
        return rc;
    }

    /* check if we have an event */
    if (!evt.event) {
        trusty_debug("%s: no event\n", __func__);
        return TRUSTY_EVENT_NONE;
    }

    chan = (struct trusty_ipc_chan *)(uintptr_t)evt.cookie;
    trusty_assert(chan && chan->ops);

    /* check if we have raw event handler */
    if (chan->ops->on_raw_event) {
        /* invoke it first */
        rc = chan->ops->on_raw_event(chan, &evt);
        if (rc < 0) {
            trusty_error("%s: chan %d: raw event cb returned (%d)\n", __func__,
                         chan->handle, rc);
            return rc;
        }
        if (rc > 0)
            return rc; /* handled */
    }

    if (evt.event & IPC_HANDLE_POLL_ERROR) {
        /* something is very wrong */
        trusty_error("%s: chan %d: chan in error state\n", __func__,
                     chan->handle);
        return TRUSTY_ERR_GENERIC;
    }

    /* send unblocked should be handled first as it is edge truggered event */
    if (evt.event & IPC_HANDLE_POLL_SEND_UNBLOCKED) {
        if (chan->ops->on_send_unblocked) {
            rc = chan->ops->on_send_unblocked(chan);
            if (rc < 0) {
                trusty_error("%s: chan %d: send unblocked cb returned (%d)\n",
                             __func__, chan->handle, rc);
                return rc;
            }
            if (rc > 0)
                return rc; /* handled */
        }
    }

    /* check for connection complete */
    if (evt.event & IPC_HANDLE_POLL_READY) {
        if (chan->ops->on_connect_complete) {
            rc = chan->ops->on_connect_complete(chan);
            if (rc < 0) {
                trusty_error("%s: chan %d: ready cb returned (%d)\n", __func__,
                             chan->handle, rc);
                return rc;
            }
            if (rc > 0)
                return rc; /* handled */
        }
    }

    /* check for incomming messages */
    if (evt.event & IPC_HANDLE_POLL_MSG) {
        if (chan->ops->on_message) {
            rc = chan->ops->on_message(chan);
            if (rc < 0) {
                trusty_error("%s: chan %d: msg cb returned (%d)\n", __func__,
                             chan->handle, rc);
                return rc;
            }
            if (rc > 0)
                return rc;
        }
    }

    /* check for hangups */
    if (evt.event & IPC_HANDLE_POLL_HUP) {
        if (chan->ops->on_disconnect) {
            rc = chan->ops->on_disconnect(chan);
            if (rc < 0) {
                trusty_error("%s: chan %d: hup cb returned (%d)\n", __func__,
                             chan->handle, rc);
                return rc;
            }
            if (rc > 0)
                return rc;
        }
    }

    return TRUSTY_ERR_NONE;
}
