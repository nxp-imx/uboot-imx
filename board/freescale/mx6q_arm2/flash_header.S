/*
 * Copyright (C) 2010-2011 Freescale Semiconductor, Inc.
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
app_code_csf:     .word 0x0
reserv2:          .word 0x0

boot_data:        .word TEXT_BASE
image_len:        .word _end_of_copy  - TEXT_BASE + CONFIG_FLASH_HEADER_OFFSET
plugin:           .word 0x0

#ifdef CONFIG_LPDDR2

dcd_hdr:          .word 0x400804D2 /* Tag=0xD2, Len=128*8 + 4 + 4, Ver=0x40 */
write_dcd_cmd:    .word 0x040404CC /* Tag=0xCC, Len=128*8 + 4, Param=0x04 */

/* DCD */
MXC_DCD_ITEM(1, CCM_BASE_ADDR + 0x18, 0x60324)

MXC_DCD_ITEM(2, IOMUXC_BASE_ADDR + 0x5a8, 0x00003038)
MXC_DCD_ITEM(3, IOMUXC_BASE_ADDR + 0x5b0, 0x00003038)
MXC_DCD_ITEM(4, IOMUXC_BASE_ADDR + 0x524, 0x00003038)
MXC_DCD_ITEM(5, IOMUXC_BASE_ADDR + 0x51c, 0x00003038)

MXC_DCD_ITEM(6, IOMUXC_BASE_ADDR + 0x518, 0x00003038)
MXC_DCD_ITEM(7, IOMUXC_BASE_ADDR + 0x50c, 0x00003038)
MXC_DCD_ITEM(8, IOMUXC_BASE_ADDR + 0x5b8, 0x00003038)
MXC_DCD_ITEM(9, IOMUXC_BASE_ADDR + 0x5c0, 0x00003038)

MXC_DCD_ITEM(10, IOMUXC_BASE_ADDR + 0x5ac, 0x00000038)
MXC_DCD_ITEM(11, IOMUXC_BASE_ADDR + 0x5b4, 0x00000038)
MXC_DCD_ITEM(12, IOMUXC_BASE_ADDR + 0x528, 0x00000038)
MXC_DCD_ITEM(13, IOMUXC_BASE_ADDR + 0x520, 0x00000038)

MXC_DCD_ITEM(14, IOMUXC_BASE_ADDR + 0x514, 0x00000038)
MXC_DCD_ITEM(15, IOMUXC_BASE_ADDR + 0x510, 0x00000038)
MXC_DCD_ITEM(16, IOMUXC_BASE_ADDR + 0x5bc, 0x00000038)
MXC_DCD_ITEM(17, IOMUXC_BASE_ADDR + 0x5c4, 0x00000038)

MXC_DCD_ITEM(18, IOMUXC_BASE_ADDR + 0x56c, 0x00000038)
MXC_DCD_ITEM(19, IOMUXC_BASE_ADDR + 0x578, 0x00000038)
MXC_DCD_ITEM(20, IOMUXC_BASE_ADDR + 0x588, 0x00000038)
MXC_DCD_ITEM(21, IOMUXC_BASE_ADDR + 0x594, 0x00000038)

MXC_DCD_ITEM(22, IOMUXC_BASE_ADDR + 0x57c, 0x00000038)
MXC_DCD_ITEM(23, IOMUXC_BASE_ADDR + 0x590, 0x00000038)
MXC_DCD_ITEM(24, IOMUXC_BASE_ADDR + 0x598, 0x00000038)
MXC_DCD_ITEM(25, IOMUXC_BASE_ADDR + 0x58c, 0x00000000)

MXC_DCD_ITEM(26, IOMUXC_BASE_ADDR + 0x59c, 0x00000038)
MXC_DCD_ITEM(27, IOMUXC_BASE_ADDR + 0x5a0, 0x00000038)
MXC_DCD_ITEM(28, IOMUXC_BASE_ADDR + 0x784, 0x00000038)
MXC_DCD_ITEM(29, IOMUXC_BASE_ADDR + 0x788, 0x00000038)

MXC_DCD_ITEM(30, IOMUXC_BASE_ADDR + 0x794, 0x00000038)
MXC_DCD_ITEM(31, IOMUXC_BASE_ADDR + 0x79c, 0x00000038)
MXC_DCD_ITEM(32, IOMUXC_BASE_ADDR + 0x7a0, 0x00000038)
MXC_DCD_ITEM(33, IOMUXC_BASE_ADDR + 0x7a4, 0x00000038)

