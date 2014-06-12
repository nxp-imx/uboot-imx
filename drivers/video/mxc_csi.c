/*
 * Copyright (C) 2014 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

/*!
 * @file fsl_csi.c, this file is derived from mx27_csi.c
 *
 * @brief mx25 CMOS Sensor interface functions
 *
 * @ingroup CSI
 */
#include <common.h>
#include <malloc.h>

#include <asm/arch/imx-regs.h>
#include <asm/errno.h>
#include <asm/io.h>

#include <linux/string.h>
#include <linux/list.h>

#include "mxc_csi.h"

enum {
	STD_NTSC = 0,
	STD_PAL,
};

void __iomem *csi_regbase;

static void csihw_reset_frame_count(void)
{
	__raw_writel(__raw_readl(CSI_CSICR3) | BIT_FRMCNT_RST, CSI_CSICR3);
}

static void csihw_reset(void)
{
	csihw_reset_frame_count();
	__raw_writel(CSICR1_RESET_VAL, CSI_CSICR1);
	__raw_writel(CSICR2_RESET_VAL, CSI_CSICR2);
	__raw_writel(CSICR3_RESET_VAL, CSI_CSICR3);
}

/*!
 * csi_init_interface
 *    Init csi interface
 */
void csi_init_interface(void)
{
	unsigned int val = 0;
	unsigned int imag_para;

	val |= BIT_SOF_POL;
	val |= BIT_REDGE;
	val |= BIT_GCLK_MODE;
	val |= BIT_HSYNC_POL;
	val |= BIT_FCC;
	val |= 1 << SHIFT_MCLKDIV;
	val |= BIT_MCLKEN;
	__raw_writel(val, CSI_CSICR1);

	imag_para = (640 << 16) | 960;
	__raw_writel(imag_para, CSI_CSIIMAG_PARA);

	val = 0x1010;
	val |= BIT_DMA_REFLASH_RFF;
	__raw_writel(val, CSI_CSICR3);
}

void csi_format_swap16(bool enable)
{
	unsigned int val;

	val = __raw_readl(CSI_CSICR1);
	if (enable) {
		val |= BIT_PACK_DIR;
		val |= BIT_SWAP16_EN;
	} else {
		val &= ~BIT_PACK_DIR;
		val &= ~BIT_SWAP16_EN;
	}

	__raw_writel(val, CSI_CSICR1);
}

void csi_enable_int(int arg)
{
	unsigned long cr1 = __raw_readl(CSI_CSICR1);

	if (arg == 1) {
		/* still capture needs DMA intterrupt */
		cr1 |= BIT_FB1_DMA_DONE_INTEN;
		cr1 |= BIT_FB2_DMA_DONE_INTEN;
	}
	__raw_writel(cr1, CSI_CSICR1);
}

void csi_disable_int(void)
{
	unsigned long cr1 = __raw_readl(CSI_CSICR1);

	cr1 &= ~BIT_FB1_DMA_DONE_INTEN;
	cr1 &= ~BIT_FB2_DMA_DONE_INTEN;
	__raw_writel(cr1, CSI_CSICR1);
}

void csi_enable(int arg)
{
	unsigned long cr = __raw_readl(CSI_CSICR18);

	if (arg == 1)
		cr |= BIT_CSI_ENABLE;
	else
		cr &= ~BIT_CSI_ENABLE;
	__raw_writel(cr, CSI_CSICR18);
}

void csi_buf_stride_set(u32 stride)
{
	__raw_writel(stride, CSI_CSIFBUF_PARA);
}

void csi_deinterlace_enable(bool enable)
{
	unsigned long cr18 = __raw_readl(CSI_CSICR18);

	if (enable == true)
		cr18 |= BIT_DEINTERLACE_EN;
	else
		cr18 &= ~BIT_DEINTERLACE_EN;

	__raw_writel(cr18, CSI_CSICR18);
}

void csi_deinterlace_mode(int mode)
{
	unsigned long cr18 = __raw_readl(CSI_CSICR18);

	if (mode == STD_NTSC)
		cr18 |= BIT_NTSC_EN;
	else
		cr18 &= ~BIT_NTSC_EN;

	__raw_writel(cr18, CSI_CSICR18);
}

