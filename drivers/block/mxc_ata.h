#ifndef _MXC_ATA_H_
#define _MXC_ATA_H_

/*
 * (C) Copyright 2008-2009 Freescale Semiconductor, Inc.
 *
 * See file CREDITS for	list of people who contributed to this
 * project.
 *
 * This	program	is free	software; you can redistribute it and/or
 * modify it under the terms of	the GNU General Public License as
 * published by	the Free Software Foundation; either version 2 of
 * the License,	or (at your option) any later version.
 *
 * This	program	is distributed in the hope that	it will	be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59	Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#define MXC_ATA_TIMING_REGS                             0x00
#define MXC_ATA_FIFO_FILL                                   0x20
#define MXC_ATA_CONTROL                                     0x24
#define MXC_ATA_INT_PEND                                   0x28
#define MXC_ATA_INT_EN                                       0x2C
#define MXC_ATA_INT_CLEAR                                 0x30
#define MXC_ATA_FIFO_ALARM                              0x34
#define MXC_ATA_ADMA_ERROR_STATUS               0x38
#define MXC_ATA_SYS_DMA_BADDR                       0x3C
#define MXC_ATA_ADMA_SYS_ADDR                       0x40
#define MXC_ATA_BLOCK_COUNT                            0x48
#define MXC_ATA_BURST_LENGTH                          0x4C
#define MXC_ATA_SECTOR_SIZE                             0x50
#define MXC_ATA_DRIVE_DATA                              0xA0
#define MXC_ATA_DFTR                                          0xA4
#define MXC_ATA_DSCR                                          0xA8
#define MXC_ATA_DSNR                                          0xAC
#define MXC_ATA_DCLR                                           0xB0
#define MXC_ATA_DCHR                                          0xB4
#define MXC_ATA_DDHR                                          0xB8
#define MXC_ATA_DCDR                                          0xBC
#define MXC_ATA_DRIVE_CONTROL                         0xD8

/* bits within MXC_ATA_CONTROL */
#define MXC_ATA_CTRL_DMA_SRST                         0x1000
#define MXC_ATA_CTRL_DMA_64ADMA                    0x800
#define MXC_ATA_CTRL_DMA_32ADMA                    0x400
#define MXC_ATA_CTRL_DMA_STAT_STOP               0x200
#define MXC_ATA_CTRL_DMA_ENABLE                     0x100
#define MXC_ATA_CTRL_FIFO_RST_B                       0x80
#define MXC_ATA_CTRL_ATA_RST_B                        0x40
#define MXC_ATA_CTRL_FIFO_TX_EN                      0x20
#define MXC_ATA_CTRL_FIFO_RCV_EN                    0x10
#define MXC_ATA_CTRL_DMA_PENDING                   0x08
#define MXC_ATA_CTRL_DMA_ULTRA                       0x04
#define MXC_ATA_CTRL_DMA_WRITE                       0x02
#define MXC_ATA_CTRL_IORDY_EN                          0x01

/* bits within the interrupt control registers */
#define MXC_ATA_INTR_ATA_INTRQ1                      0x80
#define MXC_ATA_INTR_FIFO_UNDERFLOW             0x40
#define MXC_ATA_INTR_FIFO_OVERFLOW               0x20
#define MXC_ATA_INTR_CTRL_IDLE                         0x10
#define MXC_ATA_INTR_ATA_INTRQ2                      0x08
#define MXC_ATA_INTR_DMA_ERR                           0x04
#define MXC_ATA_INTR_DMA_TRANS_OVER            0x02

/* ADMA Addr Descriptor Attribute Filed */
#define MXC_ADMA_DES_ATTR_VALID                     0x01
#define MXC_ADMA_DES_ATTR_END                        0x02
#define MXC_ADMA_DES_ATTR_INT                         0x04
#define MXC_ADMA_DES_ATTR_SET                         0x10
#define MXC_ADMA_DES_ATTR_TRAN                      0x20
#define MXC_ADMA_DES_ATTR_LINK                       0x30

#define PIO_XFER_MODE_0                                     0
#define PIO_XFER_MODE_1                                     1
#define PIO_XFER_MODE_2                                     2
#define PIO_XFER_MODE_3                                     3
#define PIO_XFER_MODE_4                                     4

#define ATA_SECTOR_SIZE                                     512
#define MAX_SECTORS                       256

#endif /* _IMX_ATA_H_ */
