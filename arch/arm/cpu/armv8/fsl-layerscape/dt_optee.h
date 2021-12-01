/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2021 NXP
 */
#ifndef __DT_OPTEE_H__
#define __DT_OPTEE_H__

extern unsigned long rom_pointer[];
int ft_add_optee_overlay(void *fdt, struct bd_info *bd);
#endif
