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

#ifndef TRUSTY_TRUSTY_IPC_H_
#define TRUSTY_TRUSTY_IPC_H_

#include <trusty/sysdeps.h>

/*
 * handle_t is an opaque 32 bit value that is used to reference an
 * Trusty IPC channel
 */
typedef uint32_t handle_t;

#define INVALID_IPC_HANDLE  0

/*
 * Error codes returned by Trusty IPC device function calls
 */
enum trusty_err {
    TRUSTY_ERR_NONE            =  0,
    TRUSTY_ERR_GENERIC         = -1,
    TRUSTY_ERR_NOT_SUPPORTED   = -2,
    TRUSTY_ERR_NO_MEMORY       = -3,
    TRUSTY_ERR_INVALID_ARGS    = -4,
    TRUSTY_ERR_SECOS_ERR       = -5,
    TRUSTY_ERR_MSG_TOO_BIG     = -6,
    TRUSTY_ERR_NO_MSG          = -7,
    TRUSTY_ERR_CHANNEL_CLOSED  = -8,
    TRUSTY_ERR_SEND_BLOCKED    = -9,
};
/*
 * Return codes for successful Trusty IPC events (failures return trusty_err)
 */
enum trusty_event_result {
    TRUSTY_EVENT_HANDLED   = 1,
    TRUSTY_EVENT_NONE      = 2
};

/*
 * Combination of these values are used for the event field
 * of trusty_ipc_event structure.
 */
enum trusty_ipc_event_type {
    IPC_HANDLE_POLL_NONE  = 0x0,
    IPC_HANDLE_POLL_READY = 0x1,
    IPC_HANDLE_POLL_ERROR = 0x2,
    IPC_HANDLE_POLL_HUP   = 0x4,
    IPC_HANDLE_POLL_MSG   = 0x8,
    IPC_HANDLE_POLL_SEND_UNBLOCKED = 0x10,
};

struct trusty_dev;
struct trusty_ipc_chan;

/*
 * Trusty IPC event
 *
 * @event:  event type
 * @handle: handle this event is related to
 * @cookie: cookie associated with handle
 */
struct trusty_ipc_event {
    uint32_t event;
    uint32_t handle;
    uint64_t cookie;
};

struct trusty_ipc_iovec {
    void *base;
    size_t len;
};

/*
 * Trusty IPC device
 *
 * @buf_vaddr: virtual address of shared buffer associated with device
 * @buf_size:  size of shared buffer
 * @buf_ns:    physical address info of shared buffer
 * @tdev:      trusty device
 */
struct trusty_ipc_dev {
    void  *buf_vaddr;
    size_t buf_size;
    struct ns_mem_page_info buf_ns;
    struct trusty_dev *tdev;
};

/*
 * Trusty IPC event handlers.
 */
struct trusty_ipc_ops {
    int (*on_raw_event)(struct trusty_ipc_chan *chan,
                        struct trusty_ipc_event *evt);
    int (*on_connect_complete)(struct trusty_ipc_chan *chan);
    int (*on_send_unblocked)(struct trusty_ipc_chan *chan);
    int (*on_message)(struct trusty_ipc_chan *chan);
    int (*on_disconnect)(struct trusty_ipc_chan *chan);
};

/*
 * Trusty IPC channel.
 *
 * @ops_ctx:  refers to additional data that may be used by trusty_ipc_ops
 * @handle:   identifier for channel
 * @complete: completion status of last event on channel
 * @dev:      Trusty IPC device used by channel, initialized with
              trusty_ipc_dev_create
 * @ops:      callbacks for Trusty events
 */
struct trusty_ipc_chan {
    void *ops_ctx;
    handle_t handle;
    volatile int complete;
    struct trusty_ipc_dev *dev;
    struct trusty_ipc_ops *ops;
};

/*
 * Creates new Trusty IPC device on @tdev. Allocates shared buffer, and calls
 * trusty_dev_init_ipc to register with secure side. Returns a trusty_err.
 *
 * @ipc_dev:  new Trusty IPC device to be initialized
 * @tdev:     associated Trusty device
 * @shared_buf_size: size of shared buffer to be allocated
 */
int trusty_ipc_dev_create(struct trusty_ipc_dev **ipc_dev,
                          struct trusty_dev *tdev,
                          size_t shared_buf_size);
