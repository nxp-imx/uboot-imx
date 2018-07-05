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
#include "anamix_common.h"
#include "ddr4_define.h"

unsigned int mr_value[3][7] = {
	{0xa34, 0x105, 0x1028, 0x240, 0x200, 0x200, 0x814}, /* pstate0 MR */
	{0x204, 0x104, 0x1000, 0x040, 0x200, 0x200, 0x014}, /* pstate1 MR */
	{0x204, 0x104, 0x1000, 0x040, 0x200, 0x200, 0x014} }; /* pstate2 MR */

static unsigned int cur_pstate;
unsigned int after_retention = 0;

void ddr4_dll_change(unsigned int pstate);
void ddr4_dll_no_change(unsigned int pstate);

void umctl2_cfg(void)
{
#ifdef DDR_ONE_RANK
	reg32_write(DDRC_MSTR(0), 0x81040010);
#else
	reg32_write(DDRC_MSTR(0), 0x83040010);
#endif

	reg32_write(DDRC_PWRCTL(0), 0x000000aa);
	reg32_write(DDRC_PWRTMG(0), 0x00221306);

	reg32_write(DDRC_RFSHCTL0(0), 0x00c0a070);
	reg32_write(DDRC_RFSHCTL1(0), 0x00010008);
	reg32_write(DDRC_RFSHCTL3(0), 0x00000010);
	reg32_write(DDRC_RFSHTMG(0), 0x004980f4);
	reg32_write(DDRC_CRCPARCTL0(0), 0x00000000);
	reg32_write(DDRC_CRCPARCTL1(0), 0x00001010);
	reg32_write(DDRC_INIT0(0), 0xc0030002);
	reg32_write(DDRC_INIT1(0), 0x00020009);
	reg32_write(DDRC_INIT2(0), 0x0000350f);
	reg32_write(DDRC_INIT3(0), (mr_value[0][0]<<16) | (mr_value[0][1]));
	reg32_write(DDRC_INIT4(0), (mr_value[0][2]<<16) | (mr_value[0][3]));
	reg32_write(DDRC_INIT5(0), 0x001103cb);
	reg32_write(DDRC_INIT6(0), (mr_value[0][4]<<16) | (mr_value[0][5]));
	reg32_write(DDRC_INIT7(0), mr_value[0][6]);
	reg32_write(DDRC_DIMMCTL(0), 0x00000032);
	reg32_write(DDRC_RANKCTL(0), 0x00000fc7);
	reg32_write(DDRC_DRAMTMG0(0), 0x14132813);
	reg32_write(DDRC_DRAMTMG1(0), 0x0004051b);
	reg32_write(DDRC_DRAMTMG2(0), 0x0808030f);
	reg32_write(DDRC_DRAMTMG3(0), 0x0000400c);
	reg32_write(DDRC_DRAMTMG4(0), 0x08030409);
	reg32_write(DDRC_DRAMTMG5(0), 0x0e090504);
	reg32_write(DDRC_DRAMTMG6(0), 0x05030000);
	reg32_write(DDRC_DRAMTMG7(0), 0x0000090e);
	reg32_write(DDRC_DRAMTMG8(0), 0x0606700c);
	reg32_write(DDRC_DRAMTMG9(0), 0x0002040c);
	reg32_write(DDRC_DRAMTMG10(0), 0x000f0c07);
	reg32_write(DDRC_DRAMTMG11(0), 0x1809011d);
	reg32_write(DDRC_DRAMTMG12(0), 0x0000000d);
	reg32_write(DDRC_DRAMTMG13(0), 0x2b000000);
	reg32_write(DDRC_DRAMTMG14(0), 0x000000a4);
	reg32_write(DDRC_DRAMTMG15(0), 0x00000000);
	reg32_write(DDRC_DRAMTMG17(0), 0x00250078);
	reg32_write(DDRC_ZQCTL0(0), 0x51000040);
	reg32_write(DDRC_ZQCTL1(0), 0x00000070);
	reg32_write(DDRC_ZQCTL2(0), 0x00000000);
	reg32_write(DDRC_DFITMG0(0), 0x038b820b);
	reg32_write(DDRC_DFITMG1(0), 0x02020103);
	reg32_write(DDRC_DFILPCFG0(0), 0x07f04011); /*  [8]dfi_lp_en_sr = 0 */
	reg32_write(DDRC_DFILPCFG1(0), 0x000000b0);
	reg32_write(DDRC_DFIUPD0(0), 0xe0400018);
	reg32_write(DDRC_DFIUPD1(0), 0x0048005a);
	reg32_write(DDRC_DFIUPD2(0), 0x80000000);
	reg32_write(DDRC_DFIMISC(0), 0x00000001);
	reg32_write(DDRC_DFITMG2(0), 0x00000b0b);
	reg32_write(DDRC_DFITMG3(0), 0x00000001);
	reg32_write(DDRC_DBICTL(0), 0x00000000);
	reg32_write(DDRC_DFIPHYMSTR(0), 0x00000000);

#ifdef DDR_ONE_RANK
	reg32_write(DDRC_ADDRMAP0(0), 0x0000001F);
#else
	reg32_write(DDRC_ADDRMAP0(0), 0x00000017); /* [4:0]cs0: 6+23 */
#endif
	reg32_write(DDRC_ADDRMAP1(0), 0x003F0909); /* [5:0] bank b0: 2+9; [13:8] b1: P3+9 ; [21:16] b2: 4+, unused */
	reg32_write(DDRC_ADDRMAP2(0), 0x01010100); /* [3:0] col-b2: 2;  [11:8] col-b3: 3+1; [19:16] col-b4: 4+1 ; [27:24] col-b5: 5+1 */
	reg32_write(DDRC_ADDRMAP3(0), 0x01010101); /* [3:0] col-b6: 6+1;  [11:8] col-b7: 7+1; [19:16] col-b8: 8+1 ; [27:24] col-b9: 9+1 */
	reg32_write(DDRC_ADDRMAP4(0), 0x00001f1f); /* col-b10, col-b11 not used */
	reg32_write(DDRC_ADDRMAP5(0), 0x07070707); /* [3:0] row-b0: 6+7;  [11:8] row-b1: 7+7; [19:16] row-b2_b10: 8~16+7; [27:24] row-b11: 17+7 */
	reg32_write(DDRC_ADDRMAP6(0), 0x07070707); /* [3:0] row-b12:18+7; [11:8] row-b13: 19+7; [19:16] row-b14:20+7; [27:24] row-b15: 21+7 */
	reg32_write(DDRC_ADDRMAP7(0), 0x00000f0f); /* col-b10, col-b11 not used */
	reg32_write(DDRC_ADDRMAP8(0), 0x00003F01); /* [5:0] bg-b0: 2+1; [13:8]bg-b1:3+, unused */
	reg32_write(DDRC_ADDRMAP9(0), 0x0a020b06); /*  it's valid only when ADDRMAP5.addrmap_row_b2_10 is set to value 15 */
	reg32_write(DDRC_ADDRMAP10(0), 0x0a0a0a0a);/*  it's valid only when ADDRMAP5.addrmap_row_b2_10 is set to value 15 */
	reg32_write(DDRC_ADDRMAP11(0), 0x00000000);

	/* FREQ0: BL8, CL=16, CWL=16, WR_PREAMBLE = 1,RD_PREAMBLE = 1, CRC_MODE = 1, so wr_odt_hold=5+1+1=7 */
	/* wr_odt_delay=DFITMG1.dfi_t_cmd_lat=0 */
	reg32_write(DDRC_ODTCFG(0), 0x07000600);
#ifdef DDR_ONE_RANK
	reg32_write(DDRC_ODTMAP(0), 0x0001);
#else
	reg32_write(DDRC_ODTMAP(0), 0x0201);/* disable ODT0x00001120); */
#endif
	reg32_write(DDRC_SCHED(0), 0x317d1a07);
	reg32_write(DDRC_SCHED1(0), 0x0000000f);
	reg32_write(DDRC_PERFHPR1(0), 0x2a001b76);
	reg32_write(DDRC_PERFLPR1(0), 0x7300b473);
	reg32_write(DDRC_PERFWR1(0), 0x30000e06);
	reg32_write(DDRC_DBG0(0), 0x00000014);
	reg32_write(DDRC_DBG1(0), 0x00000000);
	reg32_write(DDRC_DBGCMD(0), 0x00000000);
	reg32_write(DDRC_SWCTL(0), 0x00000001);
	reg32_write(DDRC_POISONCFG(0), 0x00000010);
	reg32_write(DDRC_PCCFG(0), 0x00000100);/* bl_exp_mode=1 */
	reg32_write(DDRC_PCFGR_0(0), 0x00013193);
	reg32_write(DDRC_PCFGW_0(0), 0x00006096);
	reg32_write(DDRC_PCTRL_0(0), 0x00000001);
	reg32_write(DDRC_PCFGQOS0_0(0), 0x02000c00);
	reg32_write(DDRC_PCFGQOS1_0(0), 0x003c00db);
	reg32_write(DDRC_PCFGWQOS0_0(0), 0x00100009);
	reg32_write(DDRC_PCFGWQOS1_0(0), 0x00000002);

}

