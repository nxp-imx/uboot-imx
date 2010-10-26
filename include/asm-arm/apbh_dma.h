/*
 * Freescale APBH Register Definitions
 *
 * Copyright 2008-2010 Freescale Semiconductor, Inc. All Rights Reserved.
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
 * Xml Revision: 1.3
 * Template revision: 1.3
 */

#ifndef __APBH_DMA_H__
#define __APBH_DMA_H__

#include <asm/types.h>
#include <linux/list.h>
#include <linux/stddef.h>

#define HW_APBH_CTRL0	(0x00000000)
#define HW_APBH_CTRL0_SET	(0x00000004)
#define HW_APBH_CTRL0_CLR	(0x00000008)
#define HW_APBH_CTRL0_TOG	(0x0000000c)

#define BM_APBH_CTRL0_SFTRST 0x80000000
#define BM_APBH_CTRL0_CLKGATE 0x40000000
#define BM_APBH_CTRL0_AHB_BURST8_EN 0x20000000
#define BM_APBH_CTRL0_APB_BURST_EN 0x10000000
#define BP_APBH_CTRL0_RSVD0      16
#define BM_APBH_CTRL0_RSVD0 0x0FFF0000
#define BF_APBH_CTRL0_RSVD0(v)  \
	(((v) << 16) & BM_APBH_CTRL0_RSVD0)
#define BP_APBH_CTRL0_CLKGATE_CHANNEL      0
#define BM_APBH_CTRL0_CLKGATE_CHANNEL 0x0000FFFF
#define BF_APBH_CTRL0_CLKGATE_CHANNEL(v)  \
	(((v) << 0) & BM_APBH_CTRL0_CLKGATE_CHANNEL)
#if defined(CONFIG_APBH_DMA_V1)
#define BV_APBH_CTRL0_CLKGATE_CHANNEL__SSP0  0x0001
#define BV_APBH_CTRL0_CLKGATE_CHANNEL__SSP1  0x0002
#define BV_APBH_CTRL0_CLKGATE_CHANNEL__SSP2  0x0004
#define BV_APBH_CTRL0_CLKGATE_CHANNEL__SSP3  0x0008
#define BV_APBH_CTRL0_CLKGATE_CHANNEL__NAND0 0x0010
#define BV_APBH_CTRL0_CLKGATE_CHANNEL__NAND1 0x0020
#define BV_APBH_CTRL0_CLKGATE_CHANNEL__NAND2 0x0040
#define BV_APBH_CTRL0_CLKGATE_CHANNEL__NAND3 0x0080
#define BV_APBH_CTRL0_CLKGATE_CHANNEL__NAND4 0x0100
#define BV_APBH_CTRL0_CLKGATE_CHANNEL__NAND5 0x0200
#define BV_APBH_CTRL0_CLKGATE_CHANNEL__NAND6 0x0400
#define BV_APBH_CTRL0_CLKGATE_CHANNEL__NAND7 0x0800
#define BV_APBH_CTRL0_CLKGATE_CHANNEL__HSADC 0x1000
#define BV_APBH_CTRL0_CLKGATE_CHANNEL__LCDIF 0x2000
#elif defined(CONFIG_APBH_DMA_V2)
#define BV_APBH_CTRL0_CLKGATE_CHANNEL__NAND0 0x0001
#define BV_APBH_CTRL0_CLKGATE_CHANNEL__NAND1 0x0002
#define BV_APBH_CTRL0_CLKGATE_CHANNEL__NAND2 0x0004
#define BV_APBH_CTRL0_CLKGATE_CHANNEL__NAND3 0x0008
#define BV_APBH_CTRL0_CLKGATE_CHANNEL__NAND4 0x0010
#define BV_APBH_CTRL0_CLKGATE_CHANNEL__NAND5 0x0020
#define BV_APBH_CTRL0_CLKGATE_CHANNEL__NAND6 0x0040
#define BV_APBH_CTRL0_CLKGATE_CHANNEL__NAND7 0x0080
#define BV_APBH_CTRL0_CLKGATE_CHANNEL__SSP   0x0100
#endif

#define HW_APBH_CTRL1	(0x00000010)
#define HW_APBH_CTRL1_SET	(0x00000014)
#define HW_APBH_CTRL1_CLR	(0x00000018)
#define HW_APBH_CTRL1_TOG	(0x0000001c)

#define BM_APBH_CTRL1_CH15_CMDCMPLT_IRQ_EN 0x80000000
#define BM_APBH_CTRL1_CH14_CMDCMPLT_IRQ_EN 0x40000000
#define BM_APBH_CTRL1_CH13_CMDCMPLT_IRQ_EN 0x20000000
#define BM_APBH_CTRL1_CH12_CMDCMPLT_IRQ_EN 0x10000000
#define BM_APBH_CTRL1_CH11_CMDCMPLT_IRQ_EN 0x08000000
#define BM_APBH_CTRL1_CH10_CMDCMPLT_IRQ_EN 0x04000000
#define BM_APBH_CTRL1_CH9_CMDCMPLT_IRQ_EN 0x02000000
#define BM_APBH_CTRL1_CH8_CMDCMPLT_IRQ_EN 0x01000000
#define BM_APBH_CTRL1_CH7_CMDCMPLT_IRQ_EN 0x00800000
#define BM_APBH_CTRL1_CH6_CMDCMPLT_IRQ_EN 0x00400000
#define BM_APBH_CTRL1_CH5_CMDCMPLT_IRQ_EN 0x00200000
#define BM_APBH_CTRL1_CH4_CMDCMPLT_IRQ_EN 0x00100000
#define BM_APBH_CTRL1_CH3_CMDCMPLT_IRQ_EN 0x00080000
#define BM_APBH_CTRL1_CH2_CMDCMPLT_IRQ_EN 0x00040000
#define BM_APBH_CTRL1_CH1_CMDCMPLT_IRQ_EN 0x00020000
#define BM_APBH_CTRL1_CH0_CMDCMPLT_IRQ_EN 0x00010000
#define BM_APBH_CTRL1_CH15_CMDCMPLT_IRQ 0x00008000
#define BM_APBH_CTRL1_CH14_CMDCMPLT_IRQ 0x00004000
#define BM_APBH_CTRL1_CH13_CMDCMPLT_IRQ 0x00002000
#define BM_APBH_CTRL1_CH12_CMDCMPLT_IRQ 0x00001000
#define BM_APBH_CTRL1_CH11_CMDCMPLT_IRQ 0x00000800
#define BM_APBH_CTRL1_CH10_CMDCMPLT_IRQ 0x00000400
#define BM_APBH_CTRL1_CH9_CMDCMPLT_IRQ 0x00000200
#define BM_APBH_CTRL1_CH8_CMDCMPLT_IRQ 0x00000100
#define BM_APBH_CTRL1_CH7_CMDCMPLT_IRQ 0x00000080
#define BM_APBH_CTRL1_CH6_CMDCMPLT_IRQ 0x00000040
#define BM_APBH_CTRL1_CH5_CMDCMPLT_IRQ 0x00000020
#define BM_APBH_CTRL1_CH4_CMDCMPLT_IRQ 0x00000010
#define BM_APBH_CTRL1_CH3_CMDCMPLT_IRQ 0x00000008
#define BM_APBH_CTRL1_CH2_CMDCMPLT_IRQ 0x00000004
#define BM_APBH_CTRL1_CH1_CMDCMPLT_IRQ 0x00000002
#define BM_APBH_CTRL1_CH0_CMDCMPLT_IRQ 0x00000001

