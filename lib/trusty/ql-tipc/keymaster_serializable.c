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

#include <trusty/keymaster_serializable.h>

uint8_t *append_to_buf(uint8_t *buf, const void *data, size_t data_len)
{
    if (data && data_len) {
        trusty_memcpy(buf, data, data_len);
    }
    return buf + data_len;
}

uint8_t *append_uint32_to_buf(uint8_t *buf, uint32_t val)
{
    return append_to_buf(buf, &val, sizeof(val));
}

uint8_t *append_sized_buf_to_buf(uint8_t *buf, const uint8_t *data,
                                 uint32_t data_len)
{
    buf = append_uint32_to_buf(buf, data_len);
    return append_to_buf(buf, data, data_len);
}

int km_boot_params_serialize(const struct km_boot_params *params, uint8_t** out,
                             uint32_t *out_size)
{
    uint8_t *tmp;

    if (!out || !params || !out_size) {
        return TRUSTY_ERR_INVALID_ARGS;
    }
    *out_size = (sizeof(params->os_version) + sizeof(params->os_patchlevel) +
                 sizeof(params->device_locked) +
                 sizeof(params->verified_boot_state) +
                 sizeof(params->verified_boot_key_hash_size) +
                 sizeof(params->verified_boot_hash_size) +
                 params->verified_boot_key_hash_size +
                 params->verified_boot_hash_size);
    *out = trusty_calloc(*out_size, 1);
    if (!*out) {
        return TRUSTY_ERR_NO_MEMORY;
    }

    tmp = append_uint32_to_buf(*out, params->os_version);
    tmp = append_uint32_to_buf(tmp, params->os_patchlevel);
    tmp = append_uint32_to_buf(tmp, params->device_locked);
    tmp = append_uint32_to_buf(tmp, params->verified_boot_state);
    tmp = append_sized_buf_to_buf(tmp, params->verified_boot_key_hash,
                                  params->verified_boot_key_hash_size);
    tmp = append_sized_buf_to_buf(tmp, params->verified_boot_hash,
                                  params->verified_boot_hash_size);

    return TRUSTY_ERR_NONE;
}

int km_attestation_data_serialize(const struct km_attestation_data *data,
                                 uint8_t** out, uint32_t *out_size)
{
    uint8_t *tmp;

    if (!out || !data || !out_size) {
        return TRUSTY_ERR_INVALID_ARGS;
    }
    *out_size = (sizeof(data->algorithm) + sizeof(data->data_size) +
                 data->data_size);
    *out = trusty_calloc(*out_size, 1);
    if (!*out) {
        return TRUSTY_ERR_NO_MEMORY;
    }

    tmp = append_uint32_to_buf(*out, data->algorithm);
    tmp = append_sized_buf_to_buf(tmp, data->data, data->data_size);

    return TRUSTY_ERR_NONE;
}

int km_secure_unlock_data_serialize(const struct km_secure_unlock_data *data,
                                 uint8_t** out, uint32_t *out_size)
{
    uint8_t *tmp;

    if (!out || !data || !out_size) {
        return TRUSTY_ERR_INVALID_ARGS;
    }
    *out_size = (sizeof(data->serial_size) + sizeof(data->credential_size) +
                 data->serial_size + data->credential_size);
    *out = trusty_calloc(*out_size, 1);
    if (!*out) {
        return TRUSTY_ERR_NO_MEMORY;
    }

    tmp = append_sized_buf_to_buf(*out, data->serial_data, data->serial_size);
    tmp = append_sized_buf_to_buf(tmp, data->credential_data, data->credential_size);

    return TRUSTY_ERR_NONE;
}

int km_raw_buffer_serialize(const struct km_raw_buffer *buf, uint8_t** out,
                            uint32_t *out_size)
{
    if (!out || !buf || !out_size) {
        return TRUSTY_ERR_INVALID_ARGS;
    }
    *out_size = sizeof(buf->data_size) + buf->data_size;
    *out = trusty_calloc(*out_size, 1);
    if (!*out) {
        return TRUSTY_ERR_NO_MEMORY;
    }
    append_sized_buf_to_buf(*out, buf->data, buf->data_size);

    return TRUSTY_ERR_NONE;
}
