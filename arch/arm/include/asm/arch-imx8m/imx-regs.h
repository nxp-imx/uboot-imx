/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifdef CONFIG_IMX8MQ
#include <asm/arch/imx-regs-imx8mq.h>
#elif defined(CONFIG_IMX8MM)
#include <asm/arch/imx-regs-imx8mm.h>
#else
#error "Error no imx-regs.h"
#endif
