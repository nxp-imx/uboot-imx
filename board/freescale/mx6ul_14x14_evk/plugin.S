/*
 * Copyright (C) 2015-2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <config.h>

/* DDR script */
.macro imx6ul_ddr3_evk_setting
	ldr r0, =IOMUXC_BASE_ADDR
	ldr r1, =0x000C0000
	str r1, [r0, #0x4B4]
	ldr r1, =0x00000000
	str r1, [r0, #0x4AC]
	ldr r1, =0x00000030
	str r1, [r0, #0x27C]
	ldr r1, =0x00000030
	str r1, [r0, #0x250]
	str r1, [r0, #0x24C]
	str r1, [r0, #0x490]
	str r1, [r0, #0x288]

	ldr r1, =0x00000000
	str r1, [r0, #0x270]

	ldr r1, =0x00000030
	str r1, [r0, #0x260]
	str r1, [r0, #0x264]
	str r1, [r0, #0x4A0]

	ldr r1, =0x00020000
	str r1, [r0, #0x494]

	ldr r1, =0x00000030
	str r1, [r0, #0x280]
	ldr r1, =0x00000030
	str r1, [r0, #0x284]

	ldr r1, =0x00020000
	str r1, [r0, #0x4B0]

	ldr r1, =0x00000030
	str r1, [r0, #0x498]
	str r1, [r0, #0x4A4]
	str r1, [r0, #0x244]
	str r1, [r0, #0x248]

	ldr r0, =MMDC_P0_BASE_ADDR
	ldr r1, =0x00008000
	str r1, [r0, #0x1C]
	ldr r1, =0xA1390003
	str r1, [r0, #0x800]
	ldr r1, =0x00000000
	str r1, [r0, #0x80C]
	ldr r1, =0x41570155
	str r1, [r0, #0x83C]
	ldr r1, =0x4040474A
	str r1, [r0, #0x848]
	ldr r1, =0x40405550
	str r1, [r0, #0x850]
	ldr r1, =0x33333333
	str r1, [r0, #0x81C]
	str r1, [r0, #0x820]
	ldr r1, =0xF3333333
	str r1, [r0, #0x82C]
	str r1, [r0, #0x830]
	ldr r1, =0x00921012
	str r1, [r0, #0x8C0]
	ldr r1, =0x00000800
	str r1, [r0, #0x8B8]
	ldr r1, =0x0002002D
	str r1, [r0, #0x004]
	ldr r1, =0x1B333030
	str r1, [r0, #0x008]
	ldr r1, =0x676B52F3
	str r1, [r0, #0x00C]
	ldr r1, =0xB66D0B63
	str r1, [r0, #0x010]
	ldr r1, =0x01FF00DB
	str r1, [r0, #0x014]
	ldr r1, =0x00201740
	str r1, [r0, #0x018]
	ldr r1, =0x00008000
	str r1, [r0, #0x01C]
	ldr r1, =0x000026D2
	str r1, [r0, #0x02C]
	ldr r1, =0x006B1023
	str r1, [r0, #0x030]
	ldr r1, =0x0000004F
	str r1, [r0, #0x040]
	ldr r1, =0x84180000
	str r1, [r0, #0x000]
	ldr r1, =0x23400A38
	str r1, [r0, #0x890]
	ldr r1, =0x02008032
	str r1, [r0, #0x01C]
	ldr r1, =0x00008033
	str r1, [r0, #0x01C]
	ldr r1, =0x00048031
	str r1, [r0, #0x01C]
	ldr r1, =0x15208030
	str r1, [r0, #0x01C]
	ldr r1, =0x04008040
	str r1, [r0, #0x01C]
	ldr r1, =0x00000800
	str r1, [r0, #0x020]
	ldr r1, =0x00000227
	str r1, [r0, #0x818]
	ldr r1, =0x0002552D
	str r1, [r0, #0x004]
	ldr r1, =0x00011006
	str r1, [r0, #0x404]
	ldr r1, =0x00000000
	str r1, [r0, #0x01C]
.endm

.macro imx6ul_ddr3_eol_evk_setting
	ldr r0, =IOMUXC_BASE_ADDR
	ldr r1, =0x000C0000
	str r1, [r0, #0x4B4]
	ldr r1, =0x00000000
	str r1, [r0, #0x4AC]
	ldr r1, =0x00000030
	str r1, [r0, #0x27C]
	ldr r1, =0x00000030
	str r1, [r0, #0x250]
	str r1, [r0, #0x24C]
	str r1, [r0, #0x490]
	str r1, [r0, #0x288]

	ldr r1, =0x00000000
	str r1, [r0, #0x270]

	ldr r1, =0x00000030
	str r1, [r0, #0x260]
	str r1, [r0, #0x264]
	str r1, [r0, #0x4A0]

	ldr r1, =0x00020000
	str r1, [r0, #0x494]

	ldr r1, =0x00000030
	str r1, [r0, #0x280]
	ldr r1, =0x00000030
	str r1, [r0, #0x284]

	ldr r1, =0x00020000
	str r1, [r0, #0x4B0]

	ldr r1, =0x00000030
	str r1, [r0, #0x498]
	str r1, [r0, #0x4A4]
	str r1, [r0, #0x244]
	str r1, [r0, #0x248]

	ldr r0, =MMDC_P0_BASE_ADDR
	ldr r1, =0x00008000
	str r1, [r0, #0x1C]
	ldr r1, =0xA1390003
	str r1, [r0, #0x800]
	ldr r1, =0x00000000
	str r1, [r0, #0x80C]
	ldr r1, =0x41490145
	str r1, [r0, #0x83C]
	ldr r1, =0x40404546
	str r1, [r0, #0x848]
	ldr r1, =0x4040524D
	str r1, [r0, #0x850]
	ldr r1, =0x33333333
	str r1, [r0, #0x81C]
	str r1, [r0, #0x820]
	ldr r1, =0xF3333333
	str r1, [r0, #0x82C]
	str r1, [r0, #0x830]
	ldr r1, =0x00921012
	str r1, [r0, #0x8C0]
	ldr r1, =0x00000800
	str r1, [r0, #0x8B8]
	ldr r1, =0x0002002D
	str r1, [r0, #0x004]
	ldr r1, =0x00333030
	str r1, [r0, #0x008]
	ldr r1, =0x676B52F3
	str r1, [r0, #0x00C]
	ldr r1, =0xB66D8B63
	str r1, [r0, #0x010]
	ldr r1, =0x01FF00DB
	str r1, [r0, #0x014]
	ldr r1, =0x00201740
	str r1, [r0, #0x018]
	ldr r1, =0x00008000
	str r1, [r0, #0x01C]
	ldr r1, =0x000026D2
	str r1, [r0, #0x02C]
	ldr r1, =0x006B1023
	str r1, [r0, #0x030]
	ldr r1, =0x0000004F
	str r1, [r0, #0x040]
	ldr r1, =0x84180000
	str r1, [r0, #0x000]
	ldr r1, =0x02008032
	str r1, [r0, #0x01C]
	ldr r1, =0x00008033
	str r1, [r0, #0x01C]
	ldr r1, =0x00048031
	str r1, [r0, #0x01C]
	ldr r1, =0x15208030
	str r1, [r0, #0x01C]
	ldr r1, =0x04008040
	str r1, [r0, #0x01C]
	ldr r1, =0x00000800
	str r1, [r0, #0x020]
	ldr r1, =0x00000227
	str r1, [r0, #0x818]
	ldr r1, =0x0002552D
	str r1, [r0, #0x004]
	ldr r1, =0x00011006
	str r1, [r0, #0x404]
	ldr r1, =0x00000000
	str r1, [r0, #0x01C]
.endm

.macro imx6ul_lpddr2_evk_setting
	ldr r0, =IOMUXC_BASE_ADDR
	ldr r1, =0x00080000
	str r1, [r0, #0x4B4]
	ldr r1, =0x00000000
	str r1, [r0, #0x4AC]
	ldr r1, =0x00000030
	str r1, [r0, #0x27C]
	str r1, [r0, #0x250]
	str r1, [r0, #0x24C]
	str r1, [r0, #0x490]
	str r1, [r0, #0x288]

	ldr r1, =0x00000000
	str r1, [r0, #0x270]
	str r1, [r0, #0x260]
	str r1, [r0, #0x264]

	ldr r1, =0x00000030
	str r1, [r0, #0x4A0]

	ldr r1, =0x00020000
	str r1, [r0, #0x494]

	ldr r1, =0x00003030
	str r1, [r0, #0x280]
	ldr r1, =0x00003030
	str r1, [r0, #0x284]

	ldr r1, =0x00020000
	str r1, [r0, #0x4B0]

	ldr r1, =0x00000030
	str r1, [r0, #0x498]
	str r1, [r0, #0x4A4]
	str r1, [r0, #0x244]
	str r1, [r0, #0x248]

	ldr r0, =MMDC_P0_BASE_ADDR
	ldr r1, =0x00008000
	str r1, [r0, #0x1C]
	ldr r1, =0x1b4700c7
	str r1, [r0, #0x85c]
	ldr r1, =0xA1390003
	str r1, [r0, #0x800]
	ldr r1, =0x00470000
	str r1, [r0, #0x890]
	ldr r1, =0x00000800
	str r1, [r0, #0x8b8]
	ldr r1, =0x33333333
	str r1, [r0, #0x81C]
	str r1, [r0, #0x820]
	ldr r1, =0xF3333333
	str r1, [r0, #0x82C]
	str r1, [r0, #0x830]
	ldr r1, =0x20000000
	str r1, [r0, #0x83C]
	ldr r1, =0x4040484F
	str r1, [r0, #0x848]
	ldr r1, =0x40405247
	str r1, [r0, #0x850]
	ldr r1, =0x00922012
	str r1, [r0, #0x8C0]
	ldr r1, =0x00000800
	str r1, [r0, #0x8B8]

	ldr r1, =0x00020012
	str r1, [r0, #0x004]
	ldr r1, =0x00000000
	str r1, [r0, #0x008]
	ldr r1, =0x33374133
	str r1, [r0, #0x00C]
	ldr r1, =0x00100A82
	str r1, [r0, #0x010]
	ldr r1, =0x00170557
	str r1, [r0, #0x038]
	ldr r1, =0x00000093
	str r1, [r0, #0x014]
	ldr r1, =0x00001748
	str r1, [r0, #0x018]
	ldr r1, =0x00008000
	str r1, [r0, #0x01C]
	ldr r1, =0x0F9F0682
	str r1, [r0, #0x02C]
	ldr r1, =0x009F0010
	str r1, [r0, #0x030]
	ldr r1, =0x00000047
	str r1, [r0, #0x040]
	ldr r1, =0x83100000
	str r1, [r0, #0x000]
	ldr r1, =0x00008050
	str r1, [r0, #0x01C]
	ldr r1, =0x003F8030
	str r1, [r0, #0x01C]
	ldr r1, =0xFF0A8030
	str r1, [r0, #0x01C]
	ldr r1, =0x82018030
	str r1, [r0, #0x01C]
	ldr r1, =0x04028030
	str r1, [r0, #0x01C]
	ldr r1, =0x01038030
	str r1, [r0, #0x01C]
	ldr r1, =0x00001800
	str r1, [r0, #0x020]
	ldr r1, =0x00000000
	str r1, [r0, #0x818]
	ldr r1, =0xA1310003
	str r1, [r0, #0x800]
	ldr r1, =0x00025576
	str r1, [r0, #0x004]
	ldr r1, =0x00010106
	str r1, [r0, #0x404]
	ldr r1, =0x00000000
	str r1, [r0, #0x01C]
.endm

.macro imx6_clock_gating
	ldr r0, =CCM_BASE_ADDR
	ldr r1, =0xFFFFFFFF
	str r1, [r0, #0x68]
	str r1, [r0, #0x6C]
	str r1, [r0, #0x70]
	str r1, [r0, #0x74]
	str r1, [r0, #0x78]
	str r1, [r0, #0x7C]
	str r1, [r0, #0x80]
.endm

.macro imx6_qos_setting
.endm

.macro imx6_ddr_setting
#if defined (CONFIG_TARGET_MX6UL_9X9_EVK)
	imx6ul_lpddr2_evk_setting
#elif defined(CONFIG_DDR3L_MT41K256M16HA)
	imx6ul_ddr3_eol_evk_setting
#else
	imx6ul_ddr3_evk_setting
#endif
.endm

/* include the common plugin code here */
#include <asm/arch/mx6_plugin.S>
