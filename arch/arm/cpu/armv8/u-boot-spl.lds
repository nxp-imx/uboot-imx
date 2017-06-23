/*
 * (C) Copyright 2013
 * David Feng <fenghua@phytium.com.cn>
 *
 * (C) Copyright 2002
 * Gary Jennejohn, DENX Software Engineering, <garyj@denx.de>
 *
 * (C) Copyright 2010
 * Texas Instruments, <www.ti.com>
 *	Aneesh V <aneesh@ti.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

MEMORY { .sram : ORIGIN = CONFIG_SPL_TEXT_BASE,
		LENGTH = CONFIG_SPL_MAX_SIZE }
MEMORY { .sdram : ORIGIN = CONFIG_SPL_BSS_START_ADDR,
		LENGTH = CONFIG_SPL_BSS_MAX_SIZE }

OUTPUT_FORMAT("elf64-littleaarch64", "elf64-littleaarch64", "elf64-littleaarch64")
OUTPUT_ARCH(aarch64)
ENTRY(_start)
SECTIONS
{
	.text : {
		. = ALIGN(8);
		*(.__image_copy_start)
		CPUDIR/start.o (.text*)
		*(.text*)
	} >.sram

	.rodata : {
		. = ALIGN(8);
		*(SORT_BY_ALIGNMENT(SORT_BY_NAME(.rodata*)))
	} >.sram

	.data : {
		. = ALIGN(8);
		*(.data*)
	} >.sram

	.u_boot_list : {
		. = ALIGN(8);
		KEEP(*(SORT(.u_boot_list*)));
	} >.sram

	.image_copy_end : {
		. = ALIGN(8);
		*(.__image_copy_end)
	} >.sram

#ifdef CONFIG_ARCH_IMX8M
	.firmware_imem_start : {
		. = ALIGN(8);
                *(.__firmware_imem_start)
        }>.sram

	.firmware_imem : {
		. = ALIGN(8);
		KEEP(*(.firmware_imem))
        }>.sram

	.firmware_imem_end : {
		. = ALIGN(8);
                *(.__firmware_imem_end)
        }>.sram

	.firmware_dmem_start : {
		. = ALIGN(8);
                *(.__firmware_dmem_start)
        }>.sram

	.firmware_dmem : {
		. = ALIGN(8);
		KEEP(*(.firmware_dmem))
        }>.sram

	.firmware_dmem_end : {
		. = ALIGN(8);
                *(.__firmware_dmem_end)
        }>.sram
#endif

	.end : {
		. = ALIGN(8);
		*(.__end)
	} >.sram

	_image_binary_end = .;

	.bss_start : {
		. = ALIGN(8);
		KEEP(*(.__bss_start));
	} >.sdram

	.bss : {
		*(.bss*)
		 . = ALIGN(8);
	} >.sdram

	.bss_end : {
		KEEP(*(.__bss_end));
	} >.sdram

	/DISCARD/ : { *(.dynsym) }
	/DISCARD/ : { *(.dynstr*) }
	/DISCARD/ : { *(.dynamic*) }
	/DISCARD/ : { *(.plt*) }
	/DISCARD/ : { *(.interp*) }
	/DISCARD/ : { *(.gnu*) }
}
