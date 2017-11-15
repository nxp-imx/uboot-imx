/*
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <errno.h>
#include <asm/io.h>
#include <asm/arch/ddr_memory_map.h>
#include <asm/arch/clock.h>
#include "../ddr.h"

#ifdef CONFIG_ENABLE_DDR_TRAINING_DEBUG
#define ddr_printf(args...) printf(args)
#else
#define ddr_printf(args...)
#endif

#include "../wait_ddrphy_training_complete.c"

volatile unsigned int tmp, tmp_t;
void umctl2_cfg(void){
	reg32_write(DDRC_DBG1(0), 0x00000001);
	reg32_write(DDRC_PWRCTL(0), 0x00000001);
	tmp = reg32_read(DDRC_STAT(0));
	while (tmp_t == 0x00000001){
		tmp = reg32_read(DDRC_STAT(0));
		tmp_t = tmp && 0x00000001;
	}

	reg32_write(DDRC_MSTR(0), 0x83040010); /* Two ranks */

	reg32_write(DDRC_MRCTRL0(0), 0x40007030);
	reg32_write(DDRC_MRCTRL1(0), 0x000170df);
	reg32_write(DDRC_MRCTRL2(0), 0x97d37be3);
	reg32_write(DDRC_DERATEEN(0), 0x00000302);
	reg32_write(DDRC_DERATEINT(0), 0xbc808cc7);
	reg32_write(DDRC_MSTR2(0), 0x00000001);
	reg32_write(DDRC_PWRCTL(0), 0x000001ae);
	reg32_write(DDRC_PWRTMG(0), 0x000d2800);
	reg32_write(DDRC_HWLPCTL(0), 0x000c0000);
	reg32_write(DDRC_HWFFCCTL(0), 0x00000010);
	reg32_write(DDRC_RFSHCTL0(0), 0x007090b0);
	reg32_write(DDRC_RFSHCTL1(0), 0x00420019);
	reg32_write(DDRC_RFSHCTL3(0), 0x00000010);
	reg32_write(DDRC_RFSHTMG(0), 0x0049009d);
	reg32_write(DDRC_CRCPARCTL0(0), 0x00000000);
	reg32_write(DDRC_CRCPARCTL1(0), 0x00001011);
	reg32_write(DDRC_INIT0(0), 0xc0030002);
	reg32_write(DDRC_INIT1(0), 0x00030006);
	reg32_write(DDRC_INIT2(0), 0x00000305);
	reg32_write(DDRC_INIT3(0), 0x0a300001);
	reg32_write(DDRC_INIT4(0), 0x10180240);
	reg32_write(DDRC_INIT5(0), 0x0011008a);
	reg32_write(DDRC_INIT6(0), 0x0a000042);
	reg32_write(DDRC_INIT7(0), 0x00000800);
	reg32_write(DDRC_DIMMCTL(0), 0x00000032); /* [1] dimm_addr_mirr_en, it will effect the MRS if use umctl2 to initi dram. */
	reg32_write(DDRC_RANKCTL(0), 0x00000530);
	reg32_write(DDRC_DRAMTMG0(0), 0x14132813);
	reg32_write(DDRC_DRAMTMG1(0), 0x0007051b);
	reg32_write(DDRC_DRAMTMG2(0), 0x090a050f);
	reg32_write(DDRC_DRAMTMG3(0), 0x0000f00f);
	reg32_write(DDRC_DRAMTMG4(0), 0x08030409);
	reg32_write(DDRC_DRAMTMG5(0), 0x0c0d0504);
	reg32_write(DDRC_DRAMTMG6(0), 0x00000003);
	reg32_write(DDRC_DRAMTMG7(0), 0x00000d0c);
	reg32_write(DDRC_DRAMTMG8(0), 0x05051f09);
	reg32_write(DDRC_DRAMTMG9(0), 0x0002040c);
	reg32_write(DDRC_DRAMTMG10(0), 0x000e0d0b);
	reg32_write(DDRC_DRAMTMG11(0), 0x1409011e);
	reg32_write(DDRC_DRAMTMG12(0), 0x0000000d);
	reg32_write(DDRC_DRAMTMG13(0), 0x09000000);
	reg32_write(DDRC_DRAMTMG14(0), 0x00000371);
	reg32_write(DDRC_DRAMTMG15(0), 0x80000000);
	reg32_write(DDRC_DRAMTMG17(0), 0x0076006e);
	reg32_write(DDRC_ZQCTL0(0), 0x51000040);
	reg32_write(DDRC_ZQCTL1(0), 0x00000070);
	reg32_write(DDRC_ZQCTL2(0), 0x00000000);
	reg32_write(DDRC_DFITMG0(0), 0x038f820c);
	reg32_write(DDRC_DFITMG1(0), 0x00020103);
	reg32_write(DDRC_DFILPCFG0(0), 0x07e1b011);
	reg32_write(DDRC_DFILPCFG1(0), 0x00000030);
	reg32_write(DDRC_DFIUPD0(0), 0xe0400018);
	reg32_write(DDRC_DFIUPD1(0), 0x004e00c3);
	reg32_write(DDRC_DFIUPD2(0), 0x00000000);
	reg32_write(DDRC_DFIMISC(0), 0x00000001);
	reg32_write(DDRC_DFITMG2(0), 0x00000f0c);
	reg32_write(DDRC_DFITMG3(0), 0x00000001);
	reg32_write(DDRC_DBICTL(0), 0x00000000);
	reg32_write(DDRC_DFIPHYMSTR(0), 0x00000000);

	reg32_write(DDRC_ADDRMAP0(0), 0x00001F17); /* [4:0]cs0: 6+23 */
	reg32_write(DDRC_ADDRMAP1(0), 0x003F0808); /* [5:0] bank b0: 2+8; [13:8] b1: P3+8 ; [21:16] b2: 4+, unused */
	reg32_write(DDRC_ADDRMAP2(0), 0x00000000); /* [3:0] col-b2: 2;  [11:8] col-b3: 3+0; [19:16] col-b4: 4+0 ; [27:24] col-b5: 5+0 */
	reg32_write(DDRC_ADDRMAP3(0), 0x00000000); /* [3:0] col-b6: 6+0;  [11:8] col-b7: 7+0; [19:16] col-b8: 8+0 ; [27:24] col-b9: 9+0 */
	reg32_write(DDRC_ADDRMAP4(0), 0x00001f1f); /* col-b10, col-b11 not used */
	reg32_write(DDRC_ADDRMAP5(0), 0x07070707); /* [3:0] row-b0: 6+7;  [11:8] row-b1: 7+7; [19:16] row-b2_b10: 8~16+7; [27:24] row-b11: 17+7 */
	reg32_write(DDRC_ADDRMAP6(0), 0x07070707); /* [3:0] row-b12:18+7; [11:8] row-b13: 19+7; [19:16] row-b14:20+7 */
	reg32_write(DDRC_ADDRMAP7(0), 0x00000f0f); /* col-b10, col-b11 not used */
	reg32_write(DDRC_ADDRMAP8(0), 0x00003F0A); /* [5:0] bg-b0: 2+10; [13:8]bg-b1:3+, unused */
	reg32_write(DDRC_ADDRMAP9(0), 0x00000000); /*  it's valid only when ADDRMAP5.addrmap_row_b2_10 is set to value 15 */
	reg32_write(DDRC_ADDRMAP10(0), 0x00000000);/*  it's valid only when ADDRMAP5.addrmap_row_b2_10 is set to value 15 */
	reg32_write(DDRC_ADDRMAP11(0), 0x00000000);


	reg32_write(DDRC_ODTCFG(0), 0x05170558);
	reg32_write(DDRC_ODTMAP(0), 0x00002113);
	reg32_write(DDRC_SCHED(0), 0x0d6f0705);
	reg32_write(DDRC_SCHED1(0), 0x00000000);
	reg32_write(DDRC_PERFHPR1(0), 0xe500558b);
	reg32_write(DDRC_PERFLPR1(0), 0x75001fea);
	reg32_write(DDRC_PERFWR1(0), 0x880026c7);
	reg32_write(DDRC_DBG0(0), 0x00000011);
	reg32_write(DDRC_DBG1(0), 0x00000000);
	reg32_write(DDRC_DBGCMD(0), 0x00000000);
	reg32_write(DDRC_SWCTL(0), 0x00000001);
	reg32_write(DDRC_POISONCFG(0), 0x00100011);
	reg32_write(DDRC_PCCFG(0), 0x00000100);
	reg32_write(DDRC_PCFGR_0(0), 0x00015313);
	reg32_write(DDRC_PCFGW_0(0), 0x000050dc);
	reg32_write(DDRC_PCTRL_0(0), 0x00000001);
	reg32_write(DDRC_PCFGQOS0_0(0), 0x01100200);
	reg32_write(DDRC_PCFGQOS1_0(0), 0x01ba023a);
	reg32_write(DDRC_PCFGWQOS0_0(0), 0x00110000);
	reg32_write(DDRC_PCFGWQOS1_0(0), 0x0000001e);
}

