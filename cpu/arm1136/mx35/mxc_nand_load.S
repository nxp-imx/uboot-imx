/*
 * (C) Copyright 2008 Freescale Semiconductor, Inc.
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
#include <asm/arch/mx35.h>

.macro nfc_cmd_input
	strh r3, [r12, #NAND_FLASH_CMD_REG_OFF]
	mov r3, #NAND_FLASH_CONFIG2_FCMD_EN;
	strh r3, [r12, #NAND_FLASH_CONFIG2_REG_OFF]
	bl do_wait_op_done
.endm   // nfc_cmd_input

.macro do_addr_input
	and r3, r3, #0xFF
	strh r3, [r12, #NAND_FLASH_ADD_REG_OFF]
	mov r3, #NAND_FLASH_CONFIG2_FADD_EN
	strh r3, [r12, #NAND_FLASH_CONFIG2_REG_OFF]
	bl do_wait_op_done
.endm   // do_addr_input

.section ".text.load", "x"
.globl mxc_nand_load
mxc_nand_load:
	ldr r2, U_BOOT_NAND_START
1:	ldmia r0!, {r3-r10}
	stmia r2!, {r3-r10}
	cmp r0, r1
	blo 1b

	ldr r1, CONST_0X0FFF
	ldr r2, U_BOOT_NAND_START
	and lr, lr, r1
	add lr, lr, r2
	and r12, r12, r1
	add r12, r12, r2
	add r2, r2, #0x8
	and r0, pc, r1
	add pc, r0, r2
	nop
	nop
	nop
	nop
	nop
	adr r0, SAVE_REGS
	str r12, [r0]
	str lr, [r0, #4]
Copy_Main:
	mov r0, #NFC_BASE_ADDR
	add r12, r0, #0x1E00
	ldrh r3, [r12, #NAND_FLASH_CONFIG1_REG_OFF]
	orr r3, r3, #1

	/* Setting NFC */
	ldr r7, =CCM_BASE_ADDR
	ldr r1, [r7, #CLKCTL_RCSR]
	/*BUS WIDTH setting*/
	tst r1, #0x20000000
	orrne r1, r1, #0x4000
	biceq r1, r1, #0x4000

	/*4K PAGE*/
	tst r1, #0x10000000
	orrne r1, r1, #0x200
	bne  1f
    	/*2K PAGE*/
	bic r1, r1, #0x200
	tst r1, #0x08000000
	orrne r1, r1, #0x100 /*2KB page size*/
	biceq r1, r1, #0x100 /*512B page size*/
	movne r2, #32 /*64 bytes*/
	moveq r2, #8  /*16 bytes*/
	b NAND_setup
1:
	tst r1, #0x08000000
	bicne r3, r3, #1   /*Enable 8bit ECC mode*/
	movne r2, #109 /*218 bytes*/
	moveq r2, #64  /*128 bytes*/
NAND_setup:
	str r1, [r7, #CLKCTL_RCSR]
	strh r2, [r12, #ECC_RSLT_SPARE_AREA_REG_OFF]
	strh r3, [r12, #NAND_FLASH_CONFIG1_REG_OFF]

    	//unlock internal buffer
	mov r3, #0x2
	strh r3, [r12, #NFC_CONFIGURATION_REG_OFF]
    	//unlock nand device
	mov r3, #0
	strh r3, [r12, #UNLOCK_START_BLK_ADD_REG_OFF]
	sub r3, r3, #1
	strh r3, [r12, #UNLOCK_END_BLK_ADD_REG_OFF]
	mov r3, #4
	strh r3, [r12, #NF_WR_PROT_REG_OFF]

	/* r0: NFC base address. RAM buffer base address. [constantly]
	 * r1: starting flash address to be copied. [constantly]
	 * r2: page size. [Doesn't change]
	 * r3: used as argument.
	 * r11: starting SDRAM address for copying. [Updated constantly].
	 * r12: NFC register base address. [constantly].
	 * r13: end of SDRAM address for copying. [Doesn't change].
	 */

	mov r1, #0x1000
	ldr r3, [r7, #CLKCTL_RCSR]
	tst r3, #0x200
	movne r2, #0x1000
	bne 1f
	tst r3, #0x100
	mov r1, #0x800  /*Strange Why is not 4K offset*/
	movne r2, #0x800
	moveq r2, #0x200
1: /*Update the indicator of copy area */
	ldr r11, U_BOOT_NAND_START
	add r13, r11, #0x00088000; /*512K + 32K*/
	add r11, r11, r1

Nfc_Read_Page:
	mov r3, #0x0
	nfc_cmd_input

	cmp r2, #0x800
	bhi nfc_addr_ops_4kb
	beq nfc_addr_ops_2kb

	mov r3, r1
	do_addr_input       //1st addr cycle
	mov r3, r1, lsr #9
	do_addr_input       //2nd addr cycle
	mov r3, r1, lsr #17
	do_addr_input       //3rd addr cycle
	mov r3, r1, lsr #25
	do_addr_input       //4th addr cycle
	b end_of_nfc_addr_ops

nfc_addr_ops_2kb:
	mov r3, #0
	do_addr_input       //1st addr cycle
	mov r3, #0
	do_addr_input       //2nd addr cycle
	mov r3, r1, lsr #11
	do_addr_input       //3rd addr cycle
	mov r3, r1, lsr #19
	do_addr_input       //4th addr cycle
	mov r3, r1, lsr #27
	do_addr_input       //5th addr cycle

	mov r3, #0x30
	nfc_cmd_input
	b end_of_nfc_addr_ops

nfc_addr_ops_4kb:
	mov r3, #0
	do_addr_input       //1st addr cycle
	mov r3, #0
	do_addr_input       //2nd addr cycle
	mov r3, r1, lsr #12
	do_addr_input       //3rd addr cycle
	mov r3, r1, lsr #20
	do_addr_input       //4th addr cycle
	mov r3, r1, lsr #27
	do_addr_input       //5th addr cycle

	mov r3, #0x30
	nfc_cmd_input

end_of_nfc_addr_ops:
	mov r8, #0
	bl nfc_data_output
	bl do_wait_op_done
	// Check if x16/2kb page
	cmp r2, #0x800
	bhi nfc_addr_data_output_done_4k
	beq nfc_addr_data_output_done_2k
	beq nfc_addr_data_output_done_512

	// check for bad block
	//    mov r3, r1, lsl #(32-17)    // get rid of block number
	//    cmp r3, #(0x800 << (32-17)) // check if not page 0 or 1
	b nfc_addr_data_output_done

nfc_addr_data_output_done_4k:
//TODO
	b nfc_addr_data_output_done

nfc_addr_data_output_done_2k:
	// end of 4th
	// check for bad block
	//TODO    mov r3, r1, lsl #(32-17)    // get rid of block number
	//    cmp r3, #(0x800 << (32-17)) // check if not page 0 or 1
	b nfc_addr_data_output_done

nfc_addr_data_output_done_512:
    	// check for bad block
	// TODO   mov r3, r1, lsl #(32-5-9)    // get rid of block number
	// TODO   cmp r3, #(512 << (32-5-9))   // check if not page 0 or 1

nfc_addr_data_output_done:
Copy_Good_Blk:
    //copying page
	add r2, r2, #NFC_BASE_ADDR
1:	ldmia r0!, {r3-r10}
	stmia r11!, {r3-r10}
	cmp r0, r2
	blo 1b
	sub r2, r2, #NFC_BASE_ADDR

	cmp r11, r13
	bge NAND_Copy_Main_done
	// Check if x16/2kb page
	add r1, r1, r2
	mov r0, #NFC_BASE_ADDR
	b Nfc_Read_Page

NAND_Copy_Main_done:
	adr r0, SAVE_REGS
	ldr r12, [r0]
	ldr lr, [r0, #4]
	mov pc, lr

do_wait_op_done:
1:
	ldrh r3, [r12, #NAND_FLASH_CONFIG2_REG_OFF]
	ands r3, r3, #NAND_FLASH_CONFIG2_INT_DONE
	beq 1b
	bx lr     // do

nfc_data_output:
	ldrh r3, [r12, #NAND_FLASH_CONFIG1_REG_OFF]
	orr r3, r3, #(NAND_FLASH_CONFIG1_INT_MSK | NAND_FLASH_CONFIG1_ECC_EN)
	strh r3, [r12, #NAND_FLASH_CONFIG1_REG_OFF]

	strh r8, [r12, #RAM_BUFFER_ADDRESS_REG_OFF]

	mov r3, #FDO_PAGE_SPARE_VAL
	strh r3, [r12, #NAND_FLASH_CONFIG2_REG_OFF]
	bx lr

U_BOOT_NAND_START: .word TEXT_BASE
CONST_0X0FFF:	.word 0x0FFF
SAVE_REGS:	.word 0x0
		.word 0x0
