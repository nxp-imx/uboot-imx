/*
 * Copyright 2017-2018 NXP
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
#define DDR_BOOT_P1	/* default DDR boot frequency point */
#define WR_POST_EXT_3200

volatile unsigned int tmp, tmp_t, i;
void lpddr4_800MHz_cfg_umctl2(void)
{
	/* Start to config, default 3200mbps */
	/* dis_dq=1, indicates no reads or writes are issued to SDRAM */
	 reg32_write(DDRC_DBG1(0), 0x00000001);
	/* selfref_en=1, SDRAM enter self-refresh state */
	reg32_write(DDRC_PWRCTL(0), 0x00000001);
	reg32_write(DDRC_MSTR(0), 0xa3080020);
	reg32_write(DDRC_MSTR2(0), 0x00000000);
	reg32_write(DDRC_RFSHTMG(0), 0x006100E0);
	reg32_write(DDRC_INIT0(0), 0xC003061B);
	reg32_write(DDRC_INIT1(0), 0x009D0000);
	reg32_write(DDRC_INIT3(0), 0x00D4002D);
#ifdef WR_POST_EXT_3200  // recommened to define
	reg32_write(DDRC_INIT4(0), 0x00330008);
#else
	reg32_write(DDRC_INIT4(0), 0x00310008);
#endif
	reg32_write(DDRC_INIT6(0), 0x0066004a);
	reg32_write(DDRC_INIT7(0), 0x0006004a);

	reg32_write(DDRC_DRAMTMG0(0), 0x1A201B22);
	reg32_write(DDRC_DRAMTMG1(0), 0x00060633);
	reg32_write(DDRC_DRAMTMG3(0), 0x00C0C000);
	reg32_write(DDRC_DRAMTMG4(0), 0x0F04080F);
	reg32_write(DDRC_DRAMTMG5(0), 0x02040C0C);
	reg32_write(DDRC_DRAMTMG6(0), 0x01010007);
	reg32_write(DDRC_DRAMTMG7(0), 0x00000401);
	reg32_write(DDRC_DRAMTMG12(0), 0x00020600);
	reg32_write(DDRC_DRAMTMG13(0), 0x0C100002);
	reg32_write(DDRC_DRAMTMG14(0), 0x000000E6);
	reg32_write(DDRC_DRAMTMG17(0), 0x00A00050);

	reg32_write(DDRC_ZQCTL0(0), 0x03200018);
	reg32_write(DDRC_ZQCTL1(0), 0x028061A8);
	reg32_write(DDRC_ZQCTL2(0), 0x00000000);

	reg32_write(DDRC_DFITMG0(0), 0x0497820A);
	reg32_write(DDRC_DFITMG1(0), 0x00080303);
	reg32_write(DDRC_DFIUPD0(0), 0xE0400018);
	reg32_write(DDRC_DFIUPD1(0), 0x00DF00E4);
	reg32_write(DDRC_DFIUPD2(0), 0x80000000);
	reg32_write(DDRC_DFIMISC(0), 0x00000011);
	reg32_write(DDRC_DFITMG2(0), 0x0000170A);

	reg32_write(DDRC_DBICTL(0), 0x00000001);
	reg32_write(DDRC_DFIPHYMSTR(0), 0x00000001);

	/* need be refined by ddrphy trained value */
	reg32_write(DDRC_RANKCTL(0), 0x00000c99);
	reg32_write(DDRC_DRAMTMG2(0), 0x070E171a);

	/* address mapping */
	/* Address map is from MSB 29: r15, r14, cs, r13-r0, b2-b0, c9-c0 */
	reg32_write(DDRC_ADDRMAP0(0), 0x00000015);
	reg32_write(DDRC_ADDRMAP3(0), 0x00000000);
	/* addrmap_col_b10 and addrmap_col_b11 set to de-activated (5-bit width) */
	reg32_write(DDRC_ADDRMAP4(0), 0x00001F1F);
	/* bank interleave */
	/* addrmap_bank_b2, addrmap_bank_b1, addrmap_bank_b0 */
	reg32_write(DDRC_ADDRMAP1(0), 0x00080808);
	/* addrmap_row_b11, addrmap_row_b10_b2, addrmap_row_b1, addrmap_row_b0 */
	reg32_write(DDRC_ADDRMAP5(0), 0x07070707);
	/* addrmap_row_b15, addrmap_row_b14, addrmap_row_b13, addrmap_row_b12 */
	reg32_write(DDRC_ADDRMAP6(0), 0x08080707);

	/* 667mts frequency setting */
	reg32_write(DDRC_FREQ1_DERATEEN(0), 0x0000000);
	reg32_write(DDRC_FREQ1_DERATEINT(0), 0x0800000);
	reg32_write(DDRC_FREQ1_RFSHCTL0(0), 0x0210000);
	reg32_write(DDRC_FREQ1_RFSHTMG(0), 0x014001E);
	reg32_write(DDRC_FREQ1_INIT3(0), 0x0140009);
	reg32_write(DDRC_FREQ1_INIT4(0), 0x00310008);
	reg32_write(DDRC_FREQ1_INIT6(0), 0x0066004a);
	reg32_write(DDRC_FREQ1_INIT7(0), 0x0006004a);
	reg32_write(DDRC_FREQ1_DRAMTMG0(0), 0xB070A07);
	reg32_write(DDRC_FREQ1_DRAMTMG1(0), 0x003040A);
	reg32_write(DDRC_FREQ1_DRAMTMG2(0), 0x305080C);
	reg32_write(DDRC_FREQ1_DRAMTMG3(0), 0x0505000);
	reg32_write(DDRC_FREQ1_DRAMTMG4(0), 0x3040203);
	reg32_write(DDRC_FREQ1_DRAMTMG5(0), 0x2030303);
	reg32_write(DDRC_FREQ1_DRAMTMG6(0), 0x2020004);
	reg32_write(DDRC_FREQ1_DRAMTMG7(0), 0x0000302);
	reg32_write(DDRC_FREQ1_DRAMTMG12(0), 0x0020310);
	reg32_write(DDRC_FREQ1_DRAMTMG13(0), 0xA100002);
	reg32_write(DDRC_FREQ1_DRAMTMG14(0), 0x0000020);
	reg32_write(DDRC_FREQ1_DRAMTMG17(0), 0x0220011);
	reg32_write(DDRC_FREQ1_ZQCTL0(0), 0x0A70005);
	reg32_write(DDRC_FREQ1_DFITMG0(0), 0x3858202);
	reg32_write(DDRC_FREQ1_DFITMG1(0), 0x0000404);
	reg32_write(DDRC_FREQ1_DFITMG2(0), 0x0000502);

	/* performance setting */
	dwc_ddrphy_apb_wr(DDRC_ODTCFG(0), 0x0b060908);
	dwc_ddrphy_apb_wr(DDRC_ODTMAP(0), 0x00000000);
	dwc_ddrphy_apb_wr(DDRC_SCHED(0), 0x29511505);
	dwc_ddrphy_apb_wr(DDRC_SCHED1(0), 0x0000002c);
	dwc_ddrphy_apb_wr(DDRC_PERFHPR1(0), 0x5900575b);
	/* 150T starve and 0x90 max tran len */
	dwc_ddrphy_apb_wr(DDRC_PERFLPR1(0), 0x90000096);
	/* 300T starve and 0x10 max tran len */
	dwc_ddrphy_apb_wr(DDRC_PERFWR1(0), 0x1000012c);
	dwc_ddrphy_apb_wr(DDRC_DBG0(0), 0x00000016);
	dwc_ddrphy_apb_wr(DDRC_DBG1(0), 0x00000000);
	dwc_ddrphy_apb_wr(DDRC_DBGCMD(0), 0x00000000);
	dwc_ddrphy_apb_wr(DDRC_SWCTL(0), 0x00000001);
	dwc_ddrphy_apb_wr(DDRC_POISONCFG(0), 0x00000011);
	dwc_ddrphy_apb_wr(DDRC_PCCFG(0), 0x00000111);
	dwc_ddrphy_apb_wr(DDRC_PCFGR_0(0), 0x000010f3);
	dwc_ddrphy_apb_wr(DDRC_PCFGW_0(0), 0x000072ff);
	dwc_ddrphy_apb_wr(DDRC_PCTRL_0(0), 0x00000001);
	/* disable Read Qos*/
	dwc_ddrphy_apb_wr(DDRC_PCFGQOS0_0(0), 0x00000e00);
	dwc_ddrphy_apb_wr(DDRC_PCFGQOS1_0(0), 0x0062ffff);
	/* disable Write Qos*/
	dwc_ddrphy_apb_wr(DDRC_PCFGWQOS0_0(0), 0x00000e00);
	dwc_ddrphy_apb_wr(DDRC_PCFGWQOS1_0(0), 0x0000ffff);
	dwc_ddrphy_apb_wr(DDRC_FREQ1_DERATEEN(0), 0x00000202);
	dwc_ddrphy_apb_wr(DDRC_FREQ1_DERATEINT(0), 0xec78f4b5);
	dwc_ddrphy_apb_wr(DDRC_FREQ1_RFSHCTL0(0), 0x00618040);
	dwc_ddrphy_apb_wr(DDRC_FREQ1_RFSHTMG(0), 0x00610090);
}

