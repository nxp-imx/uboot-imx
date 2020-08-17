/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2020 NXP
 */

#ifndef __IMX8MN_AB2_H
#define __IMX8MN_AB2_H

#include <linux/sizes.h>
#include <asm/arch/imx-regs.h>

#include "imx_env.h"

#define CONFIG_SPL_MAX_SIZE		(208 * 1024)
#define CONFIG_SYS_MONITOR_LEN		(512 * 1024)
#define CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_USE_SECTOR
#define CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR	0x300
#define CONFIG_SYS_MMCSD_FS_BOOT_PARTITION	1
#define CONFIG_SYS_UBOOT_BASE	\
	(QSPI0_AMBA_BASE + CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR * 512)

#ifdef CONFIG_SPL_BUILD
#define CONFIG_SPL_STACK		0x187FF0
#define CONFIG_SPL_BSS_START_ADDR      0x0095e000
#define CONFIG_SPL_BSS_MAX_SIZE		SZ_8K	/* 8 KB */
#define CONFIG_SYS_SPL_MALLOC_START	0x42200000
#define CONFIG_SYS_SPL_MALLOC_SIZE	SZ_64K	/* 64 KB */

/* malloc f used before GD_FLG_FULL_MALLOC_INIT set */
#define CONFIG_MALLOC_F_ADDR		0x184000

/* For RAW image gives a error info not panic */
#define CONFIG_SPL_ABORT_ON_RAW_IMAGE

#define CONFIG_POWER
#define CONFIG_POWER_I2C
#ifdef CONFIG_IMX8M_DDR4
#define CONFIG_POWER_BD71837
#else
#define CONFIG_POWER_PCA9450
#endif

#define CONFIG_SYS_I2C

#if defined(CONFIG_NAND_BOOT)
#define CONFIG_SPL_NAND_SUPPORT
#define CONFIG_SPL_DMA
#define CONFIG_SPL_NAND_MXS
#define CONFIG_SPL_NAND_BASE
#define CONFIG_SPL_NAND_IDENT
#define CONFIG_SYS_NAND_U_BOOT_OFFS 	0x4000000 /* Put the FIT out of first 64MB boot area */

/* Set a redundant offset in nand FIT mtdpart. The new uuu will burn full boot image (not only FIT part) to the mtdpart, so we check both two offsets */
#define CONFIG_SYS_NAND_U_BOOT_OFFS_REDUND \
	(CONFIG_SYS_NAND_U_BOOT_OFFS + CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR * 512 - 0x8400)
#endif

#endif

#define CONFIG_CMD_READ
#define CONFIG_SERIAL_TAG

#define CONFIG_REMAKE_ELF
/* ENET Config */
/* ENET1 */
#if defined(CONFIG_FEC_MXC)
#define CONFIG_ETHPRIME                 "eth0"

#define CONFIG_FEC_XCV_TYPE             RGMII
#define CONFIG_FEC_MXC_PHYADDR          0
#define FEC_QUIRK_ENET_MAC
#define PHY_ANEG_TIMEOUT                20000

#define IMX_FEC_BASE			0x30BE0000
#endif


/*
 * Another approach is add the clocks for inmates into clks_init_on
 * in clk-imx8mm.c, then clk_ingore_unused could be removed.
 */
#define JAILHOUSE_ENV \
	"jh_clk= \0 " \
	"jh_mmcboot=mw 0x303d0518 0xff; setenv fdt_file imx8mn-ddr4-ab2-root.dtb;" \
		"setenv jh_clk clk_ignore_unused; " \
			   "if run loadimage; then " \
				   "run mmcboot; " \
			   "else run jh_netboot; fi; \0" \
	"jh_netboot=mw 0x303d0518 0xff; setenv fdt_file imx8mn-ddr4-ab2-root.dtb; setenv jh_clk clk_ignore_unused; run netboot; \0 "

