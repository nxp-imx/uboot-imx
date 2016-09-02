/*
 * Copyright (C) 2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <config.h>

.macro imx6dq_scm_lpddr2_512mb_setting
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

	/* DDR clock to 400MHz */
	ldr r1, =0x00060324
	str r1, [r0, #0x018]
	/* AHB_ROOT_CLK change divide ratio from 4 to 3 for ENET */
	ldr r1, =0x00018900
	str r1, [r0, #0x014]

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
	ldr r1, =0x00000000
	str r1, [r0, #0x58c]
	ldr r1, =0x00000030
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

	ldr r1, =0x00000020
	str r1, [r0, #0x590]
	str r1, [r0, #0x598]


	ldr r0, =MMDC_P0_BASE_ADDR

	/* DDR setup */
	ldr r2, =0x00008000
	str r2, [r0, #0x1c]

	/*SCM CONF*/
	ldr r2, =0x1B4700C7
	str r2, [r0, #0x85c]

	ldr r2, =0xA1390003
	str r2, [r0, #0x800]

	/* calibration required */
	ldr r2, =0x00400000
	str r2, [r0, #0x890]

	ldr r2, =0x44404044
	str r2, [r0, #0x848]

	ldr r2, =0x34343A38
	str r2, [r0, #0x850]

	ldr r2, =0x20000000
	str r2, [r0, #0x83c]
	ldr r2, =0x00000000
	str r2, [r0, #0x840]

	ldr r2, =0x33333333
	str r2, [r0, #0x81c]
	str r2, [r0, #0x820]
	str r2, [r0, #0x824]
	str r2, [r0, #0x828]
	ldr r2, =0xf3333333
	str r2, [r0, #0x82c]
	str r2, [r0, #0x830]
	str r2, [r0, #0x834]
	str r2, [r0, #0x838]

	ldr r2, =0x00000800
	str r2, [r0, #0x8b8]

	ldr r2, =0x00020036
	str r2, [r0, #0x4]
	ldr r2, =0x00000000
	str r2, [r0, #0x8]
	ldr r2, =0x33374133
	str r2, [r0, #0xc]

	ldr r2, =0x00100a82
	str r2, [r0, #0x10]

	ldr r2, =0x00000093
	str r2, [r0, #0x14]

	ldr r2, =0x0000174C
	str r2, [r0, #0x18]
	ldr r2, =0x00008000
	str r2, [r0, #0x1c]
	ldr r2, =0x0f9f26d2
	str r2, [r0, #0x2c]
	ldr r2, =0x00000010
	str r2, [r0, #0x30]
	ldr r2, =0x00190778
	str r2, [r0, #0x38]

	ldr r2, =0x0000004F
	str r2, [r0, #0x40]

	ldr r2, =0x83110000
	str r2, [r0, #0x0]

	/* Channel 0 */
	ldr r2, =0x003F8030
	str r2, [r0, #0x1c]
	ldr r2, =0xFF0A8030
	str r2, [r0, #0x1c]
	ldr r2, =0x82018030
	str r2, [r0, #0x1c]
	ldr r2, =0x04028030
	str r2, [r0, #0x1c]
	ldr r2, =0x04038030
	str r2, [r0, #0x1c]

	ldr r2, =0xA1390003
	str r2, [r0, #0x800]

	ldr r2, =0x00001800
	str r2, [r0, #0x20]

	ldr r2, =0x00000000
	str r2, [r0, #0x818]

	ldr r2, =0x00025576
	str r2, [r0, #0x4]

	ldr r2, =0x00011006
	str r2, [r0, #0x404]

	ldr r2, =0x00000000
	str r2, [r0, #0x1c]

	ldr r0, =IOMUXC_BASE_ADDR
	ldr r1, =0xF00000CF
	str r1, [r0, #0x10]
	ldr r1, =0x007F007F
	str r1, [r0, #0x18]
	ldr r1, =0x007F007F
	str r1, [r0, #0x1c]
.endm


.macro imx6dq_scm_lpddr2_1gb_setting
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

	/* DDR clock to 400MHz */
	ldr r1, =0x00060324
	str r1, [r0, #0x018]
	/* AHB_ROOT_CLK change divide ratio from 4 to 3 for ENET */
	ldr r1, =0x00018900
	str r1, [r0, #0x014]

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
	ldr r1, =0x00000000
	str r1, [r0, #0x58c]
	ldr r1, =0x00000030
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

	ldr r1, =0x00000020
	str r1, [r0, #0x590]
	str r1, [r0, #0x598]


	ldr r0, =MMDC_P0_BASE_ADDR
	ldr r1, =MMDC_P1_BASE_ADDR

	/* DDR setup */
	ldr r2, =0x00008000
	str r2, [r0, #0x1c]
	str r2, [r1, #0x1c]

	/*SCM CONF*/
	ldr r2, =0x1B4700C7
	str r2, [r0, #0x85c]
	str r2, [r1, #0x85c]

	ldr r2, =0xA1390003
	str r2, [r0, #0x800]

	/* calibration required */
	ldr r2, =0x00400000
	str r2, [r0, #0x890]
	str r2, [r1, #0x890]

	ldr r2, =0x44404044
	str r2, [r0, #0x848]
	ldr r2, =0x44443A46
	str r2, [r1, #0x848]

	ldr r2, =0x34343A38
	str r2, [r0, #0x850]
	ldr r2, =0x3E2E483C
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
	ldr r2, =0x33374133
	str r2, [r0, #0xc]

	ldr r2, =0x00100a82
	str r2, [r0, #0x10]

	ldr r2, =0x00000093
	str r2, [r0, #0x14]

	ldr r2, =0x0000174C
	str r2, [r0, #0x18]
	ldr r2, =0x00008000
	str r2, [r0, #0x1c]
	ldr r2, =0x0f9f26d2
	str r2, [r0, #0x2c]
	ldr r2, =0x00000010
	str r2, [r0, #0x30]
	ldr r2, =0x00190778
	str r2, [r0, #0x38]
#ifdef CONFIG_INTERLEAVING_MODE
	ldr r2, =0x00000053
#else
	ldr r2, =0x0000004F
#endif
	str r2, [r0, #0x40]

	ldr r2, =0x83110000
	str r2, [r0, #0x0]

	ldr r2, =0x00020036
	str r2, [r1, #0x4]
	ldr r2, =0x00000000
	str r2, [r1, #0x8]
	ldr r2, =0x33374133
	str r2, [r1, #0xc]
	ldr r2, =0x00100a82
	str r2, [r1, #0x10]
	ldr r2, =0x00000093
	str r2, [r1, #0x14]

	ldr r2, =0x0000174C
	str r2, [r1, #0x18]
	ldr r2, =0x00008000
	str r2, [r1, #0x1c]

	ldr r2, =0x0f9f26d2
	str r2, [r1, #0x2c]

	ldr r2, =0x00000010
	str r2, [r1, #0x30]

	ldr r2, =0x00190778
	str r2, [r1, #0x38]

#ifdef CONFIG_INTERLEAVING_MODE
	ldr r2, =0x00000013
#else
	ldr r2, =0x00000017
#endif
	str r2, [r1, #0x40]

	ldr r2, =0x83110000
	str r2, [r1, #0x0]

	/* Channel 0 */
	ldr r2, =0x003F8030
	str r2, [r0, #0x1c]
	ldr r2, =0xFF0A8030
	str r2, [r0, #0x1c]
	ldr r2, =0x82018030
	str r2, [r0, #0x1c]
	ldr r2, =0x04028030
	str r2, [r0, #0x1c]
	ldr r2, =0x04038030
	str r2, [r0, #0x1c]

	/* Channel 1 */
	ldr r2, =0x003F8030
	str r2, [r1, #0x1c]
	ldr r2, =0xFF0A8030
	str r2, [r1, #0x1c]
	ldr r2, =0x82018030
	str r2, [r1, #0x1c]
	ldr r2, =0x04028030
	str r2, [r1, #0x1c]
	ldr r2, =0x04038030
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


.macro imx6dq_scm_lpddr2_2gb_setting
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

	/* DDR clock to 400MHz */
	ldr r1, =0x00060324
	str r1, [r0, #0x018]
	/* AHB_ROOT_CLK change divide ratio from 4 to 3 for ENET */
	ldr r1, =0x00018900
	str r1, [r0, #0x014]

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
	ldr r1, =0x00000000
	str r1, [r0, #0x58c]
	ldr r1, =0x00000030
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

	ldr r1, =0x00000020
	str r1, [r0, #0x590]
	str r1, [r0, #0x598]


	ldr r0, =MMDC_P0_BASE_ADDR
	ldr r1, =MMDC_P1_BASE_ADDR

	/* DDR setup */
	ldr r2, =0x00008000
	str r2, [r0, #0x1c]
	str r2, [r1, #0x1c]

	/*SCM CONF*/
	ldr r2, =0x1B4700C7
	str r2, [r0, #0x85c]
	str r2, [r1, #0x85c]

	ldr r2, =0xA1390003
	str r2, [r0, #0x800]

	/* calibration required */
	ldr r2, =0x00400000
	str r2, [r0, #0x890]
	str r2, [r1, #0x890]

	ldr r2, =0x44404044
	str r2, [r0, #0x848]
	ldr r2, =0x44443A46
	str r2, [r1, #0x848]

	ldr r2, =0x34343A38
	str r2, [r0, #0x850]
	ldr r2, =0x3E2E483C
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
	ldr r2, =0x00000000
	str r2, [r1, #0x824]
	ldr r2, =0x33333333
	str r2, [r1, #0x828]
	ldr r2, =0xf3333333
	str r2, [r0, #0x82c]
	str r2, [r0, #0x830]
	str r2, [r0, #0x834]
	str r2, [r0, #0x838]
	str r2, [r1, #0x82c]
	str r2, [r1, #0x830]
	ldr r2, =0x00000000
	str r2, [r1, #0x834]
	ldr r2, =0xf3333333
	str r2, [r1, #0x838]

	ldr r2, =0x00000800
	str r2, [r0, #0x8b8]
	str r2, [r1, #0x8b8]

	ldr r2, =0x00020036
	str r2, [r0, #0x4]
	ldr r2, =0x00000000
	str r2, [r0, #0x8]
	ldr r2, =0x33374133
	str r2, [r0, #0xc]

	ldr r2, =0x00100a82
	str r2, [r0, #0x10]

	ldr r2, =0x00000093
	str r2, [r0, #0x14]

	ldr r2, =0x0000174C
	str r2, [r0, #0x18]
	ldr r2, =0x00008000
	str r2, [r0, #0x1c]
	ldr r2, =0x0f9f26d2
	str r2, [r0, #0x2c]
	ldr r2, =0x009F0E10
	str r2, [r0, #0x30]
	ldr r2, =0x00190778
	str r2, [r0, #0x38]
#ifdef CONFIG_INTERLEAVING_MODE
	ldr r2, =0x00000063
#else
	ldr r2, =0x0000005F
#endif
	str r2, [r0, #0x40]

	ldr r2, =0xC3110000
	str r2, [r0, #0x0]

	ldr r2, =0x00020036
	str r2, [r1, #0x4]
	ldr r2, =0x00000000
	str r2, [r1, #0x8]
	ldr r2, =0x33374133
	str r2, [r1, #0xc]
	ldr r2, =0x00100a82
	str r2, [r1, #0x10]
	ldr r2, =0x00000093
	str r2, [r1, #0x14]

	ldr r2, =0x0000174C
	str r2, [r1, #0x18]
	ldr r2, =0x00008000
	str r2, [r1, #0x1c]

	ldr r2, =0x0f9f26d2
	str r2, [r1, #0x2c]

	ldr r2, =0x00000010
	str r2, [r1, #0x30]

	ldr r2, =0x00190778
	str r2, [r1, #0x38]

#ifdef CONFIG_INTERLEAVING_MODE
	ldr r2, =0x00000023
#else
	ldr r2, =0x00000027
#endif
	str r2, [r1, #0x40]

	ldr r2, =0xC3110000
	str r2, [r1, #0x0]

	/* Channel 0 */
	/* CS0 */
	ldr r2, =0x003F8030
	str r2, [r0, #0x1c]
	ldr r2, =0xFF0A8030
	str r2, [r0, #0x1c]
	ldr r2, =0x82018030
	str r2, [r0, #0x1c]
	ldr r2, =0x04028030
	str r2, [r0, #0x1c]
	ldr r2, =0x04038030
	str r2, [r0, #0x1c]
	/* CS1 */
	ldr r2, =0x003F8038
	str r2, [r0, #0x1c]
	ldr r2, =0xFF0A8038
	str r2, [r0, #0x1c]
	ldr r2, =0x82018038
	str r2, [r0, #0x1c]
	ldr r2, =0x04028038
	str r2, [r0, #0x1c]
	ldr r2, =0x04038038
	str r2, [r0, #0x1c]

	/* Channel 1 */
	/* CS0 */
	ldr r2, =0x003F8030
	str r2, [r1, #0x1c]
	ldr r2, =0xFF0A8030
	str r2, [r1, #0x1c]
	ldr r2, =0x82018030
	str r2, [r1, #0x1c]
	ldr r2, =0x04028030
	str r2, [r1, #0x1c]
	ldr r2, =0x04038030
	str r2, [r1, #0x1c]
	/* CS1 */
	ldr r2, =0x003F8038
	str r2, [r1, #0x1c]
	ldr r2, =0xFF0A8038
	str r2, [r1, #0x1c]
	ldr r2, =0x82018038
	str r2, [r1, #0x1c]
	ldr r2, =0x04028038
	str r2, [r1, #0x1c]
	ldr r2, =0x04038038
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
#if defined (CONFIG_SCM_LPDDR2_512MB)
	imx6dq_scm_lpddr2_512mb_setting
#elif defined (CONFIG_SCM_LPDDR2_2GB)
	imx6dq_scm_lpddr2_2gb_setting
#else
	imx6dq_scm_lpddr2_1gb_setting
#endif
.endm

.macro imx6_clock_gating
.endm

.macro imx6_qos_setting
.endm

/* include the common plugin code here */
#include <asm/arch/mx6_plugin.S>
