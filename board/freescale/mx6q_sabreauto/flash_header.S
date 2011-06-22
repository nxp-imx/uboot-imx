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
#include <asm/arch/mx6.h>

#ifdef	CONFIG_FLASH_HEADER
#ifndef CONFIG_FLASH_HEADER_OFFSET
# error "Must define the offset of flash header"
#endif

.section ".text.flasheader", "x"
	b	_start
	.org	CONFIG_FLASH_HEADER_OFFSET

/* First IVT to copy the plugin that initializes the system into OCRAM */
ivt_header:             .long 0x402000D1    /*Tag=0xD1, Len=0x0020, Ver=0x40 */
app_code_jump_v:        .long 0x00907458    /* Plugin entry point, address after the second IVT table */
reserv1:                .long 0x0
dcd_ptr:                .long 0x0
boot_data_ptr:          .long 0x00907420
self_ptr:               .long 0x00907400
app_code_csf:           .long 0x0
reserv2:                .long 0x0
boot_data:              .long 0x00907000
image_len:              .long 16*1024  /* plugin can be upto 16KB in size */
plugin:                 .long 0x1  /* Enable plugin flag */

/* Second IVT to give entry point into the bootloader copied to DDR */
ivt2_header:            .long 0x402000D1    /*Tag=0xD1, Len=0x0020, Ver=0x40 */
app2_code_jump_v:       .long _start       /* Entry point for uboot */
reserv3:                .long 0x0
dcd2_ptr:               .long 0x0
boot_data2_ptr:         .long boot_data2
self_ptr2:              .long ivt2_header
app_code_csf2:          .long 0x0
reserv4:                .long 0x0
boot_data2:             .long TEXT_BASE
image_len2:             .long _end_of_copy  - TEXT_BASE + CONFIG_FLASH_HEADER_OFFSET
plugin2:                .long 0x0

/* Here starts the plugin code */
plugin_start:
/* Save the return address and the function arguments */
    push    {r0-r4, lr}

