/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2016-2017,2020 NXP.
 *
 * Configuration settings for the Freescale S32V234 EVB board.
 */

#ifndef __S32V234_COMMON_H
#define __S32V234_COMMON_H

#include <asm/arch/imx-regs.h>

#define CONFIG_S32V234

/* Config DCU */
#ifdef CONFIG_FSL_DCU_FB
#define CONFIG_SYS_DCU_ADDR             0x40028000
#define DCU_LAYER_MAX_NUM               8
#define DCU_CTRL_DESC_LAYER_NUM		10
#define CONFIG_SYS_FSL_DCU_LE
#endif

/* Config GIC */
#define CONFIG_GICV2
#define GICD_BASE		0x7D001000
#define GICC_BASE		0x7D002000

#define CONFIG_REMAKE_ELF
#undef CONFIG_RUN_FROM_IRAM_ONLY

#define CONFIG_SYS_FSL_DRAM_BASE1       0x80000000
#define CONFIG_SYS_FSL_DRAM_SIZE1       CONFIG_SYS_DDR_SIZE
#define CONFIG_SYS_FSL_DRAM_BASE2       0xC0000000
#define CONFIG_SYS_FSL_DRAM_SIZE2       0x40000000

/* Run by default from DDR0  */
#define DDR_BASE_ADDR			CONFIG_SYS_FSL_DRAM_BASE1

#define CONFIG_MACH_TYPE		4146

#define CONFIG_SKIP_LOWLEVEL_INIT

/* Config CACHE */
#define CONFIG_CMD_CACHE

/* Enable DCU QoS fix */
#define CONFIG_DCU_QOS_FIX

/* Enable passing of ATAGs */
#define CONFIG_CMDLINE_TAG

/* SMP definitions */
#define CONFIG_MAX_CPUS				(4)
#define SECONDARY_CPU_BOOT_PAGE		(CONFIG_SYS_SDRAM_BASE)
#define CPU_RELEASE_ADDR			SECONDARY_CPU_BOOT_PAGE
#define CONFIG_FSL_SMP_RELEASE_ALL
#define CONFIG_ARMV8_SWITCH_TO_EL1

/* SMP Spin Table Definitions */
#define CONFIG_MP

/* Flat device tree definitions */
#define CONFIG_OF_FDT
#define CONFIG_OF_BOARD_SETUP

/* Ramdisk name */
#define RAMDISK_NAME		rootfs.uimg

/* Flat device tree definitions */
#define FDT_ADDR		0x83E00000

/*Kernel image load address */
#define LOADADDR		0x80080000

/* Ramdisk load address */
#define RAMDISK_ADDR		0x84000000

/* Flash booting */
#define UBOOT_FLASH_ADDR		(CONFIG_SYS_FSL_FLASH0_BASE + 0x0)
#define KERNEL_FLASH_ADDR		(CONFIG_SYS_FSL_FLASH0_BASE + 0x100000)
#define KERNEL_FLASH_MAXSIZE		0xA00000
#define FDT_FLASH_ADDR			(CONFIG_SYS_FSL_FLASH0_BASE + 0xB00000)
#define FDT_FLASH_MAXSIZE		0x100000
#define RAMDISK_FLASH_ADDR		(CONFIG_SYS_FSL_FLASH0_BASE + 0xC00000)
#define RAMDISK_FLASH_MAXSIZE		0x2000000

/* Generic Timer Definitions */
/* COUNTER_FREQUENCY value will be used at startup but will be replaced
 * if an older chip version is determined at runtime.
 */
#define COUNTER_FREQUENCY               (10000000)     /* 10MHz*/
#define COUNTER_FREQUENCY_CUT1          (12000000)     /* 12MHz*/

#ifndef CONFIG_EXTRA_KERNEL_BOOT_ARGS
#define CONFIG_EXTRA_KERNEL_BOOT_ARGS	""
#endif

#define CONFIG_BOOTARGS_LOGLEVEL	""

/* Size of malloc() pool */
#ifdef CONFIG_RUN_FROM_IRAM_ONLY
#define CONFIG_SYS_MALLOC_LEN		(CONFIG_ENV_SIZE + 1 * 1024 * 1024)
#else
#define CONFIG_SYS_MALLOC_LEN		(CONFIG_ENV_SIZE + 2 * 1024 * 1024)
#endif

#if CONFIG_FSL_LINFLEX_MODULE == 0
#define LINFLEXUART_BASE		LINFLEXD0_BASE_ADDR
#else
#define LINFLEXUART_BASE		LINFLEXD1_BASE_ADDR
#endif

