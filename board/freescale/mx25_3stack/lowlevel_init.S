/*
 * Copyright (c) 2009  Freescale Semiconductor
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <asm/arch/mx25-regs.h>

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

.globl lowlevel_init
lowlevel_init:

	REG 0x53F80008, 0x20034000 // ARM clk = 399, AHB clk = 133

	/* Init Debug Board CS5 */
	REG 0xB8002050, 0x0000D843
	REG 0xB8002054, 0x22252521
	REG 0xB8002058, 0x22220A00

	/* MAX (Multi-Layer AHB Crossbar Switch) setup */
	/* MAX - priority for MX25 is (SDHC2/SDMA)>USBOTG>RTIC>IAHB>DAHB */
	ldr r0, =MAX_BASE
	ldr r1, =0x00002143
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

	/* M3IF setup */
	ldr r1, =M3IF_BASE
	ldr r0, =0x00000001
	str r0, [r1]  /* M3IF control reg */

	/* default CLKO to 1/32 of the ARM core */
	ldr r0, =CCM_MCR
	ldr r1, =CCM_MCR
	bic r1, r1, #0x00F00000
	bic r1, r1, #0x7F000000
	mov r2,     #0x5F000000
	add r2, r2, #0x00200000
	orr r1, r1, r2
	str r1, [r0]

	/* enable all the clocks */
	ldr r2, =0x1FFFFFFF
	ldr r0, =CCM_CGR0
	str r2, [r0]
	ldr r2, =0xFFFFFFFF
	ldr r0, =CCM_CGR1
	str r2, [r0]
	ldr r2, =0x000FDFFF
	ldr r0, =CCM_CGR2
	str r2, [r0]
	mov	pc, lr

