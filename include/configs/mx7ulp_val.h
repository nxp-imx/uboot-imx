/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2016 Freescale Semiconductor, Inc.
 * Copyright 2017 NXP
 *
 * Configuration settings for the Freescale i.MX7ULP Validation board.
 */

#ifndef __MX7ULP_VAL_CONFIG_H
#define __MX7ULP_VAL_CONFIG_H

#include <linux/sizes.h>
#include <asm/arch/imx-regs.h>
#include "imx_env.h"

/* Using ULP WDOG for reset */
#define WDOG_BASE_ADDR		 WDG1_RBASE

#define CFG_SYS_HZ_CLOCK		1000000 /* Fixed at 1Mhz from TSTMR */

/* UART */
#ifdef CONFIG_TARGET_MX7ULP_10X10_VAL
#define LPUART_BASE     LPUART6_RBASE
#else
#define LPUART_BASE     LPUART4_RBASE
#endif

/* Miscellaneous configurable options */

/* Physical Memory Map */

#define PHYS_SDRAM			0x60000000
#ifdef CONFIG_TARGET_MX7ULP_10X10_VAL
#define PHYS_SDRAM_SIZE			    SZ_1G   /*LPDDR2 1G*/
#else
#define PHYS_SDRAM_SIZE			    SZ_512M
#endif
#define CFG_SYS_SDRAM_BASE		PHYS_SDRAM

#define CFG_MFG_ENV_SETTINGS \
	CFG_MFG_ENV_SETTINGS_DEFAULT  \
	"initrd_addr=0x66800000\0" \
	"initrd_high=0xffffffff\0" \
	"sd_dev=1\0"

#define CFG_EXTRA_ENV_SETTINGS \
	CFG_MFG_ENV_SETTINGS \
	"script=boot.scr\0" \
	"image=zImage\0" \
	"console=ttyLP0\0" \
	"fdt_high=0xffffffff\0" \
	"initrd_high=0xffffffff\0" \
	"fdt_file=" CONFIG_DEFAULT_FDT_FILE "\0" \
	"fdt_addr=0x63000000\0" \
	"boot_fdt=try\0" \
	"earlycon=lpuart32,0x402D0000\0" \
	"ip_dyn=yes\0" \
	"mmcdev="__stringify(CONFIG_SYS_MMC_ENV_DEV)"\0" \
	"mmcpart=1\0" \
	"mmcroot=/dev/mmcblk1p2 rootwait rw\0" \
	"mmcautodetect=yes\0" \
	"mmcargs=setenv bootargs console=${console},${baudrate} " \
		"root=${mmcroot}\0" \
	"loadbootscript=" \
		"fatload mmc ${mmcdev}:${mmcpart} ${loadaddr} ${script};\0" \
	"bootscript=echo Running bootscript from mmc ...; " \
		"source\0" \
	"loadimage=fatload mmc ${mmcdev}:${mmcpart} ${loadaddr} ${image}\0" \
	"loadfdt=fatload mmc ${mmcdev}:${mmcpart} ${fdt_addr} ${fdt_file}\0" \
	"mmcboot=echo Booting from mmc ...; " \
		"run mmcargs; " \
		"if test ${boot_fdt} = yes || test ${boot_fdt} = try; then " \
			"if run loadfdt; then " \
				"bootz ${loadaddr} - ${fdt_addr}; " \
			"else " \
				"if test ${boot_fdt} = try; then " \
					"bootz; " \
				"else " \
					"echo WARN: Cannot load the DT; " \
				"fi; " \
			"fi; " \
		"else " \
			"bootz; " \
		"fi;\0"


#define CFG_SYS_INIT_RAM_ADDR	IRAM_BASE_ADDR
#define CFG_SYS_INIT_RAM_SIZE	SZ_256K

/* USB Configs */
#define CFG_MXC_USB_PORTSC  (PORT_PTS_UTMI | PORT_PTS_PTW)

#endif	/* __CONFIG_H */
