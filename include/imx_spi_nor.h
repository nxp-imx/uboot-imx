/*
 * (C) Copyright 2008-2010 Freescale Semiconductor, Inc.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef _IMX_SPI_NOR_H_
#define _IMX_SPI_NOR_H_

#define READ        0x03    /* tx:1 byte cmd + 3 byte addr;rx:variable bytes */
#define READ_HS     0x0B    /* tx:1 byte cmd + 3 byte addr + 1 byte dummy; */
#define RDSR        0x05    /* read stat reg 1 byte tx cmd + 1 byte rx status */
#define RDSR_BUSY       (1 << 0)    /* 1=write-in-progress (default 0) */
#define RDSR_WEL        (1 << 1)    /* 1=write enable (default 0) */
#define RDSR_BP0        (1 << 2)    /* block write prot level (default 1) */
#define RDSR_BP1        (1 << 3)    /* block write prot level (default 1) */
#define RDSR_BP2        (1 << 4)    /* block write prot level (default 1) */
#define RDSR_BP3        (1 << 5)    /* block write prot level (default 1) */
#define RDSR_AAI        (1 << 6)    /* 1=AAI prog mode; 0=byte prog (def 0) */
#define RDSR_BPL        (1 << 7)    /* 1=BP3,BP2,BP1,BP0 RO; 0=R/W (def 0)  */
#define WREN        0x06    /* write enable. 1 byte tx cmd */
#define WRDI        0x04    /* write disable. 1 byte tx cmd */
#define EWSR        0x50    /* Enable write status. 1 byte tx cmd */
#define WRSR        0x01    /* Write stat reg. 1 byte tx cmd + 1 byte tx val */
#define ERASE_4K    0x20    /* sector erase. 1 byte cmd + 3 byte addr */
#define ERASE_32K   0x52    /* 32K block erase. 1 byte cmd + 3 byte addr */
#define ERASE_64K   0xD8    /* 64K block erase. 1 byte cmd + 3 byte addr */
#define ERASE_CHIP  0x60    /* whole chip erase */
#define BYTE_PROG   0x02    /* all tx: 1 cmd + 3 addr + 1 data */
#define AAI_PROG    0xAD    /* all tx: [1 cmd + 3 addr + 2 data] + RDSR */
				/* + [1cmd + 2 data] + .. + [WRDI] + [RDSR] */
#define JEDEC_ID    0x9F    /* read JEDEC ID. tx: 1 byte cmd; rx: 3 byte ID */

/* Atmel SPI-NOR commands */
#define WR_2_MEM_DIR	0x82
#define BUF1_WR		0x84
#define BUF2_WR		0x87
#define BUF1_TO_MEM	0x83
#define BUF2_TO_MEM	0x86
#define STAT_READ	0xD7
#define STAT_PG_SZ	(1 << 0)  /* 1=Page size is 512, 0=Page size is 528 */
#define STAT_PROT	(1 << 1)  /* 1=sector protection enabled (default 0) */
#define STAT_COMP	(1 << 6)
#define STAT_BUSY	(1 << 7) /* 1=Device not busy */
#define CONFIG_REG1	0x3D
#define CONFIG_REG2	0x2A
#define CONFIG_REG3	0x80
#define CONFIG_REG4	0xA6

#define SZ_64K      0x10000
#define SZ_32K      0x8000
#define SZ_4K       0x1000

#endif /* _IMX_SPI_NOR_H_ */
