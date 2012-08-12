/*
 * Copyright (C) 2012 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <config.h>
#include <asm/arch/mx6.h>

#ifdef	CONFIG_FLASH_HEADER
#ifndef CONFIG_FLASH_HEADER_OFFSET
# error "Must define the offset of flash header"
#endif

#ifndef CONFIG_FLASH_PLUG_IN

/********************DCD mode***********************/
#define CPU_2_BE_32(l) \
       ((((l) & 0x000000FF) << 24) | \
	(((l) & 0x0000FF00) << 8)  | \
	(((l) & 0x00FF0000) >> 8)  | \
	(((l) & 0xFF000000) >> 24))

#define MXC_DCD_ITEM(i, addr, val)   \
dcd_node_##i:                        \
        .word CPU_2_BE_32(addr) ;     \
        .word CPU_2_BE_32(val)  ;     \

.section ".text.flasheader", "x"
	b	_start
	.org	CONFIG_FLASH_HEADER_OFFSET

ivt_header:       .word 0x402000D1 /* Tag=0xD1, Len=0x0020, Ver=0x40 */
app_code_jump_v:  .word _start
reserv1:          .word 0x0
dcd_ptr:          .word dcd_hdr
boot_data_ptr:	  .word boot_data
self_ptr:         .word ivt_header
#ifdef CONFIG_SECURE_BOOT
app_code_csf:     .word __hab_data
#else
app_code_csf:     .word 0x0
#endif
reserv2:          .word 0x0

boot_data:        .word TEXT_BASE
#ifdef CONFIG_SECURE_BOOT
image_len:        .word __hab_data_end - TEXT_BASE + CONFIG_FLASH_HEADER_OFFSET
#else
image_len:        .word _end_of_copy  - TEXT_BASE + CONFIG_FLASH_HEADER_OFFSET
#endif
plugin:           .word 0x0

/* !!!! Need update Len field after adding ddr script !!!!!!!!!!!!!!!!!*/
dcd_hdr:          .word 0x404802D2 /* Tag=0xD2, Len=72*8 + 4 + 4, Ver=0x40 */
write_dcd_cmd:    .word 0x044402CC /* Tag=0xCC, Len=72*8 + 4, Param=0x04 */

/*###################put the ddr script here ######################*/
/*========================================================================*/
/*init script for i.MX6SL LPDDR2*/
/*========================================================================*/
/* Revision History*/
/* v0.1 : Init pre-silicon version for Samsung K4P8G304EB-AGC1 on CPU LPDDR2
          board. It's currently soldered, not PoPed.*/
/* v0.2 : CCM, IO, LPDDR_2ch config fixed*/

/* If someone is playing this init on different DDR device, or on PoPed board,
   please feedback me with result.*/
/* boaz.perlman@freescale.com*/

/* v0.9 : RALAT 5->3 (improved operation at low freq). CK_FT0_DCC=CK_FT1_DCC=1
          (Improved SDCLK signal shape)*/
/* v0.91 : IOMUXC_SW_PAD_CTL_PAD_DRAM_SDCKE0/1 programming is removed, as this
           got no effect on SDCKE drive strength, pull activiticy.*/
/* v0.93 : DRAM_RESET/DDR_SEL=00, as a workaround to SDCLK cross point and
           duty cycle offsets.*/
/*========================================================================*/

/*wait = on*/
/*========================================================================*/
/* Disable	WDOG*/
/*========================================================================*/
/*setmem /16	0x020bc000 =	0x30*/

/*========================================================================*/
/* Enable all clocks (they are disabled by ROM code)*/
/*========================================================================*/
/*setmem /32	0x020c4068 =	0xffffffff*/
/*setmem /32	0x020c406c =	0xffffffff*/
/*setmem /32	0x020c4070 =	0xffffffff*/
/*setmem /32	0x020c4074 =	0xffffffff*/
/*setmem /32	0x020c4078 =	0xffffffff*/
/*setmem /32	0x020c407c =	0xffffffff*/
/*setmem /32	0x020c4080 =	0xffffffff*/
/*setmem /32	0x020c4084 =	0xffffffff*/

/*DDR clk to 400MHz*/
/*CCM_BASE_ADDR = 0x020c4000*/
MXC_DCD_ITEM(1, CCM_BASE_ADDR + 0x018, 0x00260324)

/*========================================================================*/
/* IOMUX*/
/*========================================================================*/
/* Megrez note: IOMUX configs specify absolute addr in Arik IOMUXC. Changes to
   Megrez addr.*/
/* Megrez note: Good chance that drive strength change is required. to change
   them all by editing the LSB value "38"-> ""30" or "28"*/
/* Megrez note: Timing also can be tweaked by drive strength values. It is
   mainly by giving SDCLk and SDQS different values than the sampled signals*/

