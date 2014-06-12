/*
 * Copyright (C) 2014 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>

#include <asm/arch/imx-regs.h>
#include <asm/arch/sys_proto.h>
#include <asm/errno.h>
#include <asm/io.h>

#include <linux/string.h>
#include <linux/list.h>
#include <gis.h>

#include "mxc_pxp.h"

#define BV_PXP_OUT_CTRL_FORMAT__RGB888	  0x4
#define BV_PXP_OUT_CTRL_FORMAT__RGB555	  0xC
#define BV_PXP_OUT_CTRL_FORMAT__RGB444	  0xD
#define BV_PXP_OUT_CTRL_FORMAT__RGB565	  0xE
#define BV_PXP_OUT_CTRL_FORMAT__YUV1P444  0x10
#define BV_PXP_OUT_CTRL_FORMAT__UYVY1P422 0x12
#define BV_PXP_OUT_CTRL_FORMAT__VYUY1P422 0x13

#define BV_PXP_PS_CTRL_FORMAT__RGB888	 0x4
#define BV_PXP_PS_CTRL_FORMAT__RGB565	 0xE
#define BV_PXP_PS_CTRL_FORMAT__YUV1P444  0x10
#define BV_PXP_PS_CTRL_FORMAT__UYVY1P422 0x12
#define BV_PXP_PS_CTRL_FORMAT__VYUY1P422 0x13

#define BP_PXP_PS_CTRL_SWAP 5
#define BM_PXP_PS_CTRL_SWAP 0x000000E0
#define BF_PXP_PS_CTRL_SWAP(v)  \
	(((v) << 5) & BM_PXP_PS_CTRL_SWAP)

#define PXP_DOWNSCALE_THRESHOLD     0x4000

static void pxp_set_ctrl(struct pxp_config_data *pxp_conf)
{
	u32 ctrl;
	u32 fmt_ctrl;
	int need_swap = 0;   /* to support YUYV and YVYU formats */
	struct mxs_pxp_regs *regs = (struct mxs_pxp_regs *)PXP_BASE_ADDR;

	/* Configure S0 input format */
	switch (pxp_conf->s0_param.pixel_fmt) {
	case FMT_YUV444:
		fmt_ctrl = BV_PXP_PS_CTRL_FORMAT__YUV1P444;
		break;
	case FMT_UYVY:
		fmt_ctrl = BV_PXP_PS_CTRL_FORMAT__UYVY1P422;
		break;
	case FMT_YUYV:
		fmt_ctrl = BV_PXP_PS_CTRL_FORMAT__UYVY1P422;
		need_swap = 1;
	default:
		fmt_ctrl = 0;
	}

	ctrl = BF_PXP_PS_CTRL_FORMAT(fmt_ctrl) | BF_PXP_PS_CTRL_SWAP(need_swap);
	writel(ctrl, &regs->pxp_ps_ctrl);

	/* Configure output format based on out_channel format */
	switch (pxp_conf->out_param.pixel_fmt) {
	case FMT_RGB565:
		fmt_ctrl = BV_PXP_OUT_CTRL_FORMAT__RGB565;
		break;
	case FMT_RGB888:
		fmt_ctrl = BV_PXP_OUT_CTRL_FORMAT__RGB888;
		break;
	default:
		fmt_ctrl = 0;
	}

	ctrl = BF_PXP_OUT_CTRL_FORMAT(fmt_ctrl);
	writel(ctrl, &regs->pxp_out_ctrl);
}

