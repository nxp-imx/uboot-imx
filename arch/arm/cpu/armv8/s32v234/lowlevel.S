/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2014 Freescale Semiconductor
 *
 * Extracted from armv8/start.S
 */

#include <config.h>
#include <linux/linkage.h>
#include <asm/gic.h>
#include <asm/macro.h>
#include "mp.h"

ENTRY(lowlevel_init)

	mov	x29, lr			/* Save LR */

	branch_if_slave x0, 1f

watchdog_m4_disable:
	/* disable SWT4 watchdog*/
	ldr x0, =0x40086010
	ldr w1, =0xC520
	str w1, [x0]
	ldr w1, =0xD928
	str w1, [x0]
	ldr x0, =0x40086000
	ldr x1, [x0]
	and x1, x1, 0xFFFFFFFE
	str x1, [x0]
	ldr x1, [x0]
	orr x1, x1, 0x40
	str x1, [x0]

sram_init:
	/* DMA_TCDn_SOFF */
	ldr x0, =0x40003020
	ldr w1, =0x6230
	str w1, [x0]

	/* DMA_TCDn_SOFF */
	ldr x0, =0x40003024
	ldr x1, =0x03030000
	str w1, [x0]

	ldr x0,  =__bss_start
	ldr x1, =IRAM_BASE_ADDR
	add x1, x1, IRAM_SIZE
	sub x1, x1, x0

	/* DMA_TCDn_NBYTES_MLNO */
	ldr x0, =0x40003028
	str x1, [x0]

	/* DMA_TCDn_DADDR */
	ldr x0, =0x40003030
	ldr x1, =__bss_start
	str x1, [x0]

	/* DMA_TCDn_DOFF */
	ldr x0, =0x40003034
	ldr w1, =0x8
	str w1, [x0]

	/* DMA_TCDn_CITER_ELINKNO */
	ldr x0, =0x40003036
	ldr w1, =0x1
	strb w1, [x0]

	/* DMA_TCDn_CSR */
	ldr x0, =0x4000303C
	ldr w1, =0x1
	strb w1, [x0]

	/* DMA_TCDn_BITER_ELINKNO */
	ldr x0, =0x4000303E
	ldr w1, =0x1
	strb w1, [x0]

	/* loop until write is done */
ctrl_status:
	ldr x0, =0x4000303C
	ldr w1, [x0]
	and w1, w1, #0x0080
	sub w1, w1, #0x0080
	cbnz w1, ctrl_status

	/* turn on a53 slave cores from a53 master */
	/* deassert cores on reset */

start_slave_cores:

#if defined(CONFIG_GICV2) || defined(CONFIG_GICV3)
	branch_if_slave x0, 1f
	ldr	x0, =GICD_BASE
	bl	gic_init_secure

1:
#if defined(CONFIG_GICV3)
	ldr	x0, =GICR_BASE
	bl	gic_init_secure_percpu
#elif defined(CONFIG_GICV2)
	ldr	x0, =GICD_BASE
	ldr	x1, =GICC_BASE
	bl	gic_init_secure_percpu
#endif
#endif

	branch_if_master x0, x1, 2f

	/*
	 * Slave should wait for master clearing spin table.
	 * This sync prevent salves observing incorrect
	 * value of spin table and jumping to wrong place.
	 */
#if defined(CONFIG_GICV2) || defined(CONFIG_GICV3)
#ifdef CONFIG_GICV2
	ldr	x0, =GICC_BASE
#endif
	bl	gic_wait_for_interrupt
#endif

	/*
	 * MPIDR_EL1 Fields:
	 * MPIDR[1:0] = AFF0_CPUID <- Core ID (0,1)
	 * MPIDR[7:2] = AFF0_RES
	 * MPIDR[15:8] = AFF1_CLUSTERID <- Cluster ID (0,1,2,3)
	 * MPIDR[23:16] = AFF2_CLUSTERID
	 * MPIDR[24] = MT
	 * MPIDR[29:25] = RES0
	 * MPIDR[30] = U
	 * MPIDR[31] = ME
	 * MPIDR[39:32] = AFF3
	 *
	 * Linear Processor ID (LPID) calculation from MPIDR_EL1:
	 * (We only use AFF0_CPUID and AFF1_CLUSTERID for now
	 * until AFF2_CLUSTERID and AFF3 have non-zero values)
	 *
	 * LPID = MPIDR[15:8] | MPIDR[1:0]
	 */
	mrs	x0, mpidr_el1
	ubfm	x1, x0, #8, #15
	ubfm	x2, x0, #0, #1
	orr	x10, x2, x1, lsl #2	/* x10 has LPID */
	ubfm    x9, x0, #0, #15         /* x9 contains MPIDR[15:0] */
	/*
	 * offset of the spin table element for this core from start of spin
	 * table (each elem is padded to 64 bytes)
	 */
	lsl	x1, x10, #6
	ldr	x0, =CPU_RELEASE_ADDR
	/* physical address of this cpus spin table element */
	add	x11, x1, x0
	ldr	x0, =COUNTER_FREQUENCY
	msr	cntfrq_el0, x0	/* set with real frequency */
	str	x9, [x11, #16]	/* LPID */
	mov	x4, #1
	str	x4, [x11, #8]	/* STATUS */
	dsb	sy
	/*
	 * All slaves will enter EL2 and optionally EL1.
	 */
	bl	armv8_switch_to_el2
#ifdef CONFIG_ARMV8_SWITCH_TO_EL1
	bl	armv8_switch_to_el1
#endif

2:
	mov	lr, x29			/* Restore LR */
	ret

ENDPROC(lowlevel_init)

	/* Keep literals not used by the secondary boot page outside it */
	.ltorg

	.align 4
	.global secondary_boot_page
secondary_boot_page:
	.global __spin_table
__spin_table:
	.space CONFIG_MAX_CPUS*ENTRY_SIZE

	.align 4

	/* Ensure that the literals used by the secondary boot page are
	 * assembled within it
	 */
	.ltorg

	.align 4
	.globl __secondary_boot_page_size
	.type __secondary_boot_page_size, %object
	/* Secondary Boot Page ends here */
__secondary_boot_page_size:
	.quad .-secondary_boot_page
