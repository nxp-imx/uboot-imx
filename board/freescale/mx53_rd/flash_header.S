/*
 * Copyright (C) 2010 Freescale Semiconductor, Inc.
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

#ifdef	CONFIG_FLASH_HEADER
#ifndef CONFIG_FLASH_HEADER_OFFSET
# error "Must define the offset of flash header"
#endif

.section ".text.flasheader", "x"
	b	_start
	.org	CONFIG_FLASH_HEADER_OFFSET

ivt_header:             .long 0x402000D1    /* Tag=0xD1, Len=0x0020, Ver=0x40 */
app_code_jump_v:        .long (0xF8006000 + (plugin_start - TEXT_BASE))
reserv1:                .long 0x0
dcd_ptr:                .long 0x0
boot_data_ptr:          .long (0xF8006000 + (boot_data - TEXT_BASE))
self_ptr:               .long (0xF8006000 + (ivt_header - TEXT_BASE))
app_code_csf:           .long 0x0
reserv2:                .long 0x0
boot_data:              .long 0xF8006000
image_len:              .long 2*1024
plugin:                 .long 0x1

/* Second IVT to give entry point into the bootloader copied to DDR */
ivt2_header:            .long 0x402000D1    /*Tag=0xD1, Len=0x0020, Ver=0x40 */
app2_code_jump_v:       .long _start   /* Entry point for the bootloader */
reserv3:                .long 0x0
dcd2_ptr:               .long 0x0
boot_data2_ptr:         .long boot_data2
self_ptr2:              .long ivt2_header
app_code_csf2:          .long 0x0
reserv4:                .long 0x0
boot_data2:             .long TEXT_BASE
image_len2:             .long  _end - TEXT_BASE
plugin2:                .long 0x0


/* Here starts the plugin code */
plugin_start:
    /* Save the return address and the function arguments */
    push    {r0-r3, lr}

    ldr r0, =ROM_SI_REV
    ldr r1, [r0]

    cmp r1, #0x20

