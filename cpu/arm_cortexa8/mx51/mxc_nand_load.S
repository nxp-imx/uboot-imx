/*
 * (C) Copyright 2009 Freescale Semiconductor, Inc.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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
#include <asm/arch/mx51.h>
.macro do_wait_op_done
1: 	ldr r7, [r3, #0x2C]
	ands r7, r7, #0x80000000
	beq 1b
	mov r7, #0x0
	str r7, [r3, #0x2C]
.endm   // do_wait_op_done

.section ".text.load", "x"
.globl mxc_nand_load
/*
 * R0:	NFC BUF base address
 * R1:	NFC BUF data end address
 * R2: RAM destination offset address
 * R3: NFC IP control register base
 * R4: NAND block address
 * R5: RAM destination end address
 * R6: NFC DATA register base
 * R7 - r14: 8 working buffer registers
 */
mxc_nand_load:
	ldr r0, =NFC_BASE_ADDR_AXI
        add r1, r0, #NFC_BUF_SIZE

	/* For non-nand-boot, directly quit */
        cmp pc, r0
        movlo pc, lr
        cmp pc, r1
        movhi pc, lr

	mov r4, #NFC_BUF_SIZE
	/* Get NAND page size */
	ldr r3, =NFC_BASE_ADDR
	ldr r2, [r3, #0x24]
	and r2, r2, #0x3
	cmp r2, #1
	moveq r2, #0x800
	movlt r2, #0x200
	adrls r5, NFC_PAGE_MODE
	strls r2, [r5]
	/* Get actually pre-loading size*/
	subls r1, r1, #0x800
	subls r4, r4, #0x800

	/* r1 ~ r3, r12, lr(r14) must not change in relocated operation */
	ldr r2, U_BOOT_NAND_START
1:	ldmia r0!, {r5-r11, r13}
	stmia r2!, {r5-r11, r13}
	cmp r0, r1
	blo 1b

	ldr r0, CONST_0X0FFF
	ldr r5, U_BOOT_NAND_START
	and lr, lr, r0
	add lr, lr, r5
	and r12, r12, r0
	add r12, r12, r5
	add r5, r5, #0x8
	and r0, pc, r0
	add pc, r5, r0
	nop
	nop
	nop
	nop
	nop
	adr r0, SAVE_REGS	/* Save r12 & R14(lr) */
	str r12, [r0]
	str lr, [r0, #4]
Copy_Main:
	ldr r0, =NFC_BASE_ADDR_AXI

	add r6, r0, #0x1E00
	ldr r5, =_end		/* Try get right image size */
	add r5, r2, #0x00040000 /* Fixme to get actual image size */

	mov r7, #0xFF000000
	add r7, r7, #0x00FF0000
	str r7, [r3, #0x4]
	str r7, [r3, #0x8]
	str r7, [r3, #0xC]
	str r7, [r3, #0x10]
	str r7, [r3, #0x14]
	str r7, [r3, #0x18]
	str r7, [r3, #0x1C]
	str r7, [r3, #0x20]
	mov r8, #0x7
	mov r7, #0x84
1:	add r9, r7, r8, lsr #3
	str r9, [r3, #0x0]
	subs r8, r8, #0x01
	bne 1b

	mov r7, #0
	str r7, [r3, #0x2C]

	ldr r7, NFC_PAGE_MODE
Read_Page:
	/* start_nfc_addr_ops1(pg_no, pg_off) */
	cmp r7, #0x800
	movgt r7, r4, lsr #12	/* Get the page number for 4K page */
	moveq r7, r4, lsr #11	/* Get the page number for 2K page */
	mov r7, r7, lsl #16
	str r7, [r6, #0x04]	/* Set the address */

	/* writel((FLASH_Read_Mode1_LG << 8) | FLASH_Read_Mode1, NAND_CMD_REG)*/
	mov r7, #0x3000
	str r7, [r6,#0x0]

	/* writel(0x00000000, NAND_CONFIGURATION1_REG) */
	mov r7, #0x0
	str r7, [r6, #0x34]

	/* start auto-read
    	 * writel(NAND_LAUNCH_AUTO_READ, NAND_LAUNCH_REG);
	 */
	mov r7, #0x80
	str r7, [r6, #0x40]

	do_wait_op_done

Copy_Good_Blk:
1:	ldmia r0!, {r7-r14}
	stmia r2!, {r7-r14}
	cmp r0, r1
	blo 1b
	cmp r2, r5
	bge Copy_Main_done
	ldr r7, NFC_PAGE_MODE
	add r4, r4, r7
	ldr r0, =NFC_BASE_ADDR_AXI
	b	Read_Page

Copy_Main_done:
	adr r0, SAVE_REGS
	ldr r12, [r0]
	ldr lr,	[r0, #4]
	mov pc, lr

U_BOOT_NAND_START: .word TEXT_BASE
CONST_0X0FFF:	.word 0x0FFF
NFC_PAGE_MODE:	.word 0x1000
SAVE_REGS:	.word 0x0
		.word 0x0
