/*
 * Copyright (C) 2017 The Android Open Source Project
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

#ifndef TRUSTY_KEYMASTER_SERIALIZABLE_H_
#define TRUSTY_KEYMASTER_SERIALIZABLE_H_

#include <trusty/keymaster.h>

/**
 * Simple serialization routines for dynamically sized keymaster messages.
 */

/**
 * Appends |data_len| bytes at |data| to |buf|. Performs no bounds checking,
 * assumes sufficient memory allocated at |buf|. Returns |buf| + |data_len|.
 */
uint8_t *append_to_buf(uint8_t *buf, const void *data, size_t data_len);

/**
 * Appends |val| to |buf|. Performs no bounds checking. Returns |buf| +
 * sizeof(uint32_t).
 */
uint8_t *append_uint32_to_buf(uint8_t *buf, uint32_t val);

/**
 * Appends a sized buffer to |buf|. First appends |data_len| to |buf|, then
 * appends |data_len| bytes at |data| to |buf|. Performs no bounds checking.
 * Returns |buf| + sizeof(uint32_t) + |data_len|.
 */
uint8_t *append_sized_buf_to_buf(uint8_t *buf, const uint8_t *data,
                                 uint32_t data_len);

/**
 * Serializes a km_boot_params structure. On success, allocates |*out_size|
 * bytes to |*out| and writes the serialized |params| to |*out|. Caller takes
 * ownership of |*out|. Returns one of trusty_err.
 */
int km_boot_params_serialize(const struct km_boot_params *params, uint8_t **out,
                             uint32_t *out_size);

/**
 * Serializes a km_attestation_data structure. On success, allocates |*out_size|
 * bytes to |*out| and writes the serialized |data| to |*out|. Caller takes
 * ownership of |*out|. Returns one of trusty_err.
 */
int km_attestation_data_serialize(const struct km_attestation_data *data,
                                  uint8_t **out, uint32_t *out_size);

/**
 * Serializes a km_secure_unlock_data structure. On success, allocates |*out_size|
 * bytes to |*out| and writes the serialized |data| to |*out|. Caller takes
 * ownership of |*out|. Returns one of trusty_err.
 */
int km_secure_unlock_data_serialize(const struct km_secure_unlock_data *data,
                                  uint8_t **out, uint32_t *out_size);

/**
 * Serializes a km_raw_buffer structure. On success, allocates |*out_size|
 * bytes to |*out| and writes the serialized |data| to |*out|. Caller takes
 * ownership of |*out|. Returns one of trusty_err.
 */
int km_raw_buffer_serialize(const struct km_raw_buffer *buf, uint8_t** out,
                            uint32_t *out_size);

#endif /* TRUSTY_KEYMASTER_SERIALIZABLE_H_ */
