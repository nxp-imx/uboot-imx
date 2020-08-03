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
#include <trusty/trusty_dev.h>
#include <trusty/util.h>

#include <common.h>
#include <memalign.h>
#include <mmc.h>

void *rpmb_storage_get_ctx(void)
{
    /* Unused for U-boot */
    return NULL;
}

void rpmb_storage_put_ctx(void *dev)
{
}

int rpmb_storage_send(void *rpmb_dev, const void *rel_write_data,
                      size_t rel_write_size, const void *write_data,
                      size_t write_size, void *read_buf, size_t read_size)
{
    ALLOC_CACHE_ALIGN_BUFFER(uint8_t, rpmb_rel_write_data, rel_write_size);
    ALLOC_CACHE_ALIGN_BUFFER(uint8_t, rpmb_write_data, write_size);
    ALLOC_CACHE_ALIGN_BUFFER(uint8_t, rpmb_read_data, read_size);
    int ret = TRUSTY_ERR_NONE;
    struct mmc *mmc = find_mmc_device(mmc_get_env_dev());
    if (!mmc) {
	trusty_error("failed to get mmc device.\n");
	return -1;
    }
    struct blk_desc *desc = mmc_get_blk_desc(mmc);
    if (!desc) {
	trusty_error("failed to get mmc desc.\n");
	return -1;
    }
    char original_part = desc->hwpart;

    /* Switch to RPMB partition */
    if (desc->hwpart != MMC_PART_RPMB) {
        ret = mmc_switch_part(mmc, MMC_PART_RPMB);
        if (ret) {
            trusty_error("failed to switch to RPMB partition\n");
            ret = TRUSTY_ERR_GENERIC;
            goto end;
        }
        desc->hwpart = MMC_PART_RPMB;
    }

    if (rel_write_size) {
        if (rel_write_size % MMC_BLOCK_SIZE) {
            trusty_error(
                "rel_write_size is not a multiple of MMC_BLOCK_SIZE: %lu\n",
                 rel_write_size);
            ret = TRUSTY_ERR_INVALID_ARGS;
            goto end;
        }
        trusty_memcpy(rpmb_rel_write_data, rel_write_data, rel_write_size);
        ret = mmc_rpmb_request(mmc,
                               (const struct s_rpmb *)rpmb_rel_write_data,
                               rel_write_size / MMC_BLOCK_SIZE, true);
        if (ret) {
            trusty_error("failed to execute rpmb reliable write\n");
            goto end;
        }
    }
    if (write_size) {
        if (write_size % MMC_BLOCK_SIZE) {
            trusty_error("write_size is not a multiple of MMC_BLOCK_SIZE: %lu\n",
                         write_size);
            ret = TRUSTY_ERR_INVALID_ARGS;
            goto end;
        }
        trusty_memcpy(rpmb_write_data, write_data, write_size);
        ret = mmc_rpmb_request(mmc, (const struct s_rpmb *)rpmb_write_data,
                               write_size / MMC_BLOCK_SIZE, false);
        if (ret) {
            trusty_error("failed to execute rpmb write\n");
            goto end;
        }
    }
    if (read_size) {
        if (read_size % MMC_BLOCK_SIZE) {
            trusty_error("read_size is not a multiple of MMC_BLOCK_SIZE: %lu\n",
                         read_size);
            ret = TRUSTY_ERR_INVALID_ARGS;
            goto end;
        }
        ret = mmc_rpmb_response(mmc, (struct s_rpmb *)rpmb_read_data,
                                read_size / MMC_BLOCK_SIZE, 0);
        trusty_memcpy((void *)read_buf, rpmb_read_data, read_size);
        if (ret < 0) {
            trusty_error("failed to execute rpmb read\n");
        }
    }

end:
    /* Return to original partition */
    if (desc->hwpart != original_part) {
        if (mmc_switch_part(mmc, original_part) != 0) {
            trusty_error("failed to switch back to original partition\n");
            return TRUSTY_ERR_GENERIC;
        }
       desc->hwpart = original_part;
    }
    return ret;
}
