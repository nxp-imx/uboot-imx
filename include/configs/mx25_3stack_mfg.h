/*
 * (C) Copyright 2009-2010 Freescale Semiconductor
 *
 * (C) Copyright 2004
 * Texas Instruments.
 * Richard Woodruff <r-woodruff2@ti.com>
 * Kshitij Gupta <kshitij@ti.com>
 *
 * Configuration settings for the Freescale i.MX31 PDK board.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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

#include <asm/arch/mx25.h>
#include <asm/arch/mx25-regs.h>

/* High Level Configuration Options */
#define CONFIG_ARM926EJS	1	/* This is an arm1136 CPU core */
#define CONFIG_MX25		1	/* in a mx31 */
#define CONFIG_MX25_HCLK_FREQ	24000000
#define CONFIG_MX25_CLK32	32768

#define CONFIG_MFG		1
#define CONFIG_IMX_CSPI		1
#define IMX_CSPI_VER_0_7	1
#define CONFIG_IMX_SPI_CPLD

/* IF iMX25 3DS V-1.0 define it */
/* #define CONFIG_MX25_3DS_V10 */

#ifdef CONFIG_MX25_3DS_V10
#define MXC_MEMORY_MDDR
#endif

#define CONFIG_DISPLAY_CPUINFO
#define CONFIG_DISPLAY_BOARDINFO

#define CONFIG_SYS_64BIT_VSPRINTF
#define BOARD_LATE_INIT

#define CONFIG_CMDLINE_TAG		1	/* enable passing of ATAGs */
#define CONFIG_SETUP_MEMORY_TAGS	1
#define CONFIG_INITRD_TAG		1
#define CONFIG_REVISION_TAG             1

/* No support for NAND boot for i.MX31 PDK yet, so we rely on some other
 * program to initialize the SDRAM.
 */
#define CONFIG_SKIP_RELOCATE_UBOOT

/*
 * Size of malloc() pool
 */
#define CONFIG_SYS_MALLOC_LEN		(CONFIG_ENV_SIZE + 512 * 1024)
#define CONFIG_SYS_GBL_DATA_SIZE	128 /* bytes reserved initial data */

/*
 * Hardware drivers
 */
#define CONFIG_HARD_I2C		1
#define CONFIG_I2C_MXC		1
#define CONFIG_SYS_I2C_PORT		I2C1_BASE
#define CONFIG_SYS_I2C_SPEED		40000
#define CONFIG_SYS_I2C_SLAVE		0xfe

#define CONFIG_MX25_UART	1
#define CONFIG_MX25_UART1		1

/* allow to overwrite serial and ethaddr */
#define CONFIG_ENV_OVERWRITE
#define CONFIG_CONS_INDEX	1
#define CONFIG_BAUDRATE		115200
#define CONFIG_SYS_BAUDRATE_TABLE	{9600, 19200, 38400, 57600, 115200}

/***********************************************************
 * Command definition
 ***********************************************************/

#include <config_cmd_default.h>

#define CONFIG_CMD_I2C
#define CONFIG_CMD_MII
#define CONFIG_CMD_PING
/* #define CONFIG_CMD_SPI */
/* #define CONFIG_CMD_DATE */
#define CONFIG_CMD_NAND
#define CONFIG_CMD_MMC
#define CONFIG_MXC_NAND

/*
 * MMC Configs
 * */
#ifdef CONFIG_CMD_MMC
	#define CONFIG_MMC				1
	#define CONFIG_GENERIC_MMC
	#define CONFIG_IMX_MMC
	#define CONFIG_SYS_FSL_ESDHC_NUM        2
	#define CONFIG_SYS_FSL_ESDHC_ADDR       0
	#define CONFIG_SYS_MMC_ENV_DEV  0
	#define CONFIG_IMX_ESDHC_V1
	#define CONFIG_DOS_PARTITION	1
	#define CONFIG_CMD_FAT		1
#endif

/* Disabled due to compilation errors in cmd_bootm.c (IMLS seems to require
 * that CONFIG_NO_FLASH is undefined).
 */
#undef CONFIG_CMD_IMLS

#define CONFIG_BOOTDELAY	0

#define CONFIG_LOADADDR		0x80100000	/* loadaddr env var */

#define CONFIG_BOOTAGS         "console=ttymxc0,115200 "\
				"rdinit=/linuxrc"
#define CONFIG_BOOTCOMMAND      "bootm ${loadaddr} 0x80800000"
#define CONFIG_ENV_IS_EMBEDDED