void umctl2_freq1_cfg(void)
{
	reg32_write(DDRC_FREQ1_RFSHCTL0(0), 0x0021a0c0);
#ifdef PLLBYPASS_250MBPS
	reg32_write(DDRC_FREQ1_RFSHTMG(0), 0x000f0011);/* tREFI=7.8us */
#endif
#ifdef PLLBYPASS_400MBPS
	reg32_write(DDRC_FREQ1_RFSHTMG(0), 0x0018001a);/* tREFI=7.8us */
#endif

	reg32_write(DDRC_FREQ1_INIT3(0), (mr_value[1][0]<<16) | (mr_value[1][1]));
	reg32_write(DDRC_FREQ1_INIT4(0), (mr_value[1][2]<<16) | (mr_value[1][3]));
	reg32_write(DDRC_FREQ1_INIT6(0), (mr_value[1][4]<<16) | (mr_value[1][5]));
	reg32_write(DDRC_FREQ1_INIT7(0),  mr_value[1][6]);
#ifdef PLLBYPASS_250MBPS
	reg32_write(DDRC_FREQ1_DRAMTMG0(0), 0x0c0e0403);/* t_ras_max=9*7.8us, t_ras_min=35ns */
#endif
#ifdef PLLBYPASS_400MBPS
	reg32_write(DDRC_FREQ1_DRAMTMG0(0), 0x0c0e0604);/* t_ras_max=9*7.8us, t_ras_min=35ns */
#endif
	reg32_write(DDRC_FREQ1_DRAMTMG1(0), 0x00030314);
	reg32_write(DDRC_FREQ1_DRAMTMG2(0), 0x0505040a);
	reg32_write(DDRC_FREQ1_DRAMTMG3(0), 0x0000400c);
	reg32_write(DDRC_FREQ1_DRAMTMG4(0), 0x06040307); /*  tRP=6 --> 7 */
	reg32_write(DDRC_FREQ1_DRAMTMG5(0), 0x090d0202);
	reg32_write(DDRC_FREQ1_DRAMTMG6(0), 0x0a070008);
	reg32_write(DDRC_FREQ1_DRAMTMG7(0), 0x00000d09);
	reg32_write(DDRC_FREQ1_DRAMTMG8(0), 0x08084b09);
	reg32_write(DDRC_FREQ1_DRAMTMG9(0), 0x00020308);
	reg32_write(DDRC_FREQ1_DRAMTMG10(0), 0x000f0d06);
	reg32_write(DDRC_FREQ1_DRAMTMG11(0), 0x12060111);
	reg32_write(DDRC_FREQ1_DRAMTMG12(0), 0x00000008);
	reg32_write(DDRC_FREQ1_DRAMTMG13(0), 0x21000000);
	reg32_write(DDRC_FREQ1_DRAMTMG14(0), 0x00000000);
	reg32_write(DDRC_FREQ1_DRAMTMG15(0), 0x00000000);
	reg32_write(DDRC_FREQ1_DRAMTMG17(0), 0x00c6007d);
	reg32_write(DDRC_FREQ1_ZQCTL0(0), 0x51000040);
	reg32_write(DDRC_FREQ1_DFITMG0(0), 0x03858204);
	reg32_write(DDRC_FREQ1_DFITMG1(0), 0x00020103);
	reg32_write(DDRC_FREQ1_DFITMG2(0), 0x00000504);
	reg32_write(DDRC_FREQ1_DFITMG3(0), 0x00000001);
	/* FREQ1: BL8, CL=10, CWL=9, WR_PREAMBLE = 1,RD_PREAMBLE = 1, CRC_MODE = 1 */
	/* wr_odt_delay=DFITMG1.dfi_t_cmd_lat=0 */
	reg32_write(DDRC_FREQ1_ODTCFG(0), 0x07000601);
}

