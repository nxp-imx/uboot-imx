/*
 * Freescale GPMI Register Definitions
 *
 * Copyright 2008-2012 Freescale Semiconductor, Inc. All Rights Reserved.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * This file is created by xml file. Don't Edit it.
 *
 * Xml Revision: 1.19
 * Template revision: 1.3
 */

#ifndef __GPMI_NFC_GPMI_REGS_H
#define __GPMI_NFC_GPMI_REGS_H

#include <linux/mtd/mtd.h>
#include <linux/mtd/compat.h>
#include <linux/err.h>
#include <common.h>

#define HW_GPMI_CTRL0	(0x00000000)
#define HW_GPMI_CTRL0_SET	(0x00000004)
#define HW_GPMI_CTRL0_CLR	(0x00000008)
#define HW_GPMI_CTRL0_TOG	(0x0000000c)

#define BM_GPMI_CTRL0_SFTRST 0x80000000
#define BV_GPMI_CTRL0_SFTRST__RUN   0x0
#define BV_GPMI_CTRL0_SFTRST__RESET 0x1
#define BM_GPMI_CTRL0_CLKGATE 0x40000000
#define BV_GPMI_CTRL0_CLKGATE__RUN     0x0
#define BV_GPMI_CTRL0_CLKGATE__NO_CLKS 0x1
#define BM_GPMI_CTRL0_RUN 0x20000000
#define BV_GPMI_CTRL0_RUN__IDLE 0x0
#define BV_GPMI_CTRL0_RUN__BUSY 0x1
#define BM_GPMI_CTRL0_DEV_IRQ_EN 0x10000000
#if defined(CONFIG_GPMI_NFC_V0)
#define BM_GPMI_CTRL0_TIMEOUT_IRQ_EN 0x08000000
#else
#define BM_GPMI_CTRL0_LOCK_CS 0x08000000
#define BV_GPMI_CTRL0_LOCK_CS__DISABLED 0x0
#define BV_GPMI_CTRL0_LOCK_CS__ENABLED  0x1
#endif
#define BM_GPMI_CTRL0_UDMA 0x04000000
#define BV_GPMI_CTRL0_UDMA__DISABLED 0x0
#define BV_GPMI_CTRL0_UDMA__ENABLED  0x1
#define BP_GPMI_CTRL0_COMMAND_MODE      24
#define BM_GPMI_CTRL0_COMMAND_MODE 0x03000000
#define BF_GPMI_CTRL0_COMMAND_MODE(v)  \
	(((v) << 24) & BM_GPMI_CTRL0_COMMAND_MODE)
#define BV_GPMI_CTRL0_COMMAND_MODE__WRITE            0x0
#define BV_GPMI_CTRL0_COMMAND_MODE__READ             0x1
#define BV_GPMI_CTRL0_COMMAND_MODE__READ_AND_COMPARE 0x2
#define BV_GPMI_CTRL0_COMMAND_MODE__WAIT_FOR_READY   0x3
#define BM_GPMI_CTRL0_WORD_LENGTH 0x00800000
#define BV_GPMI_CTRL0_WORD_LENGTH__16_BIT 0x0
#define BV_GPMI_CTRL0_WORD_LENGTH__8_BIT  0x1
#if defined(CONFIG_GPMI_NFC_V0)
#define BM_GPMI_CTRL0_LOCK_CS            0x00400000
#define BV_GPMI_CTRL0_LOCK_CS__DISABLED  0x0
#define BV_GPMI_CTRL0_LOCK_CS__ENABLED   0x1
#endif
#define BP_GPMI_CTRL0_CS      20
#define BM_GPMI_CTRL0_CS 0x00700000
#define BF_GPMI_CTRL0_CS(v)  \
	(((v) << 20) & BM_GPMI_CTRL0_CS)
#define BP_GPMI_CTRL0_ADDRESS      17
#define BM_GPMI_CTRL0_ADDRESS 0x000E0000
#define BF_GPMI_CTRL0_ADDRESS(v)  \
	(((v) << 17) & BM_GPMI_CTRL0_ADDRESS)
#define BV_GPMI_CTRL0_ADDRESS__NAND_DATA 0x0
#define BV_GPMI_CTRL0_ADDRESS__NAND_CLE  0x1
#define BV_GPMI_CTRL0_ADDRESS__NAND_ALE  0x2
#define BM_GPMI_CTRL0_ADDRESS_INCREMENT 0x00010000
#define BV_GPMI_CTRL0_ADDRESS_INCREMENT__DISABLED 0x0
#define BV_GPMI_CTRL0_ADDRESS_INCREMENT__ENABLED  0x1
#define BP_GPMI_CTRL0_XFER_COUNT      0
#define BM_GPMI_CTRL0_XFER_COUNT 0x0000FFFF
#define BF_GPMI_CTRL0_XFER_COUNT(v)  \
	(((v) << 0) & BM_GPMI_CTRL0_XFER_COUNT)

#define HW_GPMI_COMPARE	(0x00000010)

#define BP_GPMI_COMPARE_MASK      16
#define BM_GPMI_COMPARE_MASK 0xFFFF0000
#define BF_GPMI_COMPARE_MASK(v) \
	(((v) << 16) & BM_GPMI_COMPARE_MASK)
#define BP_GPMI_COMPARE_REFERENCE      0
#define BM_GPMI_COMPARE_REFERENCE 0x0000FFFF
#define BF_GPMI_COMPARE_REFERENCE(v)  \
	(((v) << 0) & BM_GPMI_COMPARE_REFERENCE)

#define HW_GPMI_ECCCTRL	(0x00000020)
#define HW_GPMI_ECCCTRL_SET	(0x00000024)
#define HW_GPMI_ECCCTRL_CLR	(0x00000028)
#define HW_GPMI_ECCCTRL_TOG	(0x0000002c)

#define BP_GPMI_ECCCTRL_HANDLE      16
#define BM_GPMI_ECCCTRL_HANDLE 0xFFFF0000
#define BF_GPMI_ECCCTRL_HANDLE(v) \
	(((v) << 16) & BM_GPMI_ECCCTRL_HANDLE)
#define BM_GPMI_ECCCTRL_RSVD2 0x00008000
#define BP_GPMI_ECCCTRL_ECC_CMD      13
#define BM_GPMI_ECCCTRL_ECC_CMD 0x00006000
#define BF_GPMI_ECCCTRL_ECC_CMD(v)  \
	(((v) << 13) & BM_GPMI_ECCCTRL_ECC_CMD)
#if defined(CONFIG_GPMI_NFC_V0)
#define BV_GPMI_ECCCTRL_ECC_CMD__DECODE_4_BIT 0x0
#define BV_GPMI_ECCCTRL_ECC_CMD__ENCODE_4_BIT 0x1
#define BV_GPMI_ECCCTRL_ECC_CMD__DECODE_8_BIT 0x2
#define BV_GPMI_ECCCTRL_ECC_CMD__ENCODE_8_BIT 0x3
#define BV_GPMI_ECCCTRL_ECC_CMD__BCH_DECODE 0x0
#define BV_GPMI_ECCCTRL_ECC_CMD__BCH_ENCODE 0x1
#else
#define BV_GPMI_ECCCTRL_ECC_CMD__DECODE   0x0
#define BV_GPMI_ECCCTRL_ECC_CMD__ENCODE   0x1
#define BV_GPMI_ECCCTRL_ECC_CMD__RESERVE2 0x2
#define BV_GPMI_ECCCTRL_ECC_CMD__RESERVE3 0x3
#endif
#define BM_GPMI_ECCCTRL_ENABLE_ECC 0x00001000
#define BV_GPMI_ECCCTRL_ENABLE_ECC__ENABLE  0x1
#define BV_GPMI_ECCCTRL_ENABLE_ECC__DISABLE 0x0
#define BP_GPMI_ECCCTRL_RSVD1      9
#define BM_GPMI_ECCCTRL_RSVD1 0x00000E00
#define BF_GPMI_ECCCTRL_RSVD1(v)  \
	(((v) << 9) & BM_GPMI_ECCCTRL_RSVD1)
