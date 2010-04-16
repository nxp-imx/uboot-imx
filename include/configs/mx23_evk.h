/*
 * Copyright (C) 2008 Embedded Alley Solutions, Inc.
 *
 * (C) Copyright 2009-2010 Freescale Semiconductor, Inc.
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
#define CONFIG_MX23				/* MX23 SoC */
#define CONFIG_MX23_EVK				/* MX23 EVK board */
#define CONFIG_SYS_CLK_FREQ	120000000	/* Arm Clock frequency */
#define CONFIG_USE_TIMER0			/* use timer 0 */
#define CONFIG_SYS_HZ		1000		/* Ticks per second */
/*=============*/
/* Memory Info */
/*=============*/
#define CONFIG_SYS_MALLOC_LEN	(0x10000 + 128*1024)	/* malloc() len */
#define CONFIG_SYS_GBL_DATA_SIZE 128		/* reserved for initial data */
#define CONFIG_SYS_MEMTEST_START 0x40000000	/* memtest start address */
#define CONFIG_SYS_MEMTEST_END	 0x41000000	/* 16MB RAM test */
#define CONFIG_NR_DRAM_BANKS	1		/* we have 1 bank of DRAM */
#define CONFIG_STACKSIZE	(256*1024)	/* regular stack */
#define PHYS_SDRAM_1		0x40000000	/* mDDR Start */
#define PHYS_SDRAM_1_SIZE	0x02000000	/* mDDR size 32MB */

/*====================*/
/* Serial Driver info */
/*====================*/
#define CONFIG_STMP3XXX_DBGUART			/* 378x debug UART */
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
#define CONFIG_ENV_SIZE		0x20000

/*==============================*/
/* U-Boot general configuration */
/*==============================*/
#undef	CONFIG_USE_IRQ				/* No IRQ/FIQ in U-Boot */
#define CONFIG_MISC_INIT_R
#define CONFIG_NETMASK		255.255.255.0
#define CONFIG_IPADDR		192.167.10.2
#define CONFIG_SERVERIP		192.167.10.1
#define CONFIG_BOOTDELAY	2
#define CONFIG_BOOTFILE		"uImage"	/* Boot file name */
#define CONFIG_SYS_PROMPT	"MX23 U-Boot > "
						/* Monitor Command Prompt */
#define CONFIG_SYS_CBSIZE	1024		/* Console I/O Buffer Size  */
#define CONFIG_SYS_PBSIZE	(CONFIG_SYS_CBSIZE+sizeof(CONFIG_SYS_PROMPT)+16)
						/* Print buffer sz */
#define CONFIG_SYS_MAXARGS	16		/* max number of command args */
#define CONFIG_SYS_BARGSIZE	CONFIG_SYS_CBSIZE
						/* Boot Argument Buffer Size */
#define CONFIG_SYS_LOAD_ADDR	0x40400000
				/* default Linux kernel load address */
#define CONFIG_VERSION_VARIABLE
#define CONFIG_AUTO_COMPLETE	/* Won't work with hush so far, may be later */
#define CFG_HUSH_PARSER
#define CFG_PROMPT_HUSH_PS2	"> "
#define CONFIG_CMDLINE_EDITING
#define CFG_LONGHELP
#define CONFIG_CRC32_VERIFY
#define CONFIG_MX_CYCLIC

/*===================*/
/* Linux Information */
/*===================*/
#define LINUX_BOOT_PARAM_ADDR	0x40000100
#define CONFIG_CMDLINE_TAG
#define CONFIG_SETUP_MEMORY_TAGS
#define CONFIG_BOOTARGS		"console=ttyAM0,115200n8 "\
			"root=/dev/mtdblock1 rootfstype=jffs2 lcd_panel=lms350"
#define CONFIG_BOOTCOMMAND	"tftpboot ; bootm"

/*=================*/
/* U-Boot commands */
/*=================*/
#include <config_cmd_default.h>
#define CONFIG_CMD_ASKENV
#define CONFIG_CMD_DHCP
#define CONFIG_BOOTP_SUBNETMASK
#define CONFIG_BOOTP_GATEWAY
#define CONFIG_BOOTP_DNS
#undef CONFIG_CMD_DIAG
#define CONFIG_CMD_MII
#define CONFIG_CMD_PING
#define CONFIG_CMD_NET
#define CONFIG_CMD_SAVES
#undef CONFIG_CMD_IMLS

/* Ethernet chip - select an alternative driver */
#define CONFIG_ENC28J60_ETH
#define CONFIG_ENC28J60_ETH_SPI_BUS	0
#define CONFIG_ENC28J60_ETH_SPI_CS	0

#endif /* __CONFIG_H */