static int pxp_set_scaling(struct pxp_config_data *pxp_conf)
{
	int ret = 0;
	u32 xscale, yscale, s0scale;
	u32 decx, decy, xdec = 0, ydec = 0;
	struct pxp_layer_param *s0_params = &pxp_conf->s0_param;
	struct pxp_layer_param *out_params = &pxp_conf->out_param;
	struct mxs_pxp_regs *regs = (struct mxs_pxp_regs *)PXP_BASE_ADDR;

	decx = s0_params->width / out_params->width;
	decy = s0_params->height / out_params->height;
	if (decx > 1) {
		if (decx >= 2 && decx < 4) {
			decx = 2;
			xdec = 1;
		} else if (decx >= 4 && decx < 8) {
			decx = 4;
			xdec = 2;
		} else if (decx >= 8) {
			decx = 8;
			xdec = 3;
		}
		xscale = s0_params->width * 0x1000 /
			 (out_params->width * decx);
	} else {
		if ((s0_params->pixel_fmt == FMT_YUYV) ||
		    (s0_params->pixel_fmt == FMT_UYVY) ||
		    (s0_params->pixel_fmt == FMT_YUV444))
			xscale = (s0_params->width - 1) * 0x1000 /
				 (out_params->width - 1);
		else
			xscale = (s0_params->width - 2) * 0x1000 /
				 (out_params->width - 1);
	}
	if (decy > 1) {
		if (decy >= 2 && decy < 4) {
			decy = 2;
			ydec = 1;
		} else if (decy >= 4 && decy < 8) {
			decy = 4;
			ydec = 2;
		} else if (decy >= 8) {
			decy = 8;
			ydec = 3;
		}
		yscale = s0_params->height * 0x1000 /
			 (out_params->height * decy);
	} else
		yscale = (s0_params->height - 1) * 0x1000 /
			 (out_params->height - 1);

	writel((xdec << 10) | (ydec << 8), &regs->pxp_ps_ctrl);

	if (xscale > PXP_DOWNSCALE_THRESHOLD)
		xscale = PXP_DOWNSCALE_THRESHOLD;
	if (yscale > PXP_DOWNSCALE_THRESHOLD)
		yscale = PXP_DOWNSCALE_THRESHOLD;
	s0scale = BF_PXP_PS_SCALE_YSCALE(yscale) |
		BF_PXP_PS_SCALE_XSCALE(xscale);
	writel(s0scale, &regs->pxp_ps_scale);

	pxp_set_ctrl(pxp_conf);

	return ret;
}

void pxp_power_down(void)
{
	struct mxs_pxp_regs *regs = (struct mxs_pxp_regs *)PXP_BASE_ADDR;
	u32 val;

	val = BM_PXP_CTRL_SFTRST | BM_PXP_CTRL_CLKGATE;
	writel(val , &regs->pxp_ctrl);
}

void pxp_config(struct pxp_config_data *pxp_conf)
{
	struct mxs_pxp_regs *regs = (struct mxs_pxp_regs *)PXP_BASE_ADDR;

	/* reset */
	mxs_reset_block(&regs->pxp_ctrl_reg);

	/* output buffer */
	if (pxp_conf->out_param.pixel_fmt == FMT_RGB888)
		writel(BV_PXP_OUT_CTRL_FORMAT__RGB888, &regs->pxp_out_ctrl);
	else
		writel(BV_PXP_OUT_CTRL_FORMAT__RGB565, &regs->pxp_out_ctrl);

	writel((u32)pxp_conf->out_param.paddr, &regs->pxp_out_buf);

	writel(pxp_conf->out_param.stride, &regs->pxp_out_pitch);
	writel((pxp_conf->out_param.width - 1) << 16 |
			(pxp_conf->out_param.height - 1),
			&regs->pxp_out_lrc);

	/* scale needed  */
	writel(0, &regs->pxp_out_ps_ulc);
	writel((pxp_conf->out_param.width - 1) << 16 |
			(pxp_conf->out_param.height - 1),
			&regs->pxp_out_ps_lrc);
	pxp_set_scaling(pxp_conf);

	writel(0, &regs->pxp_out_as_ulc);
	writel(0, &regs->pxp_out_as_lrc);

	/* input buffer */
	if (pxp_conf->s0_param.pixel_fmt == FMT_YUV444)
		writel(BV_PXP_PS_CTRL_FORMAT__YUV1P444, &regs->pxp_ps_ctrl);
	else if (pxp_conf->s0_param.pixel_fmt == FMT_YUYV)
		writel(BV_PXP_PS_CTRL_FORMAT__UYVY1P422 | BF_PXP_PS_CTRL_SWAP(1),
				&regs->pxp_ps_ctrl);
	else if (pxp_conf->s0_param.pixel_fmt == FMT_UYVY)
		writel(BV_PXP_PS_CTRL_FORMAT__UYVY1P422, &regs->pxp_ps_ctrl);
	else
		printf("%s, unsupport fmt\n", __func__);

	writel((u32)pxp_conf->s0_param.paddr, &regs->pxp_ps_buf);
	writel(pxp_conf->s0_param.stride, &regs->pxp_ps_pitch);
	writel(0, &regs->pxp_ps_background);
	writel(0x84ab01f0, &regs->pxp_csc1_coef0);
	writel(0x01980204, &regs->pxp_csc1_coef1);
	writel(0x0730079c, &regs->pxp_csc1_coef2);

	/* pxp start  */
	writel(BM_PXP_CTRL_IRQ_ENABLE | BM_PXP_CTRL_ENABLE, &regs->pxp_ctrl);
}
