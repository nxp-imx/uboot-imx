// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2014-2016, Freescale Semiconductor, Inc.
 */

#include <common.h>
#include <cpu_func.h>
#include <asm/io.h>
#include <asm/system.h>
#include <asm/armv8/mmu.h>
#include <asm/arch/cpu.h>
#include <asm/io.h>
#include <asm/arch/mc_me_regs.h>
#include <asm/system.h>
#include <asm/io.h>
#include "cpu.h"
#include "mp.h"

DECLARE_GLOBAL_DATA_PTR;

static struct mm_region s32_mem_map[] = {
	{
		/* List terminator */
		0,
	}
};

struct mm_region *mem_map = s32_mem_map;

#ifndef CONFIG_SYS_DCACHE_OFF

static void set_pgtable_section(u64 *page_table, u64 index, u64 section,
				u64 memory_type, u64 attribute)
{
	u64 value;

	value = section | PTE_TYPE_BLOCK | PTE_BLOCK_AF;
	value |= PMD_ATTRINDX(memory_type);
	value |= attribute;
	page_table[index] = value;
}

static void set_pgtable_table(u64 *page_table, u64 index, u64 *table_addr)
{
	u64 value;

	value = (u64)table_addr | PTE_TYPE_TABLE;
	page_table[index] = value;
}

u32 cpu_mask(void)
{
	return readl(MC_ME_CS);
}

/*
 * Set the block entries according to the information of the table.
 */
static int set_block_entry(const struct sys_mmu_table *list,
			   struct table_info *table)
{
	u64 block_size = 0, block_shift = 0;
	u64 block_addr, index;
	int j;

	if (table->entry_size == BLOCK_SIZE_L1) {
		block_size = BLOCK_SIZE_L1;
		block_shift = SECTION_SHIFT_L1;
	} else if (table->entry_size == BLOCK_SIZE_L2) {
		block_size = BLOCK_SIZE_L2;
		block_shift = SECTION_SHIFT_L2;
	} else {
		printf("Wrong size\n");
	}

	block_addr = list->phys_addr;
	index = (list->virt_addr - table->table_base) >> block_shift;

	for (j = 0; j < (list->size >> block_shift); j++) {
		set_pgtable_section(table->ptr,
				    index,
				    block_addr,
				    list->memory_type,
				    list->share);
		block_addr += block_size;
		index++;
	}

	return 0;
}

/*
 * Find the corresponding table entry for the list.
 */
static int find_table(const struct sys_mmu_table *list,
		      struct table_info *table, u64 *level0_table)
{
	u64 index = 0, level = 0;
	u64 *level_table = level0_table;
	u64 temp_base = 0, block_size = 0, block_shift = 0;

	while (level < 3) {
		if (level == 0) {
			block_size = BLOCK_SIZE_L0;
			block_shift = SECTION_SHIFT_L0;
		} else if (level == 1) {
			block_size = BLOCK_SIZE_L1;
			block_shift = SECTION_SHIFT_L1;
		} else if (level == 2) {
			block_size = BLOCK_SIZE_L2;
			block_shift = SECTION_SHIFT_L2;
		}

		index = 0;
		while (list->virt_addr >= temp_base) {
			index++;
			temp_base += block_size;
		}
		temp_base -= block_size;
		if ((level_table[index - 1] & PTE_TYPE_MASK) ==
				PTE_TYPE_TABLE) {
			level_table = (u64 *)(level_table[index - 1] &
					~PTE_TYPE_MASK);
			level++;
			continue;
		} else {
			if (level == 0)
				return -1;

			if ((list->phys_addr + list->size) >
					(temp_base + block_size * NUM_OF_ENTRY))
				return -1;

			/*
			 * Check the address and size of the list member is
			 * aligned with the block size.
			 */
			if (((list->phys_addr & (block_size - 1)) != 0) ||
			    ((list->size & (block_size - 1)) != 0))
				return -1;
			table->ptr = level_table;
			table->table_base = temp_base -
				((index - 1) << block_shift);
			table->entry_size = block_size;

			return 0;
		}
	}
	return -1;
}

