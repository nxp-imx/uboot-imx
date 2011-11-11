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
#include <asm/arch/mx53.h>

#define REG_LD_AND_STR_INIT(base)	\
	ldr r0, =base;

#define REG_LD_AND_STR_OP(i, offset, val)	\
	ldr r1, =val;	\
	ldr r2, =offset;	\
	str r1, [r0, r2];

#define REG_LD_AND_STR_END(base)


#ifdef	CONFIG_FLASH_HEADER
#ifndef CONFIG_FLASH_HEADER_OFFSET
# error "Must define the offset of flash header"
#endif

.section ".text.flasheader", "x"
	b	_start
	.org	CONFIG_FLASH_HEADER_OFFSET

ivt_header:		.long 0x402000D1/* Tag=0xD1, Len=0x0020, Ver=0x40 */
app_code_jump_v:	.long (0xF8006000 + (plugin_start - TEXT_BASE))
reserv1:		.long 0x0
dcd_ptr:		.long 0x0
boot_data_ptr:		.long (0xF8006000 + (boot_data - TEXT_BASE))
self_ptr:		.long (0xF8006000 + (ivt_header - TEXT_BASE))
app_code_csf:		.long 0x0
reserv2:		.long 0x0
boot_data:		.long 0xF8006000
image_len:		.long (3 * 1024)
plugin:			.long 0x1

/* Second IVT to give entry point into the bootloader copied to DDR */
ivt2_header:		.long 0x402000D1/*Tag=0xD1, Len=0x0020, Ver=0x40 */
app2_code_jump_v:	.long _start   /* Entry point for the bootloader */
reserv3:		.long 0x0
dcd2_ptr:		.long 0x0
boot_data2_ptr:		.long boot_data2
self_ptr2:		.long ivt2_header
app_code_csf2:		.long 0x0
reserv4:		.long 0x0
boot_data2:		.long TEXT_BASE
image_len2:		.long  _end_of_copy - TEXT_BASE + CONFIG_FLASH_HEADER_OFFSET
plugin2:		.long 0x0

/* Here starts the plugin code */
plugin_start:
	/* Save the return address and the function arguments */
	push {r0-r6, lr}
	/* We should distinguish USB recovery mode(SDP mode) and internal boot mode.
	If ROM runs in SDP mode, then it needn't load boot code from storage media.
	If ROM runs in SDP mode, then r0 must be 0x00
	If ROM runs in internal boot mode, then r0 should be larger than IRAM base address. */
	mov r7, r0

	/* IOMUX */
	REG_LD_AND_STR_INIT(IOMUXC_BASE_ADDR)
	REG_LD_AND_STR_OP(1, 0x554, 0x00300000)
	REG_LD_AND_STR_OP(2, 0x558, 0x00300040)
	REG_LD_AND_STR_OP(3, 0x560, 0x00300000)
	REG_LD_AND_STR_OP(4, 0x564, 0x00300040)
	REG_LD_AND_STR_OP(5, 0x568, 0x00300040)
	REG_LD_AND_STR_OP(6, 0x570, 0x00300000)
	REG_LD_AND_STR_OP(7, 0x574, 0x00300000)
	REG_LD_AND_STR_OP(8, 0x578, 0x00300000)
	REG_LD_AND_STR_OP(9, 0x57c, 0x00300040)
	REG_LD_AND_STR_OP(10, 0x580, 0x00300040)
	REG_LD_AND_STR_OP(11, 0x584, 0x00300000)
	REG_LD_AND_STR_OP(12, 0x588, 0x00300000)
	REG_LD_AND_STR_OP(13, 0x590, 0x00300040)
	REG_LD_AND_STR_OP(14, 0x594, 0x00300000)
	REG_LD_AND_STR_OP(15, 0x6f0, 0x00300000)
	REG_LD_AND_STR_OP(16, 0x6f4, 0x00000000)
	REG_LD_AND_STR_OP(17, 0x6fc, 0x00000000)
	REG_LD_AND_STR_OP(18, 0x714, 0x00000000)
	REG_LD_AND_STR_OP(19, 0x718, 0x00300000)
	REG_LD_AND_STR_OP(20, 0x71c, 0x00300000)
	REG_LD_AND_STR_OP(21, 0x720, 0x00300000)
	REG_LD_AND_STR_OP(22, 0x724, 0x04000000)
	REG_LD_AND_STR_OP(23, 0x728, 0x00300000)
	REG_LD_AND_STR_OP(24, 0x72c, 0x00300000)
	REG_LD_AND_STR_END(IOMUXC_BASE_ADDR)

	/* ESDCTL */
	REG_LD_AND_STR_INIT(ESDCTL_BASE_ADDR)
	REG_LD_AND_STR_OP(25, 0x088, 0x35343535)
	REG_LD_AND_STR_OP(26, 0x090, 0x4d444c44)
	REG_LD_AND_STR_OP(27, 0x07c, 0x01370138)
	REG_LD_AND_STR_OP(28, 0x080, 0x013b013c)
	REG_LD_AND_STR_OP(29, 0x0f8, 0x00000800)
	REG_LD_AND_STR_OP(30, 0x018, 0x00001740)
	REG_LD_AND_STR_OP(31, 0x000, 0xc3190000)
	REG_LD_AND_STR_OP(32, 0x00c, 0x9f5152e3)
	REG_LD_AND_STR_OP(33, 0x010, 0xb68e8a63)
	REG_LD_AND_STR_OP(34, 0x014, 0x01ff00db)
	REG_LD_AND_STR_OP(35, 0x02c, 0x000026d2)
	REG_LD_AND_STR_OP(36, 0x030, 0x009f0e21)
	REG_LD_AND_STR_OP(37, 0x008, 0x12273030)
	REG_LD_AND_STR_OP(38, 0x004, 0x0002002d)
	REG_LD_AND_STR_OP(39, 0x01c, 0x00008032)
	REG_LD_AND_STR_OP(40, 0x01c, 0x00008033)
	REG_LD_AND_STR_OP(41, 0x01c, 0x00028031)
	REG_LD_AND_STR_OP(42, 0x01c, 0x052080b0)
	REG_LD_AND_STR_OP(43, 0x01c, 0x04008040)
	REG_LD_AND_STR_OP(44, 0x01c, 0x0000803a)
	REG_LD_AND_STR_OP(45, 0x01c, 0x0000803b)
	REG_LD_AND_STR_OP(46, 0x01c, 0x00028039)
	REG_LD_AND_STR_OP(47, 0x01c, 0x05208138)
	REG_LD_AND_STR_OP(48, 0x01c, 0x04008048)
	REG_LD_AND_STR_OP(49, 0x020, 0x00005800)
	REG_LD_AND_STR_OP(50, 0x040, 0x04b80003)
	REG_LD_AND_STR_OP(51, 0x058, 0x00022227)
	REG_LD_AND_STR_OP(52, 0x01C, 0x00000000)
	REG_LD_AND_STR_END(ESDCTL_BASE_ADDR)

