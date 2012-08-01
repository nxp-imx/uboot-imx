/*
 * January 2004 - Changed to support H4 device
 * Copyright (c) 2004 Texas Instruments
 *
 * (C) Copyright 2002
 * Gary Jennejohn, DENX Software Engineering, <gj@denx.de>
 *
 * (C) Copyright 2011-2012 Freescale Semiconductor, Inc.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)
SECTIONS
{
	. = 0x00000000;

	. = ALIGN(4);
	.text	   :
	{
	  /* WARNING - the following is hand-optimized to fit within	*/
	  /* the sector layout of our flash chips!	XXX FIXME XXX	*/
	  board/freescale/mx6q_arm2/flash_header.o	(.text.flasheader)
	  cpu/arm_cortexa8/start.o
	  board/freescale/mx6q_arm2/libmx6q_arm2.a	(.text)
	  lib_arm/libarm.a		(.text)
	  net/libnet.a			(.text)
	  drivers/mtd/libmtd.a		(.text)
	  drivers/mmc/libmmc.a		(.text)

	  . = DEFINED(env_offset) ? env_offset : .;
	  common/env_embedded.o(.text)

	  *(.text)
	}

	. = ALIGN(4);
	.rodata : { *(SORT_BY_ALIGNMENT(SORT_BY_NAME(.rodata*))) }

	. = ALIGN(4);
	.data : { *(.data) }

	. = ALIGN(4);
	.got : { *(.got) }

	. = .;
	__u_boot_cmd_start = .;
	.u_boot_cmd : { *(.u_boot_cmd) }
	__u_boot_cmd_end = .;

	. = ALIGN(4);
	_end_of_copy = .; /* end_of ROM copy code when HAB is not enabled */

	/* Extend to align to 0x1000, then put the Hab Data */
	. = ALIGN(0x1000);
	__hab_data = .;
	. = . + 0x2000;
	__hab_data_end = .;
	/* End of Hab Data, Place it before BSS section */

	__bss_start = .;
	.bss : { *(.bss) }
	_end = .;
}