MXC_DCD_ITEM(34, IOMUXC_BASE_ADDR + 0x7a8, 0x00000038)
MXC_DCD_ITEM(35, IOMUXC_BASE_ADDR + 0x748, 0x00000038)
MXC_DCD_ITEM(36, IOMUXC_BASE_ADDR + 0x74c, 0x00000038)
MXC_DCD_ITEM(37, IOMUXC_BASE_ADDR + 0x750, 0x00020000)

MXC_DCD_ITEM(38, IOMUXC_BASE_ADDR + 0x758, 0x00000000)
MXC_DCD_ITEM(39, IOMUXC_BASE_ADDR + 0x774, 0x00020000)
MXC_DCD_ITEM(40, IOMUXC_BASE_ADDR + 0x78c, 0x00000038)
MXC_DCD_ITEM(41, IOMUXC_BASE_ADDR + 0x798, 0x00080000)

MXC_DCD_ITEM(42, MMDC_P0_BASE_ADDR + 0x01c, 0x00008000)
MXC_DCD_ITEM(43, MMDC_P1_BASE_ADDR + 0x01c, 0x00008000)

MXC_DCD_ITEM(44, MMDC_P0_BASE_ADDR + 0x85c, 0x1b5f01ff)
MXC_DCD_ITEM(45, MMDC_P1_BASE_ADDR + 0x85c, 0x1b5f01ff)

MXC_DCD_ITEM(46, MMDC_P0_BASE_ADDR + 0x800, 0xa1390000)
MXC_DCD_ITEM(47, MMDC_P1_BASE_ADDR + 0x800, 0xa1390000)

MXC_DCD_ITEM(48, MMDC_P0_BASE_ADDR + 0x890, 0x00400000)
MXC_DCD_ITEM(49, MMDC_P1_BASE_ADDR + 0x890, 0x00400000)

MXC_DCD_ITEM(50, MMDC_P1_BASE_ADDR + 0x8bc, 0x00055555)

MXC_DCD_ITEM(51, MMDC_P0_BASE_ADDR + 0x8b8, 0x00000800)
MXC_DCD_ITEM(52, MMDC_P1_BASE_ADDR + 0x8b8, 0x00000800)

MXC_DCD_ITEM(53, MMDC_P0_BASE_ADDR + 0x81c, 0x33333333)
MXC_DCD_ITEM(54, MMDC_P0_BASE_ADDR + 0x820, 0x33333333)
MXC_DCD_ITEM(55, MMDC_P0_BASE_ADDR + 0x824, 0x33333333)
MXC_DCD_ITEM(56, MMDC_P0_BASE_ADDR + 0x828, 0x33333333)
MXC_DCD_ITEM(57, MMDC_P1_BASE_ADDR + 0x81c, 0x33333333)
MXC_DCD_ITEM(58, MMDC_P1_BASE_ADDR + 0x820, 0x33333333)
MXC_DCD_ITEM(59, MMDC_P1_BASE_ADDR + 0x824, 0x33333333)
MXC_DCD_ITEM(60, MMDC_P1_BASE_ADDR + 0x828, 0x33333333)

MXC_DCD_ITEM(61, MMDC_P0_BASE_ADDR + 0x82c, 0xf3333333)
MXC_DCD_ITEM(62, MMDC_P0_BASE_ADDR + 0x830, 0xf3333333)
MXC_DCD_ITEM(63, MMDC_P0_BASE_ADDR + 0x834, 0xf3333333)
MXC_DCD_ITEM(64, MMDC_P0_BASE_ADDR + 0x838, 0xf3333333)
MXC_DCD_ITEM(65, MMDC_P1_BASE_ADDR + 0x82c, 0xf3333333)
MXC_DCD_ITEM(66, MMDC_P1_BASE_ADDR + 0x830, 0xf3333333)
MXC_DCD_ITEM(67, MMDC_P1_BASE_ADDR + 0x834, 0xf3333333)
MXC_DCD_ITEM(68, MMDC_P1_BASE_ADDR + 0x838, 0xf3333333)