#define HW_APBH_CTRL2	(0x00000020)
#define HW_APBH_CTRL2_SET	(0x00000024)
#define HW_APBH_CTRL2_CLR	(0x00000028)
#define HW_APBH_CTRL2_TOG	(0x0000002c)

#define BM_APBH_CTRL2_CH15_ERROR_STATUS 0x80000000
#define BV_APBH_CTRL2_CH15_ERROR_STATUS__TERMINATION 0x0
#define BV_APBH_CTRL2_CH15_ERROR_STATUS__BUS_ERROR   0x1
#define BM_APBH_CTRL2_CH14_ERROR_STATUS 0x40000000
#define BV_APBH_CTRL2_CH14_ERROR_STATUS__TERMINATION 0x0
#define BV_APBH_CTRL2_CH14_ERROR_STATUS__BUS_ERROR   0x1
#define BM_APBH_CTRL2_CH13_ERROR_STATUS 0x20000000
#define BV_APBH_CTRL2_CH13_ERROR_STATUS__TERMINATION 0x0
#define BV_APBH_CTRL2_CH13_ERROR_STATUS__BUS_ERROR   0x1
#define BM_APBH_CTRL2_CH12_ERROR_STATUS 0x10000000
#define BV_APBH_CTRL2_CH12_ERROR_STATUS__TERMINATION 0x0
#define BV_APBH_CTRL2_CH12_ERROR_STATUS__BUS_ERROR   0x1
#define BM_APBH_CTRL2_CH11_ERROR_STATUS 0x08000000
#define BV_APBH_CTRL2_CH11_ERROR_STATUS__TERMINATION 0x0
#define BV_APBH_CTRL2_CH11_ERROR_STATUS__BUS_ERROR   0x1
#define BM_APBH_CTRL2_CH10_ERROR_STATUS 0x04000000
#define BV_APBH_CTRL2_CH10_ERROR_STATUS__TERMINATION 0x0
#define BV_APBH_CTRL2_CH10_ERROR_STATUS__BUS_ERROR   0x1
#define BM_APBH_CTRL2_CH9_ERROR_STATUS 0x02000000
#define BV_APBH_CTRL2_CH9_ERROR_STATUS__TERMINATION 0x0
#define BV_APBH_CTRL2_CH9_ERROR_STATUS__BUS_ERROR   0x1
#define BM_APBH_CTRL2_CH8_ERROR_STATUS 0x01000000
#define BV_APBH_CTRL2_CH8_ERROR_STATUS__TERMINATION 0x0
#define BV_APBH_CTRL2_CH8_ERROR_STATUS__BUS_ERROR   0x1
#define BM_APBH_CTRL2_CH7_ERROR_STATUS 0x00800000
#define BV_APBH_CTRL2_CH7_ERROR_STATUS__TERMINATION 0x0
#define BV_APBH_CTRL2_CH7_ERROR_STATUS__BUS_ERROR   0x1
#define BM_APBH_CTRL2_CH6_ERROR_STATUS 0x00400000
#define BV_APBH_CTRL2_CH6_ERROR_STATUS__TERMINATION 0x0
#define BV_APBH_CTRL2_CH6_ERROR_STATUS__BUS_ERROR   0x1
#define BM_APBH_CTRL2_CH5_ERROR_STATUS 0x00200000
#define BV_APBH_CTRL2_CH5_ERROR_STATUS__TERMINATION 0x0
#define BV_APBH_CTRL2_CH5_ERROR_STATUS__BUS_ERROR   0x1
#define BM_APBH_CTRL2_CH4_ERROR_STATUS 0x00100000
#define BV_APBH_CTRL2_CH4_ERROR_STATUS__TERMINATION 0x0
#define BV_APBH_CTRL2_CH4_ERROR_STATUS__BUS_ERROR   0x1
#define BM_APBH_CTRL2_CH3_ERROR_STATUS 0x00080000
#define BV_APBH_CTRL2_CH3_ERROR_STATUS__TERMINATION 0x0
#define BV_APBH_CTRL2_CH3_ERROR_STATUS__BUS_ERROR   0x1
#define BM_APBH_CTRL2_CH2_ERROR_STATUS 0x00040000
#define BV_APBH_CTRL2_CH2_ERROR_STATUS__TERMINATION 0x0
#define BV_APBH_CTRL2_CH2_ERROR_STATUS__BUS_ERROR   0x1
#define BM_APBH_CTRL2_CH1_ERROR_STATUS 0x00020000
#define BV_APBH_CTRL2_CH1_ERROR_STATUS__TERMINATION 0x0
#define BV_APBH_CTRL2_CH1_ERROR_STATUS__BUS_ERROR   0x1
#define BM_APBH_CTRL2_CH0_ERROR_STATUS 0x00010000
#define BV_APBH_CTRL2_CH0_ERROR_STATUS__TERMINATION 0x0
#define BV_APBH_CTRL2_CH0_ERROR_STATUS__BUS_ERROR   0x1
#define BM_APBH_CTRL2_CH15_ERROR_IRQ 0x00008000
#define BM_APBH_CTRL2_CH14_ERROR_IRQ 0x00004000
#define BM_APBH_CTRL2_CH13_ERROR_IRQ 0x00002000
#define BM_APBH_CTRL2_CH12_ERROR_IRQ 0x00001000
#define BM_APBH_CTRL2_CH11_ERROR_IRQ 0x00000800
#define BM_APBH_CTRL2_CH10_ERROR_IRQ 0x00000400
#define BM_APBH_CTRL2_CH9_ERROR_IRQ 0x00000200
#define BM_APBH_CTRL2_CH8_ERROR_IRQ 0x00000100
#define BM_APBH_CTRL2_CH7_ERROR_IRQ 0x00000080
#define BM_APBH_CTRL2_CH6_ERROR_IRQ 0x00000040
#define BM_APBH_CTRL2_CH5_ERROR_IRQ 0x00000020
#define BM_APBH_CTRL2_CH4_ERROR_IRQ 0x00000010
#define BM_APBH_CTRL2_CH3_ERROR_IRQ 0x00000008
#define BM_APBH_CTRL2_CH2_ERROR_IRQ 0x00000004
#define BM_APBH_CTRL2_CH1_ERROR_IRQ 0x00000002
#define BM_APBH_CTRL2_CH0_ERROR_IRQ 0x00000001

#define HW_APBH_CHANNEL_CTRL	(0x00000030)
#define HW_APBH_CHANNEL_CTRL_SET	(0x00000034)
#define HW_APBH_CHANNEL_CTRL_CLR	(0x00000038)
#define HW_APBH_CHANNEL_CTRL_TOG	(0x0000003c)

#define BP_APBH_CHANNEL_CTRL_RESET_CHANNEL      16
#define BM_APBH_CHANNEL_CTRL_RESET_CHANNEL 0xFFFF0000
#define BF_APBH_CHANNEL_CTRL_RESET_CHANNEL(v) \
	(((v) << 16) & BM_APBH_CHANNEL_CTRL_RESET_CHANNEL)

