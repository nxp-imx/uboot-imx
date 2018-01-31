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
#include "ddr.h"

#ifdef CONFIG_ENABLE_DDR_TRAINING_DEBUG
#define ddr_printf(args...) printf(args)
#else
#define ddr_printf(args...)
#endif

#include "wait_ddrphy_training_complete.c"
#ifndef SRC_DDRC_RCR_ADDR
#define SRC_DDRC_RCR_ADDR SRC_IPS_BASE_ADDR +0x1000
#endif
#ifndef DDR_CSD1_BASE_ADDR
#define DDR_CSD1_BASE_ADDR 0x40000000
#endif
#define SILICON_TRAIN

volatile unsigned int tmp, tmp_t, i;
void lpddr4_800MHz_cfg_umctl2(void)
{
	dwc_ddrphy_apb_wr(DDRC_IPS_BASE_ADDR(0) +  0x00000304, 0x00000001);
	dwc_ddrphy_apb_wr(DDRC_IPS_BASE_ADDR(0) +  0x00000030, 0x00000001);
	dwc_ddrphy_apb_wr(DDRC_IPS_BASE_ADDR(0) +  0x00000000, 0x83080020);
	dwc_ddrphy_apb_wr(DDRC_IPS_BASE_ADDR(0) +  0x00000064, 0x006180e0);
	dwc_ddrphy_apb_wr(DDRC_IPS_BASE_ADDR(0) +  0x000000d0, 0xc003061B);
	dwc_ddrphy_apb_wr(DDRC_IPS_BASE_ADDR(0) +  0x000000d4, 0x009D0000);
	dwc_ddrphy_apb_wr(DDRC_IPS_BASE_ADDR(0) +  0x000000d8, 0x0000fe05);
	dwc_ddrphy_apb_wr(DDRC_IPS_BASE_ADDR(0) +  0x000000dc, 0x00d4002d);
	dwc_ddrphy_apb_wr(DDRC_IPS_BASE_ADDR(0) +  0x000000e0, 0x00310008);
	dwc_ddrphy_apb_wr(DDRC_IPS_BASE_ADDR(0) +  0x000000e4, 0x00040009);
	dwc_ddrphy_apb_wr(DDRC_IPS_BASE_ADDR(0) +  0x000000e8, 0x0046004d);
	dwc_ddrphy_apb_wr(DDRC_IPS_BASE_ADDR(0) +  0x000000ec, 0x0005004d);
	dwc_ddrphy_apb_wr(DDRC_IPS_BASE_ADDR(0) +  0x000000f4, 0x00000979);
	dwc_ddrphy_apb_wr(DDRC_IPS_BASE_ADDR(0) +  0x00000100, 0x1a203522);
	dwc_ddrphy_apb_wr(DDRC_IPS_BASE_ADDR(0) +  0x00000104, 0x00060630);
	dwc_ddrphy_apb_wr(DDRC_IPS_BASE_ADDR(0) +  0x00000108, 0x070e1214);
	dwc_ddrphy_apb_wr(DDRC_IPS_BASE_ADDR(0) +  0x0000010c, 0x00b0c006);
	dwc_ddrphy_apb_wr(DDRC_IPS_BASE_ADDR(0) +  0x00000110, 0x0f04080f);
	dwc_ddrphy_apb_wr(DDRC_IPS_BASE_ADDR(0) +  0x00000114, 0x0d0d0c0c);
	dwc_ddrphy_apb_wr(DDRC_IPS_BASE_ADDR(0) +  0x00000118, 0x01010007);
	dwc_ddrphy_apb_wr(DDRC_IPS_BASE_ADDR(0) +  0x0000011c, 0x0000060a);
	dwc_ddrphy_apb_wr(DDRC_IPS_BASE_ADDR(0) +  0x00000120, 0x01010101);
	dwc_ddrphy_apb_wr(DDRC_IPS_BASE_ADDR(0) +  0x00000124, 0x40000008);
	dwc_ddrphy_apb_wr(DDRC_IPS_BASE_ADDR(0) +  0x00000128, 0x00050d01);
	dwc_ddrphy_apb_wr(DDRC_IPS_BASE_ADDR(0) +  0x0000012c, 0x01010008);
	dwc_ddrphy_apb_wr(DDRC_IPS_BASE_ADDR(0) +  0x00000130, 0x00020000);
	dwc_ddrphy_apb_wr(DDRC_IPS_BASE_ADDR(0) +  0x00000134, 0x18100002);
	dwc_ddrphy_apb_wr(DDRC_IPS_BASE_ADDR(0) +  0x00000138, 0x00000dc2);
	dwc_ddrphy_apb_wr(DDRC_IPS_BASE_ADDR(0) +  0x0000013c, 0x80000000);
	dwc_ddrphy_apb_wr(DDRC_IPS_BASE_ADDR(0) +  0x00000144, 0x00a00050);
	dwc_ddrphy_apb_wr(DDRC_IPS_BASE_ADDR(0) +  0x00000180, 0x53200018);
	dwc_ddrphy_apb_wr(DDRC_IPS_BASE_ADDR(0) +  0x00000184, 0x02800070);
	dwc_ddrphy_apb_wr(DDRC_IPS_BASE_ADDR(0) +  0x00000188, 0x00000000);
	dwc_ddrphy_apb_wr(DDRC_IPS_BASE_ADDR(0) +  0x00000190, 0x0397820a);
	dwc_ddrphy_apb_wr(DDRC_IPS_BASE_ADDR(0) +  0x00002190, 0x0397820a);
	dwc_ddrphy_apb_wr(DDRC_IPS_BASE_ADDR(0) +  0x00003190, 0x0397820a);
	dwc_ddrphy_apb_wr(DDRC_IPS_BASE_ADDR(0) +  0x00000194, 0x00020103);
	dwc_ddrphy_apb_wr(DDRC_IPS_BASE_ADDR(0) +  0x000001a0, 0xe0400018);
	dwc_ddrphy_apb_wr(DDRC_IPS_BASE_ADDR(0) +  0x000001a4, 0x00df00e4);
	dwc_ddrphy_apb_wr(DDRC_IPS_BASE_ADDR(0) +  0x000001a8, 0x00000000);
	dwc_ddrphy_apb_wr(DDRC_IPS_BASE_ADDR(0) +  0x000001b0, 0x00000011);
	dwc_ddrphy_apb_wr(DDRC_IPS_BASE_ADDR(0) +  0x000001b4, 0x0000170a);
	dwc_ddrphy_apb_wr(DDRC_IPS_BASE_ADDR(0) +  0x000001c0, 0x00000001);
	dwc_ddrphy_apb_wr(DDRC_IPS_BASE_ADDR(0) +  0x000001c4, 0x00000000);
	/* Address map is from MSB 29: r15, r14, cs, r13-r0, b2-b0, c9-c0 */
	dwc_ddrphy_apb_wr(DDRC_ADDRMAP0(0), 0x00000015);
	dwc_ddrphy_apb_wr(DDRC_ADDRMAP4(0), 0x00001F1F);
	/* bank interleave */
	dwc_ddrphy_apb_wr(DDRC_ADDRMAP1(0), 0x00080808);
	dwc_ddrphy_apb_wr(DDRC_ADDRMAP5(0), 0x07070707);
	dwc_ddrphy_apb_wr(DDRC_ADDRMAP6(0), 0x08080707);
	dwc_ddrphy_apb_wr(DDRC_IPS_BASE_ADDR(0) +  0x00000240, 0x020f0c54);
	dwc_ddrphy_apb_wr(DDRC_IPS_BASE_ADDR(0) +  0x00000244, 0x00000000);
	dwc_ddrphy_apb_wr(DDRC_IPS_BASE_ADDR(0) +  0x00000490, 0x00000001);

	/* performance setting */
	dwc_ddrphy_apb_wr(DDRC_ODTCFG(0), 0x0b060908);
	dwc_ddrphy_apb_wr(DDRC_ODTMAP(0), 0x00000000);
	dwc_ddrphy_apb_wr(DDRC_SCHED(0), 0x29511505);
	dwc_ddrphy_apb_wr(DDRC_SCHED1(0), 0x0000002c);
	dwc_ddrphy_apb_wr(DDRC_PERFHPR1(0), 0x5900575b);
	dwc_ddrphy_apb_wr(DDRC_PERFLPR1(0), 0x00000009);
	dwc_ddrphy_apb_wr(DDRC_PERFWR1(0), 0x02005574);
	dwc_ddrphy_apb_wr(DDRC_DBG0(0), 0x00000016);
	dwc_ddrphy_apb_wr(DDRC_DBG1(0), 0x00000000);
	dwc_ddrphy_apb_wr(DDRC_DBGCMD(0), 0x00000000);
	dwc_ddrphy_apb_wr(DDRC_SWCTL(0), 0x00000001);
	dwc_ddrphy_apb_wr(DDRC_POISONCFG(0), 0x00000011);
	dwc_ddrphy_apb_wr(DDRC_PCCFG(0), 0x00000111);
	dwc_ddrphy_apb_wr(DDRC_PCFGR_0(0), 0x000010f3);
	dwc_ddrphy_apb_wr(DDRC_PCFGW_0(0), 0x000072ff);
	dwc_ddrphy_apb_wr(DDRC_PCTRL_0(0), 0x00000001);
	dwc_ddrphy_apb_wr(DDRC_PCFGQOS0_0(0), 0x01110d00);
	dwc_ddrphy_apb_wr(DDRC_PCFGQOS1_0(0), 0x00620790);
	dwc_ddrphy_apb_wr(DDRC_PCFGWQOS0_0(0), 0x00100001);
	dwc_ddrphy_apb_wr(DDRC_PCFGWQOS1_0(0), 0x0000041f);
	dwc_ddrphy_apb_wr(DDRC_FREQ1_DERATEEN(0), 0x00000202);
	dwc_ddrphy_apb_wr(DDRC_FREQ1_DERATEINT(0), 0xec78f4b5);
	dwc_ddrphy_apb_wr(DDRC_FREQ1_RFSHCTL0(0), 0x00618040);
	dwc_ddrphy_apb_wr(DDRC_FREQ1_RFSHTMG(0), 0x00610090);
}

