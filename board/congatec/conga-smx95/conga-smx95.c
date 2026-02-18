// SPDX-License-Identifier: GPL-2.0+
/*
 * conga-smx95 U-Boot Board File
 *
 * Copyright (C) 2026 Daksh Patel
 */

#include <common.h>
#include <init.h>
#include <env.h>

int board_init(void)
{
    /* Board specific initialization */
    return 0;
}

int board_late_init(void)
{
    /* Set board specific environment variables if needed */
    env_set("board_name", "conga-smx95");
    return 0;
}

int checkboard(void)
{
    puts("Board: congatec conga-SMX95 (i.MX95)\n");
    return 0;
}