void umctl2_freq2_cfg(void)
{
	reg32_write(DDRC_FREQ2_RFSHCTL0(0), 0x0021a0c0);
	reg32_write(DDRC_FREQ2_RFSHTMG(0), 0x0006000e);/* tREFI=7.8us */
	reg32_write(DDRC_FREQ2_INIT3(0), (mr_value[2][0]<<16) | (mr_value[2][1]));
	reg32_write(DDRC_FREQ2_INIT4(0), (mr_value[2][2]<<16) | (mr_value[2][3]));
	reg32_write(DDRC_FREQ2_INIT6(0), (mr_value[2][4]<<16) | (mr_value[2][5]));
	reg32_write(DDRC_FREQ2_INIT7(0),  mr_value[2][6]);
	reg32_write(DDRC_FREQ2_DRAMTMG0(0), 0x0c0e0101);/* t_ras_max=9*7.8us, t_ras_min=35ns */
	reg32_write(DDRC_FREQ2_DRAMTMG1(0), 0x00030314);
	reg32_write(DDRC_FREQ2_DRAMTMG2(0), 0x0505040a);
	reg32_write(DDRC_FREQ2_DRAMTMG3(0), 0x0000400c);
	reg32_write(DDRC_FREQ2_DRAMTMG4(0), 0x06040307); /*  tRP=6 --> 7 */
	reg32_write(DDRC_FREQ2_DRAMTMG5(0), 0x090d0202);
	reg32_write(DDRC_FREQ2_DRAMTMG6(0), 0x0a070008);
	reg32_write(DDRC_FREQ2_DRAMTMG7(0), 0x00000d09);
	reg32_write(DDRC_FREQ2_DRAMTMG8(0), 0x08084b09);
	reg32_write(DDRC_FREQ2_DRAMTMG9(0), 0x00020308);
	reg32_write(DDRC_FREQ2_DRAMTMG10(0), 0x000f0d06);
	reg32_write(DDRC_FREQ2_DRAMTMG11(0), 0x12060111);
	reg32_write(DDRC_FREQ2_DRAMTMG12(0), 0x00000008);
	reg32_write(DDRC_FREQ2_DRAMTMG13(0), 0x21000000);
	reg32_write(DDRC_FREQ2_DRAMTMG14(0), 0x00000000);
	reg32_write(DDRC_FREQ2_DRAMTMG15(0), 0x00000000);
	reg32_write(DDRC_FREQ2_DRAMTMG17(0), 0x00c6007d);
	reg32_write(DDRC_FREQ2_ZQCTL0(0), 0x51000040);
	reg32_write(DDRC_FREQ2_DFITMG0(0), 0x03858204);
	reg32_write(DDRC_FREQ2_DFITMG1(0), 0x00020103);
	reg32_write(DDRC_FREQ2_DFITMG2(0), 0x00000504);
	reg32_write(DDRC_FREQ2_DFITMG3(0), 0x00000001);
	/* FREQ1: BL8, CL=10, CWL=9, WR_PREAMBLE = 1,RD_PREAMBLE = 1, CRC_MODE = 1 */
	/* wr_odt_delay=DFITMG1.dfi_t_cmd_lat=0 */
	reg32_write(DDRC_FREQ2_ODTCFG(0), 0x07000601);
}


