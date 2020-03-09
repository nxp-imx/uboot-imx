/*
 * Copyright 2018-2019 NXP
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <linux/kernel.h>
#include <common.h>
#include <asm/arch/ddr.h>

struct dram_cfg_param ddr3l_ddrc_cfg[] = {
	{ DDRC_MSTR(0), 0xa3040001 },
	{ DDRC_PWRCTL(0), 0x000000a8 },
	{ DDRC_PWRTMG(0), 0x00532203 },
	{ DDRC_RFSHCTL0(0), 0x00203020 },
	{ DDRC_RFSHCTL1(0), 0x0001000d },
	{ DDRC_RFSHCTL3(0), 0x00000000 },
	{ DDRC_RFSHTMG(0), 0x0061008c },
	{ DDRC_CRCPARCTL0(0), 0x00000000 },
	{ DDRC_CRCPARCTL1(0), 0x00000000 },
	{ DDRC_INIT0(0), 0xc0030002 },
	{ DDRC_INIT1(0), 0x0001000b },
	{ DDRC_INIT2(0), 0x00006303 },
	{ DDRC_INIT3(0), 0x0d700004 },/* MR1, MR0 */
	{ DDRC_INIT4(0), 0x00180000 },/* MR2 */
	{ DDRC_INIT5(0), 0x00090071 },
	{ DDRC_INIT6(0), 0x00000000 },
	{ DDRC_INIT7(0), 0x00000000 },
	{ DDRC_DIMMCTL(0), 0x00000032 }, /* [1] dimm_addr_mirr_en, it will effect the MRS if use umctl2 to initi dram. */
	{ DDRC_RANKCTL(0), 0x00000ee5 },
	{ DDRC_DRAMTMG0(0), 0x0c101a0e },
	{ DDRC_DRAMTMG1(0), 0x000a0314 },
	{ DDRC_DRAMTMG2(0), 0x04060509 },
	{ DDRC_DRAMTMG3(0), 0x00002006 },
	{ DDRC_DRAMTMG4(0), 0x06020306 },
	{ DDRC_DRAMTMG5(0), 0x0b060202 },
	{ DDRC_DRAMTMG6(0), 0x060a0009 },
	{ DDRC_DRAMTMG7(0), 0x0000060b },
	{ DDRC_DRAMTMG8(0), 0x01017c0a },
	{ DDRC_DRAMTMG9(0), 0x4000000e },
	{ DDRC_DRAMTMG10(0), 0x00070803 },
	{ DDRC_DRAMTMG11(0), 0x0101000b },
	{ DDRC_DRAMTMG12(0), 0x00000000 },
	{ DDRC_DRAMTMG13(0), 0x5d000000 },
	{ DDRC_DRAMTMG14(0), 0x00000b39 },
	{ DDRC_DRAMTMG15(0), 0x80000000 },
	{ DDRC_DRAMTMG17(0), 0x00f1006a },
	{ DDRC_ZQCTL0(0), 0x50800020 },
	{ DDRC_ZQCTL1(0), 0x00000070 },
	{ DDRC_ZQCTL2(0), 0x00000000 },
	{ DDRC_DFITMG0(0), 0x03868203 },
	{ DDRC_DFITMG1(0), 0x00020103 },
	{ DDRC_DFILPCFG0(0), 0x07713021 },
	{ DDRC_DFILPCFG1(0), 0x00000010 },
	{ DDRC_DFIUPD0(0), 0xe0400018 },
	{ DDRC_DFIUPD1(0), 0x0005003c },
	{ DDRC_DFIUPD2(0), 0x80000000 },
	{ DDRC_DFIMISC(0), 0x00000001 },
	{ DDRC_DFITMG2(0), 0x00000603 },
	{ DDRC_DFITMG3(0), 0x00000001 },
	{ DDRC_DBICTL(0), 0x00000001 },
	{ DDRC_DFIPHYMSTR(0), 0x00000000 },

	{ DDRC_ADDRMAP0(0), 0x00000016 }, /* [4:0] cs-bit0: 6+22=28; [12:8] cs-bit1: 7+0 */
	{ DDRC_ADDRMAP1(0), 0x00080808 }, /* [5:0] bank b0: 2+8; [13:8] b1: P3+8 ; [21:16] b2: 4+8 */
	{ DDRC_ADDRMAP2(0), 0x00000000 }, /* [3:0] col-b2: 2;  [11:8] col-b3: 3; [19:16] col-b4: 4 ; [27:24] col-b5: 5 */
	{ DDRC_ADDRMAP3(0), 0x00000000 }, /* [3:0] col-b6: 6;  [11:8] col-b7: 7; [19:16] col-b8: 8 ; [27:24] col-b9: 9 */
	{ DDRC_ADDRMAP4(0), 0x00001f1f }, /* col-b10, col-b11 not used */
	{ DDRC_ADDRMAP5(0), 0x07070707 }, /* [3:0] row-b0: 6;  [11:8] row-b1: 7; [19:16] row-b2_b10 ; [27:24] row-b11: 17 */
	{ DDRC_ADDRMAP6(0), 0x0f070707 }, /* [3:0] row-b12:18; [11:8] row-b13: 19; [19:16] row-b14:20 */
	{ DDRC_ADDRMAP7(0), 0x00000f0f },
	{ DDRC_ADDRMAP8(0), 0x00000000 }, /* [5:0] bg-b0; [13:8]bg-b1 */
	{ DDRC_ADDRMAP9(0), 0x0a020b06 }, /*  it's valid only when ADDRMAP5.addrmap_row_b2_10 is set to value 15 */
	{ DDRC_ADDRMAP10(0), 0x0a0a0a0a },/*  it's valid only when ADDRMAP5.addrmap_row_b2_10 is set to value 15 */
	{ DDRC_ADDRMAP11(0), 0x00000000 },

	{ DDRC_ODTCFG(0), 0x041d0f5c },
	{ DDRC_ODTMAP(0), 0x00000201 },
	{ DDRC_SCHED(0), 0x7ab50b07 },
	{ DDRC_SCHED1(0), 0x00000022 },
	{ DDRC_PERFHPR1(0), 0x7b00665e },
	{ DDRC_PERFLPR1(0), 0x2b00c4e1 },
	{ DDRC_PERFWR1(0), 0xb700c9fe },
	{ DDRC_DBG0(0), 0x00000017 },
	{ DDRC_DBG1(0), 0x00000000 },
	{ DDRC_DBGCMD(0), 0x00000000 },
	{ DDRC_SWCTL(0), 0x00000001 },
	{ DDRC_POISONCFG(0), 0x00010000 },
	{ DDRC_PCCFG(0), 0x00000100 },
	{ DDRC_PCFGR_0(0), 0x00003051 },
	{ DDRC_PCFGW_0(0), 0x000061d2 },
	{ DDRC_PCTRL_0(0), 0x00000001 },
	{ DDRC_PCFGQOS0_0(0), 0x02100b04 },
	{ DDRC_PCFGQOS1_0(0), 0x003f0353 },
	{ DDRC_PCFGWQOS0_0(0), 0x00000002 },
	{ DDRC_PCFGWQOS1_0(0), 0x000005fd },

	{ DDRC_FREQ1_RFSHCTL0(0), 0x00d19034 },
	{ DDRC_FREQ1_RFSHTMG(0), 0x0040805e },
	{ DDRC_FREQ1_INIT3(0), 0x09300004 },
	{ DDRC_FREQ1_INIT4(0), 0x00080000 },
	{ DDRC_FREQ1_INIT6(0), 0x00000000 },
	{ DDRC_FREQ1_INIT7(0), 0x00000000 },
	{ DDRC_FREQ1_DRAMTMG0(0), 0x090e110a },
	{ DDRC_FREQ1_DRAMTMG1(0), 0x0007020e },
	{ DDRC_FREQ1_DRAMTMG2(0), 0x03040407 },
	{ DDRC_FREQ1_DRAMTMG3(0), 0x00002006 },
	{ DDRC_FREQ1_DRAMTMG4(0), 0x04020304 }, /*  tRP=6 --> 7 */
	{ DDRC_FREQ1_DRAMTMG5(0), 0x09030202 },
	{ DDRC_FREQ1_DRAMTMG6(0), 0x0c020000 },
	{ DDRC_FREQ1_DRAMTMG7(0), 0x00000309 },
	{ DDRC_FREQ1_DRAMTMG8(0), 0x01010a06 },
	{ DDRC_FREQ1_DRAMTMG9(0), 0x00000003 },
	{ DDRC_FREQ1_DRAMTMG10(0), 0x00090906 },
	{ DDRC_FREQ1_DRAMTMG11(0), 0x01010011 },
	{ DDRC_FREQ1_DRAMTMG12(0), 0x00000000 },
	{ DDRC_FREQ1_DRAMTMG13(0), 0x40000000 },
	{ DDRC_FREQ1_DRAMTMG14(0), 0x000000f3 },
	{ DDRC_FREQ1_DRAMTMG15(0), 0x80000000 },
	{ DDRC_FREQ1_DRAMTMG17(0), 0x001a0046 },
	{ DDRC_FREQ1_ZQCTL0(0),  0x50800020 },
	{ DDRC_FREQ1_DFITMG0(0), 0x03828201 },
	{ DDRC_FREQ1_DFITMG1(0), 0x00020103 },
	{ DDRC_FREQ1_DFITMG2(0), 0x00000201 },
	{ DDRC_FREQ1_DFITMG3(0), 0x00000001 },
	{ DDRC_FREQ1_ODTCFG(0),  0x0a1a0768 },

	{ DDRC_FREQ2_RFSHCTL0(0), 0x00208014 },
	{ DDRC_FREQ2_RFSHTMG(0), 0x00308046 },
	{ DDRC_FREQ2_INIT3(0), 0x05200004 },
	{ DDRC_FREQ2_INIT4(0), 0x00000000 },
	{ DDRC_FREQ2_INIT6(0), 0x00000000 },
	{ DDRC_FREQ2_INIT7(0), 0x00000000 },
	{ DDRC_FREQ2_DRAMTMG0(0), 0x070a0c07 },
	{ DDRC_FREQ2_DRAMTMG1(0), 0x0005020b },
	{ DDRC_FREQ2_DRAMTMG2(0), 0x03030407 },
	{ DDRC_FREQ2_DRAMTMG3(0), 0x00002006 },
	{ DDRC_FREQ2_DRAMTMG4(0), 0x03020204 },
	{ DDRC_FREQ2_DRAMTMG5(0), 0x04070302 },
	{ DDRC_FREQ2_DRAMTMG6(0), 0x07080000 },
	{ DDRC_FREQ2_DRAMTMG7(0), 0x00000704 },
	{ DDRC_FREQ2_DRAMTMG8(0), 0x02026804 },
	{ DDRC_FREQ2_DRAMTMG9(0), 0x40000006 },
	{ DDRC_FREQ2_DRAMTMG10(0), 0x000c0b08 },
	{ DDRC_FREQ2_DRAMTMG11(0), 0x01010015 },
	{ DDRC_FREQ2_DRAMTMG12(0), 0x00000000 },
	{ DDRC_FREQ2_DRAMTMG13(0), 0x51000000 },
	{ DDRC_FREQ2_DRAMTMG14(0), 0x000002a0 },
	{ DDRC_FREQ2_DRAMTMG15(0), 0x00000000 },
	{ DDRC_FREQ2_DRAMTMG17(0), 0x008c0039 },
	{ DDRC_FREQ2_ZQCTL0(0), 0x50800020 },
	{ DDRC_FREQ2_DFITMG0(0), 0x03818200 },
	{ DDRC_FREQ2_DFITMG1(0), 0x00020103 },
	{ DDRC_FREQ2_DFITMG2(0), 0x00000100 },
	{ DDRC_FREQ2_DFITMG3(0), 0x00000001 },
	{ DDRC_FREQ2_ODTCFG(0), 0x04050800 },

	/* default start freq point */
	{ DDRC_MSTR2(0), 0x2},
};