#if defined(CONFIG_APBH_DMA_V1)
#define BV_APBH_CHANNEL_CTRL_RESET_CHANNEL__SSP0  0x0001
#define BV_APBH_CHANNEL_CTRL_RESET_CHANNEL__SSP1  0x0002
#define BV_APBH_CHANNEL_CTRL_RESET_CHANNEL__SSP2  0x0004
#define BV_APBH_CHANNEL_CTRL_RESET_CHANNEL__SSP3  0x0008
#define BV_APBH_CHANNEL_CTRL_RESET_CHANNEL__NAND0 0x0010
#define BV_APBH_CHANNEL_CTRL_RESET_CHANNEL__NAND1 0x0020
#define BV_APBH_CHANNEL_CTRL_RESET_CHANNEL__NAND2 0x0040
#define BV_APBH_CHANNEL_CTRL_RESET_CHANNEL__NAND3 0x0080
#define BV_APBH_CHANNEL_CTRL_RESET_CHANNEL__NAND4 0x0100
#define BV_APBH_CHANNEL_CTRL_RESET_CHANNEL__NAND5 0x0200
#define BV_APBH_CHANNEL_CTRL_RESET_CHANNEL__NAND6 0x0400
#define BV_APBH_CHANNEL_CTRL_RESET_CHANNEL__NAND7 0x0800
#define BV_APBH_CHANNEL_CTRL_RESET_CHANNEL__HSADC 0x1000
#define BV_APBH_CHANNEL_CTRL_RESET_CHANNEL__LCDIF 0x2000

#define BP_APBH_CHANNEL_CTRL_FREEZE_CHANNEL      0
#define BM_APBH_CHANNEL_CTRL_FREEZE_CHANNEL 0x0000FFFF
#define BF_APBH_CHANNEL_CTRL_FREEZE_CHANNEL(v)  \
	(((v) << 0) & BM_APBH_CHANNEL_CTRL_FREEZE_CHANNEL)

#define BV_APBH_CHANNEL_CTRL_FREEZE_CHANNEL__SSP0  0x0001
#define BV_APBH_CHANNEL_CTRL_FREEZE_CHANNEL__SSP1  0x0002
#define BV_APBH_CHANNEL_CTRL_FREEZE_CHANNEL__SSP2  0x0004
#define BV_APBH_CHANNEL_CTRL_FREEZE_CHANNEL__SSP3  0x0008
#define BV_APBH_CHANNEL_CTRL_FREEZE_CHANNEL__NAND0 0x0010
#define BV_APBH_CHANNEL_CTRL_FREEZE_CHANNEL__NAND1 0x0020
#define BV_APBH_CHANNEL_CTRL_FREEZE_CHANNEL__NAND2 0x0040
#define BV_APBH_CHANNEL_CTRL_FREEZE_CHANNEL__NAND3 0x0080
#define BV_APBH_CHANNEL_CTRL_FREEZE_CHANNEL__NAND4 0x0100
#define BV_APBH_CHANNEL_CTRL_FREEZE_CHANNEL__NAND5 0x0200
#define BV_APBH_CHANNEL_CTRL_FREEZE_CHANNEL__NAND6 0x0400
#define BV_APBH_CHANNEL_CTRL_FREEZE_CHANNEL__NAND7 0x0800
#define BV_APBH_CHANNEL_CTRL_FREEZE_CHANNEL__HSADC 0x1000
#define BV_APBH_CHANNEL_CTRL_FREEZE_CHANNEL__LCDIF 0x2000
#elif defined(CONFIG_APBH_DMA_V2)
#define BV_APBH_CHANNEL_CTRL_RESET_CHANNEL__NAND0 0x0001
#define BV_APBH_CHANNEL_CTRL_RESET_CHANNEL__NAND1 0x0002
#define BV_APBH_CHANNEL_CTRL_RESET_CHANNEL__NAND2 0x0004
#define BV_APBH_CHANNEL_CTRL_RESET_CHANNEL__NAND3 0x0008
#define BV_APBH_CHANNEL_CTRL_RESET_CHANNEL__NAND4 0x0010
#define BV_APBH_CHANNEL_CTRL_RESET_CHANNEL__NAND5 0x0020
#define BV_APBH_CHANNEL_CTRL_RESET_CHANNEL__NAND6 0x0040
#define BV_APBH_CHANNEL_CTRL_RESET_CHANNEL__NAND7 0x0080
#define BV_APBH_CHANNEL_CTRL_RESET_CHANNEL__SSP   0x0100
#define BP_APBH_CHANNEL_CTRL_FREEZE_CHANNEL      0
#define BM_APBH_CHANNEL_CTRL_FREEZE_CHANNEL 0x0000FFFF
#define BF_APBH_CHANNEL_CTRL_FREEZE_CHANNEL(v)  \
	(((v) << 0) & BM_APBH_CHANNEL_CTRL_FREEZE_CHANNEL)
#define BV_APBH_CHANNEL_CTRL_FREEZE_CHANNEL__NAND0 0x0001
#define BV_APBH_CHANNEL_CTRL_FREEZE_CHANNEL__NAND1 0x0002
#define BV_APBH_CHANNEL_CTRL_FREEZE_CHANNEL__NAND2 0x0004
#define BV_APBH_CHANNEL_CTRL_FREEZE_CHANNEL__NAND3 0x0008
#define BV_APBH_CHANNEL_CTRL_FREEZE_CHANNEL__NAND4 0x0010
#define BV_APBH_CHANNEL_CTRL_FREEZE_CHANNEL__NAND5 0x0020
#define BV_APBH_CHANNEL_CTRL_FREEZE_CHANNEL__NAND6 0x0040
#define BV_APBH_CHANNEL_CTRL_FREEZE_CHANNEL__NAND7 0x0080
#define BV_APBH_CHANNEL_CTRL_FREEZE_CHANNEL__SSP   0x0100
#endif

#define HW_APBH_DEVSEL	(0x00000040)

#define BP_APBH_DEVSEL_CH15      30
#define BM_APBH_DEVSEL_CH15 0xC0000000
#define BF_APBH_DEVSEL_CH15(v) \
	(((v) << 30) & BM_APBH_DEVSEL_CH15)
#define BP_APBH_DEVSEL_CH14      28
#define BM_APBH_DEVSEL_CH14 0x30000000
#define BF_APBH_DEVSEL_CH14(v)  \
	(((v) << 28) & BM_APBH_DEVSEL_CH14)
#define BP_APBH_DEVSEL_CH13      26
#define BM_APBH_DEVSEL_CH13 0x0C000000
#define BF_APBH_DEVSEL_CH13(v)  \
	(((v) << 26) & BM_APBH_DEVSEL_CH13)
#define BP_APBH_DEVSEL_CH12      24
#define BM_APBH_DEVSEL_CH12 0x03000000
#define BF_APBH_DEVSEL_CH12(v)  \
	(((v) << 24) & BM_APBH_DEVSEL_CH12)
#define BP_APBH_DEVSEL_CH11      22
#define BM_APBH_DEVSEL_CH11 0x00C00000
#define BF_APBH_DEVSEL_CH11(v)  \
	(((v) << 22) & BM_APBH_DEVSEL_CH11)
#define BP_APBH_DEVSEL_CH10      20
#define BM_APBH_DEVSEL_CH10 0x00300000
#define BF_APBH_DEVSEL_CH10(v)  \
	(((v) << 20) & BM_APBH_DEVSEL_CH10)
#define BP_APBH_DEVSEL_CH9      18
#define BM_APBH_DEVSEL_CH9 0x000C0000
#define BF_APBH_DEVSEL_CH9(v)  \
	(((v) << 18) & BM_APBH_DEVSEL_CH9)
#define BP_APBH_DEVSEL_CH8      16
#define BM_APBH_DEVSEL_CH8 0x00030000
#define BF_APBH_DEVSEL_CH8(v)  \
	(((v) << 16) & BM_APBH_DEVSEL_CH8)
#define BP_APBH_DEVSEL_CH7      14
#define BM_APBH_DEVSEL_CH7 0x0000C000
#define BF_APBH_DEVSEL_CH7(v)  \
	(((v) << 14) & BM_APBH_DEVSEL_CH7)
#define BP_APBH_DEVSEL_CH6      12
#define BM_APBH_DEVSEL_CH6 0x00003000
#define BF_APBH_DEVSEL_CH6(v)  \
	(((v) << 12) & BM_APBH_DEVSEL_CH6)
