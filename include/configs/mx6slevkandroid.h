
/*
 * Copyright (C) 2013-2015 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef MX6SL_EVK_ANDROID_H
#define MX6SL_EVK_ANDROID_H

#include <asm/imx-common/mxc_key_defs.h>

#define CONFIG_SERIAL_TAG

#define CONFIG_USB_DEVICE
#define CONFIG_IMX_UDC		       1

#define CONFIG_CMD_FASTBOOT
#define CONFIG_FASTBOOT		       1
#define CONFIG_FASTBOOT_VENDOR_ID      0x18d1
#define CONFIG_FASTBOOT_PRODUCT_ID     0x0d02
#define CONFIG_FASTBOOT_BCD_DEVICE     0x311
#define CONFIG_FASTBOOT_MANUFACTURER_STR  "Freescale"
#define CONFIG_FASTBOOT_PRODUCT_NAME_STR "i.mx6sl EVK Board"
#define CONFIG_FASTBOOT_INTERFACE_STR	 "Android fastboot"
#define CONFIG_FASTBOOT_CONFIGURATION_STR  "Android fastboot"
#define CONFIG_FASTBOOT_SERIAL_NUM	"12345"
#define CONFIG_FASTBOOT_SATA_NO		 0

#if defined CONFIG_SYS_BOOT_NAND
#define CONFIG_FASTBOOT_STORAGE_NAND
#elif defined CONFIG_SYS_BOOT_SATA
#define CONFIG_FASTBOOT_STORAGE_SATA
#else
#define CONFIG_FASTBOOT_STORAGE_MMC
#endif

/*  For system.img growing up more than 256MB, more buffer needs
*   to receive the system.img*/
#define CONFIG_FASTBOOT_TRANSFER_BUF	0x8c000000
#define CONFIG_FASTBOOT_TRANSFER_BUF_SIZE 0x19000000 /* 400M byte */


#define CONFIG_CMD_BOOTI
#define CONFIG_ANDROID_RECOVERY
/* which mmc bus is your main storage ? */
#define CONFIG_ANDROID_MAIN_MMC_BUS 2
#define CONFIG_ANDROID_BOOT_PARTITION_MMC 1
#define CONFIG_ANDROID_SYSTEM_PARTITION_MMC 5
#define CONFIG_ANDROID_RECOVERY_PARTITION_MMC 2
#define CONFIG_ANDROID_CACHE_PARTITION_MMC 6
#define CONFIG_ANDROID_DATA_PARTITION_MMC 4

/*keyboard mapping*/
#define CONFIG_VOL_DOWN_KEY     KEY_BACK
#define CONFIG_POWER_KEY        KEY_5

#define CONFIG_MXC_KPD
#define CONFIG_MXC_KEYMAPPING \
	{       \
		KEY_SELECT, KEY_BACK, KEY_1,     KEY_2, \
		KEY_3,      KEY_4,    KEY_5,     KEY_MENU, \
		KEY_6,      KEY_7,    KEY_8,     KEY_9, \
		KEY_UP,     KEY_LEFT, KEY_RIGHT, KEY_DOWN, \
	}
#define CONFIG_MXC_KPD_COLMAX 4
#define CONFIG_MXC_KPD_ROWMAX 4


#undef CONFIG_EXTRA_ENV_SETTINGS
#undef CONFIG_BOOTCOMMAND

#define CONFIG_EXTRA_ENV_SETTINGS					\
	"splashpos=m,m\0"	  \
	"fdt_high=0xffffffff\0"	  \
	"initrd_high=0xffffffff\0" \

#endif