/* PHY Initialize Configuration */
struct dram_cfg_param ddr3l_ddrphy_cfg[] = {
	{ 0x1005f, 0x3cf },
	{ 0x1015f, 0x3cf },
	{ 0x1105f, 0x3cf },
	{ 0x1115f, 0x3cf },
	{ 0x1205f, 0x3cf },
	{ 0x1215f, 0x3cf },
	{ 0x1305f, 0x3cf },
	{ 0x1315f, 0x3cf },

	{ 0x11005f, 0x3cf },
	{ 0x11015f, 0x3cf },
	{ 0x11105f, 0x3cf },
	{ 0x11115f, 0x3cf },
	{ 0x11205f, 0x3cf },
	{ 0x11215f, 0x3cf },
	{ 0x11305f, 0x3cf },
	{ 0x11315f, 0x3cf },

	{ 0x21005f, 0x3cf },
	{ 0x21015f, 0x3cf },
	{ 0x21105f, 0x3cf },
	{ 0x21115f, 0x3cf },
	{ 0x21205f, 0x3cf },
	{ 0x21215f, 0x3cf },
	{ 0x21305f, 0x3cf },
	{ 0x21315f, 0x3cf },

	{ 0x55, 0x365 },
	{ 0x1055, 0x365 },
	{ 0x2055, 0x365 },
	{ 0x3055, 0x365 },
	{ 0x4055, 0x65 },
	{ 0x5055, 0x65 },
	{ 0x6055, 0x365 },
	{ 0x7055, 0x365 },
	{ 0x8055, 0x365 },
	{ 0x9055, 0x365 },
	{ 0x200c5, 0xb },
	{ 0x1200c5, 0x7 },
	{ 0x2200c5, 0x7 },
	{ 0x2002e, 0x1 },
	{ 0x12002e, 0x1 },
	{ 0x22002e, 0x1 },
	{ 0x20024, 0x8 },
	{ 0x2003a, 0x0 },
	{ 0x120024, 0x8 },
	{ 0x2003a, 0x0 },
	{ 0x220024, 0x8 },
	{ 0x2003a, 0x0 },
	{ 0x20056, 0xa },
	{ 0x120056, 0xa },
	{ 0x220056, 0xa },
	{ 0x1004d, 0x618 },
	{ 0x1014d, 0x618 },
	{ 0x1104d, 0x618 },
	{ 0x1114d, 0x618 },
	{ 0x1204d, 0x618 },
	{ 0x1214d, 0x618 },
	{ 0x1304d, 0x618 },
	{ 0x1314d, 0x618 },
	{ 0x11004d, 0x618 },
	{ 0x11014d, 0x618 },
	{ 0x11104d, 0x618 },
	{ 0x11114d, 0x618 },
	{ 0x11204d, 0x618 },
	{ 0x11214d, 0x618 },
	{ 0x11304d, 0x618 },
	{ 0x11314d, 0x618 },
	{ 0x21004d, 0x618 },
	{ 0x21014d, 0x618 },
	{ 0x21104d, 0x618 },
	{ 0x21114d, 0x618 },
	{ 0x21204d, 0x618 },
	{ 0x21214d, 0x618 },
	{ 0x21304d, 0x618 },
	{ 0x21314d, 0x618 },
	{ 0x10049, 0xe38 },
	{ 0x10149, 0xe38 },
	{ 0x11049, 0xe38 },
	{ 0x11149, 0xe38 },
	{ 0x12049, 0xe38 },
	{ 0x12149, 0xe38 },
	{ 0x13049, 0xe38 },
	{ 0x13149, 0xe38 },
	{ 0x110049, 0xe38 },
	{ 0x110149, 0xe38 },
	{ 0x111049, 0xe38 },
	{ 0x111149, 0xe38 },
	{ 0x112049, 0xe38 },
	{ 0x112149, 0xe38 },
	{ 0x113049, 0xe38 },
	{ 0x113149, 0xe38 },
	{ 0x210049, 0xe38 },
	{ 0x210149, 0xe38 },
	{ 0x211049, 0xe38 },
	{ 0x211149, 0xe38 },
	{ 0x212049, 0xe38 },
	{ 0x212149, 0xe38 },
	{ 0x213049, 0xe38 },
	{ 0x213149, 0xe38 },
	{ 0x43, 0x63 },
	{ 0x1043, 0x63 },
	{ 0x2043, 0x63 },
	{ 0x3043, 0x63 },
	{ 0x4043, 0x63 },
	{ 0x5043, 0x63 },
	{ 0x6043, 0x63 },
	{ 0x7043, 0x63 },
	{ 0x8043, 0x63 },
	{ 0x9043, 0x63 },
	{ 0x20018, 0x5 },
	{ 0x20075, 0x0 },
	{ 0x20050, 0x0 },
	{ 0x20008, 0x190 },
	{ 0x120008, 0x85 },
	{ 0x220008, 0x53 },
	{ 0x20088, 0x9 },
	{ 0x200b2, 0xf8 },
	{ 0x10043, 0x581 },
	{ 0x10143, 0x581 },
	{ 0x11043, 0x581 },
	{ 0x11143, 0x581 },
	{ 0x12043, 0x581 },
	{ 0x12143, 0x581 },
	{ 0x13043, 0x581 },
	{ 0x13143, 0x581 },
	{ 0x1200b2, 0xf8 },
	{ 0x110043, 0x581 },
	{ 0x110143, 0x581 },
	{ 0x111043, 0x581 },
	{ 0x111143, 0x581 },
	{ 0x112043, 0x581 },
	{ 0x112143, 0x581 },
	{ 0x113043, 0x581 },
	{ 0x113143, 0x581 },
	{ 0x2200b2, 0xf8 },
	{ 0x210043, 0x581 },
	{ 0x210143, 0x581 },
	{ 0x211043, 0x581 },
	{ 0x211143, 0x581 },
	{ 0x212043, 0x581 },
	{ 0x212143, 0x581 },
	{ 0x213043, 0x581 },
	{ 0x213143, 0x581 },
	{ 0x200fa, 0x1 },
	{ 0x1200fa, 0x1 },
	{ 0x2200fa, 0x1 },
	{ 0x20019, 0x5 },
	{ 0x120019, 0x5 },
	{ 0x220019, 0x5 },
	{ 0x200f0, 0x5555 },
	{ 0x200f1, 0x5555 },
	{ 0x200f2, 0x5555 },
	{ 0x200f3, 0x5555 },
	{ 0x200f4, 0x5555 },
	{ 0x200f5, 0x5555 },
	{ 0x200f6, 0x5555 },
	{ 0x200f7, 0xf000 },
	{ 0x20025, 0x0 },
};

