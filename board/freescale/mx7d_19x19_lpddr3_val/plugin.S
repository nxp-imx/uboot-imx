/*
 * Copyright (C) 2015-2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <config.h>

/* DDR script */
.macro imx7d_ddrphy_lpddr3_latency_setting
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

.macro imx7d_ddrphy_lpddr2_latency_setting
	ldr r2, =ANATOP_BASE_ADDR
	ldr r3, [r2, #0x800]
	and r3, r3, #0xFF
	cmp r3, #0x11
	bne NO_DELAY

	/*TO 1.1*/
	ldr r1, =0x00000dee
	str r1, [r0, #0x9c]
	ldr r1, =0x08080808
	str r1, [r0, #0x7c]
	ldr r1, =0x08080808
	str r1, [r0, #0x80]
	ldr r1, =0x0a0a0808
	str r1, [r0, #0x84]
	ldr r1, =0x0000000a
	str r1, [r0, #0x88]
	ldr r1, =0x0a0a0a0a
	str r1, [r0, #0x6c]
	b TUNE_END

NO_DELAY:
	/*TO 1.0*/
	ldr r1, =0x00000d6e
	str r1, [r0, #0x9c]

TUNE_END:
.endm

.macro imx7d_19x19_lpddr3_val_setting
	/* Configure ocram_epdc */
	ldr r0, =IOMUXC_GPR_BASE_ADDR
	ldr r1, =0x4f400005
	str r1, [r0, #0x4]

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
	ldr r1, =0x00090909
	str r1, [r0, #0x204]
	ldr r1, =0x00000f00
	str r1, [r0, #0x210]
	ldr r1, =0x08080808
	str r1, [r0, #0x214]
	ldr r1, =0x0f0f0808
	str r1, [r0, #0x218]

	ldr r1, =0x06000600
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
	imx7d_ddrphy_lpddr3_latency_setting
	ldr r1, =0x1010007e
	str r1, [r0, #0xb0]
	ldr r1, =0x01010000
	str r1, [r0, #0x1c]

	ldr r2, =ANATOP_BASE_ADDR
	ldr r3, [r2, #0x800]
	and r3, r3, #0xFF
	cmp r3, #0x11
	bne	1f

	ldr r1, =0x0db60d6e
	str r1, [r0, #0x9c]
	b	2f
1:
	ldr r1, =0x00000b24
	str r1, [r0, #0x9c]
2:
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
	ldr r1, =0x0e487304
	str r1, [r0, #0xc0]
	ldr r1, =0x0e4c7304
	str r1, [r0, #0xc0]
	ldr r1, =0x0e4c7306
	str r1, [r0, #0xc0]

wait_zq:
	ldr r1, [r0, #0xc4]
	tst r1, #0x1
	beq wait_zq

	ldr r1, =0x0e487304
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

.macro imx7d_19x19_lpddr2_val_setting
	/* Configure ocram_epdc */
	ldr r0, =IOMUXC_GPR_BASE_ADDR
	ldr r1, =0x4f400005
	str r1, [r0, #0x4]

	ldr r0, =SRC_BASE_ADDR
	ldr r1, =0x2
	ldr r2, =0x1000
	str r1, [r0, r2]

	ldr r0, =DDRC_IPS_BASE_ADDR
	ldr r1, =0x03020004
	str r1, [r0]
	ldr r1, =0x80400003
	str r1, [r0, #0x1a0]
	ldr r1, =0x00100020
	str r1, [r0, #0x1a4]
	ldr r1, =0x80100004
	str r1, [r0, #0x1a8]
	ldr r1, =0x00200023
	str r1, [r0, #0x64]
	ldr r1, =0x1
	str r1, [r0, #0x490]
	ldr r1, =0x00350001
	str r1, [r0, #0xd0]
	ldr r1, =0x00001105
	str r1, [r0, #0xd8]
	ldr r1, =0x00c20006
	str r1, [r0, #0xdc]
	ldr r1, =0x00020000
	str r1, [r0, #0xe0]
	ldr r1, =0x00110006
	str r1, [r0, #0xe4]
	ldr r1, =0x33f
	str r1, [r0, #0xf4]
	ldr r1, =0x080e110b
	str r1, [r0, #0x100]
	ldr r1, =0x00020211
	str r1, [r0, #0x104]
	ldr r1, =0x02040706
	str r1, [r0, #0x108]
	ldr r1, =0x00504000
	str r1, [r0, #0x10c]
	ldr r1, =0x05010307
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
	ldr r1, =0x02098203
	str r1, [r0, #0x190]
	ldr r1, =0x00060303
	str r1, [r0, #0x194]

	ldr r1, =0x00000015
	str r1, [r0, #0x200]
	ldr r1, =0x00161616
	str r1, [r0, #0x204]
	ldr r1, =0x00000f0f
	str r1, [r0, #0x210]
	ldr r1, =0x04040404
	str r1, [r0, #0x214]
	ldr r1, =0x0f0f0404
	str r1, [r0, #0x218]

	ldr r1, =0x06000600
	str r1, [r0, #0x240]
	mov r1, #0x0
	str r1, [r0, #0x244]

	ldr r0, =SRC_BASE_ADDR
	mov r1, #0x0
	ldr r2, =0x1000
	str r1, [r0, r2]

	ldr r0, =DDRPHY_IPS_BASE_ADDR
	ldr r1, =0x17421640
	str r1, [r0]
	ldr r1, =0x10210100
	str r1, [r0, #0x4]
	ldr r1, =0x00010000
	str r1, [r0, #0x8]
	ldr r1, =0x00050408
	str r1, [r0, #0x10]
	ldr r1, =0x1010007e
	str r1, [r0, #0xb0]
	ldr r1, =0x01010000
	str r1, [r0, #0x1c]
	imx7d_ddrphy_lpddr2_latency_setting
	ldr r1, =0x0000000f
	str r1, [r0, #0x18]

	ldr r1, =0x06060606
	str r1, [r0, #0x30]
	ldr r1, =0x0a0a0a0a
	str r1, [r0, #0x20]
	ldr r1, =0x01000008
	str r1, [r0, #0x50]
	ldr r1, =0x00000008
	str r1, [r0, #0x50]

	ldr r1, =0x0e487304
	str r1, [r0, #0xc0]
	ldr r1, =0x0e4c7304
	str r1, [r0, #0xc0]
	ldr r1, =0x0e4c7306
	str r1, [r0, #0xc0]

wait_zq:
	ldr r1, [r0, #0xc4]
	tst r1, #0x1
	beq wait_zq

	ldr r1, =0x0e4c7304
	str r1, [r0, #0xc0]
	ldr r1, =0x0e487304
	str r1, [r0, #0xc0]

	ldr r0, =CCM_BASE_ADDR
	mov r1, #0x0
	ldr r2, =0x4130
	str r1, [r0, r2]
	ldr r0, =IOMUXC_GPR_BASE_ADDR
	mov r1, #0x1f8
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
#if defined (TARGET_MX7D_19X19_LPDDR2_VAL)
	imx7d_19x19_lpddr2_val_setting
#else
	imx7d_19x19_lpddr3_val_setting
#endif
.endm

/* include the common plugin code here */
#include <asm/arch/mx7_plugin.S>
