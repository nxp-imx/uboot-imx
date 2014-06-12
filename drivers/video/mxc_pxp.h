/*
 * Copyright (C) 2014 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef MXC_PXP_H
#define MXC_PXP_H

#include <asm/imx-common/regs-common.h>

struct mxs_pxp_regs{
	mxs_reg_32(pxp_ctrl)           /* 0x00  */
	mxs_reg_32(pxp_stat)           /* 0x10  */
	mxs_reg_32(pxp_out_ctrl)       /* 0x20  */
	mxs_reg_32(pxp_out_buf)        /* 0x30  */
	mxs_reg_32(pxp_out_buf2)       /* 0x40  */
	mxs_reg_32(pxp_out_pitch)      /* 0x50  */
	mxs_reg_32(pxp_out_lrc)        /* 0x60  */
	mxs_reg_32(pxp_out_ps_ulc)     /* 0x70  */
	mxs_reg_32(pxp_out_ps_lrc)     /* 0x80  */
	mxs_reg_32(pxp_out_as_ulc)     /* 0x90  */
	mxs_reg_32(pxp_out_as_lrc)     /* 0xa0  */
	mxs_reg_32(pxp_ps_ctrl)        /* 0xb0  */
	mxs_reg_32(pxp_ps_buf)         /* 0xc0  */
	mxs_reg_32(pxp_ps_ubuf)        /* 0xd0  */
	mxs_reg_32(pxp_ps_vbuf)        /* 0xe0  */
	mxs_reg_32(pxp_ps_pitch)       /* 0xf0  */
	mxs_reg_32(pxp_ps_background)  /* 0x100 */
	mxs_reg_32(pxp_ps_scale)       /* 0x110 */
	mxs_reg_32(pxp_ps_offset)      /* 0x120 */
	mxs_reg_32(pxp_ps_clrkeylow)   /* 0x130 */
	mxs_reg_32(pxp_ps_clrkeyhigh)  /* 0x140 */
	mxs_reg_32(pxp_as_ctrl)        /* 0x150 */
	mxs_reg_32(pxp_as_buf)         /* 0x160 */
	mxs_reg_32(pxp_as_pitch)       /* 0x170 */
	mxs_reg_32(pxp_as_clrkeylow)   /* 0x180 */
	mxs_reg_32(pxp_as_clrkeyhigh)  /* 0x190 */
	mxs_reg_32(pxp_csc1_coef0)     /* 0x1a0 */
	mxs_reg_32(pxp_csc1_coef1)     /* 0x1b0 */
	mxs_reg_32(pxp_csc1_coef2)     /* 0x1c0 */
	mxs_reg_32(pxp_csc2_ctrl)      /* 0x1d0 */
	mxs_reg_32(pxp_csc2_coef0)     /* 0x1e0 */
	mxs_reg_32(pxp_csc2_coef1)     /* 0x1f0 */
	mxs_reg_32(pxp_csc2_coef2)     /* 0x200 */
	mxs_reg_32(pxp_csc2_coef3)     /* 0x210 */
	mxs_reg_32(pxp_csc2_coef4)     /* 0x220 */
	mxs_reg_32(pxp_csc2_coef5)     /* 0x230 */
	mxs_reg_32(pxp_lut_ctrl)       /* 0x240 */
	mxs_reg_32(pxp_lut_addr)       /* 0x250 */
	mxs_reg_32(pxp_lut_data)       /* 0x260 */
	mxs_reg_32(pxp_lut_extmem)     /* 0x270 */
	mxs_reg_32(pxp_cfa)            /* 0x280 */
	mxs_reg_32(pxp_hist_ctrl)      /* 0x290 */
	mxs_reg_32(pxp_hist2_param)    /* 0x2a0 */
	mxs_reg_32(pxp_hist4_param)    /* 0x2b0 */
	mxs_reg_32(pxp_hist8_param0)   /* 0x2c0 */
	mxs_reg_32(pxp_hist8_param1)   /* 0x2d0 */
	mxs_reg_32(pxp_hist16_param0)  /* 0x2e0 */
	mxs_reg_32(pxp_hist16_param1)  /* 0x2f0 */
	mxs_reg_32(pxp_hist16_param2)  /* 0x300 */
	mxs_reg_32(pxp_hist16_param3)  /* 0x310 */
	mxs_reg_32(pxp_power)          /* 0x320 */
	uint32_t	reserved1[4*13];
	mxs_reg_32(pxp_next)           /* 0x400 */
};

#define BM_PXP_CTRL_IRQ_ENABLE 0x00000002
#define BM_PXP_CTRL_ENABLE 0x00000001

#define BM_PXP_STAT_IRQ 0x00000001

#define BP_PXP_OUT_CTRL_FORMAT	    0
#define BM_PXP_OUT_CTRL_FORMAT 0x0000001F
#define BF_PXP_OUT_CTRL_FORMAT(v)  \
	(((v) << 0) & BM_PXP_OUT_CTRL_FORMAT)

#define HW_PXP_PS_SCALE	(0x00000110)

#define BM_PXP_PS_SCALE_RSVD2 0x80000000
#define BP_PXP_PS_SCALE_YSCALE	    16
#define BM_PXP_PS_SCALE_YSCALE 0x7FFF0000
#define BF_PXP_PS_SCALE_YSCALE(v)  \
	(((v) << 16) & BM_PXP_PS_SCALE_YSCALE)
#define BM_PXP_PS_SCALE_RSVD1 0x00008000
#define BP_PXP_PS_SCALE_XSCALE	    0
#define BM_PXP_PS_SCALE_XSCALE 0x00007FFF
#define BF_PXP_PS_SCALE_XSCALE(v)  \
	(((v) << 0) & BM_PXP_PS_SCALE_XSCALE)

#define BP_PXP_PS_CTRL_SWAP 5
#define BM_PXP_PS_CTRL_SWAP 0x000000E0
#define BF_PXP_PS_CTRL_SWAP(v)  \
	(((v) << 5) & BM_PXP_PS_CTRL_SWAP)
#define BP_PXP_PS_CTRL_FORMAT	   0
#define BM_PXP_PS_CTRL_FORMAT 0x0000001F
#define BF_PXP_PS_CTRL_FORMAT(v)  \
	(((v) << 0) & BM_PXP_PS_CTRL_FORMAT)
#define BM_PXP_CTRL_SFTRST 0x80000000
#define BM_PXP_CTRL_CLKGATE 0x40000000

struct pxp_layer_param {
	unsigned short width;
	unsigned short height;
	unsigned short stride; /* aka pitch */
	unsigned int pixel_fmt;
	void *paddr;
};

struct pxp_config_data {
	struct pxp_layer_param s0_param;
	struct pxp_layer_param out_param;
};

void pxp_config(struct pxp_config_data *pxp_conf);

#endif
