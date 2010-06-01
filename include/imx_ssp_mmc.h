/*
 * IMX SSP MMC Defines
 *-------------------------------------------------------------------
 *
 * Copyright (C) 2007-2008, 2010 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 *-------------------------------------------------------------------
 *
 */

#ifndef __IMX_SSP_MMC_H__
#define	__IMX_SSP_MMC_H__

/* Common definition */
#define BM_CLKCTRL_SSP_CLKGATE 		0x80000000
#define BM_CLKCTRL_SSP_BUSY 		0x20000000
#define BM_CLKCTRL_SSP_DIV_FRAC_EN 	0x00000200
#define BM_CLKCTRL_SSP_DIV 		0x000001FF
#define BP_CLKCTRL_SSP_DIV 		0

struct imx_ssp_mmc_cfg {
	u32     ssp_mmc_base;

	/* CLKCTRL register offset */
	u32	clkctrl_ssp_offset;
	u32	clkctrl_clkseq_ssp_offset;
};

#ifdef CONFIG_IMX_SSP_MMC
int imx_ssp_mmc_initialize(bd_t *bis, struct imx_ssp_mmc_cfg *cfg);

extern u32 ssp_mmc_is_wp(struct mmc *mmc);
#endif /* CONFIG_IMX_SSP_MMC */

#endif  /* __IMX_SSP_MMC_H__ */
