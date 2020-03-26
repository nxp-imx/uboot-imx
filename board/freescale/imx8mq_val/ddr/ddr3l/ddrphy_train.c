/*
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/clock.h>
#include <asm/arch/ddr.h>
#include "../ddr.h"

#define DDR3_MR1_RTT120_RON40   ((0L << 9) | (1L << 6) | (0L << 2) | (0L << 5) | (0L << 1)) /* RTT(NOM):M[9,6,2]=010:120ohm;Ron:M[5,1]=00:40ohm */
#define DDR3_MR1_RTT120_RON34   ((0L << 9) | (1L << 6) | (0L << 2) | (0L << 5) | (1L << 1)) /* RTT(NOM):M[9,6,2]=010:120ohm;Ron:M[5,1]=01:34ohm */
#define DDR3_MR1_RTT60_RON40    ((0L << 9) | (0L << 6) | (1L << 2) | (0L << 5) | (0L << 1)) /* RTT(NOM):M[9,6,2]=001:60ohm;Ron:M[5,1]=00:40ohm */
#define DDR3_MR1_RTT60_RON34    ((0L << 9) | (0L << 6) | (1L << 2) | (0L << 5) | (1L << 1)) /* RTT(NOM):M[9,6,2]=001:60ohm;Ron:M[5,1]=01:34ohm */
#define DDR3_MR1_RTT40_RON34    ((0L << 9) | (1L << 6) | (1L << 2) | (0L << 5) | (1L << 1)) /* RTT(NOM):M[9,6,2]=011:40ohm;Ron:M[5,1]=01:34ohm */
#define DDR3_MR1_RTT_DIS_RON40  ((0L << 9) | (0L << 6) | (0L << 2) | (0L << 5) | (0L << 1)) /* RTT(NOM):M[9,6,2]=000:disable;Ron:M[5,1]=00:40ohm */

#define DDR3_PHY_RON40    40 /* 40ohm */
#define DDR3_PHY_RON34    34 /* 34ohm */

#define DDR3_PHY_RTT120   120 /* 120ohm */
#define DDR3_PHY_RTT60    60 /* 60ohm */
#define DDR3_PHY_RTT40    40 /* 40ohm */
#define DDR3_PHY_RTT48    48 /* 48ohm */

#define DDR3_RTT_WR_DIS   0UL
#define DDR3_RTT_WR_60    1UL
#define DDR3_RTT_WR_120   2UL

#define DDR3_MR1_VAL            DDR3_MR1_RTT120_RON40
#define DDR3_MR2_RTT_WR_VAL     DDR3_RTT_WR_DIS

#define DDR3_PHY_RON            DDR3_PHY_RON40
#define DDR3_PHY_RTT            DDR3_PHY_RTT120


