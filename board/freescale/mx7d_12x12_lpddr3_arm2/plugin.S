/*
 * Copyright (C) 2014-2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <config.h>

/* DDR script */
.macro imx7d_ddrphy_latency_setting
	ldr r2, =ANATOP_BASE_ADDR
	ldr r3, [r2, #0x800]
	and r3, r3, #0xFF
	cmp r3, #0x11
	bne TUNE_END

	/*TO 1.1*/
	ldr r1, =0x1c1c1c1c
	str r1, [r0, #0x7c]
	ldr r1, =0x1c1c1c1c
	str r1, [r0, #0x80]
	ldr r1, =0x30301c1c
	str r1, [r0, #0x84]
	ldr r1, =0x00000030
	str r1, [r0, #0x88]
	ldr r1, =0x30303030
	str r1, [r0, #0x6c]

TUNE_END:
.endm

.macro imx7d_12x12_lpddr3_arm2_setting

	/* check whether it is a LPSR resume */
	ldr	r1, =0x30270000
	ldr	r7, [r1]
	cmp	r7, #0
	beq	16f

	/* disable wdog powerdown counter */
	ldr	r0, =0x30280000
	ldrh	r1, =0x0
	strh	r1, [r0, #0x8]

	/* initialize AIPs 1-3 port */
	ldr	r0, =0x301f0000
	ldr	r1, =0x77777777
	str	r1, [r0]
	str	r1, [r0, #0x4]
	ldr	r1, =0x0
	str	r1, [r0, #0x40]
	str	r1, [r0, #0x44]
	str	r1, [r0, #0x48]
	str	r1, [r0, #0x4c]
	str	r1, [r0, #0x50]

	ldr	r0, =0x305f0000
	ldr	r1, =0x77777777
	str	r1, [r0]
	str	r1, [r0, #0x4]
	ldr	r1, =0x0
	str	r1, [r0, #0x40]
	str	r1, [r0, #0x44]
	str	r1, [r0, #0x48]
	str	r1, [r0, #0x4c]
	str	r1, [r0, #0x50]

	ldr	r0, =0x309f0000
	ldr	r1, =0x77777777
	str	r1, [r0]
	str	r1, [r0, #0x4]
	ldr	r1, =0x0
	str	r1, [r0, #0x40]
	str	r1, [r0, #0x44]
	str	r1, [r0, #0x48]
	str	r1, [r0, #0x4c]
	str	r1, [r0, #0x50]

	ldr	r1, =0x30360000
	ldr	r2, =0x30390000
	ldr	r3, =0x307a0000
	ldr	r4, =0x30790000
	ldr	r10, =0x30380000
	ldr	r11, =0x30340000

	/* turn on ddr power */
	ldr	r7, =(0x1 << 29)
	str	r7, [r1, #0x388]

	ldr	r6, =50
1:
	subs	r6, r6, #0x1
	bne	1b

	/* clear ddr_phy reset */
	ldr	r6, =0x1000
	ldr	r7, [r2, r6]
	orr	r7, r7, #0x3
	str	r7, [r2, r6]
	ldr	r7, [r2, r6]
	bic	r7, r7, #0x1
	str	r7, [r2, r6]

	/* restore DDRC */
	ldr	r6, =0x0
	ldr	r7, =0x03040008
	str	r7, [r3, r6]

	ldr	r6, =0x1a0
	ldr	r7, =0x80400003
	str	r7, [r3, r6]

	ldr	r6, =0x1a4
	ldr	r7, =0x00100020
	str	r7, [r3, r6]

	ldr	r6, =0x1a8
	ldr	r7, =0x80100004
	str	r7, [r3, r6]

	ldr	r6, =0x64
	ldr	r7, =0x00200038
	str	r7, [r3, r6]

	ldr	r6, =0xd0
	ldr	r7, =0xc0350001
	str	r7, [r3, r6]

	ldr	r6, =0xdc
	ldr	r7, =0x00C3000A
	str	r7, [r3, r6]

	ldr	r6, =0xe0
	ldr	r7, =0x00010000
	str	r7, [r3, r6]

	ldr	r6, =0xe4
	ldr	r7, =0x00110006
	str	r7, [r3, r6]

	ldr	r6, =0xf4
	ldr	r7, =0x0000033F
	str	r7, [r3, r6]

	ldr	r6, =0x100
	ldr	r7, =0x0A0E110B
	str	r7, [r3, r6]

	ldr	r6, =0x104
	ldr	r7, =0x00020211
	str	r7, [r3, r6]

	ldr	r6, =0x108
	ldr	r7, =0x03060708
	str	r7, [r3, r6]

	ldr	r6, =0x10c
	ldr	r7, =0x00A0500C
	str	r7, [r3, r6]

	ldr	r6, =0x110
	ldr	r7, =0x05020307
	str	r7, [r3, r6]

	ldr	r6, =0x114
	ldr	r7, =0x02020404
	str	r7, [r3, r6]

	ldr	r6, =0x118
	ldr	r7, =0x02020003
	str	r7, [r3, r6]

	ldr	r6, =0x11c
	ldr	r7, =0x00000202
	str	r7, [r3, r6]

	ldr	r6, =0x120
	ldr	r7, =0x00000202
	str	r7, [r3, r6]

	ldr	r6, =0x180
	ldr	r7, =0x00600018
	str	r7, [r3, r6]

	ldr	r6, =0x184
	ldr	r7, =0x00e00100
	str	r7, [r3, r6]

	ldr	r6, =0x190
	ldr	r7, =0x02098205
	str	r7, [r3, r6]

	ldr	r6, =0x194
	ldr	r7, =0x00060303
	str	r7, [r3, r6]

	ldr	r6, =0x200
	ldr	r7, =0x00000016
	str	r7, [r3, r6]

	ldr	r6, =0x204
	ldr	r7, =0x00171717
	str	r7, [r3, r6]

	ldr	r6, =0x210
	ldr	r7, =0xF00
	str	r7, [r3, r6]

	ldr	r6, =0x214
	ldr	r7, =0x05050505
	str	r7, [r3, r6]

	ldr	r6, =0x218
	ldr	r7, =0x0F0F0505
	str	r7, [r3, r6]

	ldr	r6, =0x240
	ldr	r7, =0x06000601
	str	r7, [r3, r6]

	ldr	r6, =0x244
	ldr	r7, =0x00000000
	str	r7, [r3, r6]

	ldr	r7, =0x20
	str	r7, [r3, #0x30]
	ldr	r7, =0x0
	str	r7, [r3, #0x1b0]

	/* do PHY, clear ddr_phy reset */
	ldr	r6, =0x1000
	ldr	r7, [r2, r6]
	bic	r7, r7, #0x2
	str	r7, [r2, r6]

	ldr	r7, [r1, #0x800]
	and	r7, r7, #0xFF
	cmp	r7, #0x11
	bne	2f

	/* for TO1.1 */
	ldr	r7, [r11]
	bic	r7, r7, #(1 << 27)
	str	r7, [r11]
	ldr	r7, [r11]
	bic	r7, r7, #(1 << 29)
	str	r7, [r11]
2:
	/* clear/set bit30 of SNVS_MISC_CTRL to ensure exit from ddr retention */
	ldr	r7, =(0x1 << 30)
	str	r7, [r1, #0x388]
	ldr	r7, =(0x1 << 30)
	str	r7, [r1, #0x384]

	/* need to delay ~5mS */
	ldr	r6, =0x100000
3:
	subs	r6, r6, #0x1
	bne	3b

	/* restore DDR PHY */
	ldr	r6, =0x0
	ldr	r7, =0x17421E40
	str	r7, [r4, r6]

	ldr	r6, =0x4
	ldr	r7, =0x10210100
	str	r7, [r4, r6]

	ldr	r6, =0x8
	ldr	r7, =0x00010000
	str	r7, [r4, r6]

	ldr	r6, =0x10
	ldr	r7, =0x0007080C
	str	r7, [r4, r6]

	ldr	r6, =0xb0
	ldr	r7, =0x1010007e
	str	r7, [r4, r6]

	ldr     r7, [r1, #0x800]
	and     r7, r7, #0xFF
	cmp     r7, #0x11
	bne     4f

	ldr	r6, =0x7c
	ldr	r7, =0x1c1c1c1c
	str	r7, [r4, r6]

	ldr	r6, =0x80
	ldr	r7, =0x1c1c1c1c
	str	r7, [r4, r6]

	ldr	r6, =0x84
	ldr	r7, =0x30301c1c
	str	r7, [r4, r6]

	ldr	r6, =0x88
	ldr	r7, =0x00000030
	str	r7, [r4, r6]

	ldr	r6, =0x6c
	ldr	r7, =0x30303030
	str	r7, [r4, r6]

4:
	ldr	r6, =0x1c
	ldr	r7, =0x01010000
	str	r7, [r4, r6]

	ldr	r6, =0x9c
	ldr	r7, =0x0DB60D6E
	str	r7, [r4, r6]

	ldr	r6, =0x20
	ldr	r7, =0x0a0a0a0a
	str	r7, [r4, r6]

	ldr	r6, =0x30
	ldr	r7, =0x06060606
	str	r7, [r4, r6]

	ldr	r6, =0x50
	ldr	r7, =0x01000008
	str	r7, [r4, r6]

	ldr	r6, =0x50
	ldr	r7, =0x00000008
	str	r7, [r4, r6]

	ldr	r6, =0xc0
	ldr	r7, =0x0e407304
	str	r7, [r4, r6]

	ldr	r6, =0xc0
	ldr	r7, =0x0e447304
	str	r7, [r4, r6]

	ldr	r6, =0xc0
	ldr	r7, =0x0e447306
	str	r7, [r4, r6]

	ldr	r6, =0xc0
	ldr	r7, =0x0e4c7304
	str	r7, [r4, r6]

	ldr	r6, =0xc0
	ldr	r7, =0x0e487306
	str	r7, [r4, r6]

	ldr	r7, =0x0
	add	r9, r10, #0x4000
	str	r7, [r9, #0x130]

	ldr	r7, =0x170
	orr	r7, r7, #0x8
	str	r7, [r11, #0x20]

	ldr	r7, =0x2
	add	r9, r10, #0x4000
	str	r7, [r9, #0x130]

	ldr	r7, =0xf
	str	r7, [r4, #0x18]

	/* wait until self-refresh mode entered */
11:
	ldr	r7, [r3, #0x4]
	and	r7, r7, #0x3
	cmp	r7, #0x3
	bne	11b
	ldr	r7, =0x0
	str	r7, [r3, #0x320]
	ldr	r7, =0x1
	str	r7, [r3, #0x1b0]
	ldr	r7, =0x1
	str	r7, [r3, #0x320]
12:
	ldr	r7, [r3, #0x324]
	and	r7, r7, #0x1
	cmp	r7, #0x1
	bne	12b
13:
	ldr	r7, [r3, #0x4]
	and	r7, r7, #0x20
	cmp	r7, #0x20
	bne	13b

	/* let DDR out of self-refresh */
	ldr	r7, =0x0
	str	r7, [r3, #0x30]
14:
	ldr	r7, [r3, #0x4]
	and	r7, r7, #0x30
	cmp	r7, #0x0
	bne	14b

15:
	ldr	r7, [r3, #0x4]
	and	r7, r7, #0x3
	cmp	r7, #0x1
	bne	15b

	/* enable port */
	ldr	r7, =0x1
	str	r7, [r3, #0x490]

	/* jump to kernel resume */
	ldr	r1, =0x30270000
	ldr	r7, [r1]

	mov	pc, r7
16:
	/* Configure ocram_epdc */
	ldr r0, =IOMUXC_GPR_BASE_ADDR
	ldr r1, =0x4f400005
	str r1, [r0, #0x4]

	/* clear/set bit30 of SNVS_MISC_CTRL to ensure exit from ddr retention */
	ldr r0, =ANATOP_BASE_ADDR
	ldr r1, =(0x1 << 30)
	str r1, [r0, #0x388]
	str r1, [r0, #0x384]

	ldr r0, =SRC_BASE_ADDR
	ldr r1, =0x2
	ldr r2, =0x1000
	str r1, [r0, r2]

	ldr r0, =DDRC_IPS_BASE_ADDR
	ldr r1, =0x03040008
	str r1, [r0]
	ldr r1, =0x00200038
	str r1, [r0, #0x64]
	ldr r1, =0x1
	str r1, [r0, #0x490]
	ldr r1, =0x00350001
	str r1, [r0, #0xd0]
	ldr r1, =0x00c3000a
	str r1, [r0, #0xdc]
	ldr r1, =0x00010000
	str r1, [r0, #0xe0]
	ldr r1, =0x00110006
	str r1, [r0, #0xe4]
	ldr r1, =0x33f
	str r1, [r0, #0xf4]
	ldr r1, =0x0a0e110b
	str r1, [r0, #0x100]
	ldr r1, =0x00020211
	str r1, [r0, #0x104]
	ldr r1, =0x03060708
	str r1, [r0, #0x108]
	ldr r1, =0x00a0500c
	str r1, [r0, #0x10c]
	ldr r1, =0x05020307
	str r1, [r0, #0x110]
	ldr r1, =0x02020404
	str r1, [r0, #0x114]
	ldr r1, =0x02020003
	str r1, [r0, #0x118]
	ldr r1, =0x00000202
	str r1, [r0, #0x11c]
	ldr r1, =0x00000202
	str r1, [r0, #0x120]
	ldr r1, =0x00600018
	str r1, [r0, #0x180]
	ldr r1, =0x00e00100
	str r1, [r0, #0x184]
	ldr r1, =0x02098205
	str r1, [r0, #0x190]
	ldr r1, =0x00060303
	str r1, [r0, #0x194]
	ldr r1, =0x80400003
	str r1, [r0, #0x1a0]
	ldr r1, =0x00100020
	str r1, [r0, #0x1a4]
	ldr r1, =0x80100004
	str r1, [r0, #0x1a8]

	ldr r1, =0x00000016
	str r1, [r0, #0x200]
	ldr r1, =0x00171717
	str r1, [r0, #0x204]
	ldr r1, =0x00000f00
	str r1, [r0, #0x210]
	ldr r1, =0x05050505
	str r1, [r0, #0x214]
	ldr r1, =0x0f0f0505
	str r1, [r0, #0x218]

	ldr r1, =0x06000601
	str r1, [r0, #0x240]
	mov r1, #0x0
	str r1, [r0, #0x244]

	ldr r0, =SRC_BASE_ADDR
	mov r1, #0x0
	ldr r2, =0x1000
	str r1, [r0, r2]

	ldr r0, =DDRPHY_IPS_BASE_ADDR
	ldr r1, =0x17421e40
	str r1, [r0]
	ldr r1, =0x10210100
	str r1, [r0, #0x4]
	ldr r1, =0x00010000
	str r1, [r0, #0x8]
	ldr r1, =0x0007080c
	str r1, [r0, #0x10]
	imx7d_ddrphy_latency_setting
	ldr r1, =0x1010007e
	str r1, [r0, #0xb0]
	ldr r1, =0x01010000
	str r1, [r0, #0x1c]
	ldr r1, =0x0db60d6e
	str r1, [r0, #0x9c]

	ldr r1, =0x06060606
	str r1, [r0, #0x30]
	ldr r1, =0x0a0a0a0a
	str r1, [r0, #0x20]
	ldr r1, =0x01000008
	str r1, [r0, #0x50]
	ldr r1, =0x00000008
	str r1, [r0, #0x50]

	ldr r1, =0x0000000f
	str r1, [r0, #0x18]
	ldr r1, =0x1e487304
	str r1, [r0, #0xc0]
	ldr r1, =0x1e487304
	str r1, [r0, #0xc0]
	ldr r1, =0x1e487306
	str r1, [r0, #0xc0]
	ldr r1, =0x1e4c7304
	str r1, [r0, #0xc0]

wait_zq:
	ldr r1, [r0, #0xc4]
	tst r1, #0x1
	beq wait_zq

	ldr r1, =0x1e487304
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
	imx7d_12x12_lpddr3_arm2_setting
.endm

/* include the common plugin code here */
#include <asm/arch/mx7_plugin.S>
