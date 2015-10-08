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

1:
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
