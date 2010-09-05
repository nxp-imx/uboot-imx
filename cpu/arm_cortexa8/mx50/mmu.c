/*
 * Copyright (C) 2010 Freescale Semiconductor, Inc. All Rights Reserved.
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

#include <common.h>

/*
 * Translate the virtual address of ram space to physical address
 * It is dependent on the implementation of mmu_init
 */
inline void *iomem_to_phys(unsigned long virt)
{
	if (virt >= 0xB0000000)
		return (void *)((virt - 0xB0000000) + PHYS_SDRAM_1);

	return (void *)virt;
}

/*
 * remap the physical address of ram space to uncacheable virtual address space
 * It is dependent on the implementation of hal_mmu_init
 */
void *__ioremap(unsigned long offset, size_t size, unsigned long flags)
{
	if (1 == flags) {
		if (offset >= PHYS_SDRAM_1 &&
			offset < (PHYS_SDRAM_1 + PHYS_SDRAM_1_SIZE))
			return (void *)(offset - PHYS_SDRAM_1) + 0xB0000000;
		else
			return NULL;
	} else
		return (void *)offset;
}

/*
 * Remap the physical address of ram space to uncacheable virtual address space
 * It is dependent on the implementation of hal_mmu_init
 */
void __iounmap(void *addr)
{
	return;
}

