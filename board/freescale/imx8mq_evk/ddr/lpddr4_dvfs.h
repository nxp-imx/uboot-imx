/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __LPDDR4_DVFS_H__
#define  __LPDDR4_DVFS_H__
#include <asm/arch/ddr_memory_map.h>

#define DFILP_SPT

#define ANAMIX_PLL_BASE_ADDR	0x30360000
#define HW_DRAM_PLL_CFG0_ADDR	(ANAMIX_PLL_BASE_ADDR + 0x60)
#define HW_DRAM_PLL_CFG1_ADDR	(ANAMIX_PLL_BASE_ADDR + 0x64)
#define HW_DRAM_PLL_CFG2_ADDR	(ANAMIX_PLL_BASE_ADDR + 0x68)

#define LPDDR4_HDT_CTL_2D	0xC8  /* stage completion */
#define LPDDR4_HDT_CTL_3200_1D	0xC8  /* stage completion */
#define LPDDR4_HDT_CTL_400_1D	0xC8  /* stage completion */
#define LPDDR4_HDT_CTL_100_1D	0xC8  /* stage completion */

/* 2D share & weight */
#define LPDDR4_2D_WEIGHT	0x1f7f
#define LPDDR4_2D_SHARE		1
#define LPDDR4_CATRAIN_3200_1d	0
#define LPDDR4_CATRAIN_400	0
#define LPDDR4_CATRAIN_100	0
#define LPDDR4_CATRAIN_3200_2d	0

#define WR_POST_EXT_3200  /* recommened to define */

/* lpddr4 phy training config */
/* for LPDDR4 Rtt */
#define LPDDR4_RTT40	6
#define LPDDR4_RTT48	5
#define LPDDR4_RTT60	4
#define LPDDR4_RTT80	3
#define LPDDR4_RTT120	2
#define LPDDR4_RTT240	1
#define LPDDR4_RTT_DIS	0

/* for LPDDR4 Ron */
#define LPDDR4_RON34	7
#define LPDDR4_RON40	6
#define LPDDR4_RON48	5
#define LPDDR4_RON60	4
#define LPDDR4_RON80	3

#define LPDDR4_PHY_ADDR_RON60	0x1
#define LPDDR4_PHY_ADDR_RON40   0x3
#define LPDDR4_PHY_ADDR_RON30   0x7
#define LPDDR4_PHY_ADDR_RON24   0xf
#define LPDDR4_PHY_ADDR_RON20   0x1f

/* for read channel */
#define LPDDR4_RON		LPDDR4_RON40 /* MR3[5:3] */
#define LPDDR4_PHY_RTT		30
#define LPDDR4_PHY_VREF_VALUE 	17

/* for write channel */
#define LPDDR4_PHY_RON		30
#define LPDDR4_PHY_ADDR_RON	LPDDR4_PHY_ADDR_RON40
#define LPDDR4_RTT_DQ		LPDDR4_RTT40 /* MR11[2:0] */
#define LPDDR4_RTT_CA		LPDDR4_RTT40 /* MR11[6:4] */
#define LPDDR4_RTT_CA_BANK0	LPDDR4_RTT40 /* MR11[6:4] */
#define LPDDR4_RTT_CA_BANK1	LPDDR4_RTT40 /* LPDDR4_RTT_DIS//MR11[6:4] */
#define LPDDR4_VREF_VALUE_CA		((1<<6)|(0xd)) /*((0<<6)|(0xe)) MR12 */
#define LPDDR4_VREF_VALUE_DQ_RANK0	((1<<6)|(0xd)) /* MR14 */
#define LPDDR4_VREF_VALUE_DQ_RANK1	((1<<6)|(0xd)) /* MR14 */
#define LPDDR4_MR22_RANK0           	((0<<5)|(1<<4)|(0<<3)|(LPDDR4_RTT40)) /* MR22: OP[5:3]ODTD-CA,CS,CK */
#define LPDDR4_MR22_RANK1		((0<<5)|(1<<4)|(0<<3)|(LPDDR4_RTT40)) /* MR22: OP[5:3]ODTD-CA,CS,CK */
#define LPDDR4_MR3_PU_CAL		1 /* MR3[0] */

#define LPDDR4_2D_WEIGHT 0x1f7f
#define LPDDR4_2D_SHARE 1

#endif  /*__LPDDR4_DVFS_H__ */