/*IOMUXC_BASE_ADDR = 0x020e0000*/
/*IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS0*/
MXC_DCD_ITEM(2, IOMUXC_BASE_ADDR + 0x344, 0x00003030)
/*IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS1*/
MXC_DCD_ITEM(3, IOMUXC_BASE_ADDR + 0x348, 0x00003030)
/*IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS2*/
MXC_DCD_ITEM(4, IOMUXC_BASE_ADDR + 0x34c, 0x00003030)
/*IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS3*/
MXC_DCD_ITEM(5, IOMUXC_BASE_ADDR + 0x350, 0x00003030)

/*IOMUXC_SW_PAD_CTL_PAD_DRAM_DQM0*/
MXC_DCD_ITEM(6, IOMUXC_BASE_ADDR + 0x30c, 0x00000030)
/*IOMUXC_SW_PAD_CTL_PAD_DRAM_DQM1*/
MXC_DCD_ITEM(7, IOMUXC_BASE_ADDR + 0x310, 0x00000030)
/*IOMUXC_SW_PAD_CTL_PAD_DRAM_DQM2*/
MXC_DCD_ITEM(8, IOMUXC_BASE_ADDR + 0x314, 0x00000030)
/*IOMUXC_SW_PAD_CTL_PAD_DRAM_DQM3*/
MXC_DCD_ITEM(9, IOMUXC_BASE_ADDR + 0x318, 0x00000030)

/*IOMUXC_SW_PAD_CTL_PAD_DRAM_CAS*/
MXC_DCD_ITEM(10, IOMUXC_BASE_ADDR + 0x300, 0x00000030)
/*IOMUXC_SW_PAD_CTL_PAD_DRAM_RAS*/
MXC_DCD_ITEM(11, IOMUXC_BASE_ADDR + 0x31c, 0x00000030)
/*IOMUXC_SW_PAD_CTL_PAD_DRAM_SDCLK_0*/
MXC_DCD_ITEM(12, IOMUXC_BASE_ADDR + 0x338, 0x00000028)

/*IOMUXC_SW_PAD_CTL_PAD_DRAM_RESET*/
MXC_DCD_ITEM(13, IOMUXC_BASE_ADDR + 0x320, 0x00000030)

/*IOMUXC_SW_PAD_CTL_PAD_DRAM_SDBA2 - DSE can be configured using Group Control
  Register: IOMUXC_SW_PAD_CTL_GRP_CTLDS*/
MXC_DCD_ITEM(14, IOMUXC_BASE_ADDR + 0x32c, 0x00000000)
/*IOMUXC_SW_PAD_CTL_PAD_DRAM_SDODT0*/
MXC_DCD_ITEM(15, IOMUXC_BASE_ADDR + 0x33c, 0x00000008)
/*IOMUXC_SW_PAD_CTL_PAD_DRAM_SDODT1*/
MXC_DCD_ITEM(16, IOMUXC_BASE_ADDR + 0x340, 0x00000008)
/*IOMUXC_SW_PAD_CTL_GRP_B0DS*/
MXC_DCD_ITEM(17, IOMUXC_BASE_ADDR + 0x5c4, 0x00000030)
/*IOMUXC_SW_PAD_CTL_GRP_B1DS*/
MXC_DCD_ITEM(18, IOMUXC_BASE_ADDR + 0x5cc, 0x00000030)
/*IOMUXC_SW_PAD_CTL_GRP_B2DS*/
MXC_DCD_ITEM(19, IOMUXC_BASE_ADDR + 0x5d4, 0x00000030)
/*IOMUXC_SW_PAD_CTL_GRP_B3DS*/
MXC_DCD_ITEM(20, IOMUXC_BASE_ADDR + 0x5d8, 0x00000030)

/*IOMUXC_SW_PAD_CTL_GRP_ADDDS*/
MXC_DCD_ITEM(21, IOMUXC_BASE_ADDR + 0x5ac, 0x00000030)
/*IOMUXC_SW_PAD_CTL_GRP_CTLDS*/
MXC_DCD_ITEM(22, IOMUXC_BASE_ADDR + 0x5c8, 0x00000030)
/*IOMUXC_SW_PAD_CTL_GRP_DDRMODE_CTL*/
MXC_DCD_ITEM(23, IOMUXC_BASE_ADDR + 0x5b0, 0x00020000)
/*IOMUXC_SW_PAD_CTL_GRP_DDRPKE*/
MXC_DCD_ITEM(24, IOMUXC_BASE_ADDR + 0x5b4, 0x00000000)
/*IOMUXC_SW_PAD_CTL_GRP_DDRMODE*/
MXC_DCD_ITEM(25, IOMUXC_BASE_ADDR + 0x5c0, 0x00020000)
/*IOMUXC_SW_PAD_CTL_GRP_DDR_TYPE*/
MXC_DCD_ITEM(26, IOMUXC_BASE_ADDR + 0x5d0, 0x00080000)

