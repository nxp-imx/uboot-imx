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
#include "anamix_common.h"
#include "ddr3_define.h"

void umctl2_cfg(void)
{
#ifdef DDR_ONE_RANK
	reg32_write(DDRC_MSTR(0), 0x81040001);
#else
	reg32_write(DDRC_MSTR(0), 0x83040001);
#endif

	reg32_write(DDRC_PWRCTL(0), 0x000000a8);
	reg32_write(DDRC_PWRTMG(0), 0x00532203);

	reg32_write(DDRC_RFSHCTL0(0), 0x00203020);
	reg32_write(DDRC_RFSHCTL1(0), 0x0001000d);
	reg32_write(DDRC_RFSHCTL3(0), 0x00000000);
	reg32_write(DDRC_RFSHTMG(0), 0x0061008c);
	reg32_write(DDRC_CRCPARCTL0(0), 0x00000000);
	reg32_write(DDRC_CRCPARCTL1(0), 0x00000000);
	reg32_write(DDRC_INIT0(0), 0xc0030002);
	reg32_write(DDRC_INIT1(0), 0x0001000b);
	reg32_write(DDRC_INIT2(0), 0x00006303);
	reg32_write(DDRC_INIT3(0), 0x0d700004);/* MR1, MR0 */
	reg32_write(DDRC_INIT4(0), 0x00180000);/* MR2 */
	reg32_write(DDRC_INIT5(0), 0x00090071);
	reg32_write(DDRC_INIT6(0), 0x00000000);
	reg32_write(DDRC_INIT7(0), 0x00000000);
	reg32_write(DDRC_DIMMCTL(0), 0x00000032); /* [1] dimm_addr_mirr_en, it will effect the MRS if use umctl2 to initi dram. */
	reg32_write(DDRC_RANKCTL(0), 0x00000ee5);
	reg32_write(DDRC_DRAMTMG0(0), 0x0c101a0e);
	reg32_write(DDRC_DRAMTMG1(0), 0x000a0314);
	reg32_write(DDRC_DRAMTMG2(0), 0x04060509);
	reg32_write(DDRC_DRAMTMG3(0), 0x00002006);
	reg32_write(DDRC_DRAMTMG4(0), 0x06020306);
	reg32_write(DDRC_DRAMTMG5(0), 0x0b060202);
	reg32_write(DDRC_DRAMTMG6(0), 0x060a0009);
	reg32_write(DDRC_DRAMTMG7(0), 0x0000060b);
	reg32_write(DDRC_DRAMTMG8(0), 0x01017c0a);
	reg32_write(DDRC_DRAMTMG9(0), 0x4000000e);
	reg32_write(DDRC_DRAMTMG10(0), 0x00070803);
	reg32_write(DDRC_DRAMTMG11(0), 0x0101000b);
	reg32_write(DDRC_DRAMTMG12(0), 0x00000000);
	reg32_write(DDRC_DRAMTMG13(0), 0x5d000000);
	reg32_write(DDRC_DRAMTMG14(0), 0x00000b39);
	reg32_write(DDRC_DRAMTMG15(0), 0x80000000);
	reg32_write(DDRC_DRAMTMG17(0), 0x00f1006a);
	reg32_write(DDRC_ZQCTL0(0), 0x50800020);
	reg32_write(DDRC_ZQCTL1(0), 0x00000070);
	reg32_write(DDRC_ZQCTL2(0), 0x00000000);
	reg32_write(DDRC_DFITMG0(0), 0x03868203);
	reg32_write(DDRC_DFITMG1(0), 0x00020103);
	reg32_write(DDRC_DFILPCFG0(0), 0x07713021);
	reg32_write(DDRC_DFILPCFG1(0), 0x00000010);
	reg32_write(DDRC_DFIUPD0(0), 0xe0400018);
	reg32_write(DDRC_DFIUPD1(0), 0x0005003c);
	reg32_write(DDRC_DFIUPD2(0), 0x80000000);
	reg32_write(DDRC_DFIMISC(0), 0x00000011);
	reg32_write(DDRC_DFITMG2(0), 0x00000603);
	reg32_write(DDRC_DFITMG3(0), 0x00000001);
	reg32_write(DDRC_DBICTL(0), 0x00000001);
	reg32_write(DDRC_DFIPHYMSTR(0), 0x00000000);

	/*  My test mapping in this test case, for 8Gb,(two 4Gb, x16 DDR3) (col addr:10 bits  row addr: 15 bits  bank addr: 3bits  2 ranks) */
	/*  MEMC_BURST_LENGTH = 8 */
	/* ----------------------------------------------------------------------------------------------------------------------------------- */
	/*  AXI add: 31  30  29  28  27  26  25  24  23  22  21  20  19  18  17  16  15  14  13   12  11  10  9   8   7   6   5   4   3   2   1   0 (MEM_DATWIDTH=64) */
	/*  AXI add: 30  29  28  27  26  25  24  23  22  21  20  19  18  17  16  15  14  13  12   11  10  9   8   7   6   5   4   3   2   1   0     (MEM_DATWIDTH=32) *** */
	/*  AXI add: 29  28  27  26  25  24  23  22  21  20  19  18  17  16  15  14  13  12  11   10  9   8   7   6   5   4   3   2   1   0         (MEM_DATWIDTH=16) */
	/* ----------------------------------------------------------------------------------------------------------------------------------- */
	/*  HIF add: 28  27  26  25  24  23  22  21  20  19  18  17  16  15  14  13  12  11  10   9   8   7   6   5   4   3   2   1   0   -   -   - */
	/* ----------------------------------------------------------------------------------------------------------------------------------- */
	/*  **** for Full DQ bus width (X32) **** */
	/*           cs  r14 r13 r12 r11 r10 r9  r8  r7  r6  r5  r4  r3  r2  r1  r0  b2  b1  b0  c9  c8  c7  c6  c5  c4  c3  c2  c1  c0 */
	/* Int base  6   20  19  18  17  16  15  14  13  12  11  10  9   8   7   6   4   3   2    9   8  7   6   5    4   3   2   - */
	/* p Value   22  7   7   7   7   7   7   7   7   7    7   7  7   7   7   7   8   8   8    0   0  0   0   0    0   0   0   - */
	/* ----------------------------------------------------------------------------------------------------------------------------------- */

	reg32_write(DDRC_ADDRMAP0(0), 0x00000016); /* [4:0] cs-bit0: 6+22=28; [12:8] cs-bit1: 7+0 */
	reg32_write(DDRC_ADDRMAP1(0), 0x00080808); /* [5:0] bank b0: 2+8; [13:8] b1: P3+8 ; [21:16] b2: 4+8 */
	reg32_write(DDRC_ADDRMAP2(0), 0x00000000); /* [3:0] col-b2: 2;  [11:8] col-b3: 3; [19:16] col-b4: 4 ; [27:24] col-b5: 5 */
	reg32_write(DDRC_ADDRMAP3(0), 0x00000000); /* [3:0] col-b6: 6;  [11:8] col-b7: 7; [19:16] col-b8: 8 ; [27:24] col-b9: 9 */
	reg32_write(DDRC_ADDRMAP4(0), 0x00001f1f); /* col-b10, col-b11 not used */
	reg32_write(DDRC_ADDRMAP5(0), 0x07070707); /* [3:0] row-b0: 6;  [11:8] row-b1: 7; [19:16] row-b2_b10 ; [27:24] row-b11: 17 */
	reg32_write(DDRC_ADDRMAP6(0), 0x0f070707); /* [3:0] row-b12:18; [11:8] row-b13: 19; [19:16] row-b14:20 */
	reg32_write(DDRC_ADDRMAP7(0), 0x00000f0f);
	reg32_write(DDRC_ADDRMAP8(0), 0x00000000); /* [5:0] bg-b0; [13:8]bg-b1 */
	reg32_write(DDRC_ADDRMAP9(0), 0x0a020b06); /*  it's valid only when ADDRMAP5.addrmap_row_b2_10 is set to value 15 */
	reg32_write(DDRC_ADDRMAP10(0), 0x0a0a0a0a);/*  it's valid only when ADDRMAP5.addrmap_row_b2_10 is set to value 15 */
	reg32_write(DDRC_ADDRMAP11(0), 0x00000000);


	reg32_write(DDRC_ODTCFG(0), 0x041d0f5c);
	reg32_write(DDRC_ODTMAP(0), 0x00000201);
	reg32_write(DDRC_SCHED(0), 0x7ab50b07);
	reg32_write(DDRC_SCHED1(0), 0x00000022);
	reg32_write(DDRC_PERFHPR1(0), 0x7b00665e);
	reg32_write(DDRC_PERFLPR1(0), 0x2b00c4e1);
	reg32_write(DDRC_PERFWR1(0), 0xb700c9fe);
	reg32_write(DDRC_DBG0(0), 0x00000017);
	reg32_write(DDRC_DBG1(0), 0x00000000);
	reg32_write(DDRC_DBGCMD(0), 0x00000000);
	reg32_write(DDRC_SWCTL(0), 0x00000001);
	reg32_write(DDRC_POISONCFG(0), 0x00010000);
	reg32_write(DDRC_PCCFG(0), 0x00000100);
	reg32_write(DDRC_PCFGR_0(0), 0x00003051);
	reg32_write(DDRC_PCFGW_0(0), 0x000061d2);
	reg32_write(DDRC_PCTRL_0(0), 0x00000001);
	reg32_write(DDRC_PCFGQOS0_0(0), 0x02100b04);
	reg32_write(DDRC_PCFGQOS1_0(0), 0x003f0353);
	reg32_write(DDRC_PCFGWQOS0_0(0), 0x00000002);
	reg32_write(DDRC_PCFGWQOS1_0(0), 0x000005fd);
}

