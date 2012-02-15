/*
 * Copyright (C) 2012 Freescale Semiconductor, Inc.
 *
 * Configuration settings for the MX6Q Sabre Lite2 Freescale board.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef MX6Q_SABRESD_ANDROID_H
#define MX6Q_SABRESD_ANDROID_H

#include "mx6q_sabresd.h"

/* Can't enable OTG on this board, if enable kernel will hang very
 * early stage */
#if 0
#define CONFIG_USB_DEVICE
#define CONFIG_IMX_UDC		       1
#define CONFIG_FASTBOOT		       1
#define CONFIG_FASTBOOT_STORAGE_EMMC_SATA
#define CONFIG_FASTBOOT_VENDOR_ID      0xbb4
#define CONFIG_FASTBOOT_PRODUCT_ID     0xc01
#define CONFIG_FASTBOOT_BCD_DEVICE     0x311
#define CONFIG_FASTBOOT_MANUFACTURER_STR  "Freescale"
#define CONFIG_FASTBOOT_PRODUCT_NAME_STR "i.mx6q arm"
#define CONFIG_FASTBOOT_INTERFACE_STR	 "Android fastboot"
#define CONFIG_FASTBOOT_CONFIGURATION_STR  "Android fastboot"
#define CONFIG_FASTBOOT_SERIAL_NUM	"12345"
#define CONFIG_FASTBOOT_SATA_NO		 0
#define CONFIG_FASTBOOT_TRANSFER_BUF	0x30000000
#define CONFIG_FASTBOOT_TRANSFER_BUF_SIZE 0x10000000 /* 256M byte */
#endif	/* if 0 */

#define CONFIG_ANDROID_RECOVERY
#define CONFIG_ANDROID_SYSTEM_PARTITION_MMC 2
#define CONFIG_ANDROID_RECOVERY_PARTITION_MMC 4
#define CONFIG_ANDROID_CACHE_PARTITION_MMC 6

#define CONFIG_ANDROID_RECOVERY_BOOTARGS_MMC \
	"setenv bootargs ${bootargs} init=/init root=/dev/mmcblk0p4 rootfs=ext4 rootwait enable_wait_mode=off"
#define CONFIG_ANDROID_RECOVERY_BOOTCMD_MMC  \
	"run bootargs_android_recovery; "	\
	"mmc dev 3; "	\
	"mmc read ${loadaddr} 0x800 0x2000;bootm"
#define CONFIG_ANDROID_RECOVERY_CMD_FILE "/recovery/command"
#define CONFIG_INITRD_TAG

#undef CONFIG_LOADADDR
#undef CONFIG_RD_LOADADDR
#undef CONFIG_EXTRA_ENV_SETTINGS

#define CONFIG_LOADADDR		0x10800000	/* loadaddr env var */
#define CONFIG_RD_LOADADDR      0x11000000


#define	CONFIG_EXTRA_ENV_SETTINGS					\
		"netdev=eth0\0"						\
		"ethprime=FEC0\0"					\
		"bootfile=uImage\0"	\
		"bootargs=console=ttymxc0,115200 init=/init rw " \
		"video=mxcfb0 fbmem=10M vmalloc=400M enable_wait_mode=off\0" \
		"bootcmd_SD=mmc dev 3;"		\
			"mmc read ${loadaddr} 0x800 0x2000;" \
			"mmc read ${rd_loadaddr} 0x3000 0x300\0" \
		"bootcmd=run bootcmd_SD; bootm ${loadaddr} ${rd_loadaddr}\0" \


#endif