/*========================================================================*/
/* DDR Controller Registers*/
/*========================================================================*/
/* Manufacturer:	Samsung*/
/* Device Part Number:	K4P8G304EB-AGC1*/
/* Clock Freq.: 	400MMHz*/
/* MMDC channels:       MMDC0*/
/* Density per CS in Gb: 	512M*/
/* Chip Selects used:	2*/
/* Number of Banks:	8*/
/* Row address:    	14*/
/* Column address: 	10*/
/* Data bus width	32*/
/*========================================================================*/
/*MMDC_P0_BASE_ADDR = 0x021b0000*/
/*MMDC0_MDSCR, set the Configuration request bit during MMDC set up*/
MXC_DCD_ITEM(27, MMDC_P0_BASE_ADDR + 0x01c, 0x00008000)

/*setmem /32      0x021b085c = 0x1b5f01ff*/
/*LPDDR2 ZQ params*/
/*LPDDR2 ZQ params*/
MXC_DCD_ITEM(28, MMDC_P0_BASE_ADDR + 0x85c, 0x1b4700c7)

/*========================================================================*/
/* Calibration setup.*/
/**/
/*========================================================================*/

/*DDR_PHY_P0_MPZQHWCTRL, enable on time ZQ calibration*/
MXC_DCD_ITEM(29, MMDC_P0_BASE_ADDR + 0x800, 0xa1390003)

/* Megrez note: If entire word fails, CA bus might be involved. Try changing
   this:*/
/*ca bus abs delay*/
MXC_DCD_ITEM(30, MMDC_P0_BASE_ADDR + 0x890, 0x00300000)

/* values of 20,40,50,60,7f tried. no difference seen*/

/* Megrez note: This is also for CA bus. A bit-bit fine tuning.*/
/*setmem /32	0x021b48bc =	0x00055555*/
/*DDR_PHY_P1_MPWRCADL*/

/*frc_msr.*/
MXC_DCD_ITEM(31, MMDC_P0_BASE_ADDR + 0x8b8, 0x00000800)

/*DDR_PHY_P0_MPREDQBY0DL3*/
MXC_DCD_ITEM(32, MMDC_P0_BASE_ADDR + 0x81c, 0x33333333)
/*DDR_PHY_P0_MPREDQBY1DL3*/
MXC_DCD_ITEM(33, MMDC_P0_BASE_ADDR + 0x820, 0x33333333)
/*DDR_PHY_P0_MPREDQBY2DL3*/
MXC_DCD_ITEM(34, MMDC_P0_BASE_ADDR + 0x824, 0x33333333)
/*DDR_PHY_P0_MPREDQBY3DL3*/
MXC_DCD_ITEM(35, MMDC_P0_BASE_ADDR + 0x828, 0x33333333)

/*write delayes:*/
/*all byte 0 data & dm delayed by 3*/
MXC_DCD_ITEM(36, MMDC_P0_BASE_ADDR + 0x82c, 0xf3333333)
/*all byte 1 data & dm delayed by 3*/
MXC_DCD_ITEM(37, MMDC_P0_BASE_ADDR + 0x830, 0xf3333333)
/*all byte 2 data & dm delayed by 3*/
MXC_DCD_ITEM(38, MMDC_P0_BASE_ADDR + 0x834, 0xf3333333)
/*all byte 3 data & dm delayed by 3*/
MXC_DCD_ITEM(39, MMDC_P0_BASE_ADDR + 0x838, 0xf3333333)

/* Read and write data delay, per byte.*/
/* For optimized DDR operation it is recommended to run mmdc_calibration on your
   board, and replace 4 delay register assigns with resulted values*/
/* Note:*/
/* a. DQS gating is not relevant for LPDDR2. DSQ gating calibration section
   should be skipped, or the write/read calibration comming after that will
   stall*/
/* b. The calibration code that runs for both MMDC0 & MMDC1 should be used.*/

/*it is strongly recommended to run calibration on your board, and replace
  bellow values:*/

/* Megrez note: New set of values is required for the following 2 delay
   registers. Try running calibration code as in Arik APN.*/

/*Read calibration*/
MXC_DCD_ITEM(40, MMDC_P0_BASE_ADDR + 0x848, 0x4241444a)

/*Write calibration*/

MXC_DCD_ITEM(41, MMDC_P0_BASE_ADDR + 0x850, 0x3030312b)

/*dqs gating dis*/
MXC_DCD_ITEM(42, MMDC_P0_BASE_ADDR + 0x83c, 0x20000000)
MXC_DCD_ITEM(43, MMDC_P0_BASE_ADDR + 0x840, 0x00000000)

/* Megrez note: Try enabling and changing the clock delay, as part of the
   calibration:*/
/*setmem /32 0x021b0858 = 0xa00*/
/*clk delay*/

/*fine tune duty cyc to low*/
MXC_DCD_ITEM(44, MMDC_P0_BASE_ADDR + 0x8c0, 0x24911492)

/*frc_msr*/
MXC_DCD_ITEM(45, MMDC_P0_BASE_ADDR + 0x8b8, 0x00000800)

/*========================================================================*/
/* Calibration setup end*/
/*========================================================================*/

