/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2014 Freescale Semiconductor, Inc.
 *
 * Configuration settings for the Freescale i.MX6SX Sabreauto board.
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include "mx6_common.h"
#include "imx_env.h"

#define CFG_MXC_UART_BASE		UART1_BASE

#ifdef CONFIG_NAND_BOOT
#define MFG_NAND_PARTITION "mtdparts=gpmi-nand:64m(nandboot),16m(nandkernel),16m(nanddtb),16m(nandtee),-(nandrootfs)"
#else
#define MFG_NAND_PARTITION ""
#endif

#ifdef CONFIG_IMX_BOOTAUX

/* Set to QSPI1 B flash at default */
#ifdef CONFIG_DM_SPI
#define CFG_SYS_AUXCORE_BOOTDATA 0x68000000
#define SF_QSPI1_B_CS_NUM 2
#else
#define CFG_SYS_AUXCORE_BOOTDATA 0x62000000
#define SF_QSPI1_B_CS_NUM 1
#endif


#define UPDATE_M4_ENV \
	"m4image=m4_qspi.bin\0" \
	"m4_qspi_cs="__stringify(SF_QSPI1_B_CS_NUM)"\0" \
	"loadm4image=fatload mmc ${mmcdev}:${mmcpart} ${loadaddr} ${m4image}\0" \
	"update_m4_from_sd=" \
		"if sf probe 0:${m4_qspi_cs}; then " \
			"if run loadm4image; then " \
				"setexpr fw_sz ${filesize} + 0xffff; " \
				"setexpr fw_sz ${fw_sz} / 0x10000; "	\
				"setexpr fw_sz ${fw_sz} * 0x10000; "	\
				"sf erase 0x0 ${fw_sz}; " \
				"sf write ${loadaddr} 0x0 ${filesize}; " \
			"fi; " \
		"fi\0" \
	"m4boot=sf probe 0:${m4_qspi_cs}; bootaux "__stringify(CFG_SYS_AUXCORE_BOOTDATA)"\0"
#else
#define UPDATE_M4_ENV ""
#endif


#define CFG_MFG_ENV_SETTINGS \
	CFG_MFG_ENV_SETTINGS_DEFAULT \
	"initrd_addr=0x86800000\0" \
	"initrd_high=0xffffffff\0" \
	"sd_dev=2\0" \
	"mtdparts=" MFG_NAND_PARTITION \
	"\0"\

#if defined(CONFIG_NAND_BOOT)
#define CFG_EXTRA_ENV_SETTINGS \
	CFG_MFG_ENV_SETTINGS \
	TEE_ENV \
	"tee_addr=0x84000000\0" \
	"splashimage=0x8c000000\0" \
	"fdt_addr=0x83000000\0" \
	"fdt_high=0xffffffff\0"	  \
	"console=ttymxc0\0" \
	"bootargs=console=ttymxc0,115200 ubi.mtd=nandrootfs "  \
		"root=ubi0:nandrootfs rootfstype=ubifs "		     \
		MFG_NAND_PARTITION \
		"\0" \
	"bootcmd=nand read ${loadaddr} 0x4000000 0xc00000;"\
		"nand read ${fdt_addr} 0x5000000 0x100000;"\
		"if test ${tee} = yes; then " \
			"nand read ${tee_addr} 0x6000000 0x400000;"\
			"bootm ${tee_addr} - ${fdt_addr};" \
		"else " \
			"bootz ${loadaddr} - ${fdt_addr};" \
		"fi\0"