#define BP_APBH_DEVSEL_CH5      10
#define BM_APBH_DEVSEL_CH5 0x00000C00
#define BF_APBH_DEVSEL_CH5(v)  \
	(((v) << 10) & BM_APBH_DEVSEL_CH5)
#define BP_APBH_DEVSEL_CH4      8
#define BM_APBH_DEVSEL_CH4 0x00000300
#define BF_APBH_DEVSEL_CH4(v)  \
	(((v) << 8) & BM_APBH_DEVSEL_CH4)
#define BP_APBH_DEVSEL_CH3      6
#define BM_APBH_DEVSEL_CH3 0x000000C0
#define BF_APBH_DEVSEL_CH3(v)  \
	(((v) << 6) & BM_APBH_DEVSEL_CH3)
#define BP_APBH_DEVSEL_CH2      4
#define BM_APBH_DEVSEL_CH2 0x00000030
#define BF_APBH_DEVSEL_CH2(v)  \
	(((v) << 4) & BM_APBH_DEVSEL_CH2)
#define BP_APBH_DEVSEL_CH1      2
#define BM_APBH_DEVSEL_CH1 0x0000000C
#define BF_APBH_DEVSEL_CH1(v)  \
	(((v) << 2) & BM_APBH_DEVSEL_CH1)
#define BP_APBH_DEVSEL_CH0      0
#define BM_APBH_DEVSEL_CH0 0x00000003
#define BF_APBH_DEVSEL_CH0(v)  \
	(((v) << 0) & BM_APBH_DEVSEL_CH0)

#define HW_APBH_DMA_BURST_SIZE	(0x00000050)

#define BP_APBH_DMA_BURST_SIZE_CH15      30
#define BM_APBH_DMA_BURST_SIZE_CH15 0xC0000000
#define BF_APBH_DMA_BURST_SIZE_CH15(v) \
	(((v) << 30) & BM_APBH_DMA_BURST_SIZE_CH15)
#define BP_APBH_DMA_BURST_SIZE_CH14      28
#define BM_APBH_DMA_BURST_SIZE_CH14 0x30000000
#define BF_APBH_DMA_BURST_SIZE_CH14(v)  \
	(((v) << 28) & BM_APBH_DMA_BURST_SIZE_CH14)
#define BP_APBH_DMA_BURST_SIZE_CH13      26
#define BM_APBH_DMA_BURST_SIZE_CH13 0x0C000000
#define BF_APBH_DMA_BURST_SIZE_CH13(v)  \
	(((v) << 26) & BM_APBH_DMA_BURST_SIZE_CH13)
#define BP_APBH_DMA_BURST_SIZE_CH12      24
#define BM_APBH_DMA_BURST_SIZE_CH12 0x03000000
#define BF_APBH_DMA_BURST_SIZE_CH12(v)  \
	(((v) << 24) & BM_APBH_DMA_BURST_SIZE_CH12)
#define BP_APBH_DMA_BURST_SIZE_CH11      22
#define BM_APBH_DMA_BURST_SIZE_CH11 0x00C00000
#define BF_APBH_DMA_BURST_SIZE_CH11(v)  \
	(((v) << 22) & BM_APBH_DMA_BURST_SIZE_CH11)
#define BP_APBH_DMA_BURST_SIZE_CH10      20
#define BM_APBH_DMA_BURST_SIZE_CH10 0x00300000
#define BF_APBH_DMA_BURST_SIZE_CH10(v)  \
	(((v) << 20) & BM_APBH_DMA_BURST_SIZE_CH10)
#define BP_APBH_DMA_BURST_SIZE_CH9      18
#define BM_APBH_DMA_BURST_SIZE_CH9 0x000C0000
#define BF_APBH_DMA_BURST_SIZE_CH9(v)  \
	(((v) << 18) & BM_APBH_DMA_BURST_SIZE_CH9)
#define BP_APBH_DMA_BURST_SIZE_CH8      16
#define BM_APBH_DMA_BURST_SIZE_CH8 0x00030000
#define BF_APBH_DMA_BURST_SIZE_CH8(v)  \
	(((v) << 16) & BM_APBH_DMA_BURST_SIZE_CH8)
#define BV_APBH_DMA_BURST_SIZE_CH8__BURST0 0x0
#define BV_APBH_DMA_BURST_SIZE_CH8__BURST4 0x1
#define BV_APBH_DMA_BURST_SIZE_CH8__BURST8 0x2
#define BP_APBH_DMA_BURST_SIZE_CH7      14
#define BM_APBH_DMA_BURST_SIZE_CH7 0x0000C000
#define BF_APBH_DMA_BURST_SIZE_CH7(v)  \
	(((v) << 14) & BM_APBH_DMA_BURST_SIZE_CH7)
#define BP_APBH_DMA_BURST_SIZE_CH6      12
#define BM_APBH_DMA_BURST_SIZE_CH6 0x00003000
#define BF_APBH_DMA_BURST_SIZE_CH6(v)  \
	(((v) << 12) & BM_APBH_DMA_BURST_SIZE_CH6)
#define BP_APBH_DMA_BURST_SIZE_CH5      10
#define BM_APBH_DMA_BURST_SIZE_CH5 0x00000C00
#define BF_APBH_DMA_BURST_SIZE_CH5(v)  \
	(((v) << 10) & BM_APBH_DMA_BURST_SIZE_CH5)
#define BP_APBH_DMA_BURST_SIZE_CH4      8
#define BM_APBH_DMA_BURST_SIZE_CH4 0x00000300
#define BF_APBH_DMA_BURST_SIZE_CH4(v)  \
	(((v) << 8) & BM_APBH_DMA_BURST_SIZE_CH4)
#define BP_APBH_DMA_BURST_SIZE_CH3      6
#define BM_APBH_DMA_BURST_SIZE_CH3 0x000000C0
#define BF_APBH_DMA_BURST_SIZE_CH3(v)  \
	(((v) << 6) & BM_APBH_DMA_BURST_SIZE_CH3)
#define BV_APBH_DMA_BURST_SIZE_CH3__BURST0 0x0
#define BV_APBH_DMA_BURST_SIZE_CH3__BURST4 0x1
#define BV_APBH_DMA_BURST_SIZE_CH3__BURST8 0x2

#define BP_APBH_DMA_BURST_SIZE_CH2      4
#define BM_APBH_DMA_BURST_SIZE_CH2 0x00000030
#define BF_APBH_DMA_BURST_SIZE_CH2(v)  \
	(((v) << 4) & BM_APBH_DMA_BURST_SIZE_CH2)
#define BV_APBH_DMA_BURST_SIZE_CH2__BURST0 0x0
#define BV_APBH_DMA_BURST_SIZE_CH2__BURST4 0x1
#define BV_APBH_DMA_BURST_SIZE_CH2__BURST8 0x2
#define BP_APBH_DMA_BURST_SIZE_CH1      2
#define BM_APBH_DMA_BURST_SIZE_CH1 0x0000000C
#define BF_APBH_DMA_BURST_SIZE_CH1(v)  \
	(((v) << 2) & BM_APBH_DMA_BURST_SIZE_CH1)
#define BV_APBH_DMA_BURST_SIZE_CH1__BURST0 0x0
#define BV_APBH_DMA_BURST_SIZE_CH1__BURST4 0x1
#define BV_APBH_DMA_BURST_SIZE_CH1__BURST8 0x2

#define BP_APBH_DMA_BURST_SIZE_CH0      0
#define BM_APBH_DMA_BURST_SIZE_CH0 0x00000003
#define BF_APBH_DMA_BURST_SIZE_CH0(v)  \
	(((v) << 0) & BM_APBH_DMA_BURST_SIZE_CH0)
