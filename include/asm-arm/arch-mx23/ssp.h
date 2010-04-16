/*
 * Copyright (C) 2008 Embedded Alley Solutions Inc.
 *
 * (C) Copyright 2009-2010 Freescale Semiconductor, Inc.
 *
 * SSP register definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef SSP_H
#define SSP_H

#include <asm/arch/mx23.h>

#define SSP1_BASE	(MX23_REGS_BASE + 0x10000)
#define SSP2_BASE	(MX23_REGS_BASE + 0x34000)

#define SSP_CTRL0	0x000
#define SSP_CMD0	0x010
#define SSP_CMD1	0x020
#define SSP_COMPREF	0x030
#define SSP_COMPMASK	0x040
#define SSP_TIMING	0x050
#define SSP_CTRL1	0x060
#define SSP_DATA	0x070
#define SSP_SDRESP0	0x080
#define SSP_SDRESP1	0x090
#define SSP_SDRESP2	0x0a0
#define SSP_SDRESP3	0x0b0
#define SSP_STATUS	0x0c0
#define SSP_DEBUG	0x100
#define SSP_VERSION	0x110

/* CTRL0 bits, bit fields and values */
#define CTRL0_SFTRST		(0x1 << 31)
#define CTRL0_CLKGATE		(0x1 << 30)
#define	CTRL0_RUN		(0x1 << 29)
#define CTRL0_LOCK_CS		(0x1 << 27)
#define CTRL0_IGNORE_CRC	(0x1 << 26)
#define CTRL0_DATA_XFER		(0x1 << 24)
#define CTRL0_READ		(0x1 << 25)
#define CTRL0_BUS_WIDTH		22
#define CTRL0_WAIT_FOR_IRQ	(0x1 << 21)
#define CTRL0_WAIT_FOR_CMD	(0x1 << 20)
#define CTRL0_XFER_COUNT	0

#define BUS_WIDTH_SPI1	(0x0 << CTRL0_BUS_WIDTH)
#define BUS_WIDTH_SPI4	(0x1 << CTRL0_BUS_WIDTH)
#define BUS_WIDTH_SPI8	(0x2 << CTRL0_BUS_WIDTH)

#define SPI_CS0		0x0
#define SPI_CS1		CTRL0_WAIT_FOR_CMD
#define SPI_CS2		CTRL0_WAIT_FOR_IRQ
#define SPI_CS_CLR_MASK	(CTRL0_WAIT_FOR_CMD | CTRL0_WAIT_FOR_IRQ)

/* CMD0 bits, bit fields and values */
#define CMD0_BLOCK_SIZE		16
#define CMD0_BLOCK_COUNT	12
#define CMD0_CMD		0

/* TIMING bits, bit fields and values */
#define TIMING_TIMEOUT		16
#define TIMING_CLOCK_DIVIDE	8
#define TIMING_CLOCK_RATE	0

/* CTRL1 bits, bit fields and values */
#define CTRL1_DMA_ENABLE	(0x1 << 13)
#define CTRL1_PHASE		(0x1 << 10)
#define CTRL1_POLARITY		(0x1 << 9)
#define CTRL1_SLAVE_MODE	(0x1 << 8)
#define CTRL1_WORD_LENGTH	4
#define CTRL1_SSP_MODE		0

#define WORD_LENGTH4	(0x3 << CTRL1_WORD_LENGTH)
#define WORD_LENGTH8	(0x7 << CTRL1_WORD_LENGTH)
#define WORD_LENGTH16	(0xF << CTRL1_WORD_LENGTH)

#define SSP_MODE_SPI	(0x0 << CTRL1_SSP_MODE)
#define SSP_MODE_SSI	(0x1 << CTRL1_SSP_MODE)
#define SSP_MODE_SD_MMC	(0x3 << CTRL1_SSP_MODE)
#define SSP_MODE_MS	(0x4 << CTRL1_SSP_MODE)
#define SSP_MODE_ATA	(0x7 << CTRL1_SSP_MODE)

/* CTRL1 bits, bit fields and values */
#define STATUS_FIFO_EMPTY	(1 << 5)
#define STATUS_FIFO_FULL	(1 << 8)

#endif /* SSP_H */
