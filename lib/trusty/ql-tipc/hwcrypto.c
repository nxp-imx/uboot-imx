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
 */

#include <trusty/hwcrypto.h>
#include <trusty/rpmb.h>
#include <trusty/trusty_ipc.h>
#include <trusty/util.h>
#include <memalign.h>
#include "common.h"
#include <cpu_func.h>
#include <hang.h>
#include <trusty/keymaster_serializable.h>
#include <env.h>
#include <mmc.h>

#define LOCAL_LOG 0
#define CAAM_KB_HEADER_LEN 48

static bool initialized;
static struct trusty_ipc_chan hwcrypto_chan;

static int hwcrypto_send_request(struct hwcrypto_message *msg, void *req, size_t req_len)
{
    struct trusty_ipc_iovec req_iovs[2] = {
        { .base = msg, .len = sizeof(*msg) },
        { .base = req, .len = req_len },
    };

    return trusty_ipc_send(&hwcrypto_chan, req_iovs, req ? 2 : 1, true);
}

static int hwcrypto_read_response(struct hwcrypto_message *msg, uint32_t cmd, void *resp,
                             size_t resp_len)
{
    int rc;
    struct trusty_ipc_iovec resp_iovs[2] = {
        { .base = msg, .len = sizeof(*msg) },
        { .base = resp, .len = resp_len },
    };

    rc = trusty_ipc_recv(&hwcrypto_chan, resp_iovs, resp ? 2 : 1, true);
    if (rc < 0) {
        trusty_error("failed (%d) to recv response\n", rc);
        return rc;
    }
    if (msg->cmd != (cmd | HWCRYPTO_RESP_BIT)) {
        trusty_error("malformed response\n");
        return TRUSTY_ERR_GENERIC;
    }
    /* return payload size */
    return rc - sizeof(*msg);
}

/*
 * Convenience function to send a request to the hwcrypto service and read the
 * response.
 *
 * @cmd: the command
 * @req: the request buffer
 * @req_size: size of the request buffer
 * @resp: the response buffer
 * @resp_size_p: pointer to the size of the response buffer. changed to the
                 actual size of the response read from the secure side
 */
static int hwcrypto_do_tipc(uint32_t cmd, void *req, uint32_t req_size, void *resp,
                       uint32_t *resp_size_p)
{
    int rc;
    struct hwcrypto_message msg = { .cmd = cmd };

    if (!initialized) {
        trusty_error("%s: HWCRYPTO TIPC client not initialized\n", __func__);
        return TRUSTY_ERR_GENERIC;
    }

    rc = hwcrypto_send_request(&msg, req, req_size);
    if (rc < 0) {
        trusty_error("%s: failed (%d) to send hwcrypto request\n", __func__, rc);
        return rc;
    }

    uint32_t resp_size = resp_size_p ? *resp_size_p : 0;
    rc = hwcrypto_read_response(&msg, cmd, resp, resp_size);
    if (rc < 0) {
        trusty_error("%s: failed (%d) to read HWCRYPTO response\n", __func__, rc);
        return rc;
    }
    /* change response size to actual response size */
    if (resp_size_p && rc != *resp_size_p) {
        *resp_size_p = rc;
    }
    if (msg.result != HWCRYPTO_ERROR_NONE) {
        trusty_error("%s: HWCRYPTO service returned error (%d)\n", __func__,
                     msg.result);
        return TRUSTY_ERR_GENERIC;
    }
    return TRUSTY_ERR_NONE;
}

int hwcrypto_tipc_init(struct trusty_ipc_dev *dev)
{
    int rc;

    trusty_assert(dev);
    trusty_assert(!initialized);

    trusty_ipc_chan_init(&hwcrypto_chan, dev);
    trusty_debug("Connecting to hwcrypto service\n");

    /* connect to hwcrypto service and wait for connect to complete */
    rc = trusty_ipc_connect(&hwcrypto_chan, HWCRYPTO_PORT, true);
    if (rc < 0) {
        trusty_error("failed (%d) to connect to '%s'\n", rc, HWCRYPTO_PORT);
        return rc;
    }

    /* mark as initialized */
    initialized = true;

    return TRUSTY_ERR_NONE;
}