#define BV_APBH_DMA_BURST_SIZE_CH0__BURST0 0x0
#define BV_APBH_DMA_BURST_SIZE_CH0__BURST4 0x1
#define BV_APBH_DMA_BURST_SIZE_CH0__BURST8 0x2

#define HW_APBH_DEBUG	(0x00000060)

#define BP_APBH_DEBUG_RSVD      1
#define BM_APBH_DEBUG_RSVD 0xFFFFFFFE
#define BF_APBH_DEBUG_RSVD(v) \
	(((v) << 1) & BM_APBH_DEBUG_RSVD)
#define BM_APBH_DEBUG_GPMI_ONE_FIFO 0x00000001

/*
 *  multi-register-define name HW_APBH_CHn_CURCMDAR
 *              base 0x00000100
 *              count 16
 *              offset 0x70
 */
#define HW_APBH_CHn_CURCMDAR(n)	(0x00000100 + (n) * 0x70)
#define BP_APBH_CHn_CURCMDAR_CMD_ADDR      0
#define BM_APBH_CHn_CURCMDAR_CMD_ADDR 0xFFFFFFFF
#define BF_APBH_CHn_CURCMDAR_CMD_ADDR(v)   (v)

/*
 *  multi-register-define name HW_APBH_CHn_NXTCMDAR
 *              base 0x00000110
 *              count 16
 *              offset 0x70
 */
#define HW_APBH_CHn_NXTCMDAR(n)	(0x00000110 + (n) * 0x70)
#define BP_APBH_CHn_NXTCMDAR_CMD_ADDR      0
#define BM_APBH_CHn_NXTCMDAR_CMD_ADDR 0xFFFFFFFF
#define BF_APBH_CHn_NXTCMDAR_CMD_ADDR(v)   (v)

/*
 *  multi-register-define name HW_APBH_CHn_CMD
 *              base 0x00000120
 *              count 16
 *              offset 0x70
 */
#define HW_APBH_CHn_CMD(n)	(0x00000120 + (n) * 0x70)
#define BP_APBH_CHn_CMD_XFER_COUNT      16
#define BM_APBH_CHn_CMD_XFER_COUNT 0xFFFF0000
#define BF_APBH_CHn_CMD_XFER_COUNT(v) \
	(((v) << 16) & BM_APBH_CHn_CMD_XFER_COUNT)
#define BP_APBH_CHn_CMD_CMDWORDS      12
#define BM_APBH_CHn_CMD_CMDWORDS 0x0000F000
#define BF_APBH_CHn_CMD_CMDWORDS(v)  \
	(((v) << 12) & BM_APBH_CHn_CMD_CMDWORDS)
#define BP_APBH_CHn_CMD_RSVD1      9
#define BM_APBH_CHn_CMD_RSVD1 0x00000E00
#define BF_APBH_CHn_CMD_RSVD1(v)  \
	(((v) << 9) & BM_APBH_CHn_CMD_RSVD1)
#define BM_APBH_CHn_CMD_HALTONTERMINATE 0x00000100
#define BM_APBH_CHn_CMD_WAIT4ENDCMD 0x00000080
#define BM_APBH_CHn_CMD_SEMAPHORE 0x00000040
#define BM_APBH_CHn_CMD_NANDWAIT4READY 0x00000020
#define BM_APBH_CHn_CMD_NANDLOCK 0x00000010
#define BM_APBH_CHn_CMD_IRQONCMPLT 0x00000008
#define BM_APBH_CHn_CMD_CHAIN 0x00000004
#define BP_APBH_CHn_CMD_COMMAND      0
#define BM_APBH_CHn_CMD_COMMAND 0x00000003
#define BF_APBH_CHn_CMD_COMMAND(v)  \
	(((v) << 0) & BM_APBH_CHn_CMD_COMMAND)
#define BV_APBH_CHn_CMD_COMMAND__NO_DMA_XFER 0x0
#define BV_APBH_CHn_CMD_COMMAND__DMA_WRITE   0x1
#define BV_APBH_CHn_CMD_COMMAND__DMA_READ    0x2
#define BV_APBH_CHn_CMD_COMMAND__DMA_SENSE   0x3

/*
 *  multi-register-define name HW_APBH_CHn_BAR
 *              base 0x00000130
 *              count 16
 *              offset 0x70
 */
#define HW_APBH_CHn_BAR(n)	(0x00000130 + (n) * 0x70)
#define BP_APBH_CHn_BAR_ADDRESS      0
#define BM_APBH_CHn_BAR_ADDRESS 0xFFFFFFFF
#define BF_APBH_CHn_BAR_ADDRESS(v)   (v)

/*
 *  multi-register-define name HW_APBH_CHn_SEMA
 *              base 0x00000140
 *              count 16
 *              offset 0x70
 */
#define HW_APBH_CHn_SEMA(n)	(0x00000140 + (n) * 0x70)
#define BP_APBH_CHn_SEMA_RSVD2      24
#define BM_APBH_CHn_SEMA_RSVD2 0xFF000000
#define BF_APBH_CHn_SEMA_RSVD2(v) \
	(((v) << 24) & BM_APBH_CHn_SEMA_RSVD2)
#define BP_APBH_CHn_SEMA_PHORE      16
#define BM_APBH_CHn_SEMA_PHORE 0x00FF0000
#define BF_APBH_CHn_SEMA_PHORE(v)  \
	(((v) << 16) & BM_APBH_CHn_SEMA_PHORE)
#define BP_APBH_CHn_SEMA_RSVD1      8
#define BM_APBH_CHn_SEMA_RSVD1 0x0000FF00
#define BF_APBH_CHn_SEMA_RSVD1(v)  \
	(((v) << 8) & BM_APBH_CHn_SEMA_RSVD1)
#define BP_APBH_CHn_SEMA_INCREMENT_SEMA      0
#define BM_APBH_CHn_SEMA_INCREMENT_SEMA 0x000000FF
#define BF_APBH_CHn_SEMA_INCREMENT_SEMA(v)  \
	(((v) << 0) & BM_APBH_CHn_SEMA_INCREMENT_SEMA)

/*
 *  multi-register-define name HW_APBH_CHn_DEBUG1
 *              base 0x00000150
 *              count 16
 *              offset 0x70
 */
#define HW_APBH_CHn_DEBUG1(n)	(0x00000150 + (n) * 0x70)
#define BM_APBH_CHn_DEBUG1_REQ 0x80000000
#define BM_APBH_CHn_DEBUG1_BURST 0x40000000
#define BM_APBH_CHn_DEBUG1_KICK 0x20000000
#define BM_APBH_CHn_DEBUG1_END 0x10000000
#define BM_APBH_CHn_DEBUG1_SENSE 0x08000000
#define BM_APBH_CHn_DEBUG1_READY 0x04000000
#define BM_APBH_CHn_DEBUG1_LOCK 0x02000000
#define BM_APBH_CHn_DEBUG1_NEXTCMDADDRVALID 0x01000000
#define BM_APBH_CHn_DEBUG1_RD_FIFO_EMPTY 0x00800000
#define BM_APBH_CHn_DEBUG1_RD_FIFO_FULL 0x00400000
#define BM_APBH_CHn_DEBUG1_WR_FIFO_EMPTY 0x00200000
#define BM_APBH_CHn_DEBUG1_WR_FIFO_FULL 0x00100000
#define BP_APBH_CHn_DEBUG1_RSVD1      5
#define BM_APBH_CHn_DEBUG1_RSVD1 0x000FFFE0
#define BF_APBH_CHn_DEBUG1_RSVD1(v)  \
	(((v) << 5) & BM_APBH_CHn_DEBUG1_RSVD1)