/* Channel0 - startng address 0x80000000*/

/*setmem /32	0x021b000c =	0x3f436133*/
/* MMDC0_MDCFG0*/
/*MMDC0_MDCFG0*/
MXC_DCD_ITEM(46, MMDC_P0_BASE_ADDR + 0x00c, 0x33374133)
/*MMDC0_MDPDC - where is tCKSRX and tCKSRE defined in LPDDR2 data sheet?????*/
MXC_DCD_ITEM(47, MMDC_P0_BASE_ADDR + 0x004, 0x00020024)
/*MMDC0_MDCFG1*/
MXC_DCD_ITEM(48, MMDC_P0_BASE_ADDR + 0x010, 0x00100A82)
/*MMDC0_MDCFG2*/
MXC_DCD_ITEM(49, MMDC_P0_BASE_ADDR + 0x014, 0x00000093)

/*MMDC0_MDMISC. RALAT=3. Try increasing RALAT if case of failures at higher DDR
  freq*/
MXC_DCD_ITEM(50, MMDC_P0_BASE_ADDR + 0x018, 0x00001688)
/*MMDC0_MDRWD;*/
MXC_DCD_ITEM(51, MMDC_P0_BASE_ADDR + 0x02c, 0x0f9f26d2)
/*MMDC0_MDOR*/
MXC_DCD_ITEM(52, MMDC_P0_BASE_ADDR + 0x030, 0x0000020e)

/*setmem /32	0x021b0038 =	0x001a099a*/
/* MMDC0_MDCFG3LP*/
/*MMDC0_MDCFG3LP*/
MXC_DCD_ITEM(53, MMDC_P0_BASE_ADDR + 0x038, 0x00190778)
/*MMDC0_MDOTC*/
MXC_DCD_ITEM(54, MMDC_P0_BASE_ADDR + 0x008, 0x00000000)

/*CS0_END = 0x8fffffff*/
MXC_DCD_ITEM(55, MMDC_P0_BASE_ADDR + 0x040, 0x0000004f)

/*MMDC0_MDCTL*/
MXC_DCD_ITEM(56, MMDC_P0_BASE_ADDR + 0x000, 0xc3110000)

/* Channel0 : Configure DDR device:*/
/* Megrez note: Device drive strength change might help, consult device/JEDEC
   for the values.*/

/*MRW: BA=0 CS=0 MR_ADDR=63 MR_OP=0  //reset*/
MXC_DCD_ITEM(57, MMDC_P0_BASE_ADDR + 0x01c, 0x003f8030)
/*MRW: BA=0 CS=0 MR_ADDR=10 MR_OP=ff /zq*/
MXC_DCD_ITEM(58, MMDC_P0_BASE_ADDR + 0x01c, 0xff0a8030)
/*MRW: BA=0 CS=0 MR_ADDR=1  MR_OP=c2*/
MXC_DCD_ITEM(59, MMDC_P0_BASE_ADDR + 0x01c, 0x82018030)
/*MRW: BA=0 CS=0 MR_ADDR=2  MR_OP=4. tcl=6, tcwl=3*/
MXC_DCD_ITEM(60, MMDC_P0_BASE_ADDR + 0x01c, 0x04028030)
/*MRW: BA=0 CS=0 MR_ADDR=3  MR_OP=2.drive=240/6*/
MXC_DCD_ITEM(61, MMDC_P0_BASE_ADDR + 0x01c, 0x02038030)

/* Need to comment out MRW reset command to CS1 - investigating this*/
/* Also, adding delays before and after this makes no difference*/
/*setmem /32 0x021b001c = 0x003f8038*/
/* MRW: BA=0 CS=1 MR_ADDR=63 MR_OP=0*/
/*reset*/
/*MRW: BA=0 CS=1 MR_ADDR=10 MR_OP=ff*/
MXC_DCD_ITEM(62, MMDC_P0_BASE_ADDR + 0x01c, 0xff0a8038)
/*MRW: BA=0 CS=1 MR_ADDR=1  MR_OP=c2*/
MXC_DCD_ITEM(63, MMDC_P0_BASE_ADDR + 0x01c, 0x82018038)
/*MRW: BA=0 CS=1 MR_ADDR=2  MR_OP=4. tcl=6, tcwl=3*/
MXC_DCD_ITEM(64, MMDC_P0_BASE_ADDR + 0x01c, 0x04028038)
/*MRW: BA=0 CS=1 MR_ADDR=3  MR_OP=2.drive=240/6*/
MXC_DCD_ITEM(65, MMDC_P0_BASE_ADDR + 0x01c, 0x02038038)

/*######################################################*/
/*final DDR setup, before operation start:*/
/*DDR_PHY_P0_MPZQHWCTRL, enable automatic ZQ calibration*/
MXC_DCD_ITEM(66, MMDC_P0_BASE_ADDR + 0x800, 0xa1310003)

