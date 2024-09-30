/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef IMX95_VERDIN_ANDROID_H
#define IMX95_VERDIN_ANDROID_H

#define FSL_FASTBOOT_FB_DEV "mmc"

#undef CFG_EXTRA_ENV_SETTINGS
#undef CONFIG_BOOTCOMMAND

#define CFG_EXTRA_ENV_SETTINGS          \
        "splashpos=m,m\0"                       \
        "splashimage=0x9FFF0000\0" \
        "fdt_high=0xffffffffffffffff\0"         \
        "initrd_high=0xffffffffffffffff\0"      \
        "emmc_dev=0\0"\
        "sd_dev=1\0" \

#define CFG_SYS_SPL_PTE_RAM_BASE    0x901F8000

#ifdef CONFIG_IMX_TRUSTY_OS
#define BOOTLOADER_RBIDX_OFFSET  0x3FE000
#define BOOTLOADER_RBIDX_START   0x3FF000
#define BOOTLOADER_RBIDX_LEN     0x08
#define BOOTLOADER_RBIDX_INITVAL 0
#endif

#ifdef CONFIG_IMX_TRUSTY_OS
#define AVB_RPMB
#define KEYSLOT_HWPARTITION_ID 2
#define KEYSLOT_BLKS             0x1FFF
#define NS_ARCH_ARM64 1
#endif

#endif /* IMX95_VERDIN_ANDROID_H */
