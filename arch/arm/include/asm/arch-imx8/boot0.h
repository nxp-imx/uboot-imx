/*
 * Configuration settings for XEN
 *
 * SPDX-License-Identifier:	GPL-2.0+
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
