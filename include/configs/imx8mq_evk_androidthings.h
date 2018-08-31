/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef IMX8MQ_EVK_ANDROID_THINGS_H
#define IMX8MQ_EVK_ANDROID_THINGS_H

#define CONFIG_CMD_READ

#define CONFIG_ANDROID_AB_SUPPORT
#define CONFIG_SUPPORT_EMMC_RPMB
#define CONFIG_SYSTEM_RAMDISK_SUPPORT
#define CONFIG_AVB_FUSE_BANK_SIZEW 4
#define CONFIG_AVB_FUSE_BANK_START 14
#define CONFIG_AVB_FUSE_BANK_END 14
#define CONFIG_FASTBOOT_LOCK
#define FSL_FASTBOOT_FB_DEV "mmc"

#ifdef CONFIG_AVB_ATX
#define UNLOCK_VBOOT_STATUS_OFFSET_IN_WORD 0
#define UNLOCK_VBOOT_STATUS_OFFSET_IN_BIT 0
#endif

#define CONFIG_ENABLE_LOCKSTATUS_SUPPORT

#ifdef CONFIG_SYS_MALLOC_LEN
#undef CONFIG_SYS_MALLOC_LEN
#define CONFIG_SYS_MALLOC_LEN           (64 * SZ_1M)
#endif

#define CONFIG_ANDROID_RECOVERY

/* Use below address to store the bootloader rbindex,
 * it's 4k offset to CONFIG_SYS_SPL_PTE_RAM_BASE
 * */
#if defined(CONFIG_AVB_ATX) && defined(CONFIG_DUAL_BOOTLOADER)
#define BL_RBINDEX_LOAD_ADDR 0x4157F000
#endif

#define CONFIG_CMD_BOOTA
#define CONFIG_SUPPORT_RAW_INITRD
#define CONFIG_SERIAL_TAG

#undef CONFIG_EXTRA_ENV_SETTINGS
#undef CONFIG_BOOTCOMMAND

#define CONFIG_EXTRA_ENV_SETTINGS		\
	"splashpos=m,m\0"			\
	"fdt_high=0xffffffffffffffff\0"		\
	"initrd_high=0xffffffffffffffff\0"	\

#define AVB_AB_I_UNDERSTAND_LIBAVB_AB_IS_DEPRECATED

#define ENTERPRISE_MICRON_1G   0x5c
#define ENTERPRISE_HYNIX_1G    0x56
#define ENTERPRISE_MIRCONB_3G  0x40
#define WIBO_3G                0x00
#define IMX8M_REF_3G           0x00

#define TEE_LOAD_ADDR_1G 0x7e000000
#define TEE_LOAD_ADDR_3G 0xfe000000


#if defined(CONFIG_SPL_BUILD) || defined(CONFIG_IMX_TRUSTY_OS)
#define AVB_RPMB
#endif

#define KEYSLOT_HWPARTITION_ID   2
#define KEYSLOT_BLKS             0x1FFF

#ifdef CONFIG_SPL_BUILD

#define CONFIG_SPL_SHA256
#undef CONFIG_ENV_IS_IN_MMC
#undef CONFIG_BLK
#define CONFIG_ENV_IS_NOWHERE

#ifdef AVB_RPMB
#define BOOTLOADER_RBIDX_OFFSET  0x1E000
#define BOOTLOADER_RBIDX_START   0x1F000
#define BOOTLOADER_RBIDX_LEN     0x08
#define BOOTLOADER_RBIDX_INITVAL 0
#endif

#else
/* imx8m won't touch CAAM in non-secure world. */
#undef CONFIG_FSL_CAAM_KB
#endif

/* Enlarge the spl max size to 172k */
#ifdef CONFIG_SPL_MAX_SIZE
#undef CONFIG_SPL_MAX_SIZE
#define CONFIG_SPL_MAX_SIZE 176128
#endif

#ifdef CONFIG_IMX_TRUSTY_OS
#ifdef CONFIG_DUAL_BOOTLOADER
#define ANDROID_SPL_SIZE 0x80000
#endif
#endif

#endif /* IMX8MQ_EVK_ANDROID_THINGS_H */
