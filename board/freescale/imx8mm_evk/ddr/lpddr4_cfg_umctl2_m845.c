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
#include "lpddr4_define.h"

struct ddr_ctl_param
{
	u32 reg;        /*reg address */
	u32 val;        /*config param */
};

static struct ddr_ctl_param ctl_init_cfg[] =
{
	{ .reg =DDRC_DBG1(0), .val = 	0x00000001},
	{ .reg =DDRC_PWRCTL(0), .val = 	0x00000001},
#ifdef DDR_ONE_RANK
	{ .reg =DDRC_MSTR(0), .val = 	0xa1080020},
#else
	{ .reg =DDRC_MSTR(0), .val = 	0xa3080020},
#endif
#ifdef DDR_800M_CFG
	{ .reg =DDRC_RFSHTMG(0), .val = 	0x006100E0},
#else
	{ .reg =DDRC_RFSHTMG(0), .val = 	0x005b00d2},
#endif
#ifdef PHY_TRAIN
	{ .reg =DDRC_INIT0(0), .val = 	0xC003061B},
#else
#ifdef DDR_FAST_SIM
	{ .reg =DDRC_INIT0(0), .val = 	0x00030003},
#else
	{ .reg =DDRC_INIT0(0), .val = 	0x0003061B},
#endif
#endif
#ifdef DDR_FAST_SIM
	{ .reg =DDRC_INIT1(0), .val = 	0x00060000},
#else
	{ .reg =DDRC_INIT1(0), .val = 	0x009D0000},
#endif
	{ .reg =DDRC_INIT3(0), .val = 	0x00D4002D},
#ifdef WR_POST_EXT_3200
	{ .reg =DDRC_INIT4(0), .val = 	0x00330008},
#else

	{ .reg =DDRC_INIT4(0), .val = 	0x00310000},
#endif
	{ .reg =DDRC_INIT6(0), .val = 	0x0066004a},
	{ .reg =DDRC_INIT7(0), .val = 	0x0006004a},

	{ .reg =DDRC_DRAMTMG0(0), .val = 0x1A201B22},
	{ .reg =DDRC_DRAMTMG1(0), .val = 0x00060633},
	{ .reg =DDRC_DRAMTMG3(0), .val = 0x00C0C000},
	{ .reg =DDRC_DRAMTMG4(0), .val = 0x0F04080F},
	{ .reg =DDRC_DRAMTMG5(0), .val = 0x02040C0C},
	{ .reg =DDRC_DRAMTMG6(0), .val = 0x01010007},
	{ .reg =DDRC_DRAMTMG7(0), .val = 0x00000401},
	{ .reg =DDRC_DRAMTMG12(0), .val = 0x00020600},
	{ .reg =DDRC_DRAMTMG13(0), .val = 0x0C100002},
	{ .reg =DDRC_DRAMTMG14(0), .val = 0x000000E6},
	{ .reg =DDRC_DRAMTMG17(0), .val = 0x00A00050},

	{ .reg =DDRC_ZQCTL0(0), .val = 	0x03200018},
	{ .reg =DDRC_ZQCTL1(0), .val = 	0x028061A8},
	{ .reg =DDRC_ZQCTL2(0), .val = 	0x00000000},

	{ .reg =DDRC_DFITMG0(0), .val = 	0x0497820A},
	{ .reg =DDRC_DFITMG1(0), .val = 	0x00080303},
	{ .reg =DDRC_DFIUPD0(0), .val = 	0xE0400018},

	{ .reg =DDRC_DFIUPD1(0), .val = 	0x00DF00E4},
	{ .reg =DDRC_DFIUPD2(0), .val = 	0x80000000},
	{ .reg =DDRC_DFIMISC(0), .val = 	0x00000011},
	{ .reg =DDRC_DFITMG2(0), .val = 	0x0000170A},

