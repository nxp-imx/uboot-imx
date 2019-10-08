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

#ifndef TRUSTY_AVB_H_
#define TRUSTY_AVB_H_

#include <trusty/sysdeps.h>
#include <trusty/trusty_ipc.h>
#include <interface/avb/avb.h>

/*
 * Initialize AVB TIPC client. Returns one of trusty_err.
 *
 * @dev: initialized with trusty_ipc_dev_create
 */
int avb_tipc_init(struct trusty_ipc_dev *dev);
/*
 * Shutdown AVB TIPC client.
 *
 * @dev: initialized with trusty_ipc_dev_create
 */
void avb_tipc_shutdown(struct trusty_ipc_dev *dev);
/*
 * Send request to secure side to read rollback index.
 * Returns one of trusty_err.
 *
 * @slot:    rollback index slot
 * @value:   rollback index value stored here
 */
int trusty_read_rollback_index(uint32_t slot, uint64_t *value);
/*
 * Send request to secure side to write rollback index
 * Returns one of trusty_err.
 *
 * @slot:    rollback index slot
 * @value:   rollback index value to write
 */
int trusty_write_rollback_index(uint32_t slot, uint64_t value);
/*
 * Send request to secure side to read permanent attributes. When permanent
 * attributes are stored in RPMB, a hash of the permanent attributes which is
 * given to AVB during verification MUST still be backed by write-once hardware.
 *
 * Copies attributes received by secure side to |attributes|. If |size| does not
 * match the size returned by the secure side, an error is returned. Returns one
 * of trusty_err.
 *
 * @attributes:  caller allocated buffer
 * @size:        size of |attributes|
 */
int trusty_read_permanent_attributes(uint8_t *attributes, uint32_t size);
/*
 * Send request to secure side to write permanent attributes. Permanent
 * attributes can only be written to storage once.
 *
 * Returns one of trusty_err.
 */
int trusty_write_permanent_attributes(uint8_t *attributes, uint32_t size);
/*
 * Send request to secure side to read vbmeta public key.
 *
 * Copies public key received by secure side to |publickey|. If |size| does not
 * match the size returned by the secure side, an error is returned. Returns one
 * of trusty_err.
 *
 * @publickey:   caller allocated buffer
 * @size:        size of |publickey|
 */
int trusty_read_vbmeta_public_key(uint8_t *publickey, uint32_t size);
/*
 * Send request to secure side to write vbmeta public key. Public key
 * can only be written to storage once.
 *
 * Returns one of trusty_err.
 */
int trusty_write_vbmeta_public_key(uint8_t *publickey, uint32_t size);
/*
 * Send request to secure side to read device lock state from RPMB.
 *
 * Returns one of trusty_err.
 */
int trusty_read_lock_state(uint8_t *lock_state);
/*
 * Send request to secure side to write device lock state to RPMB. If the lock
 * state is changed, all rollback index data will be cleared.
 *
 * Returns one of trusty_err.
 */
int trusty_write_lock_state(uint8_t lock_state);
/*
 * Send request to secure side to lock the boot state. After this is invoked,
 * the non-secure side will not be able to write to data managed by the AVB
 * service until next boot.
 *
 * Returns one of trusty_err.
 */
int trusty_lock_boot_state(void);
/*
 * Send request to secure side to read oem device unlock state from RPMB.
 *
 * Returns one of trusty_err.
 */
int trusty_read_oem_unlock_device_permission(uint8_t *lock_state);

#endif /* TRUSTY_AVB_H_ */
