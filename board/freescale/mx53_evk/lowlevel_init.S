/*
 * Copyright (C) 2007, Guennadi Liakhovetski <lg@denx.de>
 *
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
	/*
	 * If freq < 300MHz, we need to set dpdck0_2_en to 0
	 */
	ldr r0, =\freq
	ldr r1, =0x300
	cmp r0, r1
	ldrcs r1, =0x00001232
	ldrcc r1, =0x00000232
	ldr r0, =\pll
	str r1, [r0, #PLL_DP_CTL]
	mov r1, #0x2
	str r1, [r0, #PLL_DP_CONFIG]

	ldr r1, W_DP_OP_\freq
	str r1, [r0, #PLL_DP_OP]
	str r1, [r0, #PLL_DP_HFS_OP]

	ldr r1,	W_DP_MFD_\freq
	str r1, [r0, #PLL_DP_MFD]
	str r1, [r0, #PLL_DP_HFS_MFD]

	ldr r1,  W_DP_MFN_\freq
	str r1, [r0, #PLL_DP_MFN]
	str r1, [r0, #PLL_DP_HFS_MFN]

	ldr r1, =0x00001232
	str r1, [r0, #PLL_DP_CTL]
1:	ldr r1, [r0, #PLL_DP_CTL]
	ands r1, r1, #0x1
	beq 1b
.endm

.macro init_m4if
	/*increase master2 priority for WIFI*/
	ldr r0, =M4IF_BASE_ADDR
	ldr r1, [r0, #0x40]
	orr r1, r1, #(0x33 << 16)
	str r1, [r0, #0x40]

	/*increase master4 priority for FEC*/
	ldr r1, [r0, #0x44]
	orr r1, r1, #0x33
	str r1, [r0, #0x44]

	/*set SDHC-port3 high priority to all AHB MAX Slave port*/
	ldr r0, =AHBMAX_BASE_ADDR

#ifdef CONFIG_WIFI_SDHC_PORT3
	/*set PARK to SDHC-port3*/
	ldr r0, =AHBMAX_BASE_ADDR
	ldr r1, [r0, #0x10]
	bic r1, r1, #(0x7 << 0)
	orr r1, r1, #0x3
	bic r1, r1, #(0x3 << 4)
	bic r1, r1, #(0x3 << 8)
	orr r1, r1, #(0x1 << 30)
	bic r1, r1, #(0x1 << 31)

	ldr r1, [r0, #0x110]
	bic r1, r1, #(0x7 << 0)
	orr r1, r1, #0x3
	bic r1, r1, #(0x3 << 4)
	bic r1, r1, #(0x3 << 8)
	orr r1, r1, #(0x1 << 30)
	bic r1, r1, #(0x1 << 31)

	ldr r1, [r0, #0x210]
	bic r1, r1, #(0x7 << 0)
	orr r1, r1, #0x3
	bic r1, r1, #(0x3 << 4)
	bic r1, r1, #(0x3 << 8)
	orr r1, r1, #(0x1 << 30)
	bic r1, r1, #(0x1 << 31)

	ldr r1, [r0, #0x310]
	bic r1, r1, #(0x7 << 0)
	orr r1, r1, #0x3
	bic r1, r1, #(0x3 << 4)
	bic r1, r1, #(0x3 << 8)
	orr r1, r1, #(0x1 << 30)
	bic r1, r1, #(0x1 << 31)

	/*set SDHC-port3 arbitration*/
	ldr r0, =AHBMAX_BASE_ADDR
	ldr r1, [r0, #0xb00]
	bic r1, r1, #(0x7 << 0)
	orr r1, r1, #0x1
#endif

#ifdef CONFIG_WIFI_SDHC_PORT2
	/*set PARK to SDHC-port3*/
	ldr r0, =AHBMAX_BASE_ADDR
	ldr r1, [r0, #0x10]
	bic r1, r1, #(0x7 << 0)
	orr r1, r1, #0x2
	bic r1, r1, #(0x3 << 4)
	bic r1, r1, #(0x3 << 8)
	orr r1, r1, #(0x1 << 30)
	bic r1, r1, #(0x1 << 31)

	ldr r1, [r0, #0x110]
	bic r1, r1, #(0x7 << 0)
	orr r1, r1, #0x2
	bic r1, r1, #(0x3 << 4)
	bic r1, r1, #(0x3 << 8)
	orr r1, r1, #(0x1 << 30)
	bic r1, r1, #(0x1 << 31)

	ldr r1, [r0, #0x210]
	bic r1, r1, #(0x7 << 0)
	orr r1, r1, #0x2
	bic r1, r1, #(0x3 << 4)
	bic r1, r1, #(0x3 << 8)
	orr r1, r1, #(0x1 << 30)
	bic r1, r1, #(0x1 << 31)

	ldr r1, [r0, #0x310]
	bic r1, r1, #(0x7 << 0)
	orr r1, r1, #0x2
	bic r1, r1, #(0x3 << 4)
	bic r1, r1, #(0x3 << 8)
	orr r1, r1, #(0x1 << 30)
	bic r1, r1, #(0x1 << 31)

	/*set SDHC-port2 arbitration*/
	ldr r0, =AHBMAX_BASE_ADDR
	ldr r1, [r0, #0xA00]
	bic r1, r1, #(0x7 << 0)
#endif

	/*set JMP step to zero*/
	ldr r0, =M4IF_BASE_ADDR
	ldr r1, [r0, #0x48]
	bic r1, r1, #(0x3 << 8)
	str r1, [r0, #0x48]
.endm

.macro init_clock
	ldr r0, =ROM_SI_REV
	ldr r1, [r0]
	cmp r1, #0x20

	/* For TO2 only, set LDO to 1.3V */
	ldr r0, =0x53fa8000
	ldr r1, =0x00194005
	streq r1, [r0, #0x04]

	ldr r0, CCM_BASE_ADDR_W

	/* Gate of clocks to the peripherals first */
	ldr r1, =0x3FFFFFFF
	str r1, [r0, #CLKCTL_CCGR0]
	ldr r1, =0x0
	str r1, [r0, #CLKCTL_CCGR1]
	str r1, [r0, #CLKCTL_CCGR2]
	str r1, [r0, #CLKCTL_CCGR3]
	str r1, [r0, #CLKCTL_CCGR7]
	ldr r1, =0x00030000
	str r1, [r0, #CLKCTL_CCGR4]
	ldr r1, =0x00FFF030
	str r1, [r0, #CLKCTL_CCGR5]
	ldr r1, =0x0F00030F
	str r1, [r0, #CLKCTL_CCGR6]

	/* Switch ARM to step clock */
	mov r1, #0x4
	str r1, [r0, #CLKCTL_CCSR]

	setup_pll PLL1_BASE_ADDR, 800

        setup_pll PLL3_BASE_ADDR, 400

        /* Switch peripheral to PLL3 */
        ldr r0, CCM_BASE_ADDR_W
        ldr r1, CCM_VAL_0x00015154
        str r1, [r0, #CLKCTL_CBCMR]
        ldr r1, CCM_VAL_0x02888945
        orr r1, r1, #(1 << 16)
        str r1, [r0, #CLKCTL_CBCDR]
        /* make sure change is effective */
1:      ldr r1, [r0, #CLKCTL_CDHIPR]
        cmp r1, #0x0
        bne 1b

        setup_pll PLL2_BASE_ADDR, CONFIG_SYS_PLL2_FREQ

	/* Switch peripheral to PLL2 */
	ldr r0, CCM_BASE_ADDR_W
	ldr r1, CCM_VAL_0x00808145
	orr r1, r1, #(CONFIG_SYS_AHB_PODF << 10)
	orr r1, r1, #(CONFIG_SYS_AXIA_PODF << 16)
	orr r1, r1, #(CONFIG_SYS_AXIB_PODF << 19)
	str r1, [r0, #CLKCTL_CBCDR]

	ldr r1, CCM_VAL_0x00016154
	str r1, [r0, #CLKCTL_CBCMR]

	/*change uart clk parent to pll2*/
	ldr r1, [r0, #CLKCTL_CSCMR1]
	and r1, r1, #0xfcffffff
	orr r1, r1, #0x01000000
	str r1, [r0, #CLKCTL_CSCMR1]

	/* make sure change is effective */
1:      ldr r1, [r0, #CLKCTL_CDHIPR]
	cmp r1, #0x0
	bne 1b

        setup_pll PLL3_BASE_ADDR, 216

	setup_pll PLL4_BASE_ADDR, 455

	/* Set the platform clock dividers */
	ldr r0, PLATFORM_BASE_ADDR_W
	ldr r1, PLATFORM_CLOCK_DIV_W
	str r1, [r0, #PLATFORM_ICGC]

	ldr r0, CCM_BASE_ADDR_W
	mov r1, #1
	str r1, [r0, #CLKCTL_CACRR]

	/* Switch ARM back to PLL 1. */
	mov r1, #0x0
	str r1, [r0, #CLKCTL_CCSR]

	/* make uart div=6*/
	ldr r1, [r0, #CLKCTL_CSCDR1]
	and r1, r1, #0xffffffc0
	orr r1, r1, #0x0a
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

#ifdef CONFIG_ADJUST_WIFI_FEC_PERFORMANCE
        /*increase WIFI & FEC priority of accessing bus*/
        init_m4if
#endif

	init_clock

	mov pc, lr

/* Board level setting value */
CCM_BASE_ADDR_W:        .word CCM_BASE_ADDR
CCM_VAL_0x00016154:     .word 0x00016154
CCM_VAL_0x00808145:     .word 0x00808145
CCM_VAL_0x00015154:     .word 0x00015154
CCM_VAL_0x02888945:     .word 0x02888945
W_DP_OP_800:           	.word DP_OP_800
W_DP_MFD_800:           .word DP_MFD_800
W_DP_MFN_800:           .word DP_MFN_800
W_DP_OP_600:            .word DP_OP_600
W_DP_MFD_600:           .word DP_MFD_600
W_DP_MFN_600:           .word DP_MFN_600
W_DP_OP_400:            .word DP_OP_400
W_DP_MFD_400:           .word DP_MFD_400
W_DP_MFN_400:           .word DP_MFN_400
W_DP_OP_216:            .word DP_OP_216
W_DP_MFD_216:           .word DP_MFD_216
W_DP_MFN_216:           .word DP_MFN_216
W_DP_OP_455:            .word DP_OP_455
W_DP_MFD_455:           .word DP_MFD_455
W_DP_MFN_455:           .word DP_MFN_455
PLATFORM_BASE_ADDR_W:   .word ARM_BASE_ADDR
PLATFORM_CLOCK_DIV_W:   .word 0x00000124
