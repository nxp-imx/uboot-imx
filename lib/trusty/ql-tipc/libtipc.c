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
#include <env.h>
#include <trusty/imx_snvs.h>
#include <trusty/matter.h>

#define LOCAL_LOG 0

typedef uintptr_t vaddr_t;

static struct trusty_ipc_dev *_ipc_dev;
static struct trusty_dev _tdev; /* There should only be one trusty device */
static void *rpmb_ctx;
bool rpmbkey_is_set(void);

void rpmb_storage_put_ctx(void *dev);
void trusty_ipc_shutdown(void)
{
    (void)rpmb_storage_proxy_shutdown(_ipc_dev);
    (void)rpmb_storage_put_ctx(rpmb_ctx);

#ifndef CONFIG_IMX_MATTER_TRUSTY
    (void)avb_tipc_shutdown(_ipc_dev);
    (void)km_tipc_shutdown(_ipc_dev);
#endif

#ifdef CONFIG_IMX_MATTER_TRUSTY
    (void)matter_tipc_shutdown(_ipc_dev);
#endif

    (void)hwcrypto_tipc_shutdown(_ipc_dev);

    /* shutdown Trusty IPC device */
    (void)trusty_ipc_dev_shutdown(_ipc_dev);

    /* shutdown Trusty device */
    (void)trusty_dev_shutdown(&_tdev);
}

int trusty_ipc_init(void)
{
    int rc;
    bool use_keystore = true;
    /* init Trusty device */
    trusty_info("Initializing Trusty device\n");
    rc = trusty_dev_init(&_tdev, NULL);
    if (rc != 0) {
        trusty_error("Initializing Trusty device failed (%d)\n", rc);
        return rc;
    }

    /* create Trusty IPC device */
    trusty_info("Initializing Trusty IPC device\n");
    rc = trusty_ipc_dev_create(&_ipc_dev, &_tdev, PAGE_SIZE);
    if (rc != 0) {
        trusty_error("Initializing Trusty IPC device failed (%d)\n", rc);
        return rc;
    }

    trusty_info("Initializing Trusty Hardware Crypto client\n");
    rc = hwcrypto_tipc_init(_ipc_dev);
    if (rc != 0) {
        trusty_error("Initlializing Trusty hwcrypto client failed (%d)\n", rc);
        return rc;
    }
#ifdef CONFIG_IMX9
    // pass eMMC card ID to Trusty OS
    hwcrypto_commit_emmc_cid();
#endif

    /* get storage rpmb */
    rpmb_ctx = rpmb_storage_get_ctx();

    /* start secure storage proxy service */
    trusty_info("Initializing RPMB storage proxy service\n");
    rc = rpmb_storage_proxy_init(_ipc_dev, rpmb_ctx);
    if (rc != 0) {
        trusty_error("Initlializing RPMB storage proxy service failed (%d)\n", rc);
#ifndef CONFIG_IMX_MATTER_TRUSTY
        /* check if rpmb key has been fused. */
        if(rpmbkey_is_set()) {
            /* Go to hang if the key has been destroyed. */
            trusty_error("RPMB key was destroyed!\n");
            hang();
        }
#endif
    }

    /**
     * The proxy service can return success even the storage initialization
     * failed (when the rpmb key not set). Init the avb and keymaster service
     * only when the rpmb key has been set.
     */
#ifndef CONFIG_IMX_MATTER_TRUSTY
    if (rpmbkey_is_set()) {
        rc = avb_tipc_init(_ipc_dev);
        if (rc != 0) {
            trusty_error("Initlializing Trusty AVB client failed (%d)\n", rc);
            return rc;
        }

        trusty_info("Initializing Trusty Keymaster client\n");
        rc = km_tipc_init(_ipc_dev);
        if (rc != 0) {
            trusty_error("Initlializing Trusty Keymaster client failed (%d)\n", rc);
            return rc;
        }
    } else
        use_keystore = false;

#ifdef CONFIG_IMX8M
    trusty_info("Initializing Trusty SNVS driver\n");
    rc = imx_snvs_init(_ipc_dev);
    if (rc != 0) {
        trusty_error("Initlializing Trusty SNVS driver failed (%d)\n", rc);
        return rc;
    }
#endif
#endif /* CONFIG_IMX_MATTER_TRUSTY */

#ifdef CONFIG_IMX_MATTER_TRUSTY
    rc = matter_tipc_init(_ipc_dev);
    if (rc != 0) {
        trusty_error("Initlializing Trusty Matter failed (%d)\n", rc);
        return rc;
    }
#endif

    /* secure storage service init ok, use trusty backed keystore */
    if (use_keystore)
        env_set("keystore", "trusty");

    return TRUSTY_ERR_NONE;
}