#define BP_GPMI_ECCCTRL_BUFFER_MASK      0
#define BM_GPMI_ECCCTRL_BUFFER_MASK 0x000001FF
#define BF_GPMI_ECCCTRL_BUFFER_MASK(v)  \
	(((v) << 0) & BM_GPMI_ECCCTRL_BUFFER_MASK)
#define BV_GPMI_ECCCTRL_BUFFER_MASK__BCH_AUXONLY 0x100
#define BV_GPMI_ECCCTRL_BUFFER_MASK__BCH_PAGE    0x1FF
#if defined(CONFIG_GPMI_NFC_V0)
#define BV_GPMI_ECCCTRL_BUFFER_MASK__AUXILIARY   0x100
#define BV_GPMI_ECCCTRL_BUFFER_MASK__BUFFER7     0x080
#define BV_GPMI_ECCCTRL_BUFFER_MASK__BUFFER6     0x040
#define BV_GPMI_ECCCTRL_BUFFER_MASK__BUFFER5     0x020
#define BV_GPMI_ECCCTRL_BUFFER_MASK__BUFFER4     0x010
#define BV_GPMI_ECCCTRL_BUFFER_MASK__BUFFER3     0x008
#define BV_GPMI_ECCCTRL_BUFFER_MASK__BUFFER2     0x004
#define BV_GPMI_ECCCTRL_BUFFER_MASK__BUFFER1     0x002
#define BV_GPMI_ECCCTRL_BUFFER_MASK__BUFFER0     0x001
#endif

#define HW_GPMI_ECCCOUNT	(0x00000030)

#define BP_GPMI_ECCCOUNT_RSVD2      16
#define BM_GPMI_ECCCOUNT_RSVD2 0xFFFF0000
#define BF_GPMI_ECCCOUNT_RSVD2(v) \
	(((v) << 16) & BM_GPMI_ECCCOUNT_RSVD2)
#define BP_GPMI_ECCCOUNT_COUNT      0
#define BM_GPMI_ECCCOUNT_COUNT 0x0000FFFF
#define BF_GPMI_ECCCOUNT_COUNT(v)  \
	(((v) << 0) & BM_GPMI_ECCCOUNT_COUNT)

#define HW_GPMI_PAYLOAD	(0x00000040)

#define BP_GPMI_PAYLOAD_ADDRESS      2
#define BM_GPMI_PAYLOAD_ADDRESS 0xFFFFFFFC
#define BF_GPMI_PAYLOAD_ADDRESS(v) \
	(((v) << 2) & BM_GPMI_PAYLOAD_ADDRESS)
#define BP_GPMI_PAYLOAD_RSVD0      0
#define BM_GPMI_PAYLOAD_RSVD0 0x00000003
#define BF_GPMI_PAYLOAD_RSVD0(v)  \
	(((v) << 0) & BM_GPMI_PAYLOAD_RSVD0)

#define HW_GPMI_AUXILIARY	(0x00000050)

#define BP_GPMI_AUXILIARY_ADDRESS      2
#define BM_GPMI_AUXILIARY_ADDRESS 0xFFFFFFFC
#define BF_GPMI_AUXILIARY_ADDRESS(v) \
	(((v) << 2) & BM_GPMI_AUXILIARY_ADDRESS)
#define BP_GPMI_AUXILIARY_RSVD0      0
#define BM_GPMI_AUXILIARY_RSVD0 0x00000003
#define BF_GPMI_AUXILIARY_RSVD0(v)  \
	(((v) << 0) & BM_GPMI_AUXILIARY_RSVD0)

#define HW_GPMI_CTRL1	(0x00000060)
#define HW_GPMI_CTRL1_SET	(0x00000064)
#define HW_GPMI_CTRL1_CLR	(0x00000068)
#define HW_GPMI_CTRL1_TOG	(0x0000006c)

#if defined(CONFIG_GPMI_NFC_V0)

#define BP_GPMI_CTRL1_RSVD2	24
#define BM_GPMI_CTRL1_RSVD2	0xFF000000
#define BF_GPMI_CTRL1_RSVD2(v) \
		(((v) << 24) & BM_GPMI_CTRL1_RSVD2)
#define BM_GPMI_CTRL1_CE3_SEL	0x00800000
#define BM_GPMI_CTRL1_CE2_SEL	0x00400000
#define BM_GPMI_CTRL1_CE1_SEL	0x00200000
#define BM_GPMI_CTRL1_CE0_SEL	0x00100000
#define BM_GPMI_CTRL1_GANGED_RDYBUSY	0x00080000
#define BM_GPMI_CTRL1_GPMI_MODE	0x00000001
#define BP_GPMI_CTRL1_GPMI_MODE	0
#define BM_GPMI_CTRL1_ATA_IRQRDY_POLARITY	0x00000004
#define BM_GPMI_CTRL1_DEV_RESET	0x00000008
#define BM_GPMI_CTRL1_TIMEOUT_IRQ	0x00000200
#define BM_GPMI_CTRL1_DEV_IRQ	0x00000400
#define BM_GPMI_CTRL1_RDN_DELAY	0x0000F000
#define BP_GPMI_CTRL1_RDN_DELAY	12
#define BM_GPMI_CTRL1_BCH_MODE	0x00040000
#define BP_GPMI_CTRL1_DLL_ENABLE	17

#else

#if defined(CONFIG_GPMI_NFC_V1)
#define BP_GPMI_CTRL1_RSVD2	25
#define BM_GPMI_CTRL1_RSVD2	0xFE000000
#define BF_GPMI_CTRL1_RSVD2(v) \
		(((v) << 25) & BM_GPMI_CTRL1_RSVD2)
#elif defined(CONFIG_GPMI_NFC_V2)
#define BM_GPMI_CTRL1_DEV_CLK_STOP 0x80000000
#define BM_GPMI_CTRL1_SSYNC_CLK_STOP 0x40000000
#define BM_GPMI_CTRL1_WRITE_CLK_STOP 0x20000000
#define BM_GPMI_CTRL1_TOGGLE_MODE 0x10000000
#define BM_GPMI_CTRL1_GPMI_CLK_DIV2_EN 0x08000000
#define BM_GPMI_CTRL1_UPDATE_CS 0x04000000
#define BM_GPMI_CTRL1_SSYNCMODE 0x02000000
#define BV_GPMI_CTRL1_SSYNCMODE__ASYNC 0x0
#define BV_GPMI_CTRL1_SSYNCMODE__SSYNC 0x1
#endif
#define BM_GPMI_CTRL1_DECOUPLE_CS 0x01000000
#define BP_GPMI_CTRL1_WRN_DLY_SEL      22
#define BM_GPMI_CTRL1_WRN_DLY_SEL 0x00C00000
#define BF_GPMI_CTRL1_WRN_DLY_SEL(v)  \
	(((v) << 22) & BM_GPMI_CTRL1_WRN_DLY_SEL)
#define BM_GPMI_CTRL1_RSVD1 0x00200000
#define BM_GPMI_CTRL1_TIMEOUT_IRQ_EN 0x00100000
#define BM_GPMI_CTRL1_GANGED_RDYBUSY 0x00080000
#define BM_GPMI_CTRL1_BCH_MODE 0x00040000

#endif

#define BP_GPMI_CTRL1_DLL_ENABLE	17
#define BM_GPMI_CTRL1_DLL_ENABLE 0x00020000
#define BP_GPMI_CTRL1_HALF_PERIOD       16
#define BM_GPMI_CTRL1_HALF_PERIOD 0x00010000
#define BP_GPMI_CTRL1_RDN_DELAY      12
#define BM_GPMI_CTRL1_RDN_DELAY 0x0000F000
#define BF_GPMI_CTRL1_RDN_DELAY(v)  \
	(((v) << 12) & BM_GPMI_CTRL1_RDN_DELAY)
