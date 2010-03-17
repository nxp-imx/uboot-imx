/*
 * Copyright (C) 2007, Guennadi Liakhovetski <lg@denx.de>
 *
 * Copyright (C) 2010 Freescale Semiconductor, Inc.
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
#include <asm/arch/mx53.h>

/*
 * L2CC Cache setup/invalidation/disable
 */
.macro init_l2cc
	/* explicitly disable L2 cache */
        mrc 15, 0, r0, c1, c0, 1
        bic r0, r0, #0x2
        mcr 15, 0, r0, c1, c0, 1

        /* reconfigure L2 cache aux control reg */
        mov r0, #0xC0                   /* tag RAM */
        add r0, r0, #0x4                /* data RAM */
        orr r0, r0, #(1 << 24)          /* disable write allocate delay */
        orr r0, r0, #(1 << 23)          /* disable write allocate combine */
        orr r0, r0, #(1 << 22)          /* disable write allocate */

	mcr 15, 1, r0, c9, c0, 2
.endm /* init_l2cc */

/* AIPS setup - Only setup MPROTx registers.
 * The PACR default values are good.*/
.macro init_aips
	/*
	 * Set all MPROTx to be non-bufferable, trusted for R/W,
	 * not forced to user-mode.
	 */
	ldr r0, =AIPS1_BASE_ADDR
	ldr r1, =0x77777777
	str r1, [r0, #0x0]
	str r1, [r0, #0x4]
	ldr r0, =AIPS2_BASE_ADDR
	str r1, [r0, #0x0]
	str r1, [r0, #0x4]
.endm /* init_aips */

.macro setup_pll pll, freq
	ldr r2, =\pll
	ldr r1, =0x00001232
	str r1, [r2, #PLL_DP_CTL] /* Set DPLL ON (set UPEN bit): BRMO=1 */
	mov r1, #0x2
	str r1, [r2, #PLL_DP_CONFIG] /* Enable auto-restart AREN bit */

	str r3, [r2, #PLL_DP_OP]
	str r3, [r2, #PLL_DP_HFS_OP]

	str r4, [r2, #PLL_DP_MFD]
	str r4, [r2, #PLL_DP_HFS_MFD]

	str r5, [r2, #PLL_DP_MFN]
	str r5, [r2, #PLL_DP_HFS_MFN]

	ldr r1, =0x00001232
	str r1, [r2, #PLL_DP_CTL]
1:	ldr r1, [r2, #PLL_DP_CTL]
	ands r1, r1, #0x1
	beq 1b
.endm

.macro init_clock
	ldr r0, CCM_BASE_ADDR_W

	/* Switch ARM to step clock */
	mov r1, #0x4
	str r1, [r0, #CLKCTL_CCSR]

	mov r3, #DP_OP_800
	mov r4, #DP_MFD_800
	mov r5, #DP_MFN_800
	setup_pll PLL1_BASE_ADDR, 800

	mov r3, #DP_OP_400
        mov r4, #DP_MFD_400
        mov r5, #DP_MFN_400
        setup_pll PLL3_BASE_ADDR, 400

        /* Switch peripheral to PLL3 */
        ldr r0, CCM_BASE_ADDR_W
        ldr r1, CCM_VAL_0x00015154
        str r1, [r0, #CLKCTL_CBCMR]
        ldr r1, CCM_VAL_0x02888945
        orr r1, r1, #(1 << 16)   /* Set DDR divider to run at 200MHz */
        str r1, [r0, #CLKCTL_CBCDR]
        /* make sure mux & divider change is effective */
1:      ldr r1, [r0, #CLKCTL_CDHIPR]
        cmp r1, #0x0
        bne 1b

	mov r3, #DP_OP_600
	mov r4, #DP_MFD_600
	mov r5, #DP_MFN_600
        setup_pll PLL2_BASE_ADDR, 600

	/* Switch peripheral to PLL2 */
	ldr r0, CCM_BASE_ADDR_W
	ldr r1, CCM_VAL_0x00809145 /* AHB is 120MHz, from PLL2 */
	orr r1, r1, #(1 << 16)   /* Set DDR divider to run at 300MHz */
	orr r1, r1, #(2 << 19)   /* Set AXI_B divider to run at 200MHz */
	str r1, [r0, #CLKCTL_CBCDR]

	ldr r1, CCM_VAL_0x00016154
	str r1, [r0, #CLKCTL_CBCMR]

	/* make sure mux change is effective */
1:      ldr r1, [r0, #CLKCTL_CDHIPR]
	cmp r1, #0x0
	bne 1b

	mov r3, #DP_OP_216
        mov r4, #DP_MFD_216
        mov r5, #DP_MFN_216
        setup_pll PLL3_BASE_ADDR, 216

	/* Set the platform clock dividers */
	ldr r0, PLATFORM_BASE_ADDR_W
	ldr r1, PLATFORM_CLOCK_DIV_W
	str r1, [r0, #PLATFORM_ICGC]

	/* CPU 400M hz */
	ldr r0, CCM_BASE_ADDR_W
	mov r1, #1
	str r1, [r0, #CLKCTL_CACRR]

	/* Switch ARM back to PLL 1. */
	mov r1, #0x0
	str r1, [r0, #CLKCTL_CCSR]

	ldr r1, [r0, #CLKCTL_CSCDR1]
	orr r1, r1, #0x3f
	eor r1, r1, #0x3f
	orr r1, r1, #0x21
	str r1, [r0, #CLKCTL_CSCDR1]

	/* Restore the default values in the Gate registers */
	ldr r1, =0xFFFFFFFF
	str r1, [r0, #CLKCTL_CCGR0]
	str r1, [r0, #CLKCTL_CCGR1]
	str r1, [r0, #CLKCTL_CCGR2]
	str r1, [r0, #CLKCTL_CCGR3]
	str r1, [r0, #CLKCTL_CCGR4]
	str r1, [r0, #CLKCTL_CCGR5]
	str r1, [r0, #CLKCTL_CCGR6]
	str r1, [r0, #CLKCTL_CCGR7]

        mov r1, #0x00000
        str r1, [r0, #CLKCTL_CCDR]

        /* for cko - for ARM div by 8 */
        mov r1, #0x000A0000
        add r1, r1, #0x00000F0
        str r1, [r0, #CLKCTL_CCOSR]
.endm

.section ".text.init", "x"

.globl lowlevel_init
lowlevel_init:

#ifdef ENABLE_IMPRECISE_ABORT
        mrs r1, spsr            /* save old spsr */
        mrs r0, cpsr            /* read out the cpsr */
	bic r0, r0, #0x100      /* clear the A bit */
	msr spsr, r0            /* update spsr */
	add lr, pc, #0x8        /* update lr */
        movs pc, lr             /* update cpsr */
        nop
        nop
        nop
	nop
	msr spsr, r1            /* restore old spsr */
#endif

	/* ARM errata ID #468414 */
	mrc 15, 0, r1, c1, c0, 1
	orr r1, r1, #(1 << 5)    /* enable L1NEON bit */
	mcr 15, 0, r1, c1, c0, 1

	init_l2cc

	init_aips

	init_clock

	mov pc, lr

/* Board level setting value */
CCM_BASE_ADDR_W:        .word CCM_BASE_ADDR
CCM_VAL_0x00016154:     .word 0x00016154
CCM_VAL_0x00809145:     .word 0x00809145
CCM_VAL_0x00015154:     .word 0x00015154
CCM_VAL_0x02888945:     .word 0x02888945
PLATFORM_BASE_ADDR_W:   .word ARM_BASE_ADDR
PLATFORM_CLOCK_DIV_W:   .word 0x00000124
