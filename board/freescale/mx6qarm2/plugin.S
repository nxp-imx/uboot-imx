/*
 * Copyright (C) 2012-2016 Freescale Semiconductor, Inc.
 *
 * Author: Fabio Estevam <fabio.estevam@freescale.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <config.h>

.macro imx6dlarm2_ddr_setting
	ldr r0, =CCM_BASE_ADDR
	ldr r1, =0xffffffff
	str r1, [r0, #0x068]
	ldr r1, =0xffffffff
	str r1, [r0, #0x06c]
	ldr r1, =0xffffffff
	str r1, [r0, #0x070]
	ldr r1, =0xffffffff
	str r1, [r0, #0x074]
	ldr r1, =0xffffffff
	str r1, [r0, #0x078]
	ldr r1, =0xffffffff
	str r1, [r0, #0x07c]
	ldr r1, =0xffffffff
	str r1, [r0, #0x080]
	ldr r1, =0xffffffff
	str r1, [r0, #0x084]

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

	ldr r1, =0x00003000
	str r1, [r0, #0x590]
	str r1, [r0, #0x598]

	ldr r1, =0x00000000
	str r1, [r0, #0x58c]

	ldr r1, =0x00003030
	str r1, [r0, #0x59c]
	str r1, [r0, #0x5a0]

	ldr r1, =0x00000030
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
	ldr r1, =MMDC_P1_BASE_ADDR
	ldr r2, =0xa1390003
	str r2, [r0, #0x800]
	str r2, [r1, #0x800]

	ldr r2, =0x001F001F
	str r2, [r0, #0x80c]
	str r2, [r0, #0x810]

	ldr r2, =0x00370037
	str r2, [r1, #0x80c]
	str r2, [r1, #0x810]

	ldr r2, =0x422f0220
	str r2, [r0, #0x83c]
	ldr r2, =0x021f0219
	str r2, [r0, #0x840]

	ldr r2, =0x422f0220
	str r2, [r1, #0x83c]
	ldr r2, =0x022d022f
	str r2, [r1, #0x840]

	ldr r2, =0x47494b49
	str r2, [r0, #0x848]
	ldr r2, =0x48484c47
	str r2, [r1, #0x848]

	ldr r2, =0x39382b2f
	str r2, [r0, #0x850]
	ldr r2, =0x2f35312c
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
	ldr r2, =0x00000800
	str r2, [r1, #0x8b8]

	ldr r2, =0x0002002d
	str r2, [r0, #0x004]
	ldr r2, =0x00333030
	str r2, [r0, #0x008]
	ldr r2, =0x40445323
	str r2, [r0, #0x00c]
	ldr r2, =0xb66e8c63
	str r2, [r0, #0x010]
	ldr r2, =0x01ff00db
	str r2, [r0, #0x014]
	ldr r2, =0x00081740
	str r2, [r0, #0x018]

	ldr r2, =0x00008000
	str r2, [r0, #0x01c]

	ldr r2, =0x000026d2
	str r2, [r0, #0x02c]
	ldr r2, =0x00440e21
	str r2, [r0, #0x030]

#ifdef CONFIG_DDR_32BIT
	ldr r2, =0x00000017
	str r2, [r0, #0x040]
	ldr r2, =0xc3190000
	str r2, [r0, #0x000]
#else
	ldr r2, =0x00000027
	str r2, [r0, #0x040]
	ldr r2, =0xc31a0000
	str r2, [r0, #0x000]
#endif

	ldr r2, =0x04008032
	str r2, [r0, #0x01c]
	ldr r2, =0x0400803a
	str r2, [r0, #0x01c]

	ldr r2, =0x00008033
	str r2, [r0, #0x01c]
	ldr r2, =0x0000803b
	str r2, [r0, #0x01c]

	ldr r2, =0x00428031
	str r2, [r0, #0x01c]
	ldr r2, =0x00428039
	str r2, [r0, #0x01c]

	ldr r2, =0x07208030
	str r2, [r0, #0x01c]
	ldr r2, =0x07208038
	str r2, [r0, #0x01c]

	ldr r2, =0x04008040
	str r2, [r0, #0x01c]
	ldr r2, =0x04008048
	str r2, [r0, #0x01c]

	ldr r2, =0x00005800
	str r2, [r0, #0x020]

	ldr r2, =0x00000007
	str r2, [r0, #0x818]
	str r2, [r1, #0x818]

	ldr r2, =0x0002556d
	str r2, [r0, #0x004]
	ldr r2, =0x00011006
	str r2, [r1, #0x004]
	ldr r2, =0x00000000
	str r2, [r0, #0x01c]
.endm

.macro imx6dqarm2_ddr_setting
	ldr r0, =CCM_BASE_ADDR
	ldr r1, =0xffffffff
	str r1, [r0, #0x068]
	ldr r1, =0xffffffff
	str r1, [r0, #0x06c]
	ldr r1, =0xffffffff
	str r1, [r0, #0x070]
	ldr r1, =0xffffffff
	str r1, [r0, #0x074]
	ldr r1, =0xffffffff
	str r1, [r0, #0x078]
	ldr r1, =0xffffffff
	str r1, [r0, #0x07c]
	ldr r1, =0xffffffff
	str r1, [r0, #0x080]
	ldr r1, =0xffffffff
	str r1, [r0, #0x084]

	ldr r0, =IOMUXC_BASE_ADDR
	ldr r1, =0x000c0000
	str r1, [r0, #0x798]
	ldr r1, =0x00000000
	str r1, [r0, #0x758]

	ldr r1, =0x00000030
	str r1, [r0, #0x588]
	ldr r1, =0x00000030
	str r1, [r0, #0x594]

	ldr r1, =0x00000030
	str r1, [r0, #0x56c]
	ldr r1, =0x00000030
	str r1, [r0, #0x578]
	ldr r1, =0x00000030
	str r1, [r0, #0x74c]

	ldr r1, =0x00000030
	str r1, [r0, #0x57c]

	ldr r1, =0x00000000
	str r1, [r0, #0x58c]
	ldr r1, =0x00000030
	str r1, [r0, #0x59c]
	ldr r1, =0x00000030
	str r1, [r0, #0x5a0]
	ldr r1, =0x00000030
	str r1, [r0, #0x78c]

	ldr r1, =0x00000000
	str r1, [r0, #0x750]

	ldr r1, =0x00000038
	str r1, [r0, #0x5a8]
	ldr r1, =0x00000038
	str r1, [r0, #0x5b0]
	ldr r1, =0x00000038
	str r1, [r0, #0x524]
	ldr r1, =0x00000038
	str r1, [r0, #0x51c]
	ldr r1, =0x00000038
	str r1, [r0, #0x518]
	ldr r1, =0x00000038
	str r1, [r0, #0x50c]
	ldr r1, =0x00000038
	str r1, [r0, #0x5b8]
	ldr r1, =0x00000038
	str r1, [r0, #0x5c0]

	ldr r1, =0x00000000
	str r1, [r0, #0x774]

	ldr r1, =0x00000030
	str r1, [r0, #0x784]
	ldr r1, =0x00000030
	str r1, [r0, #0x788]
	ldr r1, =0x00000030
	str r1, [r0, #0x794]
	ldr r1, =0x00000030
	str r1, [r0, #0x79c]
	ldr r1, =0x00000030
	str r1, [r0, #0x7a0]
	ldr r1, =0x00000030
	str r1, [r0, #0x7a4]
	ldr r1, =0x00000030
	str r1, [r0, #0x7a8]
	ldr r1, =0x00000030
	str r1, [r0, #0x748]

	ldr r1, =0x00000030
	str r1, [r0, #0x5ac]
	ldr r1, =0x00000030
	str r1, [r0, #0x5b4]
	ldr r1, =0x00000030
	str r1, [r0, #0x528]
	ldr r1, =0x00000030
	str r1, [r0, #0x520]
	ldr r1, =0x00000030
	str r1, [r0, #0x514]
	ldr r1, =0x00000030
	str r1, [r0, #0x510]
	ldr r1, =0x00000030
	str r1, [r0, #0x5bc]
	ldr r1, =0x00000030
	str r1, [r0, #0x5c4]

	ldr r0, =MMDC_P0_BASE_ADDR
	ldr r2, =0xa1390003
	str r2, [r0, #0x800]

	ldr r2, =0x001F001F
	str r2, [r0, #0x80c]
	ldr r2, =0x001F001F
	str r2, [r0, #0x810]

	ldr r1, =MMDC_P1_BASE_ADDR
	ldr r2, =0x00440044
	str r2, [r1, #0x80c]
	ldr r2, =0x00440044
	str r2, [r1, #0x810]

	ldr r2, =0x4333033F
	str r2, [r0, #0x83c]
	ldr r2, =0x0339033E
	str r2, [r0, #0x840]
	ldr r2, =0x433F0343
	str r2, [r1, #0x83c]
	ldr r2, =0x03490320
	str r2, [r1, #0x840]

	ldr r2, =0x42363838
	str r2, [r0, #0x848]
	ldr r2, =0x3F343242
	str r2, [r1, #0x848]

	ldr r2, =0x37424844
	str r2, [r0, #0x850]
	ldr r2, =0x48264731
	str r2, [r1, #0x850]

	ldr r2, =0x33333333
	str r2, [r0, #0x81c]
	ldr r2, =0x33333333
	str r2, [r0, #0x820]
	ldr r2, =0x33333333
	str r2, [r0, #0x824]
	ldr r2, =0x33333333
	str r2, [r0, #0x828]
	ldr r2, =0x33333333
	str r2, [r1, #0x81c]
	ldr r2, =0x33333333
	str r2, [r1, #0x820]
	ldr r2, =0x33333333
	str r2, [r1, #0x824]
	ldr r2, =0x33333333
	str r2, [r1, #0x828]

	ldr r2, =0x00000800
	str r2, [r0, #0x8b8]
	ldr r2, =0x00000800
	str r2, [r1, #0x8b8]

	ldr r2, =0x00020036
	str r2, [r0, #0x004]
	ldr r2, =0x09444040
	str r2, [r0, #0x008]
	ldr r2, =0x555A7975
	str r2, [r0, #0x00c]
	ldr r2, =0xFF538F64
	str r2, [r0, #0x010]
	ldr r2, =0x01ff00db
	str r2, [r0, #0x014]
	ldr r2, =0x00081740
	str r2, [r0, #0x018]

	ldr r2, =0x00008000
	str r2, [r0, #0x01c]

	ldr r2, =0x000026d2
	str r2, [r0, #0x02c]
	ldr r2, =0x005a1023
	str r2, [r0, #0x030]
	ldr r2, =0x00000027
	str r2, [r0, #0x040]

	ldr r2, =0x11420000
	str r2, [r0, #0x400]
	ldr r2, =0x11420000
	str r2, [r1, #0x400]

	ldr r2, =0xc31a0000
	str r2, [r0, #0x000]

	ldr r2, =0x04088032
	str r2, [r0, #0x01c]
	ldr r2, =0x0408803a
	str r2, [r0, #0x01c]

	ldr r2, =0x00008033
	str r2, [r0, #0x01c]
	ldr r2, =0x0000803b
	str r2, [r0, #0x01c]

	ldr r2, =0x00048031
	str r2, [r0, #0x01c]
	ldr r2, =0x00048039
	str r2, [r0, #0x01c]

	ldr r2, =0x09408030
	str r2, [r0, #0x01c]
	ldr r2, =0x09408038
	str r2, [r0, #0x01c]

	ldr r2, =0x04008040
	str r2, [r0, #0x01c]
	ldr r2, =0x04008048
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

.macro imx6dlarm2_lpddr2_setting
	ldr r0, =CCM_BASE_ADDR
	ldr r1, =0xffffffff
	str r1, [r0, #0x068]
	ldr r1, =0xffffffff
	str r1, [r0, #0x06c]
	ldr r1, =0xffffffff
	str r1, [r0, #0x070]
	ldr r1, =0xffffffff
	str r1, [r0, #0x074]
	ldr r1, =0xffffffff
	str r1, [r0, #0x078]
	ldr r1, =0xffffffff
	str r1, [r0, #0x07c]
	ldr r1, =0xffffffff
	str r1, [r0, #0x080]
	ldr r1, =0xffffffff
	str r1, [r0, #0x084]

	ldr r0, =IOMUXC_BASE_ADDR
	ldr r1, =0x00003028
	str r1, [r0, #0x4bc]
	str r1, [r0, #0x4c0]
	str r1, [r0, #0x4c4]
	str r1, [r0, #0x4c8]
	str r1, [r0, #0x4cc]
	str r1, [r0, #0x4d0]
	str r1, [r0, #0x4d4]
	str r1, [r0, #0x4d8]

	ldr r1, =0x00000038
	str r1, [r0, #0x470]
	str r1, [r0, #0x474]
	str r1, [r0, #0x478]
	str r1, [r0, #0x47c]
	str r1, [r0, #0x480]
	str r1, [r0, #0x484]
	str r1, [r0, #0x488]
	str r1, [r0, #0x48c]
	str r1, [r0, #0x464]
	str r1, [r0, #0x490]
	str r1, [r0, #0x4ac]
	str r1, [r0, #0x4b0]
	str r1, [r0, #0x494]
	str r1, [r0, #0x4a4]
	str r1, [r0, #0x4a8]

	ldr r1, =0x00000000
	str r1, [r0, #0x4a0]
	ldr r1, =0x00000038
	str r1, [r0, #0x4b4]
	str r1, [r0, #0x4b8]
	str r1, [r0, #0x764]
	str r1, [r0, #0x770]
	str r1, [r0, #0x778]
	str r1, [r0, #0x77c]
	str r1, [r0, #0x780]
	str r1, [r0, #0x784]
	str r1, [r0, #0x78c]
	str r1, [r0, #0x748]
	str r1, [r0, #0x74c]
	str r1, [r0, #0x76c]
	ldr r1, =0x00020000
	str r1, [r0, #0x750]
	ldr r1, =0x00000000
	str r1, [r0, #0x754]
	ldr r1, =0x00020000
	str r1, [r0, #0x760]
	ldr r1, =0x00080000
	str r1, [r0, #0x774]

	ldr r0, =MMDC_P0_BASE_ADDR
	ldr r1, =MMDC_P1_BASE_ADDR
	ldr r2, =0x00008000
	str r2, [r0, #0x1c]
	str r2, [r1, #0x1c]
	ldr r2, =0x1b5f01ff
	str r2, [r0, #0x85c]
	str r2, [r1, #0x85c]
	ldr r2, =0xa1390003
	str r2, [r0, #0x800]
	ldr r2, =0x00400000
	str r2, [r0, #0x890]
	str r2, [r1, #0x890]
	ldr r2, =0x00055555
	str r2, [r1, #0x8bc]
	ldr r2, =0x00000800
	str r2, [r0, #0x8b8]
	str r2, [r1, #0x8b8]

	ldr r2, =0x33333333
	str r2, [r0, #0x81c]
	str r2, [r0, #0x820]
	str r2, [r0, #0x824]
	str r2, [r0, #0x828]
	str r2, [r1, #0x81c]
	str r2, [r1, #0x820]
	str r2, [r1, #0x824]
	str r2, [r1, #0x828]

	ldr r2, =0x4b4b524f
	str r2, [r0, #0x848]
	ldr r2, =0x494f4c44
	str r2, [r1, #0x848]

	ldr r2, =0x3c3d303c
	str r2, [r0, #0x850]
	ldr r2, =0x3c343d38
	str r2, [r1, #0x850]

	ldr r2, =0x20000000
	str r2, [r0, #0x83c]
	str r2, [r1, #0x83c]
	ldr r2, =0x00000000
	str r2, [r0, #0x840]
	str r2, [r1, #0x840]

	ldr r2, =0x00000a00
	str r2, [r0, #0x858]
	str r2, [r1, #0x858]

	ldr r2, =0x00000800
	str r2, [r0, #0x8b8]
	str r2, [r1, #0x8b8]

	ldr r2, =0x34386145
	str r2, [r0, #0xc]
	ldr r2, =0x00020036
	str r2, [r0, #0x4]
	ldr r2, =0x00100c83
	str r2, [r0, #0x10]
	ldr r2, =0x000000Dc
	str r2, [r0, #0x14]
	ldr r2, =0x0000174C
	str r2, [r0, #0x18]
	ldr r2, =0x0f9f26d2
	str r2, [r0, #0x2c]
	ldr r2, =0x0000020e
	str r2, [r0, #0x30]
	ldr r2, =0x00190778
	str r2, [r0, #0x38]
	ldr r2, =0x00000000
	str r2, [r0, #0x8]

	ldr r2, =0x0000005f
	str r2, [r0, #0x40]
	ldr r2, =0x0000000f
	str r2, [r0, #0x404]

	ldr r2, =0xc3010000
	str r2, [r0, #0x0]

	ldr r2, =0x34386145
	str r2, [r1, #0xc]

	ldr r2, =0x00020036
	str r2, [r1, #0x4]
	ldr r2, =0x00100c83
	str r2, [r1, #0x10]
	ldr r2, =0x000000Dc
	str r2, [r1, #0x14]
	ldr r2, =0x0000174C
	str r2, [r1, #0x18]
	ldr r2, =0x0f9f26d2
	str r2, [r1, #0x2c]
	ldr r2, =0x0000020e
	str r2, [r1, #0x30]
	ldr r2, =0x00190778
	str r2, [r1, #0x38]
	ldr r2, =0x00000000
	str r2, [r1, #0x8]

	ldr r2, =0x0000003f
	str r2, [r1, #0x40]

	ldr r2, =0xc3010000
	str r2, [r1, #0x0]

	ldr r2, =0x003f8030
	str r2, [r0, #0x1c]
	ldr r2, =0xff0a8030
	str r2, [r0, #0x1c]
	ldr r2, =0xa2018030
	str r2, [r0, #0x1c]
	ldr r2, =0x06028030
	str r2, [r0, #0x1c]
	ldr r2, =0x01038030
	str r2, [r0, #0x1c]

	ldr r2, =0x003f8030
	str r2, [r1, #0x1c]
	ldr r2, =0xff0a8030
	str r2, [r1, #0x1c]
	ldr r2, =0xa2018030
	str r2, [r1, #0x1c]
	ldr r2, =0x06028030
	str r2, [r1, #0x1c]
	ldr r2, =0x01038030
	str r2, [r1, #0x1c]

	ldr r2, =0x00005800
	str r2, [r0, #0x20]
	str r2, [r1, #0x20]

	ldr r2, =0x00000000
	str r2, [r0, #0x818]
	str r2, [r1, #0x818]

	ldr r2, =0xa1310003
	str r2, [r0, #0x800]

	ldr r2, =0x00000800
	str r2, [r0, #0x8b8]
	str r2, [r1, #0x8b8]

	ldr r2, =0x00000000
	str r2, [r0, #0x1c]
	str r2, [r1, #0x1c]

	ldr r0, =IOMUXC_BASE_ADDR
	ldr r1, =0xF00000CF
	str r1, [r0, #0x10]
	ldr r1, =0x007F007F
	str r1, [r0, #0x18]
	ldr r1, =0x007F007F
	str r1, [r0, #0x1c]
.endm

.macro imx6dqarm2_lpddr2_setting
	ldr r0, =CCM_BASE_ADDR
	ldr r1, =0x60324
	str r1, [r0, #0x18]
	ldr r1, =0xffffffff
	str r1, [r0, #0x068]
	ldr r1, =0xffffffff
	str r1, [r0, #0x06c]
	ldr r1, =0xffffffff
	str r1, [r0, #0x070]
	ldr r1, =0xffffffff
	str r1, [r0, #0x074]
	ldr r1, =0xffffffff
	str r1, [r0, #0x078]
	ldr r1, =0xffffffff
	str r1, [r0, #0x07c]
	ldr r1, =0xffffffff
	str r1, [r0, #0x080]
	ldr r1, =0xffffffff
	str r1, [r0, #0x084]

	ldr r0, =IOMUXC_BASE_ADDR
	ldr r1, =0x00003038
	str r1, [r0, #0x5a8]
	str r1, [r0, #0x5b0]
	str r1, [r0, #0x524]
	str r1, [r0, #0x51c]
	str r1, [r0, #0x518]
	str r1, [r0, #0x50c]
	str r1, [r0, #0x5b8]
	str r1, [r0, #0x5c0]

	ldr r1, =0x00000038
	str r1, [r0, #0x5ac]
	str r1, [r0, #0x5b4]
	str r1, [r0, #0x528]
	str r1, [r0, #0x520]
	str r1, [r0, #0x514]
	str r1, [r0, #0x510]
	str r1, [r0, #0x5bc]
	str r1, [r0, #0x5c4]
	str r1, [r0, #0x56c]
	str r1, [r0, #0x578]
	str r1, [r0, #0x588]
	str r1, [r0, #0x594]
	str r1, [r0, #0x57c]
	str r1, [r0, #0x590]
	str r1, [r0, #0x598]

	ldr r1, =0x00000000
	str r1, [r0, #0x58c]
	ldr r1, =0x00000038
	str r1, [r0, #0x59c]
	str r1, [r0, #0x5a0]
	str r1, [r0, #0x784]
	str r1, [r0, #0x788]
	str r1, [r0, #0x794]
	str r1, [r0, #0x79c]
	str r1, [r0, #0x7a0]
	str r1, [r0, #0x7a4]
	str r1, [r0, #0x7a8]
	str r1, [r0, #0x748]
	str r1, [r0, #0x74c]
	ldr r1, =0x00020000
	str r1, [r0, #0x750]
	ldr r1, =0x00000000
	str r1, [r0, #0x758]
	ldr r1, =0x00020000
	str r1, [r0, #0x774]
	ldr r1, =0x00000038
	str r1, [r0, #0x78c]
	ldr r1, =0x00080000
	str r1, [r0, #0x798]

	ldr r0, =MMDC_P0_BASE_ADDR
	ldr r1, =MMDC_P1_BASE_ADDR
	ldr r2, =0x00008000
	str r2, [r0, #0x1c]
	str r2, [r1, #0x1c]
	ldr r2, =0x1b5f01ff
	str r2, [r0, #0x85c]
	str r2, [r1, #0x85c]
	ldr r2, =0xa1390000
	str r2, [r0, #0x800]
	str r2, [r1, #0x800]
	ldr r2, =0x00400000
	str r2, [r0, #0x890]
	str r2, [r1, #0x890]
	ldr r2, =0x00055555
	str r2, [r1, #0x8bc]
	ldr r2, =0x00000800
	str r2, [r0, #0x8b8]
	str r2, [r1, #0x8b8]

	ldr r2, =0x33333333
	str r2, [r0, #0x81c]
	str r2, [r0, #0x820]
	str r2, [r0, #0x824]
	str r2, [r0, #0x828]
	str r2, [r1, #0x81c]
	str r2, [r1, #0x820]
	str r2, [r1, #0x824]
	str r2, [r1, #0x828]

	ldr r2, =0xf3333333
	str r2, [r0, #0x82c]
	str r2, [r0, #0x830]
	str r2, [r0, #0x834]
	str r2, [r0, #0x838]
	str r2, [r1, #0x82c]
	str r2, [r1, #0x830]
	str r2, [r1, #0x834]
	str r2, [r1, #0x838]

	ldr r2, =0x49383b39
	str r2, [r0, #0x848]
	ldr r2, =0x30364738
	str r2, [r0, #0x850]

	ldr r2, =0x3e3c3846
	str r2, [r1, #0x848]
	ldr r2, =0x4c294b35
	str r2, [r1, #0x850]

	ldr r2, =0x20000000
	str r2, [r0, #0x83c]
	str r2, [r1, #0x83c]
	ldr r2, =0x00000000
	str r2, [r0, #0x840]
	str r2, [r1, #0x840]

	ldr r2, =0x00000f00
	str r2, [r0, #0x858]
	str r2, [r1, #0x858]

	ldr r2, =0x00000800
	str r2, [r0, #0x8b8]
	str r2, [r1, #0x8b8]

	ldr r2, =0x555a61a5
	str r2, [r0, #0xc]
	ldr r2, =0x00020036
	str r2, [r0, #0x4]
	ldr r2, =0x00160e83
	str r2, [r0, #0x10]
	ldr r2, =0x000000dd
	str r2, [r0, #0x14]
	ldr r2, =0x0008174C
	str r2, [r0, #0x18]
	ldr r2, =0x0f9f26d2
	str r2, [r0, #0x2c]
	ldr r2, =0x0000020e
	str r2, [r0, #0x30]
	ldr r2, =0x200aac
	str r2, [r0, #0x38]
	ldr r2, =0x00000000
	str r2, [r0, #0x8]

	ldr r2, =0x0000005f
	str r2, [r0, #0x40]

	ldr r2, =0xc3010000
	str r2, [r0, #0x0]

	ldr r2, =0x555a61a5
	str r2, [r1, #0xc]
	ldr r2, =0x00020036
	str r2, [r1, #0x4]
	ldr r2, =0x00160e83
	str r2, [r1, #0x10]
	ldr r2, =0x000000dd
	str r2, [r1, #0x14]
	ldr r2, =0x0008174C
	str r2, [r1, #0x18]
	ldr r2, =0x0f9f26d2
	str r2, [r1, #0x2c]
	ldr r2, =0x0000020e
	str r2, [r1, #0x30]
	ldr r2, =0x00200aac
	str r2, [r1, #0x38]
	ldr r2, =0x00000000
	str r2, [r1, #0x8]

	ldr r2, =0x0000003f
	str r2, [r1, #0x40]

	ldr r2, =0xc3010000
	str r2, [r1, #0x0]

	ldr r2, =0x003f8030
	str r2, [r0, #0x1c]
	ldr r2, =0xff0a8030
	str r2, [r0, #0x1c]
	ldr r2, =0xc2018030
	str r2, [r0, #0x1c]
	ldr r2, =0x06028030
	str r2, [r0, #0x1c]
	ldr r2, =0x02038030
	str r2, [r0, #0x1c]

	ldr r2, =0x003f8030
	str r2, [r1, #0x1c]
	ldr r2, =0xff0a8030
	str r2, [r1, #0x1c]
	ldr r2, =0xc2018030
	str r2, [r1, #0x1c]
	ldr r2, =0x06028030
	str r2, [r1, #0x1c]
	ldr r2, =0x02038030
	str r2, [r1, #0x1c]

	ldr r2, =0xa1390003
	str r2, [r0, #0x800]
	str r2, [r1, #0x800]

	ldr r2, =0x00007800
	str r2, [r0, #0x20]
	str r2, [r1, #0x20]

	ldr r2, =0x00000000
	str r2, [r0, #0x818]
	str r2, [r1, #0x818]

	ldr r2, =0xa1310003
	str r2, [r0, #0x800]
	str r2, [r1, #0x800]

	ldr r2, =0x00000800
	str r2, [r0, #0x8b8]
	str r2, [r1, #0x8b8]

	ldr r2, =0x00000000
	str r2, [r0, #0x1c]
	str r2, [r1, #0x1c]

	ldr r2, =0x00011006
	str r2, [r0, #0x404]

	ldr r0, =IOMUXC_BASE_ADDR
	ldr r1, =0xF00000CF
	str r1, [r0, #0x10]
	ldr r1, =0x007F007F
	str r1, [r0, #0x18]
	ldr r1, =0x007F007F
	str r1, [r0, #0x1c]
.endm

.macro imx6dq_pop_arm2_lpddr2_setting
	ldr r0, =CCM_BASE_ADDR
	ldr r1, =0xffffffff
	str r1, [r0, #0x068]
	ldr r1, =0xffffffff
	str r1, [r0, #0x06c]
	ldr r1, =0xffffffff
	str r1, [r0, #0x070]
	ldr r1, =0xffffffff
	str r1, [r0, #0x074]
	ldr r1, =0xffffffff
	str r1, [r0, #0x078]
	ldr r1, =0xffffffff
	str r1, [r0, #0x07c]
	ldr r1, =0xffffffff
	str r1, [r0, #0x080]
	ldr r1, =0xffffffff
	str r1, [r0, #0x084]

	ldr r0, =IOMUXC_BASE_ADDR
	ldr r1, =0x00080000
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
	str r1, [r0, #0x58c]
	str r1, [r0, #0x59c]
	str r1, [r0, #0x5a0]
	str r1, [r0, #0x78c]

	ldr r1, =0x00020000
	str r1, [r0, #0x750]
	ldr r1, =0x00003030
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
	ldr r1, =MMDC_P1_BASE_ADDR
	ldr r2, =0x00008000
	str r2, [r0, #0x1c]
	str r2, [r1, #0x1c]
	ldr r2, =0x1B5F0107
	str r2, [r0, #0x85c]
	str r2, [r1, #0x85c]
	ldr r2, =0xA1390003
	str r2, [r0, #0x800]
	ldr r2, =0x00400000
	str r2, [r0, #0x890]
	str r2, [r1, #0x890]
	ldr r2, =0x3C3A3A44
	str r2, [r0, #0x848]
	str r2, [r1, #0x848]
	ldr r2, =0x4238423A
	str r2, [r0, #0x850]
	str r2, [r1, #0x850]

	ldr r2, =0x20000000
	str r2, [r0, #0x83c]
	ldr r2, =0x00000000
	str r2, [r0, #0x840]
	ldr r2, =0x20000000
	str r2, [r1, #0x83c]
	ldr r2, =0x00000000
	str r2, [r1, #0x840]

	ldr r2, =0x33333333
	str r2, [r0, #0x81c]
	str r2, [r0, #0x820]
	str r2, [r0, #0x824]
	str r2, [r0, #0x828]
	str r2, [r1, #0x81c]
	str r2, [r1, #0x820]
	str r2, [r1, #0x824]
	str r2, [r1, #0x828]

	ldr r2, =0xf3333333
	str r2, [r0, #0x82c]
	str r2, [r0, #0x830]
	str r2, [r0, #0x834]
	str r2, [r0, #0x838]
	str r2, [r1, #0x82c]
	str r2, [r1, #0x830]
	str r2, [r1, #0x834]
	str r2, [r1, #0x838]

	ldr r2, =0x00000800
	str r2, [r0, #0x8b8]
	str r2, [r1, #0x8b8]

	ldr r2, =0x00020036
	str r2, [r0, #0x4]
	ldr r2, =0x00000000
	str r2, [r0, #0x8]
	ldr r2, =0x444961A5
	str r2, [r0, #0xc]
	ldr r2, =0x00160E83
	str r2, [r0, #0x10]
	ldr r2, =0x000000DD
	str r2, [r0, #0x14]

	ldr r2, =0x0000174C
	str r2, [r0, #0x18]
	ldr r2, =0x00008000
	str r2, [r0, #0x1c]
	ldr r2, =0x149F26D2
	str r2, [r0, #0x2c]
	ldr r2, =0x00000010
	str r2, [r0, #0x30]
	ldr r2, =0x0021099B
	str r2, [r0, #0x38]
	ldr r2, =0x0000004F
	str r2, [r0, #0x40]
	ldr r2, =0x11420000
	str r2, [r0, #0x400]
	ldr r2, =0x83110000
	str r2, [r0, #0x0]

	ldr r2, =0x00020036
	str r2, [r1, #0x4]
	ldr r2, =0x00000000
	str r2, [r1, #0x8]
	ldr r2, =0x444961A5
	str r2, [r1, #0xc]
	ldr r2, =0x00160E83
	str r2, [r1, #0x10]
	ldr r2, =0x000000DD
	str r2, [r1, #0x14]

	ldr r2, =0x0000174C
	str r2, [r1, #0x18]
	ldr r2, =0x00008000
	str r2, [r1, #0x1c]
	ldr r2, =0x149F26D2
	str r2, [r1, #0x2c]
	ldr r2, =0x00000010
	str r2, [r1, #0x30]
	ldr r2, =0x0021099B
	str r2, [r1, #0x38]
	ldr r2, =0x00000017
	str r2, [r1, #0x40]
	ldr r2, =0x11420000
	str r2, [r1, #0x400]
	ldr r2, =0x83110000
	str r2, [r1, #0x0]

	ldr r2, =0x003F8030
	str r2, [r0, #0x1c]
	ldr r2, =0xFF0A8030
	str r2, [r0, #0x1c]
	ldr r2, =0xC2018030
	str r2, [r0, #0x1c]
	ldr r2, =0x06028030
	str r2, [r0, #0x1c]
	ldr r2, =0x02038030
	str r2, [r0, #0x1c]

	ldr r2, =0x003F8030
	str r2, [r1, #0x1c]
	ldr r2, =0xFF0A8030
	str r2, [r1, #0x1c]
	ldr r2, =0xC2018030
	str r2, [r1, #0x1c]
	ldr r2, =0x06028030
	str r2, [r1, #0x1c]
	ldr r2, =0x02038030
	str r2, [r1, #0x1c]

	ldr r2, =0xA1390003
	str r2, [r0, #0x800]

	ldr r2, =0x00001800
	str r2, [r0, #0x20]
	str r2, [r1, #0x20]

	ldr r2, =0x00000000
	str r2, [r0, #0x818]
	str r2, [r1, #0x818]

	ldr r2, =0x00025576
	str r2, [r0, #0x4]
	str r2, [r1, #0x4]

	ldr r2, =0x00011006
	str r2, [r0, #0x404]
	str r2, [r1, #0x404]

	ldr r2, =0x00000000
	str r2, [r0, #0x1c]
	str r2, [r1, #0x1c]

	ldr r0, =IOMUXC_BASE_ADDR
	ldr r1, =0xF00000CF
	str r1, [r0, #0x10]
	ldr r1, =0x007F007F
	str r1, [r0, #0x18]
	ldr r1, =0x007F007F
	str r1, [r0, #0x1c]
.endm

.macro imx6_ddr_setting
	#if defined (CONFIG_MX6DQ_POP_LPDDR2)
        imx6dq_pop_arm2_lpddr2_setting
	#elif defined (CONFIG_MX6DQ_LPDDR2)
	imx6dqarm2_lpddr2_setting
	#elif defined (CONFIG_MX6Q)
	imx6dqarm2_ddr_setting
	#elif defined (CONFIG_MX6DL_LPDDR2)
	imx6dlarm2_lpddr2_setting
	#elif defined (CONFIG_MX6DL)
	imx6dlarm2_ddr_setting
	#else
	#error "SOC not configured"
	#endif
.endm

.macro imx6_clock_gating
.endm

.macro imx6_qos_setting
.endm

/* include the common plugin code here */
#include <asm/arch/mx6_plugin.S>