#define BM_GPMI_CTRL1_DMA2ECC_MODE 0x00000800
#define BM_GPMI_CTRL1_DEV_IRQ 0x00000400
#define BM_GPMI_CTRL1_TIMEOUT_IRQ 0x00000200
#define BM_GPMI_CTRL1_BURST_EN 0x00000100
#if defined(CONFIG_GPMI_NFC_V0)
#define BM_GPMI_CTRL1_ABORT_WAIT_FOR_READY3	0x00000080
#define BM_GPMI_CTRL1_ABORT_WAIT_FOR_READY2	0x00000040
#define BM_GPMI_CTRL1_ABORT_WAIT_FOR_READY1	0x00000020
#define BM_GPMI_CTRL1_ABORT_WAIT_FOR_READY0	0x00000010
#else
#define BM_GPMI_CTRL1_ABORT_WAIT_REQUEST 0x00000080
#define BP_GPMI_CTRL1_ABORT_WAIT_FOR_READY_CHANNEL      4
#define BM_GPMI_CTRL1_ABORT_WAIT_FOR_READY_CHANNEL 0x00000070
#define BF_GPMI_CTRL1_ABORT_WAIT_FOR_READY_CHANNEL(v)  \
	(((v) << 4) & BM_GPMI_CTRL1_ABORT_WAIT_FOR_READY_CHANNEL)
#endif
#define BM_GPMI_CTRL1_DEV_RESET 0x00000008
#define BV_GPMI_CTRL1_DEV_RESET__ENABLED  0x0
#define BV_GPMI_CTRL1_DEV_RESET__DISABLED 0x1
#define BM_GPMI_CTRL1_ATA_IRQRDY_POLARITY 0x00000004
#define BV_GPMI_CTRL1_ATA_IRQRDY_POLARITY__ACTIVELOW  0x0
#define BV_GPMI_CTRL1_ATA_IRQRDY_POLARITY__ACTIVEHIGH 0x1
#define BM_GPMI_CTRL1_CAMERA_MODE 0x00000002
#define BM_GPMI_CTRL1_GPMI_MODE 0x00000001
#define BV_GPMI_CTRL1_GPMI_MODE__NAND 0x0
#define BV_GPMI_CTRL1_GPMI_MODE__ATA  0x1

#define HW_GPMI_TIMING0	(0x00000070)

#define BP_GPMI_TIMING0_RSVD1      24
#define BM_GPMI_TIMING0_RSVD1 0xFF000000
#define BF_GPMI_TIMING0_RSVD1(v) \
	(((v) << 24) & BM_GPMI_TIMING0_RSVD1)
#define BP_GPMI_TIMING0_ADDRESS_SETUP      16
#define BM_GPMI_TIMING0_ADDRESS_SETUP 0x00FF0000
#define BF_GPMI_TIMING0_ADDRESS_SETUP(v)  \
	(((v) << 16) & BM_GPMI_TIMING0_ADDRESS_SETUP)
#define BP_GPMI_TIMING0_DATA_HOLD      8
#define BM_GPMI_TIMING0_DATA_HOLD 0x0000FF00
#define BF_GPMI_TIMING0_DATA_HOLD(v)  \
	(((v) << 8) & BM_GPMI_TIMING0_DATA_HOLD)
#define BP_GPMI_TIMING0_DATA_SETUP      0
#define BM_GPMI_TIMING0_DATA_SETUP 0x000000FF
#define BF_GPMI_TIMING0_DATA_SETUP(v)  \
	(((v) << 0) & BM_GPMI_TIMING0_DATA_SETUP)

#define HW_GPMI_TIMING1	(0x00000080)
#define HW_GPMI_TIMING1_SET	(0x00000084)

#define BP_GPMI_TIMING1_DEVICE_BUSY_TIMEOUT      16
#define BM_GPMI_TIMING1_DEVICE_BUSY_TIMEOUT 0xFFFF0000
#define BF_GPMI_TIMING1_DEVICE_BUSY_TIMEOUT(v) \
	(((v) << 16) & BM_GPMI_TIMING1_DEVICE_BUSY_TIMEOUT)
#define BP_GPMI_TIMING1_RSVD1      0
#define BM_GPMI_TIMING1_RSVD1 0x0000FFFF
#define BF_GPMI_TIMING1_RSVD1(v)  \
	(((v) << 0) & BM_GPMI_TIMING1_RSVD1)

#define HW_GPMI_TIMING2	(0x00000090)

#if !defined(CONFIG_GPMI_NFC_V2)

#define BP_GPMI_TIMING2_UDMA_TRP	24
#define BM_GPMI_TIMING2_UDMA_TRP	0xFF000000
#define BF_GPMI_TIMING2_UDMA_TRP(v) \
		(((v) << 24) & BM_GPMI_TIMING2_UDMA_TRP)
#define BP_GPMI_TIMING2_UDMA_ENV	16
#define BM_GPMI_TIMING2_UDMA_ENV	0x00FF0000
#define BF_GPMI_TIMING2_UDMA_ENV(v)  \
		(((v) << 16) & BM_GPMI_TIMING2_UDMA_ENV)
#define BP_GPMI_TIMING2_UDMA_HOLD	8
#define BM_GPMI_TIMING2_UDMA_HOLD	0x0000FF00
#define BF_GPMI_TIMING2_UDMA_HOLD(v)  \
		(((v) << 8) & BM_GPMI_TIMING2_UDMA_HOLD)
#define BP_GPMI_TIMING2_UDMA_SETUP	0
#define BM_GPMI_TIMING2_UDMA_SETUP	0x000000FF
#define BF_GPMI_TIMING2_UDMA_SETUP(v)  \
		(((v) << 0) & BM_GPMI_TIMING2_UDMA_SETUP)

#else

#define BP_GPMI_TIMING2_RSVD1      27
#define BM_GPMI_TIMING2_RSVD1 0xF8000000
#define BF_GPMI_TIMING2_RSVD1(v) \
	(((v) << 27) & BM_GPMI_TIMING2_RSVD1)
#define BP_GPMI_TIMING2_READ_LATENCY      24
#define BM_GPMI_TIMING2_READ_LATENCY 0x07000000
#define BF_GPMI_TIMING2_READ_LATENCY(v)  \
	(((v) << 24) & BM_GPMI_TIMING2_READ_LATENCY)
#define BP_GPMI_TIMING2_RSVD0      21
#define BM_GPMI_TIMING2_RSVD0 0x00E00000
#define BF_GPMI_TIMING2_RSVD0(v)  \
	(((v) << 21) & BM_GPMI_TIMING2_RSVD0)
#define BP_GPMI_TIMING2_CE_DELAY      16
#define BM_GPMI_TIMING2_CE_DELAY 0x001F0000
#define BF_GPMI_TIMING2_CE_DELAY(v)  \
	(((v) << 16) & BM_GPMI_TIMING2_CE_DELAY)
#define BP_GPMI_TIMING2_PREAMBLE_DELAY      12
#define BM_GPMI_TIMING2_PREAMBLE_DELAY 0x0000F000
#define BF_GPMI_TIMING2_PREAMBLE_DELAY(v)  \
	(((v) << 12) & BM_GPMI_TIMING2_PREAMBLE_DELAY)
#define BP_GPMI_TIMING2_POSTAMBLE_DELAY      8
#define BM_GPMI_TIMING2_POSTAMBLE_DELAY 0x00000F00
#define BF_GPMI_TIMING2_POSTAMBLE_DELAY(v)  \
	(((v) << 8) & BM_GPMI_TIMING2_POSTAMBLE_DELAY)
#define BP_GPMI_TIMING2_CMDADD_PAUSE      4
#define BM_GPMI_TIMING2_CMDADD_PAUSE 0x000000F0
#define BF_GPMI_TIMING2_CMDADD_PAUSE(v)  \
	(((v) << 4) & BM_GPMI_TIMING2_CMDADD_PAUSE)
#define BP_GPMI_TIMING2_DATA_PAUSE      0
#define BM_GPMI_TIMING2_DATA_PAUSE 0x0000000F
#define BF_GPMI_TIMING2_DATA_PAUSE(v)  \
	(((v) << 0) & BM_GPMI_TIMING2_DATA_PAUSE)

