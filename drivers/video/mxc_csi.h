/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2014 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 */

/*!
 * @file mxc_csi.h
 *
 * @brief mxc CMOS Sensor interface functions
 *
 * @ingroup CSI
 */

#ifndef MXC_CSI_H
#define MXC_CSI_H

/* reset values */
#define CSICR1_RESET_VAL	0x40000800
#define CSICR2_RESET_VAL	0x0
#define CSICR3_RESET_VAL	0x0

/* csi control reg 1 */
#define BIT_SWAP16_EN		(0x1 << 31)
#define BIT_EXT_VSYNC		(0x1 << 30)
#define BIT_EOF_INT_EN		(0x1 << 29)
#define BIT_PRP_IF_EN		(0x1 << 28)
#define BIT_CCIR_MODE		(0x1 << 27)
#define BIT_COF_INT_EN		(0x1 << 26)
#define BIT_SF_OR_INTEN		(0x1 << 25)
#define BIT_RF_OR_INTEN		(0x1 << 24)
#define BIT_SFF_DMA_DONE_INTEN  (0x1 << 22)
#define BIT_STATFF_INTEN	(0x1 << 21)
#define BIT_FB2_DMA_DONE_INTEN  (0x1 << 20)
#define BIT_FB1_DMA_DONE_INTEN  (0x1 << 19)
#define BIT_RXFF_INTEN		(0x1 << 18)
#define BIT_SOF_POL		(0x1 << 17)
#define BIT_SOF_INTEN		(0x1 << 16)
#define BIT_MCLKDIV		(0xF << 12)
#define BIT_HSYNC_POL		(0x1 << 11)
#define BIT_CCIR_EN		(0x1 << 10)
#define BIT_MCLKEN		(0x1 << 9)
#define BIT_FCC			(0x1 << 8)
#define BIT_PACK_DIR		(0x1 << 7)
#define BIT_CLR_STATFIFO	(0x1 << 6)
#define BIT_CLR_RXFIFO		(0x1 << 5)
#define BIT_GCLK_MODE		(0x1 << 4)
#define BIT_INV_DATA		(0x1 << 3)
#define BIT_INV_PCLK		(0x1 << 2)
#define BIT_REDGE		(0x1 << 1)
#define BIT_PIXEL_BIT		(0x1 << 0)

#define SHIFT_MCLKDIV		12

/* control reg 3 */
#define BIT_FRMCNT		(0xFFFF << 16)
#define BIT_FRMCNT_RST		(0x1 << 15)
#define BIT_DMA_REFLASH_RFF	(0x1 << 14)
#define BIT_DMA_REFLASH_SFF	(0x1 << 13)
#define BIT_DMA_REQ_EN_RFF	(0x1 << 12)
#define BIT_DMA_REQ_EN_SFF	(0x1 << 11)
#define BIT_STATFF_LEVEL	(0x7 << 8)
#define BIT_HRESP_ERR_EN	(0x1 << 7)
#define BIT_RXFF_LEVEL		(0x7 << 4)
#define BIT_TWO_8BIT_SENSOR	(0x1 << 3)
#define BIT_ZERO_PACK_EN	(0x1 << 2)
#define BIT_ECC_INT_EN		(0x1 << 1)
#define BIT_ECC_AUTO_EN		(0x1 << 0)

#define SHIFT_FRMCNT		16

/* csi status reg */
#define BIT_SFF_OR_INT		(0x1 << 25)
#define BIT_RFF_OR_INT		(0x1 << 24)
#define BIT_DMA_TSF_DONE_SFF	(0x1 << 22)
#define BIT_STATFF_INT		(0x1 << 21)
#define BIT_DMA_TSF_DONE_FB2	(0x1 << 20)
#define BIT_DMA_TSF_DONE_FB1	(0x1 << 19)
#define BIT_RXFF_INT		(0x1 << 18)
#define BIT_EOF_INT		(0x1 << 17)
#define BIT_SOF_INT		(0x1 << 16)
#define BIT_F2_INT		(0x1 << 15)
#define BIT_F1_INT		(0x1 << 14)
#define BIT_COF_INT		(0x1 << 13)
#define BIT_HRESP_ERR_INT	(0x1 << 7)
#define BIT_ECC_INT		(0x1 << 1)
#define BIT_DRDY		(0x1 << 0)

/* csi control reg 18 */
#define BIT_CSI_ENABLE			(0x1 << 31)
#define BIT_BASEADDR_SWITCH_SEL	(0x1 << 5)
#define BIT_BASEADDR_SWITCH_EN	(0x1 << 4)
#define BIT_PARALLEL24_EN		(0x1 << 3)
#define BIT_DEINTERLACE_EN		(0x1 << 2)
#define BIT_TVDECODER_IN_EN		(0x1 << 1)
#define BIT_NTSC_EN				(0x1 << 0)

#define CSI_MCLK_VF		1
#define CSI_MCLK_ENC		2
#define CSI_MCLK_RAW		4
#define CSI_MCLK_I2C		8

#define CSI_CSICR1				(csi_regbase)
#define CSI_CSICR2				(csi_regbase + 0x4)
#define CSI_CSICR3				(csi_regbase + 0x8)
#define CSI_STATFIFO			(csi_regbase + 0xC)
#define CSI_CSIRXFIFO			(csi_regbase + 0x10)
#define CSI_CSIRXCNT			(csi_regbase + 0x14)
#define CSI_CSISR				(csi_regbase + 0x18)
#define CSI_CSIDBG				(csi_regbase + 0x1C)
#define CSI_CSIDMASA_STATFIFO	(csi_regbase + 0x20)
#define CSI_CSIDMATS_STATFIFO	(csi_regbase + 0x24)
#define CSI_CSIDMASA_FB1		(csi_regbase + 0x28)
#define CSI_CSIDMASA_FB2		(csi_regbase + 0x2C)
#define CSI_CSIFBUF_PARA		(csi_regbase + 0x30)
#define CSI_CSIIMAG_PARA		(csi_regbase + 0x34)
#define CSI_CSICR18				(csi_regbase + 0x48)
#define CSI_CSICR19				(csi_regbase + 0x4c)

struct mxs_csi_regs {
	u32 csi_csicr1;				/* 0x0 */
	u32 csi_csicr2;				/* 0x4 */
	u32 csi_csicr3;				/* 0x8 */
	u32 csi_statfifo;			/* 0xC */
	u32 csi_csirxfifo;			/* 0x10 */
	u32 csi_csirxcnt;			/* 0x14 */
	u32 csi_csisr;				/* 0x18 */
	u32 csi_csidbg;				/* 0x1C */
	u32 csi_csidmasa_statfifo;	/* 0x20 */
	u32 csi_csidmats_statfifo;	/* 0x24 */
	u32 csi_csidmasa_fb1;		/* 0x28 */
	u32 csi_csidmasa_fb2;		/* 0x2C */
	u32 csi_csifbuf_para;		/* 0x30 */
	u32 csi_csiimag_para;		/* 0x34 */
	u32 reserver[4];
	u32 csi_csicr18;			/* 0x48 */
	u32 csi_csicr19;			/* 0x4c */
};

struct csi_conf_param {
	unsigned short width;
	unsigned short height;
	unsigned int pixel_fmt;
	unsigned int bpp;
	bool btvmode;
	unsigned int std;
	void *fb0addr;
	void *fb1addr;
};

void csi_config(struct csi_conf_param *csi_conf);
void csi_disable(void);
#endif
