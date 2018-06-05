/*
 * (C) Copyright 2009-2016 Freescale Semiconductor, Inc.
 *
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <config.h>
#include <linux/linkage.h>
#include <asm/gic.h>
#include <asm/armv7.h>
_regs_save:
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0

#ifdef CONFIG_IMX_TRUSTY_OS
ENTRY(trusty_os_init)
	isb

	/* Save current registers */
	mov     ip, r0
	adr     r0, _regs_save
	str     ip, [r0]
	add     r0, r0, #4		@ Get _regs_save from instruction offset

	str     sp, [r0]
	add     r0, r0, #4

	stmia   r0!, {r1-r12}		@ Save r1 - r12

	str     lr, [r0]
	adr     lr, end_init_tee	@ save return address to lr

	dsb

	ldr     r1, =TRUSTY_OS_ENTRY
	ldr     r0, =TRUSTY_OS_RAM_SIZE
	movs    pc, r1			@ Go to TEE codes
end_init_tee:
	/* Restore saved registers */
	adr     lr, _regs_save
	ldr     r0, [lr]
	add     lr, lr, #4

	ldr     sp, [lr]
	add     lr, lr, #4

	ldmfd   lr!, {r1-r12}
	ldr     lr, [lr]

	dsb

	bx      lr
ENDPROC(trusty_os_init)
#endif