void lpddr4_100MHz_cfg_umctl2(void)
{
	reg32_write(DDRC_FREQ1_DRAMTMG0(0), 0x0d0b010c);
	reg32_write(DDRC_FREQ1_DRAMTMG1(0), 0x00030410);
	reg32_write(DDRC_FREQ1_DRAMTMG2(0), 0x0305090c);
	reg32_write(DDRC_FREQ1_DRAMTMG3(0), 0x00505006);
	reg32_write(DDRC_FREQ1_DRAMTMG4(0), 0x05040305);
	reg32_write(DDRC_FREQ1_DRAMTMG5(0), 0x0d0e0504);
	reg32_write(DDRC_FREQ1_DRAMTMG6(0), 0x0a060004);
	reg32_write(DDRC_FREQ1_DRAMTMG7(0), 0x0000090e);
	reg32_write(DDRC_FREQ1_DRAMTMG14(0), 0x00000032);
	reg32_write(DDRC_FREQ1_DRAMTMG15(0), 0x00000000);
	reg32_write(DDRC_FREQ1_DRAMTMG17(0), 0x0036001b);
	reg32_write(DDRC_FREQ1_DERATEINT(0), 0x7e9fbeb1);
	reg32_write(DDRC_FREQ1_RFSHCTL0(0), 0x0020d040);
	reg32_write(DDRC_FREQ1_DFITMG0(0), 0x03818200);
	reg32_write(DDRC_FREQ1_ODTCFG(0), 0x0a1a096c);
	reg32_write(DDRC_FREQ1_DFITMG2(0), 0x00000000);
	reg32_write(DDRC_FREQ1_RFSHTMG(0), 0x00038014);
	reg32_write(DDRC_FREQ1_INIT3(0), 0x00840000);
	reg32_write(DDRC_FREQ1_INIT6(0), 0x0000004d);
	reg32_write(DDRC_FREQ1_INIT7(0), 0x0000004d);
	reg32_write(DDRC_FREQ1_INIT4(0), 0x00310000);
}

