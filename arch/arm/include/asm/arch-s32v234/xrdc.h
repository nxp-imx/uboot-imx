/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2015 Freescale Semiconductor, Inc.
 */

#ifndef __ASM_ARCH_XRDC_H
#define __ASM_ARCH_XRDC_H

#include "imx-regs.h"

#define XRDC_ADDR_MIN    (0x00000000)
#define XRDC_ADDR_MAX    (0xffffffff)
#define XRDC_VALID       (0x80000000)

#define XRDC_MRGD_W0_16   (XRDC_BASE_ADDR + 0x2200L)
#define XRDC_MRGD_W1_16   (XRDC_BASE_ADDR + 0x2204L)
#define XRDC_MRGD_W3_16   (XRDC_BASE_ADDR + 0x220CL)

#define XRDC_MRGD_W0_17   (XRDC_BASE_ADDR + 0x2220L)
#define XRDC_MRGD_W1_17   (XRDC_BASE_ADDR + 0x2224L)
#define XRDC_MRGD_W3_17   (XRDC_BASE_ADDR + 0x222CL)

#define XRDC_MRGD_W0_18   (XRDC_BASE_ADDR + 0x2240L)
#define XRDC_MRGD_W1_18   (XRDC_BASE_ADDR + 0x2244L)
#define XRDC_MRGD_W3_18   (XRDC_BASE_ADDR + 0x224CL)

#define XRDC_MRGD_W0_19   (XRDC_BASE_ADDR + 0x2260L)
#define XRDC_MRGD_W1_19   (XRDC_BASE_ADDR + 0x2264L)
#define XRDC_MRGD_W3_19   (XRDC_BASE_ADDR + 0x226CL)

#endif /* __ASM_ARCH_XRDC_H */