/* memory mapped external flash */
#define CONFIG_SYS_FSL_FLASH0_BASE      0x20000000
#define CONFIG_SYS_FSL_FLASH0_SIZE      0x10000000
#define CONFIG_SYS_FSL_FLASH1_BASE      0x60000000
#define CONFIG_SYS_FSL_FLASH1_SIZE      0x10000000
#define QSPI_BASE_ADDR			0x400A6000
#define CONFIG_SYS_FLASH_BASE		CONFIG_SYS_FSL_FLASH0_BASE

/* Allow to overwrite serial and ethaddr */
#define CONFIG_ENV_OVERWRITE
#define CONFIG_SYS_UART_PORT		(1)

#define CONFIG_SYS_FSL_ESDHC_ADDR	USDHC_BASE_ADDR
#define CONFIG_SYS_FSL_ESDHC_NUM	1

/* I2C Configs */
#define CONFIG_SYS_I2C
#define CONFIG_SYS_I2C_MXC
#define CONFIG_SYS_I2C_MXC_I2C1		/* enable I2C bus 1 */
#define CONFIG_SYS_I2C_MXC_I2C2		/* enable I2C bus 2 */
#define CONFIG_SYS_I2C_MXC_I2C3		/* enable I2C bus 3 */
#define CONFIG_SYS_I2C_SPEED		100000
#define CONFIG_SYS_SPD_BUS_NUM		0

#define CONFIG_LOADADDR			LOADADDR

#undef CONFIG_BOOTARGS
#define CONFIG_BOOTARGS		"console=ttyLF"	\
				__stringify(CONFIG_FSL_LINFLEX_MODULE) \
				"," __stringify(CONFIG_BAUDRATE) \
				" root=/dev/ram rw" \
				CONFIG_BOOTARGS_LOGLEVEL " " \
				CONFIG_EXTRA_KERNEL_BOOT_ARGS

#define MMAP_DSPI		SPI0_BASE_ADDR

/* flash related definitions */
#if defined(CONFIG_SPI_FLASH) && defined(CONFIG_FSL_QSPI)
#define CONFIG_S32V234_USES_FLASH

/* Flash Size and Num need to be updated according to the board's flash type */
#define FSL_QSPI_FLASH_SIZE            SZ_128M
#define FSL_QSPI_FLASH_NUM             2

#define QSPI0_BASE_ADDR                QSPI_BASE_ADDR
#define QSPI0_AMBA_BASE                CONFIG_SYS_FSL_FLASH0_BASE

#else
#define CONFIG_S32V234_FLASH

/* QSPI/hyperflash configs */
#ifdef CONFIG_S32V234_FLASH
#define CONFIG_S32V234_USES_FLASH

/* debug stuff for qspi/hyperflash */
#undef CONFIG_DEBUG_S32V234_QSPI_QSPI

#undef CONFIG_CMD_FLASH

#define FLASH_BASE_ADR2			(CONFIG_SYS_FSL_FLASH0_BASE + 0x4000000)

#endif
#endif

#define CONFIG_HWCONFIG

#ifdef CONFIG_CMD_BOOTI

/*
 * Enable CONFIG_USE_BOOTI if the u-boot environment variables
 * specific boot command have to be defined for booti by default.
 */
#define CONFIG_USE_BOOTI
#ifdef CONFIG_USE_BOOTI
#define IMAGE_NAME Image
#define BOOT_MTD booti
#else
#define IMAGE_NAME uImage
#define BOOT_MTD bootm
#endif

#endif

#ifndef CONFIG_BOARD_EXTRA_ENV_SETTINGS
#define CONFIG_BOARD_EXTRA_ENV_SETTINGS	""
#endif

#ifdef CONFIG_FEC_MXC
#define S32V234_FEC_DEFAULT_ADDR "00:1b:c3:12:34:22"
#define FEC_EXTRA_ENV_SETTINGS	"ethaddr=" S32V234_FEC_DEFAULT_ADDR
#else
#define FEC_EXTRA_ENV_SETTINGS	""
#endif

/*
 * Enable CONFIG_BOARD_USE_RAMFS_IN_NFSBOOT if u-boot should use a ramdisk
 * for nfsbooting.
 */
#ifdef CONFIG_BOARD_USE_RAMFS_IN_NFSBOOT
#define NFSRAMFS_ADDR "${ramdisk_addr}"
#define NFSRAMFS_TFTP_CMD "run loadtftpramdisk; "
#else
#define NFSRAMFS_ADDR "-"
#define NFSRAMFS_TFTP_CMD ""
#endif

#define CONFIG_FLASHBOOT_RAMDISK " ${ramdisk_addr} "

