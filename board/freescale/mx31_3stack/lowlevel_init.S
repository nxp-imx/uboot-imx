/*
 * Copyright (C) 2008, Guennadi Liakhovetski <lg@denx.de>
 * Copyright (C) 2008, Freescale Semiconductor
 * 	Modifications for MX31 3Stack board
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

#include <asm/arch/mx31-regs.h>

.macro REG reg, val
	ldr r2, =\reg
	ldr r3, =\val
	str r3, [r2]
.endm

.macro REG8 reg, val
	ldr r2, =\reg
	ldr r3, =\val
	strb r3, [r2]
.endm

.macro DELAY loops
	ldr r2, =\loops
1:
	subs	r2, r2, #1
	nop
	bcs 1b
.endm

.macro init_aips
	/*
	 * Set all MPROTx to be non-bufferable, trusted for R/W,
	 * not forced to user-mode.
	 */
	ldr r0, =0x43F00000
	ldr r1, =0x77777777
	str r1, [r0, #0x00]
	str r1, [r0, #0x04]
	ldr r0, =0x53F00000
	str r1, [r0, #0x00]
	str r1, [r0, #0x04]

	/*
	 * Clear the on and off peripheral modules Supervisor Protect bit
	 * for SDMA to access them. Did not change the AIPS control registers
	 * (offset 0x20) access type
	 */
	ldr r0, =0x43F00000
	ldr r1, =0x0
	str r1, [r0, #0x40]
	str r1, [r0, #0x44]
	str r1, [r0, #0x48]
	str r1, [r0, #0x4C]
	ldr r1, [r0, #0x50]
	and r1, r1, #0x00FFFFFF
	str r1, [r0, #0x50]

	ldr r0, =0x53F00000
	ldr r1, =0x0
	str r1, [r0, #0x40]
	str r1, [r0, #0x44]
	str r1, [r0, #0x48]
	str r1, [r0, #0x4C]
	ldr r1, [r0, #0x50]
	and r1, r1, #0x00FFFFFF
	str r1, [r0, #0x50]
.endm /* init_aips */

.macro init_max
	ldr r0, =0x43F04000
	/* MPR - priority is M4 > M2 > M3 > M5 > M0 > M1 */
	ldr r1, =0x00302154
	str r1, [r0, #0x000]        /* for S0 */
	str r1, [r0, #0x100]        /* for S1 */
	str r1, [r0, #0x200]        /* for S2 */
	str r1, [r0, #0x300]        /* for S3 */
	str r1, [r0, #0x400]        /* for S4 */
	/* SGPCR - always park on last master */
	ldr r1, =0x10
	str r1, [r0, #0x010]        /* for S0 */
	str r1, [r0, #0x110]        /* for S1 */
	str r1, [r0, #0x210]        /* for S2 */
	str r1, [r0, #0x310]        /* for S3 */
	str r1, [r0, #0x410]        /* for S4 */
	/* MGPCR - restore default values */
	ldr r1, =0x0
	str r1, [r0, #0x800]        /* for M0 */
	str r1, [r0, #0x900]        /* for M1 */
	str r1, [r0, #0xA00]        /* for M2 */
	str r1, [r0, #0xB00]        /* for M3 */
	str r1, [r0, #0xC00]        /* for M4 */
	str r1, [r0, #0xD00]        /* for M5 */
.endm /* init_max */

.macro init_m3if
	/* Configure M3IF registers */
	ldr r1, =0xB8003000
	/*
	* M3IF Control Register (M3IFCTL)
	* MRRP[0] = L2CC0 not on priority list (0 << 0)	= 0x00000000
	* MRRP[1] = L2CC1 not on priority list (0 << 0)	= 0x00000000
	* MRRP[2] = MBX not on priority list (0 << 0)	= 0x00000000
	* MRRP[3] = MAX1 not on priority list (0 << 0)	= 0x00000000
	* MRRP[4] = SDMA not on priority list (0 << 0)	= 0x00000000
	* MRRP[5] = MPEG4 not on priority list (0 << 0)	= 0x00000000
	* MRRP[6] = IPU1 on priority list (1 << 6)	= 0x00000040
	* MRRP[7] = IPU2 not on priority list (0 << 0)	= 0x00000000
	*						------------
	*						  0x00000040
	*/
	ldr r0, =0x00000040
	str r0, [r1]  /* M3IF control reg */
.endm /* init_m3if */

.macro  init_drive_strength
	/*
	 * Disable maximum drive strength SDRAM/DDR lines by clearing DSE1 bits
	 * in SW_PAD_CTL registers
	 */

	/* SDCLK */
	ldr r1, =0x43FAC200
	ldr r0, [r1, #0x6C]
	bic r0, r0, #(1 << 12)
	str r0, [r1, #0x6C]

	/* CAS */
	ldr r0, [r1, #0x70]
	bic r0, r0, #(1 << 22)
	str r0, [r1, #0x70]

	/* RAS */
	ldr r0, [r1, #0x74]
	bic r0, r0, #(1 << 2)
	str r0, [r1, #0x74]

	/* CS2 (CSD0) */
	ldr r0, [r1, #0x7C]
	bic r0, r0, #(1 << 22)
	str r0, [r1, #0x7C]

	/* DQM3 */
	ldr r0, [r1, #0x84]
	bic r0, r0, #(1 << 22)
	str r0, [r1, #0x84]

	/* DQM2, DQM1, DQM0, SD31-SD0, A25-A0, MA10 (0x288..0x2DC) */
	ldr r2, =22	/* (0x2E0 - 0x288) / 4 = 22 */
pad_loop:
	ldr r0, [r1, #0x88]
	bic r0, r0, #(1 << 22)
	bic r0, r0, #(1 << 12)
	bic r0, r0, #(1 << 2)
	str r0, [r1, #0x88]
	add r1, r1, #4
	subs r2, r2, #0x1
	bne pad_loop
.endm /* init_drive_strength */

.section ".text.init", "x"

.globl lowlevel_init
lowlevel_init:

	ldr r0, =0x40000015        /* start from AIPS 2GB region */
	mcr p15, 0, r0, c15, c2, 4

	init_aips

	init_max

	init_m3if

	init_drive_strength

	/* Image Processing Unit: */
	/* Too early to switch display on? */
	REG	IPU_CONF, IPU_CONF_DI_EN
	/* Clock Control Module: */
	REG	CCM_CCMR, 0x074B0BF5	/* Use CKIH, MCU PLL off */

	DELAY 0x40000

	REG	CCM_CCMR, 0x074B0BF5 | CCMR_MPE		/* MCU PLL on */
	/* Switch to MCU PLL */
	REG	CCM_CCMR, (0x074B0BF5 | CCMR_MPE) & ~CCMR_MDS

	/* 532-133-66.5 */
	ldr	r0, =CCM_BASE
	ldr	r1, =0xFF871D58
	/* PDR0 */
	str	r1, [r0, #0x4]
	ldr	r1, MPCTL_PARAM_532
	/* MPCTL */
	str	r1, [r0, #0x10]

	/* Set UPLL=240MHz, USB=60MHz */
	ldr	r1, =0x49FCFE7F
	/* PDR1 */
	str	r1, [r0, #0x8]
	ldr	r1, UPCTL_PARAM_240
	/* UPCTL */
	str	r1, [r0, #0x14]
	/* default CLKO to 1/8 of the ARM core */
	mov	r1, #0x000002C0
	add	r1, r1, #0x00000006
	/* COSR */
	str	r1, [r0, #0x1c]

	/* initial CSD0 MDDR */
	REG	0xB8001004, 0x0075E73A
	REG	0xB8001010, 0x00000002 /* reset */
	REG	0xB8001010, 0x00000004
	DELAY	0x10000

	REG	0xB8001000, 0x92100000
	REG	0x80000F00, 0x0
	REG	0xB8001000, 0xA2100000
	REG	0x80000000, 0x0
	REG	0xB8001000, 0xB2100000
	REG8	0x80000033, 0x0
	REG8	0x81000000, 0xff
	REG	0xB8001000, 0x82226080
	REG	0x80000000, 0x0
	REG	0xB8001010, 0x0000000c

	mov	r13, ip
	/* copy blocks of total uboot to DDR */
	b	mxc_nand_load

MPCTL_PARAM_532:
	.word (((1-1) << 26) + ((52-1) << 16) + (10 << 10) + (12 << 0))
UPCTL_PARAM_240:
	.word (((2-1) << 26) + ((13-1) << 16) + (9  << 10) + (3  << 0))
