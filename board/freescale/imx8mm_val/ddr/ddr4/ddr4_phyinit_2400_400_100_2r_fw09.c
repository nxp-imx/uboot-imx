/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <common.h>
#include <errno.h>
#include <asm/io.h>
#include <asm/arch/ddr_memory_map.h>
#include <asm/arch/clock.h>
#include "ddr4_define.h"

extern unsigned int after_retention;
extern unsigned int mr_value[3][7];

void ddr4_phyinit_train_sw_ffc(unsigned int Train2D)
{
	/*  [dwc_ddrphy_phyinit_main] Start of dwc_ddrphy_phyinit_main() */
	/*  [dwc_ddrphy_phyinit_sequence] Start of dwc_ddrphy_phyinit_sequence() */
	/*  [dwc_ddrphy_phyinit_initStruct] Start of dwc_ddrphy_phyinit_initStruct() */
	/*  [dwc_ddrphy_phyinit_initStruct] End of dwc_ddrphy_phyinit_initStruct() */
	/*  [dwc_ddrphy_phyinit_setDefault] Start of dwc_ddrphy_phyinit_setDefault() */
	/*  [dwc_ddrphy_phyinit_setDefault] End of dwc_ddrphy_phyinit_setDefault() */


	/*  ############################################################## */
	/*  */
	/*   dwc_ddrphy_phyinit_userCustom_overrideUserInput is a user-editable function. */
	/*   */
	/*   See PhyInit App Note for detailed description and function usage */
	/*  */
	/*  ############################################################## */

	dwc_ddrphy_phyinit_userCustom_overrideUserInput ();
	/*  */
	/*   [dwc_ddrphy_phyinit_userCustom_overrideUserInput] End of dwc_ddrphy_phyinit_userCustom_overrideUserInput() */


	/*  ############################################################## */
	/*   */
	/*   Step (A) : Bring up VDD, VDDQ, and VAA */
	/*   */
	/*   See PhyInit App Note for detailed description and function usage */
	/*   */
	/*  ############################################################## */


	dwc_ddrphy_phyinit_userCustom_A_bringupPower ();

	/*  [dwc_ddrphy_phyinit_userCustom_A_bringupPower] End of dwc_ddrphy_phyinit_userCustom_A_bringupPower() */
	/*  [dwc_ddrphy_phyinit_userCustom_B_startClockResetPhy] Start of dwc_ddrphy_phyinit_userCustom_B_startClockResetPhy() */
	/*  */
	/*  */
	/*  ############################################################## */
	/*   */
	/*   Step (B) Start Clocks and Reset the PHY */
	/*   */
	/*   See PhyInit App Note for detailed description and function usage */
	/*   */
	/*  ############################################################## */
	/*  */
	/*  */
	dwc_ddrphy_phyinit_userCustom_B_startClockResetPhy ();

	/*  [dwc_ddrphy_phyinit_userCustom_B_startClockResetPhy] End of dwc_ddrphy_phyinit_userCustom_B_startClockResetPhy() */
	/*  */

	/*  ############################################################## */
	/*   */
	/*   Step (C) Initialize PHY Configuration */
	/*   */
	/*   Load the required PHY configuration registers for the appropriate mode and memory configuration */
	/*   */
	/*  ############################################################## */
	/*  */

	/*   [phyinit_C_initPhyConfig] Start of dwc_ddrphy_phyinit_C_initPhyConfig() */
	/*  */
	/*  ############################################################## */
	/*   TxPreDrvMode[2] = 0 */
	/*  ############################################################## */
	/*   [phyinit_C_initPhyConfig] Pstate=0, Memclk=1200MHz, Programming TxSlewRate::TxPreDrvMode to 0x2 */
	/*   [phyinit_C_initPhyConfig] Pstate=0, Memclk=1200MHz, Programming TxSlewRate::TxPreP to 0xd */
	/*   [phyinit_C_initPhyConfig] Pstate=0, Memclk=1200MHz, Programming TxSlewRate::TxPreN to 0xf */
	/*   [phyinit_C_initPhyConfig] ### NOTE ### Optimal setting for TxSlewRate::TxPreP and TxSlewRate::TxPreP are technology specific. */
	/*   [phyinit_C_initPhyConfig] ### NOTE ### Please consult the "Output Slew Rate" section of HSpice Model App Note in specific technology for recommended settings */

	dwc_ddrphy_apb_wr(0x1005f, 0x2fd); /*  DWC_DDRPHYA_DBYTE0_TxSlewRate_b0_p0 */
	dwc_ddrphy_apb_wr(0x1015f, 0x2fd); /*  DWC_DDRPHYA_DBYTE0_TxSlewRate_b1_p0 */
	dwc_ddrphy_apb_wr(0x1105f, 0x2fd); /*  DWC_DDRPHYA_DBYTE1_TxSlewRate_b0_p0 */
	dwc_ddrphy_apb_wr(0x1115f, 0x2fd); /*  DWC_DDRPHYA_DBYTE1_TxSlewRate_b1_p0 */
	dwc_ddrphy_apb_wr(0x1205f, 0x2fd); /*  DWC_DDRPHYA_DBYTE2_TxSlewRate_b0_p0 */
	dwc_ddrphy_apb_wr(0x1215f, 0x2fd); /*  DWC_DDRPHYA_DBYTE2_TxSlewRate_b1_p0 */
	dwc_ddrphy_apb_wr(0x1305f, 0x2fd); /*  DWC_DDRPHYA_DBYTE3_TxSlewRate_b0_p0 */
	dwc_ddrphy_apb_wr(0x1315f, 0x2fd); /*  DWC_DDRPHYA_DBYTE3_TxSlewRate_b1_p0 */
	/*   [phyinit_C_initPhyConfig] Pstate=1, Memclk=200MHz, Programming TxSlewRate::TxPreDrvMode to 0x2 */
	/*   [phyinit_C_initPhyConfig] Pstate=1, Memclk=200MHz, Programming TxSlewRate::TxPreP to 0xd */
	/*   [phyinit_C_initPhyConfig] Pstate=1, Memclk=200MHz, Programming TxSlewRate::TxPreN to 0xf */
	/*   [phyinit_C_initPhyConfig] ### NOTE ### Optimal setting for TxSlewRate::TxPreP and TxSlewRate::TxPreP are technology specific. */
	/*   [phyinit_C_initPhyConfig] ### NOTE ### Please consult the "Output Slew Rate" section of HSpice Model App Note in specific technology for recommended settings */

	dwc_ddrphy_apb_wr(0x11005f, 0x2fd); /*  DWC_DDRPHYA_DBYTE0_TxSlewRate_b0_p1 */
	dwc_ddrphy_apb_wr(0x11015f, 0x2fd); /*  DWC_DDRPHYA_DBYTE0_TxSlewRate_b1_p1 */
	dwc_ddrphy_apb_wr(0x11105f, 0x2fd); /*  DWC_DDRPHYA_DBYTE1_TxSlewRate_b0_p1 */
	dwc_ddrphy_apb_wr(0x11115f, 0x2fd); /*  DWC_DDRPHYA_DBYTE1_TxSlewRate_b1_p1 */
	dwc_ddrphy_apb_wr(0x11205f, 0x2fd); /*  DWC_DDRPHYA_DBYTE2_TxSlewRate_b0_p1 */
	dwc_ddrphy_apb_wr(0x11215f, 0x2fd); /*  DWC_DDRPHYA_DBYTE2_TxSlewRate_b1_p1 */
	dwc_ddrphy_apb_wr(0x11305f, 0x2fd); /*  DWC_DDRPHYA_DBYTE3_TxSlewRate_b0_p1 */
	dwc_ddrphy_apb_wr(0x11315f, 0x2fd); /*  DWC_DDRPHYA_DBYTE3_TxSlewRate_b1_p1 */
	/*   [phyinit_C_initPhyConfig] Pstate=2, Memclk=50MHz, Programming TxSlewRate::TxPreDrvMode to 0x2 */
	/*   [phyinit_C_initPhyConfig] Pstate=2, Memclk=50MHz, Programming TxSlewRate::TxPreP to 0xd */
	/*   [phyinit_C_initPhyConfig] Pstate=2, Memclk=50MHz, Programming TxSlewRate::TxPreN to 0xf */
	/*   [phyinit_C_initPhyConfig] ### NOTE ### Optimal setting for TxSlewRate::TxPreP and TxSlewRate::TxPreP are technology specific. */
	/*   [phyinit_C_initPhyConfig] ### NOTE ### Please consult the "Output Slew Rate" section of HSpice Model App Note in specific technology for recommended settings */

	dwc_ddrphy_apb_wr(0x21005f, 0x2fd); /*  DWC_DDRPHYA_DBYTE0_TxSlewRate_b0_p2 */
	dwc_ddrphy_apb_wr(0x21015f, 0x2fd); /*  DWC_DDRPHYA_DBYTE0_TxSlewRate_b1_p2 */
	dwc_ddrphy_apb_wr(0x21105f, 0x2fd); /*  DWC_DDRPHYA_DBYTE1_TxSlewRate_b0_p2 */
	dwc_ddrphy_apb_wr(0x21115f, 0x2fd); /*  DWC_DDRPHYA_DBYTE1_TxSlewRate_b1_p2 */
	dwc_ddrphy_apb_wr(0x21205f, 0x2fd); /*  DWC_DDRPHYA_DBYTE2_TxSlewRate_b0_p2 */
	dwc_ddrphy_apb_wr(0x21215f, 0x2fd); /*  DWC_DDRPHYA_DBYTE2_TxSlewRate_b1_p2 */
	dwc_ddrphy_apb_wr(0x21305f, 0x2fd); /*  DWC_DDRPHYA_DBYTE3_TxSlewRate_b0_p2 */
	dwc_ddrphy_apb_wr(0x21315f, 0x2fd); /*  DWC_DDRPHYA_DBYTE3_TxSlewRate_b1_p2 */
	/*   [phyinit_C_initPhyConfig] Programming ATxSlewRate::ATxPreDrvMode to 0x3, ANIB=0 */
	/*   [phyinit_C_initPhyConfig] Programming ATxSlewRate::ATxPreP to 0x5, ANIB=0 */
	/*   [phyinit_C_initPhyConfig] Programming ATxSlewRate::ATxPreN to 0x5, ANIB=0 */
	/*   [phyinit_C_initPhyConfig] ### NOTE ### Optimal setting for ATxSlewRate::ATxPreP and ATxSlewRate::ATxPreP are technology specific. */
	/*   [phyinit_C_initPhyConfig] ### NOTE ### Please consult the "Output Slew Rate" section of HSpice Model App Note in specific technology for recommended settings */

	dwc_ddrphy_apb_wr(0x55, 0x355); /*  DWC_DDRPHYA_ANIB0_ATxSlewRate */
	/*   [phyinit_C_initPhyConfig] Programming ATxSlewRate::ATxPreDrvMode to 0x3, ANIB=1 */
	/*   [phyinit_C_initPhyConfig] Programming ATxSlewRate::ATxPreP to 0x5, ANIB=1 */
	/*   [phyinit_C_initPhyConfig] Programming ATxSlewRate::ATxPreN to 0x5, ANIB=1 */
	/*   [phyinit_C_initPhyConfig] ### NOTE ### Optimal setting for ATxSlewRate::ATxPreP and ATxSlewRate::ATxPreP are technology specific. */
	/*   [phyinit_C_initPhyConfig] ### NOTE ### Please consult the "Output Slew Rate" section of HSpice Model App Note in specific technology for recommended settings */

	dwc_ddrphy_apb_wr(0x1055, 0x355); /*  DWC_DDRPHYA_ANIB1_ATxSlewRate */
	/*   [phyinit_C_initPhyConfig] Programming ATxSlewRate::ATxPreDrvMode to 0x3, ANIB=2 */
	/*   [phyinit_C_initPhyConfig] Programming ATxSlewRate::ATxPreP to 0x5, ANIB=2 */
	/*   [phyinit_C_initPhyConfig] Programming ATxSlewRate::ATxPreN to 0x5, ANIB=2 */
	/*   [phyinit_C_initPhyConfig] ### NOTE ### Optimal setting for ATxSlewRate::ATxPreP and ATxSlewRate::ATxPreP are technology specific. */
	/*   [phyinit_C_initPhyConfig] ### NOTE ### Please consult the "Output Slew Rate" section of HSpice Model App Note in specific technology for recommended settings */

	dwc_ddrphy_apb_wr(0x2055, 0x355); /*  DWC_DDRPHYA_ANIB2_ATxSlewRate */
	/*   [phyinit_C_initPhyConfig] Programming ATxSlewRate::ATxPreDrvMode to 0x3, ANIB=3 */
	/*   [phyinit_C_initPhyConfig] Programming ATxSlewRate::ATxPreP to 0x5, ANIB=3 */
	/*   [phyinit_C_initPhyConfig] Programming ATxSlewRate::ATxPreN to 0x5, ANIB=3 */
	/*   [phyinit_C_initPhyConfig] ### NOTE ### Optimal setting for ATxSlewRate::ATxPreP and ATxSlewRate::ATxPreP are technology specific. */
	/*   [phyinit_C_initPhyConfig] ### NOTE ### Please consult the "Output Slew Rate" section of HSpice Model App Note in specific technology for recommended settings */

	dwc_ddrphy_apb_wr(0x3055, 0x355); /*  DWC_DDRPHYA_ANIB3_ATxSlewRate */
	/*   [phyinit_C_initPhyConfig] Programming ATxSlewRate::ATxPreDrvMode to 0x0, ANIB=4 */
	/*   [phyinit_C_initPhyConfig] Programming ATxSlewRate::ATxPreP to 0x5, ANIB=4 */
	/*   [phyinit_C_initPhyConfig] Programming ATxSlewRate::ATxPreN to 0x5, ANIB=4 */
	/*   [phyinit_C_initPhyConfig] ### NOTE ### Optimal setting for ATxSlewRate::ATxPreP and ATxSlewRate::ATxPreP are technology specific. */
	/*   [phyinit_C_initPhyConfig] ### NOTE ### Please consult the "Output Slew Rate" section of HSpice Model App Note in specific technology for recommended settings */

	dwc_ddrphy_apb_wr(0x4055, 0x55); /*  DWC_DDRPHYA_ANIB4_ATxSlewRate */
	/*   [phyinit_C_initPhyConfig] Programming ATxSlewRate::ATxPreDrvMode to 0x0, ANIB=5 */
	/*   [phyinit_C_initPhyConfig] Programming ATxSlewRate::ATxPreP to 0x5, ANIB=5 */
	/*   [phyinit_C_initPhyConfig] Programming ATxSlewRate::ATxPreN to 0x5, ANIB=5 */
	/*   [phyinit_C_initPhyConfig] ### NOTE ### Optimal setting for ATxSlewRate::ATxPreP and ATxSlewRate::ATxPreP are technology specific. */
	/*   [phyinit_C_initPhyConfig] ### NOTE ### Please consult the "Output Slew Rate" section of HSpice Model App Note in specific technology for recommended settings */

	dwc_ddrphy_apb_wr(0x5055, 0x55); /*  DWC_DDRPHYA_ANIB5_ATxSlewRate */
	/*   [phyinit_C_initPhyConfig] Programming ATxSlewRate::ATxPreDrvMode to 0x3, ANIB=6 */
	/*   [phyinit_C_initPhyConfig] Programming ATxSlewRate::ATxPreP to 0x5, ANIB=6 */
	/*   [phyinit_C_initPhyConfig] Programming ATxSlewRate::ATxPreN to 0x5, ANIB=6 */
	/*   [phyinit_C_initPhyConfig] ### NOTE ### Optimal setting for ATxSlewRate::ATxPreP and ATxSlewRate::ATxPreP are technology specific. */
	/*   [phyinit_C_initPhyConfig] ### NOTE ### Please consult the "Output Slew Rate" section of HSpice Model App Note in specific technology for recommended settings */

	dwc_ddrphy_apb_wr(0x6055, 0x355); /*  DWC_DDRPHYA_ANIB6_ATxSlewRate */
	/*   [phyinit_C_initPhyConfig] Programming ATxSlewRate::ATxPreDrvMode to 0x3, ANIB=7 */
	/*   [phyinit_C_initPhyConfig] Programming ATxSlewRate::ATxPreP to 0x5, ANIB=7 */
	/*   [phyinit_C_initPhyConfig] Programming ATxSlewRate::ATxPreN to 0x5, ANIB=7 */
	/*   [phyinit_C_initPhyConfig] ### NOTE ### Optimal setting for ATxSlewRate::ATxPreP and ATxSlewRate::ATxPreP are technology specific. */
	/*   [phyinit_C_initPhyConfig] ### NOTE ### Please consult the "Output Slew Rate" section of HSpice Model App Note in specific technology for recommended settings */

	dwc_ddrphy_apb_wr(0x7055, 0x355); /*  DWC_DDRPHYA_ANIB7_ATxSlewRate */
	/*   [phyinit_C_initPhyConfig] Programming ATxSlewRate::ATxPreDrvMode to 0x3, ANIB=8 */
	/*   [phyinit_C_initPhyConfig] Programming ATxSlewRate::ATxPreP to 0x5, ANIB=8 */
	/*   [phyinit_C_initPhyConfig] Programming ATxSlewRate::ATxPreN to 0x5, ANIB=8 */
	/*   [phyinit_C_initPhyConfig] ### NOTE ### Optimal setting for ATxSlewRate::ATxPreP and ATxSlewRate::ATxPreP are technology specific. */
	/*   [phyinit_C_initPhyConfig] ### NOTE ### Please consult the "Output Slew Rate" section of HSpice Model App Note in specific technology for recommended settings */

	dwc_ddrphy_apb_wr(0x8055, 0x355); /*  DWC_DDRPHYA_ANIB8_ATxSlewRate */
	/*   [phyinit_C_initPhyConfig] Programming ATxSlewRate::ATxPreDrvMode to 0x3, ANIB=9 */
	/*   [phyinit_C_initPhyConfig] Programming ATxSlewRate::ATxPreP to 0x5, ANIB=9 */
	/*   [phyinit_C_initPhyConfig] Programming ATxSlewRate::ATxPreN to 0x5, ANIB=9 */
	/*   [phyinit_C_initPhyConfig] ### NOTE ### Optimal setting for ATxSlewRate::ATxPreP and ATxSlewRate::ATxPreP are technology specific. */
	/*   [phyinit_C_initPhyConfig] ### NOTE ### Please consult the "Output Slew Rate" section of HSpice Model App Note in specific technology for recommended settings */

	dwc_ddrphy_apb_wr(0x9055, 0x355); /*  DWC_DDRPHYA_ANIB9_ATxSlewRate */
	dwc_ddrphy_apb_wr(0x200c5, 0xa); /*  DWC_DDRPHYA_MASTER0_PllCtrl2_p0 */
	/*   [phyinit_C_initPhyConfig] Pstate=0,  Memclk=1200MHz, Programming PllCtrl2 to a based on DfiClk frequency = 600. */
	dwc_ddrphy_apb_wr(0x1200c5, 0x7); /*  DWC_DDRPHYA_MASTER0_PllCtrl2_p1 */
	/*   [phyinit_C_initPhyConfig] Pstate=1,  Memclk=200MHz, Programming PllCtrl2 to 7 based on DfiClk frequency = 100. */
	dwc_ddrphy_apb_wr(0x2200c5, 0x7); /*  DWC_DDRPHYA_MASTER0_PllCtrl2_p2 */
	/*   [phyinit_C_initPhyConfig] Pstate=2,  Memclk=50MHz, Programming PllCtrl2 to 7 based on DfiClk frequency = 25. */
	/*  */
	/*  ############################################################## */
	/*   */
	/*   Program ARdPtrInitVal based on Frequency and PLL Bypass inputs */
	/*   The values programmed here assume ideal properties of DfiClk */
	/*   and Pclk including: */
	/*   - DfiClk skew */
	/*   - DfiClk jitter */
	/*   - DfiClk PVT variations */
	/*   - Pclk skew */
	/*   - Pclk jitter */
	/*   */
	/*   PLL Bypassed mode: */
	/*       For MemClk frequency > 933MHz, the valid range of ARdPtrInitVal_p0[3:0] is: 2-6 */
	/*       For MemClk frequency < 933MHz, the valid range of ARdPtrInitVal_p0[3:0] is: 1-6 */
	/*   */
	/*   PLL Enabled mode: */
	/*       For MemClk frequency > 933MHz, the valid range of ARdPtrInitVal_p0[3:0] is: 1-6 */
	/*       For MemClk frequency < 933MHz, the valid range of ARdPtrInitVal_p0[3:0] is: 0-6 */
	/*   */
	/*  ############################################################## */
	/*   [phyinit_C_initPhyConfig] Pstate=0, Memclk=1200MHz, Programming ARdPtrInitVal to 0x2 */
	dwc_ddrphy_apb_wr(0x2002e, 0x2); /*  DWC_DDRPHYA_MASTER0_ARdPtrInitVal_p0 */
	/*   [phyinit_C_initPhyConfig] Pstate=1, Memclk=200MHz, Programming ARdPtrInitVal to 0x2 */
	dwc_ddrphy_apb_wr(0x12002e, 0x2); /*  DWC_DDRPHYA_MASTER0_ARdPtrInitVal_p1 */
	/*   [phyinit_C_initPhyConfig] Pstate=2, Memclk=50MHz, Programming ARdPtrInitVal to 0x2 */
	dwc_ddrphy_apb_wr(0x22002e, 0x2); /*  DWC_DDRPHYA_MASTER0_ARdPtrInitVal_p2 */
	/*   [phyinit_C_initPhyConfig] Pstate=0, Memclk=1200MHz, Programming DqsPreambleControl::TwoTckRxDqsPre to 0x0 */
	/*   [phyinit_C_initPhyConfig] Pstate=0, Memclk=1200MHz, Programming DqsPreambleControl::TwoTckTxDqsPre to 0x0 */
	/*   [phyinit_C_initPhyConfig] Pstate=0, Memclk=1200MHz, Programming DqsPreambleControl::PositionDfeInit to 0x2 */
	/*   [phyinit_C_initPhyConfig] Pstate=0, Memclk=1200MHz, Programming DqsPreambleControl::LP4TglTwoTckTxDqsPre to 0x0 */
	/*   [phyinit_C_initPhyConfig] Pstate=0, Memclk=1200MHz, Programming DqsPreambleControl::LP4PostambleExt to 0x0 */
	/*   [phyinit_C_initPhyConfig] Pstate=0, Memclk=1200MHz, Programming DqsPreambleControl::LP4SttcPreBridgeRxEn to 0x0 */
	/*   [phyinit_C_initPhyConfig] Pstate=0, Memclk=1200MHz, Programming DqsPreambleControl to 0x8 */
	dwc_ddrphy_apb_wr(0x20024, 0x8); /*  DWC_DDRPHYA_MASTER0_DqsPreambleControl_p0 */
	/*   [phyinit_C_initPhyConfig] Pstate=0, Memclk=1200MHz, Programming DbyteDllModeCntrl to 0x2 */
	dwc_ddrphy_apb_wr(0x2003a, 0x2); /*  DWC_DDRPHYA_MASTER0_DbyteDllModeCntrl */
	/*   [phyinit_C_initPhyConfig] Pstate=1, Memclk=200MHz, Programming DqsPreambleControl::TwoTckRxDqsPre to 0x0 */
	/*   [phyinit_C_initPhyConfig] Pstate=1, Memclk=200MHz, Programming DqsPreambleControl::TwoTckTxDqsPre to 0x0 */
	/*   [phyinit_C_initPhyConfig] Pstate=1, Memclk=200MHz, Programming DqsPreambleControl::PositionDfeInit to 0x2 */
	/*   [phyinit_C_initPhyConfig] Pstate=1, Memclk=200MHz, Programming DqsPreambleControl::LP4TglTwoTckTxDqsPre to 0x0 */
	/*   [phyinit_C_initPhyConfig] Pstate=1, Memclk=200MHz, Programming DqsPreambleControl::LP4PostambleExt to 0x0 */
	/*   [phyinit_C_initPhyConfig] Pstate=1, Memclk=200MHz, Programming DqsPreambleControl::LP4SttcPreBridgeRxEn to 0x0 */
	/*   [phyinit_C_initPhyConfig] Pstate=1, Memclk=200MHz, Programming DqsPreambleControl to 0x8 */
	dwc_ddrphy_apb_wr(0x120024, 0x8); /*  DWC_DDRPHYA_MASTER0_DqsPreambleControl_p1 */
	/*   [phyinit_C_initPhyConfig] Pstate=1, Memclk=200MHz, Programming DbyteDllModeCntrl to 0x2 */
	dwc_ddrphy_apb_wr(0x2003a, 0x2); /*  DWC_DDRPHYA_MASTER0_DbyteDllModeCntrl */
	/*   [phyinit_C_initPhyConfig] Pstate=2, Memclk=50MHz, Programming DqsPreambleControl::TwoTckRxDqsPre to 0x0 */
	/*   [phyinit_C_initPhyConfig] Pstate=2, Memclk=50MHz, Programming DqsPreambleControl::TwoTckTxDqsPre to 0x0 */
	/*   [phyinit_C_initPhyConfig] Pstate=2, Memclk=50MHz, Programming DqsPreambleControl::PositionDfeInit to 0x2 */
	/*   [phyinit_C_initPhyConfig] Pstate=2, Memclk=50MHz, Programming DqsPreambleControl::LP4TglTwoTckTxDqsPre to 0x0 */
	/*   [phyinit_C_initPhyConfig] Pstate=2, Memclk=50MHz, Programming DqsPreambleControl::LP4PostambleExt to 0x0 */
	/*   [phyinit_C_initPhyConfig] Pstate=2, Memclk=50MHz, Programming DqsPreambleControl::LP4SttcPreBridgeRxEn to 0x0 */
	/*   [phyinit_C_initPhyConfig] Pstate=2, Memclk=50MHz, Programming DqsPreambleControl to 0x8 */
	dwc_ddrphy_apb_wr(0x220024, 0x8); /*  DWC_DDRPHYA_MASTER0_DqsPreambleControl_p2 */
	/*   [phyinit_C_initPhyConfig] Pstate=2, Memclk=50MHz, Programming DbyteDllModeCntrl to 0x2 */
	dwc_ddrphy_apb_wr(0x2003a, 0x2); /*  DWC_DDRPHYA_MASTER0_DbyteDllModeCntrl */
	/*   [phyinit_C_initPhyConfig] Pstate=0, Memclk=1200MHz, Programming ProcOdtTimeCtl to 0x6 */
	dwc_ddrphy_apb_wr(0x20056, 0x6); /*  DWC_DDRPHYA_MASTER0_ProcOdtTimeCtl_p0 */
	/*   [phyinit_C_initPhyConfig] Pstate=1, Memclk=200MHz, Programming ProcOdtTimeCtl to 0xa */
	dwc_ddrphy_apb_wr(0x120056, 0xa); /*  DWC_DDRPHYA_MASTER0_ProcOdtTimeCtl_p1 */
	/*   [phyinit_C_initPhyConfig] Pstate=2, Memclk=50MHz, Programming ProcOdtTimeCtl to 0xa */
	dwc_ddrphy_apb_wr(0x220056, 0xa); /*  DWC_DDRPHYA_MASTER0_ProcOdtTimeCtl_p2 */
	/*   [phyinit_C_initPhyConfig] Pstate=0, Memclk=1200MHz, Programming TxOdtDrvStren::ODTStrenP to 0x1a */
	/*   [phyinit_C_initPhyConfig] Pstate=0, Memclk=1200MHz, Programming TxOdtDrvStren::ODTStrenN to 0x0 */
	dwc_ddrphy_apb_wr(0x1004d, 0x1a); /*  DWC_DDRPHYA_DBYTE0_TxOdtDrvStren_b0_p0 */
	dwc_ddrphy_apb_wr(0x1014d, 0x1a); /*  DWC_DDRPHYA_DBYTE0_TxOdtDrvStren_b1_p0 */
	dwc_ddrphy_apb_wr(0x1104d, 0x1a); /*  DWC_DDRPHYA_DBYTE1_TxOdtDrvStren_b0_p0 */
	dwc_ddrphy_apb_wr(0x1114d, 0x1a); /*  DWC_DDRPHYA_DBYTE1_TxOdtDrvStren_b1_p0 */
	dwc_ddrphy_apb_wr(0x1204d, 0x1a); /*  DWC_DDRPHYA_DBYTE2_TxOdtDrvStren_b0_p0 */
	dwc_ddrphy_apb_wr(0x1214d, 0x1a); /*  DWC_DDRPHYA_DBYTE2_TxOdtDrvStren_b1_p0 */
	dwc_ddrphy_apb_wr(0x1304d, 0x1a); /*  DWC_DDRPHYA_DBYTE3_TxOdtDrvStren_b0_p0 */
	dwc_ddrphy_apb_wr(0x1314d, 0x1a); /*  DWC_DDRPHYA_DBYTE3_TxOdtDrvStren_b1_p0 */
	/*   [phyinit_C_initPhyConfig] Pstate=1, Memclk=200MHz, Programming TxOdtDrvStren::ODTStrenP to 0x1a */
	/*   [phyinit_C_initPhyConfig] Pstate=1, Memclk=200MHz, Programming TxOdtDrvStren::ODTStrenN to 0x0 */
	dwc_ddrphy_apb_wr(0x11004d, 0x1a); /*  DWC_DDRPHYA_DBYTE0_TxOdtDrvStren_b0_p1 */
	dwc_ddrphy_apb_wr(0x11014d, 0x1a); /*  DWC_DDRPHYA_DBYTE0_TxOdtDrvStren_b1_p1 */
	dwc_ddrphy_apb_wr(0x11104d, 0x1a); /*  DWC_DDRPHYA_DBYTE1_TxOdtDrvStren_b0_p1 */
	dwc_ddrphy_apb_wr(0x11114d, 0x1a); /*  DWC_DDRPHYA_DBYTE1_TxOdtDrvStren_b1_p1 */
	dwc_ddrphy_apb_wr(0x11204d, 0x1a); /*  DWC_DDRPHYA_DBYTE2_TxOdtDrvStren_b0_p1 */
	dwc_ddrphy_apb_wr(0x11214d, 0x1a); /*  DWC_DDRPHYA_DBYTE2_TxOdtDrvStren_b1_p1 */
	dwc_ddrphy_apb_wr(0x11304d, 0x1a); /*  DWC_DDRPHYA_DBYTE3_TxOdtDrvStren_b0_p1 */
	dwc_ddrphy_apb_wr(0x11314d, 0x1a); /*  DWC_DDRPHYA_DBYTE3_TxOdtDrvStren_b1_p1 */
	/*   [phyinit_C_initPhyConfig] Pstate=2, Memclk=50MHz, Programming TxOdtDrvStren::ODTStrenP to 0x1a */
	/*   [phyinit_C_initPhyConfig] Pstate=2, Memclk=50MHz, Programming TxOdtDrvStren::ODTStrenN to 0x0 */
	dwc_ddrphy_apb_wr(0x21004d, 0x1a); /*  DWC_DDRPHYA_DBYTE0_TxOdtDrvStren_b0_p2 */
	dwc_ddrphy_apb_wr(0x21014d, 0x1a); /*  DWC_DDRPHYA_DBYTE0_TxOdtDrvStren_b1_p2 */
	dwc_ddrphy_apb_wr(0x21104d, 0x1a); /*  DWC_DDRPHYA_DBYTE1_TxOdtDrvStren_b0_p2 */
	dwc_ddrphy_apb_wr(0x21114d, 0x1a); /*  DWC_DDRPHYA_DBYTE1_TxOdtDrvStren_b1_p2 */
	dwc_ddrphy_apb_wr(0x21204d, 0x1a); /*  DWC_DDRPHYA_DBYTE2_TxOdtDrvStren_b0_p2 */
	dwc_ddrphy_apb_wr(0x21214d, 0x1a); /*  DWC_DDRPHYA_DBYTE2_TxOdtDrvStren_b1_p2 */
	dwc_ddrphy_apb_wr(0x21304d, 0x1a); /*  DWC_DDRPHYA_DBYTE3_TxOdtDrvStren_b0_p2 */
	dwc_ddrphy_apb_wr(0x21314d, 0x1a); /*  DWC_DDRPHYA_DBYTE3_TxOdtDrvStren_b1_p2 */
	/*   [phyinit_C_initPhyConfig] Pstate=0, Memclk=1200MHz, Programming TxImpedanceCtrl1::DrvStrenFSDqP to 0x38 */
	/*   [phyinit_C_initPhyConfig] Pstate=0, Memclk=1200MHz, Programming TxImpedanceCtrl1::DrvStrenFSDqN to 0x38 */
	dwc_ddrphy_apb_wr(0x10049, 0xe38); /*  DWC_DDRPHYA_DBYTE0_TxImpedanceCtrl1_b0_p0 */
	dwc_ddrphy_apb_wr(0x10149, 0xe38); /*  DWC_DDRPHYA_DBYTE0_TxImpedanceCtrl1_b1_p0 */
	dwc_ddrphy_apb_wr(0x11049, 0xe38); /*  DWC_DDRPHYA_DBYTE1_TxImpedanceCtrl1_b0_p0 */
	dwc_ddrphy_apb_wr(0x11149, 0xe38); /*  DWC_DDRPHYA_DBYTE1_TxImpedanceCtrl1_b1_p0 */
	dwc_ddrphy_apb_wr(0x12049, 0xe38); /*  DWC_DDRPHYA_DBYTE2_TxImpedanceCtrl1_b0_p0 */
	dwc_ddrphy_apb_wr(0x12149, 0xe38); /*  DWC_DDRPHYA_DBYTE2_TxImpedanceCtrl1_b1_p0 */
	dwc_ddrphy_apb_wr(0x13049, 0xe38); /*  DWC_DDRPHYA_DBYTE3_TxImpedanceCtrl1_b0_p0 */
	dwc_ddrphy_apb_wr(0x13149, 0xe38); /*  DWC_DDRPHYA_DBYTE3_TxImpedanceCtrl1_b1_p0 */
	/*   [phyinit_C_initPhyConfig] Pstate=1, Memclk=200MHz, Programming TxImpedanceCtrl1::DrvStrenFSDqP to 0x38 */
	/*   [phyinit_C_initPhyConfig] Pstate=1, Memclk=200MHz, Programming TxImpedanceCtrl1::DrvStrenFSDqN to 0x38 */
	dwc_ddrphy_apb_wr(0x110049, 0xe38); /*  DWC_DDRPHYA_DBYTE0_TxImpedanceCtrl1_b0_p1 */
	dwc_ddrphy_apb_wr(0x110149, 0xe38); /*  DWC_DDRPHYA_DBYTE0_TxImpedanceCtrl1_b1_p1 */
	dwc_ddrphy_apb_wr(0x111049, 0xe38); /*  DWC_DDRPHYA_DBYTE1_TxImpedanceCtrl1_b0_p1 */
	dwc_ddrphy_apb_wr(0x111149, 0xe38); /*  DWC_DDRPHYA_DBYTE1_TxImpedanceCtrl1_b1_p1 */
	dwc_ddrphy_apb_wr(0x112049, 0xe38); /*  DWC_DDRPHYA_DBYTE2_TxImpedanceCtrl1_b0_p1 */
	dwc_ddrphy_apb_wr(0x112149, 0xe38); /*  DWC_DDRPHYA_DBYTE2_TxImpedanceCtrl1_b1_p1 */
	dwc_ddrphy_apb_wr(0x113049, 0xe38); /*  DWC_DDRPHYA_DBYTE3_TxImpedanceCtrl1_b0_p1 */
	dwc_ddrphy_apb_wr(0x113149, 0xe38); /*  DWC_DDRPHYA_DBYTE3_TxImpedanceCtrl1_b1_p1 */
	/*   [phyinit_C_initPhyConfig] Pstate=2, Memclk=50MHz, Programming TxImpedanceCtrl1::DrvStrenFSDqP to 0x38 */
	/*   [phyinit_C_initPhyConfig] Pstate=2, Memclk=50MHz, Programming TxImpedanceCtrl1::DrvStrenFSDqN to 0x38 */
	dwc_ddrphy_apb_wr(0x210049, 0xe38); /*  DWC_DDRPHYA_DBYTE0_TxImpedanceCtrl1_b0_p2 */
	dwc_ddrphy_apb_wr(0x210149, 0xe38); /*  DWC_DDRPHYA_DBYTE0_TxImpedanceCtrl1_b1_p2 */
	dwc_ddrphy_apb_wr(0x211049, 0xe38); /*  DWC_DDRPHYA_DBYTE1_TxImpedanceCtrl1_b0_p2 */
	dwc_ddrphy_apb_wr(0x211149, 0xe38); /*  DWC_DDRPHYA_DBYTE1_TxImpedanceCtrl1_b1_p2 */
	dwc_ddrphy_apb_wr(0x212049, 0xe38); /*  DWC_DDRPHYA_DBYTE2_TxImpedanceCtrl1_b0_p2 */
	dwc_ddrphy_apb_wr(0x212149, 0xe38); /*  DWC_DDRPHYA_DBYTE2_TxImpedanceCtrl1_b1_p2 */
	dwc_ddrphy_apb_wr(0x213049, 0xe38); /*  DWC_DDRPHYA_DBYTE3_TxImpedanceCtrl1_b0_p2 */
	dwc_ddrphy_apb_wr(0x213149, 0xe38); /*  DWC_DDRPHYA_DBYTE3_TxImpedanceCtrl1_b1_p2 */
	/*   [phyinit_C_initPhyConfig] Programming ATxImpedance::ADrvStrenP to 0x3 */
	/*   [phyinit_C_initPhyConfig] Programming ATxImpedance::ADrvStrenN to 0x3 */
	dwc_ddrphy_apb_wr(0x43, 0x63); /*  DWC_DDRPHYA_ANIB0_ATxImpedance */
	dwc_ddrphy_apb_wr(0x1043, 0x63); /*  DWC_DDRPHYA_ANIB1_ATxImpedance */
	dwc_ddrphy_apb_wr(0x2043, 0x63); /*  DWC_DDRPHYA_ANIB2_ATxImpedance */
	dwc_ddrphy_apb_wr(0x3043, 0x63); /*  DWC_DDRPHYA_ANIB3_ATxImpedance */
	dwc_ddrphy_apb_wr(0x4043, 0x63); /*  DWC_DDRPHYA_ANIB4_ATxImpedance */
	dwc_ddrphy_apb_wr(0x5043, 0x63); /*  DWC_DDRPHYA_ANIB5_ATxImpedance */
	dwc_ddrphy_apb_wr(0x6043, 0x63); /*  DWC_DDRPHYA_ANIB6_ATxImpedance */
	dwc_ddrphy_apb_wr(0x7043, 0x63); /*  DWC_DDRPHYA_ANIB7_ATxImpedance */
	dwc_ddrphy_apb_wr(0x8043, 0x63); /*  DWC_DDRPHYA_ANIB8_ATxImpedance */
	dwc_ddrphy_apb_wr(0x9043, 0x63); /*  DWC_DDRPHYA_ANIB9_ATxImpedance */
	/*   [phyinit_C_initPhyConfig] Programming DfiMode to 0x5 */
	dwc_ddrphy_apb_wr(0x20018, 0x5); /*  DWC_DDRPHYA_MASTER0_DfiMode */
	/*   [phyinit_C_initPhyConfig] Programming DfiCAMode to 0x2 */
	dwc_ddrphy_apb_wr(0x20075, 0x2); /*  DWC_DDRPHYA_MASTER0_DfiCAMode */
	/*   [phyinit_C_initPhyConfig] Programming CalDrvStr0::CalDrvStrPd50 to 0x0 */
	/*   [phyinit_C_initPhyConfig] Programming CalDrvStr0::CalDrvStrPu50 to 0x0 */
	dwc_ddrphy_apb_wr(0x20050, 0x0); /*  DWC_DDRPHYA_MASTER0_CalDrvStr0 */
	/*   [phyinit_C_initPhyConfig] Pstate=0, Memclk=1200MHz, Programming CalUclkInfo::CalUClkTicksPer1uS to 0x258 */
	dwc_ddrphy_apb_wr(0x20008, 0x258); /*  DWC_DDRPHYA_MASTER0_CalUclkInfo_p0 */
	/*   [phyinit_C_initPhyConfig] Pstate=1, Memclk=200MHz, Programming CalUclkInfo::CalUClkTicksPer1uS to 0x64 */
	dwc_ddrphy_apb_wr(0x120008, 0x64); /*  DWC_DDRPHYA_MASTER0_CalUclkInfo_p1 */
	/*   [phyinit_C_initPhyConfig] Pstate=2, Memclk=50MHz, Programming CalUclkInfo::CalUClkTicksPer1uS to 0x19 */
	dwc_ddrphy_apb_wr(0x220008, 0x19); /*  DWC_DDRPHYA_MASTER0_CalUclkInfo_p2 */
	/*   [phyinit_C_initPhyConfig] Programming CalRate::CalInterval to 0x9 */
	/*   [phyinit_C_initPhyConfig] Programming CalRate::CalOnce to 0x0 */
	dwc_ddrphy_apb_wr(0x20088, 0x9); /*  DWC_DDRPHYA_MASTER0_CalRate */
	/*   [phyinit_C_initPhyConfig] Pstate=0, Programming VrefInGlobal::GlobalVrefInSel to 0x0 */
	/*   [phyinit_C_initPhyConfig] Pstate=0, Programming VrefInGlobal::GlobalVrefInDAC to 0x4d */
	/*   [phyinit_C_initPhyConfig] Pstate=0, Programming VrefInGlobal to 0x268 */
	dwc_ddrphy_apb_wr(0x200b2, 0x268); /*  DWC_DDRPHYA_MASTER0_VrefInGlobal_p0 */
	/*   [phyinit_C_initPhyConfig] Pstate=0, Programming DqDqsRcvCntrl::MajorModeDbyte to 0x3 */
	/*   [phyinit_C_initPhyConfig] Pstate=0, Programming DqDqsRcvCntrl to 0x5b1 */
	dwc_ddrphy_apb_wr(0x10043, 0x5b1); /*  DWC_DDRPHYA_DBYTE0_DqDqsRcvCntrl_b0_p0 */
	dwc_ddrphy_apb_wr(0x10143, 0x5b1); /*  DWC_DDRPHYA_DBYTE0_DqDqsRcvCntrl_b1_p0 */
	dwc_ddrphy_apb_wr(0x11043, 0x5b1); /*  DWC_DDRPHYA_DBYTE1_DqDqsRcvCntrl_b0_p0 */
	dwc_ddrphy_apb_wr(0x11143, 0x5b1); /*  DWC_DDRPHYA_DBYTE1_DqDqsRcvCntrl_b1_p0 */
	dwc_ddrphy_apb_wr(0x12043, 0x5b1); /*  DWC_DDRPHYA_DBYTE2_DqDqsRcvCntrl_b0_p0 */
	dwc_ddrphy_apb_wr(0x12143, 0x5b1); /*  DWC_DDRPHYA_DBYTE2_DqDqsRcvCntrl_b1_p0 */
	dwc_ddrphy_apb_wr(0x13043, 0x5b1); /*  DWC_DDRPHYA_DBYTE3_DqDqsRcvCntrl_b0_p0 */
	dwc_ddrphy_apb_wr(0x13143, 0x5b1); /*  DWC_DDRPHYA_DBYTE3_DqDqsRcvCntrl_b1_p0 */
	/*   [phyinit_C_initPhyConfig] Pstate=1, Programming VrefInGlobal::GlobalVrefInSel to 0x0 */
	/*   [phyinit_C_initPhyConfig] Pstate=1, Programming VrefInGlobal::GlobalVrefInDAC to 0x4d */
	/*   [phyinit_C_initPhyConfig] Pstate=1, Programming VrefInGlobal to 0x268 */
	dwc_ddrphy_apb_wr(0x1200b2, 0x268); /*  DWC_DDRPHYA_MASTER0_VrefInGlobal_p1 */
	/*   [phyinit_C_initPhyConfig] Pstate=1, Programming DqDqsRcvCntrl::MajorModeDbyte to 0x3 */
	/*   [phyinit_C_initPhyConfig] Pstate=1, Programming DqDqsRcvCntrl to 0x5b1 */
	dwc_ddrphy_apb_wr(0x110043, 0x5b1); /*  DWC_DDRPHYA_DBYTE0_DqDqsRcvCntrl_b0_p1 */
	dwc_ddrphy_apb_wr(0x110143, 0x5b1); /*  DWC_DDRPHYA_DBYTE0_DqDqsRcvCntrl_b1_p1 */
	dwc_ddrphy_apb_wr(0x111043, 0x5b1); /*  DWC_DDRPHYA_DBYTE1_DqDqsRcvCntrl_b0_p1 */
	dwc_ddrphy_apb_wr(0x111143, 0x5b1); /*  DWC_DDRPHYA_DBYTE1_DqDqsRcvCntrl_b1_p1 */
	dwc_ddrphy_apb_wr(0x112043, 0x5b1); /*  DWC_DDRPHYA_DBYTE2_DqDqsRcvCntrl_b0_p1 */
	dwc_ddrphy_apb_wr(0x112143, 0x5b1); /*  DWC_DDRPHYA_DBYTE2_DqDqsRcvCntrl_b1_p1 */
	dwc_ddrphy_apb_wr(0x113043, 0x5b1); /*  DWC_DDRPHYA_DBYTE3_DqDqsRcvCntrl_b0_p1 */
	dwc_ddrphy_apb_wr(0x113143, 0x5b1); /*  DWC_DDRPHYA_DBYTE3_DqDqsRcvCntrl_b1_p1 */
	/*   [phyinit_C_initPhyConfig] Pstate=2, Programming VrefInGlobal::GlobalVrefInSel to 0x0 */
	/*   [phyinit_C_initPhyConfig] Pstate=2, Programming VrefInGlobal::GlobalVrefInDAC to 0x4d */
	/*   [phyinit_C_initPhyConfig] Pstate=2, Programming VrefInGlobal to 0x268 */
	dwc_ddrphy_apb_wr(0x2200b2, 0x268); /*  DWC_DDRPHYA_MASTER0_VrefInGlobal_p2 */
	/*   [phyinit_C_initPhyConfig] Pstate=2, Programming DqDqsRcvCntrl::MajorModeDbyte to 0x3 */
	/*   [phyinit_C_initPhyConfig] Pstate=2, Programming DqDqsRcvCntrl to 0x5b1 */
	dwc_ddrphy_apb_wr(0x210043, 0x5b1); /*  DWC_DDRPHYA_DBYTE0_DqDqsRcvCntrl_b0_p2 */
	dwc_ddrphy_apb_wr(0x210143, 0x5b1); /*  DWC_DDRPHYA_DBYTE0_DqDqsRcvCntrl_b1_p2 */
	dwc_ddrphy_apb_wr(0x211043, 0x5b1); /*  DWC_DDRPHYA_DBYTE1_DqDqsRcvCntrl_b0_p2 */
	dwc_ddrphy_apb_wr(0x211143, 0x5b1); /*  DWC_DDRPHYA_DBYTE1_DqDqsRcvCntrl_b1_p2 */
	dwc_ddrphy_apb_wr(0x212043, 0x5b1); /*  DWC_DDRPHYA_DBYTE2_DqDqsRcvCntrl_b0_p2 */
	dwc_ddrphy_apb_wr(0x212143, 0x5b1); /*  DWC_DDRPHYA_DBYTE2_DqDqsRcvCntrl_b1_p2 */
	dwc_ddrphy_apb_wr(0x213043, 0x5b1); /*  DWC_DDRPHYA_DBYTE3_DqDqsRcvCntrl_b0_p2 */
	dwc_ddrphy_apb_wr(0x213143, 0x5b1); /*  DWC_DDRPHYA_DBYTE3_DqDqsRcvCntrl_b1_p2 */
	/*   [phyinit_C_initPhyConfig] Programming MemAlertControl::MALERTVrefLevel to 0x29 */
	/*   [phyinit_C_initPhyConfig] Programming MemAlertControl::MALERTPuStren to 0x5 */
	/*   [phyinit_C_initPhyConfig] Programming MemAlertControl to 0x7529 */
	/*   [phyinit_C_initPhyConfig] Programming MemAlertControl2::MALERTSyncBypass to 0x0 */
	dwc_ddrphy_apb_wr(0x2005b, 0x7529); /*  DWC_DDRPHYA_MASTER0_MemAlertControl */
	dwc_ddrphy_apb_wr(0x2005c, 0x0); /*  DWC_DDRPHYA_MASTER0_MemAlertControl2 */
	/*   [phyinit_C_initPhyConfig] Pstate=0, Memclk=1200MHz, Programming DfiFreqRatio_p0 to 0x1 */
	dwc_ddrphy_apb_wr(0x200fa, 0x1); /*  DWC_DDRPHYA_MASTER0_DfiFreqRatio_p0 */
	/*   [phyinit_C_initPhyConfig] Pstate=1, Memclk=200MHz, Programming DfiFreqRatio_p1 to 0x1 */
	dwc_ddrphy_apb_wr(0x1200fa, 0x1); /*  DWC_DDRPHYA_MASTER0_DfiFreqRatio_p1 */
	/*   [phyinit_C_initPhyConfig] Pstate=2, Memclk=50MHz, Programming DfiFreqRatio_p2 to 0x1 */
	dwc_ddrphy_apb_wr(0x2200fa, 0x1); /*  DWC_DDRPHYA_MASTER0_DfiFreqRatio_p2 */
	/*   [phyinit_C_initPhyConfig] Pstate=0, Memclk=1200MHz, Programming TristateModeCA::DisDynAdrTri_p0 to 0x1 */
	/*   [phyinit_C_initPhyConfig] Pstate=0, Memclk=1200MHz, Programming TristateModeCA::DDR2TMode_p0 to 0x0 */
	dwc_ddrphy_apb_wr(0x20019, 0x5); /*  DWC_DDRPHYA_MASTER0_TristateModeCA_p0 */
	/*   [phyinit_C_initPhyConfig] Pstate=1, Memclk=200MHz, Programming TristateModeCA::DisDynAdrTri_p1 to 0x1 */
	/*   [phyinit_C_initPhyConfig] Pstate=1, Memclk=200MHz, Programming TristateModeCA::DDR2TMode_p1 to 0x0 */
	dwc_ddrphy_apb_wr(0x120019, 0x5); /*  DWC_DDRPHYA_MASTER0_TristateModeCA_p1 */
	/*   [phyinit_C_initPhyConfig] Pstate=2, Memclk=50MHz, Programming TristateModeCA::DisDynAdrTri_p2 to 0x1 */
	/*   [phyinit_C_initPhyConfig] Pstate=2, Memclk=50MHz, Programming TristateModeCA::DDR2TMode_p2 to 0x0 */
	dwc_ddrphy_apb_wr(0x220019, 0x5); /*  DWC_DDRPHYA_MASTER0_TristateModeCA_p2 */
	/*   [phyinit_C_initPhyConfig] Programming DfiFreqXlat* */
	dwc_ddrphy_apb_wr(0x200f0, 0x5665); /*  DWC_DDRPHYA_MASTER0_DfiFreqXlat0 */
	dwc_ddrphy_apb_wr(0x200f1, 0x5555); /*  DWC_DDRPHYA_MASTER0_DfiFreqXlat1 */
	dwc_ddrphy_apb_wr(0x200f2, 0x5555); /*  DWC_DDRPHYA_MASTER0_DfiFreqXlat2 */
	dwc_ddrphy_apb_wr(0x200f3, 0x5555); /*  DWC_DDRPHYA_MASTER0_DfiFreqXlat3 */
	dwc_ddrphy_apb_wr(0x200f4, 0x5555); /*  DWC_DDRPHYA_MASTER0_DfiFreqXlat4 */
	dwc_ddrphy_apb_wr(0x200f5, 0x5555); /*  DWC_DDRPHYA_MASTER0_DfiFreqXlat5 */
	dwc_ddrphy_apb_wr(0x200f6, 0x5555); /*  DWC_DDRPHYA_MASTER0_DfiFreqXlat6 */
	dwc_ddrphy_apb_wr(0x200f7, 0xf000); /*  DWC_DDRPHYA_MASTER0_DfiFreqXlat7 */
	/*   [phyinit_C_initPhyConfig] Programming MasterX4Config::X4TG to 0x0 */
	dwc_ddrphy_apb_wr(0x20025, 0x0); /*  DWC_DDRPHYA_MASTER0_MasterX4Config */
	/*   [phyinit_C_initPhyConfig] Pstate=0, Memclk=1200MHz, Programming DMIPinPresent::RdDbiEnabled to 0x0 */
	dwc_ddrphy_apb_wr(0x2002d, 0x0); /*  DWC_DDRPHYA_MASTER0_DMIPinPresent_p0 */
	/*   [phyinit_C_initPhyConfig] Pstate=1, Memclk=200MHz, Programming DMIPinPresent::RdDbiEnabled to 0x0 */
	dwc_ddrphy_apb_wr(0x12002d, 0x0); /*  DWC_DDRPHYA_MASTER0_DMIPinPresent_p1 */
	/*   [phyinit_C_initPhyConfig] Pstate=2, Memclk=50MHz, Programming DMIPinPresent::RdDbiEnabled to 0x0 */
	dwc_ddrphy_apb_wr(0x22002d, 0x0); /*  DWC_DDRPHYA_MASTER0_DMIPinPresent_p2 */
	/*   [phyinit_C_initPhyConfig] End of dwc_ddrphy_phyinit_C_initPhyConfig() */
	/*  */
	/*  */
	/*  ############################################################## */
	/*   */
	/*   dwc_ddrphy_phyihunit_userCustom_customPreTrain is a user-editable function. */
	/*   */
	/*   See PhyInit App Note for detailed description and function usage */
	/*   */
	/*  ############################################################## */
	ddr_dbg("add 845S pll setting in phyinit\n");
	/*   [phyinit_userCustom_customPreTrain] Start of dwc_ddrphy_phyinit_userCustom_customPreTrain() */
	dwc_ddrphy_apb_wr(0x200c7, 0x21); /*  DWC_DDRPHYA_MASTER0_PllCtrl1_p0 */
	dwc_ddrphy_apb_wr(0x200ca, 0x24); /*  DWC_DDRPHYA_MASTER0_PllTestMode_p0 */
	/*   [phyinit_userCustom_customPreTrain] End of dwc_ddrphy_phyinit_userCustom_customPreTrain() */
	/*   [dwc_ddrphy_phyinit_D_loadIMEM, 1D] Start of dwc_ddrphy_phyinit_D_loadIMEM (Train2D=0) */
	/*  */
	/*  */
	/*  ############################################################## */
	/*   */
	/*   (D) Load the 1D IMEM image */
	/*   */
	/*   This function loads the training firmware IMEM image into the SRAM. */
	/*   See PhyInit App Note for detailed description and function usage */
	/*   */
	/*  ############################################################## */
	/*  */
	/*  */
	/*   [dwc_ddrphy_phyinit_D_loadIMEM, 1D] Programming MemResetL to 0x2 */
	if (!after_retention) {
		dwc_ddrphy_apb_wr(0x20060, 0x2);

		/*  [dwc_ddrphy_phyinit_storeIncvFile] Reading input file: ../../../../firmware/A-2017.09/ddr4/ddr4_pmu_train_imem.incv */

		/*   1.	Enable access to the internal CSRs by setting the MicroContMuxSel CSR to 0. */
		/*         This allows the memory controller unrestricted access to the configuration CSRs. */
		dwc_ddrphy_apb_wr(0xd0000, 0x0); /*  DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
		/*   [dwc_ddrphy_phyinit_WriteOutMem] STARTING. offset 0x50000 size 0x4000 */
		/*   [dwc_ddrphy_phyinit_WriteOutMem] DONE.  Index 0x4000 */
		/*   2.	Isolate the APB access from the internal CSRs by setting the MicroContMuxSel CSR to 1. */
		/*        This allows the firmware unrestricted access to the configuration CSRs. */
		dwc_ddrphy_apb_wr(0xd0000, 0x1); /*  DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
		/*   [dwc_ddrphy_phyinit_D_loadIMEM, 1D] End of dwc_ddrphy_phyinit_D_loadIMEM() */
		/*  */
		/*  */
		/*  ############################################################## */
		/*   */
		/*   Step (E) Set the PHY input clocks to the desired frequency for pstate 0 */
		/*   */
		/*   See PhyInit App Note for detailed description and function usage */
		/*   */
		/*  ############################################################## */
		/*  */
		/* dwc_ddrphy_phyinit_userCustom_E_setDfiClk (0); */

		/*  */
		/*   [dwc_ddrphy_phyinit_userCustom_E_setDfiClk] End of dwc_ddrphy_phyinit_userCustom_E_setDfiClk() */
		/*   [phyinit_F_loadDMEM, 1D] Start of dwc_ddrphy_phyinit_F_loadDMEM (pstate=0, Train2D=0) */

		dwc_ddrphy_apb_wr(0xd0000, 0x0); /*  DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
		/*  for test on silicon, load 1D dmem/imem here */
#ifdef CONFIG_SPL_VSI_FW_LOADING
		load_train_1d_code();
#else
		ddr_load_train_code(FW_1D_IMAGE);
#endif
		ddr_dbg("start 1d train\n");

		/*  */
		/*  ############################################################## */
		/*   */
		/*   (F) Load the 1D DMEM image and write the 1D Message Block parameters for the training firmware */
		/*   */
		/*   See PhyInit App Note for detailed description and function usage */
		/*   */
		/*  ############################################################## */
		/*  */
		/*  [dwc_ddrphy_phyinit_storeIncvFile] Reading input file: ../../../../firmware/A-2017.09/ddr4/ddr4_pmu_train_dmem.incv */

		/*   1.	Enable access to the internal CSRs by setting the MicroContMuxSel CSR to 0. */
		/*         This allows the memory controller unrestricted access to the configuration CSRs. */
		dwc_ddrphy_apb_wr(0xd0000, 0x0); /*  DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
		/*   [dwc_ddrphy_phyinit_WriteOutMem] STARTING. offset 0x54000 size 0x36a */
#ifdef RUN_ON_SILICON
		dwc_ddrphy_apb_wr(0x54000, 0x0);
#else
		dwc_ddrphy_apb_wr(0x54000, 0x600);
#endif
		dwc_ddrphy_apb_wr(0x54001, 0x0);
		dwc_ddrphy_apb_wr(0x54002, 0x0);
		dwc_ddrphy_apb_wr(0x54003, 0x960);
		dwc_ddrphy_apb_wr(0x54004, 0x2);
		dwc_ddrphy_apb_wr(0x54005, 0x0);
		dwc_ddrphy_apb_wr(0x54006, 0x25e);
		dwc_ddrphy_apb_wr(0x54007, 0x2000);
#ifdef DDR_ONE_RANK
		dwc_ddrphy_apb_wr(0x54008, 0x101);
		dwc_ddrphy_apb_wr(0x54009, 0x0);
#else
		dwc_ddrphy_apb_wr(0x54008, 0x303);
		dwc_ddrphy_apb_wr(0x54009, 0x200);/* no addr mirror, 0x200 addr mirror */
#endif
		dwc_ddrphy_apb_wr(0x5400a, 0x0);
#ifdef RUN_ON_SILICON
		dwc_ddrphy_apb_wr(0x5400b, 0x31f);
#else
		dwc_ddrphy_apb_wr(0x5400b, 0x1);
#endif
		dwc_ddrphy_apb_wr(0x5400c, 0xc8);
		dwc_ddrphy_apb_wr(0x5400d, 0x0);
		dwc_ddrphy_apb_wr(0x5400e, 0x0);
		dwc_ddrphy_apb_wr(0x5400f, 0x0);
		dwc_ddrphy_apb_wr(0x54010, 0x0);
		dwc_ddrphy_apb_wr(0x54011, 0x0);
		dwc_ddrphy_apb_wr(0x54012, 0x1);
		dwc_ddrphy_apb_wr(0x5402f, mr_value[0][0]);
		dwc_ddrphy_apb_wr(0x54030, mr_value[0][1]);
		dwc_ddrphy_apb_wr(0x54031, mr_value[0][2]);
		dwc_ddrphy_apb_wr(0x54032, mr_value[0][3]);
		dwc_ddrphy_apb_wr(0x54033, mr_value[0][4]);
		dwc_ddrphy_apb_wr(0x54034, mr_value[0][5]);
		dwc_ddrphy_apb_wr(0x54035, mr_value[0][6]);

#ifdef DDR_ONE_RANK
		dwc_ddrphy_apb_wr(0x54036, 0x101);
#else
		dwc_ddrphy_apb_wr(0x54036, 0x103);
#endif
		dwc_ddrphy_apb_wr(0x54037, 0x0);
		dwc_ddrphy_apb_wr(0x54038, 0x0);
		dwc_ddrphy_apb_wr(0x54039, 0x0);
		dwc_ddrphy_apb_wr(0x5403a, 0x0);
		dwc_ddrphy_apb_wr(0x5403b, 0x0);
		dwc_ddrphy_apb_wr(0x5403c, 0x0);
		dwc_ddrphy_apb_wr(0x5403d, 0x0);
		dwc_ddrphy_apb_wr(0x5403e, 0x0);
		dwc_ddrphy_apb_wr(0x5403f, 0x1221);
		dwc_ddrphy_apb_wr(0x541fc, 0x100);
		/*   [dwc_ddrphy_phyinit_WriteOutMem] DONE.  Index 0x36a */
		/*   2.	Isolate the APB access from the internal CSRs by setting the MicroContMuxSel CSR to 1. */
		/*        This allows the firmware unrestricted access to the configuration CSRs. */
		dwc_ddrphy_apb_wr(0xd0000, 0x1); /*  DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
		/*   [phyinit_F_loadDMEM, 1D] End of dwc_ddrphy_phyinit_F_loadDMEM() */
		/*  */
		/*  */
		/*  ############################################################## */
		/*   */
		/*   (G) Execute the Training Firmware */
		/*   */
		/*   See PhyInit App Note for detailed description and function usage */
		/*   */
		/*  ############################################################## */
		/*  */
		/*  */
		/*   1.  Reset the firmware microcontroller by writing the MicroReset CSR to set the StallToMicro and */
		/*       ResetToMicro fields to 1 (all other fields should be zero). */
		/*       Then rewrite the CSR so that only the StallToMicro remains set (all other fields should be zero). */
		dwc_ddrphy_apb_wr(0xd0000, 0x1); /*  DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
		dwc_ddrphy_apb_wr(0xd0099, 0x9); /*  DWC_DDRPHYA_APBONLY0_MicroReset */
		dwc_ddrphy_apb_wr(0xd0099, 0x1); /*  DWC_DDRPHYA_APBONLY0_MicroReset */
		/*  */
		/*   2. Begin execution of the training firmware by setting the MicroReset CSR to 4'b0000. */
		dwc_ddrphy_apb_wr(0xd0099, 0x0); /*  DWC_DDRPHYA_APBONLY0_MicroReset */
		/*  */
		/*   3.   Wait for the training firmware to complete by following the procedure in "uCtrl Initialization and Mailbox Messaging" */
		/*   [dwc_ddrphy_phyinit_userCustom_G_waitFwDone] Wait for the training firmware to complete.  */
		/*   Implement timeout fucntion or follow the procedure in "3.4 Running the firmware" of the Training Firmware Application Note to poll the Mailbox message. */
		dwc_ddrphy_phyinit_userCustom_G_waitFwDone ();

		/*   [dwc_ddrphy_phyinit_userCustom_G_waitFwDone] End of dwc_ddrphy_phyinit_userCustom_G_waitFwDone() */
		/*   4.   Halt the microcontroller." */
		dwc_ddrphy_apb_wr(0xd0099, 0x1); /*  DWC_DDRPHYA_APBONLY0_MicroReset */
		/*   [dwc_ddrphy_phyinit_G_execFW] End of dwc_ddrphy_phyinit_G_execFW () */
		/*  */
		/*  */
		/*  ############################################################## */
		/*   */
		/*   (H) Read the Message Block results */
		/*   */
		/*   The procedure is as follows: */
		/*   */
		/*  ############################################################## */
		/*  */
		/*  */
		/*   1.	Enable access to the internal CSRs by setting the MicroContMuxSel CSR to 0. */
		dwc_ddrphy_apb_wr(0xd0000, 0x0); /*  DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
		/*  */
		/*  2. Read the Firmware Message Block to obtain the results from the training. */
		/*  This can be accomplished by issuing APB read commands to the DMEM addresses. */
		/*  Example: */
		/*  if (Train2D) */
		/*  { */
		/*    _read_2d_message_block_outputs_ */
		/*  } */
		/*  else */
		/*  { */
		/*    _read_1d_message_block_outputs_ */
		/*  } */
		dwc_ddrphy_phyinit_userCustom_H_readMsgBlock (0);

		/*  [dwc_ddrphy_phyinit_userCustom_H_readMsgBlock] End of dwc_ddrphy_phyinit_userCustom_H_readMsgBlock () */
		/*   3.	Isolate the APB access from the internal CSRs by setting the MicroContMuxSel CSR to 1. */
		dwc_ddrphy_apb_wr(0xd0000, 0x1); /*  DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
		/*   4.	If training is required at another frequency, repeat the operations starting at step (E). */
		/*   [dwc_ddrphy_phyinit_H_readMsgBlock] End of dwc_ddrphy_phyinit_H_readMsgBlock() */
		/*  */
		/*  */
		/*  ############################################################## */
		/*   */
		/*   Step (E) Set the PHY input clocks to the desired frequency for pstate 1 */
		/*   */
		/*   See PhyInit App Note for detailed description and function usage */
		/*   */
		/*  ############################################################## */
		/*  */
#ifdef DDR4_SW_FFC
		dwc_ddrphy_phyinit_userCustom_E_setDfiClk (1);

		/*  */
		/*   [dwc_ddrphy_phyinit_userCustom_E_setDfiClk] End of dwc_ddrphy_phyinit_userCustom_E_setDfiClk() */
		/*   [phyinit_F_loadDMEM, 1D] Start of dwc_ddrphy_phyinit_F_loadDMEM (pstate=1, Train2D=0) */
		/*  */
		/*  */
		/*  ############################################################## */
		/*   */
		/*   (F) Load the 1D DMEM image and write the 1D Message Block parameters for the training firmware */
		/*   */
		/*   See PhyInit App Note for detailed description and function usage */
		/*   */
		/*  ############################################################## */
		/*  */
		/*  [dwc_ddrphy_phyinit_storeIncvFile] Reading input file: ../../../../firmware/A-2017.09/ddr4/ddr4_pmu_train_dmem.incv */

		/*   1.	Enable access to the internal CSRs by setting the MicroContMuxSel CSR to 0. */
		/*         This allows the memory controller unrestricted access to the configuration CSRs. */
		dwc_ddrphy_apb_wr(0xd0000, 0x0); /*  DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
		/*   [dwc_ddrphy_phyinit_WriteOutMem] STARTING. offset 0x54000 size 0x36a */
#ifdef RUN_ON_SILICON
		dwc_ddrphy_apb_wr(0x54000, 0x0);
#else
		dwc_ddrphy_apb_wr(0x54000, 0x600);
#endif
		dwc_ddrphy_apb_wr(0x54001, 0x0);
		dwc_ddrphy_apb_wr(0x54002, 0x101);
		dwc_ddrphy_apb_wr(0x54003, 0x190);
		dwc_ddrphy_apb_wr(0x54004, 0x2);
		dwc_ddrphy_apb_wr(0x54005, 0x0);
		dwc_ddrphy_apb_wr(0x54006, 0x25e);
		dwc_ddrphy_apb_wr(0x54007, 0x2000);
#ifdef DDR_ONE_RANK
		dwc_ddrphy_apb_wr(0x54008, 0x101);
		dwc_ddrphy_apb_wr(0x54009, 0x0);
#else
		dwc_ddrphy_apb_wr(0x54008, 0x303);
		dwc_ddrphy_apb_wr(0x54009, 0x200);
#endif
		dwc_ddrphy_apb_wr(0x5400a, 0x0);
#ifdef RUN_ON_SILICON
		dwc_ddrphy_apb_wr(0x5400b, 0x21f);
#else
		dwc_ddrphy_apb_wr(0x5400b, 0x5);
#endif
		dwc_ddrphy_apb_wr(0x5400c, 0xc8);
		dwc_ddrphy_apb_wr(0x5400d, 0x0);
		dwc_ddrphy_apb_wr(0x5400e, 0x0);
		dwc_ddrphy_apb_wr(0x5400f, 0x0);
		dwc_ddrphy_apb_wr(0x54010, 0x0);
		dwc_ddrphy_apb_wr(0x54011, 0x0);
		dwc_ddrphy_apb_wr(0x54012, 0x1);
		dwc_ddrphy_apb_wr(0x5402f, mr_value[1][0]);
		dwc_ddrphy_apb_wr(0x54030, mr_value[1][1]);
		dwc_ddrphy_apb_wr(0x54031, mr_value[1][2]);
		dwc_ddrphy_apb_wr(0x54032, mr_value[1][3]);
		dwc_ddrphy_apb_wr(0x54033, mr_value[1][4]);
		dwc_ddrphy_apb_wr(0x54034, mr_value[1][5]);
		dwc_ddrphy_apb_wr(0x54035, mr_value[1][6]);

#ifdef DDR_ONE_RANK
		dwc_ddrphy_apb_wr(0x54036, 0x101);
#else
		dwc_ddrphy_apb_wr(0x54036, 0x103);
#endif
		dwc_ddrphy_apb_wr(0x54037, 0x0);
		dwc_ddrphy_apb_wr(0x54038, 0x0);
		dwc_ddrphy_apb_wr(0x54039, 0x0);
		dwc_ddrphy_apb_wr(0x5403a, 0x0);
		dwc_ddrphy_apb_wr(0x5403b, 0x0);
		dwc_ddrphy_apb_wr(0x5403c, 0x0);
		dwc_ddrphy_apb_wr(0x5403d, 0x0);
		dwc_ddrphy_apb_wr(0x5403e, 0x0);
		dwc_ddrphy_apb_wr(0x5403f, 0x1221);
		dwc_ddrphy_apb_wr(0x541fc, 0x100);
		/*   [dwc_ddrphy_phyinit_WriteOutMem] DONE.  Index 0x36a */
		/*   2.	Isolate the APB access from the internal CSRs by setting the MicroContMuxSel CSR to 1. */
		/*        This allows the firmware unrestricted access to the configuration CSRs. */
		dwc_ddrphy_apb_wr(0xd0000, 0x1); /*  DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
		/*   [phyinit_F_loadDMEM, 1D] End of dwc_ddrphy_phyinit_F_loadDMEM() */
		/*  */
		/*  */
		/*  ############################################################## */
		/*   */
		/*   (G) Execute the Training Firmware */
		/*   */
		/*   See PhyInit App Note for detailed description and function usage */
		/*   */
		/*  ############################################################## */
		/*  */
		/*  */
		/*   1.  Reset the firmware microcontroller by writing the MicroReset CSR to set the StallToMicro and */
		/*       ResetToMicro fields to 1 (all other fields should be zero). */
		/*       Then rewrite the CSR so that only the StallToMicro remains set (all other fields should be zero). */
		dwc_ddrphy_apb_wr(0xd0000, 0x1); /*  DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
		dwc_ddrphy_apb_wr(0xd0099, 0x9); /*  DWC_DDRPHYA_APBONLY0_MicroReset */
		dwc_ddrphy_apb_wr(0xd0099, 0x1); /*  DWC_DDRPHYA_APBONLY0_MicroReset */
		/*  */
		/*   2. Begin execution of the training firmware by setting the MicroReset CSR to 4'b0000. */
		dwc_ddrphy_apb_wr(0xd0099, 0x0); /*  DWC_DDRPHYA_APBONLY0_MicroReset */
		/*  */
		/*   3.   Wait for the training firmware to complete by following the procedure in "uCtrl Initialization and Mailbox Messaging" */
		/*   [dwc_ddrphy_phyinit_userCustom_G_waitFwDone] Wait for the training firmware to complete.  Implement timeout fucntion or follow the procedure in "3.4 Running the firmware" of the Training Firmware Application Note to poll the Mailbox message. */
		dwc_ddrphy_phyinit_userCustom_G_waitFwDone ();

		/*   [dwc_ddrphy_phyinit_userCustom_G_waitFwDone] End of dwc_ddrphy_phyinit_userCustom_G_waitFwDone() */
		/*   4.   Halt the microcontroller." */
		dwc_ddrphy_apb_wr(0xd0099, 0x1); /*  DWC_DDRPHYA_APBONLY0_MicroReset */
		/*   [dwc_ddrphy_phyinit_G_execFW] End of dwc_ddrphy_phyinit_G_execFW () */
		/*  */
		/*  */
		/*  ############################################################## */
		/*   */
		/*   (H) Read the Message Block results */
		/*   */
		/*   The procedure is as follows: */
		/*   */
		/*  ############################################################## */
		/*  */
		/*  */
		/*   1.	Enable access to the internal CSRs by setting the MicroContMuxSel CSR to 0. */
		dwc_ddrphy_apb_wr(0xd0000, 0x0); /*  DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
		/*  */
		/*  2. Read the Firmware Message Block to obtain the results from the training. */
		/*  This can be accomplished by issuing APB read commands to the DMEM addresses. */
		/*  Example: */
		/*  if (Train2D) */
		/*  { */
		/*    _read_2d_message_block_outputs_ */
		/*  } */
		/*  else */
		/*  { */
		/*    _read_1d_message_block_outputs_ */
		/*  } */
		dwc_ddrphy_phyinit_userCustom_H_readMsgBlock (0);

		/*  [dwc_ddrphy_phyinit_userCustom_H_readMsgBlock] End of dwc_ddrphy_phyinit_userCustom_H_readMsgBlock () */
		/*   3.	Isolate the APB access from the internal CSRs by setting the MicroContMuxSel CSR to 1. */
		dwc_ddrphy_apb_wr(0xd0000, 0x1); /*  DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
		/*   4.	If training is required at another frequency, repeat the operations starting at step (E). */
		/*   [dwc_ddrphy_phyinit_H_readMsgBlock] End of dwc_ddrphy_phyinit_H_readMsgBlock() */
		/*  */
		/*  */
		/*  ############################################################## */
		/*   */
		/*   Step (E) Set the PHY input clocks to the desired frequency for pstate 2 */
		/*   */
		/*   See PhyInit App Note for detailed description and function usage */
		/*   */
		/*  ############################################################## */
		/*  */
		dwc_ddrphy_phyinit_userCustom_E_setDfiClk (2);

		/*  */
		/*   [dwc_ddrphy_phyinit_userCustom_E_setDfiClk] End of dwc_ddrphy_phyinit_userCustom_E_setDfiClk() */
		/*   [phyinit_F_loadDMEM, 1D] Start of dwc_ddrphy_phyinit_F_loadDMEM (pstate=2, Train2D=0) */
		/*  */
		/*  */
		/*  ############################################################## */
		/*   */
		/*   (F) Load the 1D DMEM image and write the 1D Message Block parameters for the training firmware */
		/*   */
		/*   See PhyInit App Note for detailed description and function usage */
		/*   */
		/*  ############################################################## */
		/*  */
		/*  [dwc_ddrphy_phyinit_storeIncvFile] Reading input file: ../../../../firmware/A-2017.09/ddr4/ddr4_pmu_train_dmem.incv */

		/*   1.	Enable access to the internal CSRs by setting the MicroContMuxSel CSR to 0. */
		/*         This allows the memory controller unrestricted access to the configuration CSRs. */
		dwc_ddrphy_apb_wr(0xd0000, 0x0); /*  DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
		/*   [dwc_ddrphy_phyinit_WriteOutMem] STARTING. offset 0x54000 size 0x36a */
#ifdef RUN_ON_SILICON
		dwc_ddrphy_apb_wr(0x54000, 0x0);
#else
		dwc_ddrphy_apb_wr(0x54000, 0x600);
#endif
		dwc_ddrphy_apb_wr(0x54001, 0x0);
		dwc_ddrphy_apb_wr(0x54002, 0x102);
		dwc_ddrphy_apb_wr(0x54003, 0x64);
		dwc_ddrphy_apb_wr(0x54004, 0x2);
		dwc_ddrphy_apb_wr(0x54005, 0x0);
		dwc_ddrphy_apb_wr(0x54006, 0x25e);
		dwc_ddrphy_apb_wr(0x54007, 0x2000);
#ifdef DDR_ONE_RANK
		dwc_ddrphy_apb_wr(0x54008, 0x101);
		dwc_ddrphy_apb_wr(0x54009, 0x0);
#else
		dwc_ddrphy_apb_wr(0x54008, 0x303);
		dwc_ddrphy_apb_wr(0x54009, 0x200);
#endif
		dwc_ddrphy_apb_wr(0x5400a, 0x0);
#ifdef RUN_ON_SILICON
		dwc_ddrphy_apb_wr(0x5400b, 0x21f);
#else
		dwc_ddrphy_apb_wr(0x5400b, 0x5);
#endif
		dwc_ddrphy_apb_wr(0x5400c, 0xc8);
		dwc_ddrphy_apb_wr(0x5400d, 0x0);
		dwc_ddrphy_apb_wr(0x5400e, 0x0);
		dwc_ddrphy_apb_wr(0x5400f, 0x0);
		dwc_ddrphy_apb_wr(0x54010, 0x0);
		dwc_ddrphy_apb_wr(0x54011, 0x0);
		dwc_ddrphy_apb_wr(0x54012, 0x1);
		dwc_ddrphy_apb_wr(0x5402f, mr_value[2][0]);
		dwc_ddrphy_apb_wr(0x54030, mr_value[2][1]);
		dwc_ddrphy_apb_wr(0x54031, mr_value[2][2]);
		dwc_ddrphy_apb_wr(0x54032, mr_value[2][3]);
		dwc_ddrphy_apb_wr(0x54033, mr_value[2][4]);
		dwc_ddrphy_apb_wr(0x54034, mr_value[2][5]);
		dwc_ddrphy_apb_wr(0x54035, mr_value[2][6]);

#ifdef DDR_ONE_RANK
		dwc_ddrphy_apb_wr(0x54036, 0x101);
#else
		dwc_ddrphy_apb_wr(0x54036, 0x103);
#endif
		dwc_ddrphy_apb_wr(0x54037, 0x0);
		dwc_ddrphy_apb_wr(0x54038, 0x0);
		dwc_ddrphy_apb_wr(0x54039, 0x0);
		dwc_ddrphy_apb_wr(0x5403a, 0x0);
		dwc_ddrphy_apb_wr(0x5403b, 0x0);
		dwc_ddrphy_apb_wr(0x5403c, 0x0);
		dwc_ddrphy_apb_wr(0x5403d, 0x0);
		dwc_ddrphy_apb_wr(0x5403e, 0x0);
		dwc_ddrphy_apb_wr(0x5403f, 0x1221);
		dwc_ddrphy_apb_wr(0x541fc, 0x100);
		/*   [dwc_ddrphy_phyinit_WriteOutMem] DONE.  Index 0x36a */
		/*   2.	Isolate the APB access from the internal CSRs by setting the MicroContMuxSel CSR to 1. */
		/*        This allows the firmware unrestricted access to the configuration CSRs. */
		dwc_ddrphy_apb_wr(0xd0000, 0x1); /*  DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
		/*   [phyinit_F_loadDMEM, 1D] End of dwc_ddrphy_phyinit_F_loadDMEM() */
		/*  */
		/*  */
		/*  ############################################################## */
		/*   */
		/*   (G) Execute the Training Firmware */
		/*   */
		/*   See PhyInit App Note for detailed description and function usage */
		/*   */
		/*  ############################################################## */
		/*  */
		/*  */
		/*   1.  Reset the firmware microcontroller by writing the MicroReset CSR to set the StallToMicro and */
		/*       ResetToMicro fields to 1 (all other fields should be zero). */
		/*       Then rewrite the CSR so that only the StallToMicro remains set (all other fields should be zero). */
		dwc_ddrphy_apb_wr(0xd0000, 0x1); /*  DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
		dwc_ddrphy_apb_wr(0xd0099, 0x9); /*  DWC_DDRPHYA_APBONLY0_MicroReset */
		dwc_ddrphy_apb_wr(0xd0099, 0x1); /*  DWC_DDRPHYA_APBONLY0_MicroReset */
		/*  */
		/*   2. Begin execution of the training firmware by setting the MicroReset CSR to 4'b0000. */
		dwc_ddrphy_apb_wr(0xd0099, 0x0); /*  DWC_DDRPHYA_APBONLY0_MicroReset */
		/*  */
		/*   3.   Wait for the training firmware to complete by following the procedure in "uCtrl Initialization and Mailbox Messaging" */
		/*   [dwc_ddrphy_phyinit_userCustom_G_waitFwDone] Wait for the training firmware to complete.  Implement timeout fucntion or follow the procedure in "3.4 Running the firmware" of the Training Firmware Application Note to poll the Mailbox message. */
		dwc_ddrphy_phyinit_userCustom_G_waitFwDone ();

		/*   [dwc_ddrphy_phyinit_userCustom_G_waitFwDone] End of dwc_ddrphy_phyinit_userCustom_G_waitFwDone() */
		/*   4.   Halt the microcontroller." */
		dwc_ddrphy_apb_wr(0xd0099, 0x1); /*  DWC_DDRPHYA_APBONLY0_MicroReset */
		/*   [dwc_ddrphy_phyinit_G_execFW] End of dwc_ddrphy_phyinit_G_execFW () */
		/*  */
		/*  */
		/*  ############################################################## */
		/*   */
		/*   (H) Read the Message Block results */
		/*   */
		/*   The procedure is as follows: */
		/*   */
		/*  ############################################################## */
		/*  */
		/*  */
		/*   1.	Enable access to the internal CSRs by setting the MicroContMuxSel CSR to 0. */
		dwc_ddrphy_apb_wr(0xd0000, 0x0); /*  DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
		/*  */
		/*  2. Read the Firmware Message Block to obtain the results from the training. */
		/*  This can be accomplished by issuing APB read commands to the DMEM addresses. */
		/*  Example: */
		/*  if (Train2D) */
		/*  { */
		/*    _read_2d_message_block_outputs_ */
		/*  } */
		/*  else */
		/*  { */
		/*    _read_1d_message_block_outputs_ */
		/*  } */
		dwc_ddrphy_phyinit_userCustom_H_readMsgBlock (0);

		/*  [dwc_ddrphy_phyinit_userCustom_H_readMsgBlock] End of dwc_ddrphy_phyinit_userCustom_H_readMsgBlock () */
		/*   3.	Isolate the APB access from the internal CSRs by setting the MicroContMuxSel CSR to 1. */
		dwc_ddrphy_apb_wr(0xd0000, 0x1); /*  DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
		/*   4.	If training is required at another frequency, repeat the operations starting at step (E). */
		/*   [dwc_ddrphy_phyinit_H_readMsgBlock] End of dwc_ddrphy_phyinit_H_readMsgBlock() */
		/*  */
		/*  */
		/*  ############################################################## */
		/*   */
		/*   Step (E) Set the PHY input clocks to the desired frequency for pstate 0 */
		/*   */
		/*   See PhyInit App Note for detailed description and function usage */
		/*   */
		/*  ############################################################## */
		/*  */
		dwc_ddrphy_phyinit_userCustom_E_setDfiClk (0);
#endif /* DDR4_SW_FFC */

		/*  */
		/*   [dwc_ddrphy_phyinit_userCustom_E_setDfiClk] End of dwc_ddrphy_phyinit_userCustom_E_setDfiClk() */
		/*   [dwc_ddrphy_phyinit_D_loadIMEM, 2D] Start of dwc_ddrphy_phyinit_D_loadIMEM (Train2D=1) */
		/*  */
		/*  */
		/*  ############################################################## */
		/*   */
		/*   (D) Load the 2D IMEM image */
		/*   */
		/*   This function loads the training firmware IMEM image into the SRAM. */
		/*   See PhyInit App Note for detailed description and function usage */
		/*   */
		/*  ############################################################## */
		/*  */
		/*  */
		/*  [dwc_ddrphy_phyinit_storeIncvFile] Reading input file: ../../../../firmware/A-2017.09/ddr4_2d/ddr4_2d_pmu_train_imem.incv */

		/*   1.	Enable access to the internal CSRs by setting the MicroContMuxSel CSR to 0. */
		/*         This allows the memory controller unrestricted access to the configuration CSRs. */
		dwc_ddrphy_apb_wr(0xd0000, 0x0); /*  DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
		/*   [dwc_ddrphy_phyinit_WriteOutMem] STARTING. offset 0x50000 size 0x4000 */
		/*   [dwc_ddrphy_phyinit_WriteOutMem] DONE.  Index 0x4000 */
		/*   2.	Isolate the APB access from the internal CSRs by setting the MicroContMuxSel CSR to 1. */
		/*        This allows the firmware unrestricted access to the configuration CSRs. */
		dwc_ddrphy_apb_wr(0xd0000, 0x1); /*  DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
		/*   [dwc_ddrphy_phyinit_D_loadIMEM, 2D] End of dwc_ddrphy_phyinit_D_loadIMEM() */
		/*   [phyinit_F_loadDMEM, 2D] Start of dwc_ddrphy_phyinit_F_loadDMEM (pstate=0, Train2D=1) */
		/*  */
		/*  */
		/*  ############################################################## */
		/*   */
		/*   (F) Load the 2D DMEM image and write the 2D Message Block parameters for the training firmware */
		/*   */
		/*   See PhyInit App Note for detailed description and function usage */
		/*   */
		/*  ############################################################## */
		/*  */
		/*  [dwc_ddrphy_phyinit_storeIncvFile] Reading input file: ../../../../firmware/A-2017.09/ddr4_2d/ddr4_2d_pmu_train_dmem.incv */

		ddr_dbg("C: 1D training done!!! \n");

		if (Train2D) {
			/*  for test on silicon, load 2D dmem/imem here */
			dwc_ddrphy_apb_wr(0xd0000, 0x0); /*  DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
#ifdef CONFIG_SPL_VSI_FW_LOADING
			load_train_2d_code();
#else
			ddr_load_train_code(FW_2D_IMAGE);
#endif
			ddr_dbg("start 2d train\n");

			/*   1.	Enable access to the internal CSRs by setting the MicroContMuxSel CSR to 0. */
			/*         This allows the memory controller unrestricted access to the configuration CSRs. */
			dwc_ddrphy_apb_wr(0xd0000, 0x0); /*  DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
			/*   [dwc_ddrphy_phyinit_WriteOutMem] STARTING. offset 0x54000 size 0x2d6 */
#ifdef RUN_ON_SILICON
			dwc_ddrphy_apb_wr(0x54000, 0x0);
#else
			dwc_ddrphy_apb_wr(0x54000, 0x600);
#endif
			dwc_ddrphy_apb_wr(0x54001, 0x0);
			dwc_ddrphy_apb_wr(0x54002, 0x0);
			dwc_ddrphy_apb_wr(0x54003, 0x960);
			dwc_ddrphy_apb_wr(0x54004, 0x2);
			dwc_ddrphy_apb_wr(0x54005, 0x0);
			dwc_ddrphy_apb_wr(0x54006, 0x25e);
			dwc_ddrphy_apb_wr(0x54007, 0x2000);
#ifdef DDR_ONE_RANK
			dwc_ddrphy_apb_wr(0x54008, 0x101);
			dwc_ddrphy_apb_wr(0x54009, 0x0);
#else
			dwc_ddrphy_apb_wr(0x54008, 0x303);
			dwc_ddrphy_apb_wr(0x54009, 0x200);
#endif
			dwc_ddrphy_apb_wr(0x5400a, 0x0);
#ifdef RUN_ON_SILICON
			dwc_ddrphy_apb_wr(0x5400b, 0x61);
#else
			dwc_ddrphy_apb_wr(0x5400b, 0x1);
#endif
			dwc_ddrphy_apb_wr(0x5400c, 0xc8);
			dwc_ddrphy_apb_wr(0x5400d, 0x100);
			dwc_ddrphy_apb_wr(0x5400e, 0x1f7f);
			dwc_ddrphy_apb_wr(0x5400f, 0x0);
			dwc_ddrphy_apb_wr(0x54010, 0x0);
			dwc_ddrphy_apb_wr(0x54011, 0x0);
			dwc_ddrphy_apb_wr(0x54012, 0x1);
			dwc_ddrphy_apb_wr(0x5402f, mr_value[0][0]);
			dwc_ddrphy_apb_wr(0x54030, mr_value[0][1]);
			dwc_ddrphy_apb_wr(0x54031, mr_value[0][2]);
			dwc_ddrphy_apb_wr(0x54032, mr_value[0][3]);
			dwc_ddrphy_apb_wr(0x54033, mr_value[0][4]);
			dwc_ddrphy_apb_wr(0x54034, mr_value[0][5]);
			dwc_ddrphy_apb_wr(0x54035, mr_value[0][6]);
#ifdef DDR_ONE_RANK
			dwc_ddrphy_apb_wr(0x54036, 0x101);
#else
			dwc_ddrphy_apb_wr(0x54036, 0x103);
#endif
			dwc_ddrphy_apb_wr(0x54037, 0x0);
			dwc_ddrphy_apb_wr(0x54038, 0x0);
			dwc_ddrphy_apb_wr(0x54039, 0x0);
			dwc_ddrphy_apb_wr(0x5403a, 0x0);
			dwc_ddrphy_apb_wr(0x5403b, 0x0);
			dwc_ddrphy_apb_wr(0x5403c, 0x0);
			dwc_ddrphy_apb_wr(0x5403d, 0x0);
			dwc_ddrphy_apb_wr(0x5403e, 0x0);
			dwc_ddrphy_apb_wr(0x5403f, 0x1221);
			dwc_ddrphy_apb_wr(0x541fc, 0x100);
			/*   [dwc_ddrphy_phyinit_WriteOutMem] DONE.  Index 0x2d6 */
			/*   2.	Isolate the APB access from the internal CSRs by setting the MicroContMuxSel CSR to 1. */
			/*        This allows the firmware unrestricted access to the configuration CSRs. */
			dwc_ddrphy_apb_wr(0xd0000, 0x1); /*  DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
			/*   [phyinit_F_loadDMEM, 2D] End of dwc_ddrphy_phyinit_F_loadDMEM() */
			/*  */
			/*  */
			/*  ############################################################## */
			/*   */
			/*   (G) Execute the Training Firmware */
			/*   */
			/*   See PhyInit App Note for detailed description and function usage */
			/*   */
			/*  ############################################################## */
			/*  */
			/*  */
			/*   1.  Reset the firmware microcontroller by writing the MicroReset CSR to set the StallToMicro and */
			/*       ResetToMicro fields to 1 (all other fields should be zero). */
			/*       Then rewrite the CSR so that only the StallToMicro remains set (all other fields should be zero). */
			dwc_ddrphy_apb_wr(0xd0000, 0x1); /*  DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
			dwc_ddrphy_apb_wr(0xd0099, 0x9); /*  DWC_DDRPHYA_APBONLY0_MicroReset */
			dwc_ddrphy_apb_wr(0xd0099, 0x1); /*  DWC_DDRPHYA_APBONLY0_MicroReset */
			/*  */
			/*   2. Begin execution of the training firmware by setting the MicroReset CSR to 4'b0000. */
			dwc_ddrphy_apb_wr(0xd0099, 0x0); /*  DWC_DDRPHYA_APBONLY0_MicroReset */
			/*  */
			/*   3.   Wait for the training firmware to complete by following the procedure in "uCtrl Initialization and Mailbox Messaging" */
			/*   [dwc_ddrphy_phyinit_userCustom_G_waitFwDone] Wait for the training firmware to complete.  Implement timeout fucntion or follow the procedure in "3.4 Running the firmware" of the Training Firmware Application Note to poll the Mailbox message. */
			dwc_ddrphy_phyinit_userCustom_G_waitFwDone ();

			/*   [dwc_ddrphy_phyinit_userCustom_G_waitFwDone] End of dwc_ddrphy_phyinit_userCustom_G_waitFwDone() */
			/*   4.   Halt the microcontroller." */
			dwc_ddrphy_apb_wr(0xd0099, 0x1); /*  DWC_DDRPHYA_APBONLY0_MicroReset */
			/*   [dwc_ddrphy_phyinit_G_execFW] End of dwc_ddrphy_phyinit_G_execFW () */
			/*  */
			/*  */
			/*  ############################################################## */
			/*   */
			/*   (H) Read the Message Block results */
			/*   */
			/*   The procedure is as follows: */
			/*   */
			/*  ############################################################## */
			/*  */
			/*  */
			/*   1.	Enable access to the internal CSRs by setting the MicroContMuxSel CSR to 0. */
			dwc_ddrphy_apb_wr(0xd0000, 0x0); /*  DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
			/*  */
			/*  2. Read the Firmware Message Block to obtain the results from the training. */
			/*  This can be accomplished by issuing APB read commands to the DMEM addresses. */
			/*  Example: */
			/*  if (Train2D) */
			/*  { */
			/*    _read_2d_message_block_outputs_ */
			/*  } */
			/*  else */
			/*  { */
			/*    _read_1d_message_block_outputs_ */
			/*  } */
			dwc_ddrphy_phyinit_userCustom_H_readMsgBlock (1);

			/*  [dwc_ddrphy_phyinit_userCustom_H_readMsgBlock] End of dwc_ddrphy_phyinit_userCustom_H_readMsgBlock () */
			/*   3.	Isolate the APB access from the internal CSRs by setting the MicroContMuxSel CSR to 1. */
			dwc_ddrphy_apb_wr(0xd0000, 0x1); /*  DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
			/*   4.	If training is required at another frequency, repeat the operations starting at step (E). */
			/*   [dwc_ddrphy_phyinit_H_readMsgBlock] End of dwc_ddrphy_phyinit_H_readMsgBlock() */
			/*   [phyinit_I_loadPIEImage] Start of dwc_ddrphy_phyinit_I_loadPIEImage() */
			ddr_dbg("2D training done!!!!\n");

		} /* Train2D */
	} /* !after_retention */
#ifdef ENABLE_RETENTION
	else { /* after_retention */
		restore_1d2d_trained_csr_ddr4_p012(SAVE_DDRPHY_TRAIN_ADDR);
	} /* after_retention */
#endif

	/*  */
	/*  */
	/*  ############################################################## */
	/*   */
	/*   (I) Load PHY Init Engine Image */
	/*   */
	/*   Load the PHY Initialization Engine memory with the provided initialization sequence. */
	/*   See PhyInit App Note for detailed description and function usage */
	/*   */
	/*   */
	/*  ############################################################## */
	/*  */
	/*  */
	/*   Enable access to the internal CSRs by setting the MicroContMuxSel CSR to 0. */
	/*   This allows the memory controller unrestricted access to the configuration CSRs. */
	dwc_ddrphy_apb_wr(0xd0000, 0x0); /*  DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
	/*   [phyinit_I_loadPIEImage] Programming PIE Production Code */
	dwc_ddrphy_apb_wr(0x90000, 0x10); /*  DWC_DDRPHYA_INITENG0_PreSequenceReg0b0s0 */
	dwc_ddrphy_apb_wr(0x90001, 0x400); /*  DWC_DDRPHYA_INITENG0_PreSequenceReg0b0s1 */
	dwc_ddrphy_apb_wr(0x90002, 0x10e); /*  DWC_DDRPHYA_INITENG0_PreSequenceReg0b0s2 */
	dwc_ddrphy_apb_wr(0x90003, 0x0); /*  DWC_DDRPHYA_INITENG0_PreSequenceReg0b1s0 */
	dwc_ddrphy_apb_wr(0x90004, 0x0); /*  DWC_DDRPHYA_INITENG0_PreSequenceReg0b1s1 */
	dwc_ddrphy_apb_wr(0x90005, 0x8); /*  DWC_DDRPHYA_INITENG0_PreSequenceReg0b1s2 */
	dwc_ddrphy_apb_wr(0x90029, 0xb); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b0s0 */
	dwc_ddrphy_apb_wr(0x9002a, 0x480); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b0s1 */
	dwc_ddrphy_apb_wr(0x9002b, 0x109); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b0s2 */
	dwc_ddrphy_apb_wr(0x9002c, 0x8); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b1s0 */
	dwc_ddrphy_apb_wr(0x9002d, 0x448); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b1s1 */
	dwc_ddrphy_apb_wr(0x9002e, 0x139); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b1s2 */
	dwc_ddrphy_apb_wr(0x9002f, 0x8); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b2s0 */
	dwc_ddrphy_apb_wr(0x90030, 0x478); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b2s1 */
	dwc_ddrphy_apb_wr(0x90031, 0x109); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b2s2 */
	dwc_ddrphy_apb_wr(0x90032, 0x2); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b3s0 */
	dwc_ddrphy_apb_wr(0x90033, 0x10); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b3s1 */
	dwc_ddrphy_apb_wr(0x90034, 0x139); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b3s2 */
	dwc_ddrphy_apb_wr(0x90035, 0xf); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b4s0 */
	dwc_ddrphy_apb_wr(0x90036, 0x7c0); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b4s1 */
	dwc_ddrphy_apb_wr(0x90037, 0x139); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b4s2 */
	dwc_ddrphy_apb_wr(0x90038, 0x44); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b5s0 */
	dwc_ddrphy_apb_wr(0x90039, 0x630); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b5s1 */
	dwc_ddrphy_apb_wr(0x9003a, 0x159); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b5s2 */
	dwc_ddrphy_apb_wr(0x9003b, 0x14f); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b6s0 */
	dwc_ddrphy_apb_wr(0x9003c, 0x630); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b6s1 */
	dwc_ddrphy_apb_wr(0x9003d, 0x159); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b6s2 */
	dwc_ddrphy_apb_wr(0x9003e, 0x47); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b7s0 */
	dwc_ddrphy_apb_wr(0x9003f, 0x630); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b7s1 */
	dwc_ddrphy_apb_wr(0x90040, 0x149); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b7s2 */
	dwc_ddrphy_apb_wr(0x90041, 0x4f); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b8s0 */
	dwc_ddrphy_apb_wr(0x90042, 0x630); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b8s1 */
	dwc_ddrphy_apb_wr(0x90043, 0x179); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b8s2 */
	dwc_ddrphy_apb_wr(0x90044, 0x8); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b9s0 */
	dwc_ddrphy_apb_wr(0x90045, 0xe0); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b9s1 */
	dwc_ddrphy_apb_wr(0x90046, 0x109); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b9s2 */
	dwc_ddrphy_apb_wr(0x90047, 0x0); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b10s0 */
	dwc_ddrphy_apb_wr(0x90048, 0x7c8); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b10s1 */
	dwc_ddrphy_apb_wr(0x90049, 0x109); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b10s2 */
	dwc_ddrphy_apb_wr(0x9004a, 0x0); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b11s0 */
	dwc_ddrphy_apb_wr(0x9004b, 0x1); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b11s1 */
	dwc_ddrphy_apb_wr(0x9004c, 0x8); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b11s2 */
	dwc_ddrphy_apb_wr(0x9004d, 0x0); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b12s0 */
	dwc_ddrphy_apb_wr(0x9004e, 0x45a); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b12s1 */
	dwc_ddrphy_apb_wr(0x9004f, 0x9); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b12s2 */
	dwc_ddrphy_apb_wr(0x90050, 0x0); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b13s0 */
	dwc_ddrphy_apb_wr(0x90051, 0x448); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b13s1 */
	dwc_ddrphy_apb_wr(0x90052, 0x109); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b13s2 */
	dwc_ddrphy_apb_wr(0x90053, 0x40); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b14s0 */
	dwc_ddrphy_apb_wr(0x90054, 0x630); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b14s1 */
	dwc_ddrphy_apb_wr(0x90055, 0x179); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b14s2 */
	dwc_ddrphy_apb_wr(0x90056, 0x1); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b15s0 */
	dwc_ddrphy_apb_wr(0x90057, 0x618); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b15s1 */
	dwc_ddrphy_apb_wr(0x90058, 0x109); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b15s2 */
	dwc_ddrphy_apb_wr(0x90059, 0x40c0); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b16s0 */
	dwc_ddrphy_apb_wr(0x9005a, 0x630); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b16s1 */
	dwc_ddrphy_apb_wr(0x9005b, 0x149); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b16s2 */
	dwc_ddrphy_apb_wr(0x9005c, 0x8); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b17s0 */
	dwc_ddrphy_apb_wr(0x9005d, 0x4); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b17s1 */
	dwc_ddrphy_apb_wr(0x9005e, 0x48); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b17s2 */
	dwc_ddrphy_apb_wr(0x9005f, 0x4040); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b18s0 */
	dwc_ddrphy_apb_wr(0x90060, 0x630); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b18s1 */
	dwc_ddrphy_apb_wr(0x90061, 0x149); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b18s2 */
	dwc_ddrphy_apb_wr(0x90062, 0x0); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b19s0 */
	dwc_ddrphy_apb_wr(0x90063, 0x4); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b19s1 */
	dwc_ddrphy_apb_wr(0x90064, 0x48); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b19s2 */
	dwc_ddrphy_apb_wr(0x90065, 0x40); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b20s0 */
	dwc_ddrphy_apb_wr(0x90066, 0x630); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b20s1 */
	dwc_ddrphy_apb_wr(0x90067, 0x149); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b20s2 */
	dwc_ddrphy_apb_wr(0x90068, 0x10); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b21s0 */
	dwc_ddrphy_apb_wr(0x90069, 0x4); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b21s1 */
	dwc_ddrphy_apb_wr(0x9006a, 0x18); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b21s2 */
	dwc_ddrphy_apb_wr(0x9006b, 0x0); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b22s0 */
	dwc_ddrphy_apb_wr(0x9006c, 0x4); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b22s1 */
	dwc_ddrphy_apb_wr(0x9006d, 0x78); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b22s2 */
	dwc_ddrphy_apb_wr(0x9006e, 0x549); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b23s0 */
	dwc_ddrphy_apb_wr(0x9006f, 0x630); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b23s1 */
	dwc_ddrphy_apb_wr(0x90070, 0x159); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b23s2 */
	dwc_ddrphy_apb_wr(0x90071, 0xd49); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b24s0 */
	dwc_ddrphy_apb_wr(0x90072, 0x630); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b24s1 */
	dwc_ddrphy_apb_wr(0x90073, 0x159); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b24s2 */
	dwc_ddrphy_apb_wr(0x90074, 0x94a); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b25s0 */
	dwc_ddrphy_apb_wr(0x90075, 0x630); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b25s1 */
	dwc_ddrphy_apb_wr(0x90076, 0x159); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b25s2 */
	dwc_ddrphy_apb_wr(0x90077, 0x441); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b26s0 */
	dwc_ddrphy_apb_wr(0x90078, 0x630); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b26s1 */
	dwc_ddrphy_apb_wr(0x90079, 0x149); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b26s2 */
	dwc_ddrphy_apb_wr(0x9007a, 0x42); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b27s0 */
	dwc_ddrphy_apb_wr(0x9007b, 0x630); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b27s1 */
	dwc_ddrphy_apb_wr(0x9007c, 0x149); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b27s2 */
	dwc_ddrphy_apb_wr(0x9007d, 0x1); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b28s0 */
	dwc_ddrphy_apb_wr(0x9007e, 0x630); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b28s1 */
	dwc_ddrphy_apb_wr(0x9007f, 0x149); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b28s2 */
	dwc_ddrphy_apb_wr(0x90080, 0x0); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b29s0 */
	dwc_ddrphy_apb_wr(0x90081, 0xe0); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b29s1 */
	dwc_ddrphy_apb_wr(0x90082, 0x109); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b29s2 */
	dwc_ddrphy_apb_wr(0x90083, 0xa); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b30s0 */
	dwc_ddrphy_apb_wr(0x90084, 0x10); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b30s1 */
	dwc_ddrphy_apb_wr(0x90085, 0x109); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b30s2 */
	dwc_ddrphy_apb_wr(0x90086, 0x9); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b31s0 */
	dwc_ddrphy_apb_wr(0x90087, 0x3c0); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b31s1 */
	dwc_ddrphy_apb_wr(0x90088, 0x149); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b31s2 */
	dwc_ddrphy_apb_wr(0x90089, 0x9); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b32s0 */
	dwc_ddrphy_apb_wr(0x9008a, 0x3c0); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b32s1 */
	dwc_ddrphy_apb_wr(0x9008b, 0x159); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b32s2 */
	dwc_ddrphy_apb_wr(0x9008c, 0x18); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b33s0 */
	dwc_ddrphy_apb_wr(0x9008d, 0x10); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b33s1 */
	dwc_ddrphy_apb_wr(0x9008e, 0x109); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b33s2 */
	dwc_ddrphy_apb_wr(0x9008f, 0x0); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b34s0 */
	dwc_ddrphy_apb_wr(0x90090, 0x3c0); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b34s1 */
	dwc_ddrphy_apb_wr(0x90091, 0x109); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b34s2 */
	dwc_ddrphy_apb_wr(0x90092, 0x18); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b35s0 */
	dwc_ddrphy_apb_wr(0x90093, 0x4); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b35s1 */
	dwc_ddrphy_apb_wr(0x90094, 0x48); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b35s2 */
	dwc_ddrphy_apb_wr(0x90095, 0x18); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b36s0 */
	dwc_ddrphy_apb_wr(0x90096, 0x4); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b36s1 */
	dwc_ddrphy_apb_wr(0x90097, 0x58); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b36s2 */
	dwc_ddrphy_apb_wr(0x90098, 0xa); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b37s0 */
	dwc_ddrphy_apb_wr(0x90099, 0x10); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b37s1 */
	dwc_ddrphy_apb_wr(0x9009a, 0x109); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b37s2 */
	dwc_ddrphy_apb_wr(0x9009b, 0x2); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b38s0 */
	dwc_ddrphy_apb_wr(0x9009c, 0x10); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b38s1 */
	dwc_ddrphy_apb_wr(0x9009d, 0x109); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b38s2 */
	dwc_ddrphy_apb_wr(0x9009e, 0x7); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b39s0 */
	dwc_ddrphy_apb_wr(0x9009f, 0x7c0); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b39s1 */
	dwc_ddrphy_apb_wr(0x900a0, 0x109); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b39s2 */
	dwc_ddrphy_apb_wr(0x900a1, 0x10); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b40s0 */
	dwc_ddrphy_apb_wr(0x900a2, 0x10); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b40s1 */
	dwc_ddrphy_apb_wr(0x900a3, 0x109); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b40s2 */
	dwc_ddrphy_apb_wr(0x900a4, 0x0); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b41s0 */
	dwc_ddrphy_apb_wr(0x900a5, 0x8140); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b41s1 */
	dwc_ddrphy_apb_wr(0x900a6, 0x10c); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b41s2 */
	dwc_ddrphy_apb_wr(0x900a7, 0x10); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b42s0 */
	dwc_ddrphy_apb_wr(0x900a8, 0x8138); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b42s1 */
	dwc_ddrphy_apb_wr(0x900a9, 0x10c); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b42s2 */
	dwc_ddrphy_apb_wr(0x900aa, 0x8); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b43s0 */
	dwc_ddrphy_apb_wr(0x900ab, 0x7c8); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b43s1 */
	dwc_ddrphy_apb_wr(0x900ac, 0x101); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b43s2 */
	dwc_ddrphy_apb_wr(0x900ad, 0x8); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b44s0 */
	dwc_ddrphy_apb_wr(0x900ae, 0x0); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b44s1 */
	dwc_ddrphy_apb_wr(0x900af, 0x8); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b44s2 */
	dwc_ddrphy_apb_wr(0x900b0, 0x8); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b45s0 */
	dwc_ddrphy_apb_wr(0x900b1, 0x448); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b45s1 */
	dwc_ddrphy_apb_wr(0x900b2, 0x109); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b45s2 */
	dwc_ddrphy_apb_wr(0x900b3, 0xf); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b46s0 */
	dwc_ddrphy_apb_wr(0x900b4, 0x7c0); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b46s1 */
	dwc_ddrphy_apb_wr(0x900b5, 0x109); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b46s2 */
	dwc_ddrphy_apb_wr(0x900b6, 0x47); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b47s0 */
	dwc_ddrphy_apb_wr(0x900b7, 0x630); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b47s1 */
	dwc_ddrphy_apb_wr(0x900b8, 0x109); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b47s2 */
	dwc_ddrphy_apb_wr(0x900b9, 0x8); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b48s0 */
	dwc_ddrphy_apb_wr(0x900ba, 0x618); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b48s1 */
	dwc_ddrphy_apb_wr(0x900bb, 0x109); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b48s2 */
	dwc_ddrphy_apb_wr(0x900bc, 0x8); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b49s0 */
	dwc_ddrphy_apb_wr(0x900bd, 0xe0); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b49s1 */
	dwc_ddrphy_apb_wr(0x900be, 0x109); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b49s2 */
	dwc_ddrphy_apb_wr(0x900bf, 0x0); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b50s0 */
	dwc_ddrphy_apb_wr(0x900c0, 0x7c8); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b50s1 */
	dwc_ddrphy_apb_wr(0x900c1, 0x109); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b50s2 */
	dwc_ddrphy_apb_wr(0x900c2, 0x8); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b51s0 */
	dwc_ddrphy_apb_wr(0x900c3, 0x8140); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b51s1 */
	dwc_ddrphy_apb_wr(0x900c4, 0x10c); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b51s2 */
	dwc_ddrphy_apb_wr(0x900c5, 0x0); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b52s0 */
	dwc_ddrphy_apb_wr(0x900c6, 0x1); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b52s1 */
	dwc_ddrphy_apb_wr(0x900c7, 0x8); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b52s2 */
	dwc_ddrphy_apb_wr(0x900c8, 0x8); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b53s0 */
	dwc_ddrphy_apb_wr(0x900c9, 0x4); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b53s1 */
	dwc_ddrphy_apb_wr(0x900ca, 0x8); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b53s2 */
	dwc_ddrphy_apb_wr(0x900cb, 0x8); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b54s0 */
	dwc_ddrphy_apb_wr(0x900cc, 0x7c8); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b54s1 */
	dwc_ddrphy_apb_wr(0x900cd, 0x101); /*  DWC_DDRPHYA_INITENG0_SequenceReg0b54s2 */
	dwc_ddrphy_apb_wr(0x90006, 0x0); /*  DWC_DDRPHYA_INITENG0_PostSequenceReg0b0s0 */
	dwc_ddrphy_apb_wr(0x90007, 0x0); /*  DWC_DDRPHYA_INITENG0_PostSequenceReg0b0s1 */
	dwc_ddrphy_apb_wr(0x90008, 0x8); /*  DWC_DDRPHYA_INITENG0_PostSequenceReg0b0s2 */
	dwc_ddrphy_apb_wr(0x90009, 0x0); /*  DWC_DDRPHYA_INITENG0_PostSequenceReg0b1s0 */
	dwc_ddrphy_apb_wr(0x9000a, 0x0); /*  DWC_DDRPHYA_INITENG0_PostSequenceReg0b1s1 */
	dwc_ddrphy_apb_wr(0x9000b, 0x0); /*  DWC_DDRPHYA_INITENG0_PostSequenceReg0b1s2 */
	dwc_ddrphy_apb_wr(0xd00e7, 0x400); /*  DWC_DDRPHYA_APBONLY0_SequencerOverride */
	dwc_ddrphy_apb_wr(0x90017, 0x0); /*  DWC_DDRPHYA_INITENG0_StartVector0b0 */
	dwc_ddrphy_apb_wr(0x90026, 0x2c); /*  DWC_DDRPHYA_INITENG0_StartVector0b15 */
	/*   [phyinit_I_loadPIEImage] Pstate=0,  Memclk=1200MHz, Programming Seq0BDLY0 to 0x4b */
	dwc_ddrphy_apb_wr(0x2000b, 0x4b); /*  DWC_DDRPHYA_MASTER0_Seq0BDLY0_p0 */
	/*   [phyinit_I_loadPIEImage] Pstate=0,  Memclk=1200MHz, Programming Seq0BDLY1 to 0x96 */
	dwc_ddrphy_apb_wr(0x2000c, 0x96); /*  DWC_DDRPHYA_MASTER0_Seq0BDLY1_p0 */
	/*   [phyinit_I_loadPIEImage] Pstate=0,  Memclk=1200MHz, Programming Seq0BDLY2 to 0x5dc */
	dwc_ddrphy_apb_wr(0x2000d, 0x5dc); /*  DWC_DDRPHYA_MASTER0_Seq0BDLY2_p0 */
	/*   [phyinit_I_loadPIEImage] Pstate=0,  Memclk=1200MHz, Programming Seq0BDLY3 to 0x2c */
	dwc_ddrphy_apb_wr(0x2000e, 0x2c); /*  DWC_DDRPHYA_MASTER0_Seq0BDLY3_p0 */
	/*   [phyinit_I_loadPIEImage] Pstate=1,  Memclk=200MHz, Programming Seq0BDLY0 to 0xc */
	dwc_ddrphy_apb_wr(0x12000b, 0xc); /*  DWC_DDRPHYA_MASTER0_Seq0BDLY0_p1 */
	/*   [phyinit_I_loadPIEImage] Pstate=1,  Memclk=200MHz, Programming Seq0BDLY1 to 0x19 */
	dwc_ddrphy_apb_wr(0x12000c, 0x19); /*  DWC_DDRPHYA_MASTER0_Seq0BDLY1_p1 */
	/*   [phyinit_I_loadPIEImage] Pstate=1,  Memclk=200MHz, Programming Seq0BDLY2 to 0xfa */
	dwc_ddrphy_apb_wr(0x12000d, 0xfa); /*  DWC_DDRPHYA_MASTER0_Seq0BDLY2_p1 */
	/*   [phyinit_I_loadPIEImage] Pstate=1,  Memclk=200MHz, Programming Seq0BDLY3 to 0x10 */
	dwc_ddrphy_apb_wr(0x12000e, 0x10); /*  DWC_DDRPHYA_MASTER0_Seq0BDLY3_p1 */
	/*   [phyinit_I_loadPIEImage] Pstate=2,  Memclk=50MHz, Programming Seq0BDLY0 to 0x3 */
	dwc_ddrphy_apb_wr(0x22000b, 0x3); /*  DWC_DDRPHYA_MASTER0_Seq0BDLY0_p2 */
	/*   [phyinit_I_loadPIEImage] Pstate=2,  Memclk=50MHz, Programming Seq0BDLY1 to 0x6 */
	dwc_ddrphy_apb_wr(0x22000c, 0x6); /*  DWC_DDRPHYA_MASTER0_Seq0BDLY1_p2 */
	/*   [phyinit_I_loadPIEImage] Pstate=2,  Memclk=50MHz, Programming Seq0BDLY2 to 0x3e */
	dwc_ddrphy_apb_wr(0x22000d, 0x3e); /*  DWC_DDRPHYA_MASTER0_Seq0BDLY2_p2 */
	/*   [phyinit_I_loadPIEImage] Pstate=2,  Memclk=50MHz, Programming Seq0BDLY3 to 0x10 */
	dwc_ddrphy_apb_wr(0x22000e, 0x10); /*  DWC_DDRPHYA_MASTER0_Seq0BDLY3_p2 */
	dwc_ddrphy_apb_wr(0x9000c, 0x0); /*  DWC_DDRPHYA_INITENG0_Seq0BDisableFlag0 */
	dwc_ddrphy_apb_wr(0x9000d, 0x173); /*  DWC_DDRPHYA_INITENG0_Seq0BDisableFlag1 */
	dwc_ddrphy_apb_wr(0x9000e, 0x60); /*  DWC_DDRPHYA_INITENG0_Seq0BDisableFlag2 */
	dwc_ddrphy_apb_wr(0x9000f, 0x6110); /*  DWC_DDRPHYA_INITENG0_Seq0BDisableFlag3 */
	dwc_ddrphy_apb_wr(0x90010, 0x2152); /*  DWC_DDRPHYA_INITENG0_Seq0BDisableFlag4 */
	dwc_ddrphy_apb_wr(0x90011, 0xdfbd); /*  DWC_DDRPHYA_INITENG0_Seq0BDisableFlag5 */
	dwc_ddrphy_apb_wr(0x90012, 0xffff); /*  DWC_DDRPHYA_INITENG0_Seq0BDisableFlag6 */
	dwc_ddrphy_apb_wr(0x90013, 0x6152); /*  DWC_DDRPHYA_INITENG0_Seq0BDisableFlag7 */
	/*   Disabling Ucclk (PMU) and Hclk (training hardware) */
	dwc_ddrphy_apb_wr(0xc0080, 0x0); /*  DWC_DDRPHYA_DRTUB0_UcclkHclkEnables */
	/*   Isolate the APB access from the internal CSRs by setting the MicroContMuxSel CSR to 1. */
	dwc_ddrphy_apb_wr(0xd0000, 0x1); /*  DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
	/*   [phyinit_I_loadPIEImage] End of dwc_ddrphy_phyinit_I_loadPIEImage() */
	/*  */
	/*  */
	/*  ############################################################## */
	/*   */
	/*   dwc_ddrphy_phyinit_userCustom_customPostTrain is a user-editable function. */
	/*   */
	/*   See PhyInit App Note for detailed description and function usage */
	/*  */
	/*  ############################################################## */
	/*  */
	dwc_ddrphy_phyinit_userCustom_customPostTrain ();

	/*   [dwc_ddrphy_phyinit_userCustom_customPostTrain] End of dwc_ddrphy_phyinit_userCustom_customPostTrain() */
	/*   [dwc_ddrphy_phyinit_userCustom_J_enterMissionMode] Start of dwc_ddrphy_phyinit_userCustom_J_enterMissionMode() */
	/*  */
	/*  */
	/*  ############################################################## */
	/*   */
	/*   (J) Initialize the PHY to Mission Mode through DFI Initialization */
	/*   */
	/*   Initialize the PHY to mission mode as follows: */
	/*   */
	/*   1. Set the PHY input clocks to the desired frequency. */
	/*   2. Initialize the PHY to mission mode by performing DFI Initialization. */
	/*      Please see the DFI specification for more information. See the DFI frequency bus encoding in section <XXX>. */
	/*   Note: The PHY training firmware initializes the DRAM state. if skip */
	/*   training is used, the DRAM state is not initialized. */
	/*   */
	/*  ############################################################## */
	/*  */
	dwc_ddrphy_phyinit_userCustom_J_enterMissionMode ();

	/*  */
	/*  [dwc_ddrphy_phyinit_userCustom_J_enterMissionMode] End of dwc_ddrphy_phyinit_userCustom_J_enterMissionMode() */
	/*  [dwc_ddrphy_phyinit_sequence] End of dwc_ddrphy_phyinit_sequence() */
	/*  [dwc_ddrphy_phyinit_main] End of dwc_ddrphy_phyinit_main() */

	/* ---------------------------------------------------------------------- */
	/*   save 1d2d training CSR */
	/* ---------------------------------------------------------------------- */
#ifdef ENABLE_RETENTION
	if (!after_retention) {
		save_1d2d_trained_csr_ddr4_p012(SAVE_DDRPHY_TRAIN_ADDR);
	}
#endif
}