/*MMDC0_MDREF*/
MXC_DCD_ITEM(67, MMDC_P0_BASE_ADDR + 0x020, 0x00001800)

/*DDR_PHY_P0_MPODTCTRL*/
/*setmem /32	0x021b0818 =	0*/
MXC_DCD_ITEM(68, MMDC_P0_BASE_ADDR + 0x818, 0x00000000)

/*DDR_PHY_P0_MPMUR0, frc_msr*/
MXC_DCD_ITEM(69, MMDC_P0_BASE_ADDR + 0x8b8, 0x00000800)

/*MMDC0_MDPDC now SDCTL power down enabled*/
MXC_DCD_ITEM(70, MMDC_P0_BASE_ADDR + 0x004, 0x00025564)

/*MMDC0_MAPSR ADOPT power down enabled*/
MXC_DCD_ITEM(71, MMDC_P0_BASE_ADDR + 0x404, 0x00011006)

/*MMDC0_MDSCR, clear this register (especially the configuration bit as
  initialization is complete)*/
MXC_DCD_ITEM(72, MMDC_P0_BASE_ADDR + 0x01c, 0x00000000)
/*###################put the ddr script here ######################*/

#else

.section ".text.flasheader", "x"
origin:
	b	_start
	.org	CONFIG_FLASH_HEADER_OFFSET

/* First IVT to copy the plugin that initializes the system into OCRAM */
ivt_header:             .long 0x402000D1    /*Tag=0xD1, Len=0x0020, Ver=0x40 */
app_code_jump_v:        .long IRAM_FREE_START + (plugin_start - origin)    /* Plugin entry point, address after the second IVT table */
reserv1:                .long 0x0
dcd_ptr:                .long 0x0
boot_data_ptr:          .long IRAM_FREE_START + (boot_data - origin) /*0x00907420*/
self_ptr:               .long IRAM_FREE_START + (ivt_header - origin)
app_code_csf:           .long 0x0
reserv2:                .long 0x0

boot_data:              .long IRAM_FREE_START
image_len:              .long 16*1024  /* plugin can be upto 16KB in size */
plugin:                 .long 0x1  /* Enable plugin flag */

/* Second IVT to give entry point into the bootloader copied to DDR */
ivt2_header:            .long 0x402000D1    /*Tag=0xD1, Len=0x0020, Ver=0x40 */
app2_code_jump_v:       .long _start       /* Entry point for uboot */
reserv3:                .long 0x0
dcd2_ptr:               .long 0x0
boot_data2_ptr:         .long boot_data2
self_ptr2:              .long ivt2_header
app_code_csf2:          .long 0x0
reserv4:                .long 0x0

boot_data2:             .long TEXT_BASE
image_len2:             .long _end_of_copy  - TEXT_BASE + CONFIG_FLASH_HEADER_OFFSET
plugin2:                .long 0x0

/* Here starts the plugin code */
plugin_start:
/* Save the return address and the function arguments */
    push    {r0-r4, lr}

/*
 * The following is following Megrez LPDDR init script
 * Ver 0.93
 */
/*
 * CCM Configuration
 */
	ldr r0, =CCM_BASE_ADDR