#endif

#define HW_GPMI_DATA	(0x000000a0)

#define BP_GPMI_DATA_DATA      0
#define BM_GPMI_DATA_DATA 0xFFFFFFFF
#define BF_GPMI_DATA_DATA(v)   (v)

#define HW_GPMI_STAT	(0x000000b0)

#if defined(CONFIG_GPMI_NFC_V0)

#define BM_GPMI_STAT_PRESENT	0x80000000
#define BV_GPMI_STAT_PRESENT__UNAVAILABLE 0x0
#define BV_GPMI_STAT_PRESENT__AVAILABLE   0x1
#define BP_GPMI_STAT_RSVD1	12
#define BM_GPMI_STAT_RSVD1	0x7FFFF000
#define BF_GPMI_STAT_RSVD1(v)  \
		(((v) << 12) & BM_GPMI_STAT_RSVD1)
#define BP_GPMI_STAT_RDY_TIMEOUT	8
#define BM_GPMI_STAT_RDY_TIMEOUT	0x00000F00
#define BF_GPMI_STAT_RDY_TIMEOUT(v)  \
		(((v) << 8) & BM_GPMI_STAT_RDY_TIMEOUT)
#define BM_GPMI_STAT_ATA_IRQ	0x00000080
#define BM_GPMI_STAT_INVALID_BUFFER_MASK	0x00000040
#define BM_GPMI_STAT_FIFO_EMPTY	0x00000020
#define BV_GPMI_STAT_FIFO_EMPTY__NOT_EMPTY 0x0
#define BV_GPMI_STAT_FIFO_EMPTY__EMPTY     0x1
#define BM_GPMI_STAT_FIFO_FULL	0x00000010
#define BV_GPMI_STAT_FIFO_FULL__NOT_FULL 0x0
#define BV_GPMI_STAT_FIFO_FULL__FULL     0x1
#define BM_GPMI_STAT_DEV3_ERROR	0x00000008
#define BM_GPMI_STAT_DEV2_ERROR	0x00000004
#define BM_GPMI_STAT_DEV1_ERROR	0x00000002
#define BM_GPMI_STAT_DEERROR	0x00000001

#else

#define BP_GPMI_STAT_READY_BUSY      24
#define BM_GPMI_STAT_READY_BUSY 0xFF000000
#define BF_GPMI_STAT_READY_BUSY(v) \
	(((v) << 24) & BM_GPMI_STAT_READY_BUSY)
#define BP_GPMI_STAT_RDY_TIMEOUT      16
#define BM_GPMI_STAT_RDY_TIMEOUT 0x00FF0000
#define BF_GPMI_STAT_RDY_TIMEOUT(v)  \
	(((v) << 16) & BM_GPMI_STAT_RDY_TIMEOUT)
#define BM_GPMI_STAT_DEV7_ERROR 0x00008000
#define BM_GPMI_STAT_DEV6_ERROR 0x00004000
#define BM_GPMI_STAT_DEV5_ERROR 0x00002000
#define BM_GPMI_STAT_DEV4_ERROR 0x00001000
#define BM_GPMI_STAT_DEV3_ERROR 0x00000800
#define BM_GPMI_STAT_DEV2_ERROR 0x00000400
#define BM_GPMI_STAT_DEV1_ERROR 0x00000200
#define BM_GPMI_STAT_DEV0_ERROR 0x00000100
#define BP_GPMI_STAT_RSVD1      5
#define BM_GPMI_STAT_RSVD1 0x000000E0
#define BF_GPMI_STAT_RSVD1(v)  \
	(((v) << 5) & BM_GPMI_STAT_RSVD1)
#define BM_GPMI_STAT_ATA_IRQ 0x00000010
#define BM_GPMI_STAT_INVALID_BUFFER_MASK 0x00000008
#define BM_GPMI_STAT_FIFO_EMPTY 0x00000004
#define BV_GPMI_STAT_FIFO_EMPTY__NOT_EMPTY 0x0
#define BV_GPMI_STAT_FIFO_EMPTY__EMPTY     0x1
#define BM_GPMI_STAT_FIFO_FULL 0x00000002
#define BV_GPMI_STAT_FIFO_FULL__NOT_FULL 0x0
#define BV_GPMI_STAT_FIFO_FULL__FULL     0x1
#define BM_GPMI_STAT_PRESENT 0x00000001
#define BV_GPMI_STAT_PRESENT__UNAVAILABLE 0x0
#define BV_GPMI_STAT_PRESENT__AVAILABLE   0x1

#endif

#define HW_GPMI_DEBUG	(0x000000c0)

#if defined(CONFIG_GPMI_NFC_V0)

#define BM_GPMI_DEBUG_READY3	0x80000000
#define BM_GPMI_DEBUG_READY2	0x40000000
#define BM_GPMI_DEBUG_READY1	0x20000000
#define BM_GPMI_DEBUG_READY0	0x10000000
#define BM_GPMI_DEBUG_WAIT_FOR_READY_END3	0x08000000
#define BM_GPMI_DEBUG_WAIT_FOR_READY_END2	0x04000000
#define BM_GPMI_DEBUG_WAIT_FOR_READY_END1	0x02000000
#define BM_GPMI_DEBUG_WAIT_FOR_READY_END0	0x01000000
#define BM_GPMI_DEBUG_SENSE3	0x00800000
#define BM_GPMI_DEBUG_SENSE2	0x00400000
#define BM_GPMI_DEBUG_SENSE1	0x00200000
#define BM_GPMI_DEBUG_SENSE0	0x00100000
#define BM_GPMI_DEBUG_DMAREQ3	0x00080000
#define BM_GPMI_DEBUG_DMAREQ2	0x00040000
#define BM_GPMI_DEBUG_DMAREQ1	0x00020000
#define BM_GPMI_DEBUG_DMAREQ0	0x00010000
#define BP_GPMI_DEBUG_CMD_END	12
#define BM_GPMI_DEBUG_CMD_END	0x0000F000
#define BF_GPMI_DEBUG_CMD_END(v)  \
		(((v) << 12) & BM_GPMI_DEBUG_CMD_END)
#define BP_GPMI_DEBUG_UDMA_STATE	8
#define BM_GPMI_DEBUG_UDMA_STATE	0x00000F00
#define BF_GPMI_DEBUG_UDMA_STATE(v)  \
		(((v) << 8) & BM_GPMI_DEBUG_UDMA_STATE)
#define BM_GPMI_DEBUG_BUSY	0x00000080
#define BV_GPMI_DEBUG_BUSY__DISABLED 0x0
#define BV_GPMI_DEBUG_BUSY__ENABLED  0x1
#define BP_GPMI_DEBUG_PIN_STATE	4
#define BM_GPMI_DEBUG_PIN_STATE	0x00000070
#define BF_GPMI_DEBUG_PIN_STATE(v)  \
		(((v) << 4) & BM_GPMI_DEBUG_PIN_STATE)
#define BV_GPMI_DEBUG_PIN_STATE__PSM_IDLE   0x0
#define BV_GPMI_DEBUG_PIN_STATE__PSM_BYTCNT 0x1
#define BV_GPMI_DEBUG_PIN_STATE__PSM_ADDR   0x2
#define BV_GPMI_DEBUG_PIN_STATE__PSM_STALL  0x3
#define BV_GPMI_DEBUG_PIN_STATE__PSM_STROBE 0x4
#define BV_GPMI_DEBUG_PIN_STATE__PSM_ATARDY 0x5
#define BV_GPMI_DEBUG_PIN_STATE__PSM_DHOLD  0x6
#define BV_GPMI_DEBUG_PIN_STATE__PSM_DONE   0x7
#define BP_GPMI_DEBUG_MAIN_STATE	0
#define BM_GPMI_DEBUG_MAIN_STATE	0x0000000F
#define BF_GPMI_DEBUG_MAIN_STATE(v)  \
		(((v) << 0) & BM_GPMI_DEBUG_MAIN_STATE)