void ddr4_pub_train(void)
{
	volatile unsigned int tmp_t;
	after_retention = 0;

	reg32_write(SRC_DDRC_RCR_ADDR, 0x8F00003F); /*  assert [0]ddr1_preset_n, [1]ddr1_core_reset_n, [2]ddr1_phy_reset, [3]ddr1_phy_pwrokin_n, [4]src_system_rst_b! */
	reg32_write(SRC_DDRC_RCR_ADDR, 0x8F00000F); /*  deassert [4]src_system_rst_b! */

	/* change the clock source of dram_apb_clk_root */
	clock_set_target_val(DRAM_APB_CLK_ROOT, CLK_ROOT_ON | CLK_ROOT_SOURCE_SEL(4) | CLK_ROOT_PRE_DIV(CLK_ROOT_PRE_DIV4)); /* to source 4 --800MHz/4 */

	/* DDR_PLL_CONFIG_600MHz(); */
	dram_pll_init(DRAM_PLL_OUT_600M);
	ddr_dbg("C: dram pll init finished\n");

	reg32_write(0x303A00EC, 0x0000ffff); /* PGC_CPU_MAPPING */
	reg32setbit(0x303A00F8, 5);/* PU_PGC_SW_PUP_REQ */

	reg32_write(SRC_DDRC_RCR_ADDR, 0x8F000006); /*  release [0]ddr1_preset_n, [3]ddr1_phy_pwrokin_n */

	reg32_write(DDRC_DBG1(0), 0x00000001);
	reg32_write(DDRC_PWRCTL(0), 0x00000001);

	while (0 != (0x7 & reg32_read(DDRC_STAT(0))))
		;

	ddr_dbg("C: cfg umctl2 regs ...\n");
	umctl2_cfg();
#ifdef DDR4_SW_FFC
	umctl2_freq1_cfg();
	umctl2_freq2_cfg();
#endif

	reg32_write(DDRC_RFSHCTL3(0), 0x00000011);
	/* RESET: <ctn> DEASSERTED */
	/* RESET: <a Port 0  DEASSERTED(0) */
	reg32_write(SRC_DDRC_RCR_ADDR, 0x8F000004);
	reg32_write(SRC_DDRC_RCR_ADDR, 0x8F000000);

	reg32_write(DDRC_DBG1(0), 0x00000000);
	reg32_write(DDRC_PWRCTL(0), 0x00000aa);
	reg32_write(DDRC_SWCTL(0), 0x00000000);

	reg32_write(DDRC_DFIMISC(0), 0x00000000);

	ddr_dbg("C: phy training ...\n");
	ddr4_phyinit_train_sw_ffc(1);/*  for dvfs flow, 2D training is a must item */

	do {
		tmp_t = reg32_read(IP2APB_DDRPHY_IPS_BASE_ADDR(0)+4*0x00020097);
		ddr_dbg("C: Waiting CalBusy value = 0\n");
	} while (tmp_t  != 0);

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
	while (0x1 != (0x7 & reg32_read(DDRC_STAT(0))))
		;

	reg32_write(DDRC_PWRCTL(0), 0x0000088);
	reg32_write(DDRC_PCTRL_0(0), 0x00000001);
	reg32_write(DDRC_RFSHCTL3(0), 0x00000010); /*  dis_auto-refresh is set to 0 */
}