#define BP_APBH_CHn_DEBUG1_STATEMACHINE      0
#define BM_APBH_CHn_DEBUG1_STATEMACHINE 0x0000001F
#define BF_APBH_CHn_DEBUG1_STATEMACHINE(v)  \
	(((v) << 0) & BM_APBH_CHn_DEBUG1_STATEMACHINE)
#define BV_APBH_CHn_DEBUG1_STATEMACHINE__IDLE            0x00
#define BV_APBH_CHn_DEBUG1_STATEMACHINE__REQ_CMD1        0x01
#define BV_APBH_CHn_DEBUG1_STATEMACHINE__REQ_CMD3        0x02
#define BV_APBH_CHn_DEBUG1_STATEMACHINE__REQ_CMD2        0x03
#define BV_APBH_CHn_DEBUG1_STATEMACHINE__XFER_DECODE     0x04
#define BV_APBH_CHn_DEBUG1_STATEMACHINE__REQ_WAIT        0x05
#define BV_APBH_CHn_DEBUG1_STATEMACHINE__REQ_CMD4        0x06
#define BV_APBH_CHn_DEBUG1_STATEMACHINE__PIO_REQ         0x07
#define BV_APBH_CHn_DEBUG1_STATEMACHINE__READ_FLUSH      0x08
#define BV_APBH_CHn_DEBUG1_STATEMACHINE__READ_WAIT       0x09
#define BV_APBH_CHn_DEBUG1_STATEMACHINE__WRITE           0x0C
#define BV_APBH_CHn_DEBUG1_STATEMACHINE__READ_REQ        0x0D
#define BV_APBH_CHn_DEBUG1_STATEMACHINE__CHECK_CHAIN     0x0E
#define BV_APBH_CHn_DEBUG1_STATEMACHINE__XFER_COMPLETE   0x0F
#define BV_APBH_CHn_DEBUG1_STATEMACHINE__TERMINATE       0x14
#define BV_APBH_CHn_DEBUG1_STATEMACHINE__WAIT_END        0x15
#define BV_APBH_CHn_DEBUG1_STATEMACHINE__WRITE_WAIT      0x1C
#define BV_APBH_CHn_DEBUG1_STATEMACHINE__HALT_AFTER_TERM 0x1D
#define BV_APBH_CHn_DEBUG1_STATEMACHINE__CHECK_WAIT      0x1E
#define BV_APBH_CHn_DEBUG1_STATEMACHINE__WAIT_READY      0x1F

/*
 *  multi-register-define name HW_APBH_CHn_DEBUG2
 *              base 0x00000160
 *              count 16
 *              offset 0x70
 */
#define HW_APBH_CHn_DEBUG2(n)	(0x00000160 + (n) * 0x70)
#define BP_APBH_CHn_DEBUG2_APB_BYTES      16
#define BM_APBH_CHn_DEBUG2_APB_BYTES 0xFFFF0000
#define BF_APBH_CHn_DEBUG2_APB_BYTES(v) \
	(((v) << 16) & BM_APBH_CHn_DEBUG2_APB_BYTES)
#define BP_APBH_CHn_DEBUG2_AHB_BYTES      0
#define BM_APBH_CHn_DEBUG2_AHB_BYTES 0x0000FFFF
#define BF_APBH_CHn_DEBUG2_AHB_BYTES(v)  \
	(((v) << 0) & BM_APBH_CHn_DEBUG2_AHB_BYTES)

#define HW_APBH_VERSION	(0x00000800)

#define BP_APBH_VERSION_MAJOR      24
#define BM_APBH_VERSION_MAJOR 0xFF000000
#define BF_APBH_VERSION_MAJOR(v) \
	(((v) << 24) & BM_APBH_VERSION_MAJOR)
#define BP_APBH_VERSION_MINOR      16
#define BM_APBH_VERSION_MINOR 0x00FF0000
#define BF_APBH_VERSION_MINOR(v)  \
	(((v) << 16) & BM_APBH_VERSION_MINOR)
#define BP_APBH_VERSION_STEP      0
#define BM_APBH_VERSION_STEP 0x0000FFFF
#define BF_APBH_VERSION_STEP(v)  \
	(((v) << 0) & BM_APBH_VERSION_STEP)

enum {
	MXS_DMA_CHANNEL_AHB_APBH = 0,
#if defined(CONFIG_APBH_DMA_V1)
	MXS_DMA_CHANNEL_AHB_APBH_SSP0 = MXS_DMA_CHANNEL_AHB_APBH,
	MXS_DMA_CHANNEL_AHB_APBH_SSP1,
	MXS_DMA_CHANNEL_AHB_APBH_SSP2,
	MXS_DMA_CHANNEL_AHB_APBH_SSP3,
	MXS_DMA_CHANNEL_AHB_APBH_GPMI0,
#elif defined(CONFIG_APBH_DMA_V2)
	MXS_DMA_CHANNEL_AHB_APBH_GPMI0 = MXS_DMA_CHANNEL_AHB_APBH,
#else
#	error "Undefined apbh dma version!"
#endif
	MXS_DMA_CHANNEL_AHB_APBH_GPMI1,
	MXS_DMA_CHANNEL_AHB_APBH_GPMI2,
	MXS_DMA_CHANNEL_AHB_APBH_GPMI3,
	MXS_DMA_CHANNEL_AHB_APBH_GPMI4,
	MXS_DMA_CHANNEL_AHB_APBH_GPMI5,
	MXS_DMA_CHANNEL_AHB_APBH_GPMI6,
	MXS_DMA_CHANNEL_AHB_APBH_GPMI7,
	MXS_DMA_CHANNEL_AHB_APBH_SSP,
	MXS_MAX_DMA_CHANNELS,
};

#ifndef CONFIG_ARCH_DMA_PIO_WORDS
#define DMA_PIO_WORDS	15
#else
#define DMA_PIO_WORDS	CONFIG_ARCH_DMA_PIO_WORDS
#endif

#define MXS_DMA_ALIGNMENT	8

/**
 * struct mxs_dma_cmd_bits - MXS DMA hardware command bits.
 *
 * This structure describes the in-memory layout of the command bits in a DMA
 * command. See the appropriate reference manual for a detailed description
 * of what these bits mean to the DMA hardware.
 */
struct mxs_dma_cmd_bits {
	unsigned int command:2;
#define NO_DMA_XFER	0x00
#define DMA_WRITE	0x01
#define DMA_READ	0x02
#define DMA_SENSE	0x03

	unsigned int chain:1;
	unsigned int irq:1;
	unsigned int nand_lock:1;
	unsigned int nand_wait_4_ready:1;
	unsigned int dec_sem:1;
	unsigned int wait4end:1;
	unsigned int halt_on_terminate:1;
	unsigned int terminate_flush:1;
	unsigned int resv2:2;
	unsigned int pio_words:4;
	unsigned int bytes:16;
};

/**
 * struct mxs_dma_cmd - MXS DMA hardware command.
 *
 * This structure describes the in-memory layout of an entire DMA command,
 * including space for the maximum number of PIO accesses. See the appropriate
 * reference manual for a detailed description of what these fields mean to the
 * DMA hardware.
 */
struct mxs_dma_cmd {
	unsigned long next;
	union {
		unsigned long data;
		struct mxs_dma_cmd_bits bits;
	} cmd;
	union {
		dma_addr_t address;
		unsigned long alternate;
	};
	unsigned long pio_words[DMA_PIO_WORDS];
};

