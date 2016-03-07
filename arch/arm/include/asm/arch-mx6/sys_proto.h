/*
 * (C) Copyright 2009
 * Stefano Babic, DENX Software Engineering, sbabic@denx.de.
 *
 * Copyright (C) 2015-2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <asm/imx-common/sys_proto.h>
#include <asm/arch/module_fuse.h>

void set_wdog_reset(struct wdog_regs *wdog);
#ifdef CONFIG_LDO_BYPASS_CHECK
int check_ldo_bypass(void);
int check_1_2G(void);
int set_anatop_bypass(int wdog_reset_pin);
void ldo_mode_set(int ldo_bypass);
void prep_anatop_bypass(void);
void finish_anatop_bypass(void);
#endif