void ddr4_switch_freq(unsigned int pstate)
{
	if ((pstate != 0 && cur_pstate == 0) || (pstate == 0 && cur_pstate != 0)) {
		ddr4_dll_change(pstate);
	} else {
		ddr4_dll_no_change(pstate);
		ddr_dbg("dll no change\n");
	}
	cur_pstate = pstate;
}

void dram_all_mr_cfg(unsigned int pstate)
{
	unsigned int i;
	/* 15. Perform MRS commands as required to re-program timing registers in the SDRAM for the new */
	/* frequency (in particular, CL, CWL and WR may need to be changed). */
	for (i = 0; i < 7; i++)
		ddr4_mr_write(i, mr_value[pstate][i], 0, 0x1);

#ifndef DDR_ONE_RANK
	for (i = 0; i < 7; i++)
		ddr4_mr_write(i, mr_value[pstate][i], 0, 0x2);
#endif
}

void sw_pstate(unsigned int pstate)
{
	volatile unsigned int tmp;
	unsigned int i;
	/* the the following software programming sequence to switch from DLL-on to DLL-off, or reverse: */
	reg32_write(DDRC_SWCTL(0), 0x0000);
	/* 12. Change the clock frequency to the desired value. */
	/* 13. Update any registers which may be required to change for the new frequency. This includes quasidynamic and dynamic registers. This includes both uMCTL2 registers and PHY registers. */
	reg32_write(DDRC_DFIMISC(0), 0x00000000);
	reg32_write(DDRC_MSTR2(0), pstate);/*  UMCTL2_REGS_FREQ1 */
	reg32setbit(DDRC_MSTR(0), 29);

	/* dvfs.18. Toggle RFSHCTL3.refresh_update_level to allow the new refresh-related register values to */
	/* propagate to the refresh logic. */
	tmp = reg32_read(DDRC_RFSHCTL3(0));
	if ((tmp & 0x2) == 0x2)
		reg32_write(DDRC_RFSHCTL3(0), tmp & 0xFFFFFFFD);
	else
		reg32_write(DDRC_RFSHCTL3(0), tmp | 0x2);

	/* dvfs.19. If required, trigger the initialization in the PHY. If using the gen2 multiPHY, PLL initialization */
	/* should be triggered at this point. See the PHY databook for details about the frequency change */
	/* procedure. */
	reg32_write(DDRC_DFIMISC(0), 0x00000000 | (pstate<<8));/* pstate1 */
	reg32_write(DDRC_DFIMISC(0), 0x00000020 | (pstate<<8));

	/*  wait DFISTAT.dfi_init_complete to 0 */
	do {
		tmp = 0x1 & reg32_read(DDRC_DFISTAT(0));
	} while (tmp);

	dwc_ddrphy_phyinit_userCustom_E_setDfiClk(pstate);

	reg32_write(DDRC_DFIMISC(0), 0x00000000 | (pstate<<8));
	/*  wait DFISTAT.dfi_init_complete to 1 */
	do {
		tmp = 0x1 & reg32_read(DDRC_DFISTAT(0));
	} while (!tmp);

	/* When changing frequencies the controller may violate the JEDEC requirement that no */
	/* more than 16 refreshes should be issued within 2*tREFI. These extra refreshes are not */
	/* expected to cause a problem in the SDRAM. This issue can be avoided by waiting for at */
	/* least 2*tREFI before exiting self-refresh in step 19. */
	for (i = 20; i > 0; i--)
		;
	ddr_dbg("C: waiting for 2*tREFI (2*7.8us)\n");

	/* 14. Exit the self-refresh state by setting PWRCTL.selfref_sw = 0. */
	reg32clrbit(DDRC_PWRCTL(0), 5);
	do {
		tmp  = 0x3f & (reg32_read((DDRC_STAT(0))));
		ddr_dbg("C: waiting for exit Self Refresh\n");
	} while (tmp == 0x23);
}

