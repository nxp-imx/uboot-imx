/*
 * Copyright (C) 2014 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef MXC_GIS_H
#define MXC_GIS_H

#include <asm/imx-common/regs-common.h>

struct mxs_gis_regs {
	mxs_reg_32(hw_gis_ctrl)				/* 0x00 */
	mxs_reg_32(hw_gis_config0)			/* 0x10 */
	mxs_reg_32(hw_gis_config1)			/* 0x20 */
	mxs_reg_32(hw_gis_fb0)				/* 0x30 */
	mxs_reg_32(hw_gis_fb1)				/* 0x40 */
	mxs_reg_32(hw_gis_pxp_fb0)			/* 0x50 */
	mxs_reg_32(hw_gis_pxp_fb1)			/* 0x60 */

	mxs_reg_32(hw_gis_ch0_ctrl)			/* 0x70 */
	mxs_reg_32(hw_gis_ch0_addr0)		/* 0x80 */
	mxs_reg_32(hw_gis_ch0_data0)		/* 0x90 */
	mxs_reg_32(hw_gis_ch0_addr1)		/* 0xa0 */
	mxs_reg_32(hw_gis_ch0_data1)		/* 0xb0 */
	mxs_reg_32(hw_gis_ch0_addr2)		/* 0xc0 */
	mxs_reg_32(hw_gis_ch0_data2)		/* 0xd0 */
	mxs_reg_32(hw_gis_ch0_addr3)		/* 0xe0 */
	mxs_reg_32(hw_gis_ch0_data3)		/* 0xf0 */

	mxs_reg_32(hw_gis_ch1_ctrl)			/* 0x100 */
	mxs_reg_32(hw_gis_ch1_addr0)		/* 0x110 */
	mxs_reg_32(hw_gis_ch1_data0)		/* 0x120 */
	mxs_reg_32(hw_gis_ch1_addr1)		/* 0x130 */
	mxs_reg_32(hw_gis_ch1_data1)		/* 0x140 */
	mxs_reg_32(hw_gis_ch1_addr2)		/* 0x150 */
	mxs_reg_32(hw_gis_ch1_data2)		/* 0x160 */
	mxs_reg_32(hw_gis_ch1_addr3)		/* 0x170 */
	mxs_reg_32(hw_gis_ch1_data3)		/* 0x180 */

	mxs_reg_32(hw_gis_ch2_ctrl)			/* 0x190 */
	mxs_reg_32(hw_gis_ch2_addr0)		/* 0x1a0 */
	mxs_reg_32(hw_gis_ch2_data0)		/* 0x1b0 */
	mxs_reg_32(hw_gis_ch2_addr1)		/* 0x1c0 */
	mxs_reg_32(hw_gis_ch2_data1)		/* 0x1d0 */
	mxs_reg_32(hw_gis_ch2_addr2)		/* 0x1e0 */
	mxs_reg_32(hw_gis_ch2_data2)		/* 0x1f0 */
	mxs_reg_32(hw_gis_ch2_addr3)		/* 0x200 */
	mxs_reg_32(hw_gis_ch2_data3)		/* 0x210 */

	mxs_reg_32(hw_gis_ch3_ctrl)			/* 0x220 */
	mxs_reg_32(hw_gis_ch3_addr0)		/* 0x230 */
	mxs_reg_32(hw_gis_ch3_data0)		/* 0x240 */
	mxs_reg_32(hw_gis_ch3_addr1)		/* 0x250 */
	mxs_reg_32(hw_gis_ch3_data1)		/* 0x260 */
	mxs_reg_32(hw_gis_ch3_addr2)		/* 0x270 */
	mxs_reg_32(hw_gis_ch3_data2)		/* 0x280 */
	mxs_reg_32(hw_gis_ch3_addr3)		/* 0x290 */
	mxs_reg_32(hw_gis_ch3_data3)		/* 0x2a0 */

	mxs_reg_32(hw_gis_ch4_ctrl)			/* 0x2b0 */
	mxs_reg_32(hw_gis_ch4_addr0)		/* 0x2c0 */
	mxs_reg_32(hw_gis_ch4_data0)		/* 0x2d0 */
	mxs_reg_32(hw_gis_ch4_addr1)		/* 0x2e0 */
	mxs_reg_32(hw_gis_ch4_data1)		/* 0x2f0 */
	mxs_reg_32(hw_gis_ch4_addr2)		/* 0x300 */
	mxs_reg_32(hw_gis_ch4_data2)		/* 0x310 */
	mxs_reg_32(hw_gis_ch4_addr3)		/* 0x320 */
	mxs_reg_32(hw_gis_ch4_data3)		/* 0x330 */

	mxs_reg_32(hw_gis_ch5_ctrl)			/* 0x340 */
	mxs_reg_32(hw_gis_ch5_addr0)		/* 0x350 */
	mxs_reg_32(hw_gis_ch5_data0)		/* 0x360 */
	mxs_reg_32(hw_gis_ch5_addr1)		/* 0x370 */
	mxs_reg_32(hw_gis_ch5_data1)		/* 0x380 */
	mxs_reg_32(hw_gis_ch5_addr2)		/* 0x390 */
	mxs_reg_32(hw_gis_ch5_data2)		/* 0x3a0 */
	mxs_reg_32(hw_gis_ch5_addr3)		/* 0x3b0 */
	mxs_reg_32(hw_gis_ch5_data3)		/* 0x3c0 */

	mxs_reg_32(hw_gis_debug0)			/* 0x3d0 */
	mxs_reg_32(hw_gis_debug1)			/* 0x3e0 */
	mxs_reg_32(hw_gis_version)			/* 0x3f0 */
};

