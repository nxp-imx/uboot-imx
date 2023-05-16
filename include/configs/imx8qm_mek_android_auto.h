/*
 * Copyright 2020 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef IMX8QM_MEK_ANDROID_AUTO_H
#define IMX8QM_MEK_ANDROID_AUTO_H

/* USB OTG controller configs */
#ifdef CONFIG_USB_EHCI_HCD
#ifndef CFG_MXC_USB_PORTSC
#define CFG_MXC_USB_PORTSC		(PORT_PTS_UTMI | PORT_PTS_PTW)
#endif
#endif

#define FSL_FASTBOOT_FB_DEV "mmc"

#define IMX_HDMI_FIRMWARE_LOAD_ADDR (CFG_SYS_SDRAM_BASE + SZ_64M)
#define IMX_HDMITX_FIRMWARE_SIZE 0x20000
#define IMX_HDMIRX_FIRMWARE_SIZE 0x20000

#undef CFG_EXTRA_ENV_SETTINGS
#define CFG_EXTRA_ENV_SETTINGS					\
	"splashpos=m,m\0"	  \
	"fdt_high=0xffffffffffffffff\0"	  \
	"initrd_high=0xffffffffffffffff\0" \
	"emmc_dev=0\0" \
	"sd_dev=1\0"

#ifdef CONFIG_IMX_TRUSTY_OS
#define AVB_RPMB
#define NS_ARCH_ARM64 1
#define KEYSLOT_HWPARTITION_ID	2
#define KEYSLOT_BLKS		0x3FFF

#define BOOTLOADER_RBIDX_OFFSET  0x3FE000
#define BOOTLOADER_RBIDX_START   0x3FF000
#define BOOTLOADER_RBIDX_LEN     0x08
#define BOOTLOADER_RBIDX_INITVAL 0
#define CFG_SYS_SPL_PTE_RAM_BASE 0x801F8000
#endif

#if defined(CONFIG_XEN)
#include "imx8qm_mek_android_auto_xen.h"
#endif

#endif /* IMX8QM_MEK_ANDROID_AUTO_H */
