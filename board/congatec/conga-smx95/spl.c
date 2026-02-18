// SPDX-License-Identifier: GPL-2.0+
/*
 * conga-smx95 SPL Initialization
 */

#include <common.h>
#include <init.h>

void spl_board_init(void)
{
    /* Early board-specific init for SPL */
}

void board_init_f(ulong dummy)
{
    /* This function is required for SPL */
}