MXC_DCD_ITEM(69, MMDC_P0_BASE_ADDR + 0x848, 0x49383b39)
MXC_DCD_ITEM(70, MMDC_P0_BASE_ADDR + 0x850, 0x30364738)
MXC_DCD_ITEM(71, MMDC_P1_BASE_ADDR + 0x848, 0x3e3c3846)
MXC_DCD_ITEM(72, MMDC_P1_BASE_ADDR + 0x850, 0x4c294b35)

MXC_DCD_ITEM(73, MMDC_P0_BASE_ADDR + 0x83c, 0x20000000)
MXC_DCD_ITEM(74, MMDC_P0_BASE_ADDR + 0x840, 0x0)
MXC_DCD_ITEM(75, MMDC_P1_BASE_ADDR + 0x83c, 0x20000000)
MXC_DCD_ITEM(76, MMDC_P1_BASE_ADDR + 0x840, 0x0)

MXC_DCD_ITEM(77, MMDC_P0_BASE_ADDR + 0x858, 0xf00)
MXC_DCD_ITEM(78, MMDC_P1_BASE_ADDR + 0x858, 0xf00)

MXC_DCD_ITEM(79, MMDC_P0_BASE_ADDR + 0x8b8, 0x800)
MXC_DCD_ITEM(80, MMDC_P1_BASE_ADDR + 0x8b8, 0x800)

MXC_DCD_ITEM(81, MMDC_P0_BASE_ADDR + 0xc, 0x555a61a5)
MXC_DCD_ITEM(82, MMDC_P0_BASE_ADDR + 0x4, 0x20036)
MXC_DCD_ITEM(83, MMDC_P0_BASE_ADDR + 0x10, 0x160e83)
MXC_DCD_ITEM(84, MMDC_P0_BASE_ADDR + 0x14, 0xdd)
MXC_DCD_ITEM(85, MMDC_P0_BASE_ADDR + 0x18, 0x8174c)
MXC_DCD_ITEM(86, MMDC_P0_BASE_ADDR + 0x2c, 0xf9f26d2)
MXC_DCD_ITEM(87, MMDC_P0_BASE_ADDR + 0x30, 0x20e)
MXC_DCD_ITEM(88, MMDC_P0_BASE_ADDR + 0x38, 0x200aac)
MXC_DCD_ITEM(89, MMDC_P0_BASE_ADDR + 0x8, 0x0)

MXC_DCD_ITEM(90, MMDC_P0_BASE_ADDR + 0x40, 0x5f)

MXC_DCD_ITEM(91, MMDC_P0_BASE_ADDR + 0x0, 0xc3010000)

MXC_DCD_ITEM(92, MMDC_P1_BASE_ADDR + 0xc, 0x555a61a5)
MXC_DCD_ITEM(93, MMDC_P1_BASE_ADDR + 0x4, 0x20036)
MXC_DCD_ITEM(94, MMDC_P1_BASE_ADDR + 0x10, 0x160e83)
MXC_DCD_ITEM(95, MMDC_P1_BASE_ADDR + 0x14, 0xdd)
MXC_DCD_ITEM(96, MMDC_P1_BASE_ADDR + 0x18, 0x8174c)
MXC_DCD_ITEM(97, MMDC_P1_BASE_ADDR + 0x2c, 0xf9f26d2)
MXC_DCD_ITEM(98, MMDC_P1_BASE_ADDR + 0x30, 0x20e)
MXC_DCD_ITEM(99, MMDC_P1_BASE_ADDR + 0x38, 0x200aac)
MXC_DCD_ITEM(100, MMDC_P1_BASE_ADDR + 0x8, 0x0)

MXC_DCD_ITEM(101, MMDC_P1_BASE_ADDR + 0x40, 0x3f)
MXC_DCD_ITEM(102, MMDC_P1_BASE_ADDR + 0x0, 0xc3010000)

MXC_DCD_ITEM(103, MMDC_P0_BASE_ADDR + 0x1c, 0x3f8030)
MXC_DCD_ITEM(104, MMDC_P0_BASE_ADDR + 0x1c, 0xff0a8030)
MXC_DCD_ITEM(105, MMDC_P0_BASE_ADDR + 0x1c, 0xc2018030)
MXC_DCD_ITEM(106, MMDC_P0_BASE_ADDR + 0x1c, 0x6028030)
MXC_DCD_ITEM(107, MMDC_P0_BASE_ADDR + 0x1c, 0x2038030)

