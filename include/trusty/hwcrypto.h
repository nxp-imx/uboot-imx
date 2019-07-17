/*
 * Copyright (C) 2016 The Android Open Source Project
 * Copyright NXP 2018
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
 *
 */

#ifndef TRUSTY_HWCRYPTO_H_
#define TRUSTY_HWCRYPTO_H_

#include <trusty/sysdeps.h>
#include <trusty/trusty_ipc.h>
#include <interface/hwcrypto/hwcrypto.h>

/*
 * Initialize HWCRYPTO TIPC client. Returns one of trusty_err.
 *
 * @dev: initialized with trusty_ipc_dev_create
 */
int hwcrypto_tipc_init(struct trusty_ipc_dev *dev);
/*
 * Shutdown HWCRYPTO TIPC client.
 *
 * @dev: initialized with trusty_ipc_dev_create
 */
void hwcrypto_tipc_shutdown(struct trusty_ipc_dev *dev);
/*
 * Send request to secure side to calculate sha256 hash with caam.
 * Returns one of trusty_err.
 *
 * @in_addr:   start address of the input buf
 * @in_len:    size of the input buf
 * @out_addr:  start address of the output buf
 * @out_len:   size of the output buf
 * @algo:      hash algorithm type expect to use
 */
int hwcrypto_hash(uint32_t in_addr, uint32_t in_len, uint32_t out_addr,
                  uint32_t out_len, enum hwcrypto_hash_algo algo);

/*
 * Send request to secure side to generate blob with caam.
 * Returns one of trusty_err.
 *
 * @plain_pa:   physical start address of the plain blob buffer.
 * @plain_size: size of the plain blob buffer.
 * @blob_pa:    physical start address of the generated blob buffer.
 */
int hwcrypto_gen_blob(uint32_t plain_pa,
                      uint32_t plain_size, uint32_t blob_pa);

/* Send request to secure side to generate rng with caam.
 * Returns one of trusty_err.
 *
 * @buf: physical start address of the output rng buf.
 * @len: size of required rng.
 * */
int hwcrypto_gen_rng(uint32_t buf, uint32_t len);

/* Send request to secure side to generate bkek with caam.
 * Returns one of trusty_err.
 *
 * @buf: physical start address of the output rng buf.
 * @len: size of required rng.
 * */
int hwcrypto_gen_bkek(uint32_t buf, uint32_t len);

/* Send request to secure side to lock boot state, so some
 * hwcrypto commands can't be used outside of bootloader.
 * Returns one of trusty_err.
 * */
int hwcrypto_lock_boot_state(void);

#endif /* TRUSTY_HWCRYPTO_H_ */
