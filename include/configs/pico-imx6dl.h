/*
 * Copyright (C) 2015 Technexion Ltd.
 * Copyright 2017 NXP
 *
 * Configuration settings for the Technexion Pico i.mx6DL board.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __PICO_IMX6DL_CONFIG_H
#define __PICO_IMX6DL_CONFIG_H

#include <asm/arch/imx-regs.h>
#include <linux/sizes.h>
#include "mx6_common.h"
#include <asm/imx-common/gpio.h>


/* uncomment for SECURE mode support */
/* #define CONFIG_SECURE_BOOT */

#ifdef CONFIG_SECURE_BOOT
#ifndef CONFIG_CSF_SIZE
#define CONFIG_CSF_SIZE 0x4000
#endif
#endif

#undef CONFIG_LDO_BYPASS_CHECK

/* Size of malloc() pool */
#define CONFIG_SYS_MALLOC_LEN		(16 * SZ_1M)

#define CONFIG_BOARD_LATE_INIT

#define CONFIG_MXC_UART
#define CONFIG_MXC_UART_BASE		UART1_BASE

/* allow to overwrite serial and ethaddr */
#define CONFIG_ENV_OVERWRITE
#define CONFIG_CONS_INDEX               1
#define CONFIG_BAUDRATE                 115200


/* MMC Configs */
/* #define CONFIG_SYS_FSL_ESDHC_ADDR	USDHC3_BASE_ADDR */
/* #define CONFIG_SYS_FSL_USDHC_NUM	1 */


/* MMC Configuration */
#define CONFIG_FSL_ESDHC
#define CONFIG_FSL_USDHC
#define CONFIG_SYS_FSL_USDHC_NUM        2
#define CONFIG_SYS_FSL_ESDHC_ADDR       USDHC3_BASE_ADDR



#ifdef CONFIG_CMD_NET
#define CONFIG_FEC_MXC
#define CONFIG_MII
#define CONFIG_FEC_ENET_DEV 1

#if (CONFIG_FEC_ENET_DEV == 0)
#define IMX_FEC_BASE			ENET_BASE_ADDR
#define CONFIG_FEC_MXC_PHYADDR          0x2
#define CONFIG_FEC_XCV_TYPE             RMII
#elif (CONFIG_FEC_ENET_DEV == 1)
#define IMX_FEC_BASE			ENET2_BASE_ADDR
#define CONFIG_FEC_MXC_PHYADDR          0x1  /* need board rework */
#define CONFIG_FEC_XCV_TYPE             RMII
#endif
#define CONFIG_ETHPRIME                 "FEC"

#define CONFIG_PHYLIB
#define CONFIG_PHY_MICREL
#endif

/* I2C configs */
#define CONFIG_CMD_I2C
#ifdef CONFIG_CMD_I2C
#define CONFIG_SYS_I2C
#define CONFIG_SYS_I2C_MXC
#define CONFIG_SYS_I2C_MXC_I2C1		/* enable I2C bus 1 */
#define CONFIG_SYS_I2C_MXC_I2C2		/* enable I2C bus 2 */
#define CONFIG_SYS_I2C_SPEED		100000
#endif

#ifdef CONFIG_DEFAULT_FDT_FILE
#undef CONFIG_DEFAULT_FDT_FILE
#define CONFIG_DEFAULT_FDT_FILE "imx6dl-pico.dtb"
#endif

#define PHYS_SDRAM_SIZE                        SZ_512M

/* PMIC */
#define CONFIG_POWER
#define CONFIG_POWER_I2C
#define CONFIG_POWER_PFUZE100
#define CONFIG_POWER_PFUZE100_I2C_ADDR	0x08


/* Framebuffer */
#define CONFIG_VIDEO_IPUV3
#define CONFIG_SYS_CONSOLE_OVERWRITE_ROUTINE
#define CONFIG_VIDEO_BMP_RLE8
#define CONFIG_SPLASH_SCREEN
#define CONFIG_SPLASH_SCREEN_ALIGN
#define CONFIG_BMP_16BPP
#define CONFIG_VIDEO_LOGO
#define CONFIG_VIDEO_BMP_LOGO
#define CONFIG_IPUV3_CLK 260000000
#define CONFIG_CMD_HDMIDETECT
#define CONFIG_IMX_HDMI
#define CONFIG_IMX_VIDEO_SKIP

#define CONFIG_SYS_MMC_IMG_LOAD_PART	1

#define CONFIG_MFG_ENV_SETTINGS \
	"mfgtool_args=setenv bootargs console=${console},${baudrate} " \
		"rdinit=/linuxrc " \
		"g_mass_storage.stall=0 g_mass_storage.removable=1 " \
		"g_mass_storage.idVendor=0x066F g_mass_storage.idProduct=0x37FF "\
		"g_mass_storage.iSerialNumber=\"\" "\
		"clk_ignore_unused "\
		"\0" \
	"loadaddr=0x12000000\0" \
	"fdt_addr=0x18000000\0" \
	"initrd_addr=0x12C00000\0" \
	"initrd_high=0xffffffff\0" \
	"bootcmd_mfg=run mfgtool_args;bootz ${loadaddr} ${initrd_addr} ${fdt_addr};\0" \