/*MXC_DCD_ITEM(1, CCM_BASE_ADDR + 0x018, 0x00260324)*/
	ldr r1, =0x00260324
	str r1, [r0, #0x018]

/*
 * IOMUX Configuration
 */
	ldr r0, =IOMUXC_BASE_ADDR
/*MXC_DCD_ITEM(2, IOMUXC_BASE_ADDR + 0x344, 0x00003030)*/
	ldr r1, =0x00003030
	str r1, [r0, #0x344]
/*MXC_DCD_ITEM(3, IOMUXC_BASE_ADDR + 0x348, 0x00003030)*/
	ldr r1, =0x00003030
	str r1, [r0, #0x348]
/*MXC_DCD_ITEM(4, IOMUXC_BASE_ADDR + 0x34c, 0x00003030)*/
	ldr r1, =0x00003030
	str r1, [r0, #0x34c]
/*MXC_DCD_ITEM(5, IOMUXC_BASE_ADDR + 0x350, 0x00003030)*/
	ldr r1, =0x00003030
	str r1, [r0, #0x350]
/*MXC_DCD_ITEM(6, IOMUXC_BASE_ADDR + 0x30c, 0x00000030)*/
	ldr r1, =0x00000030
	str r1, [r0, #0x30c]
/*MXC_DCD_ITEM(7, IOMUXC_BASE_ADDR + 0x310, 0x00000030)*/
	ldr r1, =0x00000030
	str r1, [r0, #0x310]
/*MXC_DCD_ITEM(8, IOMUXC_BASE_ADDR + 0x314, 0x00000030)*/
	ldr r1, =0x00000030
	str r1, [r0, #0x314]
/*MXC_DCD_ITEM(9, IOMUXC_BASE_ADDR + 0x318, 0x00000030)*/
	ldr r1, =0x00000030
	str r1, [r0, #0x318]
/*MXC_DCD_ITEM(10, IOMUXC_BASE_ADDR + 0x300, 0x00000030)*/
	ldr r1, =0x00000030
	str r1, [r0, #0x300]
/*MXC_DCD_ITEM(11, IOMUXC_BASE_ADDR + 0x31c, 0x00000030)*/
	ldr r1, =0x00000030
	str r1, [r0, #0x31c]
/*MXC_DCD_ITEM(12, IOMUXC_BASE_ADDR + 0x338, 0x00000028)*/
	ldr r1, =0x00000028
	str r1, [r0, #0x338]
/*MXC_DCD_ITEM(13, IOMUXC_BASE_ADDR + 0x320, 0x00000030)*/
	ldr r1, =0x00000030
	str r1, [r0, #0x320]
/*MXC_DCD_ITEM(14, IOMUXC_BASE_ADDR + 0x32c, 0x00000000)*/
	ldr r1, =0x00000000
	str r1, [r0, #0x32c]
/*MXC_DCD_ITEM(15, IOMUXC_BASE_ADDR + 0x33c, 0x00000008)*/
	ldr r1, =0x00000008
	str r1, [r0, #0x33c]
/*MXC_DCD_ITEM(16, IOMUXC_BASE_ADDR + 0x340, 0x00000008)*/
	ldr r1, =0x00000008
	str r1, [r0, #0x340]
/*MXC_DCD_ITEM(17, IOMUXC_BASE_ADDR + 0x5c4, 0x00000030)*/
	ldr r1, =0x00000030
	str r1, [r0, #0x5c4]
/*MXC_DCD_ITEM(18, IOMUXC_BASE_ADDR + 0x5cc, 0x00000030)*/
	ldr r1, =0x00000030
	str r1, [r0, #0x5cc]
/*MXC_DCD_ITEM(19, IOMUXC_BASE_ADDR + 0x5d4, 0x00000030)*/
	ldr r1, =0x00000030
	str r1, [r0, #0x5d4]
/*MXC_DCD_ITEM(20, IOMUXC_BASE_ADDR + 0x5d8, 0x00000030)*/
	ldr r1, =0x00000030
	str r1, [r0, #0x5d8]
/*MXC_DCD_ITEM(21, IOMUXC_BASE_ADDR + 0x5ac, 0x00000030)*/
	ldr r1, =0x00000030
	str r1, [r0, #0x5ac]
/*MXC_DCD_ITEM(22, IOMUXC_BASE_ADDR + 0x5c8, 0x00000030)*/
	ldr r1, =0x00000030
	str r1, [r0, #0x5c8]
/*MXC_DCD_ITEM(23, IOMUXC_BASE_ADDR + 0x5b0, 0x00020000)*/
	ldr r1, =0x00020000
	str r1, [r0, #0x5b0]
/*MXC_DCD_ITEM(24, IOMUXC_BASE_ADDR + 0x5b4, 0x00000000)*/
	ldr r1, =0x00000000
	str r1, [r0, #0x5b4]
/*MXC_DCD_ITEM(25, IOMUXC_BASE_ADDR + 0x5c0, 0x00020000)*/
	ldr r1, =0x00020000
	str r1, [r0, #0x5c0]
/*MXC_DCD_ITEM(26, IOMUXC_BASE_ADDR + 0x5d0, 0x00080000)*/
	ldr r1, =0x00080000
	str r1, [r0, #0x5d0]

/*
 * MMDC Configuration
 */
	ldr r0, =MMDC_P0_BASE_ADDR
/*MXC_DCD_ITEM(27, MMDC_P0_BASE_ADDR + 0x01c, 0x00008000)*/
	ldr r1, =0x00008000
	str r1, [r0, #0x01c]
/*MXC_DCD_ITEM(28, MMDC_P0_BASE_ADDR + 0x85c, 0x1b4700c7)*/
	ldr r1, =0x1b4700c7
	str r1, [r0, #0x85c]
/*MXC_DCD_ITEM(29, MMDC_P0_BASE_ADDR + 0x800, 0xa1390003)*/
	ldr r1, =0xa1390003
	str r1, [r0, #0x800]
/*MXC_DCD_ITEM(30, MMDC_P0_BASE_ADDR + 0x890, 0x00300000)*/
	ldr r1, =0x00300000
	str r1, [r0, #0x890]
/*MXC_DCD_ITEM(31, MMDC_P0_BASE_ADDR + 0x8b8, 0x00000800)*/
	ldr r1, =0x00000800
	str r1, [r0, #0x8b8]
/*MXC_DCD_ITEM(32, MMDC_P0_BASE_ADDR + 0x81c, 0x33333333)*/
	ldr r1, =0x33333333
	str r1, [r0, #0x81c]
/*MXC_DCD_ITEM(33, MMDC_P0_BASE_ADDR + 0x820, 0x33333333)*/
	ldr r1, =0x33333333
	str r1, [r0, #0x820]
/*MXC_DCD_ITEM(34, MMDC_P0_BASE_ADDR + 0x824, 0x33333333)*/
	ldr r1, =0x33333333
	str r1, [r0, #0x824]
/*MXC_DCD_ITEM(35, MMDC_P0_BASE_ADDR + 0x828, 0x33333333)*/
	ldr r1, =0x33333333
	str r1, [r0, #0x828]
/*MXC_DCD_ITEM(36, MMDC_P0_BASE_ADDR + 0x82c, 0xf3333333)*/
	ldr r1, =0xf3333333
	str r1, [r0, #0x82c]
/*MXC_DCD_ITEM(37, MMDC_P0_BASE_ADDR + 0x830, 0xf3333333)*/
	ldr r1, =0xf3333333
	str r1, [r0, #0x830]
/*MXC_DCD_ITEM(38, MMDC_P0_BASE_ADDR + 0x834, 0xf3333333)*/
	ldr r1, =0xf3333333
	str r1, [r0, #0x834]
/*MXC_DCD_ITEM(39, MMDC_P0_BASE_ADDR + 0x838, 0xf3333333)*/
	ldr r1, =0xf3333333
	str r1, [r0, #0x838]
/*MXC_DCD_ITEM(40, MMDC_P0_BASE_ADDR + 0x848, 0x4241444a)*/
	ldr r1, =0x4241444a
	str r1, [r0, #0x848]
/*MXC_DCD_ITEM(41, MMDC_P0_BASE_ADDR + 0x850, 0x3030312b)*/
	ldr r1, =0x3030312b
	str r1, [r0, #0x850]
/*MXC_DCD_ITEM(42, MMDC_P0_BASE_ADDR + 0x83c, 0x20000000)*/
	ldr r1, =0x20000000
	str r1, [r0, #0x83c]
/*MXC_DCD_ITEM(43, MMDC_P0_BASE_ADDR + 0x840, 0x00000000)*/
	ldr r1, =0x00000000
	str r1, [r0, #0x840]
/*MXC_DCD_ITEM(44, MMDC_P0_BASE_ADDR + 0x8c0, 0x24911492)*/
	ldr r1, =0x24911492
	str r1, [r0, #0x8c0]
/*MXC_DCD_ITEM(45, MMDC_P0_BASE_ADDR + 0x8b8, 0x00000800)*/
	ldr r1, =0x00000800
	str r1, [r0, #0x8b8]
/*MXC_DCD_ITEM(46, MMDC_P0_BASE_ADDR + 0x00c, 0x33374133)*/
	ldr r1, =0x33374133
	str r1, [r0, #0x00c]
/*MXC_DCD_ITEM(47, MMDC_P0_BASE_ADDR + 0x004, 0x00020024)*/
	ldr r1, =0x00020024
	str r1, [r0, #0x004]
/*MXC_DCD_ITEM(48, MMDC_P0_BASE_ADDR + 0x010, 0x00100A82)*/
	ldr r1, =0x00100A82
	str r1, [r0, #0x010]
/*MXC_DCD_ITEM(49, MMDC_P0_BASE_ADDR + 0x014, 0x00000093)*/
	ldr r1, =0x00000093
	str r1, [r0, #0x014]
/*MXC_DCD_ITEM(50, MMDC_P0_BASE_ADDR + 0x018, 0x00001688)*/
	ldr r1, =0x00001688
	str r1, [r0, #0x018]
/*MXC_DCD_ITEM(51, MMDC_P0_BASE_ADDR + 0x02c, 0x0f9f26d2)*/
	ldr r1, =0x0f9f26d2
	str r1, [r0, #0x02c]
/*MXC_DCD_ITEM(52, MMDC_P0_BASE_ADDR + 0x030, 0x0000020e)*/
	ldr r1, =0x0000020e
	str r1, [r0, #0x030]
/*MXC_DCD_ITEM(53, MMDC_P0_BASE_ADDR + 0x038, 0x00190778)*/
	ldr r1, =0x00190778
	str r1, [r0, #0x038]
/*MXC_DCD_ITEM(54, MMDC_P0_BASE_ADDR + 0x008, 0x00000000)*/
	ldr r1, =0x00000000
	str r1, [r0, #0x008]
/*MXC_DCD_ITEM(55, MMDC_P0_BASE_ADDR + 0x040, 0x0000004f)*/
	ldr r1, =0x0000004f
	str r1, [r0, #0x040]
/*MXC_DCD_ITEM(56, MMDC_P0_BASE_ADDR + 0x000, 0xc3110000)*/
	ldr r1, =0xc3110000
	str r1, [r0, #0x000]
/*MXC_DCD_ITEM(57, MMDC_P0_BASE_ADDR + 0x01c, 0x003f8030)*/
	ldr r1, =0x003f8030
	str r1, [r0, #0x01c]
/*MXC_DCD_ITEM(58, MMDC_P0_BASE_ADDR + 0x01c, 0xff0a8030)*/
	ldr r1, =0xff0a8030
	str r1, [r0, #0x01c]
/*MXC_DCD_ITEM(59, MMDC_P0_BASE_ADDR + 0x01c, 0x82018030)*/
	ldr r1, =0x82018030
	str r1, [r0, #0x01c]
/*MXC_DCD_ITEM(60, MMDC_P0_BASE_ADDR + 0x01c, 0x04028030)*/
	ldr r1, =0x04028030
	str r1, [r0, #0x01c]
/*MXC_DCD_ITEM(61, MMDC_P0_BASE_ADDR + 0x01c, 0x02038030)*/
	ldr r1, =0x02038030
	str r1, [r0, #0x01c]
/*MXC_DCD_ITEM(62, MMDC_P0_BASE_ADDR + 0x01c, 0xff0a8038)*/
	ldr r1, =0xff0a8038
	str r1, [r0, #0x01c]
/*MXC_DCD_ITEM(63, MMDC_P0_BASE_ADDR + 0x01c, 0x82018038)*/
	ldr r1, =0x82018038
	str r1, [r0, #0x01c]
/*MXC_DCD_ITEM(64, MMDC_P0_BASE_ADDR + 0x01c, 0x04028038)*/
	ldr r1, =0x04028038
	str r1, [r0, #0x01c]
/*MXC_DCD_ITEM(65, MMDC_P0_BASE_ADDR + 0x01c, 0x02038038)*/
	ldr r1, =0x02038038
	str r1, [r0, #0x01c]
/*MXC_DCD_ITEM(66, MMDC_P0_BASE_ADDR + 0x800, 0xa1310003)*/
	ldr r1, =0xa1310003
	str r1, [r0, #0x800]
/*MXC_DCD_ITEM(67, MMDC_P0_BASE_ADDR + 0x020, 0x00001800)*/
	ldr r1, =0x00001800
	str r1, [r0, #0x020]
/*MXC_DCD_ITEM(68, MMDC_P0_BASE_ADDR + 0x818, 0x00000000)*/
	ldr r1, =0x00000000
	str r1, [r0, #0x818]
/*MXC_DCD_ITEM(69, MMDC_P0_BASE_ADDR + 0x8b8, 0x00000800)*/
	ldr r1, =0x00000800
	str r1, [r0, #0x8b8]
/*MXC_DCD_ITEM(70, MMDC_P0_BASE_ADDR + 0x004, 0x00025564)*/
	ldr r1, =0x00025564
	str r1, [r0, #0x004]
/*MXC_DCD_ITEM(71, MMDC_P0_BASE_ADDR + 0x404, 0x00011006)*/
	ldr r1, =0x00011006
	str r1, [r0, #0x404]
/*MXC_DCD_ITEM(72, MMDC_P0_BASE_ADDR + 0x01c, 0x00000000)*/
	ldr r1, =0x00000000
	str r1, [r0, #0x01c]

/*
 * End of LPDDR init script
 */

/*
    The following is to fill in those arguments for this ROM function
    pu_irom_hwcnfg_setup(void **start, size_t *bytes, const void *boot_data)

    This function is used to copy data from the storage media into DDR.

    start - Initial (possibly partial) image load address on entry.  Final image load address on exit.
    bytes - Initial (possibly partial) image size on entry.  Final image size on exit.
    boot_data - Initial @ref ivt Boot Data load address.
*/

    adr r0, DDR_DEST_ADDR
    adr r1, COPY_SIZE
    adr r2, BOOT_DATA

/*
 * check the _pu_irom_api_table for the address
 * pu_irom_hwcnfg_setup is in 0x1f20
 */
before_calling_rom___pu_irom_hwcnfg_setup:
    mov r4, #0x1f00
    add r4, r4, #0x21
    blx r4	/* This address might change in future ROM versions */
after_calling_rom___pu_irom_hwcnfg_setup:

/* To return to ROM from plugin, we need to fill in these argument.
 * Here is what need to do:
 * Need to construct the paramters for this function before return to ROM:
 * plugin_download(void **start, size_t *bytes, UINT32 *ivt_offset)
 */
    pop {r0-r4, lr}
    ldr r5, DDR_DEST_ADDR
    str r5, [r0]
    ldr r5, COPY_SIZE
    str r5, [r1]
    mov r5, #0x400  /* Point to the second IVT table at offset 0x42C */
    add r5, r5, #0x2C
    str r5, [r2]
    mov r0, #1

    bx lr          /* return back to ROM code */

DDR_DEST_ADDR:    .word   TEXT_BASE
COPY_SIZE:        .word   _end_of_copy  - TEXT_BASE + CONFIG_FLASH_HEADER_OFFSET
BOOT_DATA:        .word   TEXT_BASE
                  .word   _end_of_copy  - TEXT_BASE + CONFIG_FLASH_HEADER_OFFSET
                  .word   0
#endif	/* CONFIG_FLASH_PLUG_IN */

#endif  /* CONFIG_FLASH_HEADER*/