MXC_DCD_ITEM(108, MMDC_P1_BASE_ADDR + 0x1c, 0x3f8030)
MXC_DCD_ITEM(109, MMDC_P1_BASE_ADDR + 0x1c, 0xff0a8030)
MXC_DCD_ITEM(110, MMDC_P1_BASE_ADDR + 0x1c, 0xc2018030)
MXC_DCD_ITEM(111, MMDC_P1_BASE_ADDR + 0x1c, 0x6028030)
MXC_DCD_ITEM(112, MMDC_P1_BASE_ADDR + 0x1c, 0x2038030)

MXC_DCD_ITEM(113, MMDC_P0_BASE_ADDR + 0x800, 0xa1390003)
MXC_DCD_ITEM(114, MMDC_P1_BASE_ADDR + 0x800, 0xa1390003)

MXC_DCD_ITEM(115, MMDC_P0_BASE_ADDR + 0x20, 0x7800)
MXC_DCD_ITEM(116, MMDC_P1_BASE_ADDR + 0x20, 0x7800)

MXC_DCD_ITEM(117, MMDC_P0_BASE_ADDR + 0x818, 0x0)
MXC_DCD_ITEM(118, MMDC_P1_BASE_ADDR + 0x818, 0x0)

MXC_DCD_ITEM(119, MMDC_P0_BASE_ADDR + 0x800, 0xa1390003)
MXC_DCD_ITEM(120, MMDC_P1_BASE_ADDR + 0x800, 0xa1390003)

MXC_DCD_ITEM(121, MMDC_P0_BASE_ADDR + 0x8b8, 0x800)
MXC_DCD_ITEM(122, MMDC_P1_BASE_ADDR + 0x8b8, 0x800)

MXC_DCD_ITEM(123, MMDC_P0_BASE_ADDR + 0x1c, 0x0)
MXC_DCD_ITEM(124, MMDC_P1_BASE_ADDR + 0x1c, 0x0)

MXC_DCD_ITEM(125, MMDC_P0_BASE_ADDR + 0x404, 0x00011006)

/* enable AXI cache for VDOA/VPU/IPU */
MXC_DCD_ITEM(126, IOMUXC_BASE_ADDR + 0x010, 0xf00000ff)
/* set IPU AXI-id0 Qos=0xf(bypass) AXI-id1 Qos=0x7 */
MXC_DCD_ITEM(127, IOMUXC_BASE_ADDR + 0x018, 0x007f007f)
MXC_DCD_ITEM(128, IOMUXC_BASE_ADDR + 0x01c, 0x007f007f)

#else

dcd_hdr:          .word 0x40F002D2 /* Tag=0xD2, Len=93*8 + 4 + 4, Ver=0x40 */
write_dcd_cmd:    .word 0x04EC02CC /* Tag=0xCC, Len=93*8 + 4, Param=0x04 */

/* DCD */

MXC_DCD_ITEM(1, IOMUXC_BASE_ADDR + 0x5a8, 0x00000030)
MXC_DCD_ITEM(2, IOMUXC_BASE_ADDR + 0x5b0, 0x00000030)
MXC_DCD_ITEM(3, IOMUXC_BASE_ADDR + 0x524, 0x00000030)
MXC_DCD_ITEM(4, IOMUXC_BASE_ADDR + 0x51c, 0x00000030)

MXC_DCD_ITEM(5, IOMUXC_BASE_ADDR + 0x518, 0x00000030)
MXC_DCD_ITEM(6, IOMUXC_BASE_ADDR + 0x50c, 0x00000030)
MXC_DCD_ITEM(7, IOMUXC_BASE_ADDR + 0x5b8, 0x00000030)
MXC_DCD_ITEM(8, IOMUXC_BASE_ADDR + 0x5c0, 0x00000030)

MXC_DCD_ITEM(9, IOMUXC_BASE_ADDR + 0x5ac, 0x00020030)
MXC_DCD_ITEM(10, IOMUXC_BASE_ADDR + 0x5b4, 0x00020030)
MXC_DCD_ITEM(11, IOMUXC_BASE_ADDR + 0x528, 0x00020030)
MXC_DCD_ITEM(12, IOMUXC_BASE_ADDR + 0x520, 0x00020030)

