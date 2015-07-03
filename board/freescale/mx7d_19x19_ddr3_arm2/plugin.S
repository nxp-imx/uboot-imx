/*
 * Copyright (C) 2014-2015 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <config.h>

/* DDR script */
.macro imx7d_19x19_ddr3_arm2_ddr_setting
	/* Configure ocram_epdc */
	ldr r0, =IOMUXC_GPR_BASE_ADDR
	ldr r1, =0x4f400005
	str r1, [r0, #0x4]

	ldr r0, =SRC_BASE_ADDR
	ldr r1, =0x2
	ldr r2, =0x1000
	str r1, [r0, r2]

	ldr r0, =DDRC_IPS_BASE_ADDR
	ldr r1, =0x03040001
	str r1, [r0]
	ldr r1, =0x80400003
	str r1, [r0, #0x1a0]
	ldr r1, =0x00100020
	str r1, [r0, #0x1a4]
	ldr r1, =0x80100004
	str r1, [r0, #0x1a8]
	ldr r1, =0x0040005e
	str r1, [r0, #0x64]
	ldr r1, =0x1
	str r1, [r0, #0x490]
	ldr r1, =0x00020001
	str r1, [r0, #0xd0]
	ldr r1, =0x00010000
	str r1, [r0, #0xd4]
	ldr r1, =0x09300004
	str r1, [r0, #0xdc]
	ldr r1, =0x04080000
	str r1, [r0, #0xe0]
	ldr r1, =0x00090004
	str r1, [r0, #0xe4]
	ldr r1, =0x33f
	str r1, [r0, #0xf4]
	ldr r1, =0x0908120a
	str r1, [r0, #0x100]
	ldr r1, =0x0002020e
	str r1, [r0, #0x104]
	ldr r1, =0x03040407
	str r1, [r0, #0x108]
	ldr r1, =0x00002006
	str r1, [r0, #0x10c]
	ldr r1, =0x04020204
	str r1, [r0, #0x110]
	ldr r1, =0x03030202
	str r1, [r0, #0x114]
	ldr r1, =0x03030803
	str r1, [r0, #0x120]
	ldr r1, =0x00800020
	str r1, [r0, #0x180]
	ldr r1, =0x02098204
	str r1, [r0, #0x190]
	ldr r1, =0x00030303
	str r1, [r0, #0x194]

	ldr r1, =0x00000016
	str r1, [r0, #0x200]
	ldr r1, =0x00171717
	str r1, [r0, #0x204]
	ldr r1, =0x04040404
	str r1, [r0, #0x214]
	ldr r1, =0x00040404
	str r1, [r0, #0x218]

	ldr r1, =0x06000601
	str r1, [r0, #0x240]
	ldr r1, =0x00001323
	str r1, [r0, #0x244]

	ldr r0, =SRC_BASE_ADDR
	mov r1, #0x0
	ldr r2, =0x1000
	str r1, [r0, r2]

	ldr r0, =DDRPHY_IPS_BASE_ADDR
	ldr r1, =0x17420f40
	str r1, [r0]
	ldr r1, =0x10210100
	str r1, [r0, #0x4]
	ldr r1, =0x00060807
	str r1, [r0, #0x10]
	ldr r1, =0x00000d6e
	str r1, [r0, #0x9c]
	ldr r1, =0x08080808
	str r1, [r0, #0x20]
	ldr r1, =0x08080808
	str r1, [r0, #0x30]
	ldr r1, =0x01000010
	str r1, [r0, #0x50]

	ldr r1, =0x0e407304
	str r1, [r0, #0xc0]
	ldr r1, =0x0e447304
	str r1, [r0, #0xc0]
	ldr r1, =0x0e447306
	str r1, [r0, #0xc0]

wait_zq:
	ldr r1, [r0, #0xc4]
	tst r1, #0x1
	beq wait_zq

	ldr r1, =0x0e447304
	str r1, [r0, #0xc0]
	ldr r1, =0x0e407304
	str r1, [r0, #0xc0]

	ldr r0, =CCM_BASE_ADDR
	mov r1, #0x0
	ldr r2, =0x4130
	str r1, [r0, r2]
	ldr r0, =IOMUXC_GPR_BASE_ADDR
	mov r1, #0x178
	str r1, [r0, #0x20]
	ldr r0, =CCM_BASE_ADDR
	mov r1, #0x2
	ldr r2, =0x4130
	str r1, [r0, r2]
	ldr r0, =DDRPHY_IPS_BASE_ADDR
	ldr r1, =0x0000000f
	str r1, [r0, #0x18]

	ldr r0, =DDRC_IPS_BASE_ADDR
wait_stat:
	ldr r1, [r0, #0x4]
	tst r1, #0x1
	beq wait_stat
.endm

.macro imx7_clock_gating
.endm

.macro imx7_qos_setting
.endm

.macro imx7_ddr_setting
	imx7d_19x19_ddr3_arm2_ddr_setting
.endm

/* include the common plugin code here */
#include <asm/arch/mx7_plugin.S>
