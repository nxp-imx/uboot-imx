// SPDX-License-Identifier: GPL-2.0+
/*
 * SPL for Conga-SMX95
 * i.MX95 LPDDR5 Bring-up Framework
 */

#include <common.h>
#include <init.h>
#include <spl.h>
#include <asm/global_data.h>
#include <asm/arch/ddr.h>
#include <asm/arch/sys_proto.h>

DECLARE_GLOBAL_DATA_PTR;

/* External LPDDR5 timing structure */
extern struct dram_timing_info conga_smx95_lpddr5_timing;

/*
 * Early board initialization in SPL
 */
void board_init_f(ulong dummy)
{
    /* Basic CPU init */
    arch_cpu_init();

    /* Early clock init */
    board_early_init_f();

    /* Initialize serial console */
    preloader_console_init();

    printf("\n");
    printf("========================================\n");
    printf("  Conga-SMX95 SPL Starting\n");
    printf("========================================\n");

    /*
     * DDR Initialization
     */
    printf("Initializing LPDDR5...\n");

    /* Call NXP DDR driver */
    ddr_init(&conga_smx95_lpddr5_timing);

    printf("LPDDR5 Initialization Complete.\n");

    /*
     * Continue to U-Boot proper
     */
    board_init_r(NULL, 0);
}