MXC_DCD_ITEM(13, IOMUXC_BASE_ADDR + 0x514, 0x00020030)
MXC_DCD_ITEM(14, IOMUXC_BASE_ADDR + 0x510, 0x00020030)
MXC_DCD_ITEM(15, IOMUXC_BASE_ADDR + 0x5bc, 0x00020030)
MXC_DCD_ITEM(16, IOMUXC_BASE_ADDR + 0x5c4, 0x00020030)

MXC_DCD_ITEM(17, IOMUXC_BASE_ADDR + 0x56c, 0x00020030)
MXC_DCD_ITEM(18, IOMUXC_BASE_ADDR + 0x578, 0x00020030)
MXC_DCD_ITEM(19, IOMUXC_BASE_ADDR + 0x588, 0x00020030)
MXC_DCD_ITEM(20, IOMUXC_BASE_ADDR + 0x594, 0x00020030)

MXC_DCD_ITEM(21, IOMUXC_BASE_ADDR + 0x57c, 0x00020030)
MXC_DCD_ITEM(22, IOMUXC_BASE_ADDR + 0x590, 0x00003000)
MXC_DCD_ITEM(23, IOMUXC_BASE_ADDR + 0x598, 0x00003000)
MXC_DCD_ITEM(24, IOMUXC_BASE_ADDR + 0x58c, 0x00000000)

MXC_DCD_ITEM(25, IOMUXC_BASE_ADDR + 0x59c, 0x00003030)
MXC_DCD_ITEM(26, IOMUXC_BASE_ADDR + 0x5a0, 0x00003030)
MXC_DCD_ITEM(27, IOMUXC_BASE_ADDR + 0x784, 0x00000030)
MXC_DCD_ITEM(28, IOMUXC_BASE_ADDR + 0x788, 0x00000030)

MXC_DCD_ITEM(29, IOMUXC_BASE_ADDR + 0x794, 0x00000030)
MXC_DCD_ITEM(30, IOMUXC_BASE_ADDR + 0x79c, 0x00000030)
MXC_DCD_ITEM(31, IOMUXC_BASE_ADDR + 0x7a0, 0x00000030)
MXC_DCD_ITEM(32, IOMUXC_BASE_ADDR + 0x7a4, 0x00000030)

MXC_DCD_ITEM(33, IOMUXC_BASE_ADDR + 0x7a8, 0x00000030)
MXC_DCD_ITEM(34, IOMUXC_BASE_ADDR + 0x748, 0x00000030)
MXC_DCD_ITEM(35, IOMUXC_BASE_ADDR + 0x74c, 0x00000030)
MXC_DCD_ITEM(36, IOMUXC_BASE_ADDR + 0x750, 0x00020000)

MXC_DCD_ITEM(37, IOMUXC_BASE_ADDR + 0x758, 0x00000000)
MXC_DCD_ITEM(38, IOMUXC_BASE_ADDR + 0x774, 0x00020000)
MXC_DCD_ITEM(39, IOMUXC_BASE_ADDR + 0x78c, 0x00000030)
MXC_DCD_ITEM(40, IOMUXC_BASE_ADDR + 0x798, 0x000C0000)

MXC_DCD_ITEM(41, MMDC_P0_BASE_ADDR + 0x81c, 0x33333333)
MXC_DCD_ITEM(42, MMDC_P0_BASE_ADDR + 0x820, 0x33333333)
MXC_DCD_ITEM(43, MMDC_P0_BASE_ADDR + 0x824, 0x33333333)
MXC_DCD_ITEM(44, MMDC_P0_BASE_ADDR + 0x828, 0x33333333)

MXC_DCD_ITEM(45, MMDC_P1_BASE_ADDR + 0x81c, 0x33333333)
MXC_DCD_ITEM(46, MMDC_P1_BASE_ADDR + 0x820, 0x33333333)
MXC_DCD_ITEM(47, MMDC_P1_BASE_ADDR + 0x824, 0x33333333)
MXC_DCD_ITEM(48, MMDC_P1_BASE_ADDR + 0x828, 0x33333333)

MXC_DCD_ITEM(49, MMDC_P0_BASE_ADDR + 0x018, 0x00081740)

