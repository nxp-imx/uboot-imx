/*
 * Copyright (C) 2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _SYS_PROTO_H_
#define _SYS_PROTO_H_

#define BT0CFG_LPBOOT_MASK 0x1
#define BT0CFG_DUALBOOT_MASK 0x2

enum bt_mode {
	LOW_POWER_BOOT,		/* LP_BT = 1 */
	DUAL_BOOT,			/* LP_BT = 0, DUAL_BT = 1 */
	SINGLE_BOOT			/* LP_BT = 0, DUAL_BT = 0 */
};

u32 get_cpu_rev(void);

int mmc_get_env_dev(void);
void board_late_mmc_env_init(void);

#endif
