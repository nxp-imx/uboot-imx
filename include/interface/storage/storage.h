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

#ifndef TRUSTY_INTERFACE_STORAGE_H_
#define TRUSTY_INTERFACE_STORAGE_H_

/*
 * The contents of this file are copied from
 * trusty/lib/interface/storage/include/interface/storage/storage.h.
 * It is required to stay in sync for struct formats and enum values.
 */

#include <trusty/sysdeps.h>

/*
 * @STORAGE_DISK_PROXY_PORT: Port used by non-secure proxy server
 */
#define STORAGE_DISK_PROXY_PORT    "com.android.trusty.storage.proxy"

enum storage_cmd {
    STORAGE_REQ_SHIFT = 1,
    STORAGE_RESP_BIT  = 1,

    STORAGE_RESP_MSG_ERR   = STORAGE_RESP_BIT,

    STORAGE_FILE_DELETE    = 1 << STORAGE_REQ_SHIFT,
    STORAGE_FILE_OPEN      = 2 << STORAGE_REQ_SHIFT,
    STORAGE_FILE_CLOSE     = 3 << STORAGE_REQ_SHIFT,
    STORAGE_FILE_READ      = 4 << STORAGE_REQ_SHIFT,
    STORAGE_FILE_WRITE     = 5 << STORAGE_REQ_SHIFT,
    STORAGE_FILE_GET_SIZE  = 6 << STORAGE_REQ_SHIFT,
    STORAGE_FILE_SET_SIZE  = 7 << STORAGE_REQ_SHIFT,

    STORAGE_RPMB_SEND      = 8 << STORAGE_REQ_SHIFT,

    /* transaction support */
    STORAGE_END_TRANSACTION = 9 << STORAGE_REQ_SHIFT,
};

/**
 * enum storage_err - error codes for storage protocol
 * @STORAGE_NO_ERROR:           all OK
 * @STORAGE_ERR_GENERIC:        unknown error. Can occur when there's an internal server
 *                              error, e.g. the server runs out of memory or is in a bad state.
 * @STORAGE_ERR_NOT_VALID:      input not valid. May occur if the arguments passed
 *                              into the command are not valid, for example if the file handle
 *                              passed in is not a valid one.
 * @STORAGE_ERR_UNIMPLEMENTED:  the command passed in is not recognized
 * @STORAGE_ERR_ACCESS:         the file is not accessible in the requested mode
 * @STORAGE_ERR_NOT_FOUND:      the file was not found
 * @STORAGE_ERR_EXIST           the file exists when it shouldn't as in with OPEN_CREATE | OPEN_EXCLUSIVE.
 * @STORAGE_ERR_TRANSACT        returned by various operations to indicate that current transaction
 *                              is in error state. Such state could be only cleared by sending
 *                              STORAGE_END_TRANSACTION message.
 */
enum storage_err {
    STORAGE_NO_ERROR          = 0,
    STORAGE_ERR_GENERIC       = 1,
    STORAGE_ERR_NOT_VALID     = 2,
    STORAGE_ERR_UNIMPLEMENTED = 3,
    STORAGE_ERR_ACCESS        = 4,
    STORAGE_ERR_NOT_FOUND     = 5,
    STORAGE_ERR_EXIST         = 6,
    STORAGE_ERR_TRANSACT      = 7,
};

/**
 * enum storage_msg_flag - protocol-level flags in struct storage_msg
 * @STORAGE_MSG_FLAG_BATCH:             if set, command belongs to a batch transaction.
 *                                      No response will be sent by the server until
 *                                      it receives a command with this flag unset, at
 *                                      which point a cummulative result for all messages
 *                                      sent with STORAGE_MSG_FLAG_BATCH will be sent.
 *                                      This is only supported by the non-secure disk proxy
 *                                      server.
 * @STORAGE_MSG_FLAG_PRE_COMMIT:        if set, indicates that server need to commit
 *                                      pending changes before processing this message.
 * @STORAGE_MSG_FLAG_POST_COMMIT:       if set, indicates that server need to commit
 *                                      pending changes after processing this message.
 * @STORAGE_MSG_FLAG_TRANSACT_COMPLETE: if set, indicates that server need to commit
 *                                      current transaction after processing this message.
 *                                      It is an alias for STORAGE_MSG_FLAG_POST_COMMIT.
 */
enum storage_msg_flag {
    STORAGE_MSG_FLAG_BATCH = 0x1,
    STORAGE_MSG_FLAG_PRE_COMMIT = 0x2,
    STORAGE_MSG_FLAG_POST_COMMIT = 0x4,
    STORAGE_MSG_FLAG_TRANSACT_COMPLETE = STORAGE_MSG_FLAG_POST_COMMIT,
};

/*
 * The following declarations are the message-specific contents of
 * the 'payload' element inside struct storage_msg.
 */

/**
 * struct storage_rpmb_send_req - request format for STORAGE_RPMB_SEND
 * @reliable_write_size:        size in bytes of reliable write region
 * @write_size:                 size in bytes of write region
 * @read_size:                  number of bytes to read for a read request
 * @__reserved:                 unused, must be set to 0
 * @payload:                    start of reliable write region, followed by
 *                              write region.
 *
 * Only used in proxy<->server interface.
 */
struct storage_rpmb_send_req {
    uint32_t reliable_write_size;
    uint32_t write_size;
    uint32_t read_size;
    uint32_t __reserved;
    uint8_t  payload[0];
};

/**
 * struct storage_rpmb_send_resp: response type for STORAGE_RPMB_SEND
 * @data: the data frames frames retrieved from the MMC.
 */
struct storage_rpmb_send_resp {
    uint8_t data[0];
};

/**
 * struct storage_msg - generic req/resp format for all storage commands
 * @cmd:        one of enum storage_cmd
 * @op_id:      client chosen operation identifier for an instance
 *              of a command or atomic grouping of commands (transaction).
 * @flags:      one or many of enum storage_msg_flag or'ed together.
 * @size:       total size of the message including this header
 * @result:     one of enum storage_err
 * @__reserved: unused, must be set to 0.
 * @payload:    beginning of command specific message format
 */
struct storage_msg {
    uint32_t cmd;
    uint32_t op_id;
    uint32_t flags;
    uint32_t size;
    int32_t  result;
    uint32_t __reserved;
    uint8_t  payload[0];
};

#endif /* TRUSTY_INTERFACE_STORAGE_H_ */