/*
 * To start MMU before DDR is available, we create MMU table in SRAM.
 * The base address of SRAM is IRAM_BASE_ADDR. We use three
 * levels of translation tables here to cover 40-bit address space.
 * We use 4KB granule size, with 40 bits physical address, T0SZ=24
 * Level 0 IA[39], table address @0x6000
 * Level 1 IA[38:30], table address @0x7000, 0x8000
 * Level 2 IA[29:21], table address @0x9000, 0xA000, 0xB000
 */
static inline void early_mmu_setup(void)
{
	unsigned int el, i;
	u64 *level0_table = (u64 *)(IRAM_BASE_ADDR + 0x6000);
	u64 *level1_table0 = (u64 *)(IRAM_BASE_ADDR + 0x7000);
	u64 *level1_table1 = (u64 *)(IRAM_BASE_ADDR + 0x8000);
	u64 *level2_table0 = (u64 *)(IRAM_BASE_ADDR + 0x9000);
	u64 *level2_table1 = (u64 *)(IRAM_BASE_ADDR + 0xA000);
	u64 *level2_table2 = (u64 *)(IRAM_BASE_ADDR + 0xB000);
	struct table_info table = {level0_table, 0, BLOCK_SIZE_L0};

	/* Invalidate all table entries */
	memset(level0_table, 0, PGTABLE_SIZE);
	memset(level1_table0, 0, PGTABLE_SIZE);
	memset(level1_table1, 0, PGTABLE_SIZE);
	memset(level2_table0, 0, PGTABLE_SIZE);
	memset(level2_table1, 0, PGTABLE_SIZE);
	memset(level2_table2, 0, PGTABLE_SIZE);

	/* Fill in the table entries */
	set_pgtable_table(level0_table, 0, level1_table0);
	set_pgtable_table(level0_table, 1, level1_table1);

	set_pgtable_table(level1_table0,
			  CONFIG_SYS_FSL_IRAM_BASE >> SECTION_SHIFT_L1,
			  level2_table0);
	set_pgtable_table(level1_table0,
			  CONFIG_SYS_FSL_PERIPH_BASE >> SECTION_SHIFT_L1,
			  level2_table1);

	set_pgtable_table(level1_table0,
			  CONFIG_SYS_FSL_DRAM_BASE2 >> SECTION_SHIFT_L1,
			  level2_table2);

	/* Find the table and fill in the block entries */
	for (i = 0; i < ARRAY_SIZE(s32_early_mmu_table); i++) {
		if (find_table(&s32_early_mmu_table[i],
			       &table, level0_table) == 0) {
			/*
			 * If find_table() returns error, it cannot be dealt
			 * with here. Breakpoint can be added for debugging.
			 */
			set_block_entry(&s32_early_mmu_table[i], &table);
			/*
			 * If set_block_entry() returns error, it cannot be
			 * dealt with here, either.
			 */
		}
	}

	el = current_el();
	set_ttbr_tcr_mair(el, (u64)level0_table, S32V_TCR, MEMORY_ATTRIBUTES);
	set_sctlr(get_sctlr() | CR_M);
	set_sctlr(get_sctlr() | CR_C);
}

/*
 * The final tables look similar to early tables, but different in detail.
 * These tables are in DRAM using the area reserved by dtb for spintable area.
 *
 * Level 1 table 0 contains 512 entries for each 1GB from 0 to 512GB.
 * Level 1 table 1 contains 512 entries for each 1GB from 512GB to 1TB.
 * Level 2 table 0 contains 512 entries for each 2MB from 0 to 1GB.
 * Level 2 table 1 contains 512 entries for each 2MB from 1GB to 2GB.
 * Level 2 table 2 contains 512 entries for each 2MB from 3GB to 4GB.
 */
