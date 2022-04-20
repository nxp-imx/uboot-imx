/*
 * Copyright (c) 2019, NXP. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <asm/io.h>
#include <common.h>
#include <asm/arch/imx8m_rdc.h>

void imx_rdc_init(const struct imx_rdc_cfg *rdc_cfg)
{
	const struct imx_rdc_cfg *rdc = rdc_cfg;

	while (rdc->type != RDC_INVALID) {
		switch (rdc->type) {
		case RDC_MDA:
			/* MDA config */
			writel(rdc->setting.rdc_mda, MDAn(rdc->index));
			break;
		case RDC_PDAP:
			/* peripheral access permission config */
			writel(rdc->setting.rdc_pdap, PDAPn(rdc->index));
			break;
		case RDC_MEM_REGION:
			/* memory region access permission config */
			writel(rdc->setting.rdc_mem_region[0], MRSAn(rdc->index));
			writel(rdc->setting.rdc_mem_region[1], MREAn(rdc->index));
			writel(rdc->setting.rdc_mem_region[2], MRCn(rdc->index));
			break;
		default:
			break;
		}

		rdc++;
	}
}
