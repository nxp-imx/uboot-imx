/*
 * Copyright (C) 2008 Freescale Semiconductor, Inc.
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
#include <asm/arch/mx31-regs.h>

.section ".text.load", "x"

.macro wait_op_done
1:	ldrh	r3, [r12, #NAND_FLASH_CONFIG2_REG_OFF]
	ands	r3, r3, #NAND_FLASH_CONFIG2_INT_DONE
	beq	1b
.endm

data_output:
	strh	r8, [r12, #RAM_BUFFER_ADDRESS_REG_OFF]
	mov	r3, #FDO_PAGE_SPARE_VAL
	strh	r3, [r12, #NAND_FLASH_CONFIG2_REG_OFF]
	wait_op_done
	bx	lr

send_addr:
	strh	r3, [r12, #NAND_FLASH_ADD_REG_OFF]
	mov	r3, #NAND_FLASH_CONFIG2_FADD_EN
	strh	r3, [r12, #NAND_FLASH_CONFIG2_REG_OFF]
	wait_op_done
	bx	lr

send_cmd:
	strh	r3, [r12, #NAND_FLASH_CMD_REG_OFF]
	mov	r3, #NAND_FLASH_CONFIG2_FCMD_EN
	strh	r3, [r12, #NAND_FLASH_CONFIG2_REG_OFF]
	wait_op_done
	bx	lr


nand_read_page:

	mov	r7, lr

	mov	r3, #0x0
	/* send command */
	bl	send_cmd
	/* 5 cycles address input */
	mov	r3, #0x0
	bl	send_addr
	mov	r3, #0x0
	bl	send_addr
	mov	r3, r0
	bl	send_addr
	mov	r3, #0x0
	bl	send_addr
	mov	r3, #0x0
	bl	send_addr
	/* confirm read */
	mov	r3, #0x30
	bl	send_cmd
	/* data output */
	mov	r8, #0x0
	mov	r4, #0x4
1:
	bl	data_output
	add	r8, r8, #0x01
	cmp	r8, r4
	bne	1b
	ldrh	r3, [r12, #ECC_STATUS_RESULT_REG_OFF]
	tst	r3, #0x0a
	bne	.
	mov	pc, r7

.global mxc_nand_load
mxc_nand_load:

	/* Copy image from flash to SDRAM first */
	mov	r0, #NFC_BASE_ADDR
	add	r12, r0, #0xE00		/* register */
	add	r2, r0, #0x800      /* 2K */
	ldr	r1, __TEXT_BASE

1:	ldmia	r0!, {r3-r10}
	stmia	r1!, {r3-r10}
	cmp	r0, r2
	blo	1b
	/* Jump to SDRAM */
	ldr	r1, =0x0FFF
	and	r0, pc, r1     /* offset of pc */
	ldr	r1, __TEXT_BASE
	add	r1, r1, #0x10
	add	pc, r0, r1
	nop
	nop
	nop
	nop

nand_copy_block:

	/* wait for boot complete */
4:
	ldrh	r3, [r12, #NAND_FLASH_CONFIG2_REG_OFF]
	tst	r3, #0x8000
	beq	4b

	/* unlock buffer and blocks */
	mov	r3, #0x02
	strh	r3, [r12, #NFC_CONFIGURATION_REG_OFF]
	mov	r3, #0x0
	strh	r3, [r12, #UNLOCK_START_BLK_ADD_REG_OFF]
	mov	r3, #0x800
	strh	r3, [r12, #UNLOCK_END_BLK_ADD_REG_OFF]
	mov	r3, #0x04
	strh	r3, [r12, #NF_WR_PROT_REG_OFF]
	mov	r3, #0x10
	strh	r3, [r12, #NAND_FLASH_CONFIG1_REG_OFF]

	/* read 1 block, 256K */
	mov	r0, #0x01	/* page offset */
	ldr	r11, __TEXT_BASE
	add	r11, r11, #0x800

	mov	r1, #NFC_BASE_ADDR
	add	r2, r1, #0x800
2:
	bl	nand_read_page	/* r0, r1, r2, r11 has been used */
	/* copy data from internal buffer */
3:	ldmia	r1!, {r3-r10}
	stmia	r11!, {r3-r10}
	cmp	r1, r2
	blo	3b

	add	r0, r0, #0x01
	cmp	r0, #0x80
	mov	r1, #NFC_BASE_ADDR
	bne	2b

	/* set pc to _set_env */
	ldr	r11, __TEXT_BASE
	ldr	r1, =0x7FF
	/* correct the lr */
	and	r13, r13, r1
	add	r13, r13, r11
	mov	pc, r13

__TEXT_BASE:
	.word	TEXT_BASE