#define BV_GPMI_DEBUG_MAIN_STATE__MSM_IDLE   0x0
#define BV_GPMI_DEBUG_MAIN_STATE__MSM_BYTCNT 0x1
#define BV_GPMI_DEBUG_MAIN_STATE__MSM_WAITFE 0x2
#define BV_GPMI_DEBUG_MAIN_STATE__MSM_WAITFR 0x3
#define BV_GPMI_DEBUG_MAIN_STATE__MSM_DMAREQ 0x4
#define BV_GPMI_DEBUG_MAIN_STATE__MSM_DMAACK 0x5
#define BV_GPMI_DEBUG_MAIN_STATE__MSM_WAITFF 0x6
#define BV_GPMI_DEBUG_MAIN_STATE__MSM_LDFIFO 0x7
#define BV_GPMI_DEBUG_MAIN_STATE__MSM_LDDMAR 0x8
#define BV_GPMI_DEBUG_MAIN_STATE__MSM_RDCMP  0x9
#define BV_GPMI_DEBUG_MAIN_STATE__MSM_DONE   0xA

#else

#define BP_GPMI_DEBUG_WAIT_FOR_READY_END      24
#define BM_GPMI_DEBUG_WAIT_FOR_READY_END 0xFF000000
#define BF_GPMI_DEBUG_WAIT_FOR_READY_END(v) \
	(((v) << 24) & BM_GPMI_DEBUG_WAIT_FOR_READY_END)
#define BP_GPMI_DEBUG_DMA_SENSE      16
#define BM_GPMI_DEBUG_DMA_SENSE 0x00FF0000
#define BF_GPMI_DEBUG_DMA_SENSE(v)  \
	(((v) << 16) & BM_GPMI_DEBUG_DMA_SENSE)
#define BP_GPMI_DEBUG_DMAREQ      8
#define BM_GPMI_DEBUG_DMAREQ 0x0000FF00
#define BF_GPMI_DEBUG_DMAREQ(v)  \
	(((v) << 8) & BM_GPMI_DEBUG_DMAREQ)
#define BP_GPMI_DEBUG_CMD_END      0
#define BM_GPMI_DEBUG_CMD_END 0x000000FF
#define BF_GPMI_DEBUG_CMD_END(v)  \
	(((v) << 0) & BM_GPMI_DEBUG_CMD_END)

#endif

#define HW_GPMI_VERSION	(0x000000d0)

#define BP_GPMI_VERSION_MAJOR      24
#define BM_GPMI_VERSION_MAJOR 0xFF000000
#define BF_GPMI_VERSION_MAJOR(v) \
	(((v) << 24) & BM_GPMI_VERSION_MAJOR)
#define BP_GPMI_VERSION_MINOR      16
#define BM_GPMI_VERSION_MINOR 0x00FF0000
#define BF_GPMI_VERSION_MINOR(v)  \
	(((v) << 16) & BM_GPMI_VERSION_MINOR)
#define BP_GPMI_VERSION_STEP      0
#define BM_GPMI_VERSION_STEP 0x0000FFFF
#define BF_GPMI_VERSION_STEP(v)  \
	(((v) << 0) & BM_GPMI_VERSION_STEP)

#define HW_GPMI_DEBUG2	(0x000000e0)

#if defined(CONFIG_GPMI_NFC_V0)

#define BP_GPMI_DEBUG2_RSVD1	16
#define BM_GPMI_DEBUG2_RSVD1	0xFFFF0000
#define BF_GPMI_DEBUG2_RSVD1(v)	(((v) << 16) & BM_GPMI_DEBUG2_RSVD1)

#else

#define BP_GPMI_DEBUG2_RSVD1      28
#define BM_GPMI_DEBUG2_RSVD1 0xF0000000
#define BF_GPMI_DEBUG2_RSVD1(v) \
	(((v) << 28) & BM_GPMI_DEBUG2_RSVD1)
#define BP_GPMI_DEBUG2_UDMA_STATE      24
#define BM_GPMI_DEBUG2_UDMA_STATE 0x0F000000
#define BF_GPMI_DEBUG2_UDMA_STATE(v)  \
	(((v) << 24) & BM_GPMI_DEBUG2_UDMA_STATE)
#define BM_GPMI_DEBUG2_BUSY 0x00800000
#define BV_GPMI_DEBUG2_BUSY__DISABLED 0x0
#define BV_GPMI_DEBUG2_BUSY__ENABLED  0x1
#define BP_GPMI_DEBUG2_PIN_STATE      20
#define BM_GPMI_DEBUG2_PIN_STATE 0x00700000
#define BF_GPMI_DEBUG2_PIN_STATE(v)  \
	(((v) << 20) & BM_GPMI_DEBUG2_PIN_STATE)
#define BV_GPMI_DEBUG2_PIN_STATE__PSM_IDLE   0x0
#define BV_GPMI_DEBUG2_PIN_STATE__PSM_BYTCNT 0x1
#define BV_GPMI_DEBUG2_PIN_STATE__PSM_ADDR   0x2
#define BV_GPMI_DEBUG2_PIN_STATE__PSM_STALL  0x3
#define BV_GPMI_DEBUG2_PIN_STATE__PSM_STROBE 0x4
#define BV_GPMI_DEBUG2_PIN_STATE__PSM_ATARDY 0x5
#define BV_GPMI_DEBUG2_PIN_STATE__PSM_DHOLD  0x6
#define BV_GPMI_DEBUG2_PIN_STATE__PSM_DONE   0x7
#define BP_GPMI_DEBUG2_MAIN_STATE      16
#define BM_GPMI_DEBUG2_MAIN_STATE 0x000F0000
#define BF_GPMI_DEBUG2_MAIN_STATE(v)  \
	(((v) << 16) & BM_GPMI_DEBUG2_MAIN_STATE)
#define BV_GPMI_DEBUG2_MAIN_STATE__MSM_IDLE   0x0
#define BV_GPMI_DEBUG2_MAIN_STATE__MSM_BYTCNT 0x1
#define BV_GPMI_DEBUG2_MAIN_STATE__MSM_WAITFE 0x2
#define BV_GPMI_DEBUG2_MAIN_STATE__MSM_WAITFR 0x3
#define BV_GPMI_DEBUG2_MAIN_STATE__MSM_DMAREQ 0x4
#define BV_GPMI_DEBUG2_MAIN_STATE__MSM_DMAACK 0x5
#define BV_GPMI_DEBUG2_MAIN_STATE__MSM_WAITFF 0x6
#define BV_GPMI_DEBUG2_MAIN_STATE__MSM_LDFIFO 0x7
#define BV_GPMI_DEBUG2_MAIN_STATE__MSM_LDDMAR 0x8
#define BV_GPMI_DEBUG2_MAIN_STATE__MSM_RDCMP  0x9
#define BV_GPMI_DEBUG2_MAIN_STATE__MSM_DONE   0xA
#define BP_GPMI_DEBUG2_SYND2GPMI_BE      12
#define BM_GPMI_DEBUG2_SYND2GPMI_BE 0x0000F000
#define BF_GPMI_DEBUG2_SYND2GPMI_BE(v)  \
	(((v) << 12) & BM_GPMI_DEBUG2_SYND2GPMI_BE)
#define BM_GPMI_DEBUG2_GPMI2SYND_VALID 0x00000800
#define BM_GPMI_DEBUG2_GPMI2SYND_READY 0x00000400
#define BM_GPMI_DEBUG2_SYND2GPMI_VALID 0x00000200
#define BM_GPMI_DEBUG2_SYND2GPMI_READY 0x00000100
#define BM_GPMI_DEBUG2_VIEW_DELAYED_RDN 0x00000080
#define BM_GPMI_DEBUG2_UPDATE_WINDOW 0x00000040
#define BP_GPMI_DEBUG2_RDN_TAP      0
#define BM_GPMI_DEBUG2_RDN_TAP 0x0000003F
#define BF_GPMI_DEBUG2_RDN_TAP(v)  \
	(((v) << 0) & BM_GPMI_DEBUG2_RDN_TAP)

