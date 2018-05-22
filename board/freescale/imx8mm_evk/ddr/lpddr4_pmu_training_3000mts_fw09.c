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
#include "ddr.h"

void ddr_init(void)
{
	volatile unsigned int tmp;

	/*
	 * Desc: assert [0]ddr1_preset_n, [1]ddr1_core_reset_n, 
	 *              [2]ddr1_phy_reset, [3]ddr1_phy_pwrokin_n, 
	 *              [4]src_system_rst_b!
	 */
	reg32_write(SRC_DDRC_RCR_ADDR, 0x8F00003F); 

	/* Desc: deassert [4]src_system_rst_b! */
	reg32_write(SRC_DDRC_RCR_ADDR, 0x8F00000F); 

	/*  
	 * Desc: change the clock source of dram_apb_clk_root 
	 *       to source 4 --800MHz/4
	 */  
	#if 0
	reg32_write(CCM_IP_CLK_ROOT_GEN_TAGET_CLR(1),(0x7<<24)|(0x7<<16));   
	reg32_write(CCM_IP_CLK_ROOT_GEN_TAGET_SET(1),(0x4<<24)|(0x3<<16));
	#else

	clock_set_target_val(DRAM_APB_CLK_ROOT, CLK_ROOT_ON | \
			     CLK_ROOT_SOURCE_SEL(0) | \
			     CLK_ROOT_POST_DIV(CLK_ROOT_POST_DIV1));
	clock_set_target_val(DRAM_ALT_CLK_ROOT, CLK_ROOT_ON | \
			     CLK_ROOT_SOURCE_SEL(0) | \
			     CLK_ROOT_POST_DIV(CLK_ROOT_POST_DIV1));
	clock_set_target_val(NOC_CLK_ROOT, CLK_ROOT_ON | \
			     CLK_ROOT_SOURCE_SEL(0) | \
			     CLK_ROOT_POST_DIV(CLK_ROOT_POST_DIV1));
	clock_set_target_val(NOC_APB_CLK_ROOT, CLK_ROOT_ON | \
			     CLK_ROOT_SOURCE_SEL(0) | \
			     CLK_ROOT_POST_DIV(CLK_ROOT_POST_DIV1));
	#endif

	/* Desc: disable iso  PGC_CPU_MAPPING,PU_PGC_SW_PUP_REQ */  
	reg32_write(0x303A00EC,0x0000ffff);
	reg32setbit(0x303A00F8,5);

	/*
	 * Desc: configure dram pll to 750M
	 */  
	dram_pll_init(DRAM_PLL_OUT_750M);


	/*  
	 * Desc: release [0]ddr1_preset_n, [1]ddr1_core_reset_n, 
	 *               [2]ddr1_phy_reset, [3]ddr1_phy_pwrokin_n
	 */
	reg32_write(SRC_DDRC_RCR_ADDR, 0x8F000006); 

	/* Desc: Configure uMCTL2's registers */
	lpddr4_3000mts_cfg_umctl2();

	/* Desc: diable ctlupd */
	reg32_write(DDRC_DFIUPD0(0),	0xE0300018);

	/*  
	 * Desc: release [1]ddr1_core_reset_n, [2]ddr1_phy_reset, 
	 *               [3]ddr1_phy_pwrokin_n
	 */  
	reg32_write(SRC_DDRC_RCR_ADDR, 0x8F000004);

	/*  
	 * Desc: release [1]ddr1_core_reset_n, [2]ddr1_phy_reset, 
	 *               [3]ddr1_phy_pwrokin_n
	 */  
	reg32_write(SRC_DDRC_RCR_ADDR, 0x8F000000);

	/*  
	 * Desc: ('b00000000_00000000_00000000_00000000) ('d0)
	 */  
	reg32_write(DDRC_DBG1(0), 0x00000000);

	/*  
	 * Desc: [8]--1: lpddr4_sr allowed; [5]--1: software entry to SR
	 */  
	reg32_write(DDRC_PWRCTL(0), 0x000000a8); 

	tmp=0;
	while(tmp != 0x223) {
		tmp  = 0x33f & (reg32_read(DDRC_STAT(0)));
		ddr_dbg("C: waiting for STAT selfref_type= Self Refresh\n");
	}

	/*  
	 * Desc:  ('b00000000_00000000_00000000_00000000) ('d0)
	 */  
	reg32_write(DDRC_SWCTL(0), 0x00000000);

	/*  
	 * Desc: LPDDR4 mode
	 */  
	reg32_write(DDRC_DDR_SS_GPR0, 0x01);

	/*  
	 * Desc: [12:8]dfi_freq, [5]dfi_init_start, [4]ctl_idle_en
	 */  
	reg32_write(DDRC_DFIMISC(0), 0x00000010); 

	/*  
	 * Desc: Configure LPDDR4 PHY's registers 
	 */  
	lpddr4_750M_cfg_phy();

	reg32_write(DDRC_RFSHCTL3(0), 0x00000000);
	reg32_write(DDRC_SWCTL(0), 0x0000);

	/*  
	 * Desc: Set DFIMISC.dfi_init_start to 1 
	 *       [5]--1: dfi_init_start, [4] ctl_idle_en
	 */  
	reg32_write(DDRC_DFIMISC(0), 0x00000030); 
	reg32_write(DDRC_SWCTL(0), 0x0001);

	/*  
	 * Desc: wait DFISTAT.dfi_init_complete to 1
	 */  
	while(!(0x1 & (reg32_read(DDRC_DFISTAT(0)))));

	reg32_write(DDRC_SWCTL(0), 0x0000);

	/*  
	 * Desc: clear DFIMISC.dfi_init_complete_en
	 *       ('b00000000_00000000_00000000_00010000) ('d16)
	 */  
	reg32_write(DDRC_DFIMISC(0), 0x00000010); 

	/*  
	 * Desc: set DFIMISC.dfi_init_complete_en again
	 *       ('b00000000_00000000_00000000_00010001) ('d17)
	 */  
	reg32_write(DDRC_DFIMISC(0), 0x00000011); 

	/*  
	 * Desc: ('b00000000_00000000_00000000_10001000) ('d136)
	 */  
	reg32_write(DDRC_PWRCTL(0), 0x00000088);

	/*  
	 * Desc: set SWCTL.sw_done to enable quasi-dynamic 
	 *       register programming outside reset.
	 *       ('b00000000_00000000_00000000_00000001) ('d1)
	 */  
	reg32_write(DDRC_SWCTL(0), 0x00000001); 

	/*  
	 * Desc: wait SWSTAT.sw_done_ack to 1
	 */  
	while(!(0x1 & (reg32_read(DDRC_SWSTAT(0)))));

	/*  
	 * Desc: wait STAT.operating_mode([2:0] for lpddr4) to normal state
	 */  
	while(0x1 != (0x7 & (reg32_read(DDRC_STAT(0)))));

	/*
	 * Desc: ('b00000000_00000000_00000000_10001000) ('d136)
	 */
	reg32_write(DDRC_PWRCTL(0), 0x00000088);


	/*
	 * Desc: enable port 0
	 *       ('b00000000_00000000_00000000_00000001) ('d1)
	 */
	reg32_write(DDRC_PCTRL_0(0), 0x00000001); 
}