void ddr_init(void)
{
	reg32_write(SRC_DDRC_RCR_ADDR + 0x04, 0x8F00000F);
	reg32_write(SRC_DDRC_RCR_ADDR, 0x8F00000F);
	mdelay(100);
	reg32_write(SRC_DDRC_RCR_ADDR + 0x04, 0x8F000000);

	/* change the clock source of dram_apb_clk_root */
	reg32_write(CCM_IP_CLK_ROOT_GEN_TAGET_CLR(1), (0x7<<24)|(0x7<<16));
	reg32_write(CCM_IP_CLK_ROOT_GEN_TAGET_SET(1), (0x4<<24)|(0x3<<16));

	/* disable iso */
	reg32_write(0x303A00EC, 0x0000ffff); /* PGC_CPU_MAPPING */
	reg32setbit(0x303A00F8, 5); /* PU_PGC_SW_PUP_REQ */

	dram_pll_init(SSCG_PLL_OUT_800M);

	reg32_write(SRC_DDRC_RCR_ADDR, 0x8F000006);

	/* Configure uMCTL2's registers */
	lpddr4_800MHz_cfg_umctl2();

#ifdef DDR_BOOT_P2
	reg32_write(DDRC_MSTR2(0), 0x2);
#else
#ifdef DDR_BOOT_P1
	reg32_write(DDRC_MSTR2(0), 0x1);
#endif
#endif
	/* release [1]ddr1_core_reset_n, [2]ddr1_phy_reset, [3]ddr1_phy_pwrokin_n */
	reg32_write(SRC_DDRC_RCR_ADDR, 0x8F000004);

	/* release [1]ddr1_core_reset_n, [2]ddr1_phy_reset, [3]ddr1_phy_pwrokin_n */
	reg32_write(SRC_DDRC_RCR_ADDR, 0x8F000000);

	reg32_write(DDRC_DBG1(0), 0x00000000);
	tmp = reg32_read(DDRC_PWRCTL(0));
	reg32_write(DDRC_PWRCTL(0), 0x000000a8);

	while ((reg32_read(DDRC_STAT(0)) & 0x33f) != 0x223)
		;

	reg32_write(DDRC_SWCTL(0), 0x00000000);

	/* LPDDR4 mode */
	reg32_write(DDRC_DDR_SS_GPR0, 0x01);

#ifdef DDR_BOOT_P1
	reg32_write(DDRC_DFIMISC(0), 0x00000110);
#else
	reg32_write(DDRC_DFIMISC(0), 0x00000010);
#endif
	/* LPDDR4 PHY config and training */
	lpddr4_800M_cfg_phy();

	reg32_write(DDRC_RFSHCTL3(0), 0x00000000);

	reg32_write(DDRC_SWCTL(0), 0x0000);

	/* Set DFIMISC.dfi_init_start to 1 */
#ifdef DDR_BOOT_P2
	reg32_write(DDRC_DFIMISC(0), 0x00000230);
#else
#ifdef DDR_BOOT_P1
	reg32_write(DDRC_DFIMISC(0), 0x00000130);
#else
	reg32_write(DDRC_DFIMISC(0), 0x00000030);
#endif
#endif
	reg32_write(DDRC_SWCTL(0), 0x0001);

	/* wait DFISTAT.dfi_init_complete to 1 */
	while ((reg32_read(DDRC_DFISTAT(0)) & 0x1) == 0x0)
		;

	reg32_write(DDRC_SWCTL(0), 0x0000);

#ifdef DDR_BOOT_P2
	reg32_write(DDRC_DFIMISC(0), 0x00000210);
	/* set DFIMISC.dfi_init_complete_en again */
	reg32_write(DDRC_DFIMISC(0), 0x00000211);
#else
#ifdef DDR_BOOT_P1
	reg32_write(DDRC_DFIMISC(0), 0x00000110);
	/* set DFIMISC.dfi_init_complete_en again */
	reg32_write(DDRC_DFIMISC(0), 0x00000111);
#else
	/* clear DFIMISC.dfi_init_complete_en */
	reg32_write(DDRC_DFIMISC(0), 0x00000010);
	/* set DFIMISC.dfi_init_complete_en again */
	reg32_write(DDRC_DFIMISC(0), 0x00000011);
#endif
#endif

	reg32_write(DDRC_PWRCTL(0), 0x00000088);

	tmp = reg32_read(DDRC_CRCPARSTAT(0));

	/*
	 * set SWCTL.sw_done to enable quasi-dynamic register
	 * programming outside reset.
	 */
	reg32_write(DDRC_SWCTL(0), 0x00000001);

	/* wait SWSTAT.sw_done_ack to 1 */
	while ((reg32_read(DDRC_SWSTAT(0)) & 0x1) == 0x0)
		;

	/* wait STAT.operating_mode([1:0] for ddr3) to normal state */
	while ((reg32_read(DDRC_STAT(0)) & 0x3) != 0x1)
		;

	reg32_write(DDRC_PWRCTL(0), 0x00000088);

	tmp = reg32_read(DDRC_CRCPARSTAT(0));

	reg32_write(DDRC_PCTRL_0(0), 0x00000001);

	tmp = reg32_read(DDRC_CRCPARSTAT(0));
	reg32_write(DDRC_RFSHCTL3(0), 0x00000000);
}