#endif

#define HW_GPMI_DEBUG3	(0x000000f0)

#define BP_GPMI_DEBUG3_APB_WORD_CNTR      16
#define BM_GPMI_DEBUG3_APB_WORD_CNTR 0xFFFF0000
#define BF_GPMI_DEBUG3_APB_WORD_CNTR(v) \
	(((v) << 16) & BM_GPMI_DEBUG3_APB_WORD_CNTR)
#define BP_GPMI_DEBUG3_DEV_WORD_CNTR      0
#define BM_GPMI_DEBUG3_DEV_WORD_CNTR 0x0000FFFF
#define BF_GPMI_DEBUG3_DEV_WORD_CNTR(v)  \
	(((v) << 0) & BM_GPMI_DEBUG3_DEV_WORD_CNTR)

#if defined(CONFIG_GPMI_NFC_V2)
#define HW_GPMI_READ_DDR_DLL_CTRL	(0x00000100)

#define BP_GPMI_READ_DDR_DLL_CTRL_REF_UPDATE_INT      28
#define BM_GPMI_READ_DDR_DLL_CTRL_REF_UPDATE_INT 0xF0000000
#define BF_GPMI_READ_DDR_DLL_CTRL_REF_UPDATE_INT(v) \
	(((v) << 28) & BM_GPMI_READ_DDR_DLL_CTRL_REF_UPDATE_INT)
#define BP_GPMI_READ_DDR_DLL_CTRL_SLV_UPDATE_INT      20
#define BM_GPMI_READ_DDR_DLL_CTRL_SLV_UPDATE_INT 0x0FF00000
#define BF_GPMI_READ_DDR_DLL_CTRL_SLV_UPDATE_INT(v)  \
	(((v) << 20) & BM_GPMI_READ_DDR_DLL_CTRL_SLV_UPDATE_INT)
#define BP_GPMI_READ_DDR_DLL_CTRL_RSVD1      18
#define BM_GPMI_READ_DDR_DLL_CTRL_RSVD1 0x000C0000
#define BF_GPMI_READ_DDR_DLL_CTRL_RSVD1(v)  \
	(((v) << 18) & BM_GPMI_READ_DDR_DLL_CTRL_RSVD1)
#define BP_GPMI_READ_DDR_DLL_CTRL_SLV_OVERRIDE_VAL      10
#define BM_GPMI_READ_DDR_DLL_CTRL_SLV_OVERRIDE_VAL 0x0003FC00
#define BF_GPMI_READ_DDR_DLL_CTRL_SLV_OVERRIDE_VAL(v)  \
	(((v) << 10) & BM_GPMI_READ_DDR_DLL_CTRL_SLV_OVERRIDE_VAL)
#define BM_GPMI_READ_DDR_DLL_CTRL_SLV_OVERRIDE 0x00000200
#define BM_GPMI_READ_DDR_DLL_CTRL_REFCLK_ON 0x00000100
#define BM_GPMI_READ_DDR_DLL_CTRL_GATE_UPDATE 0x00000080
#define BP_GPMI_READ_DDR_DLL_CTRL_SLV_DLY_TARGET      3
#define BM_GPMI_READ_DDR_DLL_CTRL_SLV_DLY_TARGET 0x00000078
#define BF_GPMI_READ_DDR_DLL_CTRL_SLV_DLY_TARGET(v)  \
	(((v) << 3) & BM_GPMI_READ_DDR_DLL_CTRL_SLV_DLY_TARGET)
#define BM_GPMI_READ_DDR_DLL_CTRL_SLV_FORCE_UPD 0x00000004
#define BM_GPMI_READ_DDR_DLL_CTRL_RESET 0x00000002
#define BM_GPMI_READ_DDR_DLL_CTRL_ENABLE 0x00000001

#define HW_GPMI_WRITE_DDR_DLL_CTRL	(0x00000110)

#define BP_GPMI_WRITE_DDR_DLL_CTRL_REF_UPDATE_INT      28
#define BM_GPMI_WRITE_DDR_DLL_CTRL_REF_UPDATE_INT 0xF0000000
#define BF_GPMI_WRITE_DDR_DLL_CTRL_REF_UPDATE_INT(v) \
	(((v) << 28) & BM_GPMI_WRITE_DDR_DLL_CTRL_REF_UPDATE_INT)
#define BP_GPMI_WRITE_DDR_DLL_CTRL_SLV_UPDATE_INT      20
#define BM_GPMI_WRITE_DDR_DLL_CTRL_SLV_UPDATE_INT 0x0FF00000
#define BF_GPMI_WRITE_DDR_DLL_CTRL_SLV_UPDATE_INT(v)  \
	(((v) << 20) & BM_GPMI_WRITE_DDR_DLL_CTRL_SLV_UPDATE_INT)
#define BP_GPMI_WRITE_DDR_DLL_CTRL_RSVD1      18
#define BM_GPMI_WRITE_DDR_DLL_CTRL_RSVD1 0x000C0000
#define BF_GPMI_WRITE_DDR_DLL_CTRL_RSVD1(v)  \
	(((v) << 18) & BM_GPMI_WRITE_DDR_DLL_CTRL_RSVD1)
#define BP_GPMI_WRITE_DDR_DLL_CTRL_SLV_OVERRIDE_VAL      10
#define BM_GPMI_WRITE_DDR_DLL_CTRL_SLV_OVERRIDE_VAL 0x0003FC00
#define BF_GPMI_WRITE_DDR_DLL_CTRL_SLV_OVERRIDE_VAL(v)  \
	(((v) << 10) & BM_GPMI_WRITE_DDR_DLL_CTRL_SLV_OVERRIDE_VAL)
#define BM_GPMI_WRITE_DDR_DLL_CTRL_SLV_OVERRIDE 0x00000200
#define BM_GPMI_WRITE_DDR_DLL_CTRL_REFCLK_ON 0x00000100
#define BM_GPMI_WRITE_DDR_DLL_CTRL_GATE_UPDATE 0x00000080
#define BP_GPMI_WRITE_DDR_DLL_CTRL_SLV_DLY_TARGET      3
#define BM_GPMI_WRITE_DDR_DLL_CTRL_SLV_DLY_TARGET 0x00000078
#define BF_GPMI_WRITE_DDR_DLL_CTRL_SLV_DLY_TARGET(v)  \
	(((v) << 3) & BM_GPMI_WRITE_DDR_DLL_CTRL_SLV_DLY_TARGET)
#define BM_GPMI_WRITE_DDR_DLL_CTRL_SLV_FORCE_UPD 0x00000004
#define BM_GPMI_WRITE_DDR_DLL_CTRL_RESET 0x00000002
#define BM_GPMI_WRITE_DDR_DLL_CTRL_ENABLE 0x00000001

#define HW_GPMI_READ_DDR_DLL_STS	(0x00000120)

#define BP_GPMI_READ_DDR_DLL_STS_RSVD1      25
#define BM_GPMI_READ_DDR_DLL_STS_RSVD1 0xFE000000
#define BF_GPMI_READ_DDR_DLL_STS_RSVD1(v) \
	(((v) << 25) & BM_GPMI_READ_DDR_DLL_STS_RSVD1)
#define BP_GPMI_READ_DDR_DLL_STS_REF_SEL      17
#define BM_GPMI_READ_DDR_DLL_STS_REF_SEL 0x01FE0000
#define BF_GPMI_READ_DDR_DLL_STS_REF_SEL(v)  \
	(((v) << 17) & BM_GPMI_READ_DDR_DLL_STS_REF_SEL)
#define BM_GPMI_READ_DDR_DLL_STS_REF_LOCK 0x00010000
#define BP_GPMI_READ_DDR_DLL_STS_RSVD0      9
#define BM_GPMI_READ_DDR_DLL_STS_RSVD0 0x0000FE00
#define BF_GPMI_READ_DDR_DLL_STS_RSVD0(v)  \
	(((v) << 9) & BM_GPMI_READ_DDR_DLL_STS_RSVD0)