#else
#define CFG_EXTRA_ENV_SETTINGS \
	UPDATE_M4_ENV \
	CFG_MFG_ENV_SETTINGS \
	TEE_ENV \
	"script=boot.scr\0" \
	"image=zImage\0" \
	"console=ttymxc0\0" \
	"fdt_high=0xffffffff\0" \
	"initrd_high=0xffffffff\0" \
	"fdt_file=undefined\0" \
	"fdt_addr=0x83000000\0" \
	"tee_addr=0x84000000\0" \
	"tee_file=uTee-6sxauto\0" \
	"boot_fdt=try\0" \
	"ip_dyn=yes\0" \
	"splashimage=0x8c000000\0" \
	"mmcdev="__stringify(CONFIG_SYS_MMC_ENV_DEV)"\0" \
	"mmcpart=1\0" \
	"mmcroot=/dev/mmcblk2p2 rootwait rw\0" \
	"mmcautodetect=yes\0" \
	"mmcargs=setenv bootargs console=${console},${baudrate} " \
		"root=${mmcroot}\0" \
	"loadbootscript=" \
		"fatload mmc ${mmcdev}:${mmcpart} ${loadaddr} ${script};\0" \
	"bootscript=echo Running bootscript from mmc ...; " \
		"source\0" \
	"loadimage=fatload mmc ${mmcdev}:${mmcpart} ${loadaddr} ${image}\0" \
	"loadfdt=fatload mmc ${mmcdev}:${mmcpart} ${fdt_addr} ${fdt_file}\0" \
	"loadtee=fatload mmc ${mmcdev}:${mmcpart} ${tee_addr} ${tee_file}\0" \
	"mmcboot=echo Booting from mmc ...; " \
		"run mmcargs; " \
		"if test ${tee} = yes; then " \
			"run loadfdt; run loadtee; bootm ${tee_addr} - ${fdt_addr}; " \
		"else " \
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
			"fi; " \
		"fi;\0" \
	"netargs=setenv bootargs console=${console},${baudrate} " \
		"root=/dev/nfs " \
	"ip=dhcp nfsroot=${serverip}:${nfsroot},v3,tcp\0" \
		"netboot=echo Booting from net ...; " \
		"run netargs; " \
		"if test ${ip_dyn} = yes; then " \
			"setenv get_cmd dhcp; " \
		"else " \
			"setenv get_cmd tftp; " \
		"fi; " \
		"${get_cmd} ${image}; " \
		"if test ${tee} = yes; then " \
			"${get_cmd} ${tee_addr} ${tee_file}; " \
			"${get_cmd} ${fdt_addr} ${fdt_file}; " \
			"bootm ${tee_addr} - ${fdt_addr}; " \
		"else " \
			"if test ${boot_fdt} = yes || test ${boot_fdt} = try; then " \
				"if ${get_cmd} ${fdt_addr} ${fdt_file}; then " \
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
			"fi;" \
		"fi;\0" \
	"findfdt="\
		"if test $fdt_file = undefined; then " \
			"setenv fdt_file imx6sx-sabreauto.dtb; " \
		"fi;\0" \

#endif

/* Miscellaneous configurable options */

/* Physical Memory Map */
#define PHYS_SDRAM			MMDC0_ARB_BASE_ADDR

#define CFG_SYS_SDRAM_BASE		PHYS_SDRAM
#define CFG_SYS_INIT_RAM_ADDR	IRAM_BASE_ADDR
#define CFG_SYS_INIT_RAM_SIZE	IRAM_SIZE

/* MMC Configuration */
#define CFG_SYS_FSL_ESDHC_ADDR	USDHC3_BASE_ADDR

/* NAND stuff */
#define CFG_SYS_NAND_BASE           0x40000000

/* DMA stuff, needed for GPMI/MXS NAND support */

/* Network */

#ifdef CONFIG_CMD_USB
#define CFG_MXC_USB_PORTSC  (PORT_PTS_UTMI | PORT_PTS_PTW)
#define CFG_MXC_USB_FLAGS   0
#endif

#define CFG_SYS_FSL_USDHC_NUM	2

#define CFG_SYS_I2C_PCA953X_WIDTH	{ {0x30, 8}, {0x32, 8}, {0x34, 8} }

#endif				/* __CONFIG_H */