	{ .reg =DDRC_DBICTL(0), .val = 	0x00000001},
#ifdef BUG_WR_DFI
	{ .reg =DDRC_DFIPHYMSTR(0), .val = 0x00000000},
#else
	{ .reg =DDRC_DFIPHYMSTR(0), .val = 0x00000001},
#endif
	{ .reg =DDRC_RANKCTL(0), .val = 	0x00000c99},
	{ .reg =DDRC_DRAMTMG2(0), .val = 0x070E171a},
#ifdef M845S_4GBx2
#ifdef DDR_ONE_RANK
	{ .reg =DDRC_ADDRMAP0(0), .val =  0x0000001f},
#else
	{ .reg =DDRC_ADDRMAP0(0), .val =  0x00000017},
#endif
	{ .reg =DDRC_ADDRMAP1(0), .val =  0x00080808},
	{ .reg =DDRC_ADDRMAP2(0), .val =  0x00000000},
	{ .reg =DDRC_ADDRMAP3(0), .val =  0x00000000},
	{ .reg =DDRC_ADDRMAP4(0), .val =  0x00001f1f},
	{ .reg =DDRC_ADDRMAP5(0), .val =  0x07070707},
	{ .reg =DDRC_ADDRMAP6(0), .val =  0x07070707},
	{ .reg =DDRC_ADDRMAP7(0), .val =  0x00000f0f},
#else
#ifdef DDR_ONE_RANK
	{ .reg =DDRC_ADDRMAP0(0), .val =  0x0000001f},
#else
	{ .reg =DDRC_ADDRMAP0(0), .val =  0x00000016},
#endif
	{ .reg =DDRC_ADDRMAP1(0), .val =  0x00080808},
	{ .reg =DDRC_ADDRMAP2(0), .val =  0x00000000},
	{ .reg =DDRC_ADDRMAP3(0), .val =  0x00000000},
	{ .reg =DDRC_ADDRMAP4(0), .val =  0x00001f1f},
	{ .reg =DDRC_ADDRMAP5(0), .val =  0x07070707},
	{ .reg =DDRC_ADDRMAP6(0), .val =  0x0f070707},
	{ .reg =DDRC_ADDRMAP7(0), .val =  0x00000f0f},
	{ .reg =DDRC_ADDRMAP8(0), .val =  0x00000000},
	{ .reg =DDRC_ADDRMAP9(0), .val =  0x0a020b06},
	{ .reg =DDRC_ADDRMAP10(0), .val =  0x0a0a0a0a},
	{ .reg =DDRC_ADDRMAP11(0), .val =  0x00000000},
#endif

#ifdef PERF_TEST_2
	{ .reg =DDRC_SCHED(0), .val =  0x29001701},
	{ .reg =DDRC_SCHED1(0), .val =  0x0000002c},
	{ .reg =DDRC_PERFLPR1(0), .val =  0x900093e7},
	{ .reg =DDRC_PCCFG(0), .val =  0x00000111},
	{ .reg =DDRC_PCFGW_0(0), .val =  0x000072ff},
	{ .reg =DDRC_PCFGQOS0_0(0), .val =  0x02100e07},
	{ .reg =DDRC_PCFGQOS1_0(0), .val =  0x00620096},
	{ .reg =DDRC_PCFGWQOS0_0(0), .val =  0x01100e07},
	{ .reg =DDRC_PCFGWQOS1_0(0), .val =  0x0000012c},
#else
	{ .reg =DDRC_SCHED(0), .val =  0x29001701},
	{ .reg =DDRC_SCHED1(0), .val =  0x0000002c},
	{ .reg =DDRC_PERFHPR1(0), .val =  0x04000030},
	{ .reg =DDRC_PERFLPR1(0), .val =  0x900093e7},
	{ .reg =DDRC_PCCFG(0), .val =  0x00000111},
	{ .reg =DDRC_PCFGW_0(0), .val =  0x000072ff},
	{ .reg =DDRC_PCFGQOS0_0(0), .val =  0x02100e07},
	{ .reg =DDRC_PCFGQOS1_0(0), .val =  0x00620096},
	{ .reg =DDRC_PCFGWQOS0_0(0), .val =  0x01100e07},
	{ .reg =DDRC_PCFGWQOS1_0(0), .val =  0x0000012c},
#endif

#ifdef P1_400
	{ .reg =DDRC_FREQ1_DRAMTMG0(0), .val =  0x0d0b010c},
	{ .reg =DDRC_FREQ1_DRAMTMG1(0), .val =  0x00030410},
	{ .reg =DDRC_FREQ1_DRAMTMG2(0), .val =  0x0305090c},
	{ .reg =DDRC_FREQ1_DRAMTMG3(0), .val =  0x00505006},
	{ .reg =DDRC_FREQ1_DRAMTMG4(0), .val =  0x05040305},
	{ .reg =DDRC_FREQ1_DRAMTMG5(0), .val =  0x0d0e0504},
	{ .reg =DDRC_FREQ1_DRAMTMG6(0), .val =  0x0a060004},
	{ .reg =DDRC_FREQ1_DRAMTMG7(0), .val =  0x0000090e},
	{ .reg =DDRC_FREQ1_DRAMTMG14(0), .val =  0x00000032},
	{ .reg =DDRC_FREQ1_DRAMTMG15(0), .val =  0x00000000},
	{ .reg =DDRC_FREQ1_DRAMTMG17(0), .val =  0x0036001b},
	{ .reg =DDRC_FREQ1_DERATEINT(0), .val =  0x7e9fbeb1},
	{ .reg =DDRC_FREQ1_DFITMG0(0), .val =  0x03818200},
	{ .reg =DDRC_FREQ1_DFITMG2(0), .val =  0x00000000},
	{ .reg =DDRC_FREQ1_RFSHTMG(0), .val =  0x000C001c},
	{ .reg =DDRC_FREQ1_INIT3(0), .val =  0x00840000},
	{ .reg =DDRC_FREQ1_INIT4(0), .val =  0x00310000},
	{ .reg =DDRC_FREQ1_INIT6(0), .val =  0x0066004a},
	{ .reg =DDRC_FREQ1_INIT7(0), .val =  0x0006004a},
#else
#ifdef WEI_667
	{ .reg =DDRC_FREQ1_DRAMTMG0(0), .val =  0x0d0b0107},
	{ .reg =DDRC_FREQ1_DRAMTMG1(0), .val =  0x00030410},
	{ .reg =DDRC_FREQ1_DRAMTMG2(0), .val =  0x0305080c},
	{ .reg =DDRC_FREQ1_DRAMTMG3(0), .val =  0x00505006},
	{ .reg =DDRC_FREQ1_DRAMTMG4(0), .val =  0x05040305},
	{ .reg =DDRC_FREQ1_DRAMTMG5(0), .val =  0x0f0b0504},
	{ .reg =DDRC_FREQ1_DRAMTMG6(0), .val =  0x0e0c000c},
	{ .reg =DDRC_FREQ1_DRAMTMG7(0), .val =  0x00000607},
	{ .reg =DDRC_FREQ1_DRAMTMG14(0), .val =  0x00000066},
	{ .reg =DDRC_FREQ1_DRAMTMG15(0), .val =  0x80000000},
	{ .reg =DDRC_FREQ1_DRAMTMG17(0), .val =  0x0036001b},
	{ .reg =DDRC_FREQ1_DFITMG0(0), .val =  0x03858202},
	{ .reg =DDRC_FREQ1_DFITMG2(0), .val =  0x00000502},
	{ .reg =DDRC_FREQ1_DERATEEN(0), .val =  0x00000001},
	{ .reg =DDRC_FREQ1_DERATEINT(0), .val =  0x2545eb1c},
	{ .reg =DDRC_FREQ1_RFSHTMG(0), .val =  0x0014002f},
	{ .reg =DDRC_FREQ1_INIT3(0), .val =  0x00140009},
	{ .reg =DDRC_FREQ1_INIT4(0), .val =  0x00310000},
	{ .reg =DDRC_FREQ1_INIT6(0), .val =  0x0066004d},
	{ .reg =DDRC_FREQ1_INIT7(0), .val =  0x0006004d},
#else
	{ .reg =DDRC_FREQ1_DERATEEN(0),  .val =  0x0000000},
	{ .reg =DDRC_FREQ1_DERATEINT(0), .val =  0x0800000},
	{ .reg =DDRC_FREQ1_RFSHCTL0(0),  .val =  0x0210000},
	{ .reg =DDRC_FREQ1_RFSHTMG(0),   .val =  0x014001E},
	{ .reg =DDRC_FREQ1_INIT3(0),     .val =  0x0140009},
	{ .reg =DDRC_FREQ1_INIT4(0), .val =  0x00310000},
	{ .reg =DDRC_FREQ1_INIT6(0), .val =  0x0066004a},
	{ .reg =DDRC_FREQ1_INIT7(0), .val =  0x0006004a},
	{ .reg =DDRC_FREQ1_DRAMTMG0(0),  .val =  0xB070A07},
	{ .reg =DDRC_FREQ1_DRAMTMG1(0),  .val =  0x003040A},
	{ .reg =DDRC_FREQ1_DRAMTMG2(0),  .val =  0x305080C},
	{ .reg =DDRC_FREQ1_DRAMTMG3(0),  .val =  0x0505000},
	{ .reg =DDRC_FREQ1_DRAMTMG4(0),  .val =  0x3040203},
	{ .reg =DDRC_FREQ1_DRAMTMG5(0),  .val =  0x2030303},
	{ .reg =DDRC_FREQ1_DRAMTMG6(0),  .val =  0x2020004},
	{ .reg =DDRC_FREQ1_DRAMTMG7(0),  .val =  0x0000302},
	{ .reg =DDRC_FREQ1_DRAMTMG12(0), .val =  0x0020310},
	{ .reg =DDRC_FREQ1_DRAMTMG13(0), .val =  0xA100002},
	{ .reg =DDRC_FREQ1_DRAMTMG14(0), .val =  0x0000020},
	{ .reg =DDRC_FREQ1_DRAMTMG17(0), .val =  0x0220011},
	{ .reg =DDRC_FREQ1_ZQCTL0(0),    .val =  0x0A70005},
	{ .reg =DDRC_FREQ1_DFITMG0(0),   .val =  0x3858202},
	{ .reg =DDRC_FREQ1_DFITMG1(0),   .val =  0x0000404},
	{ .reg =DDRC_FREQ1_DFITMG2(0),   .val =  0x0000502},
#endif
#endif
	{ .reg =DDRC_FREQ2_DRAMTMG0(0), .val =  0x0d0b010c},
	{ .reg =DDRC_FREQ2_DRAMTMG1(0), .val =  0x00030410},
	{ .reg =DDRC_FREQ2_DRAMTMG2(0), .val =  0x0305090c},
	{ .reg =DDRC_FREQ2_DRAMTMG3(0), .val =  0x00505006},
	{ .reg =DDRC_FREQ2_DRAMTMG4(0), .val =  0x05040305},
	{ .reg =DDRC_FREQ2_DRAMTMG5(0), .val =  0x0d0e0504},
	{ .reg =DDRC_FREQ2_DRAMTMG6(0), .val =  0x0a060004},
	{ .reg =DDRC_FREQ2_DRAMTMG7(0), .val =  0x0000090e},
	{ .reg =DDRC_FREQ2_DRAMTMG14(0), .val =  0x00000032},
	{ .reg =DDRC_FREQ2_DRAMTMG17(0), .val =  0x0036001b},
	{ .reg =DDRC_FREQ2_DERATEINT(0), .val =  0x7e9fbeb1},
	{ .reg =DDRC_FREQ2_DFITMG0(0), .val =  0x03818200},
	{ .reg =DDRC_FREQ2_DFITMG2(0), .val =  0x00000000},
	{ .reg =DDRC_FREQ2_RFSHTMG(0), .val =  0x00030007},
	{ .reg =DDRC_FREQ2_INIT3(0), .val =  0x00840000},
	{ .reg =DDRC_FREQ2_INIT4(0), .val =  0x00310000},
	{ .reg =DDRC_FREQ2_INIT6(0), .val =  0x0066004a},
	{ .reg =DDRC_FREQ2_INIT7(0), .val =  0x0006004a},
#ifdef DDR_BOOT_P2
	{ .reg =DDRC_MSTR2(0), .val = 	0x2},
#else
#ifdef DDR_BOOT_P1
	{ .reg =DDRC_MSTR2(0), .val = 	0x1},
#else
	{ .reg =DDRC_MSTR2(0), .val = 	0x0},
#endif
#endif
};

void lpddr4_3000mts_cfg_umctl2(void)
{
	u32 index, reg, val, num;

	num = sizeof(ctl_init_cfg)/sizeof(struct ddr_ctl_param);

	for (index = 0; index < num; index++) {
		val = ctl_init_cfg[index].val;
		reg = ctl_init_cfg[index].reg;
		writel(val, (void __iomem *)(u64)reg);
	}
}
