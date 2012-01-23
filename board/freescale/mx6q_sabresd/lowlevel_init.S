/*
 * Copyright (C) 2012 Freescale Semiconductor, Inc.
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

#include <config.h>
#include <asm/arch/mx6.h>

/*
 Disable L2Cache because ROM turn it on when uboot use plug-in.
 If L2Cache is on default, there are cache coherence problem if kernel have
 not config L2Cache.
*/
.macro init_l2cc
    ldr     r1, =0xa02000
    ldr     r0, =0x0
    str     r0, [r1, #0x100]
.endm /* init_l2cc */

/* invalidate the D-CACHE */
.macro inv_dcache
    mov     r0,#0
    mcr     p15,2,r0,c0,c0,0  /* cache size selection register, select dcache */
    mrc     p15,1,r0,c0,c0,0  /* cache size ID register */
    mov     r0,r0,ASR #13
    ldr     r3,=0xfff
    and     r0,r0,r3
    cmp     r0,#0x7f
    moveq   r6,#0x1000
    beq     size_done
    cmp     r0,#0xff
    moveq   r6,#0x2000
    movne   r6,#0x4000

size_done:
    mov     r2,#0
    mov     r3,#0x40000000
    mov     r4,#0x80000000
    mov     r5,#0xc0000000

d_inv_loop:
    mcr     p15,0,r2,c7,c6,2  /* invalidate dcache by set / way */
    mcr     p15,0,r3,c7,c6,2  /* invalidate dcache by set / way */
    mcr     p15,0,r4,c7,c6,2  /* invalidate dcache by set / way */
    mcr     p15,0,r5,c7,c6,2  /* invalidate dcache by set / way */
    add     r2,r2,#0x20
    add     r3,r3,#0x20
    add     r4,r4,#0x20
    add     r5,r5,#0x20

    cmp     r2,r6
    bne     d_inv_loop
.endm

/* AIPS setup - Only setup MPROTx registers.
 * The PACR default values are good.*/
.macro init_aips
	/*
	 * Set all MPROTx to be non-bufferable, trusted for R/W,
	 * not forced to user-mode.
	 */
	ldr r0, =AIPS1_ON_BASE_ADDR
	ldr r1, =0x77777777
	str r1, [r0, #0x0]
	str r1, [r0, #0x4]
	ldr r1, =0x0
	str r1, [r0, #0x40]
	str r1, [r0, #0x44]
	str r1, [r0, #0x48]
	str r1, [r0, #0x4C]
	str r1, [r0, #0x50]

	ldr r0, =AIPS2_ON_BASE_ADDR
	ldr r1, =0x77777777
	str r1, [r0, #0x0]
	str r1, [r0, #0x4]
	ldr r1, =0x0
	str r1, [r0, #0x40]
	str r1, [r0, #0x44]
	str r1, [r0, #0x48]
	str r1, [r0, #0x4C]
	str r1, [r0, #0x50]
.endm /* init_aips */

.macro setup_pll pll, freq
.endm

.macro init_clock

/* PLL1, PLL2, and PLL3 are enabled by ROM */
#ifdef CONFIG_PLL3
	/* enable PLL3 for UART */
	ldr r0, ANATOP_BASE_ADDR_W

	/* power up PLL */
	ldr r1, [r0, #ANATOP_USB1]
	orr r1, r1, #0x1000
	str r1, [r0, #ANATOP_USB1]

	/* enable PLL */
	ldr r1, [r0, #ANATOP_USB1]
	orr r1, r1, #0x2000
	str r1, [r0, #ANATOP_USB1]

	/* wait PLL lock */
100:
	ldr r1, [r0, #ANATOP_USB1]
	mov r1, r1, lsr #31
	cmp r1, #0x1
	bne 100b

	/* clear bypass bit */
	ldr r1, [r0, #ANATOP_USB1]
	and r1, r1, #0xfffeffff
	str r1, [r0, #ANATOP_USB1]
#endif

	/* Restore the default values in the Gate registers */
	ldr r0, CCM_BASE_ADDR_W
	ldr r1, =0xC0003F
	str r1, [r0, #CLKCTL_CCGR0]
	ldr r1, =0x30FC00
	str r1, [r0, #CLKCTL_CCGR1]
	ldr r1, =0xFFFC000
	str r1, [r0, #CLKCTL_CCGR2]
	ldr r1, =0x3FF00000
	str r1, [r0, #CLKCTL_CCGR3]
	ldr r1, =0xFFF300
	str r1, [r0, #CLKCTL_CCGR4]
	ldr r1, =0xF0000C3
	str r1, [r0, #CLKCTL_CCGR5]
	ldr r1, =0x3FC
	str r1, [r0, #CLKCTL_CCGR6]
.endm

.section ".text.init", "x"

.globl lowlevel_init
lowlevel_init:

	inv_dcache

	init_l2cc

	init_aips

	init_clock

	mov pc, lr

/* Board level setting value */
ANATOP_BASE_ADDR_W:		.word ANATOP_BASE_ADDR
CCM_BASE_ADDR_W:		.word CCM_BASE_ADDR