void ddr4_dll_change(unsigned int pstate)
{
	volatile unsigned int tmp;
	enum DLL_STATE { NO_CHANGE = 0, ON2OFF = 1, OFF2ON = 2} dll_sw; /* 0-no change, 1-on2off, 2-off2on.; */

	if (pstate != 0 && cur_pstate == 0) {
		dll_sw = ON2OFF;
		ddr_dbg("dll ON2OFF\n");
	} else if (pstate == 0 && cur_pstate != 0) {
		dll_sw = OFF2ON;
		ddr_dbg("dll OFF2ON\n");
	} else {
		dll_sw = NO_CHANGE;
	}

	/* the the following software programming sequence to switch from DLL-on to DLL-off, or reverse: */
	reg32_write(DDRC_SWCTL(0), 0x0000);

	/* 1. Set the DBG1.dis_hif = 1. This prevents further reads/writes being received on the HIF. */
	reg32setbit(DDRC_DBG1(0), 1);
	/* 2. Set ZQCTL0.dis_auto_zq=1, to disable automatic generation of ZQCS/MPC(ZQ calibration) */
	/* commands */
	if (pstate == 1)
		reg32setbit(DDRC_FREQ1_ZQCTL0(0), 31);
	else if (pstate == 2)
		reg32setbit(DDRC_FREQ2_ZQCTL0(0), 31);
	else
		reg32setbit(DDRC_ZQCTL0(0), 31);

	/* 3. Set RFSHCTL3.dis_auto_refresh=1, to disable automatic refreshes */
	reg32setbit(DDRC_RFSHCTL3(0), 0);
	/* 4. Ensure all commands have been flushed from the uMCTL2 by polling */
	/* DBGCAM.wr_data_pipeline_empty, DBGCAM.rd_data_pipeline_empty1, */
	/* DBGCAM.dbg_wr_q_depth, DBGCAM.dbg_lpr_q_depth, DBGCAM.dbg_rd_q_empty, */
	/* DBGCAM.dbg_wr_q_empty. */
	do {
		tmp = 0x06000000 & reg32_read(DDRC_DBGCAM(0));
	} while (tmp  != 0x06000000);
	reg32_write(DDRC_PCTRL_0(0), 0x00000000);
	/* 5. Perform an MRS command (using MRCTRL0 and MRCTRL1 registers) to disable RTT_NOM: */
	/* a. DDR3: Write 0 to MR1[9], MR1[6] and MR1[2] */
	/* b. DDR4: Write 0 to MR1[10:8] */
	if (mr_value[pstate][1] & 0x700) {
		ddr4_mr_write(1, mr_value[pstate][1] & 0xF8FF, 0, 0x1);
#ifndef DDR_ONE_RANK
		ddr4_mr_write(1, mr_value[pstate][1] & 0xF8FF, 0, 0x2);
#endif
	}
	/* 6. For DDR4 only: Perform an MRS command (using MRCTRL0 and MRCTRL1 registers) to write 0 to */
	/* MR5[8:6] to disable RTT_PARK */
	if (mr_value[pstate][5] & 0x1C0) {
		ddr4_mr_write(5, mr_value[pstate][5] & 0xFE3F, 0, 0x1);
#ifndef DDR_ONE_RANK
		ddr4_mr_write(5, mr_value[pstate][5] & 0xFE3F, 0, 0x2);
#endif
	}

	if (dll_sw == ON2OFF) {
		/* 7. Perform an MRS command (using MRCTRL0 and MRCTRL1 registers) to write 0 to MR2[11:9], to */
		/* disable RTT_WR (and therefore disable dynamic ODT). This applies for both DDR3 and DDR4. */
		if (mr_value[pstate][2] & 0xE00) {
		    ddr4_mr_write(2, mr_value[pstate][2] & 0xF1FF, 0, 0x1);
#ifndef DDR_ONE_RANK
		    ddr4_mr_write(2, mr_value[pstate][2] & 0xF1FF, 0, 0x2);
#endif
		}
		/* 8. Perform an MRS command (using MRCTRL0 and MRCTRL1 registers) to disable the DLL. The */
		/* timing of this MRS is automatically handled by the uMCTL2. */
		/* a. DDR3: Write 1 to MR1[0] */
		/* b. DDR4: Write 0 to MR1[0] */
		ddr4_mr_write(1, mr_value[pstate][1] & 0xFFFE, 0, 0x1);
#ifndef DDR_ONE_RANK
		ddr4_mr_write(1, mr_value[pstate][1] & 0xFFFE, 0, 0x2);
#endif
	}

	/* 9. Put the SDRAM into self-refresh mode by setting PWRCTL.selfref_sw = 1, and polling */
	/* STAT.operating_mode to ensure the DDRC has entered self-refresh. */
	reg32setbit(DDRC_PWRCTL(0), 5);
	/* 10. Wait until STAT.operating_mode[1:0]==11 indicating that the DWC_ddr_umctl2 core is in selfrefresh mode. Ensure transition to self-refresh was due to software by checking that */
	/* STAT.selfref_type[1:0]=2`b10. */
	do {
		tmp  = 0x3f & (reg32_read((DDRC_STAT(0))));
		ddr_dbg("C: wait DRAM in Self Refresh\n");
	} while (tmp  != 0x23);

	/* 11. Set the MSTR.dll_off_mode = 1 or 0. */
	if (dll_sw == ON2OFF)
		reg32setbit(DDRC_MSTR(0), 15);

	if (dll_sw == OFF2ON)
		reg32clrbit(DDRC_MSTR(0), 15);

	sw_pstate(pstate);

	/* DRAM dll enable */
	if (dll_sw == OFF2ON) {
		ddr4_mr_write(1, mr_value[pstate][1] | 0x1, 0, 0x1);
#ifndef DDR_ONE_RANK
		ddr4_mr_write(1, mr_value[pstate][1] | 0x1, 0, 0x2);
#endif
		/* DRAM dll reset, self-clear */
		ddr4_mr_write(0, mr_value[pstate][0] | 0x100, 0, 0x1);
#ifndef DDR_ONE_RANK
		ddr4_mr_write(0, mr_value[pstate][0] | 0x100, 0, 0x2);
#endif
	}

	dram_all_mr_cfg(pstate);

	/* 16. Re-enable automatic generation of ZQCS/MPC(ZQ calibration) commands, by setting */
	/* ZQCTL0.dis_auto_zq=0 if they were previously disabled */
	if (pstate == 1)
		reg32clrbit(DDRC_FREQ1_ZQCTL0(0), 31);
	else if (pstate == 2)
		reg32clrbit(DDRC_FREQ2_ZQCTL0(0), 31);
	else
		reg32clrbit(DDRC_ZQCTL0(0), 31);

	/* 17. Re-enable automatic refreshes (RFSHCTL3.dis_auto_refresh = 0) if they have been previously */
	/* disabled. */
	reg32clrbit(DDRC_RFSHCTL3(0), 0);
	/* 18. Restore ZQCTL0.dis_srx_zqcl */
	/* 19. Write DBG1.dis_hif = 0 to re-enable reads and writes. */
	reg32clrbit(DDRC_DBG1(0), 1);

	reg32_write(DDRC_PCTRL_0(0), 0x00000001);
	/* 27. Write 1 to SBRCTL.scrub_en. Enable SBR if desired, only required if SBR instantiated. */

	/*  set SWCTL.sw_done to enable quasi-dynamic register programming outside reset. */
	reg32_write(DDRC_SWCTL(0), 0x0001);

	/* wait SWSTAT.sw_done_ack to 1 */
	do {
		tmp = 0x1 & reg32_read(DDRC_SWSTAT(0));
	} while (!tmp);
}

