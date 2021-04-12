/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2021 NXP
 */

#ifndef __ASM_ARCH_IMX8ULP_RDC_H
#define __ASM_ARCH_IMX8ULP_RDC_H

int release_xrdc(void);
void xrdc_mrc_region_set_access(int mrc_index, u32 addr, u32 access);

#endif