/*
 * Note: The DDR settings provided below are specific to Freescale development boards and are the latest settings at the time of release.
 * However, it is recommended to contact your Freescale representative in case there are any improvements to these settings.
 */

    /* Init the DDR according the init script */
    ldr r0, =CCM_BASE_ADDR
    /* select 528MHz for pre_periph_clk_sel */
    ldr r1, =0x00020324
    str r1, [r0,#0x18]

    /* IOMUX setting */
    ldr r0, =IOMUXC_BASE_ADDR
    mov r1, #0x30
    str r1, [r0,#0x5a8]
    str r1, [r0,#0x5b0]
    str r1, [r0,#0x524]
    str r1, [r0,#0x51c]
    str r1, [r0,#0x518]
    str r1, [r0,#0x50c]
    str r1, [r0,#0x5b8]
    str r1, [r0,#0x5c0]

    ldr r1, =0x00020030
    str r1, [r0,#0x5ac]
    str r1, [r0,#0x5b4]
    str r1, [r0,#0x528]
    str r1, [r0,#0x520]
    str r1, [r0,#0x514]
    str r1, [r0,#0x510]
    str r1, [r0,#0x5bc]
    str r1, [r0,#0x5c4]

    str r1, [r0,#0x56c]
    str r1, [r0,#0x578]
    str r1, [r0,#0x588]
    str r1, [r0,#0x594]
    str r1, [r0,#0x57c]

    ldr r1, =0x00003000
    str r1, [r0,#0x590]
    str r1, [r0,#0x598]
    mov r1, #0x00000000
    str r1, [r0,#0x58c]
    ldr r1, =0x00003030
    str r1, [r0,#0x59c]
    str r1, [r0,#0x5a0]

    ldr r1, =0x00000030
    str r1, [r0,#0x784]
    str r1, [r0,#0x788]
    str r1, [r0,#0x794]
    str r1, [r0,#0x79c]
    str r1, [r0,#0x7a0]
    str r1, [r0,#0x7a4]
    str r1, [r0,#0x7a8]
    str r1, [r0,#0x748]
    str r1, [r0,#0x74c]

    mov r1, #0x00020000
    str r1, [r0,#0x750]

    mov r1, #0x00000000
    str r1, [r0,#0x758]

    mov r1, #0x00020000
    str r1, [r0,#0x774]
    mov r1, #0x30
    str r1, [r0,#0x78c]
    mov r1, #0x000c0000
    str r1, [r0,#0x798]

    /* Initialize 2GB DDR3 - Micron MT41J128M */
    ldr r0, =MMDC_P0_BASE_ADDR
    ldr r2, =MMDC_P1_BASE_ADDR

    ldr r1, =0x33333333
    str r1, [r0,#0x81c]
    str r1, [r0,#0x820]
    str r1, [r0,#0x824]
    str r1, [r0,#0x828]
    str r1, [r2,#0x81c]
    str r1, [r2,#0x820]
    str r1, [r2,#0x824]
    str r1, [r2,#0x828]

    ldr r1, =0x00081740
    str r1, [r0,#0x18]
    ldr r1, =0x00008000
    str r1, [r0,#0x1c]
    ldr r1, =0x555a7975
    str r1, [r0,#0x0c]
    ldr r1, =0xff538e64
    str r1, [r0,#0x10]
    ldr r1, =0x01ff00db
    str r1, [r0,#0x14]

    ldr r1, =0x000026d2
    str r1, [r0,#0x2c]
    ldr r1, =0x005b0e21
    str r1, [r0,#0x30]
    ldr r1, =0x94444040
    str r1, [r0,#0x08]
    ldr r1, =0x00020036
    str r1, [r0,#0x04]
    ldr r1, =0x00000027
    str r1, [r0,#0x40]
    ldr r1, =0xc31a0000
    str r1, [r0,#0x00]

    ldr r1, =0x04088032
    str r1, [r0,#0x1c]
    ldr r1, =0x0408803a
    str r1, [r0,#0x1c]
    ldr r1, =0x00008033
    str r1, [r0,#0x1c]
    ldr r1, =0x0000803b
    str r1, [r0,#0x1c]
    ldr r1, =0x00428031
    str r1, [r0,#0x1c]
    ldr r1, =0x00428039
    str r1, [r0,#0x1c]

    ldr r1, =0x09408030
    str r1, [r0,#0x1c]
    ldr r1, =0x09408038
    str r1, [r0,#0x1c]
    ldr r1, =0x04008040
    str r1, [r0,#0x1c]
    ldr r1, =0x04008048
    str r1, [r0,#0x1c]

    ldr r1, =0xa5380003
    str r1, [r0,#0x800]
    ldr r1, =0xa5380003
    str r1, [r2,#0x800]

    ldr r1, =0x00005800
    str r1, [r0,#0x20]

    ldr r1, =0x00022227
    str r1, [r0,#0x818]
    ldr r1, =0x00022227
    str r1, [r2,#0x818]

    ldr r1, =0x433f033f
    str r1, [r0,#0x83c]

    ldr r1, =0x033f033f
    str r1, [r0,#0x840]

    ldr r1, =0x433f033f
    str r1, [r2,#0x83c]

    ldr r1, =0x0344033b
    str r1, [r2,#0x840]

    ldr r1, =0x4337373e
    str r1, [r0,#0x848]
    ldr r1, =0x3634303d
    str r1, [r2,#0x848]

    ldr r1, =0x35374640
    str r1, [r0,#0x850]
    ldr r1, =0x4a294b35
    str r1, [r2,#0x850]

    ldr r1, =0x001F001F
    str r1, [r0,#0x80c]
    ldr r1, =0x001F001F
    str r1, [r0,#0x810]

    ldr r1, =0x00440044
    str r1, [r2,#0x80c]
    ldr r1, =0x00440044
    str r1, [r2,#0x810]

    ldr r1, =0x00000800
    str r1, [r0,#0x8b8]
    ldr r1, =0x00000800
    str r1, [r2,#0x8b8]

    ldr r1, =0x00000000
    str r1, [r0,#0x1c]

/********************
    The following is to fill in those arguments for this ROM function
    pu_irom_hwcnfg_setup(void **start, size_t *bytes, const void *boot_data)

    This function is used to copy data from the storage media into DDR.

    start - Initial (possibly partial) image load address on entry.  Final image load address on exit.
    bytes - Initial (possibly partial) image size on entry.  Final image size on exit.
    boot_data - Initial @ref ivt Boot Data load address.
*/

    adr r0, DDR_DEST_ADDR
    adr r1, COPY_SIZE
    adr r2, BOOT_DATA

/*
 * check the _pu_irom_api_table for the address
 */
before_calling_rom___pu_irom_hwcnfg_setup:
    mov r4, #0x2000
    add r4, r4, #0xed
    blx r4	/* This address might change in future ROM versions */
after_calling_rom___pu_irom_hwcnfg_setup:

/* To return to ROM from plugin, we need to fill in these argument.
 * Here is what need to do:
 * Need to construct the paramters for this function before return to ROM:
 * plugin_download(void **start, size_t *bytes, UINT32 *ivt_offset)
 */
    pop {r0-r4, lr}
    ldr r5, DDR_DEST_ADDR
    str r5, [r0]
    ldr r5, COPY_SIZE
    str r5, [r1]
    mov r5, #0x400  /* Point to the second IVT table at offset 0x42C */
    add r5, r5, #0x2C
    str r5, [r2]
    mov r0, #1

    bx lr          /* return back to ROM code */

DDR_DEST_ADDR:    .word   TEXT_BASE
COPY_SIZE:        .word   _end_of_copy  - TEXT_BASE + CONFIG_FLASH_HEADER_OFFSET
BOOT_DATA:        .word   TEXT_BASE
                  .word   _end_of_copy  - TEXT_BASE + CONFIG_FLASH_HEADER_OFFSET
                  .word   0

#endif