void ddr4_dll_no_change(unsigned int pstate)
{
	volatile unsigned int tmp;
	/* ------------------------------------------------------------------------------------- */
	/*   change to pstate1 */
	/* ------------------------------------------------------------------------------------- */
	/* 1. Program one of UMCTL2_REGS_FREQ1/2/3, whichever you prefer, timing register-set with the */
	/* timing settings required for the alternative clock frequency. */
	/*  set SWCTL.sw_done to disable quasi-dynamic register programming outside reset. */
	reg32_write(DDRC_SWCTL(0), 0x0000);

	/*    set SWCTL.sw_done to enable quasi-dynamic register programming outside reset. */
	/*    wait SWSTAT.sw_done_ack to 1 */

	/* 2. Write 0 to PCTRL_n.port_en. This blocks AXI port(s) from taking any transaction (blocks traffic on */
	/* AXI ports). */
	reg32_write(DDRC_PCTRL_0(0), 0x00000000);
	/* 3. Poll PSTAT.rd_port_busy_n=0 and PSTAT.wr_port_busy_n=0. Wait until all AXI ports are idle (the */
	/* uMCTL2 core has to be idle). */
	do {
		tmp = reg32_read(DDRC_PSTAT(0));
	} while (tmp & 0x10001);

	/* 4. Write 0 to SBRCTL.scrub_en. Disable SBR, required only if SBR instantiated. */
	/* 5. Poll SBRSTAT.scrub_busy=0. Indicates that there are no outstanding SBR read commands (required */
	/* only if SBR instantiated). */
	/* 6. Set DERATEEN.derate_enable = 0, if DERATEEN.derate_eanble = 1 and the read latency (RL) value */
	/* needs to change after the frequency change (LPDDR2/3/4 only). */
	/* 7. Set DBG1.dis_hif=1 so that no new commands will be accepted by the uMCTL2. */
	reg32setbit(DDRC_DBG1(0), 1);
	/* 8. Poll DBGCAM.dbg_wr_q_empty and DBGCAM.dbg_rd_q_empty to ensure that write and read data */
	/* buffers are empty. */
	do {
		tmp = 0x06000000 & reg32_read(DDRC_DBGCAM(0));
	} while (tmp != 0x06000000);
	/* 9. For DDR4, update MR6 with the new tDLLK value via the Mode Register Write signals */
	/* (MRCTRL0.mr_x/MRCTRL1.mr_x). */
	/* 10. Set DFILPCFG0.dfi_lp_en_sr = 0, if DFILPCFG0.dfi_lp_en_sr = 1, and wait until DFISTAT.dfi_lp_ack */
	/* = 0. */
	/* 11. If DFI PHY Master interface is active in uMCTL2 (DFIPHYMSTR.phymstr_en == 1'b1) then disable it */
	/* by programming DFIPHYMSTR.phymstr_en = 1'b0. */
	/* 12. Wait until STAT.operating_mode[1:0]!=11 indicating that the DWC_ddr_umctl2 controller is not in */
	/* self-refresh mode. */
	tmp  = 0x3 & (reg32_read((DDRC_STAT(0))));
	if (tmp == 0x3) {
		ddr_dbg("C: Error DRAM should not in Self Refresh\n");
		ddr_dbg("vt_error\n");
	}
	/* 13. Assert PWRCTL.selfref_sw for the DWC_ddr_umctl2 core to enter the self-refresh mode. */
	reg32setbit(DDRC_PWRCTL(0), 5);
	/* 14. Wait until STAT.operating_mode[1:0]==11 indicating that the DWC_ddr_umctl2 core is in selfrefresh mode. Ensure transition to self-refresh was due to software by checking that STAT.selfref_type[1:0]=2'b10. */
	do {
		tmp  = 0x3f & (reg32_read((DDRC_STAT(0))));
		ddr_dbg("C: DRAM in Self Refresh\n");
	} while (tmp != 0x23);

	sw_pstate(pstate);
	dram_all_mr_cfg(pstate);


	/* 23. Enable HIF commands by setting DBG1.dis_hif=0. */
	reg32clrbit(DDRC_DBG1(0), 1);
	/* 24. Reset DERATEEN.derate_enable = 1 if DERATEEN.derate_enable has been set to 0 in step 6. */
	/* 25. If DFI PHY Master interface was active in uMCTL2 (DFIPHYMSTR.phymstr_en == 1'b1) before the */
	/* step 11 then enable it back by programming DFIPHYMSTR.phymstr_en = 1'b1. */
	/* 26. Write 1 to PCTRL_n.port_en. AXI port(s) are no longer blocked from taking transactions (Re-enable */
	/* traffic on AXI ports). */
	reg32_write(DDRC_PCTRL_0(0), 0x00000001);
	/* 27. Write 1 to SBRCTL.scrub_en. Enable SBR if desired, only required if SBR instantiated. */



	/*  set SWCTL.sw_done to enable quasi-dynamic register programming outside reset. */
	reg32_write(DDRC_SWCTL(0), 0x0001);

	/* wait SWSTAT.sw_done_ack to 1 */
	do {
		tmp  = 0x1 & reg32_read(DDRC_SWSTAT(0));
	} while (!tmp);


}

void ddr_init(void)
{
    /* initialize DDR4-2400 (umctl2@800MHz) */
    ddr4_pub_train();
}