void hwcrypto_tipc_shutdown(struct trusty_ipc_dev *dev)
{
    if (!initialized)
        return; /* nothing to do */

    /* close channel */
    trusty_ipc_close(&hwcrypto_chan);

    initialized = false;
}

int hwcrypto_hash(uint32_t in_addr, uint32_t in_len, uint32_t out_addr,
                  uint32_t out_len, enum hwcrypto_hash_algo algo)
{
    hwcrypto_hash_msg req;
    unsigned long start, end;

    /* check the address */
    if (in_addr == 0 || out_addr == 0)
        return TRUSTY_ERR_INVALID_ARGS;
    /* fill the request buffer */
    req.in_addr  = in_addr;
    req.out_addr = out_addr;
    req.in_len   = in_len;
    req.out_len  = out_len;
    req.algo     = algo;

    /* flush dcache for input buffer */
    start = (unsigned long)in_addr & ~(ARCH_DMA_MINALIGN - 1);
    end   = ALIGN((unsigned long)in_addr + in_len, ARCH_DMA_MINALIGN);
    flush_dcache_range(start, end);

    /* invalidate dcache for output buffer */
    start = (unsigned long)out_addr & ~(ARCH_DMA_MINALIGN - 1);
    end   = ALIGN((unsigned long)out_addr + out_len, ARCH_DMA_MINALIGN);
    invalidate_dcache_range(start, end);

    int rc = hwcrypto_do_tipc(HWCRYPTO_HASH, (void*)&req,
                              sizeof(req), NULL, 0);

    /* invalidate the dcache again before read to avoid coherency
     * problem caused by speculative memory access by the CPU.
     */
    invalidate_dcache_range(start, end);

    return rc;
}

int hwcrypto_gen_blob(uint32_t plain_pa,
                      uint32_t plain_size, uint32_t blob_pa)
{
    hwcrypto_blob_msg req;
    unsigned long start, end;

    /* check the address */
    if (plain_pa == 0 || blob_pa == 0)
        return TRUSTY_ERR_INVALID_ARGS;
    /* fill the request buffer */
    req.plain_pa   = plain_pa;
    req.plain_size = plain_size;
    req.blob_pa    = blob_pa;

    /* flush dcache for input buffer */
    start = (unsigned long)plain_pa & ~(ARCH_DMA_MINALIGN - 1);
    end   = ALIGN((unsigned long)plain_pa + plain_size, ARCH_DMA_MINALIGN);
    flush_dcache_range(start, end);

    /* invalidate dcache for output buffer */
    start = (unsigned long)blob_pa & ~(ARCH_DMA_MINALIGN - 1);
    end   = ALIGN((unsigned long)blob_pa + plain_size +
                  CAAM_KB_HEADER_LEN, ARCH_DMA_MINALIGN);
    invalidate_dcache_range(start, end);

    int rc = hwcrypto_do_tipc(HWCRYPTO_ENCAP_BLOB, (void*)&req,
                              sizeof(req), NULL, 0);

    /* invalidate the dcache again before read to avoid coherency
     * problem caused by speculative memory access by the CPU.
     */
    invalidate_dcache_range(start, end);
    return rc;
}

int hwcrypto_gen_rng(uint8_t *buf, uint32_t len)
{
    hwcrypto_rng_req req;
    uint8_t *resp = NULL;
    int resp_len;
    int rc = 0;

    req.len = len;
    resp = trusty_calloc(len, 1);
    if (!resp) {
        trusty_error("Failed to allocate memory!\n");
        return TRUSTY_ERR_NO_MEMORY;
    }
    resp_len = len;

    rc = hwcrypto_do_tipc(HWCRYPTO_GEN_RNG, (void*)&req, sizeof(req), resp, &resp_len);
    if (rc && (resp_len != len)) {
        trusty_error("Failed to generate RNG!\n");
        goto exit;
    }
    memcpy(buf, resp, resp_len);
    rc = TRUSTY_ERR_NONE;

exit:
    if (resp)
        free(resp);

    return rc;
}

