/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2019 NXP
 */

#ifdef CONFIG_XEN
	add     x13, x18, #0x16
	b       reset
	/* start of zImage header */
	.quad   0x80000		// Image load offset from start of RAM
	.quad   _end - _start	// Effective size of kernel image
	.quad   0		// Flags
	.quad   0		// reserved
	.quad   0		// reserved
	.quad   0		// reserved
	.byte   0x41		// Magic number, "ARM\x64"
	.byte   0x52
	.byte   0x4d
	.byte   0x64
	/* end of zImage header */
#endif

#if defined(CONFIG_SPL_BUILD)
	/*
	 * We use absolute address not PC relative address to jump.
	 * When running SPL on iMX8, the A core starts at address 0, a alias to OCRAM 0x100000,
	 * our linker address for SPL is from 0x100000. So using absolute address can jump to
	 * the OCRAM address from the alias.
	 * The alias only map first 96KB of OCRAM, so this require the SPL size can't beyond 96KB.
	 * But when using SPL DM, the size increase significantly and may exceed 96KB.
	 * That's why we have to jump to OCRAM.
	 */

	ldr	x0, =reset
	br	x0
#else
	b	reset
#endif
