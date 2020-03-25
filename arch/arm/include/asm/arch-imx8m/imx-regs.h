/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2017-2019 NXP
 */

#ifndef __IMX8M_REGS_H_
#define __IMX8M_REGS_H_

#define ARCH_MXC

#ifdef CONFIG_IMX8MQ
#include <asm/arch/imx-regs-imx8mq.h>
#elif defined(CONFIG_IMX8MM) || defined(CONFIG_IMX8MN) || defined(CONFIG_IMX8MP)
#include <asm/arch/imx-regs-imx8mm.h>
#else
#error "Error no imx-regs.h"
#endif

#endif
