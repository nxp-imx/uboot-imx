/*
 * Copyright 2004-2011 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#ifndef __ARM_ARCH_MMU_H
#define __ARM_ARCH_MMU_H

#include <linux/types.h>

/*
 * Translation Table Base Bit Masks
 */
#define ARM_TRANSLATION_TABLE_MASK               0xFFFFC000

/*
 * Domain Access Control Bit Masks
 */
#define ARM_ACCESS_TYPE_NO_ACCESS(domain_num)    (0x0 << (domain_num)*2)
#define ARM_ACCESS_TYPE_CLIENT(domain_num)       (0x1 << (domain_num)*2)
#define ARM_ACCESS_TYPE_MANAGER(domain_num)      (0x3 << (domain_num)*2)

struct ARM_MMU_FIRST_LEVEL_FAULT {
	unsigned int id:2;
	unsigned int sbz:30;
};

#define ARM_MMU_FIRST_LEVEL_FAULT_ID 0x0

struct ARM_MMU_FIRST_LEVEL_PAGE_TABLE {
	unsigned int id:2;
	unsigned int imp:2;
	unsigned int domain:4;
	unsigned int sbz:1;
	unsigned int base_address:23;
};

#define ARM_MMU_FIRST_LEVEL_PAGE_TABLE_ID 0x1

struct ARM_MMU_FIRST_LEVEL_SECTION {
	unsigned int id:2;
	unsigned int b:1;
	unsigned int c:1;
	unsigned int imp:1;
	unsigned int domain:4;
	unsigned int sbz0:1;
	unsigned int ap:2;
	unsigned int sbz1:8;
	unsigned int base_address:12;
};

#define ARM_MMU_FIRST_LEVEL_SECTION_ID 0x2

struct ARM_MMU_FIRST_LEVEL_RESERVED {
	unsigned int id:2;
	unsigned int sbz:30;
};

#define ARM_MMU_FIRST_LEVEL_RESERVED_ID 0x3

#define ARM_MMU_FIRST_LEVEL_DESCRIPTOR_ADDRESS(ttb_base, table_index) \
	(unsigned long *)((unsigned long)(ttb_base) + ((table_index) << 2))

#define ARM_FIRST_LEVEL_PAGE_TABLE_SIZE 0x4000

#define ARM_MMU_SECTION(ttb_base, actual_base, virtual_base,		\
			cacheable, bufferable, perm)			\
	{								\
	register union ARM_MMU_FIRST_LEVEL_DESCRIPTOR desc;		\
	desc.word = 0;							\
	desc.section.id = ARM_MMU_FIRST_LEVEL_SECTION_ID;		\
	desc.section.domain = 0;					\
	desc.section.c = (cacheable);					\
	desc.section.b = (bufferable);					\
	desc.section.ap = (perm);					\
	desc.section.base_address = (actual_base);			\
	*ARM_MMU_FIRST_LEVEL_DESCRIPTOR_ADDRESS(ttb_base, (virtual_base)) \
				= desc.word;				\
	}

#define X_ARM_MMU_SECTION(abase, vbase, size, cache, buff, access)	\
	{								\
		int i; int j = abase; int k = vbase;			\
		for (i = size; i > 0 ; i--, j++, k++)			\
			ARM_MMU_SECTION(ttb_base, j, k, cache, buff, access); \
	}

union ARM_MMU_FIRST_LEVEL_DESCRIPTOR {
	unsigned long word;
	struct ARM_MMU_FIRST_LEVEL_FAULT fault;
	struct ARM_MMU_FIRST_LEVEL_PAGE_TABLE page_table;
	struct ARM_MMU_FIRST_LEVEL_SECTION section;
	struct ARM_MMU_FIRST_LEVEL_RESERVED reserved;
};

#define ARM_UNCACHEABLE		0
#define ARM_CACHEABLE		1
#define ARM_UNBUFFERABLE	0
#define ARM_BUFFERABLE		1

#define ARM_ACCESS_PERM_NONE_NONE	0
#define ARM_ACCESS_PERM_RO_NONE		0
#define ARM_ACCESS_PERM_RO_RO		0
#define ARM_ACCESS_PERM_RW_NONE		1
#define ARM_ACCESS_PERM_RW_RO		2
#define ARM_ACCESS_PERM_RW_RW		3

/*
 * Initialization for the Domain Access Control Register
 */
#define ARM_ACCESS_DACR_DEFAULT      (	\
	ARM_ACCESS_TYPE_MANAGER(0)    |	\
	ARM_ACCESS_TYPE_NO_ACCESS(1)  |	\
	ARM_ACCESS_TYPE_NO_ACCESS(2)  |	\
	ARM_ACCESS_TYPE_NO_ACCESS(3)  | \
	ARM_ACCESS_TYPE_NO_ACCESS(4)  | \
	ARM_ACCESS_TYPE_NO_ACCESS(5)  | \
	ARM_ACCESS_TYPE_NO_ACCESS(6)  | \
	ARM_ACCESS_TYPE_NO_ACCESS(7)  | \
	ARM_ACCESS_TYPE_NO_ACCESS(8)  | \
	ARM_ACCESS_TYPE_NO_ACCESS(9)  | \
	ARM_ACCESS_TYPE_NO_ACCESS(10) | \
	ARM_ACCESS_TYPE_NO_ACCESS(11) | \
	ARM_ACCESS_TYPE_NO_ACCESS(12) | \
	ARM_ACCESS_TYPE_NO_ACCESS(13) |	\
	ARM_ACCESS_TYPE_NO_ACCESS(14) |	\
	ARM_ACCESS_TYPE_NO_ACCESS(15))

/*
 * Translate the virtual address of ram space to physical address
 * It is dependent on the implementation of mmu_init
 */
inline unsigned long iomem_to_phys(unsigned long virt)
{
#if defined(CONFIG_MX53_SMD) || defined(CONFIG_MX53_LOCO) \
	|| defined(CONFIG_MX53_PCBA)
	if (virt >= 0x90000000 && virt <= 0xafffffff)
		return (unsigned long)((virt - 0x90000000) + PHYS_SDRAM_1);
	if (virt >= 0xD0000000 && virt <= 0xEfffffff)
		return (unsigned long)((virt - 0xD0000000) + PHYS_SDRAM_2);
#else
	if (virt >= 0xB0000000)
		return (unsigned long)((virt - 0xB0000000) + PHYS_SDRAM_1);
#endif

	return (unsigned long)virt;
}

/*
 * remap the physical address of ram space to uncacheable virtual address space
 * It is dependent on the implementation of hal_mmu_init
 */
void *__ioremap(unsigned long offset, size_t size, unsigned long flags)
{
	if (1 == flags) {
#if defined(CONFIG_MX53_SMD) || defined(CONFIG_MX53_LOCO) \
		|| defined(CONFIG_MX53_PCBA)
		if (offset >= PHYS_SDRAM_1 &&
		offset < (unsigned long)(PHYS_SDRAM_1 + PHYS_SDRAM_1_SIZE))
			return (void *)((offset - PHYS_SDRAM_1) + 0x90000000);
		else if (offset >= PHYS_SDRAM_1 &&
			offset < (unsigned long)(PHYS_SDRAM_2 + PHYS_SDRAM_2_SIZE))
			return (void *)((offset - PHYS_SDRAM_2) + 0xD0000000);
#else
		if (offset >= PHYS_SDRAM_1 &&
		offset < (unsigned long)(PHYS_SDRAM_1 + PHYS_SDRAM_1_SIZE))
			return (void *)((offset - PHYS_SDRAM_1) + 0xB0000000);
#endif
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

#endif