/* P0 message block paremeter for training firmware */
struct dram_cfg_param ddr3l_fsp0_cfg[] = {
	{ 0xd0000, 0x0 },
	{ 0x54000, 0x0 },
	{ 0x54001, 0x0 },
	{ 0x54002, 0x0 },
	{ 0x54003, 0x640 },
	{ 0x54004, 0x2 },
	{ 0x54005, 0x0 },
	{ 0x54006, 0x140 },
	{ 0x54007, 0x2000 },
	{ 0x54008, 0x303 },
	{ 0x54009, 0x200 },
	{ 0x5400a, 0x0 },
	{ 0x5400b, 0x31f },
	{ 0x5400c, 0xc8 },
	{ 0x5400d, 0x0 },
	{ 0x5400e, 0x0 },
	{ 0x5400f, 0x0 },
	{ 0x54010, 0x0 },
	{ 0x54011, 0x0 },
	{ 0x54012, 0x1 },
	{ 0x5402f, 0xd70 },
	{ 0x54030, 0x4 },
	{ 0x54031, 0x18 },
	{ 0x5403a, 0x1221 },
	{ 0x5403b, 0x4884 },
	{ 0xd0000, 0x1 },
};

/* P1 message block paremeter for training firmware */
struct dram_cfg_param ddr3l_fsp1_cfg[] = {
	{ 0xd0000, 0x0 },
	{ 0x54000, 0x0 },
	{ 0x54001, 0x0 },
	{ 0x54002, 0x1 },
	{ 0x54003, 0x214 },
	{ 0x54004, 0x2 },
	{ 0x54005, 0x0 },
	{ 0x54006, 0x140 },
	{ 0x54007, 0x2000 },
	{ 0x54008, 0x303 },
	{ 0x54009, 0x200 },
	{ 0x5400a, 0x0 },
	{ 0x5400b, 0x21f },
	{ 0x5400c, 0xc8 },
	{ 0x5400d, 0x0 },
	{ 0x5400e, 0x0 },
	{ 0x5400f, 0x0 },
	{ 0x54010, 0x0 },
	{ 0x54011, 0x0 },
	{ 0x54012, 0x1 },
	{ 0x5402f, 0x930 },
	{ 0x54030, 0x4 },
	{ 0x54031, 0x8 },
	{ 0x5403a, 0x1221 },
	{ 0x5403b, 0x4884 },
	{ 0xd0000, 0x1 },
};