int hwcrypto_gen_bkek(uint32_t buf, uint32_t len)
{
    hwcrypto_bkek_msg req;
    unsigned long start, end;

    /* check the address */
    if (buf == 0)
        return TRUSTY_ERR_INVALID_ARGS;
    /* fill the request buffer */
    req.buf = buf;
    req.len = len;

    /* invalidate dcache for output buffer */
    start = (unsigned long)buf & ~(ARCH_DMA_MINALIGN - 1);
    end   = ALIGN((unsigned long)buf + len, ARCH_DMA_MINALIGN);
    invalidate_dcache_range(start, end);

    int rc = hwcrypto_do_tipc(HWCRYPTO_GEN_BKEK, (void*)&req,
                              sizeof(req), NULL, 0);

    /* invalidate the dcache again before read to avoid coherency
     * problem caused by speculative memory access by the CPU.
     */
    invalidate_dcache_range(start, end);
    return rc;
}

int hwcrypto_lock_boot_state(void)
{
    return hwcrypto_do_tipc(HWCRYPTO_LOCK_BOOT_STATE, NULL, 0, NULL, 0);
}

int hwcrypto_provision_wv_key(const char *data, uint32_t data_size)
{
    uint8_t *req = NULL, *tmp;
    /* sanity check */
    if (!data || !data_size)
        return TRUSTY_ERR_INVALID_ARGS;

    /* serialize the request */
    req = trusty_calloc(data_size + sizeof(data_size), 1);
    if (!req) {
        return TRUSTY_ERR_NO_MEMORY;
    }
    tmp = append_sized_buf_to_buf(req, (uint8_t *)data, data_size);

    int rc = hwcrypto_do_tipc(HWCRYPTO_PROVISION_WV_KEY, (void*)req,
                              data_size + sizeof(data_size), NULL, 0);

    if (req)
        trusty_free(req);

    return rc;
}

int hwcrypto_provision_wv_key_enc(const char *data, uint32_t data_size)
{
    uint8_t *req = NULL, *tmp;
    /* sanity check */
    if (!data || !data_size)
        return TRUSTY_ERR_INVALID_ARGS;

    /* serialize the request */
    req = trusty_calloc(data_size + sizeof(data_size), 1);
    if (!req) {
        return TRUSTY_ERR_NO_MEMORY;
    }
    tmp = append_sized_buf_to_buf(req, (uint8_t *)data, data_size);

    int rc = hwcrypto_do_tipc(HWCRYPTO_PROVISION_WV_KEY_ENC, (void*)req,
                              data_size + sizeof(data_size), NULL, 0);

    if (req)
        trusty_free(req);

    return rc;
}

#define CAAM_KB_HEADER_LEN 48
#define HAB_DEK_BLOB_HEADER_LEN 8
int hwcrypto_gen_dek_blob(char *data, uint32_t *data_size)
{
    uint8_t *req = NULL, *resp = NULL, *tmp = NULL;
    uint32_t out_data_size;

    /* sanity check */
    if (!data || ((*data_size != 16) && (*data_size != 24) && (*data_size != 32))) {
        trusty_error("Wrong input parameters!\n");
        return TRUSTY_ERR_INVALID_ARGS;
    }

    /* serialize the request */
    req = trusty_calloc(*data_size + sizeof(*data_size), 1);
    if (!req) {
        return TRUSTY_ERR_NO_MEMORY;
    }
    tmp = append_sized_buf_to_buf(req, (uint8_t *)data, *data_size);

    /* allocate memory for result */
    out_data_size = *data_size + CAAM_KB_HEADER_LEN + HAB_DEK_BLOB_HEADER_LEN;
    resp = trusty_calloc(out_data_size + sizeof(*data_size), 1);
    if (!resp) {
        goto exit;
    }

    int rc = hwcrypto_do_tipc(HWCRYPTO_GEN_DEK_BLOB, (void*)req,
                              *data_size + sizeof(data_size), resp, &out_data_size);
    if (!rc) {
        memcpy(data, resp, out_data_size);
        *data_size = out_data_size;
    }

exit:
    if (req)
        trusty_free(req);

    if (resp)
        trusty_free(resp);

    return rc;
}

