/*
 * Processor reset for Freescale MX23 SoC.
 *
 * (C) Copyright 2009-2010 Freescale Semiconductor, Inc.
 *
 * Copyright (C) 2007 Sergey Kubushyn <ksi@koi8.net>
 *
 * -----------------------------------------------------
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

.globl reset_cpu
reset_cpu:
	ldr	r0, POWER_CHARGE
	mov	r1, #0x0
	str	r1, [r0]
	ldr	r0, POWER_MINPWR
	str	r1, [r0]
	ldr	r0, CLKCTRL_RESET
	mov	r1, #0x1
	str	r1, [r0]
_loop_forever:
	b	_loop_forever

POWER_MINPWR:
	.word 0x80044020
POWER_CHARGE:
	.word 0x80044030
CLKCTRL_RESET:
	.word 0x80040120