#define BP_GPMI_READ_DDR_DLL_STS_SLV_SEL      1
#define BM_GPMI_READ_DDR_DLL_STS_SLV_SEL 0x000001FE
#define BF_GPMI_READ_DDR_DLL_STS_SLV_SEL(v)  \
	(((v) << 1) & BM_GPMI_READ_DDR_DLL_STS_SLV_SEL)
#define BM_GPMI_READ_DDR_DLL_STS_SLV_LOCK 0x00000001

#define HW_GPMI_WRITE_DDR_DLL_STS	(0x00000130)

#define BP_GPMI_WRITE_DDR_DLL_STS_RSVD1      25
#define BM_GPMI_WRITE_DDR_DLL_STS_RSVD1 0xFE000000
#define BF_GPMI_WRITE_DDR_DLL_STS_RSVD1(v) \
	(((v) << 25) & BM_GPMI_WRITE_DDR_DLL_STS_RSVD1)
#define BP_GPMI_WRITE_DDR_DLL_STS_REF_SEL      17
#define BM_GPMI_WRITE_DDR_DLL_STS_REF_SEL 0x01FE0000
#define BF_GPMI_WRITE_DDR_DLL_STS_REF_SEL(v)  \
	(((v) << 17) & BM_GPMI_WRITE_DDR_DLL_STS_REF_SEL)
#define BM_GPMI_WRITE_DDR_DLL_STS_REF_LOCK 0x00010000
#define BP_GPMI_WRITE_DDR_DLL_STS_RSVD0      9
#define BM_GPMI_WRITE_DDR_DLL_STS_RSVD0 0x0000FE00
#define BF_GPMI_WRITE_DDR_DLL_STS_RSVD0(v)  \
	(((v) << 9) & BM_GPMI_WRITE_DDR_DLL_STS_RSVD0)
#define BP_GPMI_WRITE_DDR_DLL_STS_SLV_SEL      1
#define BM_GPMI_WRITE_DDR_DLL_STS_SLV_SEL 0x000001FE
#define BF_GPMI_WRITE_DDR_DLL_STS_SLV_SEL(v)  \
	(((v) << 1) & BM_GPMI_WRITE_DDR_DLL_STS_SLV_SEL)
#define BM_GPMI_WRITE_DDR_DLL_STS_SLV_LOCK 0x00000001
#endif

#define GPMI_NFC_COMMAND_BUFFER_SIZE (10)

/* ECC Macros */
#define GPMI_NFC_METADATA_SIZE	(10)
#define GPMI_NFC_CHUNK_DATA_CHUNK_SIZE	(512)
#define GPMI_NFC_CHUNK_DATA_CHUNK_SIZE_IN_BITS	(512 * 6)
#define GPMI_NFC_CHUNK_ECC_SIZE_IN_BITS(ecc_str)	(ecc_str * 13)
#define GPMI_NFC_ECC_CHUNK_CNT(page_data_size)	\
	(page_data_size / GPMI_NFC_CHUNK_DATA_CHUNK_SIZE)

#define GPMI_NFC_AUX_STATUS_OFF	((GPMI_NFC_METADATA_SIZE + 0x3) & ~0x3)
#define GPMI_NFC_AUX_SIZE(page_size)	((GPMI_NFC_AUX_STATUS_OFF) + \
	((GPMI_NFC_ECC_CHUNK_CNT(page_size) + 0x3) & ~0x3))

static inline int abs(int n)
{
	if (n >= 0)
		return n;
	else
		return n * -1;
}

static inline u32 gpmi_nfc_get_blk_mark_bit_ofs(u32 page_data_size,
						u32 ecc_strength)
{
	u32 chunk_data_size_in_bits;
	u32 chunk_ecc_size_in_bits;
	u32 chunk_total_size_in_bits;
	u32 block_mark_chunk_number;
	u32 block_mark_chunk_bit_offset;
	u32 block_mark_bit_offset;

	/* 4096 bits */
	chunk_data_size_in_bits = GPMI_NFC_CHUNK_DATA_CHUNK_SIZE * 8;
	/* 208 bits */
	chunk_ecc_size_in_bits  = GPMI_NFC_CHUNK_ECC_SIZE_IN_BITS(ecc_strength);

	/* 4304 bits */
	chunk_total_size_in_bits =
			chunk_data_size_in_bits + chunk_ecc_size_in_bits;

	/* Compute the bit offset of the block mark within the physical page. */
	/* 4096 * 8 = 32768 bits */
	block_mark_bit_offset = page_data_size * 8;

	/* Subtract the metadata bits. */
	/* 32688 bits */
	block_mark_bit_offset -= GPMI_NFC_METADATA_SIZE * 8;

	/*
	 * Compute the chunk number (starting at zero) in which the block mark
	 * appears.
	 */
	/* 7 */
	block_mark_chunk_number =
			block_mark_bit_offset / chunk_total_size_in_bits;

	/*
	 * Compute the bit offset of the block mark within its chunk, and
	 * validate it.
	 */
	/* 2560 bits */
	block_mark_chunk_bit_offset =
			block_mark_bit_offset -
			(block_mark_chunk_number * chunk_total_size_in_bits);

	if (block_mark_chunk_bit_offset > chunk_data_size_in_bits)
		return 1;

	/*
	 * Now that we know the chunk number in which the block mark appears,
	 * we can subtract all the ECC bits that appear before it.
	 */
	/* 31232 bits */
	block_mark_bit_offset -=
		block_mark_chunk_number * chunk_ecc_size_in_bits;

	return block_mark_bit_offset;
}

static inline u32 gpmi_nfc_get_ecc_strength(u32 page_data_size,
						u32 page_oob_size)
{
	if (2048 == page_data_size)
		return 8;
	else if (4096 == page_data_size) {
		if (128 == page_oob_size)
			return 8;
		else if (218 == page_oob_size)
			return 16;
		else
			return 0;
	} else if (8192 == page_data_size)
		return 24;
	else
		return 0;
}

/**
 * struct gpmi_nfc_info - i.MX NFC per-device data.
 *
 * Note that the "device" managed by this driver represents the NAND Flash
 * controller *and* the NAND Flash medium behind it. Thus, the per-device data
 * structure has information about the controller, the chips to which it is
 * connected, and properties of the medium as a whole.
 *
 * @dev:                 A pointer to the owning struct device.
 * @pdev:                A pointer to the owning struct platform_device.
 * @pdata:               A pointer to the device's platform data.
 * @resources:           Information about system resources used by this driver.
 * @device_info:         A structure that contains detailed information about
 *                       the NAND Flash device.
 * @physical_geometry:   A description of the medium's physical geometry.
 * @nfc:                 A pointer to a structure that represents the underlying
 *                       NFC hardware.
 * @nfc_geometry:        A description of the medium geometry as viewed by the
 *                       NFC.
 * @rom:                 A pointer to a structure that represents the underlying
 *                       Boot ROM.
 * @rom_geometry:        A description of the medium geometry as viewed by the
 *                       Boot ROM.
 * @mil:                 A collection of information used by the MTD Interface
 *                       Layer.
 */

struct gpmi_nfc_info {

	s32 cur_chip;
	u8  *data_buf;
	u8  *oob_buf;

	u8 m_u8MarkingBadBlock;
	u8 m_u8RawOOBMode;

	u32 gf_len;
	u32 ecc_strength;
	u32 metadata_size;
	u32 ecc_chunk_size;
	u32 ecc_chunk_count;
	u32 auxiliary_size;
	u32 auxiliary_status_offset;
	u32 block_mark_byte_offset;
	u32 block_mark_bit_offset;

	int (*hooked_read_oob)(struct mtd_info *mtd,
				loff_t from, struct mtd_oob_ops *ops);
	int (*hooked_write_oob)(struct mtd_info *mtd,
				loff_t to, struct mtd_oob_ops *ops);
	int (*hooked_block_markbad)(struct mtd_info *mtd,
				loff_t ofs);

	/* NFC HAL */
	struct nfc_hal *nfc;
};