/**
 * struct mxs_dma_desc - MXS DMA command descriptor.
 *
 * This structure incorporates an MXS DMA hardware command structure, along
 * with metadata.
 *
 * @cmd:      The MXS DMA hardware command block.
 * @flags:    Flags that represent the state of this DMA descriptor.
 * @address:  The physical address of this descriptor.
 * @buffer:   A convenient place for software to put the virtual address of the
 *            associated data buffer (the physical address of the buffer
 *            appears in the DMA command). The MXS platform DMA software doesn't
 *            use this field -- it is provided as a convenience.
 * @node:     Links this structure into a list.
 */
struct mxs_dma_desc {
	struct mxs_dma_cmd cmd;
	unsigned int flags;
#define MXS_DMA_DESC_READY 0x80000000
#define MXS_DMA_DESC_FIRST 0x00000001
#define MXS_DMA_DESC_LAST  0x00000002
	dma_addr_t address;
	void *buffer;
	struct list_head node;
};

struct mxs_dma_info {
	unsigned int status;
#define MXS_DMA_INFO_ERR       0x00000001
#define MXS_DMA_INFO_ERR_STAT  0x00010000
	unsigned int buf_addr;
};

/**
 * struct mxs_dma_chan - MXS DMA channel
 *
 * This structure represents a single DMA channel. The MXS platform code
 * maintains an array of these structures to represent every DMA channel in the
 * system (see mxs_dma_channels).
 *
 * @name:         A human-readable string that describes how this channel is
 *                being used or what software "owns" it. This field is set when
 *                when the channel is reserved by mxs_dma_request().
 * @dev:          A pointer to a struct device *, cast to an unsigned long, and
 *                representing the software that "owns" the channel. This field
 *                is set when when the channel is reserved by mxs_dma_request().
 * @lock:         Arbitrates access to this channel.
 * @dma:          A pointer to a struct mxs_dma_device representing the driver
 *                code that operates this channel.
 * @flags:        Flag bits that represent the state of this channel.
 * @active_num:   If the channel is not busy, this value is zero. If the channel
 *                is busy, this field contains the number of DMA command
 *                descriptors at the head of the active list that the hardware
 *                has been told to process. This value is set at the moment the
 *                channel is enabled by mxs_dma_enable(). More descriptors may
 *                arrive after the channel is enabled, so the number of
 *                descriptors on the active list may be greater than this value.
 *                In fact, it should always be active_num + pending_num.
 * @pending_num:  The number of DMA command descriptors at the tail of the
 *                active list that the hardware has not been told to process.
 * @active:       The list of DMA command descriptors either currently being
 *                processed by the hardware or waiting to be processed.
 *                Descriptors being processed appear at the head of the list,
 *                while pending descriptors appear at the tail. The total number
 *                should always be active_num + pending_num.
 * @done:         The list of DMA command descriptors that have either been
 *                processed by the DMA hardware or aborted by a call to
 *                mxs_dma_disable().
 */
struct mxs_dma_chan {
	const char *name;
	unsigned long dev;
	struct mxs_dma_device *dma;
	unsigned int flags;
#define MXS_DMA_FLAGS_IDLE	0x00000000
#define MXS_DMA_FLAGS_BUSY	0x00000001
#define MXS_DMA_FLAGS_FREE	0x00000000
#define MXS_DMA_FLAGS_ALLOCATED	0x00010000
#define MXS_DMA_FLAGS_VALID	0x80000000
	unsigned int active_num;
	unsigned int pending_num;
	struct list_head active;
	struct list_head done;
};

/**
 * struct mxs_dma_device - DMA channel driver interface.
 *
 * This structure represents the driver that operates a DMA channel. Every
 * struct mxs_dma_chan contains a pointer to a structure of this type, which is
 * installed when the driver registers to "own" the channel (see
 * mxs_dma_device_register()).
 */
struct mxs_dma_device {
	struct list_head node;
	const char *name;
	void *base;
	unsigned int chan_base;
	unsigned int chan_num;
	unsigned int data;
};

/**
 * mxs_dma_device_register - Register a DMA driver.
 *
 * This function registers a driver for a contiguous group of DMA channels (the
 * ordering of DMA channels is specified by the globally unique DMA channel
 * numbers given in mach/dma.h).
 *
 * @pdev:  A pointer to a structure that represents the driver. This structure
 *         contains fields that specify the first DMA channel number and the
 *         number of channels.
 */
extern int mxs_dma_device_register(struct mxs_dma_device *pdev);

/**
 * mxs_dma_request - Request to reserve a DMA channel.
 *
 * @channel:  The channel number. This is one of the globally unique DMA channel
 *            numbers given in mach/dma.h.
 * @dev:      A pointer to a struct device representing the channel "owner."
 * @name:     A human-readable string that identifies the channel owner or the
 *            purpose of the channel.
 */
extern int mxs_dma_request(int channel);

/**
 * mxs_dma_release - Release a DMA channel.
 *
 * This function releases a DMA channel from its current owner.
 *
 * The channel will NOT be released if it's marked "busy" (see
 * mxs_dma_enable()).
 *
 * @channel:  The channel number. This is one of the globally unique DMA channel
 *            numbers given in mach/dma.h.
 * @dev:      A pointer to a struct device representing the channel "owner." If
 *            this doesn't match the owner given to mxs_dma_request(), the
 *            channel will NOT be released.
 */
extern void mxs_dma_release(int channel);

/**
 * mxs_dma_enable - Enable a DMA channel.
 *
 * If the given channel has any DMA descriptors on its active list, this
 * function causes the DMA hardware to begin processing them.
 *
 * This function marks the DMA channel as "busy," whether or not there are any
 * descriptors to process.
 *
 * @channel:  The channel number. This is one of the globally unique DMA channel
 *            numbers given in mach/dma.h.
 */
extern int mxs_dma_enable(int channel);

/**
 * mxs_dma_disable - Disable a DMA channel.
 *
 * This function shuts down a DMA channel and marks it as "not busy." Any
 * descriptors on the active list are immediately moved to the head of the
 * "done" list, whether or not they have actually been processed by the
 * hardware. The "ready" flags of these descriptors are NOT cleared, so they
 * still appear to be active.
 *
 * This function immediately shuts down a DMA channel's hardware, aborting any
 * I/O that may be in progress, potentially leaving I/O hardware in an undefined
 * state. It is unwise to call this function if there is ANY chance the hardware
 * is still processing a command.
 *
 * @channel:  The channel number. This is one of the globally unique DMA channel
 *            numbers given in mach/dma.h.
 */
extern void mxs_dma_disable(int channel);

/**
 * mxs_dma_reset - Resets the DMA channel hardware.
 *
 * @channel:  The channel number. This is one of the globally unique DMA channel
 *            numbers given in mach/dma.h.
 */
extern void mxs_dma_reset(int channel);

/**
 * mxs_dma_freeze - Freeze a DMA channel.
 *
 * This function causes the channel to continuously fail arbitration for bus
 * access, which halts all forward progress without losing any state. A call to
 * mxs_dma_unfreeze() will cause the channel to continue its current operation
 * with no ill effect.
 *
 * @channel:  The channel number. This is one of the globally unique DMA channel
 *            numbers given in mach/dma.h.
 */
extern void mxs_dma_freeze(int channel);

/**
 * mxs_dma_unfreeze - Unfreeze a DMA channel.
 *
 * This function reverses the effect of mxs_dma_freeze(), enabling the DMA
 * channel to continue from where it was frozen.
 *
 * @channel:  The channel number. This is one of the globally unique DMA channel
 *            numbers given in mach/dma.h.
 */

extern void mxs_dma_unfreeze(int channel);

/* get dma channel information */
extern int mxs_dma_get_info(int channel, struct mxs_dma_info *info);

