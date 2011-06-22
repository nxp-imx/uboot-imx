/*
 * Copyright (C) 2010-2011 Freescale Semiconductor, Inc.
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

/* AIPS setup - Only setup MPROTx registers.
 * The PACR default values are good.*/
.macro init_aips
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
	ldr r1, =0xFFFFFFFF
	ldr r0, CCM_BASE_ADDR_W
	str r1, [r0, #CLKCTL_CCGR0]
	str r1, [r0, #CLKCTL_CCGR1]
	str r1, [r0, #CLKCTL_CCGR2]
	str r1, [r0, #CLKCTL_CCGR3]
	str r1, [r0, #CLKCTL_CCGR4]
	str r1, [r0, #CLKCTL_CCGR5]
	str r1, [r0, #CLKCTL_CCGR6]
	str r1, [r0, #CLKCTL_CCGR7]

.endm

.section ".text.init", "x"

.globl lowlevel_init
lowlevel_init:

	init_l2cc

	init_aips

	init_clock

	mov pc, lr

/* Board level setting value */
ANATOP_BASE_ADDR_W:		.word ANATOP_BASE_ADDR
CCM_BASE_ADDR_W:		.word CCM_BASE_ADDR
