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

#ifndef TRUSTY_RPMB_H_
#define TRUSTY_RPMB_H_

#include <trusty/sysdeps.h>
#include <trusty/trusty_ipc.h>

#define MMC_BLOCK_SIZE 512

/*
 * Initialize RPMB storage proxy. Returns one of trusty_err.
 *
 * @dev:      initialized with trusty_ipc_dev_create
 * @rpmb_dev: Context of RPMB device, initialized with rpmb_storage_get_ctx
 */
int rpmb_storage_proxy_init(struct trusty_ipc_dev *dev, void *rpmb_dev);
/*
 * Shutdown RPMB storage proxy
 *
 * @dev: initialized with trusty_ipc_dev_create
 */
void rpmb_storage_proxy_shutdown(struct trusty_ipc_dev *dev);
/*
 * Execute RPMB command. Implementation is platform specific.
 * Returns one of trusty_err.
 *
 * @rpmb_dev:            Context of RPMB device, initialized with
 *                       rpmb_storage_get_ctx
 * @reliable_write_data: Buffer containing RPMB structs for reliable write
 * @reliable_write_size: Size of reliable_write_data
 * @write_data:          Buffer containing RPMB structs for write
 * @write_size:          Size of write_data
 * @read_data:           Buffer to be filled with RPMB structs read from RPMB
 *                       partition
 * @read_size:           Size of read_data
 */
int rpmb_storage_send(void *rpmb_dev,
                      const void *reliable_write_data,
                      size_t reliable_write_size,
                      const void *write_data, size_t write_size,
                      void *read_buf, size_t read_size);
/*
 * Return context for RPMB device. This is called when the RPMB storage proxy is
 * initialized, and subsequently used when issuing RPMB storage requests.
 * Implementation is platform specific.
 */
void *rpmb_storage_get_ctx(void);

/*
 * Put back RPMB device. This is called when the RPMB storage proxy is
 * shutdown
 */
void rpmb_storage_put_ctx(void *dev);

#endif /* TRUSTY_RPMB_H_ */
