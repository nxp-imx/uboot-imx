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

#include <common.h>
#include <trusty/avb.h>
#include <trusty/hwcrypto.h>
#include <trusty/keymaster.h>
#include <trusty/rpmb.h>
#include <trusty/trusty_dev.h>
#include <trusty/trusty_ipc.h>
#include <trusty/util.h>
#include <hang.h>

#define LOCAL_LOG 0

typedef uintptr_t vaddr_t;

static struct trusty_ipc_dev *_ipc_dev;
static struct trusty_dev _tdev; /* There should only be one trusty device */
static void *rpmb_ctx;
#ifndef CONFIG_AVB_ATX
bool rpmbkey_is_set(void);
#endif

void rpmb_storage_put_ctx(void *dev);
void trusty_ipc_shutdown(void)
{
    (void)rpmb_storage_proxy_shutdown(_ipc_dev);
    (void)rpmb_storage_put_ctx(rpmb_ctx);

    (void)avb_tipc_shutdown(_ipc_dev);
    (void)km_tipc_shutdown(_ipc_dev);

#ifndef CONFIG_AVB_ATX
    (void)hwcrypto_tipc_shutdown(_ipc_dev);
#endif

    /* shutdown Trusty IPC device */
    (void)trusty_ipc_dev_shutdown(_ipc_dev);

    /* shutdown Trusty device */
    (void)trusty_dev_shutdown(&_tdev);
}

int trusty_ipc_init(void)
{
    int rc;
    /* init Trusty device */
    trusty_info("Initializing Trusty device\n");
    rc = trusty_dev_init(&_tdev, NULL);
    if (rc != 0) {
        trusty_error("Initializing Trusty device failed (%d)\n", rc);
        return rc;
    }

    /* create Trusty IPC device */
    rc = trusty_ipc_dev_create(&_ipc_dev, &_tdev, PAGE_SIZE);
    if (rc != 0) {
        trusty_error("Initializing Trusty IPC device failed (%d)\n", rc);
        return rc;
    }

    /* get storage rpmb */
    rpmb_ctx = rpmb_storage_get_ctx();

    /* start secure storage proxy service */
    rc = rpmb_storage_proxy_init(_ipc_dev, rpmb_ctx);
    if (rc != 0) {
        trusty_error("Initlializing RPMB storage proxy service failed (%d)\n",
                     rc);
#ifndef CONFIG_AVB_ATX
        /* check if rpmb key has been fused. */
        if(rpmbkey_is_set()) {
            /* Go to hang if the key has been destroyed. */
            trusty_error("RPMB key was destroyed!\n");
            hang();
        }
#else
    return rc;
#endif
    } else {
        /* secure storage service init ok, use trusty backed keystore */
        env_set("keystore", "trusty");

        rc = avb_tipc_init(_ipc_dev);
        if (rc != 0) {
            trusty_error("Initlializing Trusty AVB client failed (%d)\n", rc);
            return rc;
        }

        rc = km_tipc_init(_ipc_dev);
        if (rc != 0) {
            trusty_error("Initlializing Trusty Keymaster client failed (%d)\n", rc);
            return rc;
        }
    }

#ifndef CONFIG_AVB_ATX
    rc = hwcrypto_tipc_init(_ipc_dev);
    if (rc != 0) {
        trusty_error("Initlializing Trusty Keymaster client failed (%d)\n", rc);
        return rc;
    }
#endif

    return TRUSTY_ERR_NONE;
}