MXC_DCD_ITEM(50, MMDC_P0_BASE_ADDR + 0x01c, 0x00008000)
MXC_DCD_ITEM(51, MMDC_P0_BASE_ADDR + 0x00c, 0x555A7975)
MXC_DCD_ITEM(52, MMDC_P0_BASE_ADDR + 0x010, 0xFF538E64)
MXC_DCD_ITEM(53, MMDC_P0_BASE_ADDR + 0x014, 0x01FF00DB)
MXC_DCD_ITEM(54, MMDC_P0_BASE_ADDR + 0x02c, 0x000026D2)

MXC_DCD_ITEM(55, MMDC_P0_BASE_ADDR + 0x030, 0x005B0E21)
MXC_DCD_ITEM(56, MMDC_P0_BASE_ADDR + 0x008, 0x09444040)
MXC_DCD_ITEM(57, MMDC_P0_BASE_ADDR + 0x004, 0x00025576)
MXC_DCD_ITEM(58, MMDC_P0_BASE_ADDR + 0x040, 0x00000027)
MXC_DCD_ITEM(59, MMDC_P0_BASE_ADDR + 0x000, 0xC31A0000)

MXC_DCD_ITEM(60, MMDC_P0_BASE_ADDR + 0x01c, 0x04088032)
MXC_DCD_ITEM(61, MMDC_P0_BASE_ADDR + 0x01c, 0x0408803A)
MXC_DCD_ITEM(62, MMDC_P0_BASE_ADDR + 0x01c, 0x00008033)
MXC_DCD_ITEM(63, MMDC_P0_BASE_ADDR + 0x01c, 0x0000803B)
MXC_DCD_ITEM(64, MMDC_P0_BASE_ADDR + 0x01c, 0x00428031)
MXC_DCD_ITEM(65, MMDC_P0_BASE_ADDR + 0x01c, 0x00428039)
MXC_DCD_ITEM(66, MMDC_P0_BASE_ADDR + 0x01c, 0x09408030)
MXC_DCD_ITEM(67, MMDC_P0_BASE_ADDR + 0x01c, 0x09408038)

MXC_DCD_ITEM(68, MMDC_P0_BASE_ADDR + 0x01c, 0x04008040)
MXC_DCD_ITEM(69, MMDC_P0_BASE_ADDR + 0x01c, 0x04008048)
MXC_DCD_ITEM(70, MMDC_P0_BASE_ADDR + 0x800, 0xA1380003)
MXC_DCD_ITEM(71, MMDC_P1_BASE_ADDR + 0x800, 0xA1380003)
MXC_DCD_ITEM(72, MMDC_P0_BASE_ADDR + 0x020, 0x00005800)
MXC_DCD_ITEM(73, MMDC_P0_BASE_ADDR + 0x818, 0x00022227)
MXC_DCD_ITEM(74, MMDC_P1_BASE_ADDR + 0x818, 0x00022227)

MXC_DCD_ITEM(75, MMDC_P0_BASE_ADDR + 0x83c, 0x434B0350)
MXC_DCD_ITEM(76, MMDC_P0_BASE_ADDR + 0x840, 0x034C0359)
MXC_DCD_ITEM(77, MMDC_P1_BASE_ADDR + 0x83c, 0x434B0350)
MXC_DCD_ITEM(78, MMDC_P1_BASE_ADDR + 0x840, 0x03650348)
MXC_DCD_ITEM(79, MMDC_P0_BASE_ADDR + 0x848, 0x4436383B)
MXC_DCD_ITEM(80, MMDC_P1_BASE_ADDR + 0x848, 0x39393341)
MXC_DCD_ITEM(81, MMDC_P0_BASE_ADDR + 0x850, 0x35373933)
MXC_DCD_ITEM(82, MMDC_P1_BASE_ADDR + 0x850, 0x48254A36)

MXC_DCD_ITEM(83, MMDC_P0_BASE_ADDR + 0x80c, 0x001F001F)
MXC_DCD_ITEM(84, MMDC_P0_BASE_ADDR + 0x810, 0x001F001F)

MXC_DCD_ITEM(85, MMDC_P1_BASE_ADDR + 0x80c, 0x00440044)
MXC_DCD_ITEM(86, MMDC_P1_BASE_ADDR + 0x810, 0x00440044)

MXC_DCD_ITEM(87, MMDC_P0_BASE_ADDR + 0x8b8, 0x00000800)
MXC_DCD_ITEM(88, MMDC_P1_BASE_ADDR + 0x8b8, 0x00000800)

