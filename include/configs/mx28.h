/*
 * Copyright (C) 2008 Embedded Alley Solutions, Inc.
 *
 * Copyright (C) 2010 Freescale Semiconductor, Inc.
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

#ifndef __CONFIG_H
#define __CONFIG_H
#include <asm/sizes.h>

/*
 * Define this to make U-Boot skip low level initialization when loaded
 * by initial bootloader. Not required by NAND U-Boot version but IS
 * required for a NOR version used to burn the real NOR U-Boot into
 * NOR Flash. NAND and NOR support for DaVinci chips is mutually exclusive
 * so it is NOT possible to build a U-Boot with both NAND and NOR routines.
 * NOR U-Boot is loaded directly from Flash so it must perform all the
 * low level initialization itself. NAND version is loaded by an initial
 * bootloader (UBL in TI-ese) that performs such an initialization so it's
 * skipped in NAND version. The third DaVinci boot mode loads a bootloader
 * via UART0 and that bootloader in turn loads and runs U-Boot (or whatever)
 * performing low level init prior to loading. All that means we can NOT use
 * NAND version to put U-Boot into NOR because it doesn't have NOR support and
 * we can NOT use NOR version because it performs low level initialization
 * effectively destroying itself in DDR memory. That's why a separate NOR
 * version with this define is needed. It is loaded via UART, then one uses
 * it to somehow download a proper NOR version built WITHOUT this define to
 * RAM (tftp?) and burn it to NOR Flash. I would be probably able to squeeze
 * NOR support into the initial bootloader so it won't be needed but DaVinci
 * static RAM might be too small for this (I have something like 2Kbytes left
 * as of now, without NOR support) so this might've not happened...
 *
 */

/*===================*/
/* SoC Configuration */
/*===================*/
#define CONFIG_ARM926EJS			/* arm926ejs CPU core */
#define CONFIG_MX28				/* STMP378x SoC */
#define CONFIG_SYS_CLK_FREQ	120000000	/* Arm Clock frequency */
#define CONFIG_USE_TIMER0			/* use timer 0 */
#define CONFIG_SYS_HZ		1000		/* Ticks per second */
/*=============*/
/* Memory Info */
/*=============*/
#define CONFIG_SYS_MALLOC_LEN	(0x10000 + 128*1024)	/* malloc() len */
#define CONFIG_SYS_GBL_DATA_SIZE 128		/* reserved for initial data */
#define CONFIG_SYS_MEMTEST_START 0x40000000	/* memtest start address */
#define CONFIG_SYS_MEMTEST_END	 0x40400000	/* 16MB RAM test */
#define CONFIG_NR_DRAM_BANKS	1		/* we have 1 bank of DRAM */
#define CONFIG_STACKSIZE	(256*1024)	/* regular stack */
#define PHYS_SDRAM_1		0x40000000	/* mDDR Start */
#define PHYS_SDRAM_1_SIZE	0x08000000	/* mDDR size 32MB */

/*====================*/
/* Serial Driver info */
/*====================*/
#define CONFIG_DBGUART_CLK	24000000
#define CONFIG_BAUDRATE		115200		/* Default baud rate */
#define CONFIG_SYS_BAUDRATE_TABLE	{ 9600, 19200, 38400, 57600, 115200 }

/*====================*/
/* SPI Driver info */
/*====================*/
#define CONFIG_SSP_CLK		48000000
#define CONFIG_SPI_CLK		3000000
#define CONFIG_SPI_SSP1
#undef CONFIG_SPI_SSP2

/*=====================*/
/* Flash & Environment */
/*=====================*/
#define CONFIG_SYS_NO_FLASH			/* Flash is not supported */
#define CONFIG_ENV_IS_NOWHERE		/* Store ENV in memory only */


/* ROM loads UBOOT into DRAM */
#define CONFIG_SKIP_RELOCATE_UBOOT


/*==============================*/
/* U-Boot general configuration */
/*==============================*/
#define CONFIG_BOOTDELAY	2
#define CONFIG_BOOTFILE		"uImage"	/* Boot file name */
#define CONFIG_SYS_PROMPT	"MX28 U-Boot > "
#define CONFIG_SYS_CBSIZE	1024		/* Console I/O buffer size */
#define CONFIG_SYS_PBSIZE \
	(CONFIG_SYS_CBSIZE + sizeof(CONFIG_SYS_PROMPT) + 16)
						/* Print buffer size */
