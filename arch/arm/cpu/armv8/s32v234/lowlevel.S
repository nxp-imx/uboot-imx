/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2014 Freescale Semiconductor
 * (C) Copyright 2017-2018 NXP
 *
 * Extracted from armv8/start.S
 */

#include <config.h>
#include <linux/linkage.h>
#include <asm/gic.h>
#include <asm/macro.h>
#include "mp.h"
#include <asm/armv8/mmu.h>

#define DMA_CHANNEL 1
#define DMA_CR					(EDMA_BASE_ADDR)
#define DMA_ES					(EDMA_BASE_ADDR + 0x4)
#define DMA_TCDn(channel)			(EDMA_BASE_ADDR + 0x1000 + \
							0x20 * (channel))
#define DMA_TCDn_SADDR(channel)			(DMA_TCDn(channel))
#define DMA_TCDn_SOFF(channel)			(DMA_TCDn(channel) + 0x4)
#define DMA_TCDn_NBYTES_MLNO(channel)		(DMA_TCDn(channel) + 0x8)
#define DMA_TCDn_DADDR(channel)			(DMA_TCDn(channel) + 0x10)
#define DMA_TCDn_DOFF(channel)			(DMA_TCDn(channel) + 0x14)
#define DMA_TCDn_CITER_ELINKNO(channel)		(DMA_TCDn(channel) + 0x16)
#define DMA_TCDn_CSR(channel)			(DMA_TCDn(channel) + 0x1C)
#define DMA_TCDn_BITER_ELINKNO(channel)		(DMA_TCDn(channel) + 0x1E)

#define SWT_SR(SWTn_BASE_ADDR) (SWTn_BASE_ADDR + 0x10)

/*************************************************************************
 *
 * void dma_mem_clr(start, size);
 *
 * Clear a memory region of size 'size' starting with address 'start'
 * (using DMA)
 *
 *************************************************************************/

ENTRY(dma_mem_clr)
	/*
	 * x0: start address of memory to clear
	 * x1: size of memory area to clear
	 */
dma_mem_clr:

	/* DMA_TCDn_SADDR */
	ldr x9, =DMA_TCDn_SADDR(DMA_CHANNEL)
	ldr x10, =initvar
	/* Point DMA source address to initialization data block */
	str w10, [x9]

	/* DMA_TCDn_SOFF */
	ldr x9, =DMA_TCDn_SOFF(DMA_CHANNEL)
	ldr x10, =0x03030000
	str w10, [x9]

	/* DMA_TCDn_NBYTES_MLNO */
	ldr x9, =DMA_TCDn_NBYTES_MLNO(DMA_CHANNEL)
	/* load the size */
	mov x10, x1
	str x10, [x9]

	/* DMA_TCDn_DADDR */
	ldr x9, =DMA_TCDn_DADDR(DMA_CHANNEL)
	/* load the start address */
	mov x10, x0
	str x10, [x9]

	/* DMA_TCDn_DOFF */
	ldr x9, =DMA_TCDn_DOFF(DMA_CHANNEL)
	ldr w10, =0x8
	str w10, [x9]

	/* DMA_TCDn_CITER_ELINKNO */
	ldr x9, =DMA_TCDn_CITER_ELINKNO(DMA_CHANNEL)
	ldr w10, =0x1
	strb w10, [x9]

	/* DMA_TCDn_CSR */
	ldr x9, =DMA_TCDn_CSR(DMA_CHANNEL)
	ldr w10, =0x1
	strb w10, [x9]

	/* DMA_TCDn_BITER_ELINKNO */
	ldr x9, =DMA_TCDn_BITER_ELINKNO(DMA_CHANNEL)
	ldr w10, =0x1
	strb w10, [x9]

	/* loop until write is done */
ctrl_status:
	ldr x9, =DMA_TCDn_CSR(DMA_CHANNEL)
	ldr w10, [x9]
	/* Check transfer done */
	and w10, w10, #0x0080
	sub w10, w10, #0x0080
	cbnz w10, ctrl_status

	ret

ENDPROC(dma_mem_clr)


ENTRY(lowlevel_init)

	mov	x29, lr			/* Save LR */

	branch_if_slave x0, 1f

watchdog_cortexm_disable:
	/* disable SWT4 watchdog*/
	ldr x0, =SWT_SR(SWT4_BASE_ADDR)
	ldr w1, =0xC520
	str w1, [x0]
	ldr w1, =0xD928
	str w1, [x0]
	ldr x0, =SWT4_BASE_ADDR
	ldr x1, [x0]
	and x1, x1, 0xFFFFFFFE
	str x1, [x0]
	ldr x1, [x0]
	orr x1, x1, 0x40
	str x1, [x0]

sram_init:
	/* Size of the SRAM memory to init */
	ldr x0,  =__bss_start
	ldr x1, =IRAM_BASE_ADDR
	add x1, x1, IRAM_SIZE
	sub x1, x1, x0

	/* Start address of the SRAM memory to init */
	ldr x0, =__bss_start

	bl dma_mem_clr

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

	mrs	x0, S3_1_c15_c2_1
	orr	x0, x0, #(1 << 6)
	msr	S3_1_c15_c2_1, x0
	isb
	branch_if_master x0, x1, 2f

	/*
	 * Slave should wait for master clearing spin table and
	 * the mmu page tables.
	 * This sync prevent salves observing incorrect
	 * value of spin table and jumping to wrong place.
	 */

	wfe
	tlbi	alle3
	dsb	sy
	isb
	isb

	ldr	x0, =CPU_RELEASE_ADDR
	ldr	x1, =0x1000
	add	x0, x0, x1
	msr	ttbr0_el3, x0
	msr	ttbr0_el2, x0

	/* PS = 40 bits, 1 Tbyte; SH0 = 2 (Outer shareable);
	 * ORGN0 = 1 - Normal memory, outer write-back write-allocate cacheable
	 * IRGN0 = 1 - Normal memory, inner write-back write-allocate cacheable
	 * T0SZ=24
	 */
	ldr	x0, =(2<<16) | (2<<12) | (1<<10) | (1<<8) | (24 << 0)
	msr	tcr_el3, x0
	msr	tcr_el2, x0

	ldr	x0,=MEMORY_ATTRIBUTES
	msr	mair_el3, x0
	msr	mair_el2, x0

	/* icache & dcache */
	mrs	x0, sctlr_el3
	/* bits SA(3)  M(0) */
	ldr	x1, =(1<<12) | (1<<5) | (1 << 3) | (1<<2) |(1 << 0)
	orr	x0, x0, x1     /* set bits */
	msr	sctlr_el3, x0
	isb

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
	ubfm	x9, x0, #0, #15         /* x9 contains MPIDR[15:0] */
	/*
	 * offset of the spin table element for this core from start of spin
	 * table (each elem is padded to 64 bytes)
	 */
	lsl	x1, x10, #6
	ldr	x0, =CPU_RELEASE_ADDR
	/* physical address of this cpus spin table element */
	add	x11, x1, x0
	ldr	x0, =__real_cntfrq
	ldr     x0, [x0]
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

	/* Ensure that the literals used by the secondary boot page are
	 * assembled within it
	 */
	.ltorg

	/* 64 bit alignment for elements accessed as data */
	.align 4
	.global __real_cntfrq
__real_cntfrq:
	.quad COUNTER_FREQUENCY
	.globl __secondary_boot_page_size
	.type __secondary_boot_page_size, %object
	/* Secondary Boot Page ends here */
__secondary_boot_page_size:
	.quad .-secondary_boot_page

	.data
initvar:
	.quad 0x0