#define CONFIG_EXTRA_ENV_SETTINGS \
	CONFIG_MFG_ENV_SETTINGS \
	"script=boot.scr\0" \
	"image=zImage\0" \
	"console=ttymxc0\0" \
	"splashpos=m,m\0" \
	"som=autodetect\0" \
	"baseboard=dwarf\0" \
	"default_baseboard=dwarf\0" \
	"fdtfile=undefined\0" \
	"fdt_high=0xffffffff\0" \
	"initrd_high=0xffffffff\0" \
	"fdt_file=" CONFIG_DEFAULT_FDT_FILE "\0" \
	"fdt_addr=0x18000000\0" \
	"boot_fdt=try\0" \
	"mmcdev="__stringify(CONFIG_SYS_MMC_ENV_DEV)"\0" \
	"mmcpart=" __stringify(CONFIG_SYS_MMC_IMG_LOAD_PART) "\0" \
	"mmcroot=" CONFIG_MMCROOT " rootwait rw\0" \
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

#define CONFIG_BOOTCOMMAND \
	   "mmc dev ${mmcdev};" \
	   "mmc dev ${mmcdev}; if mmc rescan; then " \
		   "if run loadbootscript; then " \
			   "run bootscript; " \
		   "else " \
			   "if run loadimage; then " \
				   "run mmcboot; " \
			   "else run netboot; " \
			   "fi; " \
		   "fi; " \
	   "else run netboot; fi"

#define CONFIG_SYS_MEMTEST_START	0x80000000
#define CONFIG_SYS_MEMTEST_END		(CONFIG_SYS_MEMTEST_START + (PHYS_SDRAM_SIZE >> 1))

#define CONFIG_SYS_LOAD_ADDR		CONFIG_LOADADDR
#define CONFIG_SYS_HZ			1000

#define CONFIG_STACKSIZE		SZ_128K

/* Physical Memory Map */
#define CONFIG_NR_DRAM_BANKS		1
#define PHYS_SDRAM			MMDC0_ARB_BASE_ADDR

#define CONFIG_SYS_SDRAM_BASE		PHYS_SDRAM
#define CONFIG_SYS_INIT_RAM_ADDR	IRAM_BASE_ADDR
#define CONFIG_SYS_INIT_RAM_SIZE	IRAM_SIZE

#define CONFIG_SYS_INIT_SP_OFFSET \
	(CONFIG_SYS_INIT_RAM_SIZE - GENERATED_GBL_DATA_SIZE)
#define CONFIG_SYS_INIT_SP_ADDR \
	(CONFIG_SYS_INIT_RAM_ADDR + CONFIG_SYS_INIT_SP_OFFSET)

#define CONFIG_ENV_SIZE			SZ_8K

#ifdef CONFIG_SYS_BOOT_QSPI
#define CONFIG_FSL_QSPI
#define CONFIG_ENV_IS_IN_SPI_FLASH
#elif defined CONFIG_NAND_BOOT
#define CONFIG_SYS_USE_NAND
#define CONFIG_ENV_IS_IN_NAND
#else
#define CONFIG_ENV_IS_IN_MMC
#endif

#ifdef CONFIG_SYS_USE_NAND
#define CONFIG_CMD_NAND
#define CONFIG_CMD_NAND_TRIMFFS

/* NAND stuff */
#define CONFIG_NAND_MXS
#define CONFIG_SYS_MAX_NAND_DEVICE	1
#define CONFIG_SYS_NAND_BASE		0x40000000
#define CONFIG_SYS_NAND_5_ADDR_CYCLE
#define CONFIG_SYS_NAND_ONFI_DETECTION

/* DMA stuff, needed for GPMI/MXS NAND support */
#define CONFIG_APBH_DMA
#define CONFIG_APBH_DMA_BURST
#define CONFIG_APBH_DMA_BURST8
#endif

#if defined(CONFIG_ENV_IS_IN_MMC)
#define CONFIG_ENV_OFFSET		(12 * SZ_64K)
#elif defined(CONFIG_ENV_IS_IN_SPI_FLASH)
#define CONFIG_ENV_OFFSET		(384 * 1024)
#define CONFIG_ENV_SECT_SIZE		(64 * 1024)
#define CONFIG_ENV_SPI_BUS		CONFIG_SF_DEFAULT_BUS
#define CONFIG_ENV_SPI_CS		CONFIG_SF_DEFAULT_CS
#define CONFIG_ENV_SPI_MODE		CONFIG_SF_DEFAULT_MODE
#define CONFIG_ENV_SPI_MAX_HZ		CONFIG_SF_DEFAULT_SPEED
#elif defined(CONFIG_ENV_IS_IN_NAND)
#undef CONFIG_ENV_SIZE
#define CONFIG_ENV_OFFSET		(60 << 20)
#define CONFIG_ENV_SECT_SIZE		(128 << 10)
#define CONFIG_ENV_SIZE			CONFIG_ENV_SECT_SIZE
#endif

#define CONFIG_SYS_MMC_ENV_DEV		0  /* USDHC3 */
#define CONFIG_SYS_MMC_ENV_PART		1	/* user area */
#define CONFIG_MMCROOT			"/dev/mmcblk2p2"  /* USDHC3 */

#define CONFIG_CMD_BMODE

/* USB Configs */
#ifdef CONFIG_CMD_USB
#define CONFIG_USB_EHCI
#define CONFIG_USB_EHCI_MX6
#define CONFIG_EHCI_HCD_INIT_AFTER_RESET
#define CONFIG_USB_HOST_ETHER
#define CONFIG_USB_ETHER_ASIX
#define CONFIG_MXC_USB_PORTSC  (PORT_PTS_UTMI | PORT_PTS_PTW)
#define CONFIG_MXC_USB_FLAGS   0
#define CONFIG_USB_MAX_CONTROLLER_COUNT 2
#endif

#define PRODUCT_NAME "imx6dl_pico"
#define VARIANT_NAME "imx6dl_pico"

#if defined(CONFIG_ANDROID_THINGS_SUPPORT)
#include "pico-imx6dl_android_things.h"
#endif

#endif