/*
 * Shutdown @dev. Frees shared buffer, and calls trusty_dev_shutdown_ipc
 * to shutdown on the secure side.
 */
void trusty_ipc_dev_shutdown(struct trusty_ipc_dev *dev);

/*
 * Calls into secure OS to initiate a new connection to a Trusty IPC service.
 * Returns handle for the new channel, a trusty_err on error.
 *
 * @dev:    Trusty IPC device initialized with trusty_ipc_dev_create
 * @port:   name of port to connect to on secure side
 * @cookie: cookie associated with new channel.
 */
int trusty_ipc_dev_connect(struct trusty_ipc_dev *dev, const char *port,
                           uint64_t cookie);
/*
 * Calls into secure OS to close connection to Trusty IPC service.
 * Returns a trusty_err.
 *
 * @dev:  Trusty IPC device
 * @chan: handle for connection, opened with trusty_ipc_dev_connect
 */
int trusty_ipc_dev_close(struct trusty_ipc_dev *dev, handle_t chan);

/*
 * Calls into secure OS to receive pending event. Returns a trusty_err.
 *
 * @dev:   Trusty IPC device
 * @chan:  handle for connection
 * @event: pointer to output event struct
 */
int trusty_ipc_dev_get_event(struct trusty_ipc_dev *dev, handle_t chan,
                             struct trusty_ipc_event *event);
/*
 * Calls into secure OS to send message to channel. Returns a trusty_err.
 *
 * @dev:      Trusty IPC device
 * @chan:     handle for connection
 * @iovs:     contains messages to be sent
 * @iovs_cnt: number of iovecs to be sent
 */
int trusty_ipc_dev_send(struct trusty_ipc_dev *dev, handle_t chan,
                        const struct trusty_ipc_iovec *iovs, size_t iovs_cnt);
/*
 * Calls into secure OS to receive message on channel. Returns number of bytes
 * received on success, trusty_err on failure.
 *
 * @dev:      Trusty IPC device
 * @chan:     handle for connection
 * @iovs:     contains received messages
 * @iovs_cnt: number of iovecs received
 */
int trusty_ipc_dev_recv(struct trusty_ipc_dev *dev, handle_t chan,
                        const struct trusty_ipc_iovec *iovs, size_t iovs_cnt);

void trusty_ipc_dev_idle(struct trusty_ipc_dev *dev);

/*
 * Initializes @chan with default values and @dev.
 */
void trusty_ipc_chan_init(struct trusty_ipc_chan *chan,
                          struct trusty_ipc_dev *dev);
/*
 * Calls trusty_ipc_dev_connect to get a handle for channel.
 * Returns a trusty_err.
 *
 * @chan: channel to initialize with new handle
 * @port: name of port to connect to on secure side
 * @wait: flag to wait for connect to complete by polling for
 *        IPC_HANDLE_POLL_READY event
 */
int trusty_ipc_connect(struct trusty_ipc_chan *chan, const char *port,
                       bool wait);
/*
 * Calls trusty_ipc_dev_close and invalidates @chan. Returns a trusty_err.
 */
int trusty_ipc_close(struct trusty_ipc_chan *chan);
/*
 * Calls trusty_ipc_dev_get_event to poll @dev for events. Handles
 * events by calling appropriate callbacks. Returns nonnegative on success.
 */
int trusty_ipc_poll_for_event(struct trusty_ipc_dev *dev);
/*
 * Calls trusty_ipc_dev_send to send a message. Returns a trusty_err.
 *
 * @chan:     handle for connection
 * @iovs:     contains messages to be sent
 * @iovs_cnt: number of iovecs to be sent
 * @wait:     flag to wait for send to complete
 */
int trusty_ipc_send(struct trusty_ipc_chan *chan,
                    const struct trusty_ipc_iovec *iovs, size_t iovs_cnt,
                    bool wait);
/*
 * Calls trusty_ipc_dev_recv to receive a message. Return number of bytes
 * received on success, trusty_err on failure.
 *
 * @chan:     handle for connection
 * @iovs:     contains received messages
 * @iovs_cnt: number of iovecs received
 * @wait:     flag to wait for a message to receive
 */
int trusty_ipc_recv(struct trusty_ipc_chan *chan,
                    const struct trusty_ipc_iovec *iovs, size_t iovs_cnt,
                    bool wait);

#endif /* TRUSTY_TRUSTY_IPC_H_ */