#define INVALID_MMC_ID 100
#define RPMB_EMMC_CID_SIZE 16
#define RPMB_CID_PRV_OFFSET 9
#define RPMB_CID_CRC_OFFSET 15
int hwcrypto_commit_emmc_cid(void)
{
    struct mmc *mmc = NULL;
    uint8_t cid[RPMB_EMMC_CID_SIZE];
    uint8_t *req = NULL, *tmp;
    int emmc_dev, i;

    emmc_dev = env_get_ulong("emmc_dev", 10, INVALID_MMC_ID);
    if (emmc_dev == INVALID_MMC_ID) {
        trusty_error("environment variable 'emmc_dev' is not set!\n");
        return TRUSTY_ERR_GENERIC;
    }
    mmc = find_mmc_device(emmc_dev);
    if (!mmc) {
        trusty_error("failed to get mmc device.\n");
        return TRUSTY_ERR_GENERIC;
    }
    if (!(mmc->has_init) && mmc_init(mmc)) {
        trusty_error("failed to init eMMC device.\n");
        return TRUSTY_ERR_GENERIC;
    }
    /* Get mmc card id (in big endian) */
    /*
     * PRV/CRC would be changed when doing eMMC FFU
     * The following fields should be masked off when deriving RPMB key
     *
     * CID [55: 48]: PRV (Product revision)
     * CID [07: 01]: CRC (CRC7 checksum)
     * CID [00]: not used
     */
    for (i = 0; i < ARRAY_SIZE(mmc->cid); i++)
        ((uint32_t *)cid)[i] = cpu_to_be32(mmc->cid[i]);

    memset(cid + RPMB_CID_PRV_OFFSET, 0, 1);
    memset(cid + RPMB_CID_CRC_OFFSET, 0, 1);

    /* serialize the request */
    req = trusty_calloc(RPMB_EMMC_CID_SIZE + sizeof(uint32_t), 1);
    if (!req) {
        return TRUSTY_ERR_NO_MEMORY;
    }
    tmp = append_sized_buf_to_buf(req, cid, RPMB_EMMC_CID_SIZE);

    int rc = hwcrypto_do_tipc(HWCRYPTO_SET_EMMC_CID, (void*)req,
                              RPMB_EMMC_CID_SIZE + sizeof(uint32_t), NULL, 0);

    if (req)
        trusty_free(req);

    return rc;
}

int hwcrypto_provision_firmware_sign_key(const char *data, uint32_t data_size)
{
    uint8_t *req = NULL, *tmp;
    /* sanity check */
    if (!data || !data_size)
        return TRUSTY_ERR_INVALID_ARGS;

    /* serialize the request */
    req = trusty_calloc(data_size + sizeof(data_size), 1);
    if (!req) {
        return TRUSTY_ERR_NO_MEMORY;
    }
    tmp = append_sized_buf_to_buf(req, (uint8_t *)data, data_size);

    int rc = hwcrypto_do_tipc(HWCRYPTO_PROVISION_FIRMWARE_SIGN_KEY, (void*)req,
                              data_size + sizeof(data_size), NULL, 0);

    if (req)
        trusty_free(req);

    return rc;
}

int hwcrypto_provision_firmware_encrypt_key(const char *data, uint32_t data_size)
{
    uint8_t *req = NULL, *tmp;
    /* sanity check */
    if (!data || !data_size)
        return TRUSTY_ERR_INVALID_ARGS;

    /* serialize the request */
    req = trusty_calloc(data_size + sizeof(data_size), 1);
    if (!req) {
        return TRUSTY_ERR_NO_MEMORY;
    }
    tmp = append_sized_buf_to_buf(req, (uint8_t *)data, data_size);

    int rc = hwcrypto_do_tipc(HWCRYPTO_PROVISION_FIRMWARE_ENCRYPT_KEY, (void*)req,
                              data_size + sizeof(data_size), NULL, 0);

    if (req)
        trusty_free(req);

    return rc;
}