/* register bit */
#define GIS_CTRL_SFTRST_CLR				0
#define GIS_CTRL_SFTRST_SET				(1 << 31)
#define GIS_CTRL_CLK_GATE_CLR			0
#define GIS_CTRL_CLK_GATE_SET			(1 << 30)
#define GIS_CTRL_LCDIF1_IRQ_POL_LOW		0
#define GIS_CTRL_LCDIF1_IRQ_POL_HIGH	(1 << 8)
#define GIS_CTRL_LCDIF0_IRQ_POL_LOW		0
#define GIS_CTRL_LCDIF0_IRQ_POL_HIGH	(1 << 7)
#define GIS_CTRL_PXP_IRQ_POL_LOW		0
#define GIS_CTRL_PXP_IRQ_POL_HIGH		(1 << 6)
#define GIS_CTRL_CSI1_IRQ_POL_LOW		0
#define GIS_CTRL_CSI1_IRQ_POL_HIGH		(1 << 5)
#define GIS_CTRL_CSI0_IRQ_POL_LOW		0
#define GIS_CTRL_CSI0_IRQ_POL_HIGH		(1 << 4)
#define GIS_CTRL_CSI_SEL_CSI0			0
#define GIS_CTRL_CSI_SEL_CSI1			(1 << 3)
#define GIS_CTRL_LCDIF_SEL_LCDIF0		0
#define GIS_CTRL_LCDIF_SEL_LCDIF1		(1 << 2)
#define GIS_CTRL_FB_START_FB0			0
#define GIS_CTRL_FB_START_FB1			(1 << 1)
#define GIS_CTRL_ENABLE_CLR				0
#define GIS_CTRL_ENABLE_SET				(1 << 0)

#define GIS_CONFIG0_CH3_NUM_MASK		(0x7 << 27)
#define GIS_CONFIG0_CH3_NUM_SHIFT		27
#define GIS_CONFIG0_CH3_MAPPING_MASK	(0x7 << 24)
#define GIS_CONFIG0_CH3_MAPPING_SHIFT	24
#define GIS_CONFIG0_CH2_NUM_MASK		(0x7 << 19)
#define GIS_CONFIG0_CH2_NUM_SHIFT		19
#define GIS_CONFIG0_CH2_MAPPING_MASK	(0x7 << 16)
#define GIS_CONFIG0_CH2_MAPPING_SHIFT	16
#define GIS_CONFIG0_CH1_NUM_MASK		(0x7 << 11)
#define GIS_CONFIG0_CH1_NUM_SHIFT		11
#define GIS_CONFIG0_CH1_MAPPING_MASK	(0x7 << 8)
#define GIS_CONFIG0_CH1_MAPPING_SHIFT	8
#define GIS_CONFIG0_CH0_NUM_MASK		(0x7 << 3)
#define GIS_CONFIG0_CH0_NUM_SHIFT		3
#define GIS_CONFIG0_CH0_MAPPING_MASK	(0x7 << 0)
#define GIS_CONFIG0_CH0_MAPPING_SHIFT	0

