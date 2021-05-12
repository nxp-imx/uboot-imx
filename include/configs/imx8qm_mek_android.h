/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef IMX8QM_MEK_ANDROID_H
#define IMX8QM_MEK_ANDROID_H

#define CONFIG_USB_GADGET_VBUS_DRAW	2

#define FSL_FASTBOOT_FB_DEV "mmc"

#define IMX_HDMI_FIRMWARE_LOAD_ADDR (CONFIG_SYS_SDRAM_BASE + SZ_64M)
#define IMX_HDMITX_FIRMWARE_SIZE 0x20000
#define IMX_HDMIRX_FIRMWARE_SIZE 0x20000

#undef CONFIG_EXTRA_ENV_SETTINGS
#undef CONFIG_BOOTCOMMAND

#define CONFIG_EXTRA_ENV_SETTINGS		\
	"splashpos=m,m\0"	  		\
	"splashimage=0x9e000000\0" 		\
	"fdt_high=0xffffffffffffffff\0"	  	\
	"initrd_high=0xffffffffffffffff\0" 	\

#ifdef CONFIG_IMX_TRUSTY_OS
#define NS_ARCH_ARM64 1
#define KEYSLOT_HWPARTITION_ID   2
#define KEYSLOT_BLKS             0x3FFF
#define AVB_RPMB

#ifdef CONFIG_ID_ATTESTATION
#define ATTESTATION_ID_BRAND "Android"
#define ATTESTATION_ID_DEVICE "mek_8q"
#define ATTESTATION_ID_MANUFACTURER "nxp"
#define ATTESTATION_ID_MODEL "MEK-MX8Q"
#ifdef CONFIG_ATTESTATION_ID_PRODUCT
#undef CONFIG_ATTESTATION_ID_PRODUCT
#endif
#define CONFIG_ATTESTATION_ID_PRODUCT "mek_8q"
#endif

#endif

#endif /* IMX8QM_MEK_ANDROID_H */
