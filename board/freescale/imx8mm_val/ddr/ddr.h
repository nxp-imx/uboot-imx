/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier: GPL-2.0+
 * Common file for ddr code
 */

#ifndef __M845S_DDR_H_
#define __M845S_DDR_H_

#ifdef DDR_DEBUG
#define ddr_dbg(fmt, ...) printf("DDR: debug:" fmt "\n", ##__VA_ARGS__)
#else
#define ddr_dbg(fmt, ...)
#endif

/*******************************************************************
 Desc: prototype

 *******************************************************************/
void ddr_load_train_code(enum fw_type type);
void dwc_ddrphy_phyinit_userCustom_E_setDfiClk(unsigned int pstate);
void dwc_ddrphy_phyinit_userCustom_J_enterMissionMode(void);
void dwc_ddrphy_phyinit_userCustom_customPostTrain(void);
void dwc_ddrphy_phyinit_userCustom_B_startClockResetPhy(void);
void dwc_ddrphy_phyinit_userCustom_A_bringupPower(void);
void dwc_ddrphy_phyinit_userCustom_overrideUserInput(void);
void dwc_ddrphy_phyinit_userCustom_H_readMsgBlock(unsigned int run_2D);
void dwc_ddrphy_phyinit_userCustom_G_waitFwDone(void);

#endif