#define M7_BOOT_ENV \
	"m7_boot=no\0" \
	"m7_image=nxh3670.itb\0" \
	"m7_loadaddr=0x80000000\0" \
	"m7_nxh_app_loadaddr=0x81000000\0" \
	"m7_nxh_rfmac_loadaddr=0x81012000\0" \
	"m7_nxh_cf_loadaddr=0x81016000\0" \
	"m7_nxh_data_loadaddr=0x8101E000\0" \
	"m7_fdt_file=imx8mn-ab2-m7.dtb\0" \
	"m7_nxh_bin=main@1\0" \
	"m7_nxh_app=app@1\0" \
	"m7_nxh_rfmac=rfmac@1\0" \
	"m7_nxh_cf=cf@1\0" \
	"m7_nxh_data=data@1\0" \
	"loadm7nxhfw=imxtract ${loadaddr} ${m7_nxh_bin} ${m7_loadaddr}; " \
		"imxtract ${loadaddr} ${m7_nxh_app} ${m7_nxh_app_loadaddr}; " \
		"imxtract ${loadaddr} ${m7_nxh_rfmac} ${m7_nxh_rfmac_loadaddr}; " \
		"imxtract ${loadaddr} ${m7_nxh_cf} ${m7_nxh_cf_loadaddr}; " \
		"imxtract ${loadaddr} ${m7_nxh_data} ${m7_nxh_data_loadaddr}\0" \
	"loadm7image=fatload mmc ${mmcdev}:${mmcpart} ${loadaddr} ${m7_image}\0" \
	"update_m7_from_sd=" \
		"if sf probe 0:0; then " \
			"if run loadm7image; then " \
				"setexpr fw_sz ${filesize} + 0xffff; " \
				"setexpr fw_sz ${fw_sz} / 0x10000; " \
				"setexpr fw_sz ${fw_sz} * 0x10000; " \
				"sf erase 0x100000 ${fw_sz}; " \
				"sf write ${m7_loadaddr} 0x100000 ${filesize}; " \
			"fi; " \
		"fi\0" \
	"m7boot=run loadm7image; run loadm7nxhfw; dcache flush; bootaux ${m7_loadaddr}\0" \
	"m7netboot=${get_cmd} ${loaddadr} ${m7_image}; " \
		"run loadm7nxhfw; dcache flush; bootaux ${m7_loadaddr}; \0" \
	"m7boot_sf=sf probe 0:0; dcache flush; bootaux ${m7_sf_loadaddr}\0"

#define CONFIG_MFG_ENV_SETTINGS \
	CONFIG_MFG_ENV_SETTINGS_DEFAULT \
	"initrd_addr=0x43800000\0" \
	"initrd_high=0xffffffffffffffff\0" \
	"emmc_dev=2\0"\
	"sd_dev=1\0" \

/* Initial environment variables */
#define CONFIG_EXTRA_ENV_SETTINGS		\
	CONFIG_MFG_ENV_SETTINGS \
	JAILHOUSE_ENV \
	M7_BOOT_ENV \
	"script=boot.scr\0" \
	"image=Image\0" \
	"console=ttymxc1,115200\0" \
	"fdt_addr=0x43000000\0"			\
	"fdt_high=0xffffffffffffffff\0"		\
	"boot_fit=no\0" \
	"fdt_file=" CONFIG_DEFAULT_FDT_FILE "\0" \
	"initrd_addr=0x43800000\0"		\
	"initrd_high=0xffffffffffffffff\0" \
	"mmcdev="__stringify(CONFIG_SYS_MMC_ENV_DEV)"\0" \
	"mmcpart=" __stringify(CONFIG_SYS_MMC_IMG_LOAD_PART) "\0" \
	"mmcroot=" CONFIG_MMCROOT " rootwait rw\0" \
	"mmcautodetect=yes\0" \
	"mmcargs=setenv bootargs ${jh_clk} console=${console} root=${mmcroot}\0 " \
	"loadbootscript=fatload mmc ${mmcdev}:${mmcpart} ${loadaddr} ${script};\0" \
	"bootscript=echo Running bootscript from mmc ...; " \
		"source\0" \
	"loadimage=fatload mmc ${mmcdev}:${mmcpart} ${loadaddr} ${image}\0" \
	"loadfdt=fatload mmc ${mmcdev}:${mmcpart} ${fdt_addr} ${fdt_file}\0" \
	"mmcboot=echo Booting from mmc ...; " \
		"run mmcargs; " \
		"if test ${m7_boot} = yes || test ${m7_boot} = try; then "\
			"echo Booting M7 aux core...; " \
			"run m7boot; " \
		"fi; " \
		"if test ${boot_fit} = yes || test ${boot_fit} = try; then " \
			"bootm ${loadaddr}; " \
		"else " \
			"if run loadfdt; then " \
				"booti ${loadaddr} - ${fdt_addr}; " \
			"else " \
				"echo WARN: Cannot load the DT; " \
			"fi; " \
		"fi;\0" \
	"netargs=setenv bootargs ${jh_clk} console=${console} " \
		"root=/dev/nfs " \
		"ip=dhcp nfsroot=${serverip}:${nfsroot},v3,tcp\0" \
	"netboot=echo Booting from net ...; " \
		"run netargs;  " \
		"if test ${ip_dyn} = yes; then " \
			"setenv get_cmd dhcp; " \
		"else " \
			"setenv get_cmd tftp; " \
		"fi; " \
		"if test ${m7_boot} = yes || test ${m7_boot} = try; then " \
			"echo Booting M7 aux core...;" \
			"run m7netboot;" \
		"fi; " \
		"${get_cmd} ${loadaddr} ${image}; " \
		"if test ${boot_fit} = yes || test ${boot_fit} = try; then " \
			"bootm ${loadaddr}; " \
		"else " \
			"if ${get_cmd} ${fdt_addr} ${fdt_file}; then " \
				"booti ${loadaddr} - ${fdt_addr}; " \
			"else " \
				"echo WARN: Cannot load the DT; " \
			"fi; " \
		"fi;\0"

