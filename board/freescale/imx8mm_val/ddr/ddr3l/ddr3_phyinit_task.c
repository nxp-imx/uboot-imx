/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <common.h>
#include <errno.h>
#include <asm/io.h>
#include <asm/arch/ddr.h>
#include <asm/arch/clock.h>
#include "ddr3_define.h"

void dwc_ddrphy_phyinit_userCustom_E_setDfiClk (unsigned int pstate) {
	if (pstate == 0) {
		ddr_dbg("C: 1 ...\n");
		dram_pll_init(DRAM_PLL_OUT_400M);
	} else if (pstate == 1) {
		ddr_dbg("C: 2 ...\n");
		dram_pll_init(DRAM_PLL_OUT_266M);
	} else if (pstate == 2) {
		ddr_dbg("C: 3 ...\n");
		dram_pll_init(DRAM_PLL_OUT_167M);
	} else {
		printf("C: no freq match\n");
	}
}

void dwc_ddrphy_phyinit_userCustom_G_waitFwDone(void)
{
	wait_ddrphy_training_complete();
}
void dwc_ddrphy_phyinit_userCustom_overrideUserInput (void) {}
void dwc_ddrphy_phyinit_userCustom_A_bringupPower (void) {}
void dwc_ddrphy_phyinit_userCustom_B_startClockResetPhy (void) {}
void dwc_ddrphy_phyinit_userCustom_H_readMsgBlock(unsigned int Train2D) {}
void dwc_ddrphy_phyinit_userCustom_customPostTrain(void) {}
void dwc_ddrphy_phyinit_userCustom_J_enterMissionMode(void) {}

void ddr3_mr_write(unsigned int mr, unsigned int data, unsigned int read, unsigned int rank)
{
	unsigned int tmp, mr_mirror, data_mirror;

	/* 1. Poll MRSTAT.mr_wr_busy until it is 0. This checks that there is no outstanding MR transaction. No */
	/* writes should be performed to MRCTRL0 and MRCTRL1 if MRSTAT.mr_wr_busy = 1. */
	do {
		tmp = reg32_read(DDRC_MRSTAT(0));
	} while (tmp & 0x1);

	/* 2. Write the MRCTRL0.mr_type, MRCTRL0.mr_addr, MRCTRL0.mr_rank and (for MRWs) */
	/* MRCTRL1.mr_data to define the MR transaction. */
	/*  (A3, A4), (A5, A6), (A7, A8), (BA0, BA1),*/
	tmp = reg32_read(DDRC_DIMMCTL(0));
	if ((tmp & 0x2) && (rank == 0x2)) {
	    mr_mirror = (mr & 0x4) | ((mr & 0x1) << 1) | ((mr & 0x2) >> 1);/* BA0, BA1 swap */
		data_mirror = (data & 0xfe07) | ((data & 0x8) << 1) | ((data & 0x10) >> 1) | ((data & 0x20) << 1) | ((data & 0x40) >> 1) | ((data & 0x80) << 1) | ((data & 0x100) >> 1);
	} else {
	    mr_mirror = mr;
	    data_mirror = data;
	}

	reg32_write(DDRC_MRCTRL0(0), read | (mr_mirror << 12) | (rank << 4));
	reg32_write(DDRC_MRCTRL1(0), data_mirror);

	/* 3. In a separate APB transaction, write the MRCTRL0.mr_wr to 1. This bit is self-clearing, and triggers */
	/* the MR transaction. The uMCTL2 then asserts the MRSTAT.mr_wr_busy while it performs the MR */
	/* transaction to SDRAM, and no further accesses can be initiated until it is deasserted. */
	reg32setbit(DDRC_MRCTRL0(0), 31);
	do {
		tmp = reg32_read(DDRC_MRSTAT(0));
	} while (tmp);

}
