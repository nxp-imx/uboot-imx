/*
 * (C) Copyright 2008-2009 Freescale Semiconductor, Inc.
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

#ifndef __IMX_SPI_H__
#define __IMX_SPI_H__

#include <spi.h>

#undef IMX_SPI_DEBUG

#define IMX_SPI_ACTIVE_HIGH     1
#define IMX_SPI_ACTIVE_LOW      0
#define SPI_RETRY_TIMES         100

#if defined(IMX_CSPI_VER_0_7)
	#define	SPI_RX_DATA		0x0
	#define SPI_TX_DATA		0x4
	#define SPI_CON_REG		0x8
	#define SPI_INT_REG		0xC
	#define SPI_DMA_REG		0x10
	#define SPI_STAT_REG		0x14
	#define SPI_PERIOD_REG		0x18

	#define SPI_CTRL_EN		(1 << 0)
	#define SPI_CTRL_MODE		(1 << 1)
	#define SPI_CTRL_REG_XCH_BIT	(1 << 2)
	#define SPI_CTRL_SSPOL		(1 << 7)
	#define SPI_CTRL_SSPOL_OFF	(7)
	#define SPI_CTRL_SSCTL		(1 << 6)
	#define SPI_CTRL_SSCTL_OFF	(6)
	#define SPI_CTRL_SCLK_POL	(1 << 4)
	#define SPI_CTRL_SCLK_POL_OFF	(4)
	#define SPI_CTRL_SCLK_PHA	(1 << 5)
	#define SPI_CTRL_SCLK_PHA_OFF	(5)
	#define SPI_CTRL_SS_OFF		(12)
	#define SPI_CTRL_SS_MASK	(3 << 12)
	#define SPI_CTRL_DATA_OFF	(16)
	#define SPI_CTRL_DATA_MASK	(7 << 16)
	#define SPI_CTRL_BURST_OFF	(20)
	#define SPI_CTRL_BURST_MASK	(0xFFF << 20)
	#define SPI_INT_STAT_TC		(1 << 7)

#elif defined(IMX_CSPI_VER_2_3)
	#define	SPI_RX_DATA		0x0
	#define SPI_TX_DATA		0x4
	#define SPI_CON_REG		0x8
	#define SPI_CFG_REG		0xC
	#define SPI_INT_REG		0x10
	#define SPI_DMA_REG		0x14
	#define SPI_STAT_REG		0x18
	#define SPI_PERIOD_REG		0x1C
#endif

struct spi_reg_t {
	u32 ctrl_reg;
	u32 cfg_reg;
};

struct imx_spi_dev_t {
	struct spi_slave slave;
	u32 base;      /* base address of SPI module the device is connected to */
	u32 freq;      /* desired clock freq in Hz for this device */
	u32 ss_pol;    /* ss polarity: 1=active high; 0=active low */
	u32 ss;        /* slave select */
	u32 in_sctl;   /* inactive sclk ctl: 1=stay low; 0=stay high */
	u32 in_dctl;   /* inactive data ctl: 1=stay low; 0=stay high */
	u32 ssctl;     /* single burst mode vs multiple: 0=single; 1=multi */
	u32 sclkpol;   /* sclk polarity: active high=0; active low=1 */
	u32 sclkpha;   /* sclk phase: 0=phase 0; 1=phase1 */
	u32 fifo_sz;   /* fifo size in bytes for either tx or rx. Don't add them up! */
	u32 us_delay;  /* us delay in each xfer */
	struct spi_reg_t reg; /* pointer to a set of SPI registers */
};

extern void spi_io_init(struct imx_spi_dev_t *dev);

#endif /* __IMX_SPI_H__ */
