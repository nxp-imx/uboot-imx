/*
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __ASM_ARCH_IMX8_PINS_H__
#define __ASM_ARCH_IMX8_PINS_H__

#if defined(CONFIG_IMX8QM)
#include "imx8qm_pads.h"
#elif defined(CONFIG_IMX8QXP)
#include "imx8qxp_pads.h"
#else
#error "No pin header"
#endif

#endif	/* __ASM_ARCH_IMX8_PINS_H__ */
