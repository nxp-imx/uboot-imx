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
image_len:              .long 4*1024
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
    push    {r0-r6, lr}
    /* We should distinguish USB recovery mode(SDP mode) and internal boot mode.
    If ROM runs in SDP mode, then it needn't load boot code from storage media.
    If ROM runs in SDP mode, then r0 must be 0x00
    If ROM runs in internal boot mode, then r0 should be larger than IRAM base address. */
    mov     r7,	r0

/* DDR3 script for SMD and ARM2 CPU3 board */
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

    ldr r0, =0x53fa8600
    str r1, [r0, #0xf0]
    mov r2, #0x00000000
    str r2, [r0, #0xf4]
    str r2, [r0, #0xfc]

    ldr r0, =0x53fa8700
    str r2, [r0, #0x14]
    str r1, [r0, #0x18]
    str r1, [r0, #0x1c]
    str r1, [r0, #0x20]
    mov r2, #0x04000000
    str r2, [r0, #0x24]
    str r1, [r0, #0x28]
    str r1, [r0, #0x2c]

    /* Initialize DDR3 memory for calibration */
    ldr r0, =ESDCTL_BASE_ADDR

    ldr r1, =0x01340135
    str r1, [r0, #0x07c]
    ldr r1, =0x01390139
    str r1, [r0, #0x080]
    ldr r1, =0x35343534
    str r1, [r0, #0x088]
    ldr r1, =0x4f485146
    str r1, [r0, #0x090]

    ldr r1, =0x00000800
    str r1, [r0, #0xf8]

    ldr r1, =0x00011740
    str r1, [r0, #0x018]

    ldr r1, =0xc3190000
    str r1, [r0, #0x00]

    ldr r1, =0x9f515333
    str r1, [r0, #0x0C]

    ldr r1, =0xb68e8a63
    str r1, [r0, #0x10]

    ldr r1, =0x01ff00db
    str r1, [r0, #0x14]

    ldr r1, =0x000026d2
    str r1, [r0, #0x2C]

    ldr r1, =0x009f0e21
    str r1, [r0, #0x30]

    ldr r1, =0x12373030
    str r1, [r0, #0x08]

    ldr r1, =0x4402002d
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

//    ldr r1, =0x04b80003
    ldr r1, =0x04b90003
    str r1, [r0, #0x40]

    ldr r1, =0x00022227
    str r1, [r0, #0x58]

    ldr r1, =0x00000000
    str r1, [r0, #0x1C]

    ldr r6, =0x0

retry:
	add r6, r6, #0x1

    //delay
    ldr r1, =0x0
100:
    nop
    add r1, r1, #0x1
    cmp r1, #0x200
    bne 100b

    /* DQS calibration */
    ldr r3, =0x55aaaa55
    ldr r4, =0x55555555
    ldr r5, =0xAAAAAAAA
    str r3, [r0, #0x0cc]

    ldr r1, =0x0
    ldr r2, =0x0
110:
    ldr r0, =CSD0_BASE_ADDR
    add r0, r0, #0x10000000
    add r0, r0, r2
    str r4, [r0, #0x000]
    str r5, [r0, #0x004]
    str r5, [r0, #0x008]
    str r4, [r0, #0x00c]

    add r2, r2, #0x10
    add r1, r1, #0x1
    cmp r1, #0x4
    bne 110b

    ldr r0, =ESDCTL_BASE_ADDR
    ldr r1, [r0, #0x07c]
    orr r1, r1, #(1 << 30)
    str r1, [r0, #0x07c]

    ldr r1, [r0, #0x07c]
    orr r1, r1, #(1 << 28)
    str r1, [r0, #0x07c]

    // check for DQS calibration complete
120:
    ldr r1, [r0, #0x07c]
    mov r2, #0x00000000
    orr r2, r2, #(1 << 28)
    and r2, r1, r2
    cmp r2, #0x0
    bne 120b

    // check for calibration error
    mov r2, #0x00000000
    orr r2, r2, #(1 << 12)
    and r2, r1, r2
    cmp r2, #0x0
    beq 130f
    ldr r0, =WDOG1_BASE_ADDR
    ldr r1, =0x0033
    strh r1, [r0, #0x00]
    ldr r1, =0x0037
    strh r1, [r0, #0x00]
    ldr r1, =0x5555
    strh r1, [r0, #0x02]
    ldr r1, =0xaaaa
    strh r1, [r0, #0x02]
//	bne retry
130:

    /* Reset */
    ldr r1, [r0, #0x07c]
    orr r1, r1, #(1 << 31)
    str r1, [r0, #0x07c]

140:
    ldr r1, [r0, #0x07c]
    mov r2, #0x00000000
    orr r2, r2, #(1 << 31)
    and r1, r1, r2
    cmp r1, #0x0
    bne 140b

    ldr r2, =CSD0_BASE_ADDR
    add r2, r2, #0x1100000
    ldr r3, =0xf0f0f0f0
    ldr r4, =0xc3c3c3c3
    str r3, [r2, #0x000]
    str r4, [r2, #0x004]
    ldr r3, [r2, #0x000]
    ldr r4, [r2, #0x004]

    /* RD calibration */
    ldr r4, =0x55555555
    ldr r5, =0xAAAAAAAA
    ldr r1, =0x0
    ldr r2, =0x0

150:
    ldr r0, =CSD0_BASE_ADDR
    add r0, r0, #0x10000000
    add r0, r0, r2
    str r4, [r0, #0x000]
    str r5, [r0, #0x004]
    str r5, [r0, #0x008]
    str r4, [r0, #0x00c]

    add r2, r2, #0x10
    add r1, r1, #0x1
    cmp r1, #0x4
    bne 150b

    ldr r0, =ESDCTL_BASE_ADDR
    ldr r1, =0x04000050
    str r1, [r0, #0x01c]
    ldr r1, =0x00000030
    str r1, [r0, #0x0a0]

    // check for RD calibration complete
160:
    ldr r1, [r0, #0x0a0]
    and r2, r1, #0x10
    cmp r2, #0x0
    bne 160b

    // check for calibration error
    and r2, r1, #0xf
    cmp r2, #0x0
    beq 170f
    ldr r0, =WDOG1_BASE_ADDR
    ldr r1, =0x0033
    strh r1, [r0, #0x00]
    ldr r1, =0x0037
    strh r1, [r0, #0x00]
    ldr r1, =0x5555
    strh r1, [r0, #0x02]
    ldr r1, =0xaaaa
    strh r1, [r0, #0x02]
//	bne retry
170:

    /* Reset */
    ldr r1, [r0, #0x07c]
    orr r1, r1, #(1 << 31)
    str r1, [r0, #0x07c]

180:
    ldr r1, [r0, #0x07c]
    mov r2, #0x00000000
    orr r2, r2, #(1 << 31)
    and r1, r1, r2
    cmp r1, #0x0
    bne 180b

    ldr r2, =CSD0_BASE_ADDR
    add r2, r2, #0x1000000
    ldr r3, =0x01234567
    ldr r4, =0x89abcdef
    str r3, [r2, #0x000]
    str r4, [r2, #0x004]
    ldr r3, [r2, #0x000]
    ldr r4, [r2, #0x004]

    //delay
    ldr r1, =0x0
181:
    nop
    add r1, r1, #0x1
    cmp r1, #0x200
    bne 181b

    /* WR calibration */
    ldr r4, =0x55555555
    ldr r5, =0xAAAAAAAA
    ldr r1, =0x0
    ldr r2, =0x0

190:
    ldr r0, =CSD0_BASE_ADDR
    add r0, r0, #0x10000000
    add r0, r0, r2
    str r4, [r0, #0x000]
    str r5, [r0, #0x004]
    str r5, [r0, #0x008]
    str r4, [r0, #0x00c]

    add r2, r2, #0x10
    add r1, r1, #0x1
    cmp r1, #0x4
    bne 190b

    ldr r0, =ESDCTL_BASE_ADDR
    ldr r1, =0x04000050
    str r1, [r0, #0x01c]
    ldr r1, =0x00000030
    str r1, [r0, #0x0a4]

    // check for WR calibration complete
200:
    ldr r1, [r0, #0x0a4]
    and r2, r1, #0x10
    cmp r2, #0x0
    bne 200b

    // check for calibration error
    and r2, r1, #0xf
    cmp r2, #0x0
    beq 210f
    ldr r0, =WDOG1_BASE_ADDR
    ldr r1, =0x0033
    strh r1, [r0, #0x00]
    ldr r1, =0x0037
    strh r1, [r0, #0x00]
    ldr r1, =0x5555
    strh r1, [r0, #0x02]
    ldr r1, =0xaaaa
    strh r1, [r0, #0x02]
//	bne retry
210:

    /* Reset */
    ldr r1, [r0, #0x07c]
    orr r1, r1, #(1 << 31)
    str r1, [r0, #0x07c]

220:
    ldr r1, [r0, #0x07c]
    mov r2, #0x00000000
    orr r2, r2, #(1 << 31)
    and r1, r1, r2
    cmp r1, #0x0
    bne 220b

    ldr r2, =CSD0_BASE_ADDR
    add r2, r2, #0x1000000
    ldr r3, =0x01234567
    ldr r4, =0x89abcdef
    str r3, [r2, #0x000]
    str r4, [r2, #0x004]
    ldr r3, [r2, #0x000]
    ldr r4, [r2, #0x004]

    /* Reset */
    ldr r1, [r0, #0x07c]
    orr r1, r1, #(1 << 31)
    str r1, [r0, #0x07c]

230:
    ldr r1, [r0, #0x07c]
    mov r2, #0x00000000
    orr r2, r2, #(1 << 31)
    and r1, r1, r2
    cmp r1, #0x0
    bne 230b

//	cmp r6, #0xff  // for test only
//	bls retry  // for test only

    ldr r5, [r0, #0x07c]
    bic r5, r5, #(1 << 30)  // clear bit 30
    ldr r6, [r0, #0x080]
    ldr r3, [r0, #0x088]
    ldr r4, [r0, #0x090]

	// Disable the ESDCTL
    ldr r0, =ESDCTL_BASE_ADDR
    ldr r1, =0x00000000
    str r1, [r0, #0x00]

    //delay
    ldr r1, =0x0
260:
    nop
    nop
    nop
    nop
    add r1, r1, #0x1
    cmp r1, #0x100
    bne 260b
#if 0
    /* Reset the DDR controller */
    ldr r1, [r0, #0x01c]
    orr r1, r1, #(1 << 15)
    str r1, [r0, #0x01c]

240:
    ldr r1, [r0, #0x01c]
    mov r2, #0x00000000
    orr r2, r2, #(1 << 14)
    and r1, r1, r2
    cmp r1, #0x0
    beq 240b

    ldr r1, [r0, #0x018]
    orr r1, r1, #(1 << 1)
    str r1, [r0, #0x018]

250:
    ldr r1, [r0, #0x018]
    mov r2, #0x00000000
    orr r2, r2, #(1 << 1)
    and r1, r1, r2
    cmp r1, #0x0
    bne 250b

    ldr r1, =0x00000000
    str r1, [r0, #0x01c]
#endif
    nop
    nop
    nop
    nop

    /* Re-initialize DDR3 memory with calibrated data */
    ldr r0, =ESDCTL_BASE_ADDR

    str r5, [r0, #0x07c]
    str r6, [r0, #0x080]
    str r3, [r0, #0x088]
    str r4, [r0, #0x090]

    ldr r1, =0x00011740
    str r1, [r0, #0x018]

    ldr r1, =0xc3190000
    str r1, [r0, #0x00]

    ldr r1, =0x9f515333
    str r1, [r0, #0x0C]

    ldr r1, =0xb68e8a63
    str r1, [r0, #0x10]

    ldr r1, =0x01ff00db
    str r1, [r0, #0x14]

    ldr r1, =0x000026d2
    str r1, [r0, #0x2C]

    ldr r1, =0x009f0e21
    str r1, [r0, #0x30]

    ldr r1, =0x12373030
    str r1, [r0, #0x08]

    ldr r1, =0x4402002d
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

    /* Different ROM address for TO 1.0 & TO 2.x */
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


2:  blx r6 /* This address might change in future ROM versions */
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
    cmp     r5, #0x20       /* check silicon revision to determine the function entry address */

    ldrlt r4, =0x00000edd   /* function entry in TO1 ROM */
    ldrge r4, =0x0040487d   /* function entry in TO2 ROM */
    blx     r4

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

    bx lr          /* return back to ROM code */

DDR_DEST_ADDR:    .word   TEXT_BASE
COPY_SIZE:        .word   _end - TEXT_BASE
BOOT_DATA:        .word   TEXT_BASE
                  .word   _end - TEXT_BASE
                  .word   0

#endif
