/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2021 NXP
 */

#ifndef __ASM_ARCH_IMX8ULP_RDC_H
#define __ASM_ARCH_IMX8ULP_RDC_H

enum rdc_type {
	RDC_TRDC,
	RDC_XRDC,
};

int release_rdc(enum rdc_type type);
void xrdc_mrc_region_set_access(int mrc_index, u32 addr, u32 access);
int trdc_mbc_set_access(u32 mbc_x, u32 dom_x, u32 mem_x, u32 blk_x, bool sec_access);

#endif