#define CONFIG_SYS_MAXARGS	16		/* Max number of command args */
#define CONFIG_SYS_BARGSIZE	CONFIG_SYS_CBSIZE
						/* Boot argument buffer size */
#define CONFIG_VERSION_VARIABLE			/* U-BOOT version */
#define CONFIG_AUTO_COMPLETE			/* Command auto complete */
#define CONFIG_CMDLINE_EDITING			/* Command history etc */
#define CONFIG_VERSION_VARIABLE
#define CONFIG_AUTO_COMPLETE	/* Won't work with hush so far, may be later */
#define CFG_HUSH_PARSER
#define CFG_PROMPT_HUSH_PS2	"> "
#define CONFIG_CMDLINE_EDITING
#define CFG_LONGHELP
#define CONFIG_CRC32_VERIFY
#define CONFIG_MX_CYCLIC

/*
 * Boot Linux
 */
#define LINUX_BOOT_PARAM_ADDR	0x40000100
#define CONFIG_CMDLINE_TAG
#define CONFIG_SETUP_MEMORY_TAGS
#define CONFIG_BOOTARGS		"console=ttyAM0,115200n8 "
#define CONFIG_BOOTCOMMAND	"run bootcmd_net"
#define CONFIG_LOADADDR		0x42000000
#define CONFIG_SYS_LOAD_ADDR	CONFIG_LOADADDR

/*
 * Extra Environments
 */
#define	CONFIG_EXTRA_ENV_SETTINGS \
	"nfsroot=/data/rootfs_home/rootfs\0" \
	"bootargs_nfs=setenv bootargs ${bootargs} root=/dev/nfs " \
		"ip=dhcp nfsroot=${serverip}:${nfsroot},v3,tcp\0" \
	"bootcmd_net=run bootargs_nfs; dhcp; bootm\0" \
	"bootargs_mmc=setenv bootargs ${bootargs} root=/dev/mmcblk0p2 " \
		"ip=dhcp rootfstype=ext2\0" \
	"bootcmd_mmc=run bootargs_mmc; " \
		"mmc read 0 ${loadaddr} 100 3000; bootm\0" \

/*=================*/
/* U-Boot commands */
/*=================*/
#include <config_cmd_default.h>
#undef  CONFIG_CMD_MMC /* MX28 use special mmc command*/
#define CONFIG_ARCH_CPU_INIT
#define CONFIG_DISPLAY_CPUINFO

/*
 * ENET Driver
 */
#define CONFIG_MXC_ENET
#define CONFIG_NET_MULTI
#define CONFIG_ETH_PRIME
#define CONFIG_CMD_MII
#define CONFIG_DISCOVER_PHY
#define CONFIG_CMD_DHCP
#define CONFIG_BOOTP_SUBNETMASK
#define CONFIG_BOOTP_GATEWAY
#define CONFIG_BOOTP_DNS
#define CONFIG_CMD_PING
#define CONFIG_IPADDR			192.168.1.101
#define CONFIG_SERVERIP			192.168.1.100
#define CONFIG_NETMASK			255.255.255.0

/*
 * MMC Driver
 */
#define CONFIG_IMX_SSP_MMC		/* MMC driver based on SSP */
#define CONFIG_GENERIC_MMC
#define CONFIG_CUSTOMIZE_MMCOPS		/* To customize do_mmcops() */
#define CONFIG_DOS_PARTITION
#define CONFIG_CMD_FAT
#define CONFIG_MMC

/*
 * Environments on MMC
 */
#define CONFIG_CMD_ENV
#define CONFIG_ENV_OVERWRITE
#define CONFIG_ENV_IS_IN_MMC
/* Assoiated with the MMC layout defined in mmcops.c */
#define CONFIG_ENV_OFFSET		(0x400) /* 1 KB */
#define CONFIG_ENV_SIZE			(0x20000 - 0x400) /* 127 KB */

#endif /* __CONFIG_H */
