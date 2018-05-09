/*
 * Copyright 2017-2018 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifdef CONFIG_IMX8MQ
#include <asm/arch/clock_imx8mq.h>
#elif defined(CONFIG_IMX8MM)
#include <asm/arch/clock_imx8mm.h>
#else
#error "Error no clock.h"
#endif