/* DDR2 script for EVK, ARD, and ARM2 CPU2 boards */
#if !defined(CONFIG_MX53_ARM2_DDR3)

    /* IOMUX Setup */
    ldr r0, =0x53fa8500
    moveq r1, #0x00200000
    movne r1, #0x00380000
    add r2, r1, #0x40
    mov r3, #0x00280000
    mov r4, #0x00200000

    str r1, [r0, #0x54]
    str r1, [r0, #0x60]
    str r1, [r0, #0x94]
    str r1, [r0, #0x84]
    str r2, [r0, #0x58]
    str r2, [r0, #0x68]
    str r2, [r0, #0x90]
    str r2, [r0, #0x7c]
    str r2, [r0, #0x64]
    str r2, [r0, #0x80]

    streq r1, [r0, #0x70]
#if defined(CONFIG_MX53_EVK)
    strne r4, [r0, #0x70]
#else
    strne r1, [r0, #0x70]
#endif
    str r3, [r0, #0x74]
    streq r1, [r0, #0x78]
#if defined(CONFIG_MX53_EVK)
    strne r4, [r0, #0x78]
#else
    strne r1, [r0, #0x78]
#endif
    str r3, [r0, #0x88]

    ldr r0, =0x53fa86f0
    str r3, [r0, #0x0]
    mov r2, #0x00000200
    str r2, [r0, #0x4]
    mov r2, #0x00000000
    str r2, [r0, #0xc]

    ldr r0, =0x53fa8700
    str r2, [r0, #0x14]
    str r1, [r0, #0x18]
    str r1, [r0, #0x1c]
    str r3, [r0, #0x20]

    moveq r2, #0x02000000
    movne r2, #0x06000000

    str r2, [r0, #0x24]
    str r1, [r0, #0x28]
    str r1, [r0, #0x2c]

    /* Initialize DDR2 memory - Hynix H5PS2G83AFR */
    ldr r0, =ESDCTL_BASE_ADDR

    ldreq r1, =0x34333936
    ldrne r1, =0x2b2f322f

    str r1, [r0, #0x088]

    ldreq r1, =0x49434942
    ldrne r1, =0x43403a3a
    str r1, [r0, #0x090]

    /* add 3 logic unit of delay to sdclk  */
    ldr r1, =0x00000f00
    strne r1, [r0, #0x098]

    ldr r1, =0x00000800
    str r1, [r0, #0x0F8]

    ldreq r1, =0x01350138
    ldrne r1, =0x020c0211

    str r1, [r0, #0x07c]

    ldreq r1, =0x01380139
    ldrne r1, =0x0133014b

    str r1, [r0, #0x080]

    /* Enable bank interleaving, RALAT = 0x4, DDR2_EN = 1 */
    ldr r1, =0x00001710
    str r1, [r0, #0x018]

    ldr r1, =0xc4110000
    str r1, [r0, #0x00]

    ldr r1, =0x4d5122d2
    str r1, [r0, #0x0C]

    ldr r1, =0x92d18a22
    str r1, [r0, #0x10]

    ldr r1, =0x00c70092
    str r1, [r0, #0x14]

    ldr r1, =0x000026d2
    str r1, [r0, #0x2C]

    ldr r1, =0x009f000e
    str r1, [r0, #0x30]

    ldr r1, =0x12272000
    str r1, [r0, #0x08]

    ldr r1, =0x00030012
    str r1, [r0, #0x04]

    ldr r1, =0x04008010
    str r1, [r0, #0x1C]

    ldr r1, =0x00008032
    str r1, [r0, #0x1C]

    ldr r1, =0x00008033
    str r1, [r0, #0x1C]

    ldr r1, =0x00008031
    str r1, [r0, #0x1C]

    ldr r1, =0x0b5280b0
    str r1, [r0, #0x1C]

    ldr r1, =0x04008010
    str r1, [r0, #0x1C]

    ldr r1, =0x00008020
    str r1, [r0, #0x1C]

    ldr r1, =0x00008020
    str r1, [r0, #0x1C]

    ldr r1, =0x0a528030
    str r1, [r0, #0x1C]

    ldr r1, =0x03c68031
    str r1, [r0, #0x1C]

    ldreq r1, =0x00448031
    ldrne r1, =0x00468031
    str r1, [r0, #0x1C]

    /* Even though Rev B does not have DDR on CSD1, keep these
      * mode register initialization sequences for future uses since
      * it does not hurt to keep them
      */
    ldr r1, =0x04008018
    str r1, [r0, #0x1C]

    ldr r1, =0x0000803a
    str r1, [r0, #0x1C]

    ldr r1, =0x0000803b
    str r1, [r0, #0x1C]

    ldr r1, =0x00008039
    str r1, [r0, #0x1C]

    ldr r1, =0x0b528138
    str r1, [r0, #0x1C]

    ldr r1, =0x04008018
    str r1, [r0, #0x1C]

    ldr r1, =0x00008028
    str r1, [r0, #0x1C]

    ldr r1, =0x00008028
    str r1, [r0, #0x1C]

    ldr r1, =0x0a528038
    str r1, [r0, #0x1C]

    ldr r1, =0x03c68039
    str r1, [r0, #0x1C]

    ldreq r1, =0x00448039
    ldrne r1, =0x00468039
    str r1, [r0, #0x1C]

    ldr r1, =0x00005800
    str r1, [r0, #0x20]

    /* Enable 50ohm ODT for TO2*/
    ldreq r1, =0x00033335
    ldrne r1, =0x00033337
    str r1, [r0, #0x58]

    ldr r1, =0x00000000
    str r1, [r0, #0x1C]

/* Enable ZQ calibration for TO2 */
    ldr r1, =0x04b80003
    streq r1, [r0, #0x40]

/* For TO2 only, set LDO to 1.3V */
    ldr r0, =0x53fa8000
    ldr r1, =0x00194005
    streq r1, [r0, #0x04]

/* DDR3 script for SMD and ARM2 CPU3 board */
#else

    /* IOMUX Setup */
    ldr r0, =0x53fa8500
    mov r1, #0x00300000
    add r2, r1, #0x40

    str r1, [r0, #0x54]
    str r2, [r0, #0x58]
    str r1, [r0, #0x60]
    str r2, [r0, #0x64]
    str r2, [r0, #0x68]

    str r1, [r0, #0x70]
    str r1, [r0, #0x74]
    str r1, [r0, #0x78]
    str r2, [r0, #0x7c]
    str r2, [r0, #0x80]
    str r1, [r0, #0x84]
    str r1, [r0, #0x88]
    str r2, [r0, #0x90]
    str r1, [r0, #0x94]

    ldr r0, =0x53fa86f0
    str r1, [r0, #0x0]
    mov r2, #0x00000000
    str r2, [r0, #0x4]
    str r2, [r0, #0xc]

    ldr r0, =0x53fa8700
    str r2, [r0, #0x14]
    str r1, [r0, #0x18]
    str r1, [r0, #0x1c]
    str r1, [r0, #0x20]

    mov r2, #0x04000000
    str r2, [r0, #0x24]
    str r1, [r0, #0x28]
    str r1, [r0, #0x2c]

    /* Initialize DDR3 memory */
    ldr r0, =ESDCTL_BASE_ADDR

    ldr r1, =0x32383535
    str r1, [r0, #0x088]

    ldr r1, =0x40383538
    str r1, [r0, #0x090]

    ldr r1, =0x0136014d
    str r1, [r0, #0x07c]

    ldr r1, =0x01510141
    str r1, [r0, #0x080]

    ldr r1, =0x00091740
    str r1, [r0, #0x018]

    ldr r1, =0xc4190000
    str r1, [r0, #0x00]

    ldr r1, =0x565a7543
    str r1, [r0, #0x0C]

    ldr r1, =0xb6ae8aa3
    str r1, [r0, #0x10]

    ldr r1, =0x01ff00db
    str r1, [r0, #0x14]

    ldr r1, =0x000026d2
    str r1, [r0, #0x2C]

    ldr r1, =0x009f0e21
    str r1, [r0, #0x30]

    ldr r1, =0x12272000
    str r1, [r0, #0x08]

    ldr r1, =0x00030012
    str r1, [r0, #0x04]

    ldr r1, =0x00008032
    str r1, [r0, #0x1C]

    ldr r1, =0x00008033
    str r1, [r0, #0x1C]

    ldr r1, =0x00028031
    str r1, [r0, #0x1C]

    ldr r1, =0x092080b0
    str r1, [r0, #0x1C]

    ldr r1, =0x04008040
    str r1, [r0, #0x1C]

    ldr r1, =0x0000803a
    str r1, [r0, #0x1C]

    ldr r1, =0x0000803b
    str r1, [r0, #0x1C]

    ldr r1, =0x00028039
    str r1, [r0, #0x1C]

    ldr r1, =0x09208138
    str r1, [r0, #0x1C]

    ldr r1, =0x04008048
    str r1, [r0, #0x1C]

    ldr r1, =0x00001800
    str r1, [r0, #0x20]

    ldr r1, =0x04b80003
    str r1, [r0, #0x40]

    ldr r1, =0x00022227
    str r1, [r0, #0x58]

    ldr r1, =0x00000000
    str r1, [r0, #0x1C]

#endif

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

    /* Different ROM address for TO 1.0 & TO 2.0 */
    moveq r4, #0x1800
    addeq r4, r4, #0x4d
    beq 2f
    mov r4, #0x400000
    add r4, r4, #0x5000
    add r4, r4, #0xc7

2:  blx r4 /* This address might change in future ROM versions */
after_calling_rom___pu_irom_hwcnfg_setup:

/* To return to ROM from plugin, we need to fill in these argument.
  * Here is what need to do:
  * Need to construct the paramters for this function before return to ROM:
  * plugin_download(void **start, size_t *bytes, UINT32 *ivt_offset)
  */
    pop {r0-r3, lr}
    ldr r4, DDR_DEST_ADDR
    str r4, [r0]
    ldr r4, COPY_SIZE
    str r4, [r1]
    mov r4, #0x400  /* Point to the second IVT table at offset 0x42C */
    add r4, r4, #0x2C
    str r4, [r2]
    mov r0, #1

    bx lr          /* return back to ROM code */

DDR_DEST_ADDR:    .word   TEXT_BASE
COPY_SIZE:        .word   _end - TEXT_BASE
BOOT_DATA:        .word   TEXT_BASE
                  .word   _end - TEXT_BASE
                  .word   0
#endif
