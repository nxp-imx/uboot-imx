/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef LPDDR4_DEFINE_H
#define LPDDR4_DEFINE_H

#include "ddr.h"

#define RUN_ON_SILICON
#define DFI_BUG_WR
#define DEVINIT_PHY

#define DDR_ONE_RANK
#define BUG_WR_DFI
#define M845S_4GBx2

#ifdef LPDDR4_667MTS
#define P0_667
#endif
#ifdef LPDDR4_1600MTS
#define P0_1600
#endif
#ifdef LPDDR4_DVFS
#define DVFS_TEST
#define PHY_TRAIN
#define DDR_BOOT_P1
#endif
#ifdef LPDDR4_RETENTION
#define NORMAL_RET_EN
#endif

#ifdef P0_667
#define P0_DRATE 667
#else
#ifdef P0_1600
#define P0_DRATE 1600
#else
#define P0_DRATE 3000
#endif
#endif

#define P1_DRATE 667
#define P2_DRATE 100

#ifdef RUN_ON_SILICON
#define PHY_TRAIN
#define ADD_P0_2D_BF_P1
#ifdef HWFFC
#define ADD_TRAIN_1D_P2
#endif
#else
#define DDR_FAST_SIM
#endif

#ifdef PHY_TRAIN
#define ADD_TRAIN_1D_P0
#ifdef DVFS_TEST
#define ADD_TRAIN_1D_P1
#endif
#endif

/* define BOOT FREQ, not modify */
#ifdef DDR_BOOT_P1
#define BOOT_FREQ P1_DRATE
#else
#ifdef DDR_BOOT_P2
#define  BOOT_FREQ P2_DRATE
#else
#define  BOOT_FREQ P0_DRATE
#endif
#endif

/* #define P1_FREQ 167 */
#ifdef PHY_TRAIN
#define CLOCK_SWITCH_PLL P0_DRATE
#else
#define CLOCK_SWITCH_PLL BOOT_FREQ
#endif

#define DDR_CSD2_BASE_ADDR 0x80000000
#define GPC_PU_PWRHSK 0x303A01FC

//----------------------------------------------------------------
// PHY training feature
//----------------------------------------------------------------
#define LPDDR4_HDT_CTL_2D 0xC8  //stage completion
#define LPDDR4_HDT_CTL_3200_1D 0xC8  //stage completion
#define LPDDR4_HDT_CTL_400_1D 0xC8  //stage completion
#define LPDDR4_HDT_CTL_100_1D 0xC8  //stage completion

#define LPDDR4_HDT_CTL_2D 0xC8  //stage completion
#define LPDDR4_HDT_CTL_3200_1D 0xC8  //stage completion
#define LPDDR4_HDT_CTL_400_1D 0xC8  //stage completion
#define LPDDR4_HDT_CTL_100_1D 0xC8  //stage completion

#ifdef RUN_ON_SILICON
// 400/100 training seq
#define LPDDR4_TRAIN_SEQ_P2 0x121f
#define LPDDR4_TRAIN_SEQ_P1 0x121f
#define LPDDR4_TRAIN_SEQ_P0 0x121f
#else
#define LPDDR4_TRAIN_SEQ_P2 0x7
#define LPDDR4_TRAIN_SEQ_P1 0x7
#define LPDDR4_TRAIN_SEQ_P0 0x7
#endif

//2D share & weight
#define LPDDR4_2D_WEIGHT 0x1f7f
#define LPDDR4_2D_SHARE 1
#define LPDDR4_CATRAIN_3200_1d 0
#define LPDDR4_CATRAIN_400 0
#define LPDDR4_CATRAIN_100 0
#define LPDDR4_CATRAIN_3200_2d 0

/* MRS parameter */
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
#define LPDDR4_PHY_RTT		30 /* //30//40//28 */
/* #define LPDDR4_PHY_VREF_VALUE 27//17//17//20//16///17,//for M845S */
#define LPDDR4_PHY_VREF_VALUE 	17 /*//17//20//16///17,//for M850D*/

/* for write channel */
#define LPDDR4_PHY_RON			30
#define LPDDR4_PHY_ADDR_RON		LPDDR4_PHY_ADDR_RON40
#define LPDDR4_RTT_DQ			LPDDR4_RTT40
#define LPDDR4_RTT_CA			LPDDR4_RTT40
#define LPDDR4_RTT_CA_BANK0		LPDDR4_RTT40
#define LPDDR4_RTT_CA_BANK1		LPDDR4_RTT40
#define LPDDR4_VREF_VALUE_CA		((1 << 6)|0xd)
#define LPDDR4_VREF_VALUE_DQ_RANK0	((1 << 6)|0xd)
#define LPDDR4_VREF_VALUE_DQ_RANK1	((1 << 6)|0xd)
#define LPDDR4_MR22_RANK0		((0 << 5)|(0 << 4)|(0 << 3)|(LPDDR4_RTT40))
#define LPDDR4_MR22_RANK1		((1 << 5)|(0 << 4)|(1 << 3)|(LPDDR4_RTT40))
#define LPDDR4_MR3_PU_CAL		1

#endif