void lpddr4_25MHz_cfg_umctl2(void)
{
	reg32_write(DDRC_FREQ2_DRAMTMG0(0), 0x0d0b010c);
	reg32_write(DDRC_FREQ2_DRAMTMG1(0), 0x00030410);
	reg32_write(DDRC_FREQ2_DRAMTMG2(0), 0x0305090c);
	reg32_write(DDRC_FREQ2_DRAMTMG3(0), 0x00505006);
	reg32_write(DDRC_FREQ2_DRAMTMG4(0), 0x05040305);
	reg32_write(DDRC_FREQ2_DRAMTMG5(0), 0x0d0e0504);
	reg32_write(DDRC_FREQ2_DRAMTMG6(0), 0x0a060004);
	reg32_write(DDRC_FREQ2_DRAMTMG7(0), 0x0000090e);
	reg32_write(DDRC_FREQ2_DRAMTMG14(0), 0x00000032);
	reg32_write(DDRC_FREQ2_DRAMTMG15(0), 0x00000000);
	reg32_write(DDRC_FREQ2_DRAMTMG17(0), 0x0036001b);
	reg32_write(DDRC_FREQ2_DERATEINT(0), 0x7e9fbeb1);
	reg32_write(DDRC_FREQ2_RFSHCTL0(0), 0x0020d040);
	reg32_write(DDRC_FREQ2_DFITMG0(0), 0x03818200);
	reg32_write(DDRC_FREQ2_ODTCFG(0), 0x0a1a096c);
	reg32_write(DDRC_FREQ2_DFITMG2(0), 0x00000000);
	reg32_write(DDRC_FREQ2_RFSHTMG(0), 0x0003800c);
	reg32_write(DDRC_FREQ2_INIT3(0), 0x00840000);
	reg32_write(DDRC_FREQ2_INIT6(0), 0x0000004d);
	reg32_write(DDRC_FREQ2_INIT7(0), 0x0000004d);
	reg32_write(DDRC_FREQ2_INIT4(0), 0x00310000);
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

	dram_pll_init(SSCG_PLL_OUT_800M);

	reg32_write(SRC_DDRC_RCR_ADDR, 0x8F000006);

	/* Configure uMCTL2's registers */
	lpddr4_800MHz_cfg_umctl2();

	reg32_write(SRC_DDRC_RCR_ADDR, 0x8F000004);
	reg32_write(SRC_DDRC_RCR_ADDR, 0x8F000000);

	reg32_write(DDRC_DBG1(0), 0x00000000);
	tmp = reg32_read(DDRC_PWRCTL(0));
	reg32_write(DDRC_PWRCTL(0), 0x000000a8);
	/* reg32_write(DDRC_PWRCTL(0), 0x0000018a); */
	reg32_write(DDRC_SWCTL(0), 0x00000000);
	reg32_write(DDRC_DDR_SS_GPR0, 0x01);
	reg32_write(DDRC_DFIMISC(0), 0x00000010);

	/* Configure LPDDR4 PHY's registers */
	lpddr4_800M_cfg_phy();

	reg32_write(DDRC_RFSHCTL3(0), 0x00000000);
	reg32_write(DDRC_SWCTL(0), 0x0000);
	/*
	 * ------------------- 9 -------------------
	 * Set DFIMISC.dfi_init_start to 1
	 *  -----------------------------------------
	 */
	reg32_write(DDRC_DFIMISC(0), 0x00000030);
	reg32_write(DDRC_SWCTL(0), 0x0001);

	/* wait DFISTAT.dfi_init_complete to 1 */
	tmp_t = 0;
	while(tmp_t==0){
		tmp  = reg32_read(DDRC_DFISTAT(0));
		tmp_t = tmp & 0x01;
		tmp  = reg32_read(DDRC_MRSTAT(0));
	}

	reg32_write(DDRC_SWCTL(0), 0x0000);

	/* clear DFIMISC.dfi_init_complete_en */
	reg32_write(DDRC_DFIMISC(0), 0x00000010);
	reg32_write(DDRC_DFIMISC(0), 0x00000011);
	reg32_write(DDRC_PWRCTL(0), 0x00000088);

	tmp = reg32_read(DDRC_CRCPARSTAT(0));
	/*
	 * set SWCTL.sw_done to enable quasi-dynamic register
	 * programming outside reset.
	 */
	reg32_write(DDRC_SWCTL(0), 0x00000001);

	/* wait SWSTAT.sw_done_ack to 1 */
	while((reg32_read(DDRC_SWSTAT(0)) & 0x1) == 0)
		;

	/* wait STAT.operating_mode([1:0] for ddr3) to normal state */
	while ((reg32_read(DDRC_STAT(0)) & 0x3) != 0x1)
		;

	reg32_write(DDRC_PWRCTL(0), 0x00000088);
	/* reg32_write(DDRC_PWRCTL(0), 0x018a); */
	tmp = reg32_read(DDRC_CRCPARSTAT(0));

	/* enable port 0 */
	reg32_write(DDRC_PCTRL_0(0), 0x00000001);
	tmp = reg32_read(DDRC_CRCPARSTAT(0));
	reg32_write(DDRC_RFSHCTL3(0), 0x00000000);

	reg32_write(DDRC_SWCTL(0), 0x0);
	lpddr4_100MHz_cfg_umctl2();
	lpddr4_25MHz_cfg_umctl2();
	reg32_write(DDRC_SWCTL(0), 0x1);

	/* wait SWSTAT.sw_done_ack to 1 */
	while((reg32_read(DDRC_SWSTAT(0)) & 0x1) == 0)
		;

	reg32_write(DDRC_SWCTL(0), 0x0);

}
