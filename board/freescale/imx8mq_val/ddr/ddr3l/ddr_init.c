/*
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <errno.h>
#include <asm/io.h>
#include <asm/arch/ddr.h>
#include <asm/arch/clock.h>
#include "../ddr.h"

#ifdef CONFIG_ENABLE_DDR_TRAINING_DEBUG
#define ddr_printf(args...) printf(args)
#else
#define ddr_printf(args...)
#endif

#include "../wait_ddrphy_training_complete.c"

static inline void reg32clrbit(unsigned long addr, u32 bit)
{
	clrbits_le32(addr, (1 << bit));
}

volatile unsigned int tmp;
void umctl2_cfg(void){
	reg32_write(DDRC_DBG1(0), 0x00000001);
	reg32_write(DDRC_PWRCTL(0), 0x00000001);
	do{
	  tmp = 0x7 & (reg32_read(DDRC_STAT(0)));
	} while (tmp);/* wait init state */

	reg32_write(DDRC_MSTR(0), 0x83040001);/* two rank */

	reg32_write(DDRC_MRCTRL0(0), 0x40004030);
	reg32_write(DDRC_MRCTRL1(0), 0x0001c68e);
	reg32_write(DDRC_MRCTRL2(0), 0x921b7e95);
	reg32_write(DDRC_DERATEEN(0), 0x00000506);
	reg32_write(DDRC_DERATEINT(0), 0x9a4fbdf1);
	reg32_write(DDRC_MSTR2(0), 0x00000001);
	reg32_write(DDRC_PWRCTL(0), 0x000000a8);
	reg32_write(DDRC_PWRTMG(0), 0x00532203);
	reg32_write(DDRC_HWLPCTL(0), 0x0b6d0000);
	reg32_write(DDRC_HWFFCCTL(0), 0x00000030);
	reg32_write(DDRC_RFSHCTL0(0), 0x00203020);
	reg32_write(DDRC_RFSHCTL1(0), 0x0001000d);
	reg32_write(DDRC_RFSHCTL3(0), 0x00000000);
	reg32_write(DDRC_RFSHTMG(0), 0x0061008c);
	reg32_write(DDRC_CRCPARCTL0(0), 0x00000000);
	reg32_write(DDRC_CRCPARCTL1(0), 0x00000000);
	reg32_write(DDRC_INIT0(0), 0xc0030002);
	reg32_write(DDRC_INIT1(0), 0x0001000b);
	reg32_write(DDRC_INIT2(0), 0x00006303);
	reg32_write(DDRC_INIT3(0), 0x0d700044);/* MR1, MR0 */
	reg32_write(DDRC_INIT4(0), 0x00180000);/* MR2 */
	reg32_write(DDRC_INIT5(0), 0x00090071);
	reg32_write(DDRC_INIT6(0), 0x00000000);
	reg32_write(DDRC_INIT7(0), 0x00000000);
	reg32_write(DDRC_DIMMCTL(0), 0x00000032);
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
	reg32_write(DDRC_DFILPCFG0(0), 0x07713121);
	reg32_write(DDRC_DFILPCFG1(0), 0x00000010);
	reg32_write(DDRC_DFIUPD0(0), 0xe0400018);
	reg32_write(DDRC_DFIUPD1(0), 0x0005003c);
	reg32_write(DDRC_DFIUPD2(0), 0x00000000);
	reg32_write(DDRC_DFIMISC(0), 0x00000011);
	reg32_write(DDRC_DFITMG2(0), 0x00000603);
	reg32_write(DDRC_DFITMG3(0), 0x00000001);
	reg32_write(DDRC_DBICTL(0), 0x00000001);
	reg32_write(DDRC_DFIPHYMSTR(0), 0x00000000);

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

int ddr_init(struct dram_timing_info *timing_info)
{
	/* change the clock source of dram_apb_clk_root  */
	clock_set_target_val(DRAM_APB_CLK_ROOT, CLK_ROOT_ON |
		     CLK_ROOT_SOURCE_SEL(4) |
		     CLK_ROOT_PRE_DIV(CLK_ROOT_PRE_DIV4));

	/* disable the clock gating */
	reg32_write(0x303A00EC,0x0000ffff);
	reg32setbit(0x303A00F8,5);
	reg32_write(SRC_DDRC_RCR_ADDR + 0x04, 0x8F000000);

	dram_pll_init(MHZ(400));

	reg32_write(SRC_DDRC_RCR_ADDR, 0x8F000006);

	/* Configure uMCTL2's registers */
	umctl2_cfg();

	reg32setbit(DDRC_RFSHCTL3(0),0); /* dis_auto_refresh */
	reg32_write(SRC_DDRC_RCR_ADDR, 0x8F000000);

	ddr_load_train_code(FW_1D_IMAGE);

	reg32_write(DDRC_DBG1(0), 0x00000000); /* ('b00000000_00000000_00000000_00000000) ('d0) */
	reg32setbit(DDRC_PWRCTL(0),5); /* selfref_sw=1, self-refresh */
	reg32clrbit(DDRC_SWCTL(0), 0); /* sw_done=0, enable quasi-dynamic programming */
	reg32_write(DDRC_DFIMISC(0), 0x00000000);

	/* Configure DDR3L PHY's registers */
	ddr3_phyinit_train_1600mts();

	do {
		tmp = reg32_read(IP2APB_DDRPHY_IPS_BASE_ADDR(0)+4*0x00020097);
	} while (tmp != 0);

	reg32setbit(DDRC_DFIMISC(0),5);/* dfi_init_start=1 */
	do{
		tmp = 0x1 & (reg32_read(DDRC_DFISTAT(0)));
	} while (!tmp);/*  wait DFISTAT.dfi_init_complete to 1 */

	reg32clrbit(DDRC_DFIMISC(0),5);/* dfi_init_start=0 */
	reg32setbit(DDRC_DFIMISC(0),0);/* dfi_init_complete_en=1 */

	reg32clrbit(DDRC_PWRCTL(0),5);/* selfref_sw=0, exit self-refresh */

	reg32setbit(DDRC_SWCTL(0), 0);/* sw_done=1, disable quasi-dynamic programming */

	/* wait SWSTAT.sw_done_ack to 1 */
	do{
		tmp = 0x1 & (reg32_read(DDRC_SWSTAT(0)));
	} while (!tmp);

	/* wait STAT to normal state */
	do{
		tmp = 0x7 & (reg32_read(DDRC_STAT(0)));
	} while (tmp != 0x1);

	reg32_write(DDRC_PCTRL_0(0), 0x00000001); /* enable port 0 */

	reg32clrbit(DDRC_RFSHCTL3(0), 0); /* auto-refresh enable */

	return 0;
}
