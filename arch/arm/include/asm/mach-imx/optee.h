/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2020 NXP
 */
#ifndef __IMX_OPTEE_H__
#define __IMX_OPTEE_H__

#include <common.h>

#define OPTEE_SHM_SIZE 0x00400000
int ft_add_optee_node(void *fdt, bd_t *bd);
#endif