/**
 * struct gpmi_nfc_timing - GPMI NFC timing parameters
 *
 * This structure contains the fundamental timing attributes for the NAND Flash
 * bus and the GPMI NFC hardware.
 *
 * @data_setup_in_ns:         The data setup time, in nanoseconds. Usually the
 *                            maximum of tDS and tWP. A negative value
 *                            indicates this characteristic isn't known.
 * @data_hold_in_ns:          The data hold time, in nanoseconds. Usually the
 *                            maximum of tDH, tWH and tREH. A negative value
 *                            indicates this characteristic isn't known.
 * @address_setup_in_ns:      The address setup time, in nanoseconds. Usually
 *                            the maximum of tCLS, tCS and tALS. A negative
 *                            value indicates this characteristic isn't known.
 * @gpmi_sample_delay_in_ns:  A GPMI-specific timing parameter. A negative value
 *                            indicates this characteristic isn't known.
 * @tREA_in_ns:               tREA, in nanoseconds, from the data sheet. A
 *                            negative value indicates this characteristic isn't
 *                            known.
 * @tRLOH_in_ns:              tRLOH, in nanoseconds, from the data sheet. A
 *                            negative value indicates this characteristic isn't
 *                            known.
 * @tRHOH_in_ns:              tRHOH, in nanoseconds, from the data sheet. A
 *                            negative value indicates this characteristic isn't
 *                            known.
 */

struct gpmi_nfc_timing {
	u8 m_u8DataSetup;
	u8 m_u8DataHold;
	u8 m_u8AddressSetup;
	u8 m_u8HalfPeriods;
	u8 m_u8SampleDelay;
	u8 m_u8NandTimingState;
	u8 m_u8tREA;
	u8 m_u8tRLOH;
	u8 m_u8tRHOH;
};

/**
 * struct nfc_hal - GPMI NFC HAL
 *
 * This structure embodies an abstract interface to the underlying NFC hardware.
 *
 * @version:                     The NFC hardware version.
 * @description:                 A pointer to a human-readable description of
 *                               the NFC hardware.
 * @max_chip_count:              The maximum number of chips the NFC can
 *                               possibly support (this value is a constant for
 *                               each NFC version). This may *not* be the actual
 *                               number of chips connected.
 * @max_data_setup_cycles:       The maximum number of data setup cycles that
 *                               can be expressed in the hardware.
 * @internal_data_setup_in_ns:   The time, in ns, that the NFC hardware requires
 *                               for data read internal setup. In the Reference
 *                               Manual, see the chapter "High-Speed NAND
 *                               Timing" for more details.
 * @max_sample_delay_factor:     The maximum sample delay factor that can be
 *                               expressed in the hardware.
 * @max_dll_clock_period_in_ns:  The maximum period of the GPMI clock that the
 *                               sample delay DLL hardware can possibly work
 *                               with (the DLL is unusable with longer periods).
 *                               If the full-cycle period is greater than HALF
 *                               this value, the DLL must be configured to use
 *                               half-periods.
 * @max_dll_delay_in_ns:         The maximum amount of delay, in ns, that the
 *                               DLL can implement.
 * @dma_descriptors:             A pool of DMA descriptors.
 * @isr_dma_channel:             The DMA channel with which the NFC HAL is
 *                               working. We record this here so the ISR knows
 *                               which DMA channel to acknowledge.
 * @dma_done:                    The completion structure used for DMA
 *                               interrupts.
 * @bch_done:                    The completion structure used for BCH
 *                               interrupts.
 * @timing:                      The current timing configuration.
 * @clock_frequency_in_hz:       The clock frequency, in Hz, during the current
 *                               I/O transaction. If no I/O transaction is in
 *                               progress, this is the clock frequency during
 *                               the most recent I/O transaction.
 * @hardware_timing:             The hardware timing configuration in effect
 *                               during the current I/O transaction. If no I/O
 *                               transaction is in progress, this is the
 *                               hardware timing configuration during the most
 *                               recent I/O transaction.
 * @init:                        Initializes the NFC hardware and data
 *                               structures. This function will be called after
 *                               everything has been set up for communication
 *                               with the NFC itself, but before the platform
 *                               has set up off-chip communication. Thus, this
 *                               function must not attempt to communicate with
 *                               the NAND Flash hardware.
 * @set_geometry:                Configures the NFC hardware and data structures
 *                               to match the physical NAND Flash geometry.
 * @set_geometry:                Configures the NFC hardware and data structures
 *                               to match the physical NAND Flash geometry.
 * @set_timing:                  Configures the NFC hardware and data structures
 *                               to match the given NAND Flash bus timing.
 * @get_timing:                  Returns the the clock frequency, in Hz, and
 *                               the hardware timing configuration during the
 *                               current I/O transaction. If no I/O transaction
 *                               is in progress, this is the timing state during
 *                               the most recent I/O transaction.
 * @exit:                        Shuts down the NFC hardware and data
 *                               structures. This function will be called after
 *                               the platform has shut down off-chip
 *                               communication but while communication with the
 *                               NFC itself still works.
 * @clear_bch:                   Clears a BCH interrupt (intended to be called
 *                               by a more general interrupt handler to do
 *                               device-specific clearing).
 * @is_ready:                    Returns true if the given chip is ready.
 * @begin:                       Begins an interaction with the NFC. This
 *                               function must be called before *any* of the
 *                               following functions so the NFC can prepare
 *                               itself.
 * @end:                         Ends interaction with the NFC. This function
 *                               should be called to give the NFC a chance to,
 *                               among other things, enter a lower-power state.
 * @send_command:                Sends the given buffer of command bytes.
 * @send_data:                   Sends the given buffer of data bytes.
 * @read_data:                   Reads data bytes into the given buffer.
 * @send_page:                   Sends the given given data and OOB bytes,
 *                               using the ECC engine.
 * @read_page:                   Reads a page through the ECC engine and
 *                               delivers the data and OOB bytes to the given
 *                               buffers.
 */

#define  NFC_DMA_DESCRIPTOR_COUNT  (4)

struct nfc_hal {

	/* Hardware attributes. */

	const unsigned int      version;
	const char              *description;
	const unsigned int      max_chip_count;
	const unsigned int      max_data_setup_cycles;
	const unsigned int      internal_data_setup_in_ns;
	const unsigned int      max_sample_delay_factor;
	const unsigned int      max_dll_clock_period_in_ns;
	const unsigned int      max_dll_delay_in_ns;

	/* Working variables. */
	struct gpmi_nfc_timing  timing;
	unsigned long           clock_frequency_in_hz;

	/* Configuration functions. */

	int   (*init)        (void);
	int   (*set_geometry)(struct mtd_info *);
	int   (*set_timing)  (struct mtd_info *,
					const struct gpmi_nfc_timing *);
	void  (*get_timing)  (struct mtd_info *,
					unsigned long *clock_frequency_in_hz,
					struct gpmi_nfc_timing *);
	void  (*exit)        (struct mtd_info *);

	/* Call these functions to begin and end I/O. */

	void  (*begin)       (struct mtd_info *);
	void  (*end)         (struct mtd_info *);

	/* Call these I/O functions only between begin() and end(). */

	void  (*clear_bch)   (struct mtd_info *);
	int   (*is_ready)    (struct mtd_info *, unsigned chip);
	int   (*send_command)(struct mtd_info *, unsigned chip,
				dma_addr_t buffer, unsigned length);
	int   (*send_data)   (struct mtd_info *, unsigned chip,
				dma_addr_t buffer, unsigned length);
	int   (*read_data)   (struct mtd_info *, unsigned chip,
				dma_addr_t buffer, unsigned length);
	int   (*send_page)   (struct mtd_info *, unsigned chip,
				dma_addr_t payload, dma_addr_t auxiliary);
	int   (*read_page)   (struct mtd_info *, unsigned chip,
				dma_addr_t payload, dma_addr_t auxiliary);
};

extern struct nfc_hal  gpmi_nfc_hal;

#endif /* __ARCH_ARM___GPMI_H */
