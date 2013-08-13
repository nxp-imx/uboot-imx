/*
 * Copyright (C) 2012-2013 Freescale Semiconductor, Inc.
 *
 * Author: Fabio Estevam <fabio.estevam@freescale.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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
 */

#include <config.h>

#define ROM_API_TABLE_BASE_ADDR		0xC0
#define ROM_API_HWCNFG_SETUP_OFFSET	0x08

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

.macro imx6_ddr_setting
	#if defined (CONFIG_MX6Q)
	imx6dqarm2_ddr_setting
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
