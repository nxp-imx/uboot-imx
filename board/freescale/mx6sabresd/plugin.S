/*
 * Copyright (C) 2012-2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <config.h>

/* DDR script */
.macro imx6dqpsabresd_ddr_setting
	ldr r0, =IOMUXC_BASE_ADDR
	ldr r1, =0x000c0000
	str r1, [r0, #0x798]
	ldr r1, =0x00000000
	str r1, [r0, #0x758]

	ldr r1, =0x00000030
	str r1, [r0, #0x588]
	str r1, [r0, #0x594]
	str r1, [r0, #0x56c]
	str r1, [r0, #0x578]
	str r1, [r0, #0x74c]
	str r1, [r0, #0x57c]

	ldr r1, =0x00000000
	str r1, [r0, #0x58c]

	ldr r1, =0x00000030
	str r1, [r0, #0x59c]
	str r1, [r0, #0x5a0]
	str r1, [r0, #0x78c]

	ldr r1, =0x00020000
	str r1, [r0, #0x750]

	ldr r1, =0x00000030
	str r1, [r0, #0x5a8]
	str r1, [r0, #0x5b0]
	str r1, [r0, #0x524]
	str r1, [r0, #0x51c]
	str r1, [r0, #0x518]
	str r1, [r0, #0x50c]
	str r1, [r0, #0x5b8]
	str r1, [r0, #0x5c0]

	ldr r1, =0x00018200
	str r1, [r0, #0x534]
	ldr r1, =0x00008000
	str r1, [r0, #0x538]
	ldr r1, =0x00018200
	str r1, [r0, #0x53c]
	str r1, [r0, #0x540]
	str r1, [r0, #0x544]
	str r1, [r0, #0x548]
	str r1, [r0, #0x54c]
	str r1, [r0, #0x550]

	ldr r1, =0x00020000
	str r1, [r0, #0x774]

	ldr r1, =0x00000030
	str r1, [r0, #0x784]
	str r1, [r0, #0x788]
	str r1, [r0, #0x794]
	str r1, [r0, #0x79c]
	str r1, [r0, #0x7a0]
	str r1, [r0, #0x7a4]
	str r1, [r0, #0x7a8]
	str r1, [r0, #0x748]
	str r1, [r0, #0x5ac]
	str r1, [r0, #0x5b4]
	str r1, [r0, #0x528]
	str r1, [r0, #0x520]
	str r1, [r0, #0x514]
	str r1, [r0, #0x510]
	str r1, [r0, #0x5bc]
	str r1, [r0, #0x5c4]

	ldr r0, =MMDC_P0_BASE_ADDR
	ldr r2, =0xa1390003
	str r2, [r0, #0x800]

	ldr r2, =0x001b001e
	str r2, [r0, #0x80c]
	ldr r2, =0x002e0029
	str r2, [r0, #0x810]

	ldr r1, =MMDC_P1_BASE_ADDR
	ldr r2, =0x001b002a
	str r2, [r1, #0x80c]
	ldr r2, =0x0019002c
	str r2, [r1, #0x810]

	ldr r2, =0x43240334
	str r2, [r0, #0x83c]
	ldr r2, =0x0324031a
	str r2, [r0, #0x840]

	ldr r2, =0x43340344
	str r2, [r1, #0x83c]
	ldr r2, =0x03280276
	str r2, [r1, #0x840]

	ldr r2, =0x44383A3E
	str r2, [r0, #0x848]
	ldr r2, =0x3C3C3846
	str r2, [r1, #0x848]

	ldr r2, =0x2e303230
	str r2, [r0, #0x850]
	ldr r2, =0x38283E34
	str r2, [r1, #0x850]

	ldr r2, =0x33333333
	str r2, [r0, #0x81c]
	str r2, [r0, #0x820]
	str r2, [r0, #0x824]
	str r2, [r0, #0x828]
	str r2, [r1, #0x81c]
	str r2, [r1, #0x820]
	str r2, [r1, #0x824]
	str r2, [r1, #0x828]

	ldr r2, =0x24912489
	str r2, [r0, #0x8c0]
	ldr r2, =0x24914452
	str r2, [r1, #0x8c0]

	ldr r2, =0x00000800
	str r2, [r0, #0x8b8]
	str r2, [r1, #0x8b8]

	ldr r2, =0x00020036
	str r2, [r0, #0x004]
	ldr r2, =0x24444040
	str r2, [r0, #0x008]

	ldr r2, =0x555A7955
	str r2, [r0, #0x00c]
	ldr r2, =0xFF320F64
	str r2, [r0, #0x010]

	ldr r2, =0x01FF00DB
	str r2, [r0, #0x014]
	ldr r2, =0x00011740
	str r2, [r0, #0x018]

	ldr r2, =0x00008000
	str r2, [r0, #0x01c]
	ldr r2, =0x000026d2
	str r2, [r0, #0x02c]
	ldr r2, =0x005A1023
	str r2, [r0, #0x030]
	ldr r2, =0x00000027
	str r2, [r0, #0x040]

	ldr r2, =0x14420000
	str r2, [r0, #0x400]

	ldr r2, =0x831A0000
	str r2, [r0, #0x000]

	ldr r2, =0x00400C58
	str r2, [r0, #0x890]

	ldr r3, =0x00bb0000
	ldr r2, =0x00000000
	str r2, [r3, #0x008]
	ldr r2, =0x2891E41A
	str r2, [r3, #0x00C]
	ldr r2, =0x00000564
	str r2, [r3, #0x038]
	ldr r2, =0x00000040
	str r2, [r3, #0x014]
	ldr r2, =0x00000020
	str r2, [r3, #0x028]
	ldr r2, =0x00000020
	str r2, [r3, #0x02c]

	ldr r2, =0x04088032
	str r2, [r0, #0x01c]
	ldr r2, =0x00008033
	str r2, [r0, #0x01c]
	ldr r2, =0x00048031
	str r2, [r0, #0x01c]
	ldr r2, =0x09408030
	str r2, [r0, #0x01c]
	ldr r2, =0x04008040
	str r2, [r0, #0x01c]

	ldr r2, =0x00005800
	str r2, [r0, #0x020]
	ldr r2, =0x00011117
	str r2, [r0, #0x818]
	str r2, [r1, #0x818]
	ldr r2, =0x00025576
	str r2, [r0, #0x004]
	ldr r2, =0x00011006
	str r2, [r0, #0x404]
	ldr r2, =0x00000000
	str r2, [r0, #0x01c]
.endm

.macro imx6dqsabresd_ddr_setting
	ldr r0, =IOMUXC_BASE_ADDR
	ldr r1, =0x000c0000
	str r1, [r0, #0x798]
	ldr r1, =0x00000000
	str r1, [r0, #0x758]

	ldr r1, =0x00000030
	str r1, [r0, #0x588]
	str r1, [r0, #0x594]
	str r1, [r0, #0x56c]
	str r1, [r0, #0x578]
	str r1, [r0, #0x74c]
	str r1, [r0, #0x57c]

	ldr r1, =0x00000000
	str r1, [r0, #0x58c]

	ldr r1, =0x00000030
	str r1, [r0, #0x59c]
	str r1, [r0, #0x5a0]
	str r1, [r0, #0x78c]

	ldr r1, =0x00020000
	str r1, [r0, #0x750]

	ldr r1, =0x00000030
	str r1, [r0, #0x5a8]
	str r1, [r0, #0x5b0]
	str r1, [r0, #0x524]
	str r1, [r0, #0x51c]
	str r1, [r0, #0x518]
	str r1, [r0, #0x50c]
	str r1, [r0, #0x5b8]
	str r1, [r0, #0x5c0]

	ldr r1, =0x00020000
	str r1, [r0, #0x774]

	ldr r1, =0x00000030
	str r1, [r0, #0x784]
	str r1, [r0, #0x788]
	str r1, [r0, #0x794]
	str r1, [r0, #0x79c]
	str r1, [r0, #0x7a0]
	str r1, [r0, #0x7a4]
	str r1, [r0, #0x7a8]
	str r1, [r0, #0x748]
	str r1, [r0, #0x5ac]
	str r1, [r0, #0x5b4]
	str r1, [r0, #0x528]
	str r1, [r0, #0x520]
	str r1, [r0, #0x514]
	str r1, [r0, #0x510]
	str r1, [r0, #0x5bc]
	str r1, [r0, #0x5c4]

	ldr r0, =MMDC_P0_BASE_ADDR
	ldr r2, =0xa1390003
	str r2, [r0, #0x800]

	ldr r2, =0x001F001F
	str r2, [r0, #0x80c]
	str r2, [r0, #0x810]
	ldr r1, =MMDC_P1_BASE_ADDR
	str r2, [r1, #0x80c]
	str r2, [r1, #0x810]

	ldr r2, =0x43270338
	str r2, [r0, #0x83c]
	ldr r2, =0x03200314
	str r2, [r0, #0x840]

	ldr r2, =0x431A032F
	str r2, [r1, #0x83c]
	ldr r2, =0x03200263
	str r2, [r1, #0x840]

	ldr r2, =0x4B434748
	str r2, [r0, #0x848]
	ldr r2, =0x4445404C
	str r2, [r1, #0x848]

	ldr r2, =0x38444542
	str r2, [r0, #0x850]
	ldr r2, =0x4935493A
	str r2, [r1, #0x850]

	ldr r2, =0x33333333
	str r2, [r0, #0x81c]
	str r2, [r0, #0x820]
	str r2, [r0, #0x824]
	str r2, [r0, #0x828]
	str r2, [r1, #0x81c]
	str r2, [r1, #0x820]
	str r2, [r1, #0x824]
	str r2, [r1, #0x828]

	ldr r2, =0x00000800
	str r2, [r0, #0x8b8]
	str r2, [r1, #0x8b8]

	ldr r2, =0x00020036
	str r2, [r0, #0x004]
	ldr r2, =0x09444040
	str r2, [r0, #0x008]

	ldr r2, =0x555A7975
	str r2, [r0, #0x00c]
	ldr r2, =0xFF538F64
	str r2, [r0, #0x010]

	ldr r2, =0x01FF00DB
	str r2, [r0, #0x014]
	ldr r2, =0x00001740
	str r2, [r0, #0x018]

	ldr r2, =0x00008000
	str r2, [r0, #0x01c]
	ldr r2, =0x000026d2
	str r2, [r0, #0x02c]
	ldr r2, =0x005A1023
	str r2, [r0, #0x030]
	ldr r2, =0x00000027
	str r2, [r0, #0x040]

	ldr r2, =0x831A0000
	str r2, [r0, #0x000]

	ldr r2, =0x04088032
	str r2, [r0, #0x01c]
	ldr r2, =0x00008033
	str r2, [r0, #0x01c]
	ldr r2, =0x00048031
	str r2, [r0, #0x01c]
	ldr r2, =0x09408030
	str r2, [r0, #0x01c]
	ldr r2, =0x04008040
	str r2, [r0, #0x01c]

	ldr r2, =0x00005800
	str r2, [r0, #0x020]
	ldr r2, =0x00011117
	str r2, [r0, #0x818]
	str r2, [r1, #0x818]
	ldr r2, =0x00025576
	str r2, [r0, #0x004]
	ldr r2, =0x00011006
	str r2, [r0, #0x404]
	ldr r2, =0x00000000
	str r2, [r0, #0x01c]
.endm

.macro imx6dlsabresd_ddr_setting
	ldr r0, =IOMUXC_BASE_ADDR
	ldr r1, =0x000c0000
	str r1, [r0, #0x774]
	ldr r1, =0x00000000
	str r1, [r0, #0x754]

	ldr r1, =0x00000030
	str r1, [r0, #0x4ac]
	str r1, [r0, #0x4b0]
	str r1, [r0, #0x464]
	str r1, [r0, #0x490]
	str r1, [r0, #0x74c]
	str r1, [r0, #0x494]

	ldr r1, =0x00000000
	str r1, [r0, #0x4a0]

	ldr r1, =0x00000030
	str r1, [r0, #0x4b4]
	str r1, [r0, #0x4b8]
	str r1, [r0, #0x76c]

	ldr r1, =0x00020000
	str r1, [r0, #0x750]

	ldr r1, =0x00000030
	str r1, [r0, #0x4bc]
	str r1, [r0, #0x4c0]
	str r1, [r0, #0x4c4]
	str r1, [r0, #0x4c8]
	str r1, [r0, #0x4cc]
	str r1, [r0, #0x4d0]
	str r1, [r0, #0x4d4]
	str r1, [r0, #0x4d8]

	ldr r1, =0x00020000
	str r1, [r0, #0x760]

	ldr r1, =0x00000030
	str r1, [r0, #0x764]
	str r1, [r0, #0x770]
	str r1, [r0, #0x778]
	str r1, [r0, #0x77c]
	str r1, [r0, #0x780]
	str r1, [r0, #0x784]
	str r1, [r0, #0x78c]
	str r1, [r0, #0x748]
	str r1, [r0, #0x470]
	str r1, [r0, #0x474]
	str r1, [r0, #0x478]
	str r1, [r0, #0x47c]
	str r1, [r0, #0x480]
	str r1, [r0, #0x484]
	str r1, [r0, #0x488]
	str r1, [r0, #0x48c]

	ldr r0, =MMDC_P0_BASE_ADDR
	ldr r2, =0xa1390003
	str r2, [r0, #0x800]

	ldr r2, =0x001f001f
	str r2, [r0, #0x80c]
	str r2, [r0, #0x810]
	ldr r1, =MMDC_P1_BASE_ADDR
	str r2, [r1, #0x80c]
	str r2, [r1, #0x810]

	ldr r2, =0x4220021F
	str r2, [r0, #0x83c]
	ldr r2, =0x0207017E
	str r2, [r0, #0x840]

	ldr r2, =0x4201020C
	str r2, [r1, #0x83c]
	ldr r2, =0x01660172
	str r2, [r1, #0x840]

	ldr r2, =0x4A4D4E4D
	str r2, [r0, #0x848]
	ldr r2, =0x4A4F5049
	str r2, [r1, #0x848]

	ldr r2, =0x3F3C3D31
	str r2, [r0, #0x850]
	ldr r2, =0x3238372B
	str r2, [r1, #0x850]

	ldr r2, =0x33333333
	str r2, [r0, #0x81c]
	str r2, [r0, #0x820]
	str r2, [r0, #0x824]
	str r2, [r0, #0x828]
	str r2, [r1, #0x81c]
	str r2, [r1, #0x820]
	str r2, [r1, #0x824]
	str r2, [r1, #0x828]

	ldr r2, =0x00000800
	str r2, [r0, #0x8b8]
	str r2, [r1, #0x8b8]

	ldr r2, =0x0002002D
	str r2, [r0, #0x004]
	ldr r2, =0x00333030
	str r2, [r0, #0x008]

	ldr r2, =0x3F435313
	str r2, [r0, #0x00c]
	ldr r2, =0xB66E8B63
	str r2, [r0, #0x010]

	ldr r2, =0x01FF00DB
	str r2, [r0, #0x014]
	ldr r2, =0x00001740
	str r2, [r0, #0x018]

	ldr r2, =0x00008000
	str r2, [r0, #0x01c]
	ldr r2, =0x000026d2
	str r2, [r0, #0x02c]
	ldr r2, =0x00431023
	str r2, [r0, #0x030]
	ldr r2, =0x00000027
	str r2, [r0, #0x040]

	ldr r2, =0x831A0000
	str r2, [r0, #0x000]

	ldr r2, =0x04008032
	str r2, [r0, #0x01c]
	ldr r2, =0x00008033
	str r2, [r0, #0x01c]
	ldr r2, =0x00048031
	str r2, [r0, #0x01c]
	ldr r2, =0x05208030
	str r2, [r0, #0x01c]
	ldr r2, =0x04008040
	str r2, [r0, #0x01c]

	ldr r2, =0x00005800
	str r2, [r0, #0x020]
	ldr r2, =0x00011117
	str r2, [r0, #0x818]
	str r2, [r1, #0x818]
	ldr r2, =0x0002556D
	str r2, [r0, #0x004]
	ldr r2, =0x00011006
	str r2, [r0, #0x404]
	ldr r2, =0x00000000
	str r2, [r0, #0x01c]
.endm

.macro imx6solosabresd_ddr_setting
	ldr r0, =IOMUXC_BASE_ADDR
	ldr r1, =0x000c0000
	str r1, [r0, #0x774]
	ldr r1, =0x00000000
	str r1, [r0, #0x754]

	ldr r1, =0x00000030
	str r1, [r0, #0x4ac]
	str r1, [r0, #0x4b0]
	str r1, [r0, #0x464]
	str r1, [r0, #0x490]
	str r1, [r0, #0x74c]
	str r1, [r0, #0x494]

	ldr r1, =0x00000000
	str r1, [r0, #0x4a0]

	ldr r1, =0x00000030
	str r1, [r0, #0x4b4]
	str r1, [r0, #0x4b8]
	str r1, [r0, #0x76c]

	ldr r1, =0x00020000
	str r1, [r0, #0x750]

	ldr r1, =0x00000030
	str r1, [r0, #0x4bc]
	str r1, [r0, #0x4c0]
	str r1, [r0, #0x4c4]
	str r1, [r0, #0x4c8]

	ldr r1, =0x00020000
	str r1, [r0, #0x760]

	ldr r1, =0x00000030
	str r1, [r0, #0x764]
	str r1, [r0, #0x770]
	str r1, [r0, #0x778]
	str r1, [r0, #0x77c]
	str r1, [r0, #0x470]
	str r1, [r0, #0x474]
	str r1, [r0, #0x478]
	str r1, [r0, #0x47c]

	ldr r0, =MMDC_P0_BASE_ADDR
	ldr r2, =0xa1390003
	str r2, [r0, #0x800]

	ldr r2, =0x001F001F
	str r2, [r0, #0x80c]
	str r2, [r0, #0x810]

	ldr r2, =0x42190219
	str r2, [r0, #0x83c]
	ldr r2, =0x017B0177
	str r2, [r0, #0x840]

	ldr r2, =0x4B4D4E4D
	str r2, [r0, #0x848]

	ldr r2, =0x3F3E2D36
	str r2, [r0, #0x850]

	ldr r2, =0x33333333
	str r2, [r0, #0x81c]
	str r2, [r0, #0x820]
	str r2, [r0, #0x824]
	str r2, [r0, #0x828]

	ldr r2, =0x00000800
	str r2, [r0, #0x8b8]

	ldr r2, =0x0002002D
	str r2, [r0, #0x004]
	ldr r2, =0x00333030
	str r2, [r0, #0x008]

	ldr r2, =0x3F435313
	str r2, [r0, #0x00c]
	ldr r2, =0xB66E8B63
	str r2, [r0, #0x010]

	ldr r2, =0x01FF00DB
	str r2, [r0, #0x014]
	ldr r2, =0x00001740
	str r2, [r0, #0x018]

	ldr r2, =0x00008000
	str r2, [r0, #0x01c]
	ldr r2, =0x000026d2
	str r2, [r0, #0x02c]
	ldr r2, =0x00431023
	str r2, [r0, #0x030]
	ldr r2, =0x00000017
	str r2, [r0, #0x040]

	ldr r2, =0x83190000
	str r2, [r0, #0x000]

	ldr r2, =0x04008032
	str r2, [r0, #0x01c]
	ldr r2, =0x00008033
	str r2, [r0, #0x01c]
	ldr r2, =0x00048031
	str r2, [r0, #0x01c]
	ldr r2, =0x05208030
	str r2, [r0, #0x01c]
	ldr r2, =0x04008040
	str r2, [r0, #0x01c]

	ldr r2, =0x00005800
	str r2, [r0, #0x020]
	ldr r2, =0x00011117
	str r2, [r0, #0x818]
	ldr r2, =0x0002556D
	str r2, [r0, #0x004]
	ldr r2, =0x00011006
	str r2, [r0, #0x404]
	ldr r2, =0x00000000
	str r2, [r0, #0x01c]
.endm
.macro imx6_clock_gating
	ldr r0, =CCM_BASE_ADDR
	ldr r1, =0x00C03F3F
	str r1, [r0, #0x068]
	ldr r1, =0x0030FC03
	str r1, [r0, #0x06c]
	ldr r1, =0x0FFFC000
	str r1, [r0, #0x070]
	ldr r1, =0x3FF00000
	str r1, [r0, #0x074]
	ldr r1, =0x00FFF300
	str r1, [r0, #0x078]
	ldr r1, =0x0F0000C3
	str r1, [r0, #0x07c]
	ldr r1, =0x000003FF
	str r1, [r0, #0x080]
.endm

.macro imx6_qos_setting
	ldr r0, =IOMUXC_BASE_ADDR
	ldr r1, =0xF00000CF
	str r1, [r0, #0x10]

#if defined(CONFIG_MX6QP)
	ldr r1, =0x77177717
	str r1, [r0, #0x18]
	str r1, [r0, #0x1c]
#else
	ldr r1, =0x007F007F
	str r1, [r0, #0x18]
	str r1, [r0, #0x1c]
#endif
.endm

.macro imx6_ddr_setting
#if defined (CONFIG_MX6SOLO)
	imx6solosabresd_ddr_setting
#elif defined (CONFIG_MX6DL)
	imx6dlsabresd_ddr_setting
#elif defined (CONFIG_MX6QP)
	imx6dqpsabresd_ddr_setting
#elif defined (CONFIG_MX6Q)
	imx6dqsabresd_ddr_setting
#else
	#error "SOC not configured"
#endif

.endm

/* include the common plugin code here */
#include <asm/arch/mx6_plugin.S>