#define CONFIG_BOOTCOMMAND \
	   "mmc dev ${mmcdev}; if mmc rescan; then " \
		   "if run loadbootscript; then " \
			   "run bootscript; " \
		   "else " \
			   "if run loadimage; then " \
				   "run mmcboot; " \
			   "else run netboot; " \
			   "fi; " \
		   "fi; " \
	   "fi;"

/* Link Definitions */
#define CONFIG_LOADADDR			0x40480000

#define CONFIG_SYS_LOAD_ADDR           CONFIG_LOADADDR

#define CONFIG_SYS_INIT_RAM_ADDR        0x40000000
#define CONFIG_SYS_INIT_RAM_SIZE        0x80000
#define CONFIG_SYS_INIT_SP_OFFSET \
	(CONFIG_SYS_INIT_RAM_SIZE - GENERATED_GBL_DATA_SIZE)
#define CONFIG_SYS_INIT_SP_ADDR \
	(CONFIG_SYS_INIT_RAM_ADDR + CONFIG_SYS_INIT_SP_OFFSET)

#define CONFIG_ENV_OVERWRITE
#if defined(CONFIG_ENV_IS_IN_SPI_FLASH)
#define CONFIG_ENV_SPI_BUS		CONFIG_SF_DEFAULT_BUS
#define CONFIG_ENV_SPI_CS		CONFIG_SF_DEFAULT_CS
#define CONFIG_ENV_SPI_MODE		CONFIG_SF_DEFAULT_MODE
#define CONFIG_ENV_SPI_MAX_HZ		CONFIG_SF_DEFAULT_SPEED
#endif
#define CONFIG_SYS_MMC_ENV_DEV		1   /* USDHC2 */
#define CONFIG_MMCROOT			"/dev/mmcblk1p2"  /* USDHC2 */

/* Size of malloc() pool */
#define CONFIG_SYS_MALLOC_LEN		SZ_32M

#define CONFIG_SYS_SDRAM_BASE           0x40000000
#define PHYS_SDRAM                      0x40000000
#define PHYS_SDRAM_SIZE			0x80000000 /* 2GB DDR */

#define CONFIG_SYS_MEMTEST_START    PHYS_SDRAM
#define CONFIG_SYS_MEMTEST_END      (CONFIG_SYS_MEMTEST_START + (PHYS_SDRAM_SIZE >> 1))

#define CONFIG_MXC_UART_BASE		UART2_BASE_ADDR

/* Monitor Command Prompt */
#define CONFIG_SYS_PROMPT_HUSH_PS2	"> "
#define CONFIG_SYS_CBSIZE		2048
#define CONFIG_SYS_MAXARGS		64
#define CONFIG_SYS_BARGSIZE		CONFIG_SYS_CBSIZE
#define CONFIG_SYS_PBSIZE		(CONFIG_SYS_CBSIZE + \
					sizeof(CONFIG_SYS_PROMPT) + 16)

#define CONFIG_IMX_BOOTAUX

/* USDHC */
#define CONFIG_FSL_USDHC

#define CONFIG_SYS_FSL_USDHC_NUM	2
#define CONFIG_SYS_FSL_ESDHC_ADDR	0

#define CONFIG_SYS_MMC_IMG_LOAD_PART	1

#ifdef CONFIG_FSL_FSPI
#define FSL_FSPI_FLASH_SIZE		SZ_32M
#define FSL_FSPI_FLASH_NUM		1
#define FSPI0_BASE_ADDR			0x30bb0000
#define FSPI0_AMBA_BASE			0x0
#define CONFIG_FSPI_QUAD_SUPPORT

#define CONFIG_SYS_FSL_FSPI_AHB
#endif

#ifdef CONFIG_NAND_MXS
#define CONFIG_CMD_NAND_TRIMFFS

/* NAND stuff */
#define CONFIG_SYS_MAX_NAND_DEVICE     1
#define CONFIG_SYS_NAND_BASE           0x20000000
#define CONFIG_SYS_NAND_5_ADDR_CYCLE
#define CONFIG_SYS_NAND_ONFI_DETECTION
#define CONFIG_SYS_NAND_USE_FLASH_BBT
#endif /* CONFIG_NAND_MXS */

#define CONFIG_SYS_I2C_SPEED		100000

#endif
