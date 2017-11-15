/*
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/clock.h>
#include <asm/arch/ddr_memory_map.h>
#include "../ddr.h"

#define DDR_RON 2
#define PHY_RTT 48
#define PHYREF_VALUE 0x3b

#define PHY_RON 40
#define DDR_RTT 5
#define MR6_VALUE 0x1f


#define dwc_ddrphy_apb_wr(addr, data)  reg32_write(IP2APB_DDRPHY_IPS_BASE_ADDR(0)+4*(addr), data)
#define dwc_ddrphy_apb_rd(addr)        (reg32_read(IP2APB_DDRPHY_IPS_BASE_ADDR(0)+4*(addr)))

void ddr4_phyinit_train_2400mts(){
	dwc_ddrphy_apb_wr(0x1005f,0x2ff); /* DWC_DDRPHYA_DBYTE0_TxSlewRate_b0_p0 */
	dwc_ddrphy_apb_wr(0x1015f,0x2ff); /* DWC_DDRPHYA_DBYTE0_TxSlewRate_b1_p0 */
	dwc_ddrphy_apb_wr(0x1105f,0x2ff); /* DWC_DDRPHYA_DBYTE1_TxSlewRate_b0_p0 */
	dwc_ddrphy_apb_wr(0x1115f,0x2ff); /* DWC_DDRPHYA_DBYTE1_TxSlewRate_b1_p0 */
	dwc_ddrphy_apb_wr(0x1205f,0x2ff); /* DWC_DDRPHYA_DBYTE2_TxSlewRate_b0_p0 */
	dwc_ddrphy_apb_wr(0x1215f,0x2ff); /* DWC_DDRPHYA_DBYTE2_TxSlewRate_b1_p0 */
	dwc_ddrphy_apb_wr(0x1305f,0x2ff); /* DWC_DDRPHYA_DBYTE3_TxSlewRate_b0_p0 */
	dwc_ddrphy_apb_wr(0x1315f,0x2ff); /* DWC_DDRPHYA_DBYTE3_TxSlewRate_b1_p0 */

	dwc_ddrphy_apb_wr(0x55,0x3ff); /* DWC_DDRPHYA_ANIB0_ATxSlewRate */
	dwc_ddrphy_apb_wr(0x1055,0x3ff); /* DWC_DDRPHYA_ANIB1_ATxSlewRate */
	dwc_ddrphy_apb_wr(0x2055,0x3ff); /* DWC_DDRPHYA_ANIB2_ATxSlewRate */
	dwc_ddrphy_apb_wr(0x3055,0x3ff); /* DWC_DDRPHYA_ANIB3_ATxSlewRate */
	dwc_ddrphy_apb_wr(0x4055,0xff); /* DWC_DDRPHYA_ANIB4_ATxSlewRate */
	dwc_ddrphy_apb_wr(0x5055,0xff); /* DWC_DDRPHYA_ANIB5_ATxSlewRate */
	dwc_ddrphy_apb_wr(0x6055,0x3ff); /* DWC_DDRPHYA_ANIB6_ATxSlewRate */
	dwc_ddrphy_apb_wr(0x7055,0x3ff); /* DWC_DDRPHYA_ANIB7_ATxSlewRate */
	dwc_ddrphy_apb_wr(0x8055,0x3ff); /* DWC_DDRPHYA_ANIB8_ATxSlewRate */

	dwc_ddrphy_apb_wr(0x9055,0x3ff); /* DWC_DDRPHYA_ANIB9_ATxSlewRate */
	dwc_ddrphy_apb_wr(0x200c5,0xa); /* DWC_DDRPHYA_MASTER0_PllCtrl2_p0 */
	dwc_ddrphy_apb_wr(0x2002e,0x2); /* DWC_DDRPHYA_MASTER0_ARdPtrInitVal_p0 */
	dwc_ddrphy_apb_wr(0x20024,0x9); /* DWC_DDRPHYA_MASTER0_DqsPreambleControl_p0 */
	dwc_ddrphy_apb_wr(0x2003a,0x2); /* DWC_DDRPHYA_MASTER0_DbyteDllModeCntrl */
	dwc_ddrphy_apb_wr(0x20056,0x2); /* DWC_DDRPHYA_MASTER0_ProcOdtTimeCtl_p0 */
	dwc_ddrphy_apb_wr(0x1004d,0x1a); /* DWC_DDRPHYA_DBYTE0_TxOdtDrvStren_b0_p0 */
	dwc_ddrphy_apb_wr(0x1014d,0x1a); /* DWC_DDRPHYA_DBYTE0_TxOdtDrvStren_b1_p0 */
	dwc_ddrphy_apb_wr(0x1104d,0x1a); /* DWC_DDRPHYA_DBYTE1_TxOdtDrvStren_b0_p0 */
	dwc_ddrphy_apb_wr(0x1114d,0x1a); /* DWC_DDRPHYA_DBYTE1_TxOdtDrvStren_b1_p0 */
	dwc_ddrphy_apb_wr(0x1204d,0x1a); /* DWC_DDRPHYA_DBYTE2_TxOdtDrvStren_b0_p0 */
	dwc_ddrphy_apb_wr(0x1214d,0x1a); /* DWC_DDRPHYA_DBYTE2_TxOdtDrvStren_b1_p0 */
	dwc_ddrphy_apb_wr(0x1304d,0x1a); /* DWC_DDRPHYA_DBYTE3_TxOdtDrvStren_b0_p0 */
	dwc_ddrphy_apb_wr(0x1314d,0x1a); /* DWC_DDRPHYA_DBYTE3_TxOdtDrvStren_b1_p0 */
	dwc_ddrphy_apb_wr(0x10049,0xe38); /* DWC_DDRPHYA_DBYTE0_TxImpedanceCtrl1_b0_p0 */
	dwc_ddrphy_apb_wr(0x10149,0xe38); /* DWC_DDRPHYA_DBYTE0_TxImpedanceCtrl1_b1_p0 */
	dwc_ddrphy_apb_wr(0x11049,0xe38); /* DWC_DDRPHYA_DBYTE1_TxImpedanceCtrl1_b0_p0 */
	dwc_ddrphy_apb_wr(0x11149,0xe38); /* DWC_DDRPHYA_DBYTE1_TxImpedanceCtrl1_b1_p0 */
	dwc_ddrphy_apb_wr(0x12049,0xe38); /* DWC_DDRPHYA_DBYTE2_TxImpedanceCtrl1_b0_p0 */
	dwc_ddrphy_apb_wr(0x12149,0xe38); /* DWC_DDRPHYA_DBYTE2_TxImpedanceCtrl1_b1_p0 */
	dwc_ddrphy_apb_wr(0x13049,0xe38); /* DWC_DDRPHYA_DBYTE3_TxImpedanceCtrl1_b0_p0 */
	dwc_ddrphy_apb_wr(0x13149,0xe38); /* DWC_DDRPHYA_DBYTE3_TxImpedanceCtrl1_b1_p0 */
	dwc_ddrphy_apb_wr(0x43,0x3ff); /* DWC_DDRPHYA_ANIB0_ATxImpedance */
	dwc_ddrphy_apb_wr(0x1043,0x3ff); /* DWC_DDRPHYA_ANIB1_ATxImpedance */
	dwc_ddrphy_apb_wr(0x2043,0x3ff); /* DWC_DDRPHYA_ANIB2_ATxImpedance */
	dwc_ddrphy_apb_wr(0x3043,0x3ff); /* DWC_DDRPHYA_ANIB3_ATxImpedance */
	dwc_ddrphy_apb_wr(0x4043,0x3ff); /* DWC_DDRPHYA_ANIB4_ATxImpedance */
	dwc_ddrphy_apb_wr(0x5043,0x3ff); /* DWC_DDRPHYA_ANIB5_ATxImpedance */
	dwc_ddrphy_apb_wr(0x6043,0x3ff); /* DWC_DDRPHYA_ANIB6_ATxImpedance */
	dwc_ddrphy_apb_wr(0x7043,0x3ff); /* DWC_DDRPHYA_ANIB7_ATxImpedance */
	dwc_ddrphy_apb_wr(0x8043,0x3ff); /* DWC_DDRPHYA_ANIB8_ATxImpedance */
	dwc_ddrphy_apb_wr(0x9043,0x3ff); /* DWC_DDRPHYA_ANIB9_ATxImpedance */
	dwc_ddrphy_apb_wr(0x20018,0x5); /* DWC_DDRPHYA_MASTER0_DfiMode */
	dwc_ddrphy_apb_wr(0x20075,0x2); /* DWC_DDRPHYA_MASTER0_DfiCAMode */
	dwc_ddrphy_apb_wr(0x20050,0x0); /* DWC_DDRPHYA_MASTER0_CalDrvStr0 */
	dwc_ddrphy_apb_wr(0x20008,0x258); /* DWC_DDRPHYA_MASTER0_CalUclkInfo_p0 */
	dwc_ddrphy_apb_wr(0x20088,0x9); /* DWC_DDRPHYA_MASTER0_CalRate */
	dwc_ddrphy_apb_wr(0x200b2,0x288); /* DWC_DDRPHYA_MASTER0_VrefInGlobal_p0 */
	dwc_ddrphy_apb_wr(0x10043,0x5b1); /* DWC_DDRPHYA_DBYTE0_DqDqsRcvCntrl_b0_p0 */
	dwc_ddrphy_apb_wr(0x10143,0x5b1); /* DWC_DDRPHYA_DBYTE0_DqDqsRcvCntrl_b1_p0 */
	dwc_ddrphy_apb_wr(0x11043,0x5b1); /* DWC_DDRPHYA_DBYTE1_DqDqsRcvCntrl_b0_p0 */
	dwc_ddrphy_apb_wr(0x11143,0x5b1); /* DWC_DDRPHYA_DBYTE1_DqDqsRcvCntrl_b1_p0 */
	dwc_ddrphy_apb_wr(0x12043,0x5b1); /* DWC_DDRPHYA_DBYTE2_DqDqsRcvCntrl_b0_p0 */
	dwc_ddrphy_apb_wr(0x12143,0x5b1); /* DWC_DDRPHYA_DBYTE2_DqDqsRcvCntrl_b1_p0 */
	dwc_ddrphy_apb_wr(0x13043,0x5b1); /* DWC_DDRPHYA_DBYTE3_DqDqsRcvCntrl_b0_p0 */
	dwc_ddrphy_apb_wr(0x13143,0x5b1); /* DWC_DDRPHYA_DBYTE3_DqDqsRcvCntrl_b1_p0 */
	dwc_ddrphy_apb_wr(0x200fa,0x1); /* DWC_DDRPHYA_MASTER0_DfiFreqRatio_p0 */
	dwc_ddrphy_apb_wr(0x20019,0x5); /* DWC_DDRPHYA_MASTER0_TristateModeCA_p0 */
	dwc_ddrphy_apb_wr(0x200f0,0x5555); /* DWC_DDRPHYA_MASTER0_DfiFreqXlat0 */
	dwc_ddrphy_apb_wr(0x200f1,0x5555); /* DWC_DDRPHYA_MASTER0_DfiFreqXlat1 */
	dwc_ddrphy_apb_wr(0x200f2,0x5555); /* DWC_DDRPHYA_MASTER0_DfiFreqXlat2 */
	dwc_ddrphy_apb_wr(0x200f3,0x5555); /* DWC_DDRPHYA_MASTER0_DfiFreqXlat3 */
	dwc_ddrphy_apb_wr(0x200f4,0x5555); /* DWC_DDRPHYA_MASTER0_DfiFreqXlat4 */
	dwc_ddrphy_apb_wr(0x200f5,0x5555); /* DWC_DDRPHYA_MASTER0_DfiFreqXlat5 */
	dwc_ddrphy_apb_wr(0x200f6,0x5555); /* DWC_DDRPHYA_MASTER0_DfiFreqXlat6 */
	dwc_ddrphy_apb_wr(0x200f7,0xf000); /* DWC_DDRPHYA_MASTER0_DfiFreqXlat7 */
	dwc_ddrphy_apb_wr(0x2000b,0x4c); /* DWC_DDRPHYA_MASTER0_Seq0BDLY0_p0 */
	dwc_ddrphy_apb_wr(0x2000c,0x97); /* DWC_DDRPHYA_MASTER0_Seq0BDLY1_p0 */
	dwc_ddrphy_apb_wr(0x2000d,0x5dd); /* DWC_DDRPHYA_MASTER0_Seq0BDLY2_p0 */
	dwc_ddrphy_apb_wr(0x2000e,0x2c); /* DWC_DDRPHYA_MASTER0_Seq0BDLY3_p0 */
	dwc_ddrphy_apb_wr(0x20025,0x0); /* DWC_DDRPHYA_MASTER0_MasterX4Config */
	dwc_ddrphy_apb_wr(0x2002d,0x0); /* DWC_DDRPHYA_MASTER0_DMIPinPresent_p0 */
	dwc_ddrphy_apb_wr(0x20060,0x2); /* DWC_DDRPHYA_MASTER0_MemResetL */
	dwc_ddrphy_apb_wr(0xd0000,0x0); /* DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
	dwc_ddrphy_apb_wr(0xd0000,0x1); /* DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
	dwc_ddrphy_apb_wr(0xd0000,0x0); /* DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
	dwc_ddrphy_apb_wr(0x54000,0x80);/* should be 0x80 in silicon */
	dwc_ddrphy_apb_wr(0x54001,0x0);
	dwc_ddrphy_apb_wr(0x54002,0x0);
	dwc_ddrphy_apb_wr(0x54003,0x960);
	dwc_ddrphy_apb_wr(0x54004,0x2);

	dwc_ddrphy_apb_wr(0x54005,((PHY_RON<<8)|(PHY_RTT<<0))/*0x2830*/);
	dwc_ddrphy_apb_wr(0x54006,(0x200|PHYREF_VALUE)/*0x23b*/);
	dwc_ddrphy_apb_wr(0x54007,0x2000);

	dwc_ddrphy_apb_wr(0x54008,0x303); /* Two ranks */

	dwc_ddrphy_apb_wr(0x54009,0x200);/* no addr mirror, 0x200 addr mirror */
	dwc_ddrphy_apb_wr(0x5400a,0x0);
	dwc_ddrphy_apb_wr(0x5400b,0x31f);/* should be 0x31f in silicon */

	dwc_ddrphy_apb_wr(0x5400c,0xc8); /* 0xc8 indicates stage completion messages showed */

	dwc_ddrphy_apb_wr(0x5400d,0x0);
	dwc_ddrphy_apb_wr(0x5400e,0x0);
	dwc_ddrphy_apb_wr(0x5400f,0x0);
	dwc_ddrphy_apb_wr(0x54010,0x0);
	dwc_ddrphy_apb_wr(0x54011,0x0);
	dwc_ddrphy_apb_wr(0x54012,0x1);
	dwc_ddrphy_apb_wr(0x54013,0x0);
	dwc_ddrphy_apb_wr(0x54014,0x0);
	dwc_ddrphy_apb_wr(0x54015,0x0);
	dwc_ddrphy_apb_wr(0x54016,0x0);
	dwc_ddrphy_apb_wr(0x54017,0x0);
	dwc_ddrphy_apb_wr(0x54018,0x0);
	dwc_ddrphy_apb_wr(0x54019,0x0);
	dwc_ddrphy_apb_wr(0x5401a,0x0);
	dwc_ddrphy_apb_wr(0x5401b,0x0);
	dwc_ddrphy_apb_wr(0x5401c,0x0);
	dwc_ddrphy_apb_wr(0x5401d,0x0);
	dwc_ddrphy_apb_wr(0x5401e,0x0);
	dwc_ddrphy_apb_wr(0x5401f,0x0);
	dwc_ddrphy_apb_wr(0x54020,0x0);
	dwc_ddrphy_apb_wr(0x54021,0x0);
	dwc_ddrphy_apb_wr(0x54022,0x0);
	dwc_ddrphy_apb_wr(0x54023,0x0);
	dwc_ddrphy_apb_wr(0x54024,0x0);
	dwc_ddrphy_apb_wr(0x54025,0x0);
	dwc_ddrphy_apb_wr(0x54026,0x0);
	dwc_ddrphy_apb_wr(0x54027,0x0);
	dwc_ddrphy_apb_wr(0x54028,0x0);
	dwc_ddrphy_apb_wr(0x54029,0x0);
	dwc_ddrphy_apb_wr(0x5402a,0x0);
	dwc_ddrphy_apb_wr(0x5402b,0x0);
	dwc_ddrphy_apb_wr(0x5402c,0x0);
	dwc_ddrphy_apb_wr(0x5402d,0x0);
	dwc_ddrphy_apb_wr(0x5402e,0x0);

	dwc_ddrphy_apb_wr(0x5402f,0xa30);/* MR0 */
	dwc_ddrphy_apb_wr(0x54030, ((DDR_RTT<<8)|(DDR_RON<<1)|0x1)/*0x1*/);/* MR1 */
	dwc_ddrphy_apb_wr(0x54031,0x1018);/* MR2 */
	dwc_ddrphy_apb_wr(0x54032,0x240);/* MR3 */
	dwc_ddrphy_apb_wr(0x54033,0xa00);/* MR4 */
	dwc_ddrphy_apb_wr(0x54034,0x42);/* MR5 */
	dwc_ddrphy_apb_wr(0x54035,(0x800|MR6_VALUE)/*0x800*/);/* MR6 */

	reg32_read(IP2APB_DDRPHY_IPS_BASE_ADDR(0) + 4*(0x54030));
	reg32_read(IP2APB_DDRPHY_IPS_BASE_ADDR(0) + 4*(0x54035));

	dwc_ddrphy_apb_wr(0x54036,0x103);
	dwc_ddrphy_apb_wr(0x54037,0x0);
	dwc_ddrphy_apb_wr(0x54038,0x0);
	dwc_ddrphy_apb_wr(0x54039,0x0);
	dwc_ddrphy_apb_wr(0x5403a,0x0);
	dwc_ddrphy_apb_wr(0x5403b,0x0);
	dwc_ddrphy_apb_wr(0x5403c,0x0);
	dwc_ddrphy_apb_wr(0x5403d,0x0);
	dwc_ddrphy_apb_wr(0x5403e,0x0);
	dwc_ddrphy_apb_wr(0x5403f,0x1221);
	dwc_ddrphy_apb_wr(0x54040,0x0);
	dwc_ddrphy_apb_wr(0x54041,0x0);
	dwc_ddrphy_apb_wr(0x54042,0x0);
	dwc_ddrphy_apb_wr(0x54043,0x0);
	dwc_ddrphy_apb_wr(0x54044,0x0);
	dwc_ddrphy_apb_wr(0x54045,0x0);
	dwc_ddrphy_apb_wr(0x54046,0x0);
	dwc_ddrphy_apb_wr(0x54047,0x0);
	dwc_ddrphy_apb_wr(0x54048,0x0);
	dwc_ddrphy_apb_wr(0x54049,0x0);
	dwc_ddrphy_apb_wr(0x5404a,0x0);
	dwc_ddrphy_apb_wr(0x5404b,0x0);
	dwc_ddrphy_apb_wr(0x5404c,0x0);
	dwc_ddrphy_apb_wr(0x5404d,0x0);
	dwc_ddrphy_apb_wr(0x5404e,0x0);
	dwc_ddrphy_apb_wr(0x5404f,0x0);
	dwc_ddrphy_apb_wr(0x54050,0x0);
	dwc_ddrphy_apb_wr(0x54051,0x0);
	dwc_ddrphy_apb_wr(0x54052,0x0);
	dwc_ddrphy_apb_wr(0x54053,0x0);
	dwc_ddrphy_apb_wr(0x54054,0x0);
	dwc_ddrphy_apb_wr(0x54055,0x0);
	dwc_ddrphy_apb_wr(0x54056,0x0);
	dwc_ddrphy_apb_wr(0x54057,0x0);
	dwc_ddrphy_apb_wr(0x54058,0x0);
	dwc_ddrphy_apb_wr(0x54059,0x0);
	dwc_ddrphy_apb_wr(0x5405a,0x0);
	dwc_ddrphy_apb_wr(0x5405b,0x0);
	dwc_ddrphy_apb_wr(0x5405c,0x0);
	dwc_ddrphy_apb_wr(0x5405d,0x0);
	dwc_ddrphy_apb_wr(0x5405e,0x0);
	dwc_ddrphy_apb_wr(0x5405f,0x0);
	dwc_ddrphy_apb_wr(0x54060,0x0);
	dwc_ddrphy_apb_wr(0x54061,0x0);
	dwc_ddrphy_apb_wr(0x54062,0x0);
	dwc_ddrphy_apb_wr(0x54063,0x0);
	dwc_ddrphy_apb_wr(0x54064,0x0);
	dwc_ddrphy_apb_wr(0x54065,0x0);
	dwc_ddrphy_apb_wr(0x54066,0x0);
	dwc_ddrphy_apb_wr(0x54067,0x0);
	dwc_ddrphy_apb_wr(0x54068,0x0);
	dwc_ddrphy_apb_wr(0x54069,0x0);
	dwc_ddrphy_apb_wr(0x5406a,0x0);
	dwc_ddrphy_apb_wr(0x5406b,0x0);
	dwc_ddrphy_apb_wr(0x5406c,0x0);
	dwc_ddrphy_apb_wr(0x5406d,0x0);
	dwc_ddrphy_apb_wr(0x5406e,0x0);
	dwc_ddrphy_apb_wr(0x5406f,0x0);
	dwc_ddrphy_apb_wr(0x54070,0x0);
	dwc_ddrphy_apb_wr(0x54071,0x0);
	dwc_ddrphy_apb_wr(0x54072,0x0);
	dwc_ddrphy_apb_wr(0x54073,0x0);
	dwc_ddrphy_apb_wr(0x54074,0x0);
	dwc_ddrphy_apb_wr(0x54075,0x0);
	dwc_ddrphy_apb_wr(0x54076,0x0);
	dwc_ddrphy_apb_wr(0x54077,0x0);
	dwc_ddrphy_apb_wr(0x54078,0x0);
	dwc_ddrphy_apb_wr(0x54079,0x0);
	dwc_ddrphy_apb_wr(0x5407a,0x0);
	dwc_ddrphy_apb_wr(0x5407b,0x0);
	dwc_ddrphy_apb_wr(0x5407c,0x0);
	dwc_ddrphy_apb_wr(0x5407d,0x0);
	dwc_ddrphy_apb_wr(0x5407e,0x0);
	dwc_ddrphy_apb_wr(0x5407f,0x0);
	dwc_ddrphy_apb_wr(0x54080,0x0);
	dwc_ddrphy_apb_wr(0x54081,0x0);
	dwc_ddrphy_apb_wr(0x54082,0x0);
	dwc_ddrphy_apb_wr(0x54083,0x0);
	dwc_ddrphy_apb_wr(0x54084,0x0);
	dwc_ddrphy_apb_wr(0x54085,0x0);
	dwc_ddrphy_apb_wr(0x54086,0x0);
	dwc_ddrphy_apb_wr(0x54087,0x0);
	dwc_ddrphy_apb_wr(0x54088,0x0);
	dwc_ddrphy_apb_wr(0x54089,0x0);
	dwc_ddrphy_apb_wr(0x5408a,0x0);
	dwc_ddrphy_apb_wr(0x5408b,0x0);
	dwc_ddrphy_apb_wr(0x5408c,0x0);
	dwc_ddrphy_apb_wr(0x5408d,0x0);
	dwc_ddrphy_apb_wr(0x5408e,0x0);
	dwc_ddrphy_apb_wr(0x5408f,0x0);
	dwc_ddrphy_apb_wr(0x54090,0x0);
	dwc_ddrphy_apb_wr(0x54091,0x0);
	dwc_ddrphy_apb_wr(0x54092,0x0);
	dwc_ddrphy_apb_wr(0x54093,0x0);
	dwc_ddrphy_apb_wr(0x54094,0x0);
	dwc_ddrphy_apb_wr(0x54095,0x0);
	dwc_ddrphy_apb_wr(0x54096,0x0);
	dwc_ddrphy_apb_wr(0x54097,0x0);
	dwc_ddrphy_apb_wr(0x54098,0x0);
	dwc_ddrphy_apb_wr(0x54099,0x0);
	dwc_ddrphy_apb_wr(0x5409a,0x0);
	dwc_ddrphy_apb_wr(0x5409b,0x0);
	dwc_ddrphy_apb_wr(0x5409c,0x0);
	dwc_ddrphy_apb_wr(0x5409d,0x0);
	dwc_ddrphy_apb_wr(0x5409e,0x0);
	dwc_ddrphy_apb_wr(0x5409f,0x0);
	dwc_ddrphy_apb_wr(0x540a0,0x0);
	dwc_ddrphy_apb_wr(0x540a1,0x0);
	dwc_ddrphy_apb_wr(0x540a2,0x0);
	dwc_ddrphy_apb_wr(0x540a3,0x0);
	dwc_ddrphy_apb_wr(0x540a4,0x0);
	dwc_ddrphy_apb_wr(0x540a5,0x0);
	dwc_ddrphy_apb_wr(0x540a6,0x0);
	dwc_ddrphy_apb_wr(0x540a7,0x0);
	dwc_ddrphy_apb_wr(0x540a8,0x0);
	dwc_ddrphy_apb_wr(0x540a9,0x0);
	dwc_ddrphy_apb_wr(0x540aa,0x0);
	dwc_ddrphy_apb_wr(0x540ab,0x0);
	dwc_ddrphy_apb_wr(0x540ac,0x0);
	dwc_ddrphy_apb_wr(0x540ad,0x0);
	dwc_ddrphy_apb_wr(0x540ae,0x0);
	dwc_ddrphy_apb_wr(0x540af,0x0);
	dwc_ddrphy_apb_wr(0x540b0,0x0);
	dwc_ddrphy_apb_wr(0x540b1,0x0);
	dwc_ddrphy_apb_wr(0x540b2,0x0);
	dwc_ddrphy_apb_wr(0x540b3,0x0);
	dwc_ddrphy_apb_wr(0x540b4,0x0);
	dwc_ddrphy_apb_wr(0x540b5,0x0);
	dwc_ddrphy_apb_wr(0x540b6,0x0);
	dwc_ddrphy_apb_wr(0x540b7,0x0);
	dwc_ddrphy_apb_wr(0x540b8,0x0);
	dwc_ddrphy_apb_wr(0x540b9,0x0);
	dwc_ddrphy_apb_wr(0x540ba,0x0);
	dwc_ddrphy_apb_wr(0x540bb,0x0);
	dwc_ddrphy_apb_wr(0x540bc,0x0);
	dwc_ddrphy_apb_wr(0x540bd,0x0);
	dwc_ddrphy_apb_wr(0x540be,0x0);
	dwc_ddrphy_apb_wr(0x540bf,0x0);
	dwc_ddrphy_apb_wr(0x540c0,0x0);
	dwc_ddrphy_apb_wr(0x540c1,0x0);
	dwc_ddrphy_apb_wr(0x540c2,0x0);
	dwc_ddrphy_apb_wr(0x540c3,0x0);
	dwc_ddrphy_apb_wr(0x540c4,0x0);
	dwc_ddrphy_apb_wr(0x540c5,0x0);
	dwc_ddrphy_apb_wr(0x540c6,0x0);
	dwc_ddrphy_apb_wr(0x540c7,0x0);
	dwc_ddrphy_apb_wr(0x540c8,0x0);
	dwc_ddrphy_apb_wr(0x540c9,0x0);
	dwc_ddrphy_apb_wr(0x540ca,0x0);
	dwc_ddrphy_apb_wr(0x540cb,0x0);
	dwc_ddrphy_apb_wr(0x540cc,0x0);
	dwc_ddrphy_apb_wr(0x540cd,0x0);
	dwc_ddrphy_apb_wr(0x540ce,0x0);
	dwc_ddrphy_apb_wr(0x540cf,0x0);
	dwc_ddrphy_apb_wr(0x540d0,0x0);
	dwc_ddrphy_apb_wr(0x540d1,0x0);
	dwc_ddrphy_apb_wr(0x540d2,0x0);
	dwc_ddrphy_apb_wr(0x540d3,0x0);
	dwc_ddrphy_apb_wr(0x540d4,0x0);
	dwc_ddrphy_apb_wr(0x540d5,0x0);
	dwc_ddrphy_apb_wr(0x540d6,0x0);
	dwc_ddrphy_apb_wr(0x540d7,0x0);
	dwc_ddrphy_apb_wr(0x540d8,0x0);
	dwc_ddrphy_apb_wr(0x540d9,0x0);
	dwc_ddrphy_apb_wr(0x540da,0x0);
	dwc_ddrphy_apb_wr(0x540db,0x0);
	dwc_ddrphy_apb_wr(0x540dc,0x0);
	dwc_ddrphy_apb_wr(0x540dd,0x0);
	dwc_ddrphy_apb_wr(0x540de,0x0);
	dwc_ddrphy_apb_wr(0x540df,0x0);
	dwc_ddrphy_apb_wr(0x540e0,0x0);
	dwc_ddrphy_apb_wr(0x540e1,0x0);
	dwc_ddrphy_apb_wr(0x540e2,0x0);
	dwc_ddrphy_apb_wr(0x540e3,0x0);
	dwc_ddrphy_apb_wr(0x540e4,0x0);
	dwc_ddrphy_apb_wr(0x540e5,0x0);
	dwc_ddrphy_apb_wr(0x540e6,0x0);
	dwc_ddrphy_apb_wr(0x540e7,0x0);
	dwc_ddrphy_apb_wr(0x540e8,0x0);
	dwc_ddrphy_apb_wr(0x540e9,0x0);
	dwc_ddrphy_apb_wr(0x540ea,0x0);
	dwc_ddrphy_apb_wr(0x540eb,0x0);
	dwc_ddrphy_apb_wr(0x540ec,0x0);
	dwc_ddrphy_apb_wr(0x540ed,0x0);
	dwc_ddrphy_apb_wr(0x540ee,0x0);
	dwc_ddrphy_apb_wr(0x540ef,0x0);
	dwc_ddrphy_apb_wr(0x540f0,0x0);
	dwc_ddrphy_apb_wr(0x540f1,0x0);
	dwc_ddrphy_apb_wr(0x540f2,0x0);
	dwc_ddrphy_apb_wr(0x540f3,0x0);
	dwc_ddrphy_apb_wr(0x540f4,0x0);
	dwc_ddrphy_apb_wr(0x540f5,0x0);
	dwc_ddrphy_apb_wr(0x540f6,0x0);
	dwc_ddrphy_apb_wr(0x540f7,0x0);
	dwc_ddrphy_apb_wr(0x540f8,0x0);
	dwc_ddrphy_apb_wr(0x540f9,0x0);
	dwc_ddrphy_apb_wr(0x540fa,0x0);
	dwc_ddrphy_apb_wr(0x540fb,0x0);
	dwc_ddrphy_apb_wr(0x540fc,0x0);
	dwc_ddrphy_apb_wr(0x540fd,0x0);
	dwc_ddrphy_apb_wr(0x540fe,0x0);
	dwc_ddrphy_apb_wr(0x540ff,0x0);
	dwc_ddrphy_apb_wr(0x54100,0x0);
	dwc_ddrphy_apb_wr(0x54101,0x0);
	dwc_ddrphy_apb_wr(0x54102,0x0);
	dwc_ddrphy_apb_wr(0x54103,0x0);
	dwc_ddrphy_apb_wr(0x54104,0x0);
	dwc_ddrphy_apb_wr(0x54105,0x0);
	dwc_ddrphy_apb_wr(0x54106,0x0);
	dwc_ddrphy_apb_wr(0x54107,0x0);
	dwc_ddrphy_apb_wr(0x54108,0x0);
	dwc_ddrphy_apb_wr(0x54109,0x0);
	dwc_ddrphy_apb_wr(0x5410a,0x0);
	dwc_ddrphy_apb_wr(0x5410b,0x0);
	dwc_ddrphy_apb_wr(0x5410c,0x0);
	dwc_ddrphy_apb_wr(0x5410d,0x0);
	dwc_ddrphy_apb_wr(0x5410e,0x0);
	dwc_ddrphy_apb_wr(0x5410f,0x0);
	dwc_ddrphy_apb_wr(0x54110,0x0);
	dwc_ddrphy_apb_wr(0x54111,0x0);
	dwc_ddrphy_apb_wr(0x54112,0x0);
	dwc_ddrphy_apb_wr(0x54113,0x0);
	dwc_ddrphy_apb_wr(0x54114,0x0);
	dwc_ddrphy_apb_wr(0x54115,0x0);
	dwc_ddrphy_apb_wr(0x54116,0x0);
	dwc_ddrphy_apb_wr(0x54117,0x0);
	dwc_ddrphy_apb_wr(0x54118,0x0);
	dwc_ddrphy_apb_wr(0x54119,0x0);
	dwc_ddrphy_apb_wr(0x5411a,0x0);
	dwc_ddrphy_apb_wr(0x5411b,0x0);
	dwc_ddrphy_apb_wr(0x5411c,0x0);
	dwc_ddrphy_apb_wr(0x5411d,0x0);
	dwc_ddrphy_apb_wr(0x5411e,0x0);
	dwc_ddrphy_apb_wr(0x5411f,0x0);
	dwc_ddrphy_apb_wr(0x54120,0x0);
	dwc_ddrphy_apb_wr(0x54121,0x0);
	dwc_ddrphy_apb_wr(0x54122,0x0);
	dwc_ddrphy_apb_wr(0x54123,0x0);
	dwc_ddrphy_apb_wr(0x54124,0x0);
	dwc_ddrphy_apb_wr(0x54125,0x0);
	dwc_ddrphy_apb_wr(0x54126,0x0);
	dwc_ddrphy_apb_wr(0x54127,0x0);
	dwc_ddrphy_apb_wr(0x54128,0x0);
	dwc_ddrphy_apb_wr(0x54129,0x0);
	dwc_ddrphy_apb_wr(0x5412a,0x0);
	dwc_ddrphy_apb_wr(0x5412b,0x0);
	dwc_ddrphy_apb_wr(0x5412c,0x0);
	dwc_ddrphy_apb_wr(0x5412d,0x0);
	dwc_ddrphy_apb_wr(0x5412e,0x0);
	dwc_ddrphy_apb_wr(0x5412f,0x0);
	dwc_ddrphy_apb_wr(0x54130,0x0);
	dwc_ddrphy_apb_wr(0x54131,0x0);
	dwc_ddrphy_apb_wr(0x54132,0x0);
	dwc_ddrphy_apb_wr(0x54133,0x0);
	dwc_ddrphy_apb_wr(0x54134,0x0);
	dwc_ddrphy_apb_wr(0x54135,0x0);
	dwc_ddrphy_apb_wr(0x54136,0x0);
	dwc_ddrphy_apb_wr(0x54137,0x0);
	dwc_ddrphy_apb_wr(0x54138,0x0);
	dwc_ddrphy_apb_wr(0x54139,0x0);
	dwc_ddrphy_apb_wr(0x5413a,0x0);
	dwc_ddrphy_apb_wr(0x5413b,0x0);
	dwc_ddrphy_apb_wr(0x5413c,0x0);
	dwc_ddrphy_apb_wr(0x5413d,0x0);
	dwc_ddrphy_apb_wr(0x5413e,0x0);
	dwc_ddrphy_apb_wr(0x5413f,0x0);
	dwc_ddrphy_apb_wr(0x54140,0x0);
	dwc_ddrphy_apb_wr(0x54141,0x0);
	dwc_ddrphy_apb_wr(0x54142,0x0);
	dwc_ddrphy_apb_wr(0x54143,0x0);
	dwc_ddrphy_apb_wr(0x54144,0x0);
	dwc_ddrphy_apb_wr(0x54145,0x0);
	dwc_ddrphy_apb_wr(0x54146,0x0);
	dwc_ddrphy_apb_wr(0x54147,0x0);
	dwc_ddrphy_apb_wr(0x54148,0x0);
	dwc_ddrphy_apb_wr(0x54149,0x0);
	dwc_ddrphy_apb_wr(0x5414a,0x0);
	dwc_ddrphy_apb_wr(0x5414b,0x0);
	dwc_ddrphy_apb_wr(0x5414c,0x0);
	dwc_ddrphy_apb_wr(0x5414d,0x0);
	dwc_ddrphy_apb_wr(0x5414e,0x0);
	dwc_ddrphy_apb_wr(0x5414f,0x0);
	dwc_ddrphy_apb_wr(0x54150,0x0);
	dwc_ddrphy_apb_wr(0x54151,0x0);
	dwc_ddrphy_apb_wr(0x54152,0x0);
	dwc_ddrphy_apb_wr(0x54153,0x0);
	dwc_ddrphy_apb_wr(0x54154,0x0);
	dwc_ddrphy_apb_wr(0x54155,0x0);
	dwc_ddrphy_apb_wr(0x54156,0x0);
	dwc_ddrphy_apb_wr(0x54157,0x0);
	dwc_ddrphy_apb_wr(0x54158,0x0);
	dwc_ddrphy_apb_wr(0x54159,0x0);
	dwc_ddrphy_apb_wr(0x5415a,0x0);
	dwc_ddrphy_apb_wr(0x5415b,0x0);
	dwc_ddrphy_apb_wr(0x5415c,0x0);
	dwc_ddrphy_apb_wr(0x5415d,0x0);
	dwc_ddrphy_apb_wr(0x5415e,0x0);
	dwc_ddrphy_apb_wr(0x5415f,0x0);
	dwc_ddrphy_apb_wr(0x54160,0x0);
	dwc_ddrphy_apb_wr(0x54161,0x0);
	dwc_ddrphy_apb_wr(0x54162,0x0);
	dwc_ddrphy_apb_wr(0x54163,0x0);
	dwc_ddrphy_apb_wr(0x54164,0x0);
	dwc_ddrphy_apb_wr(0x54165,0x0);
	dwc_ddrphy_apb_wr(0x54166,0x0);
	dwc_ddrphy_apb_wr(0x54167,0x0);
	dwc_ddrphy_apb_wr(0x54168,0x0);
	dwc_ddrphy_apb_wr(0x54169,0x0);
	dwc_ddrphy_apb_wr(0x5416a,0x0);
	dwc_ddrphy_apb_wr(0x5416b,0x0);
	dwc_ddrphy_apb_wr(0x5416c,0x0);
	dwc_ddrphy_apb_wr(0x5416d,0x0);
	dwc_ddrphy_apb_wr(0x5416e,0x0);
	dwc_ddrphy_apb_wr(0x5416f,0x0);
	dwc_ddrphy_apb_wr(0x54170,0x0);
	dwc_ddrphy_apb_wr(0x54171,0x0);
	dwc_ddrphy_apb_wr(0x54172,0x0);
	dwc_ddrphy_apb_wr(0x54173,0x0);
	dwc_ddrphy_apb_wr(0x54174,0x0);
	dwc_ddrphy_apb_wr(0x54175,0x0);
	dwc_ddrphy_apb_wr(0x54176,0x0);
	dwc_ddrphy_apb_wr(0x54177,0x0);
	dwc_ddrphy_apb_wr(0x54178,0x0);
	dwc_ddrphy_apb_wr(0x54179,0x0);
	dwc_ddrphy_apb_wr(0x5417a,0x0);
	dwc_ddrphy_apb_wr(0x5417b,0x0);
	dwc_ddrphy_apb_wr(0x5417c,0x0);
	dwc_ddrphy_apb_wr(0x5417d,0x0);
	dwc_ddrphy_apb_wr(0x5417e,0x0);
	dwc_ddrphy_apb_wr(0x5417f,0x0);
	dwc_ddrphy_apb_wr(0x54180,0x0);
	dwc_ddrphy_apb_wr(0x54181,0x0);
	dwc_ddrphy_apb_wr(0x54182,0x0);
	dwc_ddrphy_apb_wr(0x54183,0x0);
	dwc_ddrphy_apb_wr(0x54184,0x0);
	dwc_ddrphy_apb_wr(0x54185,0x0);
	dwc_ddrphy_apb_wr(0x54186,0x0);
	dwc_ddrphy_apb_wr(0x54187,0x0);
	dwc_ddrphy_apb_wr(0x54188,0x0);
	dwc_ddrphy_apb_wr(0x54189,0x0);
	dwc_ddrphy_apb_wr(0x5418a,0x0);
	dwc_ddrphy_apb_wr(0x5418b,0x0);
	dwc_ddrphy_apb_wr(0x5418c,0x0);
	dwc_ddrphy_apb_wr(0x5418d,0x0);
	dwc_ddrphy_apb_wr(0x5418e,0x0);
	dwc_ddrphy_apb_wr(0x5418f,0x0);
	dwc_ddrphy_apb_wr(0x54190,0x0);
	dwc_ddrphy_apb_wr(0x54191,0x0);
	dwc_ddrphy_apb_wr(0x54192,0x0);
	dwc_ddrphy_apb_wr(0x54193,0x0);
	dwc_ddrphy_apb_wr(0x54194,0x0);
	dwc_ddrphy_apb_wr(0x54195,0x0);
	dwc_ddrphy_apb_wr(0x54196,0x0);
	dwc_ddrphy_apb_wr(0x54197,0x0);
	dwc_ddrphy_apb_wr(0x54198,0x0);
	dwc_ddrphy_apb_wr(0x54199,0x0);
	dwc_ddrphy_apb_wr(0x5419a,0x0);
	dwc_ddrphy_apb_wr(0x5419b,0x0);
	dwc_ddrphy_apb_wr(0x5419c,0x0);
	dwc_ddrphy_apb_wr(0x5419d,0x0);
	dwc_ddrphy_apb_wr(0x5419e,0x0);
	dwc_ddrphy_apb_wr(0x5419f,0x0);
	dwc_ddrphy_apb_wr(0x541a0,0x0);
	dwc_ddrphy_apb_wr(0x541a1,0x0);
	dwc_ddrphy_apb_wr(0x541a2,0x0);
	dwc_ddrphy_apb_wr(0x541a3,0x0);
	dwc_ddrphy_apb_wr(0x541a4,0x0);
	dwc_ddrphy_apb_wr(0x541a5,0x0);
	dwc_ddrphy_apb_wr(0x541a6,0x0);
	dwc_ddrphy_apb_wr(0x541a7,0x0);
	dwc_ddrphy_apb_wr(0x541a8,0x0);
	dwc_ddrphy_apb_wr(0x541a9,0x0);
	dwc_ddrphy_apb_wr(0x541aa,0x0);
	dwc_ddrphy_apb_wr(0x541ab,0x0);
	dwc_ddrphy_apb_wr(0x541ac,0x0);
	dwc_ddrphy_apb_wr(0x541ad,0x0);
	dwc_ddrphy_apb_wr(0x541ae,0x0);
	dwc_ddrphy_apb_wr(0x541af,0x0);
	dwc_ddrphy_apb_wr(0x541b0,0x0);
	dwc_ddrphy_apb_wr(0x541b1,0x0);
	dwc_ddrphy_apb_wr(0x541b2,0x0);
	dwc_ddrphy_apb_wr(0x541b3,0x0);
	dwc_ddrphy_apb_wr(0x541b4,0x0);
	dwc_ddrphy_apb_wr(0x541b5,0x0);
	dwc_ddrphy_apb_wr(0x541b6,0x0);
	dwc_ddrphy_apb_wr(0x541b7,0x0);
	dwc_ddrphy_apb_wr(0x541b8,0x0);
	dwc_ddrphy_apb_wr(0x541b9,0x0);
	dwc_ddrphy_apb_wr(0x541ba,0x0);
	dwc_ddrphy_apb_wr(0x541bb,0x0);
	dwc_ddrphy_apb_wr(0x541bc,0x0);
	dwc_ddrphy_apb_wr(0x541bd,0x0);
	dwc_ddrphy_apb_wr(0x541be,0x0);
	dwc_ddrphy_apb_wr(0x541bf,0x0);
	dwc_ddrphy_apb_wr(0x541c0,0x0);
	dwc_ddrphy_apb_wr(0x541c1,0x0);
	dwc_ddrphy_apb_wr(0x541c2,0x0);
	dwc_ddrphy_apb_wr(0x541c3,0x0);
	dwc_ddrphy_apb_wr(0x541c4,0x0);
	dwc_ddrphy_apb_wr(0x541c5,0x0);
	dwc_ddrphy_apb_wr(0x541c6,0x0);
	dwc_ddrphy_apb_wr(0x541c7,0x0);
	dwc_ddrphy_apb_wr(0x541c8,0x0);
	dwc_ddrphy_apb_wr(0x541c9,0x0);
	dwc_ddrphy_apb_wr(0x541ca,0x0);
	dwc_ddrphy_apb_wr(0x541cb,0x0);
	dwc_ddrphy_apb_wr(0x541cc,0x0);
	dwc_ddrphy_apb_wr(0x541cd,0x0);
	dwc_ddrphy_apb_wr(0x541ce,0x0);
	dwc_ddrphy_apb_wr(0x541cf,0x0);
	dwc_ddrphy_apb_wr(0x541d0,0x0);
	dwc_ddrphy_apb_wr(0x541d1,0x0);
	dwc_ddrphy_apb_wr(0x541d2,0x0);
	dwc_ddrphy_apb_wr(0x541d3,0x0);
	dwc_ddrphy_apb_wr(0x541d4,0x0);
	dwc_ddrphy_apb_wr(0x541d5,0x0);
	dwc_ddrphy_apb_wr(0x541d6,0x0);
	dwc_ddrphy_apb_wr(0x541d7,0x0);
	dwc_ddrphy_apb_wr(0x541d8,0x0);
	dwc_ddrphy_apb_wr(0x541d9,0x0);
	dwc_ddrphy_apb_wr(0x541da,0x0);
	dwc_ddrphy_apb_wr(0x541db,0x0);
	dwc_ddrphy_apb_wr(0x541dc,0x0);
	dwc_ddrphy_apb_wr(0x541dd,0x0);
	dwc_ddrphy_apb_wr(0x541de,0x0);
	dwc_ddrphy_apb_wr(0x541df,0x0);
	dwc_ddrphy_apb_wr(0x541e0,0x0);
	dwc_ddrphy_apb_wr(0x541e1,0x0);
	dwc_ddrphy_apb_wr(0x541e2,0x0);
	dwc_ddrphy_apb_wr(0x541e3,0x0);
	dwc_ddrphy_apb_wr(0x541e4,0x0);
	dwc_ddrphy_apb_wr(0x541e5,0x0);
	dwc_ddrphy_apb_wr(0x541e6,0x0);
	dwc_ddrphy_apb_wr(0x541e7,0x0);
	dwc_ddrphy_apb_wr(0x541e8,0x0);
	dwc_ddrphy_apb_wr(0x541e9,0x0);
	dwc_ddrphy_apb_wr(0x541ea,0x0);
	dwc_ddrphy_apb_wr(0x541eb,0x0);
	dwc_ddrphy_apb_wr(0x541ec,0x0);
	dwc_ddrphy_apb_wr(0x541ed,0x0);
	dwc_ddrphy_apb_wr(0x541ee,0x0);
	dwc_ddrphy_apb_wr(0x541ef,0x0);
	dwc_ddrphy_apb_wr(0x541f0,0x0);
	dwc_ddrphy_apb_wr(0x541f1,0x0);
	dwc_ddrphy_apb_wr(0x541f2,0x0);
	dwc_ddrphy_apb_wr(0x541f3,0x0);
	dwc_ddrphy_apb_wr(0x541f4,0x0);
	dwc_ddrphy_apb_wr(0x541f5,0x0);
	dwc_ddrphy_apb_wr(0x541f6,0x0);
	dwc_ddrphy_apb_wr(0x541f7,0x0);
	dwc_ddrphy_apb_wr(0x541f8,0x0);
	dwc_ddrphy_apb_wr(0x541f9,0x0);
	dwc_ddrphy_apb_wr(0x541fa,0x0);
	dwc_ddrphy_apb_wr(0x541fb,0x0);
	dwc_ddrphy_apb_wr(0x541fc,0x100);
	dwc_ddrphy_apb_wr(0xd0000,0x1); /* DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
	dwc_ddrphy_apb_wr(0xd0000,0x1); /* DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
	dwc_ddrphy_apb_wr(0xd0099,0x9); /* DWC_DDRPHYA_APBONLY0_MicroReset */
	dwc_ddrphy_apb_wr(0xd0099,0x1); /* DWC_DDRPHYA_APBONLY0_MicroReset */
	dwc_ddrphy_apb_wr(0xd0099,0x0); /* DWC_DDRPHYA_APBONLY0_MicroReset */

	wait_ddrphy_training_complete();

	dwc_ddrphy_apb_wr(0xd0099,0x1); /* DWC_DDRPHYA_APBONLY0_MicroReset */
	dwc_ddrphy_apb_wr(0xd0000,0x0); /* DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
	dwc_ddrphy_apb_wr(0xd0000,0x1); /* DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
	dwc_ddrphy_apb_wr(0xd0000,0x0); /* DWC_DDRPHYA_APBONLY0_MicroContMuxSel */

	ddr_load_train_code(FW_2D_IMAGE);

	dwc_ddrphy_apb_wr(0xd0000,0x0); /* DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
	dwc_ddrphy_apb_wr(0xd0000,0x1); /* DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
	dwc_ddrphy_apb_wr(0xd0000,0x0); /* DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
	dwc_ddrphy_apb_wr(0x54000,0x80);/* should be 0x80 in silicon */
	dwc_ddrphy_apb_wr(0x54001,0x0);
	dwc_ddrphy_apb_wr(0x54002,0x0);
	dwc_ddrphy_apb_wr(0x54003,0x960);
	dwc_ddrphy_apb_wr(0x54004,0x2);

	dwc_ddrphy_apb_wr(0x54005,((PHY_RON<<8)|(PHY_RTT<<0))/*0x2830*/);
	dwc_ddrphy_apb_wr(0x54006,(0x200|PHYREF_VALUE)/*0x23b*/);
	dwc_ddrphy_apb_wr(0x54007,0x2000);

	dwc_ddrphy_apb_wr(0x54008,0x303);

	dwc_ddrphy_apb_wr(0x54009,0x200);
	dwc_ddrphy_apb_wr(0x5400a,0x0);
	dwc_ddrphy_apb_wr(0x5400b,0x61);/* should be 0x61 in silicon */

	dwc_ddrphy_apb_wr(0x5400c,0xc8); /* 0xc8 indicates stage completion messages showed */

	dwc_ddrphy_apb_wr(0x5400d,0x0);
	dwc_ddrphy_apb_wr(0x5400e,0x8020);
	dwc_ddrphy_apb_wr(0x5400f,0x0);
	dwc_ddrphy_apb_wr(0x54010,0x0);
	dwc_ddrphy_apb_wr(0x54011,0x0);
	dwc_ddrphy_apb_wr(0x54012,0x1);
	dwc_ddrphy_apb_wr(0x54013,0x0);
	dwc_ddrphy_apb_wr(0x54014,0x0);
	dwc_ddrphy_apb_wr(0x54015,0x0);
	dwc_ddrphy_apb_wr(0x54016,0x0);
	dwc_ddrphy_apb_wr(0x54017,0x0);
	dwc_ddrphy_apb_wr(0x54018,0x0);
	dwc_ddrphy_apb_wr(0x54019,0x0);
	dwc_ddrphy_apb_wr(0x5401a,0x0);
	dwc_ddrphy_apb_wr(0x5401b,0x0);
	dwc_ddrphy_apb_wr(0x5401c,0x0);
	dwc_ddrphy_apb_wr(0x5401d,0x0);
	dwc_ddrphy_apb_wr(0x5401e,0x0);
	dwc_ddrphy_apb_wr(0x5401f,0x0);
	dwc_ddrphy_apb_wr(0x54020,0x0);
	dwc_ddrphy_apb_wr(0x54021,0x0);
	dwc_ddrphy_apb_wr(0x54022,0x0);
	dwc_ddrphy_apb_wr(0x54023,0x0);
	dwc_ddrphy_apb_wr(0x54024,0x0);
	dwc_ddrphy_apb_wr(0x54025,0x0);
	dwc_ddrphy_apb_wr(0x54026,0x0);
	dwc_ddrphy_apb_wr(0x54027,0x0);
	dwc_ddrphy_apb_wr(0x54028,0x0);
	dwc_ddrphy_apb_wr(0x54029,0x0);
	dwc_ddrphy_apb_wr(0x5402a,0x0);
	dwc_ddrphy_apb_wr(0x5402b,0x0);
	dwc_ddrphy_apb_wr(0x5402c,0x0);
	dwc_ddrphy_apb_wr(0x5402d,0x0);
	dwc_ddrphy_apb_wr(0x5402e,0x0);

	dwc_ddrphy_apb_wr(0x5402f,0xa30);/* MR0 */
	dwc_ddrphy_apb_wr(0x54030, ((DDR_RTT<<8)|(DDR_RON<<1)|0x1)/*0x1*/);/* MR1 */
	dwc_ddrphy_apb_wr(0x54031,0x1018);/* MR2 */
	dwc_ddrphy_apb_wr(0x54032,0x240);/* MR3 */
	dwc_ddrphy_apb_wr(0x54033,0xa00);/* MR4 */
	dwc_ddrphy_apb_wr(0x54034,0x42);/* MR5 */
	dwc_ddrphy_apb_wr(0x54035,(0x800|MR6_VALUE)/*0x800*/);/* MR6 */

	dwc_ddrphy_apb_wr(0x54036,0x103);
	dwc_ddrphy_apb_wr(0x54037,0x0);
	dwc_ddrphy_apb_wr(0x54038,0x0);
	dwc_ddrphy_apb_wr(0x54039,0x0);
	dwc_ddrphy_apb_wr(0x5403a,0x0);
	dwc_ddrphy_apb_wr(0x5403b,0x0);
	dwc_ddrphy_apb_wr(0x5403c,0x0);
	dwc_ddrphy_apb_wr(0x5403d,0x0);
	dwc_ddrphy_apb_wr(0x5403e,0x0);
	dwc_ddrphy_apb_wr(0x5403f,0x1221);
	dwc_ddrphy_apb_wr(0x54040,0x0);
	dwc_ddrphy_apb_wr(0x54041,0x0);
	dwc_ddrphy_apb_wr(0x54042,0x0);
	dwc_ddrphy_apb_wr(0x54043,0x0);
	dwc_ddrphy_apb_wr(0x54044,0x0);
	dwc_ddrphy_apb_wr(0x54045,0x0);
	dwc_ddrphy_apb_wr(0x54046,0x0);
	dwc_ddrphy_apb_wr(0x54047,0x0);
	dwc_ddrphy_apb_wr(0x54048,0x0);
	dwc_ddrphy_apb_wr(0x54049,0x0);
	dwc_ddrphy_apb_wr(0x5404a,0x0);
	dwc_ddrphy_apb_wr(0x5404b,0x0);
	dwc_ddrphy_apb_wr(0x5404c,0x0);
	dwc_ddrphy_apb_wr(0x5404d,0x0);
	dwc_ddrphy_apb_wr(0x5404e,0x0);
	dwc_ddrphy_apb_wr(0x5404f,0x0);
	dwc_ddrphy_apb_wr(0x54050,0x0);
	dwc_ddrphy_apb_wr(0x54051,0x0);
	dwc_ddrphy_apb_wr(0x54052,0x0);
	dwc_ddrphy_apb_wr(0x54053,0x0);
	dwc_ddrphy_apb_wr(0x54054,0x0);
	dwc_ddrphy_apb_wr(0x54055,0x0);
	dwc_ddrphy_apb_wr(0x54056,0x0);
	dwc_ddrphy_apb_wr(0x54057,0x0);
	dwc_ddrphy_apb_wr(0x54058,0x0);
	dwc_ddrphy_apb_wr(0x54059,0x0);
	dwc_ddrphy_apb_wr(0x5405a,0x0);
	dwc_ddrphy_apb_wr(0x5405b,0x0);
	dwc_ddrphy_apb_wr(0x5405c,0x0);
	dwc_ddrphy_apb_wr(0x5405d,0x0);
	dwc_ddrphy_apb_wr(0x5405e,0x0);
	dwc_ddrphy_apb_wr(0x5405f,0x0);
	dwc_ddrphy_apb_wr(0x54060,0x0);
	dwc_ddrphy_apb_wr(0x54061,0x0);
	dwc_ddrphy_apb_wr(0x54062,0x0);
	dwc_ddrphy_apb_wr(0x54063,0x0);
	dwc_ddrphy_apb_wr(0x54064,0x0);
	dwc_ddrphy_apb_wr(0x54065,0x0);
	dwc_ddrphy_apb_wr(0x54066,0x0);
	dwc_ddrphy_apb_wr(0x54067,0x0);
	dwc_ddrphy_apb_wr(0x54068,0x0);
	dwc_ddrphy_apb_wr(0x54069,0x0);
	dwc_ddrphy_apb_wr(0x5406a,0x0);
	dwc_ddrphy_apb_wr(0x5406b,0x0);
	dwc_ddrphy_apb_wr(0x5406c,0x0);
	dwc_ddrphy_apb_wr(0x5406d,0x0);
	dwc_ddrphy_apb_wr(0x5406e,0x0);
	dwc_ddrphy_apb_wr(0x5406f,0x0);
	dwc_ddrphy_apb_wr(0x54070,0x0);
	dwc_ddrphy_apb_wr(0x54071,0x0);
	dwc_ddrphy_apb_wr(0x54072,0x0);
	dwc_ddrphy_apb_wr(0x54073,0x0);
	dwc_ddrphy_apb_wr(0x54074,0x0);
	dwc_ddrphy_apb_wr(0x54075,0x0);
	dwc_ddrphy_apb_wr(0x54076,0x0);
	dwc_ddrphy_apb_wr(0x54077,0x0);
	dwc_ddrphy_apb_wr(0x54078,0x0);
	dwc_ddrphy_apb_wr(0x54079,0x0);
	dwc_ddrphy_apb_wr(0x5407a,0x0);
	dwc_ddrphy_apb_wr(0x5407b,0x0);
	dwc_ddrphy_apb_wr(0x5407c,0x0);
	dwc_ddrphy_apb_wr(0x5407d,0x0);
	dwc_ddrphy_apb_wr(0x5407e,0x0);
	dwc_ddrphy_apb_wr(0x5407f,0x0);
	dwc_ddrphy_apb_wr(0x54080,0x0);
	dwc_ddrphy_apb_wr(0x54081,0x0);
	dwc_ddrphy_apb_wr(0x54082,0x0);
	dwc_ddrphy_apb_wr(0x54083,0x0);
	dwc_ddrphy_apb_wr(0x54084,0x0);
	dwc_ddrphy_apb_wr(0x54085,0x0);
	dwc_ddrphy_apb_wr(0x54086,0x0);
	dwc_ddrphy_apb_wr(0x54087,0x0);
	dwc_ddrphy_apb_wr(0x54088,0x0);
	dwc_ddrphy_apb_wr(0x54089,0x0);
	dwc_ddrphy_apb_wr(0x5408a,0x0);
	dwc_ddrphy_apb_wr(0x5408b,0x0);
	dwc_ddrphy_apb_wr(0x5408c,0x0);
	dwc_ddrphy_apb_wr(0x5408d,0x0);
	dwc_ddrphy_apb_wr(0x5408e,0x0);
	dwc_ddrphy_apb_wr(0x5408f,0x0);
	dwc_ddrphy_apb_wr(0x54090,0x0);
	dwc_ddrphy_apb_wr(0x54091,0x0);
	dwc_ddrphy_apb_wr(0x54092,0x0);
	dwc_ddrphy_apb_wr(0x54093,0x0);
	dwc_ddrphy_apb_wr(0x54094,0x0);
	dwc_ddrphy_apb_wr(0x54095,0x0);
	dwc_ddrphy_apb_wr(0x54096,0x0);
	dwc_ddrphy_apb_wr(0x54097,0x0);
	dwc_ddrphy_apb_wr(0x54098,0x0);
	dwc_ddrphy_apb_wr(0x54099,0x0);
	dwc_ddrphy_apb_wr(0x5409a,0x0);
	dwc_ddrphy_apb_wr(0x5409b,0x0);
	dwc_ddrphy_apb_wr(0x5409c,0x0);
	dwc_ddrphy_apb_wr(0x5409d,0x0);
	dwc_ddrphy_apb_wr(0x5409e,0x0);
	dwc_ddrphy_apb_wr(0x5409f,0x0);
	dwc_ddrphy_apb_wr(0x540a0,0x0);
	dwc_ddrphy_apb_wr(0x540a1,0x0);
	dwc_ddrphy_apb_wr(0x540a2,0x0);
	dwc_ddrphy_apb_wr(0x540a3,0x0);
	dwc_ddrphy_apb_wr(0x540a4,0x0);
	dwc_ddrphy_apb_wr(0x540a5,0x0);
	dwc_ddrphy_apb_wr(0x540a6,0x0);
	dwc_ddrphy_apb_wr(0x540a7,0x0);
	dwc_ddrphy_apb_wr(0x540a8,0x0);
	dwc_ddrphy_apb_wr(0x540a9,0x0);
	dwc_ddrphy_apb_wr(0x540aa,0x0);
	dwc_ddrphy_apb_wr(0x540ab,0x0);
	dwc_ddrphy_apb_wr(0x540ac,0x0);
	dwc_ddrphy_apb_wr(0x540ad,0x0);
	dwc_ddrphy_apb_wr(0x540ae,0x0);
	dwc_ddrphy_apb_wr(0x540af,0x0);
	dwc_ddrphy_apb_wr(0x540b0,0x0);
	dwc_ddrphy_apb_wr(0x540b1,0x0);
	dwc_ddrphy_apb_wr(0x540b2,0x0);
	dwc_ddrphy_apb_wr(0x540b3,0x0);
	dwc_ddrphy_apb_wr(0x540b4,0x0);
	dwc_ddrphy_apb_wr(0x540b5,0x0);
	dwc_ddrphy_apb_wr(0x540b6,0x0);
	dwc_ddrphy_apb_wr(0x540b7,0x0);
	dwc_ddrphy_apb_wr(0x540b8,0x0);
	dwc_ddrphy_apb_wr(0x540b9,0x0);
	dwc_ddrphy_apb_wr(0x540ba,0x0);
	dwc_ddrphy_apb_wr(0x540bb,0x0);
	dwc_ddrphy_apb_wr(0x540bc,0x0);
	dwc_ddrphy_apb_wr(0x540bd,0x0);
	dwc_ddrphy_apb_wr(0x540be,0x0);
	dwc_ddrphy_apb_wr(0x540bf,0x0);
	dwc_ddrphy_apb_wr(0x540c0,0x0);
	dwc_ddrphy_apb_wr(0x540c1,0x0);
	dwc_ddrphy_apb_wr(0x540c2,0x0);
	dwc_ddrphy_apb_wr(0x540c3,0x0);
	dwc_ddrphy_apb_wr(0x540c4,0x0);
	dwc_ddrphy_apb_wr(0x540c5,0x0);
	dwc_ddrphy_apb_wr(0x540c6,0x0);
	dwc_ddrphy_apb_wr(0x540c7,0x0);
	dwc_ddrphy_apb_wr(0x540c8,0x0);
	dwc_ddrphy_apb_wr(0x540c9,0x0);
	dwc_ddrphy_apb_wr(0x540ca,0x0);
	dwc_ddrphy_apb_wr(0x540cb,0x0);
	dwc_ddrphy_apb_wr(0x540cc,0x0);
	dwc_ddrphy_apb_wr(0x540cd,0x0);
	dwc_ddrphy_apb_wr(0x540ce,0x0);
	dwc_ddrphy_apb_wr(0x540cf,0x0);
	dwc_ddrphy_apb_wr(0x540d0,0x0);
	dwc_ddrphy_apb_wr(0x540d1,0x0);
	dwc_ddrphy_apb_wr(0x540d2,0x0);
	dwc_ddrphy_apb_wr(0x540d3,0x0);
	dwc_ddrphy_apb_wr(0x540d4,0x0);
	dwc_ddrphy_apb_wr(0x540d5,0x0);
	dwc_ddrphy_apb_wr(0x540d6,0x0);
	dwc_ddrphy_apb_wr(0x540d7,0x0);
	dwc_ddrphy_apb_wr(0x540d8,0x0);
	dwc_ddrphy_apb_wr(0x540d9,0x0);
	dwc_ddrphy_apb_wr(0x540da,0x0);
	dwc_ddrphy_apb_wr(0x540db,0x0);
	dwc_ddrphy_apb_wr(0x540dc,0x0);
	dwc_ddrphy_apb_wr(0x540dd,0x0);
	dwc_ddrphy_apb_wr(0x540de,0x0);
	dwc_ddrphy_apb_wr(0x540df,0x0);
	dwc_ddrphy_apb_wr(0x540e0,0x0);
	dwc_ddrphy_apb_wr(0x540e1,0x0);
	dwc_ddrphy_apb_wr(0x540e2,0x0);
	dwc_ddrphy_apb_wr(0x540e3,0x0);
	dwc_ddrphy_apb_wr(0x540e4,0x0);
	dwc_ddrphy_apb_wr(0x540e5,0x0);
	dwc_ddrphy_apb_wr(0x540e6,0x0);
	dwc_ddrphy_apb_wr(0x540e7,0x0);
	dwc_ddrphy_apb_wr(0x540e8,0x0);
	dwc_ddrphy_apb_wr(0x540e9,0x0);
	dwc_ddrphy_apb_wr(0x540ea,0x0);
	dwc_ddrphy_apb_wr(0x540eb,0x0);
	dwc_ddrphy_apb_wr(0x540ec,0x0);
	dwc_ddrphy_apb_wr(0x540ed,0x0);
	dwc_ddrphy_apb_wr(0x540ee,0x0);
	dwc_ddrphy_apb_wr(0x540ef,0x0);
	dwc_ddrphy_apb_wr(0x540f0,0x0);
	dwc_ddrphy_apb_wr(0x540f1,0x0);
	dwc_ddrphy_apb_wr(0x540f2,0x0);
	dwc_ddrphy_apb_wr(0x540f3,0x0);
	dwc_ddrphy_apb_wr(0x540f4,0x0);
	dwc_ddrphy_apb_wr(0x540f5,0x0);
	dwc_ddrphy_apb_wr(0x540f6,0x0);
	dwc_ddrphy_apb_wr(0x540f7,0x0);
	dwc_ddrphy_apb_wr(0x540f8,0x0);
	dwc_ddrphy_apb_wr(0x540f9,0x0);
	dwc_ddrphy_apb_wr(0x540fa,0x0);
	dwc_ddrphy_apb_wr(0x540fb,0x0);
	dwc_ddrphy_apb_wr(0x540fc,0x0);
	dwc_ddrphy_apb_wr(0x540fd,0x0);
	dwc_ddrphy_apb_wr(0x540fe,0x0);
	dwc_ddrphy_apb_wr(0x540ff,0x0);
	dwc_ddrphy_apb_wr(0x54100,0x0);
	dwc_ddrphy_apb_wr(0x54101,0x0);
	dwc_ddrphy_apb_wr(0x54102,0x0);
	dwc_ddrphy_apb_wr(0x54103,0x0);
	dwc_ddrphy_apb_wr(0x54104,0x0);
	dwc_ddrphy_apb_wr(0x54105,0x0);
	dwc_ddrphy_apb_wr(0x54106,0x0);
	dwc_ddrphy_apb_wr(0x54107,0x0);
	dwc_ddrphy_apb_wr(0x54108,0x0);
	dwc_ddrphy_apb_wr(0x54109,0x0);
	dwc_ddrphy_apb_wr(0x5410a,0x0);
	dwc_ddrphy_apb_wr(0x5410b,0x0);
	dwc_ddrphy_apb_wr(0x5410c,0x0);
	dwc_ddrphy_apb_wr(0x5410d,0x0);
	dwc_ddrphy_apb_wr(0x5410e,0x0);
	dwc_ddrphy_apb_wr(0x5410f,0x0);
	dwc_ddrphy_apb_wr(0x54110,0x0);
	dwc_ddrphy_apb_wr(0x54111,0x0);
	dwc_ddrphy_apb_wr(0x54112,0x0);
	dwc_ddrphy_apb_wr(0x54113,0x0);
	dwc_ddrphy_apb_wr(0x54114,0x0);
	dwc_ddrphy_apb_wr(0x54115,0x0);
	dwc_ddrphy_apb_wr(0x54116,0x0);
	dwc_ddrphy_apb_wr(0x54117,0x0);
	dwc_ddrphy_apb_wr(0x54118,0x0);
	dwc_ddrphy_apb_wr(0x54119,0x0);
	dwc_ddrphy_apb_wr(0x5411a,0x0);
	dwc_ddrphy_apb_wr(0x5411b,0x0);
	dwc_ddrphy_apb_wr(0x5411c,0x0);
	dwc_ddrphy_apb_wr(0x5411d,0x0);
	dwc_ddrphy_apb_wr(0x5411e,0x0);
	dwc_ddrphy_apb_wr(0x5411f,0x0);
	dwc_ddrphy_apb_wr(0x54120,0x0);
	dwc_ddrphy_apb_wr(0x54121,0x0);
	dwc_ddrphy_apb_wr(0x54122,0x0);
	dwc_ddrphy_apb_wr(0x54123,0x0);
	dwc_ddrphy_apb_wr(0x54124,0x0);
	dwc_ddrphy_apb_wr(0x54125,0x0);
	dwc_ddrphy_apb_wr(0x54126,0x0);
	dwc_ddrphy_apb_wr(0x54127,0x0);
	dwc_ddrphy_apb_wr(0x54128,0x0);
	dwc_ddrphy_apb_wr(0x54129,0x0);
	dwc_ddrphy_apb_wr(0x5412a,0x0);
	dwc_ddrphy_apb_wr(0x5412b,0x0);
	dwc_ddrphy_apb_wr(0x5412c,0x0);
	dwc_ddrphy_apb_wr(0x5412d,0x0);
	dwc_ddrphy_apb_wr(0x5412e,0x0);
	dwc_ddrphy_apb_wr(0x5412f,0x0);
	dwc_ddrphy_apb_wr(0x54130,0x0);
	dwc_ddrphy_apb_wr(0x54131,0x0);
	dwc_ddrphy_apb_wr(0x54132,0x0);
	dwc_ddrphy_apb_wr(0x54133,0x0);
	dwc_ddrphy_apb_wr(0x54134,0x0);
	dwc_ddrphy_apb_wr(0x54135,0x0);
	dwc_ddrphy_apb_wr(0x54136,0x0);
	dwc_ddrphy_apb_wr(0x54137,0x0);
	dwc_ddrphy_apb_wr(0x54138,0x0);
	dwc_ddrphy_apb_wr(0x54139,0x0);
	dwc_ddrphy_apb_wr(0x5413a,0x0);
	dwc_ddrphy_apb_wr(0x5413b,0x0);
	dwc_ddrphy_apb_wr(0x5413c,0x0);
	dwc_ddrphy_apb_wr(0x5413d,0x0);
	dwc_ddrphy_apb_wr(0x5413e,0x0);
	dwc_ddrphy_apb_wr(0x5413f,0x0);
	dwc_ddrphy_apb_wr(0x54140,0x0);
	dwc_ddrphy_apb_wr(0x54141,0x0);
	dwc_ddrphy_apb_wr(0x54142,0x0);
	dwc_ddrphy_apb_wr(0x54143,0x0);
	dwc_ddrphy_apb_wr(0x54144,0x0);
	dwc_ddrphy_apb_wr(0x54145,0x0);
	dwc_ddrphy_apb_wr(0x54146,0x0);
	dwc_ddrphy_apb_wr(0x54147,0x0);
	dwc_ddrphy_apb_wr(0x54148,0x0);
	dwc_ddrphy_apb_wr(0x54149,0x0);
	dwc_ddrphy_apb_wr(0x5414a,0x0);
	dwc_ddrphy_apb_wr(0x5414b,0x0);
	dwc_ddrphy_apb_wr(0x5414c,0x0);
	dwc_ddrphy_apb_wr(0x5414d,0x0);
	dwc_ddrphy_apb_wr(0x5414e,0x0);
	dwc_ddrphy_apb_wr(0x5414f,0x0);
	dwc_ddrphy_apb_wr(0x54150,0x0);
	dwc_ddrphy_apb_wr(0x54151,0x0);
	dwc_ddrphy_apb_wr(0x54152,0x0);
	dwc_ddrphy_apb_wr(0x54153,0x0);
	dwc_ddrphy_apb_wr(0x54154,0x0);
	dwc_ddrphy_apb_wr(0x54155,0x0);
	dwc_ddrphy_apb_wr(0x54156,0x0);
	dwc_ddrphy_apb_wr(0x54157,0x0);
	dwc_ddrphy_apb_wr(0x54158,0x0);
	dwc_ddrphy_apb_wr(0x54159,0x0);
	dwc_ddrphy_apb_wr(0x5415a,0x0);
	dwc_ddrphy_apb_wr(0x5415b,0x0);
	dwc_ddrphy_apb_wr(0x5415c,0x0);
	dwc_ddrphy_apb_wr(0x5415d,0x0);
	dwc_ddrphy_apb_wr(0x5415e,0x0);
	dwc_ddrphy_apb_wr(0x5415f,0x0);
	dwc_ddrphy_apb_wr(0x54160,0x0);
	dwc_ddrphy_apb_wr(0x54161,0x0);
	dwc_ddrphy_apb_wr(0x54162,0x0);
	dwc_ddrphy_apb_wr(0x54163,0x0);
	dwc_ddrphy_apb_wr(0x54164,0x0);
	dwc_ddrphy_apb_wr(0x54165,0x0);
	dwc_ddrphy_apb_wr(0x54166,0x0);
	dwc_ddrphy_apb_wr(0x54167,0x0);
	dwc_ddrphy_apb_wr(0x54168,0x0);
	dwc_ddrphy_apb_wr(0x54169,0x0);
	dwc_ddrphy_apb_wr(0x5416a,0x0);
	dwc_ddrphy_apb_wr(0x5416b,0x0);
	dwc_ddrphy_apb_wr(0x5416c,0x0);
	dwc_ddrphy_apb_wr(0x5416d,0x0);
	dwc_ddrphy_apb_wr(0x5416e,0x0);
	dwc_ddrphy_apb_wr(0x5416f,0x0);
	dwc_ddrphy_apb_wr(0x54170,0x0);
	dwc_ddrphy_apb_wr(0x54171,0x0);
	dwc_ddrphy_apb_wr(0x54172,0x0);
	dwc_ddrphy_apb_wr(0x54173,0x0);
	dwc_ddrphy_apb_wr(0x54174,0x0);
	dwc_ddrphy_apb_wr(0x54175,0x0);
	dwc_ddrphy_apb_wr(0x54176,0x0);
	dwc_ddrphy_apb_wr(0x54177,0x0);
	dwc_ddrphy_apb_wr(0x54178,0x0);
	dwc_ddrphy_apb_wr(0x54179,0x0);
	dwc_ddrphy_apb_wr(0x5417a,0x0);
	dwc_ddrphy_apb_wr(0x5417b,0x0);
	dwc_ddrphy_apb_wr(0x5417c,0x0);
	dwc_ddrphy_apb_wr(0x5417d,0x0);
	dwc_ddrphy_apb_wr(0x5417e,0x0);
	dwc_ddrphy_apb_wr(0x5417f,0x0);
	dwc_ddrphy_apb_wr(0x54180,0x0);
	dwc_ddrphy_apb_wr(0x54181,0x0);
	dwc_ddrphy_apb_wr(0x54182,0x0);
	dwc_ddrphy_apb_wr(0x54183,0x0);
	dwc_ddrphy_apb_wr(0x54184,0x0);
	dwc_ddrphy_apb_wr(0x54185,0x0);
	dwc_ddrphy_apb_wr(0x54186,0x0);
	dwc_ddrphy_apb_wr(0x54187,0x0);
	dwc_ddrphy_apb_wr(0x54188,0x0);
	dwc_ddrphy_apb_wr(0x54189,0x0);
	dwc_ddrphy_apb_wr(0x5418a,0x0);
	dwc_ddrphy_apb_wr(0x5418b,0x0);
	dwc_ddrphy_apb_wr(0x5418c,0x0);
	dwc_ddrphy_apb_wr(0x5418d,0x0);
	dwc_ddrphy_apb_wr(0x5418e,0x0);
	dwc_ddrphy_apb_wr(0x5418f,0x0);
	dwc_ddrphy_apb_wr(0x54190,0x0);
	dwc_ddrphy_apb_wr(0x54191,0x0);
	dwc_ddrphy_apb_wr(0x54192,0x0);
	dwc_ddrphy_apb_wr(0x54193,0x0);
	dwc_ddrphy_apb_wr(0x54194,0x0);
	dwc_ddrphy_apb_wr(0x54195,0x0);
	dwc_ddrphy_apb_wr(0x54196,0x0);
	dwc_ddrphy_apb_wr(0x54197,0x0);
	dwc_ddrphy_apb_wr(0x54198,0x0);
	dwc_ddrphy_apb_wr(0x54199,0x0);
	dwc_ddrphy_apb_wr(0x5419a,0x0);
	dwc_ddrphy_apb_wr(0x5419b,0x0);
	dwc_ddrphy_apb_wr(0x5419c,0x0);
	dwc_ddrphy_apb_wr(0x5419d,0x0);
	dwc_ddrphy_apb_wr(0x5419e,0x0);
	dwc_ddrphy_apb_wr(0x5419f,0x0);
	dwc_ddrphy_apb_wr(0x541a0,0x0);
	dwc_ddrphy_apb_wr(0x541a1,0x0);
	dwc_ddrphy_apb_wr(0x541a2,0x0);
	dwc_ddrphy_apb_wr(0x541a3,0x0);
	dwc_ddrphy_apb_wr(0x541a4,0x0);
	dwc_ddrphy_apb_wr(0x541a5,0x0);
	dwc_ddrphy_apb_wr(0x541a6,0x0);
	dwc_ddrphy_apb_wr(0x541a7,0x0);
	dwc_ddrphy_apb_wr(0x541a8,0x0);
	dwc_ddrphy_apb_wr(0x541a9,0x0);
	dwc_ddrphy_apb_wr(0x541aa,0x0);
	dwc_ddrphy_apb_wr(0x541ab,0x0);
	dwc_ddrphy_apb_wr(0x541ac,0x0);
	dwc_ddrphy_apb_wr(0x541ad,0x0);
	dwc_ddrphy_apb_wr(0x541ae,0x0);
	dwc_ddrphy_apb_wr(0x541af,0x0);
	dwc_ddrphy_apb_wr(0x541b0,0x0);
	dwc_ddrphy_apb_wr(0x541b1,0x0);
	dwc_ddrphy_apb_wr(0x541b2,0x0);
	dwc_ddrphy_apb_wr(0x541b3,0x0);
	dwc_ddrphy_apb_wr(0x541b4,0x0);
	dwc_ddrphy_apb_wr(0x541b5,0x0);
	dwc_ddrphy_apb_wr(0x541b6,0x0);
	dwc_ddrphy_apb_wr(0x541b7,0x0);
	dwc_ddrphy_apb_wr(0x541b8,0x0);
	dwc_ddrphy_apb_wr(0x541b9,0x0);
	dwc_ddrphy_apb_wr(0x541ba,0x0);
	dwc_ddrphy_apb_wr(0x541bb,0x0);
	dwc_ddrphy_apb_wr(0x541bc,0x0);
	dwc_ddrphy_apb_wr(0x541bd,0x0);
	dwc_ddrphy_apb_wr(0x541be,0x0);
	dwc_ddrphy_apb_wr(0x541bf,0x0);
	dwc_ddrphy_apb_wr(0x541c0,0x0);
	dwc_ddrphy_apb_wr(0x541c1,0x0);
	dwc_ddrphy_apb_wr(0x541c2,0x0);
	dwc_ddrphy_apb_wr(0x541c3,0x0);
	dwc_ddrphy_apb_wr(0x541c4,0x0);
	dwc_ddrphy_apb_wr(0x541c5,0x0);
	dwc_ddrphy_apb_wr(0x541c6,0x0);
	dwc_ddrphy_apb_wr(0x541c7,0x0);
	dwc_ddrphy_apb_wr(0x541c8,0x0);
	dwc_ddrphy_apb_wr(0x541c9,0x0);
	dwc_ddrphy_apb_wr(0x541ca,0x0);
	dwc_ddrphy_apb_wr(0x541cb,0x0);
	dwc_ddrphy_apb_wr(0x541cc,0x0);
	dwc_ddrphy_apb_wr(0x541cd,0x0);
	dwc_ddrphy_apb_wr(0x541ce,0x0);
	dwc_ddrphy_apb_wr(0x541cf,0x0);
	dwc_ddrphy_apb_wr(0x541d0,0x0);
	dwc_ddrphy_apb_wr(0x541d1,0x0);
	dwc_ddrphy_apb_wr(0x541d2,0x0);
	dwc_ddrphy_apb_wr(0x541d3,0x0);
	dwc_ddrphy_apb_wr(0x541d4,0x0);
	dwc_ddrphy_apb_wr(0x541d5,0x0);
	dwc_ddrphy_apb_wr(0x541d6,0x0);
	dwc_ddrphy_apb_wr(0x541d7,0x0);
	dwc_ddrphy_apb_wr(0x541d8,0x0);
	dwc_ddrphy_apb_wr(0x541d9,0x0);
	dwc_ddrphy_apb_wr(0x541da,0x0);
	dwc_ddrphy_apb_wr(0x541db,0x0);
	dwc_ddrphy_apb_wr(0x541dc,0x0);
	dwc_ddrphy_apb_wr(0x541dd,0x0);
	dwc_ddrphy_apb_wr(0x541de,0x0);
	dwc_ddrphy_apb_wr(0x541df,0x0);
	dwc_ddrphy_apb_wr(0x541e0,0x0);
	dwc_ddrphy_apb_wr(0x541e1,0x0);
	dwc_ddrphy_apb_wr(0x541e2,0x0);
	dwc_ddrphy_apb_wr(0x541e3,0x0);
	dwc_ddrphy_apb_wr(0x541e4,0x0);
	dwc_ddrphy_apb_wr(0x541e5,0x0);
	dwc_ddrphy_apb_wr(0x541e6,0x0);
	dwc_ddrphy_apb_wr(0x541e7,0x0);
	dwc_ddrphy_apb_wr(0x541e8,0x0);
	dwc_ddrphy_apb_wr(0x541e9,0x0);
	dwc_ddrphy_apb_wr(0x541ea,0x0);
	dwc_ddrphy_apb_wr(0x541eb,0x0);
	dwc_ddrphy_apb_wr(0x541ec,0x0);
	dwc_ddrphy_apb_wr(0x541ed,0x0);
	dwc_ddrphy_apb_wr(0x541ee,0x0);
	dwc_ddrphy_apb_wr(0x541ef,0x0);
	dwc_ddrphy_apb_wr(0x541f0,0x0);
	dwc_ddrphy_apb_wr(0x541f1,0x0);
	dwc_ddrphy_apb_wr(0x541f2,0x0);
	dwc_ddrphy_apb_wr(0x541f3,0x0);
	dwc_ddrphy_apb_wr(0x541f4,0x0);
	dwc_ddrphy_apb_wr(0x541f5,0x0);
	dwc_ddrphy_apb_wr(0x541f6,0x0);
	dwc_ddrphy_apb_wr(0x541f7,0x0);
	dwc_ddrphy_apb_wr(0x541f8,0x0);
	dwc_ddrphy_apb_wr(0x541f9,0x0);
	dwc_ddrphy_apb_wr(0x541fa,0x0);
	dwc_ddrphy_apb_wr(0x541fb,0x0);
	dwc_ddrphy_apb_wr(0x541fc,0x100);
	dwc_ddrphy_apb_wr(0xd0000,0x1); /* DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
	dwc_ddrphy_apb_wr(0xd0000,0x1); /* DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
	dwc_ddrphy_apb_wr(0xd0099,0x9); /* DWC_DDRPHYA_APBONLY0_MicroReset */
	dwc_ddrphy_apb_wr(0xd0099,0x1); /* DWC_DDRPHYA_APBONLY0_MicroReset */
	dwc_ddrphy_apb_wr(0xd0099,0x0); /* DWC_DDRPHYA_APBONLY0_MicroReset */

	wait_ddrphy_training_complete();

	dwc_ddrphy_apb_wr(0xd0099,0x1); /* DWC_DDRPHYA_APBONLY0_MicroReset */
	dwc_ddrphy_apb_wr(0xd0000,0x0); /* DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
	dwc_ddrphy_apb_wr(0xd0000,0x1); /* DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
	dwc_ddrphy_apb_wr(0xd0000,0x0); /* DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
	dwc_ddrphy_apb_wr(0x90000,0x10); /* DWC_DDRPHYA_INITENG0_PreSequenceReg0b0s0 */
	dwc_ddrphy_apb_wr(0x90001,0x400); /* DWC_DDRPHYA_INITENG0_PreSequenceReg0b0s1 */
	dwc_ddrphy_apb_wr(0x90002,0x10e); /* DWC_DDRPHYA_INITENG0_PreSequenceReg0b0s2 */
	dwc_ddrphy_apb_wr(0x90003,0x0); /* DWC_DDRPHYA_INITENG0_PreSequenceReg0b1s0 */
	dwc_ddrphy_apb_wr(0x90004,0x0); /* DWC_DDRPHYA_INITENG0_PreSequenceReg0b1s1 */
	dwc_ddrphy_apb_wr(0x90005,0x8); /* DWC_DDRPHYA_INITENG0_PreSequenceReg0b1s2 */
	dwc_ddrphy_apb_wr(0x90029,0xb); /* DWC_DDRPHYA_INITENG0_SequenceReg0b0s0 */
	dwc_ddrphy_apb_wr(0x9002a,0x480); /* DWC_DDRPHYA_INITENG0_SequenceReg0b0s1 */
	dwc_ddrphy_apb_wr(0x9002b,0x109); /* DWC_DDRPHYA_INITENG0_SequenceReg0b0s2 */
	dwc_ddrphy_apb_wr(0x9002c,0x8); /* DWC_DDRPHYA_INITENG0_SequenceReg0b1s0 */
	dwc_ddrphy_apb_wr(0x9002d,0x448); /* DWC_DDRPHYA_INITENG0_SequenceReg0b1s1 */
	dwc_ddrphy_apb_wr(0x9002e,0x139); /* DWC_DDRPHYA_INITENG0_SequenceReg0b1s2 */
	dwc_ddrphy_apb_wr(0x9002f,0x8); /* DWC_DDRPHYA_INITENG0_SequenceReg0b2s0 */
	dwc_ddrphy_apb_wr(0x90030,0x478); /* DWC_DDRPHYA_INITENG0_SequenceReg0b2s1 */
	dwc_ddrphy_apb_wr(0x90031,0x109); /* DWC_DDRPHYA_INITENG0_SequenceReg0b2s2 */
	dwc_ddrphy_apb_wr(0x90032,0x2); /* DWC_DDRPHYA_INITENG0_SequenceReg0b3s0 */
	dwc_ddrphy_apb_wr(0x90033,0x10); /* DWC_DDRPHYA_INITENG0_SequenceReg0b3s1 */
	dwc_ddrphy_apb_wr(0x90034,0x139); /* DWC_DDRPHYA_INITENG0_SequenceReg0b3s2 */
	dwc_ddrphy_apb_wr(0x90035,0xf); /* DWC_DDRPHYA_INITENG0_SequenceReg0b4s0 */
	dwc_ddrphy_apb_wr(0x90036,0x7c0); /* DWC_DDRPHYA_INITENG0_SequenceReg0b4s1 */
	dwc_ddrphy_apb_wr(0x90037,0x139); /* DWC_DDRPHYA_INITENG0_SequenceReg0b4s2 */
	dwc_ddrphy_apb_wr(0x90038,0x44); /* DWC_DDRPHYA_INITENG0_SequenceReg0b5s0 */
	dwc_ddrphy_apb_wr(0x90039,0x630); /* DWC_DDRPHYA_INITENG0_SequenceReg0b5s1 */
	dwc_ddrphy_apb_wr(0x9003a,0x159); /* DWC_DDRPHYA_INITENG0_SequenceReg0b5s2 */
	dwc_ddrphy_apb_wr(0x9003b,0x14f); /* DWC_DDRPHYA_INITENG0_SequenceReg0b6s0 */
	dwc_ddrphy_apb_wr(0x9003c,0x630); /* DWC_DDRPHYA_INITENG0_SequenceReg0b6s1 */
	dwc_ddrphy_apb_wr(0x9003d,0x159); /* DWC_DDRPHYA_INITENG0_SequenceReg0b6s2 */
	dwc_ddrphy_apb_wr(0x9003e,0x47); /* DWC_DDRPHYA_INITENG0_SequenceReg0b7s0 */
	dwc_ddrphy_apb_wr(0x9003f,0x630); /* DWC_DDRPHYA_INITENG0_SequenceReg0b7s1 */
	dwc_ddrphy_apb_wr(0x90040,0x149); /* DWC_DDRPHYA_INITENG0_SequenceReg0b7s2 */
	dwc_ddrphy_apb_wr(0x90041,0x4f); /* DWC_DDRPHYA_INITENG0_SequenceReg0b8s0 */
	dwc_ddrphy_apb_wr(0x90042,0x630); /* DWC_DDRPHYA_INITENG0_SequenceReg0b8s1 */
	dwc_ddrphy_apb_wr(0x90043,0x179); /* DWC_DDRPHYA_INITENG0_SequenceReg0b8s2 */
	dwc_ddrphy_apb_wr(0x90044,0x8); /* DWC_DDRPHYA_INITENG0_SequenceReg0b9s0 */
	dwc_ddrphy_apb_wr(0x90045,0xe0); /* DWC_DDRPHYA_INITENG0_SequenceReg0b9s1 */
	dwc_ddrphy_apb_wr(0x90046,0x109); /* DWC_DDRPHYA_INITENG0_SequenceReg0b9s2 */
	dwc_ddrphy_apb_wr(0x90047,0x0); /* DWC_DDRPHYA_INITENG0_SequenceReg0b10s0 */
	dwc_ddrphy_apb_wr(0x90048,0x7c8); /* DWC_DDRPHYA_INITENG0_SequenceReg0b10s1 */
	dwc_ddrphy_apb_wr(0x90049,0x109); /* DWC_DDRPHYA_INITENG0_SequenceReg0b10s2 */
	dwc_ddrphy_apb_wr(0x9004a,0x0); /* DWC_DDRPHYA_INITENG0_SequenceReg0b11s0 */
	dwc_ddrphy_apb_wr(0x9004b,0x1); /* DWC_DDRPHYA_INITENG0_SequenceReg0b11s1 */
	dwc_ddrphy_apb_wr(0x9004c,0x8); /* DWC_DDRPHYA_INITENG0_SequenceReg0b11s2 */
	dwc_ddrphy_apb_wr(0x9004d,0x0); /* DWC_DDRPHYA_INITENG0_SequenceReg0b12s0 */
	dwc_ddrphy_apb_wr(0x9004e,0x45a); /* DWC_DDRPHYA_INITENG0_SequenceReg0b12s1 */
	dwc_ddrphy_apb_wr(0x9004f,0x9); /* DWC_DDRPHYA_INITENG0_SequenceReg0b12s2 */
	dwc_ddrphy_apb_wr(0x90050,0x0); /* DWC_DDRPHYA_INITENG0_SequenceReg0b13s0 */
	dwc_ddrphy_apb_wr(0x90051,0x448); /* DWC_DDRPHYA_INITENG0_SequenceReg0b13s1 */
	dwc_ddrphy_apb_wr(0x90052,0x109); /* DWC_DDRPHYA_INITENG0_SequenceReg0b13s2 */
	dwc_ddrphy_apb_wr(0x90053,0x40); /* DWC_DDRPHYA_INITENG0_SequenceReg0b14s0 */
	dwc_ddrphy_apb_wr(0x90054,0x630); /* DWC_DDRPHYA_INITENG0_SequenceReg0b14s1 */
	dwc_ddrphy_apb_wr(0x90055,0x179); /* DWC_DDRPHYA_INITENG0_SequenceReg0b14s2 */
	dwc_ddrphy_apb_wr(0x90056,0x1); /* DWC_DDRPHYA_INITENG0_SequenceReg0b15s0 */
	dwc_ddrphy_apb_wr(0x90057,0x618); /* DWC_DDRPHYA_INITENG0_SequenceReg0b15s1 */
	dwc_ddrphy_apb_wr(0x90058,0x109); /* DWC_DDRPHYA_INITENG0_SequenceReg0b15s2 */
	dwc_ddrphy_apb_wr(0x90059,0x40c0); /* DWC_DDRPHYA_INITENG0_SequenceReg0b16s0 */
	dwc_ddrphy_apb_wr(0x9005a,0x630); /* DWC_DDRPHYA_INITENG0_SequenceReg0b16s1 */
	dwc_ddrphy_apb_wr(0x9005b,0x149); /* DWC_DDRPHYA_INITENG0_SequenceReg0b16s2 */
	dwc_ddrphy_apb_wr(0x9005c,0x8); /* DWC_DDRPHYA_INITENG0_SequenceReg0b17s0 */
	dwc_ddrphy_apb_wr(0x9005d,0x4); /* DWC_DDRPHYA_INITENG0_SequenceReg0b17s1 */
	dwc_ddrphy_apb_wr(0x9005e,0x48); /* DWC_DDRPHYA_INITENG0_SequenceReg0b17s2 */
	dwc_ddrphy_apb_wr(0x9005f,0x4040); /* DWC_DDRPHYA_INITENG0_SequenceReg0b18s0 */
	dwc_ddrphy_apb_wr(0x90060,0x630); /* DWC_DDRPHYA_INITENG0_SequenceReg0b18s1 */
	dwc_ddrphy_apb_wr(0x90061,0x149); /* DWC_DDRPHYA_INITENG0_SequenceReg0b18s2 */
	dwc_ddrphy_apb_wr(0x90062,0x0); /* DWC_DDRPHYA_INITENG0_SequenceReg0b19s0 */
	dwc_ddrphy_apb_wr(0x90063,0x4); /* DWC_DDRPHYA_INITENG0_SequenceReg0b19s1 */
	dwc_ddrphy_apb_wr(0x90064,0x48); /* DWC_DDRPHYA_INITENG0_SequenceReg0b19s2 */
	dwc_ddrphy_apb_wr(0x90065,0x40); /* DWC_DDRPHYA_INITENG0_SequenceReg0b20s0 */
	dwc_ddrphy_apb_wr(0x90066,0x630); /* DWC_DDRPHYA_INITENG0_SequenceReg0b20s1 */
	dwc_ddrphy_apb_wr(0x90067,0x149); /* DWC_DDRPHYA_INITENG0_SequenceReg0b20s2 */
	dwc_ddrphy_apb_wr(0x90068,0x10); /* DWC_DDRPHYA_INITENG0_SequenceReg0b21s0 */
	dwc_ddrphy_apb_wr(0x90069,0x4); /* DWC_DDRPHYA_INITENG0_SequenceReg0b21s1 */
	dwc_ddrphy_apb_wr(0x9006a,0x18); /* DWC_DDRPHYA_INITENG0_SequenceReg0b21s2 */
	dwc_ddrphy_apb_wr(0x9006b,0x0); /* DWC_DDRPHYA_INITENG0_SequenceReg0b22s0 */
	dwc_ddrphy_apb_wr(0x9006c,0x4); /* DWC_DDRPHYA_INITENG0_SequenceReg0b22s1 */
	dwc_ddrphy_apb_wr(0x9006d,0x78); /* DWC_DDRPHYA_INITENG0_SequenceReg0b22s2 */
	dwc_ddrphy_apb_wr(0x9006e,0x549); /* DWC_DDRPHYA_INITENG0_SequenceReg0b23s0 */
	dwc_ddrphy_apb_wr(0x9006f,0x630); /* DWC_DDRPHYA_INITENG0_SequenceReg0b23s1 */
	dwc_ddrphy_apb_wr(0x90070,0x159); /* DWC_DDRPHYA_INITENG0_SequenceReg0b23s2 */
	dwc_ddrphy_apb_wr(0x90071,0xd49); /* DWC_DDRPHYA_INITENG0_SequenceReg0b24s0 */
	dwc_ddrphy_apb_wr(0x90072,0x630); /* DWC_DDRPHYA_INITENG0_SequenceReg0b24s1 */
	dwc_ddrphy_apb_wr(0x90073,0x159); /* DWC_DDRPHYA_INITENG0_SequenceReg0b24s2 */
	dwc_ddrphy_apb_wr(0x90074,0x94a); /* DWC_DDRPHYA_INITENG0_SequenceReg0b25s0 */
	dwc_ddrphy_apb_wr(0x90075,0x630); /* DWC_DDRPHYA_INITENG0_SequenceReg0b25s1 */
	dwc_ddrphy_apb_wr(0x90076,0x159); /* DWC_DDRPHYA_INITENG0_SequenceReg0b25s2 */
	dwc_ddrphy_apb_wr(0x90077,0x441); /* DWC_DDRPHYA_INITENG0_SequenceReg0b26s0 */
	dwc_ddrphy_apb_wr(0x90078,0x630); /* DWC_DDRPHYA_INITENG0_SequenceReg0b26s1 */
	dwc_ddrphy_apb_wr(0x90079,0x149); /* DWC_DDRPHYA_INITENG0_SequenceReg0b26s2 */
	dwc_ddrphy_apb_wr(0x9007a,0x42); /* DWC_DDRPHYA_INITENG0_SequenceReg0b27s0 */
	dwc_ddrphy_apb_wr(0x9007b,0x630); /* DWC_DDRPHYA_INITENG0_SequenceReg0b27s1 */
	dwc_ddrphy_apb_wr(0x9007c,0x149); /* DWC_DDRPHYA_INITENG0_SequenceReg0b27s2 */
	dwc_ddrphy_apb_wr(0x9007d,0x1); /* DWC_DDRPHYA_INITENG0_SequenceReg0b28s0 */
	dwc_ddrphy_apb_wr(0x9007e,0x630); /* DWC_DDRPHYA_INITENG0_SequenceReg0b28s1 */
	dwc_ddrphy_apb_wr(0x9007f,0x149); /* DWC_DDRPHYA_INITENG0_SequenceReg0b28s2 */
	dwc_ddrphy_apb_wr(0x90080,0x0); /* DWC_DDRPHYA_INITENG0_SequenceReg0b29s0 */
	dwc_ddrphy_apb_wr(0x90081,0xe0); /* DWC_DDRPHYA_INITENG0_SequenceReg0b29s1 */
	dwc_ddrphy_apb_wr(0x90082,0x109); /* DWC_DDRPHYA_INITENG0_SequenceReg0b29s2 */
	dwc_ddrphy_apb_wr(0x90083,0xa); /* DWC_DDRPHYA_INITENG0_SequenceReg0b30s0 */
	dwc_ddrphy_apb_wr(0x90084,0x10); /* DWC_DDRPHYA_INITENG0_SequenceReg0b30s1 */
	dwc_ddrphy_apb_wr(0x90085,0x109); /* DWC_DDRPHYA_INITENG0_SequenceReg0b30s2 */
	dwc_ddrphy_apb_wr(0x90086,0x9); /* DWC_DDRPHYA_INITENG0_SequenceReg0b31s0 */
	dwc_ddrphy_apb_wr(0x90087,0x3c0); /* DWC_DDRPHYA_INITENG0_SequenceReg0b31s1 */
	dwc_ddrphy_apb_wr(0x90088,0x149); /* DWC_DDRPHYA_INITENG0_SequenceReg0b31s2 */
	dwc_ddrphy_apb_wr(0x90089,0x9); /* DWC_DDRPHYA_INITENG0_SequenceReg0b32s0 */
	dwc_ddrphy_apb_wr(0x9008a,0x3c0); /* DWC_DDRPHYA_INITENG0_SequenceReg0b32s1 */
	dwc_ddrphy_apb_wr(0x9008b,0x159); /* DWC_DDRPHYA_INITENG0_SequenceReg0b32s2 */
	dwc_ddrphy_apb_wr(0x9008c,0x18); /* DWC_DDRPHYA_INITENG0_SequenceReg0b33s0 */
	dwc_ddrphy_apb_wr(0x9008d,0x10); /* DWC_DDRPHYA_INITENG0_SequenceReg0b33s1 */
	dwc_ddrphy_apb_wr(0x9008e,0x109); /* DWC_DDRPHYA_INITENG0_SequenceReg0b33s2 */
	dwc_ddrphy_apb_wr(0x9008f,0x0); /* DWC_DDRPHYA_INITENG0_SequenceReg0b34s0 */
	dwc_ddrphy_apb_wr(0x90090,0x3c0); /* DWC_DDRPHYA_INITENG0_SequenceReg0b34s1 */
	dwc_ddrphy_apb_wr(0x90091,0x109); /* DWC_DDRPHYA_INITENG0_SequenceReg0b34s2 */
	dwc_ddrphy_apb_wr(0x90092,0x18); /* DWC_DDRPHYA_INITENG0_SequenceReg0b35s0 */
	dwc_ddrphy_apb_wr(0x90093,0x4); /* DWC_DDRPHYA_INITENG0_SequenceReg0b35s1 */
	dwc_ddrphy_apb_wr(0x90094,0x48); /* DWC_DDRPHYA_INITENG0_SequenceReg0b35s2 */
	dwc_ddrphy_apb_wr(0x90095,0x18); /* DWC_DDRPHYA_INITENG0_SequenceReg0b36s0 */
	dwc_ddrphy_apb_wr(0x90096,0x4); /* DWC_DDRPHYA_INITENG0_SequenceReg0b36s1 */
	dwc_ddrphy_apb_wr(0x90097,0x58); /* DWC_DDRPHYA_INITENG0_SequenceReg0b36s2 */
	dwc_ddrphy_apb_wr(0x90098,0xa); /* DWC_DDRPHYA_INITENG0_SequenceReg0b37s0 */
	dwc_ddrphy_apb_wr(0x90099,0x10); /* DWC_DDRPHYA_INITENG0_SequenceReg0b37s1 */
	dwc_ddrphy_apb_wr(0x9009a,0x109); /* DWC_DDRPHYA_INITENG0_SequenceReg0b37s2 */
	dwc_ddrphy_apb_wr(0x9009b,0x2); /* DWC_DDRPHYA_INITENG0_SequenceReg0b38s0 */
	dwc_ddrphy_apb_wr(0x9009c,0x10); /* DWC_DDRPHYA_INITENG0_SequenceReg0b38s1 */
	dwc_ddrphy_apb_wr(0x9009d,0x109); /* DWC_DDRPHYA_INITENG0_SequenceReg0b38s2 */
	dwc_ddrphy_apb_wr(0x9009e,0x7); /* DWC_DDRPHYA_INITENG0_SequenceReg0b39s0 */
	dwc_ddrphy_apb_wr(0x9009f,0x7c0); /* DWC_DDRPHYA_INITENG0_SequenceReg0b39s1 */
	dwc_ddrphy_apb_wr(0x900a0,0x109); /* DWC_DDRPHYA_INITENG0_SequenceReg0b39s2 */
	dwc_ddrphy_apb_wr(0x900a1,0x10); /* DWC_DDRPHYA_INITENG0_SequenceReg0b40s0 */
	dwc_ddrphy_apb_wr(0x900a2,0x10); /* DWC_DDRPHYA_INITENG0_SequenceReg0b40s1 */
	dwc_ddrphy_apb_wr(0x900a3,0x109); /* DWC_DDRPHYA_INITENG0_SequenceReg0b40s2 */
	dwc_ddrphy_apb_wr(0x900a4,0x0); /* DWC_DDRPHYA_INITENG0_SequenceReg0b41s0 */
	dwc_ddrphy_apb_wr(0x900a5,0x8140); /* DWC_DDRPHYA_INITENG0_SequenceReg0b41s1 */
	dwc_ddrphy_apb_wr(0x900a6,0x10c); /* DWC_DDRPHYA_INITENG0_SequenceReg0b41s2 */
	dwc_ddrphy_apb_wr(0x900a7,0x10); /* DWC_DDRPHYA_INITENG0_SequenceReg0b42s0 */
	dwc_ddrphy_apb_wr(0x900a8,0x8138); /* DWC_DDRPHYA_INITENG0_SequenceReg0b42s1 */
	dwc_ddrphy_apb_wr(0x900a9,0x10c); /* DWC_DDRPHYA_INITENG0_SequenceReg0b42s2 */
	dwc_ddrphy_apb_wr(0x900aa,0x8); /* DWC_DDRPHYA_INITENG0_SequenceReg0b43s0 */
	dwc_ddrphy_apb_wr(0x900ab,0x7c8); /* DWC_DDRPHYA_INITENG0_SequenceReg0b43s1 */
	dwc_ddrphy_apb_wr(0x900ac,0x101); /* DWC_DDRPHYA_INITENG0_SequenceReg0b43s2 */
	dwc_ddrphy_apb_wr(0x900ad,0x8); /* DWC_DDRPHYA_INITENG0_SequenceReg0b44s0 */
	dwc_ddrphy_apb_wr(0x900ae,0x0); /* DWC_DDRPHYA_INITENG0_SequenceReg0b44s1 */
	dwc_ddrphy_apb_wr(0x900af,0x8); /* DWC_DDRPHYA_INITENG0_SequenceReg0b44s2 */
	dwc_ddrphy_apb_wr(0x900b0,0x8); /* DWC_DDRPHYA_INITENG0_SequenceReg0b45s0 */
	dwc_ddrphy_apb_wr(0x900b1,0x448); /* DWC_DDRPHYA_INITENG0_SequenceReg0b45s1 */
	dwc_ddrphy_apb_wr(0x900b2,0x109); /* DWC_DDRPHYA_INITENG0_SequenceReg0b45s2 */
	dwc_ddrphy_apb_wr(0x900b3,0xf); /* DWC_DDRPHYA_INITENG0_SequenceReg0b46s0 */
	dwc_ddrphy_apb_wr(0x900b4,0x7c0); /* DWC_DDRPHYA_INITENG0_SequenceReg0b46s1 */
	dwc_ddrphy_apb_wr(0x900b5,0x109); /* DWC_DDRPHYA_INITENG0_SequenceReg0b46s2 */
	dwc_ddrphy_apb_wr(0x900b6,0x47); /* DWC_DDRPHYA_INITENG0_SequenceReg0b47s0 */
	dwc_ddrphy_apb_wr(0x900b7,0x630); /* DWC_DDRPHYA_INITENG0_SequenceReg0b47s1 */
	dwc_ddrphy_apb_wr(0x900b8,0x109); /* DWC_DDRPHYA_INITENG0_SequenceReg0b47s2 */
	dwc_ddrphy_apb_wr(0x900b9,0x8); /* DWC_DDRPHYA_INITENG0_SequenceReg0b48s0 */
	dwc_ddrphy_apb_wr(0x900ba,0x618); /* DWC_DDRPHYA_INITENG0_SequenceReg0b48s1 */
	dwc_ddrphy_apb_wr(0x900bb,0x109); /* DWC_DDRPHYA_INITENG0_SequenceReg0b48s2 */
	dwc_ddrphy_apb_wr(0x900bc,0x8); /* DWC_DDRPHYA_INITENG0_SequenceReg0b49s0 */
	dwc_ddrphy_apb_wr(0x900bd,0xe0); /* DWC_DDRPHYA_INITENG0_SequenceReg0b49s1 */
	dwc_ddrphy_apb_wr(0x900be,0x109); /* DWC_DDRPHYA_INITENG0_SequenceReg0b49s2 */
	dwc_ddrphy_apb_wr(0x900bf,0x0); /* DWC_DDRPHYA_INITENG0_SequenceReg0b50s0 */
	dwc_ddrphy_apb_wr(0x900c0,0x7c8); /* DWC_DDRPHYA_INITENG0_SequenceReg0b50s1 */
	dwc_ddrphy_apb_wr(0x900c1,0x109); /* DWC_DDRPHYA_INITENG0_SequenceReg0b50s2 */
	dwc_ddrphy_apb_wr(0x900c2,0x8); /* DWC_DDRPHYA_INITENG0_SequenceReg0b51s0 */
	dwc_ddrphy_apb_wr(0x900c3,0x8140); /* DWC_DDRPHYA_INITENG0_SequenceReg0b51s1 */
	dwc_ddrphy_apb_wr(0x900c4,0x10c); /* DWC_DDRPHYA_INITENG0_SequenceReg0b51s2 */
	dwc_ddrphy_apb_wr(0x900c5,0x0); /* DWC_DDRPHYA_INITENG0_SequenceReg0b52s0 */
	dwc_ddrphy_apb_wr(0x900c6,0x1); /* DWC_DDRPHYA_INITENG0_SequenceReg0b52s1 */
	dwc_ddrphy_apb_wr(0x900c7,0x8); /* DWC_DDRPHYA_INITENG0_SequenceReg0b52s2 */
	dwc_ddrphy_apb_wr(0x900c8,0x8); /* DWC_DDRPHYA_INITENG0_SequenceReg0b53s0 */
	dwc_ddrphy_apb_wr(0x900c9,0x4); /* DWC_DDRPHYA_INITENG0_SequenceReg0b53s1 */
	dwc_ddrphy_apb_wr(0x900ca,0x8); /* DWC_DDRPHYA_INITENG0_SequenceReg0b53s2 */
	dwc_ddrphy_apb_wr(0x900cb,0x8); /* DWC_DDRPHYA_INITENG0_SequenceReg0b54s0 */
	dwc_ddrphy_apb_wr(0x900cc,0x7c8); /* DWC_DDRPHYA_INITENG0_SequenceReg0b54s1 */
	dwc_ddrphy_apb_wr(0x900cd,0x101); /* DWC_DDRPHYA_INITENG0_SequenceReg0b54s2 */
	dwc_ddrphy_apb_wr(0x90006,0x0); /* DWC_DDRPHYA_INITENG0_PostSequenceReg0b0s0 */
	dwc_ddrphy_apb_wr(0x90007,0x0); /* DWC_DDRPHYA_INITENG0_PostSequenceReg0b0s1 */
	dwc_ddrphy_apb_wr(0x90008,0x8); /* DWC_DDRPHYA_INITENG0_PostSequenceReg0b0s2 */
	dwc_ddrphy_apb_wr(0x90009,0x0); /* DWC_DDRPHYA_INITENG0_PostSequenceReg0b1s0 */
	dwc_ddrphy_apb_wr(0x9000a,0x0); /* DWC_DDRPHYA_INITENG0_PostSequenceReg0b1s1 */
	dwc_ddrphy_apb_wr(0x9000b,0x0); /* DWC_DDRPHYA_INITENG0_PostSequenceReg0b1s2 */
	dwc_ddrphy_apb_wr(0xd00e7,0x400); /* DWC_DDRPHYA_APBONLY0_SequencerOverride */
	dwc_ddrphy_apb_wr(0x90017,0x0); /* DWC_DDRPHYA_INITENG0_StartVector0b0 */
	dwc_ddrphy_apb_wr(0x90026,0x2c); /* DWC_DDRPHYA_INITENG0_StartVector0b15 */
	dwc_ddrphy_apb_wr(0x9000c,0x0); /* DWC_DDRPHYA_INITENG0_Seq0BDisableFlag0 */
	dwc_ddrphy_apb_wr(0x9000d,0x173); /* DWC_DDRPHYA_INITENG0_Seq0BDisableFlag1 */
	dwc_ddrphy_apb_wr(0x9000e,0x60); /* DWC_DDRPHYA_INITENG0_Seq0BDisableFlag2 */
	dwc_ddrphy_apb_wr(0x9000f,0x6110); /* DWC_DDRPHYA_INITENG0_Seq0BDisableFlag3 */
	dwc_ddrphy_apb_wr(0x90010,0x2152); /* DWC_DDRPHYA_INITENG0_Seq0BDisableFlag4 */
	dwc_ddrphy_apb_wr(0x90011,0xdfbd); /* DWC_DDRPHYA_INITENG0_Seq0BDisableFlag5 */
	dwc_ddrphy_apb_wr(0x90012,0xffff); /* DWC_DDRPHYA_INITENG0_Seq0BDisableFlag6 */
	dwc_ddrphy_apb_wr(0x90013,0x6152); /* DWC_DDRPHYA_INITENG0_Seq0BDisableFlag7 */
	dwc_ddrphy_apb_wr(0xc0080,0x0); /* DWC_DDRPHYA_DRTUB0_UcclkHclkEnables */
	dwc_ddrphy_apb_wr(0xd0000,0x1); /* DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
}