#define GIS_CONFIG1_CH5_NUM_MASK		(0x7 << 11)
#define GIS_CONFIG1_CH5_NUM_SHIFT		11
#define GIS_CONFIG1_CH5_MAPPING_MASK	(0x7 << 8)
#define GIS_CONFIG1_CH5_MAPPING_SHIFT	8
#define GIS_CONFIG1_CH4_NUM_MASK		(0x7 << 3)
#define GIS_CONFIG1_CH4_NUM_SHIFT		3
#define GIS_CONFIG1_CH4_MAPPING_MASK	(0x7 << 0)
#define GIS_CONFIG1_CH4_MAPPING_SHIFT	0

#define GIS_CH_CTRL_CMD3_ACC_MASK		(0x1 << 31)
#define GIS_CH_CTRL_CMD3_ACC_SHIFT		31
#define GIS_CH_CTRL_CMD3_ALU_MASK		(0x7 << 28)
#define GIS_CH_CTRL_CMD3_ALU_SHIFT		28
#define GIS_CH_CTRL_CMD3_OPCODE_MASK	(0xF << 24)
#define GIS_CH_CTRL_CMD3_OPCODE_SHIFT	24
#define GIS_CH_CTRL_CMD2_ACC_MASK		(0x1 << 23)
#define GIS_CH_CTRL_CMD2_ACC_SHIFT		23
#define GIS_CH_CTRL_CMD2_ALU_MASK		(0xF << 20)
#define GIS_CH_CTRL_CMD2_ALU_SHIFT		20
#define GIS_CH_CTRL_CMD2_OPCODE_MASK	(0xF << 16)
#define GIS_CH_CTRL_CMD2_OPCODE_SHIFT	16
#define GIS_CH_CTRL_CMD1_ACC_MASK		(0x1 << 15)
#define GIS_CH_CTRL_CMD1_ACC_SHIFT		15
#define GIS_CH_CTRL_CMD1_ALU_MASK		(0x7 << 12)
#define GIS_CH_CTRL_CMD1_ALU_SHIFT		12
#define GIS_CH_CTRL_CMD1_OPCODE_MASK	(0xF << 8)
#define GIS_CH_CTRL_CMD1_OPCODE_SHIFT	8
#define GIS_CH_CTRL_CMD0_ACC_MASK		(0x1 << 7)
#define GIS_CH_CTRL_CMD0_ACC_SHIFT		7
#define GIS_CH_CTRL_CMD0_ALU_MASK		(0x7 << 4)
#define GIS_CH_CTRL_CMD0_ALU_SHIFT		4
#define GIS_CH_CTRL_CMD0_OPCODE_MASK	(0xF << 0)
#define GIS_CH_CTRL_CMD0_OPCODE_SHIFT	0

#define GIS_CH_CTRL_CMD_ACC_NO_NEGATE	0
#define GIS_CH_CTRL_CMD_ACC_NEGATE		1

#define GIS_CH_ADDR_SEL_MASK			(0xF8 << 27)
#define GIS_CH_ADDR_SEL_LCDIF1			(0x1 << 31)
#define GIS_CH_ADDR_SEL_LCDIF0			(0x1 << 30)
#define GIS_CH_ADDR_SEL_PXP				(0x1 << 29)
#define GIS_CH_ADDR_SEL_CSI1			(0x1 << 28)
#define GIS_CH_ADDR_SEL_CSI0			(0x1 << 27)
#define GIS_CH_ADDR_SEL_SHIFT			27
#define GIS_CH_ADDR_ADDR_MASK			0x7FFFFFF
#define GIS_CH_ADDR_ADDR_SHIFT			0

#endif