void umctl2_freq1_cfg(void)
{
	reg32_write(DDRC_FREQ1_RFSHCTL0(0), 0x00d19034);
	reg32_write(DDRC_FREQ1_RFSHTMG(0), 0x0040805e);
	reg32_write(DDRC_FREQ1_INIT3(0), 0x09300004);
	reg32_write(DDRC_FREQ1_INIT4(0), 0x00080000);
	reg32_write(DDRC_FREQ1_INIT6(0), 0x00000000);
	reg32_write(DDRC_FREQ1_INIT7(0), 0x00000000);
	reg32_write(DDRC_FREQ1_DRAMTMG0(0), 0x090e110a);
	reg32_write(DDRC_FREQ1_DRAMTMG1(0), 0x0007020e);
	reg32_write(DDRC_FREQ1_DRAMTMG2(0), 0x03040407);
	reg32_write(DDRC_FREQ1_DRAMTMG3(0), 0x00002006);
	reg32_write(DDRC_FREQ1_DRAMTMG4(0), 0x04020304); /*  tRP=6 --> 7 */
	reg32_write(DDRC_FREQ1_DRAMTMG5(0), 0x09030202);
	reg32_write(DDRC_FREQ1_DRAMTMG6(0), 0x0c020000);
	reg32_write(DDRC_FREQ1_DRAMTMG7(0), 0x00000309);
	reg32_write(DDRC_FREQ1_DRAMTMG8(0), 0x01010a06);
	reg32_write(DDRC_FREQ1_DRAMTMG9(0), 0x00000003);
	reg32_write(DDRC_FREQ1_DRAMTMG10(0), 0x00090906);
	reg32_write(DDRC_FREQ1_DRAMTMG11(0), 0x01010011);
	reg32_write(DDRC_FREQ1_DRAMTMG12(0), 0x00000000);
	reg32_write(DDRC_FREQ1_DRAMTMG13(0), 0x40000000);
	reg32_write(DDRC_FREQ1_DRAMTMG14(0), 0x000000f3);
	reg32_write(DDRC_FREQ1_DRAMTMG15(0), 0x80000000);
	reg32_write(DDRC_FREQ1_DRAMTMG17(0), 0x001a0046);
	reg32_write(DDRC_FREQ1_ZQCTL0(0),  0x50800020);
	reg32_write(DDRC_FREQ1_DFITMG0(0), 0x03828201);
	reg32_write(DDRC_FREQ1_DFITMG1(0), 0x00020103);
	reg32_write(DDRC_FREQ1_DFITMG2(0), 0x00000201);
	reg32_write(DDRC_FREQ1_DFITMG3(0), 0x00000001);
	reg32_write(DDRC_FREQ1_ODTCFG(0),  0x0a1a0768);

}