/*
 *  The following is to fill in those arguments for this ROM function
 * pu_irom_hwcnfg_setup(void **start, size_t *bytes, const void *boot_data)
 *
 *  This function is used to copy data from the storage media into DDR.
 *
 *  start - Initial (possibly partial) image load address on entry. Final image
 *            load address on exit.
 *  bytes - Initial (possibly partial) image size on entry. Final image size on
 *             exit.
 *  boot_data - Initial @ref ivt Boot Data load address.
 */
	adr r0, DDR_DEST_ADDR
	adr r1, COPY_SIZE
	adr r2, BOOT_DATA
before_calling_rom___pu_irom_hwcnfg_setup:
	/* We should distinguish USB recovery mode(SDP mode) and internal boot mode.
	If ROM runs in SDP mode, then it needn't load boot code from storage media.
	If ROM runs in SDP mode, then r0 must be 0x00
	If ROM runs in internal boot mode, then r0 should be larger than IRAM base address. */
	cmp r7, #0xF8000000
	bls return_sdp
	/* Different ROM address for TO 1.0 & TO 2.0 */
	ldr r3, =ROM_SI_REV
	ldr r4, [r3]

	cmp r4, #0x21
	/* TO2.1 */
	moveq r6, #0x1800
	addeq r6, r6, #0x4d
	beq 2f

	cmp r4, #0x20
	/* TO2 */
	moveq r6, #0x1800
	addeq r6, r6, #0x4d
	beq 2f

	/* TO1 */
	mov r6, #0x400000
	add r6, r6, #0x5000
	add r6, r6, #0xc7
2:
	blx r6 /* This address might change in future ROM versions */
after_calling_rom___pu_irom_hwcnfg_setup:

return_sdp:
	cmp r7, #0xF8000000
	bhi quit_plugin

	/* Workaround run plug-ins in SDP mode without USB re-enumeration.
	how it works:
	ROM running in usb download mode.
	Host manufacturing application sends SDP command to download plug-in image.
	Host manufacturing application sends SDP command to jump to plug-in image and run it.
	Plug-in starts execution and after its regular tasks plug-in will then call into ROM
	call into pl_parse_and_handle() */
	ldr r3, =ROM_SI_REV
	ldr r5, [r3]
	cmp r5, #0x20       /* check silicon revision to determine the function entry address */

	ldrlt r4, =0x00000edd   /* function entry in TO1 ROM */
	ldrge r4, =0x0040487d   /* function entry in TO2 ROM */
	blx r4

/* To return to ROM from plugin, we need to fill in these argument.
 * Here is what need to do:
 * Need to construct the paramters for this function before return to ROM:
 * plugin_download(void **start, size_t *bytes, UINT32 *ivt_offset)
 */
quit_plugin:
	pop {r0-r6, lr}
	ldr r7, DDR_DEST_ADDR
	str r7, [r0]
	ldr r7, COPY_SIZE
	str r7, [r1]
	mov r7, #0x400  /* Point to the second IVT table at offset 0x42C */
	add r7, r7, #0x2C
	str r7, [r2]
	mov r0, #1

	bx lr  /* return back to ROM code */

DDR_DEST_ADDR:    .word   TEXT_BASE
COPY_SIZE:        .word   _end - TEXT_BASE
BOOT_DATA:        .word   TEXT_BASE
                  .word   _end - TEXT_BASE
                  .word   0

#endif