static inline void final_mmu_setup(void)
{
	unsigned int el, i;
	u64 *level0_table = (u64 *)(CPU_RELEASE_ADDR + 0x1000);
	u64 *level1_table0 = (u64 *)(CPU_RELEASE_ADDR + 0x2000);
	u64 *level1_table1 = (u64 *)(CPU_RELEASE_ADDR + 0x3000);
	u64 *level2_table0 = (u64 *)(CPU_RELEASE_ADDR + 0x4000);
	u64 *level2_table1 = (u64 *)(CPU_RELEASE_ADDR + 0x5000);
	u64 *level2_table2 = (u64 *)(CPU_RELEASE_ADDR + 0x6000);
	struct table_info table = {level0_table, 0, BLOCK_SIZE_L0};

	/* Invalidate all table entries */
	memset(level0_table, 0, PGTABLE_SIZE);
	memset(level1_table0, 0, PGTABLE_SIZE);
	memset(level1_table1, 0, PGTABLE_SIZE);
	memset(level2_table0, 0, PGTABLE_SIZE);
	memset(level2_table1, 0, PGTABLE_SIZE);
	memset(level2_table2, 0, PGTABLE_SIZE);

	/* Fill in the table entries */
	set_pgtable_table(level0_table, 0, level1_table0);
	set_pgtable_table(level0_table, 1, level1_table1);
	set_pgtable_table(level1_table0,
			  CONFIG_SYS_FSL_IRAM_BASE >> SECTION_SHIFT_L1,
			  level2_table0);
	set_pgtable_table(level1_table0,
			  CONFIG_SYS_FSL_PERIPH_BASE >> SECTION_SHIFT_L1,
			  level2_table1);
	set_pgtable_table(level1_table0,
			  CONFIG_SYS_FSL_DRAM_BASE2 >> SECTION_SHIFT_L1,
			  level2_table2);

	/* Find the table and fill in the block entries */
	for (i = 0; i < ARRAY_SIZE(s32_final_mmu_table); i++) {
		if (find_table(&s32_final_mmu_table[i],
			       &table, level0_table) == 0) {
			if (set_block_entry(&s32_final_mmu_table[i],
					    &table) != 0) {
				printf("MMU error: could not set block entry for %p\n",
				       &s32_final_mmu_table[i]);
			}
		} else {
			printf("MMU error: could not find the table for %p\n",
			       &s32_final_mmu_table[i]);
		}
	}

	/* flush new MMU table */
	flush_dcache_range(gd->arch.tlb_addr,
			   gd->arch.tlb_addr +  gd->arch.tlb_size);

	/* point TTBR to the new table */
	el = current_el();
	set_ttbr_tcr_mair(el, (u64)level0_table, S32V_TCR_FINAL,
			  MEMORY_ATTRIBUTES);
	/*
	 * MMU is already enabled, just need to invalidate TLB to load the
	 * new table. The new table is compatible with the current table, if
	 * MMU somehow walks through the new table before invalidation TLB,
	 * it still works. So we don't need to turn off MMU here.
	 */
}

int arch_cpu_init(void)
{
	icache_enable();
	__asm_invalidate_dcache_all();
	__asm_invalidate_tlb_all();
	early_mmu_setup();

	return 0;
}

/*
 * This function is called from lib/board.c.
 * It recreates MMU table in main memory. MMU and d-cache are enabled earlier.
 * There is no need to disable d-cache for this operation.
 */
void enable_caches(void)
{
	final_mmu_setup();
	__asm_invalidate_tlb_all();
}

#endif

/*
 * Return the number of cores on this SOC.
 */
int cpu_numcores(void)
{
	int numcores;
	u32 mask;

	mask = cpu_mask();
	numcores = hweight32(cpu_mask());

	/* Verify if M4 is deactivated */
	if (mask & 0x1)
		numcores--;

	return numcores;
}

#if defined(CONFIG_ARCH_EARLY_INIT_R)
int arch_early_init_r(void)
{
	int rv;
	asm volatile("dsb sy");
	rv = fsl_s32v234_wake_seconday_cores();

	if (rv)
		printf("Did not wake secondary cores\n");

	asm volatile("sev");
	return 0;
}
#endif /* CONFIG_ARCH_EARLY_INIT_R */