#define CONFIG_EXTRA_ENV_SETTINGS \
	CONFIG_BOARD_EXTRA_ENV_SETTINGS  \
	"ipaddr=10.0.0.100\0" \
	"serverip=10.0.0.1\0" \
	"netmask=255.255.255.0\0" \
	"nfsbootargs=setenv bootargs console=${console},${baudrate} " \
		"root=/dev/nfs rw " \
		"ip=${ipaddr}:${serverip}::${netmask}::eth0:off " \
		"nfsroot=${serverip}:/tftpboot/rfs,nolock,v3,tcp " \
		CONFIG_EXTRA_KERNEL_BOOT_ARGS "\0" \
	"loadtftpimage=tftp ${loadaddr} ${image};\0" \
	"loadtftpramdisk=tftp ${ramdisk_addr} ${ramdisk};\0" \
	"loadtftpfdt=tftp ${fdt_addr} ${fdt_file};\0" \
	"nfsboot=echo Booting from net using tftp and nfs...; " \
		"run nfsbootargs;"\
		"run loadtftpimage; " NFSRAMFS_TFTP_CMD "run loadtftpfdt;"\
		"${boot_mtd} ${loadaddr} " NFSRAMFS_ADDR " ${fdt_addr};\0" \
	"script=boot.scr\0" \
	"boot_mtd=" __stringify(BOOT_MTD) "\0" \
	"image=" __stringify(IMAGE_NAME) "\0" \
	"ramdisk=" __stringify(RAMDISK_NAME) "\0"\
	"console=ttyLF" __stringify(CONFIG_FSL_LINFLEX_MODULE) "\0" \
	"fdt_high=0xa0000000\0" \
	"initrd_high=0xffffffff\0" \
	"fdt_file="  __stringify(FDT_FILE) "\0" \
	"fdt_addr=" __stringify(FDT_ADDR) "\0" \
	"ramdisk_addr=" __stringify(RAMDISK_ADDR) "\0" \
	"boot_fdt=try\0" \
	"mmcdev=" __stringify(CONFIG_SYS_MMC_ENV_DEV) "\0" \
	"mmcpart=" __stringify(CONFIG_MMC_PART) "\0" \
	"mmcroot=/dev/mmcblk0p2 rootwait rw\0" \
	"update_sd_firmware_filename=u-boot.s32\0" \
	"update_sd_firmware=" \
		"if test ${ip_dyn} = yes; then " \
			"setenv get_cmd dhcp; " \
		"else " \
			"setenv get_cmd tftp; " \
		"fi; " \
		"if mmc dev ${mmcdev}; then "	\
			"if ${get_cmd} ${update_sd_firmware_filename}; then " \
				"setexpr fw_sz ${filesize} / 0x200; " \
				"setexpr fw_sz ${fw_sz} + 1; "	\
				"mmc write ${loadaddr} 0x2 ${fw_sz}; " \
			"fi; "	\
		"fi\0" \
	"mmcargs=setenv bootargs console=${console},${baudrate} " \
		CONFIG_BOOTARGS_LOGLEVEL \
		"root=${mmcroot} " CONFIG_EXTRA_KERNEL_BOOT_ARGS "\0" \
	"loadbootscript=" \
		"fatload mmc ${mmcdev}:${mmcpart} ${loadaddr} ${script};\0" \
	"bootscript=echo Running bootscript from mmc ...; " \
		"source\0" \
	"loadimage=fatload mmc ${mmcdev}:${mmcpart} ${loadaddr} ${image}\0" \
	"loadramdisk=fatload mmc ${mmcdev}:${mmcpart} ${ramdisk_addr} \
		${ramdisk}\0" \
	"loadfdt=fatload mmc ${mmcdev}:${mmcpart} ${fdt_addr} ${fdt_file}\0" \
	"jtagboot=echo Booting using jtag...; " \
		"${boot_mtd} ${loadaddr} ${ramdisk_addr} ${fdt_addr} \0" \
	"jtagsdboot=echo Booting loading Linux with ramdisk from SD...; " \
		"run loadimage; run loadramdisk; run loadfdt;"\
		"${boot_mtd} ${loadaddr} ${ramdisk_addr} ${fdt_addr} \0" \
	"mmcboot=echo Booting from mmc ...; " \
		"run mmcargs; " \
		"run loadimage; if run loadfdt; then " \
			"${boot_mtd} ${loadaddr} - ${fdt_addr}; " \
		"else " \
			"echo WARN: Cannot load the DT; " \
		"fi;\0" \
	"flashbootargs=setenv bootargs console=${console}" \
		CONFIG_BOOTARGS_LOGLEVEL " root=/dev/ram rw earlycon " \
		CONFIG_EXTRA_KERNEL_BOOT_ARGS ";" \
		"setexpr uboot_flashaddr " __stringify(UBOOT_FLASH_ADDR) ";" \
		"setexpr kernel_flashaddr " __stringify(KERNEL_FLASH_ADDR) ";" \
		"setenv kernel_maxsize " __stringify(KERNEL_FLASH_MAXSIZE) ";" \
		"setexpr fdt_flashaddr " __stringify(FDT_FLASH_ADDR) ";" \
		"setenv fdt_maxsize " __stringify(FDT_FLASH_MAXSIZE) ";" \
		"setexpr ramdisk_flashaddr " \
				__stringify(RAMDISK_FLASH_ADDR) ";" \
		"setenv ramdisk_maxsize " \
				__stringify(RAMDISK_FLASH_MAXSIZE) ";\0" \
	"flashboot=echo Booting from flash...; " \
		"run flashbootargs;"\
		"cp.b ${kernel_flashaddr} ${loadaddr} ${kernel_maxsize};"\
		"cp.b ${fdt_flashaddr} ${fdt_addr} ${fdt_maxsize};"\
		"cp.b ${ramdisk_flashaddr} ${ramdisk_addr} ${ramdisk_maxsize};"\
		"${boot_mtd} ${loadaddr}" CONFIG_FLASHBOOT_RAMDISK \
		"${fdt_addr};\0" \
	FEC_EXTRA_ENV_SETTINGS