/**
 * mxs_dma_cooked - Clean up processed DMA descriptors.
 *
 * This function removes processed DMA descriptors from the "active" list. Pass
 * in a non-NULL list head to get the descriptors moved to your list. Pass NULL
 * to get the descriptors moved to the channel's "done" list. Descriptors on
 * the "done" list can be retrieved with mxs_dma_get_cooked().
 *
 * This function marks the DMA channel as "not busy" if no unprocessed
 * descriptors remain on the "active" list.
 *
 * @channel:  The channel number. This is one of the globally unique DMA channel
 *            numbers given in mach/dma.h.
 * @head:     If this is not NULL, it is the list to which the processed
 *            descriptors should be moved. If this list is NULL, the descriptors
 *            will be moved to the "done" list.
 */
extern int mxs_dma_cooked(int channel, struct list_head *head);

/**
 * mxs_dma_read_semaphore - Read a DMA channel's hardware semaphore.
 *
 * As used by the MXS platform's DMA software, the DMA channel's hardware
 * semaphore reflects the number of DMA commands the hardware will process, but
 * has not yet finished. This is a volatile value read directly from hardware,
 * so it must be be viewed as immediately stale.
 *
 * If the channel is not marked busy, or has finished processing all its
 * commands, this value should be zero.
 *
 * See mxs_dma_append() for details on how DMA command blocks must be configured
 * to maintain the expected behavior of the semaphore's value.
 *
 * @channel:  The channel number. This is one of the globally unique DMA channel
 *            numbers given in mach/dma.h.
 */
extern int mxs_dma_read_semaphore(int channel);

/**
 * mxs_dma_irq_is_pending - Check if a DMA interrupt is pending.
 *
 * @channel:  The channel number. This is one of the globally unique DMA channel
 *            numbers given in mach/dma.h.
 */
extern int mxs_dma_irq_is_pending(int channel);

/**
 * mxs_dma_enable_irq - Enable or disable DMA interrupt.
 *
 * This function enables the given DMA channel to interrupt the CPU.
 *
 * @channel:  The channel number. This is one of the globally unique DMA channel
 *            numbers given in mach/dma.h.
 * @en:       True if the interrupt for this channel should be enabled. False
 *            otherwise.
 */
extern void mxs_dma_enable_irq(int channel, int en);

/**
 * mxs_dma_ack_irq - Clear DMA interrupt.
 *
 * The software that is using the DMA channel must register to receive its
 * interrupts and, when they arrive, must call this function to clear them.
 *
 * @channel:  The channel number. This is one of the globally unique DMA channel
 *            numbers given in mach/dma.h.
 */
extern void mxs_dma_ack_irq(int channel);

/**
 * mxs_dma_set_target - Set the target for a DMA channel.
 *
 * @channel:  The channel number. This is one of the globally unique DMA channel
 *            numbers given in mach/dma.h.
 * @target:   Indicates the target for the channel.
 */
extern void mxs_dma_set_target(int channel, int target);

/* mxs dma utility functions */
extern struct mxs_dma_desc *mxs_dma_alloc_desc(void);
extern void mxs_dma_free_desc(struct mxs_dma_desc *);

/**
 * mxs_dma_cmd_address - Return the address of the command within a descriptor.
 *
 * @desc:  The DMA descriptor of interest.
 */
static inline unsigned int mxs_dma_cmd_address(struct mxs_dma_desc *desc)
{
	return desc->address += offsetof(struct mxs_dma_desc, cmd);
}

/**
 * mxs_dma_desc_pending - Check if descriptor is on a channel's active list.
 *
 * This function returns the state of a descriptor's "ready" flag. This flag is
 * usually set only if the descriptor appears on a channel's active list. The
 * descriptor may or may not have already been processed by the hardware.
 *
 * The "ready" flag is set when the descriptor is submitted to a channel by a
 * call to mxs_dma_append() or mxs_dma_append_list(). The "ready" flag is
 * cleared when a processed descriptor is moved off the active list by a call
 * to mxs_dma_cooked(). The "ready" flag is NOT cleared if the descriptor is
 * aborted by a call to mxs_dma_disable().
 *
 * @desc:  The DMA descriptor of interest.
 */
static inline int mxs_dma_desc_pending(struct mxs_dma_desc *pdesc)
{
	return pdesc->flags & MXS_DMA_DESC_READY;
}

/**
 * mxs_dma_desc_append - Add a DMA descriptor to a channel.
 *
 * If the descriptor list for this channel is not empty, this function sets the
 * CHAIN bit and the NEXTCMD_ADDR fields in the last descriptor's DMA command so
 * it will chain to the new descriptor's command.
 *
 * Then, this function marks the new descriptor as "ready," adds it to the end
 * of the active descriptor list, and increments the count of pending
 * descriptors.
 *
 * The MXS platform DMA software imposes some rules on DMA commands to maintain
 * important invariants. These rules are NOT checked, but they must be carefully
 * applied by software that uses MXS DMA channels.
 *
 * Invariant:
 *     The DMA channel's hardware semaphore must reflect the number of DMA
 *     commands the hardware will process, but has not yet finished.
 *
 * Explanation:
 *     A DMA channel begins processing commands when its hardware semaphore is
 *     written with a value greater than zero, and it stops processing commands
 *     when the semaphore returns to zero.
 *
 *     When a channel finishes a DMA command, it will decrement its semaphore if
 *     the DECREMENT_SEMAPHORE bit is set in that command's flags bits.
 *
 *     In principle, it's not necessary for the DECREMENT_SEMAPHORE to be set,
 *     unless it suits the purposes of the software. For example, one could
 *     construct a series of five DMA commands, with the DECREMENT_SEMAPHORE
 *     bit set only in the last one. Then, setting the DMA channel's hardware
 *     semaphore to one would cause the entire series of five commands to be
 *     processed. However, this example would violate the invariant given above.
 *
 * Rule:
 *    ALL DMA commands MUST have the DECREMENT_SEMAPHORE bit set so that the DMA
 *    channel's hardware semaphore will be decremented EVERY time a command is
 *    processed.
 *
 * @channel:  The channel number. This is one of the globally unique DMA channel
 *            numbers given in mach/dma.h.
 * @pdesc:    A pointer to the new descriptor.
 */
extern int mxs_dma_desc_append(int channel, struct mxs_dma_desc *pdesc);

/**
 * mxs_dma_desc_add_list - Add a list of DMA descriptors to a channel.
 *
 * This function marks all the new descriptors as "ready," adds them to the end
 * of the active descriptor list, and adds the length of the list to the count
 * of pending descriptors.
 *
 * See mxs_dma_desc_append() for important rules that apply to incoming DMA
 * descriptors.
 *
 * @channel:  The channel number. This is one of the globally unique DMA channel
 *            numbers given in mach/dma.h.
 * @head:     A pointer to the head of the list of DMA descriptors to add.
 */
extern int mxs_dma_desc_add_list(int channel, struct list_head *head);

/**
 * mxs_dma_desc_get_cooked - Retrieve processed DMA descriptors.
 *
 * This function moves all the descriptors from the DMA channel's "done" list to
 * the head of the given list.
 *
 * @channel:  The channel number. This is one of the globally unique DMA channel
 *            numbers given in mach/dma.h.
 * @head:     A pointer to the head of the list that will receive the
 *            descriptors on the "done" list.
 */
extern int mxs_dma_get_cooked(int channel, struct list_head *head);

extern int mxs_dma_init(void);
extern int mxs_dma_wait_complete(u32 uSecTimeout, unsigned int chan);
extern int mxs_dma_go(int chan);

#endif /* __ARCH_ARM___APBH_H */