MXC_DCD_ITEM(89, MMDC_P0_BASE_ADDR + 0x01c, 0x00000000)
MXC_DCD_ITEM(90, MMDC_P0_BASE_ADDR + 0x404, 0x00011006)

/* enable AXI cache for VDOA/VPU/IPU */
MXC_DCD_ITEM(91, IOMUXC_BASE_ADDR + 0x010, 0xf00000ff)
/* set IPU AXI-id0 Qos=0xf(bypass) AXI-id1 Qos=0x7 */
MXC_DCD_ITEM(92, IOMUXC_BASE_ADDR + 0x018, 0x007f007f)
MXC_DCD_ITEM(93, IOMUXC_BASE_ADDR + 0x01c, 0x007f007f)
#endif


#else
/*****************PLUGIN IN mode********************/

/*DDR clock:480MHz, ipg clock:40MHz, AHB clock:80MHz*/
#define CONFIG_IPG_40M_FR_PLL3

.section ".text.flasheader", "x"
	b	_start
	.org	CONFIG_FLASH_HEADER_OFFSET

/* First IVT to copy the plugin that initializes the system into OCRAM */
ivt_header:             .long 0x402000D1    /*Tag=0xD1, Len=0x0020, Ver=0x40 */
app_code_jump_v:        .long 0x00907458    /* Plugin entry point, address after the second IVT table */
reserv1:                .long 0x0
dcd_ptr:                .long 0x0
boot_data_ptr:          .long 0x00907420
self_ptr:               .long 0x00907400
app_code_csf:           .long 0x0
reserv2:                .long 0x0
boot_data:              .long 0x00907000
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
 * Note: The DDR settings provided below are specific to Freescale development boards and are the latest settings at the time of release.
 * However, it is recommended to contact your Freescale representative in case there are any improvements to these settings.
 */