void ddr_init(void)
{
	/* change the clock source of dram_apb_clk_root  */
	reg32_write(CCM_IP_CLK_ROOT_GEN_TAGET_CLR(1),(0x7<<24)|(0x7<<16));
	reg32_write(CCM_IP_CLK_ROOT_GEN_TAGET_SET(1),(0x4<<24)|(0x3<<16));

	/* disable the clock gating */
	reg32_write(0x303A00EC,0x0000ffff);
	reg32setbit(0x303A00F8,5);
	reg32_write(SRC_DDRC_RCR_ADDR + 0x04, 0x8F000000);

	dram_pll_init(SSCG_PLL_OUT_600M);

	reg32_write(SRC_DDRC_RCR_ADDR, 0x8F000006);

	/* Configure uMCTL2's registers */
	umctl2_cfg();

	tmp = reg32_read(DDRC_RFSHCTL3(0));
	reg32_write(DDRC_RFSHCTL3(0), 0x00000011);

	reg32_write(SRC_DDRC_RCR_ADDR, 0x8F000000);

	ddr_load_train_code(FW_1D_IMAGE);

	reg32_write(DDRC_DBG1(0), 0x00000000);
	tmp = reg32_read(DDRC_PWRCTL(0));
	reg32_write(DDRC_PWRCTL(0), 0x000001ae);
	tmp = reg32_read(DDRC_PWRCTL(0));
	reg32_write(DDRC_PWRCTL(0), 0x000001ac);
	reg32_write(DDRC_SWCTL(0), 0x00000000);
	tmp = reg32_read(DDRC_CRCPARSTAT(0));

	reg32_write(DDRC_DFIMISC(0), 0x00000000);
	reg32_write(DDRC_DFIMISC(0), 0x00000000);

	tmp = reg32_read(DDRC_DBICTL(0));
	tmp = reg32_read(DDRC_MSTR(0));
	tmp = reg32_read(DDRC_INIT3(0));
	tmp = reg32_read(DDRC_INIT4(0));
	tmp = reg32_read(DDRC_INIT6(0));
	tmp = reg32_read(DDRC_INIT7(0));
	tmp = reg32_read(DDRC_INIT0(0));

	ddr4_phyinit_train_2400mts();

	do {
		tmp_t = reg32_read(IP2APB_DDRPHY_IPS_BASE_ADDR(0) + 4*0x00020097);
	} while (tmp_t != 0);


	reg32_write(DDRC_DFIMISC(0), 0x00000020);

	/*  wait DFISTAT.dfi_init_complete to 1 */
	tmp_t = 0;
	while(tmp_t == 0){
		tmp  = reg32_read(DDRC_DFISTAT(0));
		tmp_t = tmp & 0x01;
	}

	/*  clear DFIMISC.dfi_init_complete_en */
	reg32_write(DDRC_DFIMISC(0), 0x00000000);
	/*  set DFIMISC.dfi_init_complete_en again */
	reg32_write(DDRC_DFIMISC(0), 0x00000001);
	reg32_write(DDRC_PWRCTL(0), 0x0000018c);

	/*  set SWCTL.sw_done to enable quasi-dynamic register programming outside reset .*/
	reg32_write(DDRC_SWCTL(0), 0x00000001);

	/* wait SWSTAT.sw_done_ack to 1 */
	tmp_t = 0;
	while(tmp_t==0){
		tmp  = reg32_read(DDRC_SWSTAT(0));
		tmp_t = tmp & 0x01;
	}

	/* wait STAT to normal state */
	tmp_t = 0;
	while(tmp_t==0){
		tmp  = reg32_read(DDRC_STAT(0));
		tmp_t = tmp & 0x01;
	}

	tmp = reg32_read(DDRC_CRCPARSTAT(0));

	reg32_write(DDRC_PWRCTL(0), 0x0000018c);

	reg32_write(DDRC_DERATEEN(0), 0x00000302);

	reg32_write(DDRC_PCTRL_0(0), 0x00000001);

	reg32_write(DDRC_RFSHCTL3(0), 0x00000010); /*  dis_auto-refresh is set to 0 */

	reg32_write(IP2APB_DDRPHY_IPS_BASE_ADDR(0) + 4*(0xd0000), 0);
	tmp = reg32_read(IP2APB_DDRPHY_IPS_BASE_ADDR(0) + 4*(0x54030));
	tmp = reg32_read(IP2APB_DDRPHY_IPS_BASE_ADDR(0) + 4*(0x54035));
	reg32_write(IP2APB_DDRPHY_IPS_BASE_ADDR(0) + 4*(0xd0000), 1);
}
