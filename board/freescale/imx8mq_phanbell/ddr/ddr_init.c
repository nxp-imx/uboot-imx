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

int get_imx8m_baseboard_id(void);
void ddr_cfg_phy(void);
volatile unsigned int tmp, tmp_t, i;
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
		reg32_write(0x30360068,0xac0784);
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
		reg32_write(0x3d400020,0x101);
		reg32_write(0x3d400024,0xc35000);
		reg32_write(0x3d400064,0x300048);
		reg32_write(0x3d4000d0,0xc002030f);
		reg32_write(0x3d4000d4,0x500000);
		reg32_write(0x3d4000dc,0xa40012);
		reg32_write(0x3d4000e0,0x310008);
		reg32_write(0x3d4000e8,0x66004a);
		reg32_write(0x3d4000ec,0x16004a);
		reg32_write(0x3d400100,0x10100d11);
		reg32_write(0x3d400104,0x3041a);
		reg32_write(0x3d40010c,0x606000);
		reg32_write(0x3d400110,0x8040408);
		reg32_write(0x3d400114,0x2030606);
		reg32_write(0x3d400118,0x1010004);
		reg32_write(0x3d40011c,0x301);
		reg32_write(0x3d400130,0x20300);
		reg32_write(0x3d400134,0xa100002);
		reg32_write(0x3d400138,0x4b);
		reg32_write(0x3d400144,0x500028);
		reg32_write(0x3d400180,0x190000c);
		reg32_write(0x3d400184,0x14030d4);
		reg32_write(0x3d400188,0x0);
		reg32_write(0x3d400190,0x4898204);
		reg32_write(0x3d400194,0x80303);
		reg32_write(0x3d4001a0,0xe0400018);
		reg32_write(0x3d4001a4,0xdf00e4);
		reg32_write(0x3d4001a8,0x80000000);
		reg32_write(0x3d4001b0,0x11);
		reg32_write(0x3d4001b4,0x904);
		reg32_write(0x3d4001c0,0x1);
		reg32_write(0x3d4001c4,0x1);
		reg32_write(0x3d4000f4,0x639);
		reg32_write(0x3d400108,0x4070b0d);
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
		tmp = reg32_read(DDRC_CRCPARSTAT(0));
		reg32_write(DDRC_RFSHCTL3(0), 0x00000000);
	} else {
		/** Initialize DDR clock and DDRC registers **/
		reg32_write(0x3038a088,0x7070000);
		reg32_write(0x3038a084,0x4030000);
		reg32_write(0x303a00ec,0xffff);
		tmp=reg32_read(0x303a00f8);
		tmp |= 0x20;
		reg32_write(0x303a00f8,tmp);
		reg32_write(0x30391000,0x8f000000);
		reg32_write(0x30391004,0x8f000000);
		reg32_write(0x30360068,0xbbe582);
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
		reg32_write(0x3d400000,0x83080020);
		reg32_write(0x3d400064,0x300070);
		reg32_write(0x3d4000d0,0xc002030e);
		reg32_write(0x3d4000d4,0x4f0000);
		reg32_write(0x3d4000dc,0xa40012);
		reg32_write(0x3d4000e0,0x310008);
		reg32_write(0x3d4000e8,0x46004d);
		reg32_write(0x3d4000ec,0x15004d);
		reg32_write(0x3d4000f4,0x639);
		reg32_write(0x3d400100,0x10100d11);
		reg32_write(0x3d400104,0x3031a);
		reg32_write(0x3d400108,0x4070a0d);
		reg32_write(0x3d40010c,0x606000);
		reg32_write(0x3d400110,0x8040408);
		reg32_write(0x3d400114,0x2030606);
		reg32_write(0x3d400118,0x1010004);
		reg32_write(0x3d40011c,0x301);
		reg32_write(0x3d400130,0x20300);
		reg32_write(0x3d400134,0xb100002);
		reg32_write(0x3d400138,0x73);
		reg32_write(0x3d400144,0x500028);
		reg32_write(0x3d400180,0x190000c);
		reg32_write(0x3d400184,0x14030d4);
		reg32_write(0x3d400190,0x4898204);
		reg32_write(0x3d400194,0x80303);
		reg32_write(0x3d4001b4,0x904);
		reg32_write(0x3d4001b0,0x11);
		reg32_write(0x3d4001a0,0xe0400018);
		reg32_write(0x3d4001a4,0xdf00e4);
		reg32_write(0x3d4001a8,0x0);
		reg32_write(0x3d4001c0,0x1);
		reg32_write(0x3d4001c4,0x1);
		reg32_write(0x3d400200,0x15);
		reg32_write(0x3d40020c,0x0);
		reg32_write(0x3d400210,0x1f1f);
		reg32_write(0x3d400204,0x80808);
		reg32_write(0x3d400214,0x7070707);
		reg32_write(0x3d400218,0x48080707);
		reg32_write(0x3d400244,0x0);
		reg32_write(0x3d40025c,0x9);
		reg32_write(0x3d400490,0x1);
		reg32_write(0x30391000,0x8f000004);
		reg32_write(0x30391000,0x8f000000);
		reg32_write(0x3d400304,0x0);
		reg32_write(0x3d400030,0xa8);
		reg32_write(0x3d400320,0x0);
		reg32_write(0x3d000000,0x1);
		reg32_write(0x3d4001b0,0x10);
		reg32_write(0x3d402100,0xa040305);
		reg32_write(0x3d402104,0x30407);
		reg32_write(0x3d402108,0x203060b);
		reg32_write(0x3d40210c,0x505000);
		reg32_write(0x3d402110,0x2040202);
		reg32_write(0x3d402114,0x2030202);
		reg32_write(0x3d402118,0x1010004);
		reg32_write(0x3d40211c,0x301);
		reg32_write(0x3d402138,0x1d);
		reg32_write(0x3d402144,0x14000a);
		reg32_write(0x3d403024,0x30d400);
		reg32_write(0x3d402050,0x20d040);
		reg32_write(0x3d402190,0x3818200);
		reg32_write(0x3d4021b4,0x100);
		reg32_write(0x3d402064,0xc001c);
		reg32_write(0x3d4020dc,0x840000);
		reg32_write(0x3d4020e8,0x46004d);
		reg32_write(0x3d4020ec,0x15004d);
		reg32_write(0x3d4020e0,0x310000);
		reg32_write(0x3d403100,0x6010102);
		reg32_write(0x3d403104,0x30404);
		reg32_write(0x3d403108,0x203060b);
		reg32_write(0x3d40310c,0x505000);
		reg32_write(0x3d403110,0x2040202);
		reg32_write(0x3d403114,0x2030202);
		reg32_write(0x3d403118,0x1010004);
		reg32_write(0x3d40311c,0x301);
		reg32_write(0x3d403138,0x8);
		reg32_write(0x3d403144,0x50003);
		reg32_write(0x3d403024,0xc3500);
		reg32_write(0x3d403050,0x20d040);
		reg32_write(0x3d403190,0x3818200);
		reg32_write(0x3d4031b4,0x100);
		reg32_write(0x3d403064,0x30007);
		reg32_write(0x3d4030dc,0x840000);
		reg32_write(0x3d4030e8,0x46004d);
		reg32_write(0x3d4030ec,0x15004d);
		reg32_write(0x3d4030e0,0x310000);
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

		/* Configure LPDDR4 PHY's registers */
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
		tmp = reg32_read(DDRC_CRCPARSTAT(0));
		reg32_write(DDRC_RFSHCTL3(0), 0x00000000);
	}
}