/* P2 message block paremeter for training firmware */
struct dram_cfg_param ddr3l_fsp2_cfg[] = {
	{ 0xd0000, 0x0 },
	{ 0x54000, 0x0 },
	{ 0x54001, 0x0 },
	{ 0x54002, 0x2 },
	{ 0x54003, 0x14c },
	{ 0x54004, 0x2 },
	{ 0x54005, 0x0 },
	{ 0x54006, 0x140 },
	{ 0x54007, 0x2000 },
	{ 0x54008, 0x303 },
	{ 0x54009, 0x200 },
	{ 0x5400a, 0x0 },
	{ 0x5400b, 0x21f },
	{ 0x5400c, 0xc8 },
	{ 0x5400d, 0x0 },
	{ 0x5400e, 0x0 },
	{ 0x5400f, 0x0 },
	{ 0x54010, 0x0 },
	{ 0x54011, 0x0 },
	{ 0x54012, 0x1 },
	{ 0x5402f, 0x520 },
	{ 0x54030, 0x4 },
	{ 0x54031, 0x0 },
	{ 0x5403a, 0x1221 },
	{ 0x5403b, 0x4884 },
	{ 0xd0000, 0x1 },
};

/* DRAM PHY init engine image */
struct dram_cfg_param ddr3l_phy_pie[] = {
	{ 0xd0000, 0x0 }, /*  DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
	{ 0x90000, 0x10 }, /*  DWC_DDRPHYA_INITENG0_PreSequenceReg0b0s0 */
	{ 0x90001, 0x400 }, /*  DWC_DDRPHYA_INITENG0_PreSequenceReg0b0s1 */
	{ 0x90002, 0x10e }, /*  DWC_DDRPHYA_INITENG0_PreSequenceReg0b0s2 */
	{ 0x90003, 0x0 }, /*  DWC_DDRPHYA_INITENG0_PreSequenceReg0b1s0 */
	{ 0x90004, 0x0 }, /*  DWC_DDRPHYA_INITENG0_PreSequenceReg0b1s1 */
	{ 0x90005, 0x8 }, /*  DWC_DDRPHYA_INITENG0_PreSequenceReg0b1s2 */
	{ 0x90029, 0xb }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b0s0 */
	{ 0x9002a, 0x480 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b0s1 */
	{ 0x9002b, 0x109 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b0s2 */
	{ 0x9002c, 0x8 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b1s0 */
	{ 0x9002d, 0x448 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b1s1 */
	{ 0x9002e, 0x139 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b1s2 */
	{ 0x9002f, 0x8 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b2s0 */
	{ 0x90030, 0x478 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b2s1 */
	{ 0x90031, 0x109 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b2s2 */
	{ 0x90032, 0x2 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b3s0 */
	{ 0x90033, 0x10 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b3s1 */
	{ 0x90034, 0x139 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b3s2 */
	{ 0x90035, 0xf }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b4s0 */
	{ 0x90036, 0x7c0 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b4s1 */
	{ 0x90037, 0x139 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b4s2 */
	{ 0x90038, 0x44 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b5s0 */
	{ 0x90039, 0x630 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b5s1 */
	{ 0x9003a, 0x159 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b5s2 */
	{ 0x9003b, 0x14f }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b6s0 */
	{ 0x9003c, 0x630 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b6s1 */
	{ 0x9003d, 0x159 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b6s2 */
	{ 0x9003e, 0x47 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b7s0 */
	{ 0x9003f, 0x630 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b7s1 */
	{ 0x90040, 0x149 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b7s2 */
	{ 0x90041, 0x4f }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b8s0 */
	{ 0x90042, 0x630 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b8s1 */
	{ 0x90043, 0x179 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b8s2 */
	{ 0x90044, 0x8 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b9s0 */
	{ 0x90045, 0xe0 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b9s1 */
	{ 0x90046, 0x109 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b9s2 */
	{ 0x90047, 0x0 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b10s0 */
	{ 0x90048, 0x7c8 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b10s1 */
	{ 0x90049, 0x109 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b10s2 */
	{ 0x9004a, 0x0 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b11s0 */
	{ 0x9004b, 0x1 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b11s1 */
	{ 0x9004c, 0x8 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b11s2 */
	{ 0x9004d, 0x0 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b12s0 */
	{ 0x9004e, 0x45a }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b12s1 */
	{ 0x9004f, 0x9 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b12s2 */
	{ 0x90050, 0x0 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b13s0 */
	{ 0x90051, 0x448 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b13s1 */
	{ 0x90052, 0x109 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b13s2 */
	{ 0x90053, 0x40 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b14s0 */
	{ 0x90054, 0x630 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b14s1 */
	{ 0x90055, 0x179 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b14s2 */
	{ 0x90056, 0x1 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b15s0 */
	{ 0x90057, 0x618 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b15s1 */
	{ 0x90058, 0x109 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b15s2 */
	{ 0x90059, 0x40c0 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b16s0 */
	{ 0x9005a, 0x630 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b16s1 */
	{ 0x9005b, 0x149 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b16s2 */
	{ 0x9005c, 0x8 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b17s0 */
	{ 0x9005d, 0x4 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b17s1 */
	{ 0x9005e, 0x48 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b17s2 */
	{ 0x9005f, 0x4040 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b18s0 */
	{ 0x90060, 0x630 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b18s1 */
	{ 0x90061, 0x149 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b18s2 */
	{ 0x90062, 0x0 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b19s0 */
	{ 0x90063, 0x4 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b19s1 */
	{ 0x90064, 0x48 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b19s2 */
	{ 0x90065, 0x40 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b20s0 */
	{ 0x90066, 0x630 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b20s1 */
	{ 0x90067, 0x149 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b20s2 */
	{ 0x90068, 0x10 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b21s0 */
	{ 0x90069, 0x4 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b21s1 */
	{ 0x9006a, 0x18 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b21s2 */
	{ 0x9006b, 0x0 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b22s0 */
	{ 0x9006c, 0x4 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b22s1 */
	{ 0x9006d, 0x78 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b22s2 */
	{ 0x9006e, 0x549 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b23s0 */
	{ 0x9006f, 0x630 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b23s1 */
	{ 0x90070, 0x159 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b23s2 */
	{ 0x90071, 0xd49 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b24s0 */
	{ 0x90072, 0x630 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b24s1 */
	{ 0x90073, 0x159 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b24s2 */
	{ 0x90074, 0x94a }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b25s0 */
	{ 0x90075, 0x630 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b25s1 */
	{ 0x90076, 0x159 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b25s2 */
	{ 0x90077, 0x441 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b26s0 */
	{ 0x90078, 0x630 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b26s1 */
	{ 0x90079, 0x149 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b26s2 */
	{ 0x9007a, 0x42 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b27s0 */
	{ 0x9007b, 0x630 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b27s1 */
	{ 0x9007c, 0x149 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b27s2 */
	{ 0x9007d, 0x1 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b28s0 */
	{ 0x9007e, 0x630 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b28s1 */
	{ 0x9007f, 0x149 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b28s2 */
	{ 0x90080, 0x0 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b29s0 */
	{ 0x90081, 0xe0 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b29s1 */
	{ 0x90082, 0x109 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b29s2 */
	{ 0x90083, 0xa }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b30s0 */
	{ 0x90084, 0x10 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b30s1 */
	{ 0x90085, 0x109 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b30s2 */
	{ 0x90086, 0x9 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b31s0 */
	{ 0x90087, 0x3c0 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b31s1 */
	{ 0x90088, 0x149 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b31s2 */
	{ 0x90089, 0x9 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b32s0 */
	{ 0x9008a, 0x3c0 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b32s1 */
	{ 0x9008b, 0x159 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b32s2 */
	{ 0x9008c, 0x18 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b33s0 */
	{ 0x9008d, 0x10 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b33s1 */
	{ 0x9008e, 0x109 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b33s2 */
	{ 0x9008f, 0x0 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b34s0 */
	{ 0x90090, 0x3c0 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b34s1 */
	{ 0x90091, 0x109 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b34s2 */
	{ 0x90092, 0x18 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b35s0 */
	{ 0x90093, 0x4 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b35s1 */
	{ 0x90094, 0x48 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b35s2 */
	{ 0x90095, 0x18 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b36s0 */
	{ 0x90096, 0x4 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b36s1 */
	{ 0x90097, 0x58 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b36s2 */
	{ 0x90098, 0xa }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b37s0 */
	{ 0x90099, 0x10 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b37s1 */
	{ 0x9009a, 0x109 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b37s2 */
	{ 0x9009b, 0x2 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b38s0 */
	{ 0x9009c, 0x10 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b38s1 */
	{ 0x9009d, 0x109 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b38s2 */
	{ 0x9009e, 0x7 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b39s0 */
	{ 0x9009f, 0x7c0 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b39s1 */
	{ 0x900a0, 0x109 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b39s2 */
	{ 0x900a1, 0x10 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b40s0 */
	{ 0x900a2, 0x10 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b40s1 */
	{ 0x900a3, 0x109 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b40s2 */
	{ 0x900a4, 0x0 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b41s0 */
	{ 0x900a5, 0x8140 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b41s1 */
	{ 0x900a6, 0x10c }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b41s2 */
	{ 0x900a7, 0x10 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b42s0 */
	{ 0x900a8, 0x8138 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b42s1 */
	{ 0x900a9, 0x10c }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b42s2 */
	{ 0x900aa, 0x8 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b43s0 */
	{ 0x900ab, 0x7c8 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b43s1 */
	{ 0x900ac, 0x101 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b43s2 */
	{ 0x900ad, 0x8 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b44s0 */
	{ 0x900ae, 0x0 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b44s1 */
	{ 0x900af, 0x8 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b44s2 */
	{ 0x900b0, 0x8 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b45s0 */
	{ 0x900b1, 0x448 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b45s1 */
	{ 0x900b2, 0x109 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b45s2 */
	{ 0x900b3, 0xf }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b46s0 */
	{ 0x900b4, 0x7c0 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b46s1 */
	{ 0x900b5, 0x109 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b46s2 */
	{ 0x900b6, 0x47 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b47s0 */
	{ 0x900b7, 0x630 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b47s1 */
	{ 0x900b8, 0x109 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b47s2 */
	{ 0x900b9, 0x8 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b48s0 */
	{ 0x900ba, 0x618 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b48s1 */
	{ 0x900bb, 0x109 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b48s2 */
	{ 0x900bc, 0x8 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b49s0 */
	{ 0x900bd, 0xe0 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b49s1 */
	{ 0x900be, 0x109 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b49s2 */
	{ 0x900bf, 0x0 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b50s0 */
	{ 0x900c0, 0x7c8 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b50s1 */
	{ 0x900c1, 0x109 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b50s2 */
	{ 0x900c2, 0x8 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b51s0 */
	{ 0x900c3, 0x8140 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b51s1 */
	{ 0x900c4, 0x10c }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b51s2 */
	{ 0x900c5, 0x0 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b52s0 */
	{ 0x900c6, 0x1 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b52s1 */
	{ 0x900c7, 0x8 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b52s2 */
	{ 0x900c8, 0x8 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b53s0 */
	{ 0x900c9, 0x4 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b53s1 */
	{ 0x900ca, 0x8 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b53s2 */
	{ 0x900cb, 0x8 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b54s0 */
	{ 0x900cc, 0x7c8 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b54s1 */
	{ 0x900cd, 0x101 }, /*  DWC_DDRPHYA_INITENG0_SequenceReg0b54s2 */
	{ 0x90006, 0x0 }, /*  DWC_DDRPHYA_INITENG0_PostSequenceReg0b0s0 */
	{ 0x90007, 0x0 }, /*  DWC_DDRPHYA_INITENG0_PostSequenceReg0b0s1 */
	{ 0x90008, 0x8 }, /*  DWC_DDRPHYA_INITENG0_PostSequenceReg0b0s2 */
	{ 0x90009, 0x0 }, /*  DWC_DDRPHYA_INITENG0_PostSequenceReg0b1s0 */
	{ 0x9000a, 0x0 }, /*  DWC_DDRPHYA_INITENG0_PostSequenceReg0b1s1 */
	{ 0x9000b, 0x0 }, /*  DWC_DDRPHYA_INITENG0_PostSequenceReg0b1s2 */
	{ 0xd00e7, 0x400 }, /*  DWC_DDRPHYA_APBONLY0_SequencerOverride */
	{ 0x90017, 0x0 }, /*  DWC_DDRPHYA_INITENG0_StartVector0b0 */
	{ 0x90026, 0x2c }, /*  DWC_DDRPHYA_INITENG0_StartVector0b15 */
	{ 0x2000b, 0x32 }, /*  DWC_DDRPHYA_MASTER0_Seq0BDLY0_p0 */
	{ 0x2000c, 0x64 }, /*  DWC_DDRPHYA_MASTER0_Seq0BDLY1_p0 */
	{ 0x2000d, 0x3e8 }, /*  DWC_DDRPHYA_MASTER0_Seq0BDLY2_p0 */
	{ 0x2000e, 0x2c }, /*  DWC_DDRPHYA_MASTER0_Seq0BDLY3_p0 */
	{ 0x12000b, 0x10 }, /*  DWC_DDRPHYA_MASTER0_Seq0BDLY0_p1 */
	{ 0x12000c, 0x21 }, /*  DWC_DDRPHYA_MASTER0_Seq0BDLY1_p1 */
	{ 0x12000d, 0x14c }, /*  DWC_DDRPHYA_MASTER0_Seq0BDLY2_p1 */
	{ 0x12000e, 0x10 }, /*  DWC_DDRPHYA_MASTER0_Seq0BDLY3_p1 */
	{ 0x22000b, 0xa }, /*  DWC_DDRPHYA_MASTER0_Seq0BDLY0_p2 */
	{ 0x22000c, 0x14 }, /*  DWC_DDRPHYA_MASTER0_Seq0BDLY1_p2 */
	{ 0x22000d, 0xcf }, /*  DWC_DDRPHYA_MASTER0_Seq0BDLY2_p2 */
	{ 0x22000e, 0x10 }, /*  DWC_DDRPHYA_MASTER0_Seq0BDLY3_p2 */
	{ 0x9000c, 0x0 }, /*  DWC_DDRPHYA_INITENG0_Seq0BDisableFlag0 */
	{ 0x9000d, 0x173 }, /*  DWC_DDRPHYA_INITENG0_Seq0BDisableFlag1 */
	{ 0x9000e, 0x60 }, /*  DWC_DDRPHYA_INITENG0_Seq0BDisableFlag2 */
	{ 0x9000f, 0x6110 }, /*  DWC_DDRPHYA_INITENG0_Seq0BDisableFlag3 */
	{ 0x90010, 0x2152 }, /*  DWC_DDRPHYA_INITENG0_Seq0BDisableFlag4 */
	{ 0x90011, 0xdfbd }, /*  DWC_DDRPHYA_INITENG0_Seq0BDisableFlag5 */
	{ 0x90012, 0xffff }, /*  DWC_DDRPHYA_INITENG0_Seq0BDisableFlag6 */
	{ 0x90013, 0x6152 }, /*  DWC_DDRPHYA_INITENG0_Seq0BDisableFlag7 */
	{ 0xc0080, 0x0 }, /*  DWC_DDRPHYA_DRTUB0_UcclkHclkEnables */
	{ 0xd0000, 0x1 }, /*  DWC_DDRPHYA_APBONLY0_MicroContMuxSel */
};

struct dram_fsp_msg ddr3l_dram_fsp_msg[] = {
	{
		/* P0 2400mts 1D */
		.drate = 1600,
		.fw_type = FW_1D_IMAGE,
		.fsp_cfg = ddr3l_fsp0_cfg,
		.fsp_cfg_num = ARRAY_SIZE(ddr3l_fsp0_cfg),
	},
#if 1
	{
		/* P1 1066mts 1D */
		.drate = 1066,
		.fw_type = FW_1D_IMAGE,
		.fsp_cfg = ddr3l_fsp1_cfg,
		.fsp_cfg_num = ARRAY_SIZE(ddr3l_fsp1_cfg),
	},
	{
		/* P2 667mts 1D */
		.drate = 667,
		.fw_type = FW_1D_IMAGE,
		.fsp_cfg = ddr3l_fsp2_cfg,
		.fsp_cfg_num = ARRAY_SIZE(ddr3l_fsp2_cfg),
	},
#endif
};

/* ddr3l timing config params on VAL board */
struct dram_timing_info dram_timing = {
	.ddrc_cfg = ddr3l_ddrc_cfg,
	.ddrc_cfg_num = ARRAY_SIZE(ddr3l_ddrc_cfg),
	.ddrphy_cfg = ddr3l_ddrphy_cfg,
	.ddrphy_cfg_num = ARRAY_SIZE(ddr3l_ddrphy_cfg),
	.fsp_msg = ddr3l_dram_fsp_msg,
	.fsp_msg_num = ARRAY_SIZE(ddr3l_dram_fsp_msg),
	.ddrphy_pie = ddr3l_phy_pie,
	.ddrphy_pie_num = ARRAY_SIZE(ddr3l_phy_pie),
	.fsp_table = { 1600, 1066, 667 },
};