#ifdef CONFIG_IPG_40M_FR_PLL3
    /*select pll3 for ipg clk 40M */
    ldr r0, =CCM_BASE_ADDR
    ldr r1, [r0,#0x14]
    ldr r2, =0x2000000
    orr r1, r1, r2
    ldr r2, =0x1c00
    bic r1, r2
    ldr r2, =0x1400
    orr r1, r1, r2
    str r1, [r0,#0x14]

    /*enable pll3 */
    ldr r0, =ANATOP_BASE_ADDR
    ldr r1, =0x10000
    str r1, [r0,#0x28]
    ldr r1, =0x3040
    str r1, [r0,#0x24]
#endif

    /* Init the DDR according the init script */
    ldr r0, =CCM_BASE_ADDR
    /* select 528MHz for pre_periph_clk_sel */
    ldr r1, =0x00020324
    str r1, [r0,#0x18]

    /* IOMUX setting */
    ldr r0, =IOMUXC_BASE_ADDR
    mov r1, #0x30
    str r1, [r0,#0x5a8]
    str r1, [r0,#0x5b0]
    str r1, [r0,#0x524]
    str r1, [r0,#0x51c]
    str r1, [r0,#0x518]
    str r1, [r0,#0x50c]
    str r1, [r0,#0x5b8]
    str r1, [r0,#0x5c0]

    ldr r1, =0x00020030
    str r1, [r0,#0x5ac]
    str r1, [r0,#0x5b4]
    str r1, [r0,#0x528]
    str r1, [r0,#0x520]
    str r1, [r0,#0x514]
    str r1, [r0,#0x510]
    str r1, [r0,#0x5bc]
    str r1, [r0,#0x5c4]

    str r1, [r0,#0x56c]
    str r1, [r0,#0x578]
    str r1, [r0,#0x588]
    str r1, [r0,#0x594]
    str r1, [r0,#0x57c]

    ldr r1, =0x00003000
    str r1, [r0,#0x590]
    str r1, [r0,#0x598]
    mov r1, #0x00000000
    str r1, [r0,#0x58c]
    ldr r1, =0x00003030
    str r1, [r0,#0x59c]
    str r1, [r0,#0x5a0]

    ldr r1, =0x00000030
    str r1, [r0,#0x784]
    str r1, [r0,#0x788]
    str r1, [r0,#0x794]
    str r1, [r0,#0x79c]
    str r1, [r0,#0x7a0]
    str r1, [r0,#0x7a4]
    str r1, [r0,#0x7a8]
    str r1, [r0,#0x748]
    str r1, [r0,#0x74c]

    mov r1, #0x00020000
    str r1, [r0,#0x750]

    mov r1, #0x00000000
    str r1, [r0,#0x758]

    mov r1, #0x00020000
    str r1, [r0,#0x774]
    mov r1, #0x30
    str r1, [r0,#0x78c]
    mov r1, #0x000c0000
    str r1, [r0,#0x798]

    /* Initialize 2GB DDR3 - Micron MT41J128M */
    ldr r0, =MMDC_P0_BASE_ADDR
    ldr r2, =MMDC_P1_BASE_ADDR

    ldr r1, =0x33333333
    str r1, [r0,#0x81c]
    str r1, [r0,#0x820]
    str r1, [r0,#0x824]
    str r1, [r0,#0x828]
    str r1, [r2,#0x81c]
    str r1, [r2,#0x820]
    str r1, [r2,#0x824]
    str r1, [r2,#0x828]

    ldr r1, =0x00081740
    str r1, [r0,#0x18]
    ldr r1, =0x00008000
    str r1, [r0,#0x1c]
    ldr r1, =0x555a7975
    str r1, [r0,#0x0c]
    ldr r1, =0xff538e64
    str r1, [r0,#0x10]
    ldr r1, =0x01ff00db
    str r1, [r0,#0x14]

    ldr r1, =0x000026d2
    str r1, [r0,#0x2c]
    ldr r1, =0x005b0e21
    str r1, [r0,#0x30]
    ldr r1, =0x94444040
    str r1, [r0,#0x08]
    ldr r1, =0x00020036
    str r1, [r0,#0x04]
    ldr r1, =0x00000027
    str r1, [r0,#0x40]
    ldr r1, =0xc31a0000
    str r1, [r0,#0x00]

    ldr r1, =0x04088032
    str r1, [r0,#0x1c]
    ldr r1, =0x0408803a
    str r1, [r0,#0x1c]
    ldr r1, =0x00008033
    str r1, [r0,#0x1c]
    ldr r1, =0x0000803b
    str r1, [r0,#0x1c]
    ldr r1, =0x00428031
    str r1, [r0,#0x1c]
    ldr r1, =0x00428039
    str r1, [r0,#0x1c]

    ldr r1, =0x09408030
    str r1, [r0,#0x1c]
    ldr r1, =0x09408038
    str r1, [r0,#0x1c]
    ldr r1, =0x04008040
    str r1, [r0,#0x1c]
    ldr r1, =0x04008048
    str r1, [r0,#0x1c]

    ldr r1, =0xa5380003
    str r1, [r0,#0x800]
    ldr r1, =0xa5380003
    str r1, [r2,#0x800]

    ldr r1, =0x00005800
    str r1, [r0,#0x20]

    ldr r1, =0x00022227
    str r1, [r0,#0x818]
    ldr r1, =0x00022227
    str r1, [r2,#0x818]

    ldr r1, =0x433f033f
    str r1, [r0,#0x83c]

    ldr r1, =0x033f033f
    str r1, [r0,#0x840]

    ldr r1, =0x433f033f
    str r1, [r2,#0x83c]

    ldr r1, =0x0344033b
    str r1, [r2,#0x840]

    ldr r1, =0x4337373e
    str r1, [r0,#0x848]
    ldr r1, =0x3634303d
    str r1, [r2,#0x848]

    ldr r1, =0x35374640
    str r1, [r0,#0x850]
    ldr r1, =0x4a294b35
    str r1, [r2,#0x850]

    ldr r1, =0x001F001F
    str r1, [r0,#0x80c]
    ldr r1, =0x001F001F
    str r1, [r0,#0x810]

    ldr r1, =0x00440044
    str r1, [r2,#0x80c]
    ldr r1, =0x00440044
    str r1, [r2,#0x810]

    ldr r1, =0x00000800
    str r1, [r0,#0x8b8]
    ldr r1, =0x00000800
    str r1, [r2,#0x8b8]

    ldr r1, =0x00000000
    str r1, [r0,#0x1c]


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
 */
before_calling_rom___pu_irom_hwcnfg_setup:
    mov r4, #0x2000
    add r4, r4, #0xed
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
/*********************************************************************/
#endif

#endif