void umctl2_freq2_cfg(void)
{
	reg32_write(DDRC_FREQ2_RFSHCTL0(0), 0x00208014);
	reg32_write(DDRC_FREQ2_RFSHTMG(0), 0x00308046);
	reg32_write(DDRC_FREQ2_INIT3(0), 0x05200004);
	reg32_write(DDRC_FREQ2_INIT4(0), 0x00000000);
	reg32_write(DDRC_FREQ2_INIT6(0), 0x00000000);
	reg32_write(DDRC_FREQ2_INIT7(0), 0x00000000);
	reg32_write(DDRC_FREQ2_DRAMTMG0(0), 0x070a0c07);
	reg32_write(DDRC_FREQ2_DRAMTMG1(0), 0x0005020b);
	reg32_write(DDRC_FREQ2_DRAMTMG2(0), 0x03030407);
	reg32_write(DDRC_FREQ2_DRAMTMG3(0), 0x00002006);
	reg32_write(DDRC_FREQ2_DRAMTMG4(0), 0x03020204);
	reg32_write(DDRC_FREQ2_DRAMTMG5(0), 0x04070302);
	reg32_write(DDRC_FREQ2_DRAMTMG6(0), 0x07080000);
	reg32_write(DDRC_FREQ2_DRAMTMG7(0), 0x00000704);
	reg32_write(DDRC_FREQ2_DRAMTMG8(0), 0x02026804);
	reg32_write(DDRC_FREQ2_DRAMTMG9(0), 0x40000006);
	reg32_write(DDRC_FREQ2_DRAMTMG10(0), 0x000c0b08);
	reg32_write(DDRC_FREQ2_DRAMTMG11(0), 0x01010015);
	reg32_write(DDRC_FREQ2_DRAMTMG12(0), 0x00000000);
	reg32_write(DDRC_FREQ2_DRAMTMG13(0), 0x51000000);
	reg32_write(DDRC_FREQ2_DRAMTMG14(0), 0x000002a0);
	reg32_write(DDRC_FREQ2_DRAMTMG15(0), 0x00000000);
	reg32_write(DDRC_FREQ2_DRAMTMG17(0), 0x008c0039);
	reg32_write(DDRC_FREQ2_ZQCTL0(0), 0x50800020);
	reg32_write(DDRC_FREQ2_DFITMG0(0), 0x03818200);
	reg32_write(DDRC_FREQ2_DFITMG1(0), 0x00020103);
	reg32_write(DDRC_FREQ2_DFITMG2(0), 0x00000100);
	reg32_write(DDRC_FREQ2_DFITMG3(0), 0x00000001);
	reg32_write(DDRC_FREQ2_ODTCFG(0), 0x04050800);

}

