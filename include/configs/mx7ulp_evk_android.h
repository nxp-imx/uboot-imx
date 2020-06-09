/*
 * Copyright 2020 NXP
 *
 */

#ifndef __MX7ULP_EVK_ANDROID_H
#define __MX7ULP_EVK_ANDROID_H

#define FSL_FASTBOOT_FB_DEV "mmc"

#define CONFIG_SHA1

#ifdef CONFIG_SYS_CBSIZE
#undef CONFIG_SYS_CBSIZE
#define CONFIG_SYS_CBSIZE 2048
#endif

#ifdef CONFIG_SYS_MAXARGS
#undef CONFIG_SYS_MAXARGS
#define CONFIG_SYS_MAXARGS     64
#endif

#ifdef CONFIG_SYS_MALLOC_LEN
#undef CONFIG_SYS_MALLOC_LEN
#define CONFIG_SYS_MALLOC_LEN           (96 * SZ_1M)
#endif

#undef CONFIG_EXTRA_ENV_SETTINGS
#undef CONFIG_BOOTCOMMAND

#define CONFIG_EXTRA_ENV_SETTINGS       \
	"splashpos=m,m\0"		\
	"splashimage=0x78000000\0"	\
	"fdt_high=0xffffffff\0"		\
	"initrd_high=0xffffffff\0"	\

/* Enable mcu firmware flash */
#ifdef CONFIG_FLASH_MCUFIRMWARE_SUPPORT
#define ANDROID_MCU_FRIMWARE_DEV_TYPE DEV_SF
#define ANDROID_MCU_FIRMWARE_START 0
#define ANDROID_MCU_FIRMWARE_SIZE  0x20000
#endif

#endif