void csi_tvdec_enable(bool enable)
{
	unsigned long cr18 = __raw_readl(CSI_CSICR18);
	unsigned long cr1 = __raw_readl(CSI_CSICR1);

	if (enable == true) {
		cr18 |= (BIT_TVDECODER_IN_EN | BIT_BASEADDR_SWITCH_EN);
		cr1 |= BIT_CCIR_MODE | BIT_EXT_VSYNC;
		cr1 &= ~(BIT_SOF_POL | BIT_REDGE);
	} else {
		cr18 &= ~(BIT_TVDECODER_IN_EN | BIT_BASEADDR_SWITCH_EN);
		cr1 &= ~(BIT_CCIR_MODE | BIT_EXT_VSYNC);
		cr1 |= BIT_SOF_POL | BIT_REDGE;
	}

	__raw_writel(cr18, CSI_CSICR18);
	__raw_writel(cr1, CSI_CSICR1);
}

void csi_set_32bit_imagpara(int width, int height)
{
	int imag_para = 0;
	unsigned long cr3 = __raw_readl(CSI_CSICR3);

	imag_para = (width << 16) | height;
	__raw_writel(imag_para, CSI_CSIIMAG_PARA);


	/* reflash the embeded DMA controller */
	__raw_writel(cr3 | BIT_DMA_REFLASH_RFF, CSI_CSICR3);
}

void csi_set_16bit_imagpara(int width, int height)
{
	int imag_para = 0;
	unsigned long cr3 = __raw_readl(CSI_CSICR3);

	imag_para = (width << 16) | (height * 2);
	__raw_writel(imag_para, CSI_CSIIMAG_PARA);

	/* reflash the embeded DMA controller */
	__raw_writel(cr3 | BIT_DMA_REFLASH_RFF, CSI_CSICR3);
}

void csi_set_12bit_imagpara(int width, int height)
{
	int imag_para = 0;
	unsigned long cr3 = __raw_readl(CSI_CSICR3);

	imag_para = (width << 16) | (height * 3 / 2);
	__raw_writel(imag_para, CSI_CSIIMAG_PARA);

	/* reflash the embeded DMA controller */
	__raw_writel(cr3 | BIT_DMA_REFLASH_RFF, CSI_CSICR3);
}

void csi_dmareq_rff_enable(void)
{
	unsigned long cr3 = __raw_readl(CSI_CSICR3);

	cr3 |= BIT_DMA_REQ_EN_RFF;
	cr3 |= BIT_HRESP_ERR_EN;
	__raw_writel(cr3, CSI_CSICR3);
}

void csi_dmareq_rff_disable(void)
{
	unsigned long cr3 = __raw_readl(CSI_CSICR3);

	cr3 &= ~BIT_DMA_REQ_EN_RFF;
	cr3 &= ~BIT_HRESP_ERR_EN;
	__raw_writel(cr3, CSI_CSICR3);
}

void csi_disable(void)
{
	csi_dmareq_rff_disable();
	csi_disable_int();
	csi_buf_stride_set(0);
	csi_deinterlace_enable(false);
	csi_tvdec_enable(false);
	csi_enable(0);
}

void csi_config(struct csi_conf_param *csi_conf)
{
	csi_regbase = (u32 *)CSI1_BASE_ADDR;

	csihw_reset();

	csi_init_interface();
	csi_dmareq_rff_disable();

	switch (csi_conf->bpp) {
	case 32:
		csi_set_32bit_imagpara(csi_conf->width, csi_conf->height);
		break;
	case 16:
		csi_set_16bit_imagpara(csi_conf->width, csi_conf->height);
		break;
	default:
		printf(" %s case not supported, bpp=%d\n",
				__func__, csi_conf->bpp);
		return;
	}

	__raw_writel((u32)csi_conf->fb0addr, CSI_CSIDMASA_FB1);
	__raw_writel((u32)csi_conf->fb1addr, CSI_CSIDMASA_FB2);

	csi_buf_stride_set(0);
	if (csi_conf->btvmode) {
		/* Enable csi PAL/NTSC deinterlace mode */
		csi_buf_stride_set(csi_conf->width);
		csi_deinterlace_mode(csi_conf->std);
		csi_deinterlace_enable(true);
		csi_tvdec_enable(true);
	}

	/* start csi */
	csi_dmareq_rff_enable();
	csi_enable_int(1);
	csi_enable(1);
}