#undef CONFIG_BOOTCOMMAND
#if defined(CONFIG_FLASH_BOOT)
#define CONFIG_BOOTCOMMAND \
	"run flashboot"
#elif defined(CONFIG_SD_BOOT)
#define CONFIG_BOOTCOMMAND \
	   "mmc dev ${mmcdev}; if mmc rescan; then " \
		   "if run loadimage; then " \
			   "run mmcboot; " \
		   "else run netboot; " \
		   "fi; " \
	   "else run netboot; fi"
#endif

/* Miscellaneous configurable options */
#define CONFIG_SYS_PROMPT_HUSH_PS2      "> "
#define CONFIG_SYS_CBSIZE		256	/* Console I/O Buffer Size */
#define CONFIG_SYS_PBSIZE		\
		(CONFIG_SYS_CBSIZE + sizeof(CONFIG_SYS_PROMPT) + 16)
#define CONFIG_SYS_MAXARGS		16 /* max number of command args */
#define CONFIG_SYS_BARGSIZE		CONFIG_SYS_CBSIZE

#define CONFIG_SYS_MEMTEST_START	(DDR_BASE_ADDR)
#define CONFIG_SYS_MEMTEST_END		(DDR_BASE_ADDR + \
						(CONFIG_SYS_DDR_SIZE - 1))

#define CONFIG_SYS_LOAD_ADDR		CONFIG_LOADADDR

#define IMX_FEC_BASE			ENET_BASE_ADDR

#define CONFIG_SYS_TEXT_OFFSET		0x00020000

#ifdef CONFIG_RUN_FROM_IRAM_ONLY
#define CONFIG_SYS_MALLOC_BASE		(DDR_BASE_ADDR)
#endif

/* Physical memory map */
#define PHYS_SDRAM			(DDR_BASE_ADDR)
#define PHYS_SDRAM_SIZE			(CONFIG_SYS_DDR_SIZE)

#define CONFIG_SYS_SDRAM_BASE		PHYS_SDRAM
#define CONFIG_SYS_INIT_RAM_ADDR	IRAM_BASE_ADDR
#define CONFIG_SYS_INIT_RAM_SIZE	IRAM_SIZE

#define CONFIG_SYS_INIT_SP_OFFSET \
	(CONFIG_SYS_INIT_RAM_SIZE - GENERATED_GBL_DATA_SIZE - \
	 CONFIG_SYS_TEXT_OFFSET)
#define CONFIG_SYS_INIT_SP_ADDR \
	(CONFIG_SYS_INIT_RAM_ADDR + CONFIG_SYS_INIT_SP_OFFSET)

/* environment organization */

#define CONFIG_SYS_MMC_ENV_DEV		0
#define CONFIG_MMC_PART			1

#define FLASH_SECTOR_SIZE		0x40000 /* 256 KB */

#if defined(CONFIG_FLASH_BOOT)
#define CONFIG_SYS_MAX_FLASH_BANKS	1
#define CONFIG_SYS_MAX_FLASH_SECT	(0x4000000 / CONFIG_ENV_SECT_SIZE)
#endif

#define CONFIG_BOOTP_BOOTFILESIZE

#ifdef CONFIG_CMD_PCI
#define CONFIG_GICSUPPORT
#define CONFIG_CMD_IRQ
#define CONFIG_PCIE_S32V234
#define CONFIG_PCI
#define CONFIG_PCI_PNP
#define CONFIG_PCI_SCAN_SHOW
#endif /* CONFIG_CMD_PCI */

#endif
