/*
 * Copyright (C) 2007, Guennadi Liakhovetski <lg@denx.de>
 *
 * (C) Copyright 2009 Freescale Semiconductor, Inc.
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
#include <asm/arch/mx51.h>
#include "board-imx51.h"

/*
 * return soc version
 * 	0x10:  TO1
 *	0x20:  TO2
 *	0x30:  TO3
 */
.macro check_soc_version ret, tmp
.endm

/*
 * L2CC Cache setup/invalidation/disable
 */
.macro init_l2cc
	/* reconfigure L2 cache aux control reg */
	ldr r0, =0x03C000C4
	mcr p15, 1, r0, c9, c0, 2
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
	/*
	 * Clear the on and off peripheral modules Supervisor Protect bit
	 * for SDMA to access them. Did not change the AIPS control registers
	 * (offset 0x20) access type
	 */
.endm /* init_aips */

/* MAX (Multi-Layer AHB Crossbar Switch) setup */
.macro init_max
.endm /* init_max */

/* M4IF setup */
.macro init_m4if
	/* VPU and IPU given higher priority (0x4)
	 * IPU accesses with ID=0x1 given highest priority (=0xA)
	 */
	ldr r0, =M4IF_BASE_ADDR

	ldr r1, =0x00000203
	str r1, [r0, #0x40]

	ldr r1, =0x0
	str r1, [r0, #0x44]

	ldr r1, =0x00120125
	str r1, [r0, #0x9C]

	ldr r1, =0x001901A3
	str r1, [r0, #0x48]

/*
	ldr r1, =0x00000a01
	str r1, [r0, #0x48]
	ldr r1, =0x00000404
	str r1, [r0, #0x40]
*/
.endm /* init_m4if */

/* To support 133MHz DDR */
.macro  init_drive_strength
.endm /* init_drive_strength */

/* CPLD on CS5 setup */
.macro init_debug_board
.endm /* init_debug_board */

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
	ldr r0, =CCM_BASE_ADDR
	mov r1, #0x00060000
	str r1, [r0, #CLKCTL_CCDR]

	/* Switch ARM to step clock */
	mov r1, #0x4
	str r1, [r0, #CLKCTL_CCSR]

	mov r3, #DP_OP_800
	mov r4, #DP_MFD_800
	mov r5, #DP_MFN_800
	setup_pll PLL1_BASE_ADDR
	mov r3, #DP_OP_665
	mov r4, #DP_MFD_665
	mov r5, #DP_MFN_665
	setup_pll PLL3_BASE_ADDR

	/* Switch peripheral to PLL 3 */
	ldr r1, =0x0000D3C0
	str r1, [r0, #CLKCTL_CBCMR]
	ldr r1, =0x033B9145
	str r1, [r0, #CLKCTL_CBCDR]
	mov r3, #DP_OP_665
	mov r4, #DP_MFD_665
	mov r5, #DP_MFN_665
	setup_pll PLL2_BASE_ADDR

	/* Switch peripheral to PLL2 */
	ldr r1, =0x013B9145
	str r1, [r0, #CLKCTL_CBCDR]
	ldr r1, =0x0000E3C0
	str r1, [r0, #CLKCTL_CBCMR]

	mov r3, #DP_OP_216
	mov r4, #DP_MFD_216
	mov r5, #DP_MFN_216
	setup_pll PLL3_BASE_ADDR

	/* Set the platform clock dividers */
	ldr r2, =ARM_BASE_ADDR
	ldr r1, =0x00000725
	str r1, [r2, #0x14]

	/* Switch ARM back to PLL 1 */
	mov r1, #0
	str r1, [r0,  #CLKCTL_CCSR]
	str r1, [r0,  #CLKCTL_CACRR]

	/* Use lp_apm (24MHz) source for perclk */
	mov r2, #0x48
	ldr r2, [r0]
	cmp r2, #0x10
	ldrhs r1, =0x000020C2
	ldrlo r1, =0x0000E3C2
	str r1, [r0, #CLKCTL_CBCMR]
	/* TO1.x emi = ahb, all perclk dividers are 1 since using 24MHz */
	/* TO2.x ddr from PLL1, all perclk dividers are 1 since using 24MHz */
	ldrhs r1, =0x59239100
	ldrlo r1, =0x013D9100
	strlo r1, [r0, #CLKCTL_CBCDR]

	/* use PLL2 for UART source, get 66.5MHz */
	ldr r1, =0xA5A2A020
	str r1, [r0, #CLKCTL_CSCMR1]
	ldr r1, =0x00C30321
	str r1, [r0, #CLKCTL_CSCDR1]

	/* make sure divider effective */
1:	ldr r1, [r0, #CLKCTL_CDHIPR]
	cmp r1, #0
	bne 1b

	mov r1, #0x0
	str r1, [r0, #CLKCTL_CCDR]
.endm

.macro setup_wdog
	ldr r0, =WDOG1_BASE_ADDR
	mov r1, #0x30
	strh r1, [r0]
.endm

.section ".text.init", "x"

.globl lowlevel_init
lowlevel_init:
	/* Platform CHIP level init*/
	ldr r0, =GPIO1_BASE_ADDR
    ldr r1, [r0, #0x0]
    orr r1, r1, #(1 << 23)
    str r1, [r0, #0x0]
    ldr r1, [r0, #0x4]
    orr r1, r1, #(1 << 23)
    str r1, [r0, #0x4]

#ifdef TURN_OFF_IMPRECISE_ABORT
	mrs r0, cpsr
	bic r0, r0, #0x100
	msr cpsr, r0
#endif

	mrc 15, 0, r1, c1, c0, 0

#ifndef BRANCH_PREDICTION_ENABLE
    	mrc 15, 0, r0, c1, c0, 1
    	bic r0, r0, #7
    	mcr 15, 0, r0, c1, c0, 1
#else
    	mrc 15, 0, r0, c1, c0, 1
    	orr r0, r0, #7
    	mcr 15, 0, r0, c1, c0, 1
    	orr r1, r1, #(1<<11)
#endif

#ifdef UNALIGNED_ACCESS_ENABLE
    	orr r1, r1, #(1<<22)
#endif

#ifdef LOW_INT_LATENCY_ENABLE
    	orr r1, r1, #(1<<21)
#endif
    	mcr 15, 0, r1, c1, c0, 0

    	mov r0, #0
#ifdef BRANCH_PREDICTION_ENABLE
   	mcr 15, 0, r0, c15, c2, 4
#endif
	mcr 15, 0, r0, c7, c10, 4       /* Drain the write buffer */

	init_l2cc

	init_aips

	setup_wdog

	init_max

	init_m4if

	init_drive_strength

	cmp pc, #PHYS_SDRAM_1
	blo init_clock_start
	cmp pc, #(PHYS_SDRAM_1 + PHYS_SDRAM_1_SIZE)
	blo init_clock_start

init_clock_start:
	init_clock
	init_debug_board
	/*init_sdram*/

	/* return from mxc_nand_load */
	/* r12 saved upper lr*/
	b mxc_nand_load

/* Board level setting value */
DDR_PERCHARGE_CMD:	.word 0x04008008
DDR_REFRESH_CMD:	.word 0x00008010
DDR_LMR1_W:		.word 0x00338018
DDR_LMR_CMD:		.word 0xB2220000
DDR_TIMING_W:		.word 0xB02567A9
DDR_MISC_W:		.word 0x000A0104
