/*
 * Copyright (C) 2014 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <config.h>

/* DDR script */
.macro imx6slevk_ddr_setting
	ldr r0, =CCM_BASE_ADDR
	ldr r1, =0x00260324
	str r1, [r0, #0x018]

	ldr r0, =IOMUXC_BASE_ADDR
	ldr r1, =0x00020000
	str r1, [r0, #0x5c0]
	ldr r1, =0x00000000
	str r1, [r0, #0x5b4]

	ldr r1, =0x00000028
	str r1, [r0, #0x338]

	ldr r1, =0x00000030
	str r1, [r0, #0x300]
	str r1, [r0, #0x31c]
	str r1, [r0, #0x320]

	ldr r1, =0x00000000
	str r1, [r0, #0x32c]

	ldr r1, =0x00000030
	str r1, [r0, #0x5ac]
	str r1, [r0, #0x5c8]

	ldr r1, =0x00020000
	str r1, [r0, #0x5b0]

	ldr r1, =0x00003030
	str r1, [r0, #0x344]
	str r1, [r0, #0x348]
	str r1, [r0, #0x34c]
	str r1, [r0, #0x350]

	ldr r1, =0x00080000
	str r1, [r0, #0x5d0]

	ldr r1, =0x00000030
	str r1, [r0, #0x5c4]
	str r1, [r0, #0x5cc]
	str r1, [r0, #0x5d4]
	str r1, [r0, #0x5d8]

	str r1, [r0, #0x30c]
	str r1, [r0, #0x310]
	str r1, [r0, #0x314]
	str r1, [r0, #0x318]

	ldr r0, =MMDC_P0_BASE_ADDR
	ldr r2, =0x00008000
	str r2, [r0, #0x01c]

	ldr r2, =0x1b4700c7
	str r2, [r0, #0x85c]

	ldr r2, =0xa1390003
	str r2, [r0, #0x800]

	ldr r2, =0x00300000
	str r2, [r0, #0x890]

	ldr r2, =0x00000800
	str r2, [r0, #0x8b8]

	ldr r2, =0x33333333
	str r2, [r0, #0x81c]
	str r2, [r0, #0x820]
	str r2, [r0, #0x824]
	str r2, [r0, #0x828]
	str r2, [r0, #0x82c]
	str r2, [r0, #0x830]
	str r2, [r0, #0x834]
	str r2, [r0, #0x838]

	ldr r2, =0x4241444a
	str r2, [r0, #0x848]

	ldr r2, =0x3030312b
	str r2, [r0, #0x850]

	ldr r2, =0x20000000
	str r2, [r0, #0x83c]

	ldr r2, =0x00000000
	str r2, [r0, #0x840]

	ldr r2, =0x00000800
	str r2, [r0, #0x8b8]

	ldr r2, =0x33374133
	str r2, [r0, #0x00c]

	ldr r2, =0x00020024
	str r2, [r0, #0x004]

	ldr r2, =0x00100A82
	str r2, [r0, #0x010]
	ldr r2, =0x00000093
	str r2, [r0, #0x014]
	ldr r2, =0x00001688
	str r2, [r0, #0x018]
	ldr r2, =0x0F9F26D2
	str r2, [r0, #0x02c]

	ldr r2, =0x0000020E
	str r2, [r0, #0x030]
	ldr r2, =0x00190778
	str r2, [r0, #0x038]
	ldr r2, =0x00000000
	str r2, [r0, #0x008]
	ldr r2, =0x0000004F
	str r2, [r0, #0x040]
	ldr r2, =0xC3110000
	str r2, [r0, #0x000]

	ldr r2, =0x003F8030
	str r2, [r0, #0x01c]
	ldr r2, =0xFF0A8030
	str r2, [r0, #0x01c]
	ldr r2, =0x82018030
	str r2, [r0, #0x01c]
	ldr r2, =0x04028030
	str r2, [r0, #0x01c]
	ldr r2, =0x02038030
	str r2, [r0, #0x01c]
	ldr r2, =0xFF0A8038
	str r2, [r0, #0x01c]
	ldr r2, =0x82018038
	str r2, [r0, #0x01c]
	ldr r2, =0x04028038
	str r2, [r0, #0x01c]
	ldr r2, =0x02038038
	str r2, [r0, #0x01c]

	ldr r2, =0xa1310003
	str r2, [r0, #0x800]

	ldr r2, =0x00001800
	str r2, [r0, #0x020]

	ldr r2, =0x00000000
	str r2, [r0, #0x818]

	ldr r2, =0x00000800
	str r2, [r0, #0x8b8]

	ldr r2, =0x00025564
	str r2, [r0, #0x004]

	ldr r2, =0x00011006
	str r2, [r0, #0x404]

	ldr r2, =0x00000000
	str r2, [r0, #0x01c]
.endm
.macro imx6_clock_gating
	ldr r0, =CCM_BASE_ADDR
	ldr r1, =0xffffffff
	str r1, [r0, #0x068]
	str r1, [r0, #0x06c]
	str r1, [r0, #0x070]
	str r1, [r0, #0x074]
	str r1, [r0, #0x078]
	str r1, [r0, #0x07c]
	str r1, [r0, #0x080]
	str r1, [r0, #0x084]
.endm

.macro imx6_qos_setting
.endm

.macro imx6_ddr_setting
	imx6slevk_ddr_setting
.endm

/* include the common plugin code here */
#include <asm/arch/mx6_plugin.S>
