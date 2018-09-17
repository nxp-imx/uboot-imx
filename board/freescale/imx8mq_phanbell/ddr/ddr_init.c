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

int get_imx8m_baseboard_id(void);
void ddr_cfg_phy(void);
void ddr_init(void)
{
	int board_id = 0;

	board_id = get_imx8m_baseboard_id();
	if ((board_id == ENTERPRISE_MICRON_1G) ||
			(board_id == ENTERPRISE_HYNIX_1G)) {
		/** Initialize DDR clock and DDRC registers **/
		reg32_write(0x3038a088,0x7070000);
		reg32_write(0x3038a084,0x4030000);
		reg32_write(0x303a00ec,0xffff);
		tmp=reg32_read(0x303a00f8);
		tmp |= 0x20;
		reg32_write(0x303a00f8,tmp);
		reg32_write(0x30391000,0x8f000000);
		reg32_write(0x30391004,0x8f000000);
		reg32_write(0x30360068,0xece580);
		tmp=reg32_read(0x30360060);
		tmp &= ~0x80;
		reg32_write(0x30360060,tmp);
		tmp=reg32_read(0x30360060);
		tmp |= 0x200;
		reg32_write(0x30360060,tmp);
		tmp=reg32_read(0x30360060);
		tmp &= ~0x20;
		reg32_write(0x30360060,tmp);
		tmp=reg32_read(0x30360060);
		tmp &= ~0x10;
		reg32_write(0x30360060,tmp);
		do{
			tmp=reg32_read(0x30360060);
			if(tmp&0x80000000) break;
		}while(1);
		reg32_write(0x30391000,0x8f000006);
		reg32_write(0x3d400304,0x1);
		reg32_write(0x3d400030,0x1);
		reg32_write(0x3d400000,0xa1080020);
		reg32_write(0x3d400028,0x0);
		reg32_write(0x3d400020,0x203);
		reg32_write(0x3d400024,0x186a000);
		reg32_write(0x3d400064,0x610090);
		reg32_write(0x3d4000d0,0xc003061c);
		reg32_write(0x3d4000d4,0x9e0000);
		reg32_write(0x3d4000dc,0xd4002d);
		reg32_write(0x3d4000e0,0x310008);
		reg32_write(0x3d4000e8,0x66004a);
		reg32_write(0x3d4000ec,0x16004a);
		reg32_write(0x3d400100,0x1a201b22);
		reg32_write(0x3d400104,0x60633);
		reg32_write(0x3d40010c,0xc0c000);
		reg32_write(0x3d400110,0xf04080f);
		reg32_write(0x3d400114,0x2040c0c);
		reg32_write(0x3d400118,0x1010007);
		reg32_write(0x3d40011c,0x401);
		reg32_write(0x3d400130,0x20600);
		reg32_write(0x3d400134,0xc100002);
		reg32_write(0x3d400138,0x96);
		reg32_write(0x3d400144,0xa00050);
		reg32_write(0x3d400180,0x3200018);
		reg32_write(0x3d400184,0x28061a8);
		reg32_write(0x3d400188,0x0);
		reg32_write(0x3d400190,0x497820a);
		reg32_write(0x3d400194,0x80303);
		reg32_write(0x3d4001a0,0xe0400018);
		reg32_write(0x3d4001a4,0xdf00e4);
		reg32_write(0x3d4001a8,0x80000000);
		reg32_write(0x3d4001b0,0x11);
		reg32_write(0x3d4001b4,0x170a);
		reg32_write(0x3d4001c0,0x1);
		reg32_write(0x3d4001c4,0x1);
		reg32_write(0x3d4000f4,0x639);
		reg32_write(0x3d400108,0x70e1214);
		reg32_write(0x3d400200,0x1f);
		reg32_write(0x3d40020c,0x0);
		reg32_write(0x3d400210,0x1f1f);
		reg32_write(0x3d400204,0x80808);
		reg32_write(0x3d400214,0x7070707);
		reg32_write(0x3d400218,0xf070707);
		reg32_write(0x3d402020,0x1);
		reg32_write(0x3d402024,0x518b00);
		reg32_write(0x3d402050,0x20d040);
		reg32_write(0x3d402064,0x14001f);
		reg32_write(0x3d4020dc,0x940009);
		reg32_write(0x3d4020e0,0x310000);
		reg32_write(0x3d4020e8,0x66004a);
		reg32_write(0x3d4020ec,0x16004a);
		reg32_write(0x3d402100,0xb070508);
		reg32_write(0x3d402104,0x3040b);
		reg32_write(0x3d402108,0x305090c);
		reg32_write(0x3d40210c,0x505000);
		reg32_write(0x3d402110,0x4040204);
		reg32_write(0x3d402114,0x2030303);
		reg32_write(0x3d402118,0x1010004);
		reg32_write(0x3d40211c,0x301);
		reg32_write(0x3d402130,0x20300);
		reg32_write(0x3d402134,0xa100002);
		reg32_write(0x3d402138,0x20);
		reg32_write(0x3d402144,0x220011);
		reg32_write(0x3d402180,0xa70006);
		reg32_write(0x3d402190,0x3858202);
		reg32_write(0x3d402194,0x80303);
		reg32_write(0x3d4021b4,0x502);
		reg32_write(0x3d400244,0x0);
		reg32_write(0x3d400250,0x29001505);
		reg32_write(0x3d400254,0x2c);
		reg32_write(0x3d40025c,0x5900575b);
		reg32_write(0x3d400264,0x9);
		reg32_write(0x3d40026c,0x2005574);
		reg32_write(0x3d400300,0x16);
		reg32_write(0x3d400304,0x0);
		reg32_write(0x3d40030c,0x0);
		reg32_write(0x3d400320,0x1);
		reg32_write(0x3d40036c,0x11);
		reg32_write(0x3d400400,0x111);
		reg32_write(0x3d400404,0x10f3);
		reg32_write(0x3d400408,0x72ff);
		reg32_write(0x3d400490,0x1);
		reg32_write(0x3d400494,0x1110d00);
		reg32_write(0x3d400498,0x620790);
		reg32_write(0x3d40049c,0x100001);
		reg32_write(0x3d4004a0,0x41f);
		reg32_write(0x30391000,0x8f000004);
		reg32_write(0x30391000,0x8f000000);
		reg32_write(0x3d400030,0xa8);
		do{
			tmp=reg32_read(0x3d400004);
			if(tmp&0x223) break;
		}while(1);
		reg32_write(0x3d400320,0x0);
		reg32_write(0x3d000000,0x1);
		reg32_write(0x3d4001b0,0x10);
		reg32_write(0x3c040280,0x0);
		reg32_write(0x3c040284,0x1);
		reg32_write(0x3c040288,0x2);
		reg32_write(0x3c04028c,0x3);
		reg32_write(0x3c040290,0x4);
		reg32_write(0x3c040294,0x5);
		reg32_write(0x3c040298,0x6);
		reg32_write(0x3c04029c,0x7);
		reg32_write(0x3c044280,0x0);
		reg32_write(0x3c044284,0x1);
		reg32_write(0x3c044288,0x2);
		reg32_write(0x3c04428c,0x3);
		reg32_write(0x3c044290,0x4);
		reg32_write(0x3c044294,0x5);
		reg32_write(0x3c044298,0x6);
		reg32_write(0x3c04429c,0x7);
		reg32_write(0x3c048280,0x0);
		reg32_write(0x3c048284,0x1);
		reg32_write(0x3c048288,0x2);
		reg32_write(0x3c04828c,0x3);
		reg32_write(0x3c048290,0x4);
		reg32_write(0x3c048294,0x5);
		reg32_write(0x3c048298,0x6);
		reg32_write(0x3c04829c,0x7);
		reg32_write(0x3c04c280,0x0);
		reg32_write(0x3c04c284,0x1);
		reg32_write(0x3c04c288,0x2);
		reg32_write(0x3c04c28c,0x3);
		reg32_write(0x3c04c290,0x4);
		reg32_write(0x3c04c294,0x5);
		reg32_write(0x3c04c298,0x6);
		reg32_write(0x3c04c29c,0x7);

		/* Configure DDR PHY's registers */
		ddr_cfg_phy();

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
		/* enable DDR auto-refresh mode */
		tmp = reg32_read(DDRC_RFSHCTL3(0)) & ~0x1;
		reg32_write(DDRC_RFSHCTL3(0), tmp);
	} else {
		/* Default use 3G DDR */
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
}
