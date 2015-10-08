// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2014-2016, Freescale Semiconductor, Inc.
 */

#include <common.h>
#include <cpu_func.h>
#include <asm/io.h>
#include <asm/system.h>
#include <asm/armv8/mmu.h>
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

u32 cpu_mask(void)
{
	return readl(MC_ME_CS);
}

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