void ddr3_pub_train(void)
{
	volatile unsigned int tmp_t;

	reg32_write(SRC_DDRC_RCR_ADDR, 0x8F00003F); /*  assert [0]ddr1_preset_n, [1]ddr1_core_reset_n, [2]ddr1_phy_reset, [3]ddr1_phy_pwrokin_n, [4]src_system_rst_b! */
	reg32_write(SRC_DDRC_RCR_ADDR, 0x8F00000F); /*  deassert [4]src_system_rst_b! */

	/* change the clock source of dram_apb_clk_root */
	clock_set_target_val(DRAM_APB_CLK_ROOT, CLK_ROOT_ON | CLK_ROOT_SOURCE_SEL(4) | CLK_ROOT_PRE_DIV(CLK_ROOT_PRE_DIV4)); /* to source 4 --800MHz/4 */

	dram_pll_init(DRAM_PLL_OUT_400M);
	ddr_dbg("C: dram pll init finished\n");

	reg32_write(0x303A00EC, 0x0000ffff); /* PGC_CPU_MAPPING */
	reg32setbit(0x303A00F8, 5);/* PU_PGC_SW_PUP_REQ */

	reg32_write(SRC_DDRC_RCR_ADDR, 0x8F000006); /*  release [0]ddr1_preset_n, [3]ddr1_phy_pwrokin_n */

	reg32_write(DDRC_DBG1(0), 0x00000001);
	reg32_write(DDRC_PWRCTL(0), 0x00000001);

	while (0 != (0x3 & reg32_read(DDRC_STAT(0))))
		;

	ddr_dbg("C: cfg umctl2 regs ...\n");
	umctl2_cfg();
#ifdef DDR3_SW_FFC
	umctl2_freq1_cfg();
	umctl2_freq2_cfg();
#endif

	reg32_write(DDRC_RFSHCTL3(0), 0x00000011);
	/* RESET: <ctn> DEASSERTED */
	/* RESET: <a Port 0  DEASSERTED(0) */
	reg32_write(SRC_DDRC_RCR_ADDR, 0x8F000000); /*  release all reset */

	reg32_write(DDRC_DBG1(0), 0x00000000);
	reg32_write(DDRC_PWRCTL(0), 0x00000a8);
	reg32_write(DDRC_SWCTL(0), 0x00000000);

	reg32_write(DDRC_DFIMISC(0), 0x00000000);

	ddr_dbg("C: phy training ...\n");
	ddr3_phyinit_train_sw_ffc(0);

	do {
		tmp_t = reg32_read(IP2APB_DDRPHY_IPS_BASE_ADDR(0) + 4 * 0x00020097);
		ddr_dbg("C: Waiting CalBusy value = 0\n");
	} while (tmp_t != 0);

	reg32_write(DDRC_DFIMISC(0), 0x00000020);

	/*  wait DFISTAT.dfi_init_complete to 1 */
	while (0 == (0x1 & reg32_read(DDRC_DFISTAT(0))))
		;

	/*  clear DFIMISC.dfi_init_complete_en */
	reg32_write(DDRC_DFIMISC(0), 0x00000000);
	/*  set DFIMISC.dfi_init_complete_en again */
	reg32_write(DDRC_DFIMISC(0), 0x00000001);
	reg32_write(DDRC_PWRCTL(0), 0x0000088);

	/*  set SWCTL.sw_done to enable quasi-dynamic register programming outside reset. */
	reg32_write(DDRC_SWCTL(0), 0x00000001);
	/* wait SWSTAT.sw_done_ack to 1 */
	while (0 == (0x1 & reg32_read(DDRC_SWSTAT(0))))
		;

	/* wait STAT to normal state */
	while (0x1 != (0x3 & reg32_read(DDRC_STAT(0))))
		;

	reg32_write(DDRC_PWRCTL(0), 0x0000088);

	reg32_write(DDRC_PCTRL_0(0), 0x00000001);
	reg32_write(DDRC_RFSHCTL3(0), 0x00000010); /*  dis_auto-refresh is set to 0 */

 }

void ddr_init(void)
{
	/* initialize DDR4-2400 (umctl2@800MHz) */
	ddr3_pub_train();
}