/*
#define	CONFIG_EXTRA_ENV_SETTINGS					\
	"netdev=eth0\0"							\
	"ethprime=smc911x-0\0"						\
	"uboot=u-boot.bin\0"						\
	"uboot_addr=0xa0000000\0"					\
	"kernel=uImage\0"						\
	"bootargs_base=setenv bootargs console=ttymxc0,115200\0"	\
	"bootargs_nfs=setenv bootargs $(bootargs) root=/dev/nfs "	\
		"ip=dhcp nfsroot=$(serverip):$(nfsrootfs),v3,tcp\0"	\
	"bootcmd=run bootcmd_net\0"					\
	"bootcmd_net=run bootargs_base bootargs_nfs; "			\
		"tftpboot ${loadaddr} ${kernel}; bootm\0"		\
	"load_uboot=tftpboot ${loadaddr} ${uboot}\0"
*/

/*Support LAN9217*/
#define CONFIG_SMC911X
#define CONFIG_SMC911X_CPLD
#define CONFIG_SMC911X_BASE	0

#define CONFIG_HAS_ETH1
#define CONFIG_CMD_NET
#define CONFIG_NET_RETRY_COUNT	100
#define CONFIG_CMD_DHCP
#define CONFIG_BOOTP_SUBNETMASK
#define CONFIG_BOOTP_GATEWAY
#define CONFIG_BOOTP_DNS
#define CONFIG_NET_MULTI
#define CONFIG_ETHPRIME

#define CONFIG_MXC_FEC
#define CONFIG_MII
#define CONFIG_MII_GASKET
#define CONFIG_DISCOVER_PHY

#define CONFIG_FEC0_IOBASE FEC_BASE
#define CONFIG_FEC0_PINMUX -1
#define CONFIG_FEC0_PHY_ADDR 0x1F
#define CONFIG_FEC0_MIIBASE -1

/*
 * Miscellaneous configurable options
 */
#define CONFIG_SYS_LONGHELP	/* undef to save memory */
#define CONFIG_SYS_PROMPT	"MX25 U-Boot > "
#define CONFIG_AUTO_COMPLETE
#define CONFIG_SYS_CBSIZE	256  /* Console I/O Buffer Size */
/* Print Buffer Size */
#define CONFIG_SYS_PBSIZE	(CONFIG_SYS_CBSIZE+sizeof(CONFIG_SYS_PROMPT)+16)
#define CONFIG_SYS_MAXARGS	16	/* max number of command args */
#define CONFIG_SYS_BARGSIZE	CONFIG_SYS_CBSIZE /* Boot Argument Buf Size */

#define CONFIG_SYS_MEMTEST_START	PHYS_SDRAM_1	/* memtest works on */
#define CONFIG_SYS_MEMTEST_END	0x10000

#define CONFIG_SYS_LOAD_ADDR	CONFIG_LOADADDR /* default load address */

#define CONFIG_SYS_HZ			1000

#define UBOOT_IMAGE_SIZE	0x40000

#define CONFIG_CMDLINE_EDITING	1

/*-----------------------------------------------------------------------
 * Stack sizes
 *
 * The stack sizes are set up in start.S using the settings below
 */
#define CONFIG_STACKSIZE	(128 * 1024) /* regular stack */

/*-----------------------------------------------------------------------
 * Physical Memory Map
 */
#define CONFIG_NR_DRAM_BANKS	1
#define PHYS_SDRAM_1		CSD0_BASE

/* iMX25 V-1.0 has 128MB but V-1.1 has only 64MB */
#ifdef CONFIG_MX25_3DS_V10
#define PHYS_SDRAM_1_SIZE	(128 * 1024 * 1024)
#else
#define PHYS_SDRAM_1_SIZE       (64 * 1024 * 1024)
#endif

/* Monitor at beginning of flash */
/* #define CONFIG_FSL_ENV_IN_MMC */
/*#define CONFIG_FSL_ENV_IN_NAND*/

/*-----------------------------------------------------------------------
 * FLASH and environment organization
 */
/*
#if defined(CONFIG_FSL_ENV_IN_NAND)
	#define CONFIG_ENV_IS_IN_NAND 1
	#define CONFIG_ENV_OFFSET	0x80000
#elif defined(CONFIG_FSL_ENV_IN_MMC)
	#define CONFIG_ENV_IS_IN_MMC	1
	#define CONFIG_ENV_OFFSET	(768 * 1024)
#else
	#define CONFIG_ENV_IS_NOWHERE	1
#endif
*/

#define CONFIG_ENV_IS_NOWHERE

#define CONFIG_ENV_SECT_SIZE	(256 * 1024)
#define CONFIG_ENV_SIZE         CONFIG_ENV_SECT_SIZE

/*-----------------------------------------------------------------------
 * NAND FLASH driver setup
 */
#define CONFIG_SYS_NO_FLASH
#define NAND_MAX_CHIPS		1
#define CONFIG_SYS_MAX_NAND_DEVICE	1
#define CONFIG_SYS_NAND_BASE		0x40000000

/*
 * JFFS2 partitions TODO:
 */
/*
#undef CONFIG_JFFS2_CMDLINE
#define CONFIG_JFFS2_DEV	"nand0"
*/
#endif /* __CONFIG_H */