void ddr3_phyinit_train_1600mts(void){
	dwc_ddrphy_apb_wr(0xd0000,0x0); /* DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
	dwc_ddrphy_apb_wr(0x1005f,0x3ff); /*  DWC_DDRPHYA_DBYTE0_TxSlewRate_b0_p0 */
	dwc_ddrphy_apb_wr(0x1015f,0x3ff); /*  DWC_DDRPHYA_DBYTE0_TxSlewRate_b1_p0 */
	dwc_ddrphy_apb_wr(0x1105f,0x3ff); /*  DWC_DDRPHYA_DBYTE1_TxSlewRate_b0_p0 */
	dwc_ddrphy_apb_wr(0x1115f,0x3ff); /*  DWC_DDRPHYA_DBYTE1_TxSlewRate_b1_p0 */
	dwc_ddrphy_apb_wr(0x1205f,0x3ff); /*  DWC_DDRPHYA_DBYTE2_TxSlewRate_b0_p0 */
	dwc_ddrphy_apb_wr(0x1215f,0x3ff); /*  DWC_DDRPHYA_DBYTE2_TxSlewRate_b1_p0 */
	dwc_ddrphy_apb_wr(0x1305f,0x3ff); /*  DWC_DDRPHYA_DBYTE3_TxSlewRate_b0_p0 */
	dwc_ddrphy_apb_wr(0x1315f,0x3ff); /*  DWC_DDRPHYA_DBYTE3_TxSlewRate_b1_p0 */

	dwc_ddrphy_apb_wr(0x55,0x3ff); /*  DWC_DDRPHYA_ANIB0_ATxSlewRate */
	dwc_ddrphy_apb_wr(0x1055,0x3ff); /*  DWC_DDRPHYA_ANIB1_ATxSlewRate */
	dwc_ddrphy_apb_wr(0x2055,0x3ff); /*  DWC_DDRPHYA_ANIB2_ATxSlewRat */
	dwc_ddrphy_apb_wr(0x3055,0x3ff); /*  DWC_DDRPHYA_ANIB3_ATxSlewRate */
	dwc_ddrphy_apb_wr(0x4055,0xff); /*  DWC_DDRPHYA_ANIB4_ATxSlewRate */
	dwc_ddrphy_apb_wr(0x5055,0xff); /*  DWC_DDRPHYA_ANIB5_ATxSlewRate */
	dwc_ddrphy_apb_wr(0x6055,0x3ff); /*  DWC_DDRPHYA_ANIB6_ATxSlewRate */
	dwc_ddrphy_apb_wr(0x7055,0x3ff); /*  DWC_DDRPHYA_ANIB7_ATxSlewRate */
	dwc_ddrphy_apb_wr(0x8055,0x3ff); /*  DWC_DDRPHYA_ANIB8_ATxSlewRate */
	dwc_ddrphy_apb_wr(0x9055,0x3ff); /*  DWC_DDRPHYA_ANIB9_ATxSlewRate */
	dwc_ddrphy_apb_wr(0x200c5,0xb); /*  DWC_DDRPHYA_MASTER0_PllCtrl2_p0 */
	dwc_ddrphy_apb_wr(0x2002e,0x1); /*  DWC_DDRPHYA_MASTER0_ARdPtrInitVal_p0 */

	dwc_ddrphy_apb_wr(0x20024,0x8); /*  DWC_DDRPHYA_MASTER0_DqsPreambleControl_p0 */
	dwc_ddrphy_apb_wr(0x2003a,0x0); /*  DWC_DDRPHYA_MASTER0_DbyteDllModeCntrl */
	dwc_ddrphy_apb_wr(0x20056,0xa); /*  DWC_DDRPHYA_MASTER0_ProcOdtTimeCtl_p0 */
	dwc_ddrphy_apb_wr(0x1004d,0x208); /* DWC_DDRPHYA_DBYTE0_TxOdtDrvStren_b0_p0 */
	dwc_ddrphy_apb_wr(0x1014d,0x208); /* DWC_DDRPHYA_DBYTE0_TxOdtDrvStren_b1_p0 */
	dwc_ddrphy_apb_wr(0x1104d,0x208); /* DWC_DDRPHYA_DBYTE1_TxOdtDrvStren_b0_p0 */
	dwc_ddrphy_apb_wr(0x1114d,0x208); /* DWC_DDRPHYA_DBYTE1_TxOdtDrvStren_b1_p0 */
	dwc_ddrphy_apb_wr(0x1204d,0x208); /* DWC_DDRPHYA_DBYTE2_TxOdtDrvStren_b0_p0 */
	dwc_ddrphy_apb_wr(0x1214d,0x208); /* DWC_DDRPHYA_DBYTE2_TxOdtDrvStren_b1_p0 */
	dwc_ddrphy_apb_wr(0x1304d,0x208); /* DWC_DDRPHYA_DBYTE3_TxOdtDrvStren_b0_p0 */
	dwc_ddrphy_apb_wr(0x1314d,0x208); /* DWC_DDRPHYA_DBYTE3_TxOdtDrvStren_b1_p0 */
	dwc_ddrphy_apb_wr(0x10049,0xe38); /*  DWC_DDRPHYA_DBYTE0_TxImpedanceCtrl1_b0_p0 */
	dwc_ddrphy_apb_wr(0x10149,0xe38); /*  DWC_DDRPHYA_DBYTE0_TxImpedanceCtrl1_b1_p0 */
	dwc_ddrphy_apb_wr(0x11049,0xe38); /*  DWC_DDRPHYA_DBYTE1_TxImpedanceCtrl1_b0_p0 */
	dwc_ddrphy_apb_wr(0x11149,0xe38); /*  DWC_DDRPHYA_DBYTE1_TxImpedanceCtrl1_b1_p0 */
	dwc_ddrphy_apb_wr(0x12049,0xe38); /*  DWC_DDRPHYA_DBYTE2_TxImpedanceCtrl1_b0_p0 */
	dwc_ddrphy_apb_wr(0x12149,0xe38); /*  DWC_DDRPHYA_DBYTE2_TxImpedanceCtrl1_b1_p0 */
	dwc_ddrphy_apb_wr(0x13049,0xe38); /*  DWC_DDRPHYA_DBYTE3_TxImpedanceCtrl1_b0_p0 */
	dwc_ddrphy_apb_wr(0x13149,0xe38); /*  DWC_DDRPHYA_DBYTE3_TxImpedanceCtrl1_b1_p0 */
	dwc_ddrphy_apb_wr(0x43,0x63); /* DWC_DDRPHYA_ANIB0_ATxImpedance */
	dwc_ddrphy_apb_wr(0x1043,0x63); /* DWC_DDRPHYA_ANIB1_ATxImpedance */
	dwc_ddrphy_apb_wr(0x2043,0x63); /* DWC_DDRPHYA_ANIB2_ATxImpedance */
	dwc_ddrphy_apb_wr(0x3043,0x63); /* DWC_DDRPHYA_ANIB3_ATxImpedance */
	dwc_ddrphy_apb_wr(0x4043,0x63); /* DWC_DDRPHYA_ANIB4_ATxImpedance */
	dwc_ddrphy_apb_wr(0x5043,0x63); /* DWC_DDRPHYA_ANIB5_ATxImpedance */
	dwc_ddrphy_apb_wr(0x6043,0x63); /* DWC_DDRPHYA_ANIB6_ATxImpedance */
	dwc_ddrphy_apb_wr(0x7043,0x63); /* DWC_DDRPHYA_ANIB7_ATxImpedance */
	dwc_ddrphy_apb_wr(0x8043,0x63); /* DWC_DDRPHYA_ANIB8_ATxImpedance */
	dwc_ddrphy_apb_wr(0x9043,0x63); /* DWC_DDRPHYA_ANIB9_ATxImpedance */
	dwc_ddrphy_apb_wr(0x20018,0x5); /*  DWC_DDRPHYA_MASTER0_DfiMode */
	dwc_ddrphy_apb_wr(0x20075,0x0); /*  DWC_DDRPHYA_MASTER0_DfiCAMode */
	dwc_ddrphy_apb_wr(0x20050,0x0); /*  DWC_DDRPHYA_MASTER0_CalDrvStr0 */
	dwc_ddrphy_apb_wr(0x20008,0x190); /*  DWC_DDRPHYA_MASTER0_CalUclkInfo_p0 */
	dwc_ddrphy_apb_wr(0x20088,0x9); /*  DWC_DDRPHYA_MASTER0_CalRate */
	dwc_ddrphy_apb_wr(0x200b2,0xf8); /*  DWC_DDRPHYA_MASTER0_VrefInGlobal_p0 */
	dwc_ddrphy_apb_wr(0x10043,0x581); /*  DWC_DDRPHYA_DBYTE0_DqDqsRcvCntrl_b0_p0 */
	dwc_ddrphy_apb_wr(0x10143,0x581); /*  DWC_DDRPHYA_DBYTE0_DqDqsRcvCntrl_b1_p0 */
	dwc_ddrphy_apb_wr(0x11043,0x581); /*  DWC_DDRPHYA_DBYTE1_DqDqsRcvCntrl_b0_p0 */
	dwc_ddrphy_apb_wr(0x11143,0x581); /*  DWC_DDRPHYA_DBYTE1_DqDqsRcvCntrl_b1_p0 */
	dwc_ddrphy_apb_wr(0x12043,0x581); /*  DWC_DDRPHYA_DBYTE2_DqDqsRcvCntrl_b0_p0 */
	dwc_ddrphy_apb_wr(0x12143,0x581); /*  DWC_DDRPHYA_DBYTE2_DqDqsRcvCntrl_b1_p0 */
	dwc_ddrphy_apb_wr(0x13043,0x581); /*  DWC_DDRPHYA_DBYTE3_DqDqsRcvCntrl_b0_p0 */
	dwc_ddrphy_apb_wr(0x13143,0x581); /*  DWC_DDRPHYA_DBYTE3_DqDqsRcvCntrl_b1_p0 */
	dwc_ddrphy_apb_wr(0x200fa,0x1); /*  DWC_DDRPHYA_MASTER0_DfiFreqRatio_p0 */
	dwc_ddrphy_apb_wr(0x20019,0x5); /*  DWC_DDRPHYA_MASTER0_TristateModeCA_p0 */
	dwc_ddrphy_apb_wr(0x200f0,0x5555); /*  DWC_DDRPHYA_MASTER0_DfiFreqXlat0 */
	dwc_ddrphy_apb_wr(0x200f1,0x5555); /*  DWC_DDRPHYA_MASTER0_DfiFreqXlat1 */
	dwc_ddrphy_apb_wr(0x200f2,0x5555); /*  DWC_DDRPHYA_MASTER0_DfiFreqXlat2 */
	dwc_ddrphy_apb_wr(0x200f3,0x5555); /*  DWC_DDRPHYA_MASTER0_DfiFreqXlat3 */
	dwc_ddrphy_apb_wr(0x200f4,0x5555); /*  DWC_DDRPHYA_MASTER0_DfiFreqXlat4 */
	dwc_ddrphy_apb_wr(0x200f5,0x5555); /*  DWC_DDRPHYA_MASTER0_DfiFreqXlat5 */
	dwc_ddrphy_apb_wr(0x200f6,0x5555); /*  DWC_DDRPHYA_MASTER0_DfiFreqXlat6 */
	dwc_ddrphy_apb_wr(0x200f7,0xf000); /*  DWC_DDRPHYA_MASTER0_DfiFreqXlat7 */
	dwc_ddrphy_apb_wr(0x2000b,0x33); /*  DWC_DDRPHYA_MASTER0_Seq0BDLY0_p0 */
	dwc_ddrphy_apb_wr(0x2000c,0x65); /*  DWC_DDRPHYA_MASTER0_Seq0BDLY1_p0 */
	dwc_ddrphy_apb_wr(0x2000d,0x3e9); /*  DWC_DDRPHYA_MASTER0_Seq0BDLY2_p0 */
	dwc_ddrphy_apb_wr(0x2000e,0x2c); /*  DWC_DDRPHYA_MASTER0_Seq0BDLY3_p0 */
	dwc_ddrphy_apb_wr(0x20025,0x0); /*  DWC_DDRPHYA_MASTER0_MasterX4Config */
	dwc_ddrphy_apb_wr(0x2002d,0x0); /*  DWC_DDRPHYA_MASTER0_DMIPinPresent_p0 */

	dwc_ddrphy_apb_wr(0x20060,0x2); /*  DWC_DDRPHYA_MASTER0_MemResetL */
	dwc_ddrphy_apb_wr(0xd0000,0x0); /*  DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
	dwc_ddrphy_apb_wr(0xd0000,0x1); /*  DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
	dwc_ddrphy_apb_wr(0xd0000,0x0); /*  DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
	dwc_ddrphy_apb_wr(0x54000,0x0);
	dwc_ddrphy_apb_wr(0x54001,0x0);
	dwc_ddrphy_apb_wr(0x54002,0x0);
	dwc_ddrphy_apb_wr(0x54003,0x640);
	dwc_ddrphy_apb_wr(0x54004,0x2);
	dwc_ddrphy_apb_wr(0x54005,((DDR3_PHY_RON << 8) | (DDR3_PHY_RTT << 0)));
	dwc_ddrphy_apb_wr(0x54006,0x13b);
	dwc_ddrphy_apb_wr(0x54007,0x2000);

	dwc_ddrphy_apb_wr(0x54008,0x303); /* two ranks */

	dwc_ddrphy_apb_wr(0x54009,0x200);
	dwc_ddrphy_apb_wr(0x5400a,0x0);
	dwc_ddrphy_apb_wr(0x5400b,0x31f);
	dwc_ddrphy_apb_wr(0x5400c,0xc8);

	dwc_ddrphy_apb_wr(0x54012,0x1);
	dwc_ddrphy_apb_wr(0x5402f,0xd70); /* MR0 */
	dwc_ddrphy_apb_wr(0x54030,DDR3_MR1_VAL); /* MR1=6:Ron=34ohm/Rtt(NOM)=60ohm */
	dwc_ddrphy_apb_wr(0x54031,(0x18 | (DDR3_MR2_RTT_WR_VAL << 9))); /*MR2 */
	dwc_ddrphy_apb_wr(0x5403a,0x1221);
	dwc_ddrphy_apb_wr(0x5403b,0x4884);
	dwc_ddrphy_apb_wr(0xd0000,0x1); /*  DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
	dwc_ddrphy_apb_wr(0xd0000,0x1); /*  DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
	dwc_ddrphy_apb_wr(0xd0099,0x9); /*  DWC_DDRPHYA_APBONLY0_MicroReset */
	dwc_ddrphy_apb_wr(0xd0099,0x1); /*  DWC_DDRPHYA_APBONLY0_MicroReset */
	dwc_ddrphy_apb_wr(0xd0099,0x0); /*  DWC_DDRPHYA_APBONLY0_MicroReset */

	wait_ddrphy_training_complete();

	dwc_ddrphy_apb_wr(0xd0099,0x1); /*  DWC_DDRPHYA_APBONLY0_MicroReset */
	dwc_ddrphy_apb_wr(0xd0000,0x0); /*  DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
	dwc_ddrphy_apb_wr(0xd0000,0x1); /*  DWC_DDRPHYA_APBONLY0_MicroContMuxSel */

	dwc_ddrphy_apb_wr(0xd0000,0x0); /*  DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
	dwc_ddrphy_apb_wr(0x90000,0x10); /*  DWC_DDRPHYA_INITENG0_PreSequenceReg0b0s0 */
	dwc_ddrphy_apb_wr(0x90001,0x400); /*  DWC_DDRPHYA_INITENG0_PreSequenceReg0b0s1 */
	dwc_ddrphy_apb_wr(0x90002,0x10e); /*  DWC_DDRPHYA_INITENG0_PreSequenceReg0b0s2 */
	dwc_ddrphy_apb_wr(0x90003,0x0); /*  DWC_DDRPHYA_INITENG0_PreSequenceReg0b1s0 */
	dwc_ddrphy_apb_wr(0x90004,0x0); /*  DWC_DDRPHYA_INITENG0_PreSequenceReg0b1s1 */
	dwc_ddrphy_apb_wr(0x90005,0x8); /*  DWC_DDRPHYA_INITENG0_PreSequenceReg0b1s2 */
	dwc_ddrphy_apb_wr(0x90029,0xb); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b0s0 */
	dwc_ddrphy_apb_wr(0x9002a,0x480); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b0s1 */
	dwc_ddrphy_apb_wr(0x9002b,0x109); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b0s2 */
	dwc_ddrphy_apb_wr(0x9002c,0x8); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b1s0 */
	dwc_ddrphy_apb_wr(0x9002d,0x448); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b1s1 */
	dwc_ddrphy_apb_wr(0x9002e,0x139); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b1s2 */
	dwc_ddrphy_apb_wr(0x9002f,0x8); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b2s0 */
	dwc_ddrphy_apb_wr(0x90030,0x478); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b2s1 */
	dwc_ddrphy_apb_wr(0x90031,0x109); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b2s2 */
	dwc_ddrphy_apb_wr(0x90032,0x2); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b3s0 */
	dwc_ddrphy_apb_wr(0x90033,0x10); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b3s1 */
	dwc_ddrphy_apb_wr(0x90034,0x139); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b3s2 */
	dwc_ddrphy_apb_wr(0x90035,0xf); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b4s0 */
	dwc_ddrphy_apb_wr(0x90036,0x7c0); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b4s1 */
	dwc_ddrphy_apb_wr(0x90037,0x139); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b4s2 */
	dwc_ddrphy_apb_wr(0x90038,0x44); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b5s0 */
	dwc_ddrphy_apb_wr(0x90039,0x630); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b5s1 */
	dwc_ddrphy_apb_wr(0x9003a,0x159); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b5s2 */
	dwc_ddrphy_apb_wr(0x9003b,0x14f); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b6s0 */
	dwc_ddrphy_apb_wr(0x9003c,0x630); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b6s1 */
	dwc_ddrphy_apb_wr(0x9003d,0x159); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b6s2 */
	dwc_ddrphy_apb_wr(0x9003e,0x47); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b7s0 */
	dwc_ddrphy_apb_wr(0x9003f,0x630); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b7s1 */
	dwc_ddrphy_apb_wr(0x90040,0x149); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b7s2 */
	dwc_ddrphy_apb_wr(0x90041,0x4f); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b8s0 */
	dwc_ddrphy_apb_wr(0x90042,0x630); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b8s1 */
	dwc_ddrphy_apb_wr(0x90043,0x179); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b8s2 */
	dwc_ddrphy_apb_wr(0x90044,0x8); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b9s0 */
	dwc_ddrphy_apb_wr(0x90045,0xe0); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b9s1 */
	dwc_ddrphy_apb_wr(0x90046,0x109); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b9s2 */
	dwc_ddrphy_apb_wr(0x90047,0x0); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b10s0 */
	dwc_ddrphy_apb_wr(0x90048,0x7c8); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b10s1 */
	dwc_ddrphy_apb_wr(0x90049,0x109); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b10s2 */
	dwc_ddrphy_apb_wr(0x9004a,0x0); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b11s0 */
	dwc_ddrphy_apb_wr(0x9004b,0x1); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b11s1 */
	dwc_ddrphy_apb_wr(0x9004c,0x8); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b11s2 */
	dwc_ddrphy_apb_wr(0x9004d,0x0); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b12s0 */
	dwc_ddrphy_apb_wr(0x9004e,0x45a); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b12s1 */
	dwc_ddrphy_apb_wr(0x9004f,0x9); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b12s2 */
	dwc_ddrphy_apb_wr(0x90050,0x0); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b13s0 */
	dwc_ddrphy_apb_wr(0x90051,0x448); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b13s1 */
	dwc_ddrphy_apb_wr(0x90052,0x109); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b13s2 */
	dwc_ddrphy_apb_wr(0x90053,0x40); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b14s0 */
	dwc_ddrphy_apb_wr(0x90054,0x630); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b14s1 */
	dwc_ddrphy_apb_wr(0x90055,0x179); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b14s2 */
	dwc_ddrphy_apb_wr(0x90056,0x1); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b15s0 */
	dwc_ddrphy_apb_wr(0x90057,0x618); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b15s1 */
	dwc_ddrphy_apb_wr(0x90058,0x109); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b15s2 */
	dwc_ddrphy_apb_wr(0x90059,0x40c0); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b16s0 */
	dwc_ddrphy_apb_wr(0x9005a,0x630); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b16s1 */
	dwc_ddrphy_apb_wr(0x9005b,0x149); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b16s2 */
	dwc_ddrphy_apb_wr(0x9005c,0x8); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b17s0 */
	dwc_ddrphy_apb_wr(0x9005d,0x4); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b17s1 */
	dwc_ddrphy_apb_wr(0x9005e,0x48); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b17s2 */
	dwc_ddrphy_apb_wr(0x9005f,0x4040); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b18s0 */
	dwc_ddrphy_apb_wr(0x90060,0x630); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b18s1 */
	dwc_ddrphy_apb_wr(0x90061,0x149); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b18s2 */
	dwc_ddrphy_apb_wr(0x90062,0x0); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b19s0 */
	dwc_ddrphy_apb_wr(0x90063,0x4); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b19s1 */
	dwc_ddrphy_apb_wr(0x90064,0x48); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b19s2 */
	dwc_ddrphy_apb_wr(0x90065,0x40); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b20s0 */
	dwc_ddrphy_apb_wr(0x90066,0x630); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b20s1 */
	dwc_ddrphy_apb_wr(0x90067,0x149); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b20s2 */
	dwc_ddrphy_apb_wr(0x90068,0x10); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b21s0 */
	dwc_ddrphy_apb_wr(0x90069,0x4); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b21s1 */
	dwc_ddrphy_apb_wr(0x9006a,0x18); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b21s2 */
	dwc_ddrphy_apb_wr(0x9006b,0x0); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b22s0 */
	dwc_ddrphy_apb_wr(0x9006c,0x4); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b22s1 */
	dwc_ddrphy_apb_wr(0x9006d,0x78); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b22s2 */
	dwc_ddrphy_apb_wr(0x9006e,0x549); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b23s0 */
	dwc_ddrphy_apb_wr(0x9006f,0x630); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b23s1 */
	dwc_ddrphy_apb_wr(0x90070,0x159); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b23s2 */
	dwc_ddrphy_apb_wr(0x90071,0xd49); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b24s0 */
	dwc_ddrphy_apb_wr(0x90072,0x630); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b24s1 */
	dwc_ddrphy_apb_wr(0x90073,0x159); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b24s2 */
	dwc_ddrphy_apb_wr(0x90074,0x94a); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b25s0 */
	dwc_ddrphy_apb_wr(0x90075,0x630); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b25s1 */
	dwc_ddrphy_apb_wr(0x90076,0x159); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b25s2 */
	dwc_ddrphy_apb_wr(0x90077,0x441); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b26s0 */
	dwc_ddrphy_apb_wr(0x90078,0x630); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b26s1 */
	dwc_ddrphy_apb_wr(0x90079,0x149); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b26s2 */
	dwc_ddrphy_apb_wr(0x9007a,0x42); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b27s0 */
	dwc_ddrphy_apb_wr(0x9007b,0x630); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b27s1 */
	dwc_ddrphy_apb_wr(0x9007c,0x149); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b27s2 */
	dwc_ddrphy_apb_wr(0x9007d,0x1); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b28s0 */
	dwc_ddrphy_apb_wr(0x9007e,0x630); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b28s1 */
	dwc_ddrphy_apb_wr(0x9007f,0x149); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b28s2 */
	dwc_ddrphy_apb_wr(0x90080,0x0); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b29s0 */
	dwc_ddrphy_apb_wr(0x90081,0xe0); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b29s1 */
	dwc_ddrphy_apb_wr(0x90082,0x109); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b29s2 */
	dwc_ddrphy_apb_wr(0x90083,0xa); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b30s0 */
	dwc_ddrphy_apb_wr(0x90084,0x10); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b30s1 */
	dwc_ddrphy_apb_wr(0x90085,0x109); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b30s2 */
	dwc_ddrphy_apb_wr(0x90086,0x9); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b31s0 */
	dwc_ddrphy_apb_wr(0x90087,0x3c0); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b31s1 */
	dwc_ddrphy_apb_wr(0x90088,0x149); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b31s2 */
	dwc_ddrphy_apb_wr(0x90089,0x9); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b32s0 */
	dwc_ddrphy_apb_wr(0x9008a,0x3c0); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b32s1 */
	dwc_ddrphy_apb_wr(0x9008b,0x159); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b32s2 */
	dwc_ddrphy_apb_wr(0x9008c,0x18); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b33s0 */
	dwc_ddrphy_apb_wr(0x9008d,0x10); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b33s1 */
	dwc_ddrphy_apb_wr(0x9008e,0x109); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b33s2 */
	dwc_ddrphy_apb_wr(0x9008f,0x0); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b34s0 */
	dwc_ddrphy_apb_wr(0x90090,0x3c0); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b34s1 */
	dwc_ddrphy_apb_wr(0x90091,0x109); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b34s2 */
	dwc_ddrphy_apb_wr(0x90092,0x18); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b35s0 */
	dwc_ddrphy_apb_wr(0x90093,0x4); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b35s1 */
	dwc_ddrphy_apb_wr(0x90094,0x48); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b35s2 */
	dwc_ddrphy_apb_wr(0x90095,0x18); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b36s0 */
	dwc_ddrphy_apb_wr(0x90096,0x4); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b36s1 */
	dwc_ddrphy_apb_wr(0x90097,0x58); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b36s2 */
	dwc_ddrphy_apb_wr(0x90098,0xa); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b37s0 */
	dwc_ddrphy_apb_wr(0x90099,0x10); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b37s1 */
	dwc_ddrphy_apb_wr(0x9009a,0x109); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b37s2 */
	dwc_ddrphy_apb_wr(0x9009b,0x2); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b38s0 */
	dwc_ddrphy_apb_wr(0x9009c,0x10); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b38s1 */
	dwc_ddrphy_apb_wr(0x9009d,0x109); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b38s2 */
	dwc_ddrphy_apb_wr(0x9009e,0x7); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b39s0 */
	dwc_ddrphy_apb_wr(0x9009f,0x7c0); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b39s1 */
	dwc_ddrphy_apb_wr(0x900a0,0x109); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b39s2 */
	dwc_ddrphy_apb_wr(0x900a1,0x10); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b40s0 */
	dwc_ddrphy_apb_wr(0x900a2,0x10); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b40s1 */
	dwc_ddrphy_apb_wr(0x900a3,0x109); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b40s2 */
	dwc_ddrphy_apb_wr(0x900a4,0x0); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b41s0 */
	dwc_ddrphy_apb_wr(0x900a5,0x8140); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b41s1 */
	dwc_ddrphy_apb_wr(0x900a6,0x10c); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b41s2 */
	dwc_ddrphy_apb_wr(0x900a7,0x10); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b42s0 */
	dwc_ddrphy_apb_wr(0x900a8,0x8138); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b42s1 */
	dwc_ddrphy_apb_wr(0x900a9,0x10c); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b42s2 */
	dwc_ddrphy_apb_wr(0x900aa,0x8); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b43s0 */
	dwc_ddrphy_apb_wr(0x900ab,0x7c8); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b43s1 */
	dwc_ddrphy_apb_wr(0x900ac,0x101); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b43s2 */
	dwc_ddrphy_apb_wr(0x900ad,0x8); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b44s0 */
	dwc_ddrphy_apb_wr(0x900ae,0x0); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b44s1 */
	dwc_ddrphy_apb_wr(0x900af,0x8); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b44s2 */
	dwc_ddrphy_apb_wr(0x900b0,0x8); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b45s0 */
	dwc_ddrphy_apb_wr(0x900b1,0x448); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b45s1 */
	dwc_ddrphy_apb_wr(0x900b2,0x109); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b45s2 */
	dwc_ddrphy_apb_wr(0x900b3,0xf); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b46s0 */
	dwc_ddrphy_apb_wr(0x900b4,0x7c0); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b46s1 */
	dwc_ddrphy_apb_wr(0x900b5,0x109); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b46s2 */
	dwc_ddrphy_apb_wr(0x900b6,0x47); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b47s0 */
	dwc_ddrphy_apb_wr(0x900b7,0x630); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b47s1 */
	dwc_ddrphy_apb_wr(0x900b8,0x109); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b47s2 */
	dwc_ddrphy_apb_wr(0x900b9,0x8); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b48s0 */
	dwc_ddrphy_apb_wr(0x900ba,0x618); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b48s1 */
	dwc_ddrphy_apb_wr(0x900bb,0x109); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b48s2 */
	dwc_ddrphy_apb_wr(0x900bc,0x8); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b49s0 */
	dwc_ddrphy_apb_wr(0x900bd,0xe0); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b49s1 */
	dwc_ddrphy_apb_wr(0x900be,0x109); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b49s2 */
	dwc_ddrphy_apb_wr(0x900bf,0x0); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b50s0 */
	dwc_ddrphy_apb_wr(0x900c0,0x7c8); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b50s1 */
	dwc_ddrphy_apb_wr(0x900c1,0x109); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b50s2 */
	dwc_ddrphy_apb_wr(0x900c2,0x8); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b51s0 */
	dwc_ddrphy_apb_wr(0x900c3,0x8140); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b51s1 */
	dwc_ddrphy_apb_wr(0x900c4,0x10c); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b51s2 */
	dwc_ddrphy_apb_wr(0x900c5,0x0); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b52s0 */
	dwc_ddrphy_apb_wr(0x900c6,0x1); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b52s1 */
	dwc_ddrphy_apb_wr(0x900c7,0x8); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b52s2 */
	dwc_ddrphy_apb_wr(0x900c8,0x8); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b53s0 */
	dwc_ddrphy_apb_wr(0x900c9,0x4); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b53s1 */
	dwc_ddrphy_apb_wr(0x900ca,0x8); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b53s2 */
	dwc_ddrphy_apb_wr(0x900cb,0x8); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b54s0 */
	dwc_ddrphy_apb_wr(0x900cc,0x7c8); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b54s1 */
	dwc_ddrphy_apb_wr(0x900cd,0x101); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b54s2 */
	dwc_ddrphy_apb_wr(0x90006,0x0); /*  DWC_DDRPHYA_INITENG0_PostSequenceReg0b0s0 */
	dwc_ddrphy_apb_wr(0x90007,0x0); /*  DWC_DDRPHYA_INITENG0_PostSequenceReg0b0s1 */
	dwc_ddrphy_apb_wr(0x90008,0x8); /*  DWC_DDRPHYA_INITENG0_PostSequenceReg0b0s2 */
	dwc_ddrphy_apb_wr(0x90009,0x0); /*  DWC_DDRPHYA_INITENG0_PostSequenceReg0b1s0 */
	dwc_ddrphy_apb_wr(0x9000a,0x0); /*  DWC_DDRPHYA_INITENG0_PostSequenceReg0b1s1 */
	dwc_ddrphy_apb_wr(0x9000b,0x0); /*  DWC_DDRPHYA_INITENG0_PostSequenceReg0b1s2 */
	dwc_ddrphy_apb_wr(0xd00e7,0x400); /*  DWC_DDRPHYA_APBONLY0_SequencerOverride */
	dwc_ddrphy_apb_wr(0x90017,0x0); /*  DWC_DDRPHYA_INITENG0_StartVector0b0 */
	dwc_ddrphy_apb_wr(0x90026,0x2c); /*  DWC_DDRPHYA_INITENG0_StartVector0b15 */
	dwc_ddrphy_apb_wr(0x9000c,0x0); /*  DWC_DDRPHYA_INITENG0_Seq0BDisableFlag0 */
	dwc_ddrphy_apb_wr(0x9000d,0x173); /*  DWC_DDRPHYA_INITENG0_Seq0BDisableFlag1 */
	dwc_ddrphy_apb_wr(0x9000e,0x60); /*  DWC_DDRPHYA_INITENG0_Seq0BDisableFlag2 */
	dwc_ddrphy_apb_wr(0x9000f,0x6110); /*  DWC_DDRPHYA_INITENG0_Seq0BDisableFlag3 */
	dwc_ddrphy_apb_wr(0x90010,0x2152); /*  DWC_DDRPHYA_INITENG0_Seq0BDisableFlag4 */
	dwc_ddrphy_apb_wr(0x90011,0xdfbd); /*  DWC_DDRPHYA_INITENG0_Seq0BDisableFlag5 */
	dwc_ddrphy_apb_wr(0x90012,0xffff); /*  DWC_DDRPHYA_INITENG0_Seq0BDisableFlag6 */
	dwc_ddrphy_apb_wr(0x90013,0x6152); /*  DWC_DDRPHYA_INITENG0_Seq0BDisableFlag7 */
	dwc_ddrphy_apb_wr(0xc0080,0x0); /*  DWC_DDRPHYA_DRTUB0_UcclkHclkEnables */
	dwc_ddrphy_apb_wr(0xd0000,0x1); /*  DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
}
