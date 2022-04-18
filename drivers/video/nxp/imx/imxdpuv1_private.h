/*
 * Copyright (c) 2005-2016 Freescale Semiconductor, Inc.
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

/* Instance: imxdpuv1_private.h */
#ifndef IMXDPUV1_PRIVATE_H
#define IMXDPUV1_PRIVATE_H

#include <asm/io.h>
#include <asm/string.h>

#include <linux/types.h>
#include "imxdpuv1.h"

typedef enum {
	IMXDPUV1_BURST_UNKNOWN = 0,
	IMXDPUV1_BURST_LEFT_RIGHT_DOWN,
	IMXDPUV1_BURST_HORIZONTAL,
	IMXDPUV1_BURST_VERTICAL,
	IMXDPUV1_BURST_FREE,
} imxdpuv1_burst_t;

#define INTSTAT0_BIT(__bit__) (1U<<(__bit__))
#define INTSTAT1_BIT(__bit__) (1U<<((__bit__)-32))
#define INTSTAT2_BIT(__bit__) (1U<<((__bit__)-64))

struct imxdpuv1_irq_node {
	int(*handler) (int, void *);
	const char *name;
	void *data;
	uint32_t  flags;
};

/* Generic definitions that are common to many registers */
#define IMXDPUV1_COLOR_BITSALPHA0_MASK 0xFU
#define IMXDPUV1_COLOR_BITSALPHA0_SHIFT 0U
#define IMXDPUV1_COLOR_BITSBLUE0_MASK 0xF00U
#define IMXDPUV1_COLOR_BITSBLUE0_SHIFT 8U
#define IMXDPUV1_COLOR_BITSGREEN0_MASK 0xF0000U
#define IMXDPUV1_COLOR_BITSGREEN0_SHIFT 16U
#define IMXDPUV1_COLOR_BITSRED0_MASK 0xF000000U
#define IMXDPUV1_COLOR_BITSRED0_SHIFT 24U

#define IMXDPUV1_COLOR_SHIFTALPHA0_MASK 0x1FU
#define IMXDPUV1_COLOR_SHIFTALPHA0_SHIFT 0U
#define IMXDPUV1_COLOR_SHIFTBLUE0_MASK 0x1F00U
#define IMXDPUV1_COLOR_SHIFTBLUE0_SHIFT 8U
#define IMXDPUV1_COLOR_SHIFTGREEN0_MASK 0x1F0000U
#define IMXDPUV1_COLOR_SHIFTGREEN0_SHIFT 16U
#define IMXDPUV1_COLOR_SHIFTRED0_MASK 0x1F000000U
#define IMXDPUV1_COLOR_SHIFTRED0_SHIFT 24U

#define IMXDPUV1_COLOR_CONSTALPHA_MASK 0xFFU
#define IMXDPUV1_COLOR_CONSTALPHA_SHIFT 0U
#define IMXDPUV1_COLOR_CONSTBLUE_MASK 0xFF00U
#define IMXDPUV1_COLOR_CONSTBLUE_SHIFT 8U
#define IMXDPUV1_COLOR_CONSTGREEN_MASK 0xFF0000U
#define IMXDPUV1_COLOR_CONSTGREEN_SHIFT 16U
#define IMXDPUV1_COLOR_CONSTRED_MASK  0xFF000000U
#define IMXDPUV1_COLOR_CONSTRED_SHIFT 24U

/* these are common for fetch but not store */
#define IMXDPUV1_BUFF_ATTR_STRIDE_MASK 0xFFFFU
#define IMXDPUV1_BUFF_ATTR_STRIDE_SHIFT 0U
#define IMXDPUV1_BUFF_ATTR_BITSPERPIXEL_MASK 0x3F0000U
#define IMXDPUV1_BUFF_ATTR_BITSPERPIXEL_SHIFT 16U

#define IMXDPUV1_BUFF_DIMEN_LINECOUNT_SHIFT 16U
#define IMXDPUV1_BUFF_DIMEN_LINEWIDTH_MASK 0x3FFFU
#define IMXDPUV1_BUFF_DIMEN_LINEWIDTH_SHIFT 0U
#define IMXDPUV1_BUFF_DIMEN_LINECOUNT_MASK 0x3FFF0000U

#define IMXDPUV1_LAYER_XOFFSET_MASK 0x7FFFU
#define IMXDPUV1_LAYER_XOFFSET_SHIFT 0U
#define IMXDPUV1_LAYER_XSBIT_MASK 0x4000U
#define IMXDPUV1_LAYER_XSBIT_SHIFT 0U

#define IMXDPUV1_LAYER_YOFFSET_MASK 0x7FFF0000U
#define IMXDPUV1_LAYER_YOFFSET_SHIFT 16U
#define IMXDPUV1_LAYER_YSBIT_MASK 0x4000U
#define IMXDPUV1_LAYER_YSBIT_SHIFT 16U

#define IMXDPUV1_CLIP_XOFFSET_MASK 0x7FFFU
#define IMXDPUV1_CLIP_XOFFSET_SHIFT 0U
#define IMXDPUV1_CLIP_YOFFSET_MASK 0x7FFF0000U
#define IMXDPUV1_CLIP_YOFFSET_SHIFT 16U

#define IMXDPUV1_CLIP_WIDTH_MASK 0x3FFFU
#define IMXDPUV1_CLIP_WIDTH_SHIFT 0U
#define IMXDPUV1_CLIP_HEIGHT_MASK 0x3FFF0000U
#define IMXDPUV1_CLIP_HEIGHT_SHIFT 16U

#define IMXDPUV1_FRAMEWIDTH_MASK 0x3FFFU
#define IMXDPUV1_FRAMEWIDTH_SHIFT 0U
#define IMXDPUV1_FRAMEHEIGHT_MASK 0x3FFF0000U
#define IMXDPUV1_FRAMEHEIGHT_SHIFT 16U
#define IMXDPUV1_EMPTYFRAME_MASK 0x80000000U
#define IMXDPUV1_EMPTYFRAME_SHIFT 31U

#define IMXDPUV1_PIXENGCFG_SRC_SEL__DISABLE 0U
#define IMXDPUV1_PIXENGCFG_SRC_SEL_MASK 0x3FU
#define IMXDPUV1_PIXENGCFG_SRC_SEL_SHIFT 0U

#define IMXDPUV1_PIXENGCFG_LAYERBLEND_PRIM_SEL_MASK 0x3FU
#define IMXDPUV1_PIXENGCFG_LAYERBLEND_PRIM_SEL_SHIFT 0U
#define IMXDPUV1_PIXENGCFG_LAYERBLEND_PRIM_SEL__DISABLE 0U

#define IMXDPUV1_PIXENGCFG_LAYERBLEND_SEC_SEL_MASK 0x3F00U
#define IMXDPUV1_PIXENGCFG_LAYERBLEND_SEC_SEL_SHIFT 8U
#define IMXDPUV1_PIXENGCFG_LAYERBLEND_SEC_SEL__DISABLE 0U

#define IMXDPUV1_PIXENGCFG_CLKEN_MASK 0x3000000U
#define IMXDPUV1_PIXENGCFG_CLKEN_SHIFT 24U
/* Field Value: _CLKEN__DISABLE, Clock for block is disabled  */
#define IMXDPUV1_PIXENGCFG_CLKEN__DISABLE 0U
/* Field Value: _CLKEN__AUTOMATIC, Clock is enabled if unit is used,
 * frequency is defined by the register setting for this pipeline (see
 * [endpoint_name]_Static register)  */
#define IMXDPUV1_PIXENGCFG_CLKEN__AUTOMATIC 0x1U
/* Field Value: _CLKEN__FULL, Clock for block is without gating  */
#define IMXDPUV1_PIXENGCFG_CLKEN__FULL 0x3U


/* Register: IMXDPUV1_LayerProperty0 Common Bits */
#define IMXDPUV1_LAYERPROPERTY_OFFSET          ((uint32_t)(0x40))
#define IMXDPUV1_LAYERPROPERTY_RESET_VALUE     0x80000100U
#define IMXDPUV1_LAYERPROPERTY_RESET_MASK      0xFFFFFFFFU
#define IMXDPUV1_LAYERPROPERTY_PALETTEENABLE_MASK 0x1U
#define IMXDPUV1_LAYERPROPERTY_PALETTEENABLE_SHIFT 0U
#define IMXDPUV1_LAYERPROPERTY_TILEMODE_MASK  0x30U
#define IMXDPUV1_LAYERPROPERTY_TILEMODE_SHIFT 4U
/* Field Value: TILEMODE0__TILE_FILL_ZERO, Use zero value  */
#define IMXDPUV1_LAYERPROPERTY_TILEMODE__TILE_FILL_ZERO 0U
/* Field Value: TILEMODE0__TILE_FILL_CONSTANT, Use constant color register
 * value  */
#define IMXDPUV1_LAYERPROPERTY_TILEMODE__TILE_FILL_CONSTANT 0x1U
/* Field Value: TILEMODE0__TILE_PAD, Use closest pixel from source buffer.
 * Must not be used for DECODE or YUV422 operations or when SourceBufferEnable
 * is 0.  */
#define IMXDPUV1_LAYERPROPERTY_TILEMODE__TILE_PAD 0x2U
/* Field Value: TILEMODE0__TILE_PAD_ZERO, Use closest pixel from source buffer
 * but zero for alpha component. Must not be used for DECODE or YUV422
 * operations or when SourceBufferEnable is 0.  */
#define IMXDPUV1_LAYERPROPERTY_TILEMODE__TILE_PAD_ZERO 0x3U
#define IMXDPUV1_LAYERPROPERTY_ALPHASRCENABLE_MASK 0x100U
#define IMXDPUV1_LAYERPROPERTY_ALPHASRCENABLE_SHIFT 8U
#define IMXDPUV1_LAYERPROPERTY_ALPHACONSTENABLE_MASK 0x200U
#define IMXDPUV1_LAYERPROPERTY_ALPHACONSTENABLE_SHIFT 9U
#define IMXDPUV1_LAYERPROPERTY_ALPHAMASKENABLE_MASK 0x400U
#define IMXDPUV1_LAYERPROPERTY_ALPHAMASKENABLE_SHIFT 10U
#define IMXDPUV1_LAYERPROPERTY_ALPHATRANSENABLE_MASK 0x800U
#define IMXDPUV1_LAYERPROPERTY_ALPHATRANSENABLE_SHIFT 11U
#define IMXDPUV1_LAYERPROPERTY_RGBALPHASRCENABLE_MASK 0x1000U
#define IMXDPUV1_LAYERPROPERTY_RGBALPHASRCENABLE_SHIFT 12U
#define IMXDPUV1_LAYERPROPERTY_RGBALPHACONSTENABLE_MASK 0x2000U
#define IMXDPUV1_LAYERPROPERTY_RGBALPHACONSTENABLE_SHIFT 13U
#define IMXDPUV1_LAYERPROPERTY_RGBALPHAMASKENABLE_MASK 0x4000U
#define IMXDPUV1_LAYERPROPERTY_RGBALPHAMASKENABLE_SHIFT 14U
#define IMXDPUV1_LAYERPROPERTY_RGBALPHATRANSENABLE_MASK 0x8000U
#define IMXDPUV1_LAYERPROPERTY_RGBALPHATRANSENABLE_SHIFT 15U
#define IMXDPUV1_LAYERPROPERTY_PREMULCONSTRGB_MASK 0x10000U
#define IMXDPUV1_LAYERPROPERTY_PREMULCONSTRGB_SHIFT 16U
#define IMXDPUV1_LAYERPROPERTY_YUVCONVERSIONMODE_MASK 0x60000U
#define IMXDPUV1_LAYERPROPERTY_YUVCONVERSIONMODE_SHIFT 17U
/* Field Value: YUVCONVERSIONMODE0__OFF, No conversion.  */
#define IMXDPUV1_LAYERPROPERTY_YUVCONVERSIONMODE__OFF 0U
/* Field Value: YUVCONVERSIONMODE0__ITU601, Conversion from YCbCr (YUV) to
 * RGB according to ITU recommendation BT.601-6 (standard definition TV).
 * Input range is 16..235 for Y and 16..240 for U/V.  */
#define IMXDPUV1_LAYERPROPERTY_YUVCONVERSIONMODE__ITU601 0x1U
/* Field Value: YUVCONVERSIONMODE0__ITU601_FR, Conversion from YCbCr (YUV)
 * to RGB according to ITU recommendation BT.601-6, but assuming full range
 * YUV inputs (0..255). Most typically used for computer graphics (e.g.
 * for JPEG encoding).  */
#define IMXDPUV1_LAYERPROPERTY_YUVCONVERSIONMODE__ITU601_FR 0x2U
/* Field Value: YUVCONVERSIONMODE0__ITU709, Conversion from YCbCr (YUV) to
 * RGB according to ITU recommendation BT.709-5 part 2 (high definition
 * TV). Input range is 16..235 for Y and 16..240 for U/V.  */
#define IMXDPUV1_LAYERPROPERTY_YUVCONVERSIONMODE__ITU709 0x3U
#define IMXDPUV1_LAYERPROPERTY_GAMMAREMOVEENABLE_MASK 0x100000U
#define IMXDPUV1_LAYERPROPERTY_GAMMAREMOVEENABLE_SHIFT 20U
#define IMXDPUV1_LAYERPROPERTY_CLIPWINDOWENABLE_MASK 0x40000000U
#define IMXDPUV1_LAYERPROPERTY_CLIPWINDOWENABLE_SHIFT 30U
#define IMXDPUV1_LAYERPROPERTY_SOURCEBUFFERENABLE_MASK 0x80000000U
#define IMXDPUV1_LAYERPROPERTY_SOURCEBUFFERENABLE_SHIFT 31U

typedef struct {
	/* Source buffer base address of layer 0. */
	uint32_t baseaddress0;
	/* Source buffer attributes for layer 0. */
	uint32_t sourcebufferattributes0;
	/* Source buffer dimension of layer 0. */
	uint32_t sourcebufferdimension0;
	/* Size of color components for RGB, YUV and index formats (layer 0). */
	uint32_t colorcomponentbits0;
	/* Bit position of color components for RGB, YUV and index
	   formats (layer 0). */
	uint32_t colorcomponentshift0;
	/* Position of layer 0 within the destination frame. */
	uint32_t layeroffset0;
	/* Clip window position for layer 0. */
	uint32_t clipwindowoffset0;
	/* Clip window size for layer 0. */
	uint32_t clipwindowdimensions0;
	/* Constant color for layer 0. */
	uint32_t constantcolor0;
	/* Common properties of layer 0. */
	uint32_t layerproperty0;
} fetch_layer_setup_t;

typedef struct {
	/* Destination buffer base address of layer 0. */
	uint32_t baseaddress0;
	/* Destination buffer attributes for layer 0. */
	uint32_t destbufferattributes0;
	/* Source buffer dimension of layer 0. */
	uint32_t destbufferdimension0;
	/* Frame offset of layer 0. */
	uint32_t frameoffset0;
	/* Size of color components for RGB, YUV and index formats (layer 0). */
	uint32_t colorcomponentbits0;
	/* Bit position of color components for RGB, YUV and index
	   formats (layer 0). */
	uint32_t colorcomponentshift0;
} store_layer_setup_t;

typedef enum {
	IMXDPUV1_SHDLD_IDX_DISP0   =  (0),
	IMXDPUV1_SHDLD_IDX_DISP1   =  (1),
	IMXDPUV1_SHDLD_IDX_CONST0  =  (2), /* IMXDPUV1_ID_CONSTFRAME0 */
	IMXDPUV1_SHDLD_IDX_CONST1  =  (3), /* IMXDPUV1_ID_CONSTFRAME1 */
	IMXDPUV1_SHDLD_IDX_CHAN_00 =  (4), /* IMXDPUV1_ID_FETCHDECODE2 */
	IMXDPUV1_SHDLD_IDX_CHAN_01 =  (5), /* IMXDPUV1_ID_FETCHDECODE0 */
	IMXDPUV1_SHDLD_IDX_CHAN_02 =  (6), /* IMXDPUV1_ID_FETCHLAYER0 */
	IMXDPUV1_SHDLD_IDX_CHAN_03 =  (7), /* IMXDPUV1_ID_FETCHLAYER0 */
	IMXDPUV1_SHDLD_IDX_CHAN_04 =  (8), /* IMXDPUV1_ID_FETCHLAYER0 */
	IMXDPUV1_SHDLD_IDX_CHAN_05 =  (9), /* IMXDPUV1_ID_FETCHLAYER0 */
	IMXDPUV1_SHDLD_IDX_CHAN_06 = (10), /* IMXDPUV1_ID_FETCHLAYER0 */
	IMXDPUV1_SHDLD_IDX_CHAN_07 = (11), /* IMXDPUV1_ID_FETCHLAYER0 */
	IMXDPUV1_SHDLD_IDX_CHAN_08 = (12), /* IMXDPUV1_ID_FETCHLAYER0 */
	IMXDPUV1_SHDLD_IDX_CHAN_09 = (13), /* IMXDPUV1_ID_FETCHLAYER0 */
	IMXDPUV1_SHDLD_IDX_CHAN_10 = (14), /* IMXDPUV1_ID_FETCHWARP2 */
	IMXDPUV1_SHDLD_IDX_CHAN_11 = (15), /* IMXDPUV1_ID_FETCHWARP2 */
	IMXDPUV1_SHDLD_IDX_CHAN_12 = (16), /* IMXDPUV1_ID_FETCHWARP2 */
	IMXDPUV1_SHDLD_IDX_CHAN_13 = (17), /* IMXDPUV1_ID_FETCHWARP2 */
	IMXDPUV1_SHDLD_IDX_CHAN_14 = (18), /* IMXDPUV1_ID_FETCHWARP2 */
	IMXDPUV1_SHDLD_IDX_CHAN_15 = (19), /* IMXDPUV1_ID_FETCHWARP2 */
	IMXDPUV1_SHDLD_IDX_CHAN_16 = (20), /* IMXDPUV1_ID_FETCHWARP2 */
	IMXDPUV1_SHDLD_IDX_CHAN_17 = (21), /* IMXDPUV1_ID_FETCHWARP2 */
	IMXDPUV1_SHDLD_IDX_CHAN_18 = (22), /* IMXDPUV1_ID_FETCHDECODE3 */
	IMXDPUV1_SHDLD_IDX_CHAN_19 = (23), /* IMXDPUV1_ID_FETCHDECODE1 */
	IMXDPUV1_SHDLD_IDX_CHAN_20 = (24), /* IMXDPUV1_ID_FETCHLAYER1*/
	IMXDPUV1_SHDLD_IDX_CHAN_21 = (25), /* IMXDPUV1_ID_FETCHLAYER1*/
	IMXDPUV1_SHDLD_IDX_CHAN_22 = (26), /* IMXDPUV1_ID_FETCHLAYER1*/
	IMXDPUV1_SHDLD_IDX_CHAN_23 = (27), /* IMXDPUV1_ID_FETCHLAYER1*/
	IMXDPUV1_SHDLD_IDX_CHAN_24 = (28), /* IMXDPUV1_ID_FETCHLAYER1*/
	IMXDPUV1_SHDLD_IDX_CHAN_25 = (29), /* IMXDPUV1_ID_FETCHLAYER1*/
	IMXDPUV1_SHDLD_IDX_CHAN_26 = (30), /* IMXDPUV1_ID_FETCHLAYER1*/
	IMXDPUV1_SHDLD_IDX_CHAN_27 = (31), /* IMXDPUV1_ID_FETCHLAYER1*/
	IMXDPUV1_SHDLD_IDX_CHAN_28 = (32), /* IMXDPUV1_ID_FETCHECO0*/
	IMXDPUV1_SHDLD_IDX_CHAN_29 = (33), /* IMXDPUV1_ID_FETCHECO1*/
	IMXDPUV1_SHDLD_IDX_CHAN_30 = (34), /* IMXDPUV1_ID_FETCHECO2*/
	IMXDPUV1_SHDLD_IDX_MAX     = (35),
} imxdpuv1_shadow_load_index_t;

typedef struct {
	bool prim_sync_state;
	bool sec_sync_state;
	uint32_t prim_sync_count;
	uint32_t sec_sync_count;
	uint32_t skew_error_count;
	uint32_t prim_fifo_empty_count;
	uint32_t sec_fifo_empty_count;
	uint32_t frame_count;
} frame_gen_stats_t;

/*!
 * Definition of IMXDPU channel structure
 */
typedef struct {
	int8_t disp_id;	/* Iris instance id of "owner" */

	imxdpuv1_chan_t chan;
	uint32_t src_pixel_fmt;
	int16_t src_top;
	int16_t src_left;
	uint16_t src_width;
	uint16_t src_height;
	int16_t clip_top;
	int16_t clip_left;
	uint16_t clip_width;
	uint16_t clip_height;
	uint16_t stride;
	uint32_t dest_pixel_fmt;
	int16_t dest_top;
	int16_t dest_left;
	uint16_t dest_width;
	uint16_t dest_height;
	uint16_t const_color;

	uint32_t h_scale_factor;	/* downscaling  out/in */
	uint32_t h_phase;
	uint32_t v_scale_factor;	/* downscaling  out/in */
	uint32_t v_phase[2][2];

	bool use_video_proc;
	bool interlaced;
	bool use_eco_fetch;
	bool use_global_alpha;
	bool use_local_alpha;

	/* note: dma_addr_t changes for 64-bit arch */
	dma_addr_t phyaddr_0;

	uint32_t u_offset;
	uint32_t v_offset;

	uint8_t blend_layer;
	uint8_t destination_stream;
	uint8_t source_id;

	imxdpuv1_rotate_mode_t rot_mode;

	/* todo add features sub-windows, upscaling, warping */
	fetch_layer_setup_t fetch_layer_prop;
	store_layer_setup_t store_layer_prop;

	bool in_use;

	/* todo: add channel features */
} chan_private_t;

typedef union {
	struct {
		uint8_t request;
		uint8_t processing;
		uint8_t complete;
		uint8_t trys;
	} state;
	uint32_t word;
} imxdpuv1_shadow_state_t;

/* PRIVATE DATA */
struct imxdpuv1_soc {
	int8_t devtype;
	int8_t online;
	uint32_t enabled_int[3];
	struct imxdpuv1_irq_node irq_list[IMXDPUV1_INTERRUPT_MAX];

	struct device *dev;
	struct imxdpuv1_videomode video_mode[IMXDPUV1_NUM_DI];
	struct imxdpuv1_videomode capture_mode[IMXDPUV1_NUM_CI];
	frame_gen_stats_t fgen_stats[IMXDPUV1_NUM_DI];
	uint32_t irq_count;


	/*
	 * Bypass reset to avoid display channel being
	 * stopped by probe since it may starts to work
	 * in bootloader.
	 */
	int8_t bypass_reset;

	/* todo: need to decide where the locking is implemented */

	/*clk*/

	/*irq*/

	/*reg*/
	void __iomem           *base;

	/*use count*/
	imxdpuv1_layer_t blend_layer[IMXDPUV1_LAYER_MAX];
	chan_private_t chan_data[IMXDPUV1_CHAN_IDX_MAX];

	uint8_t	shadow_load_pending[IMXDPUV1_NUM_DI][IMXDPUV1_SHDLD_IDX_MAX];
	imxdpuv1_shadow_state_t shadow_load_state[IMXDPUV1_NUM_DI][IMXDPUV1_SHDLD_IDX_MAX];
};



/* PRIVATE FUNCTIONS */
#ifdef ENABLE_IMXDPUV1_TRACE_REG
uint32_t _imxdpuv1_read(struct imxdpuv1_soc *dpu, u32 offset, char *file, int line);
#define imxdpuv1_read(_inst_, _offset_) _imxdpuv1_read(_inst_, _offset_, __FILE__, __LINE__)
#else
static inline uint32_t imxdpuv1_read(struct imxdpuv1_soc *dpu, uint32_t offset)
{
	return __raw_readl(dpu->base + offset);
}
#endif

#ifdef ENABLE_IMXDPUV1_TRACE_IRQ_READ
uint32_t _imxdpuv1_read_irq(struct imxdpuv1_soc *dpu, u32 offset, char *file, int line);
#define imxdpuv1_read_irq(_inst_, _offset_) _imxdpuv1_read_irq(_inst_, _offset_, __FILE__, __LINE__)
#else
static inline uint32_t imxdpuv1_read_irq(struct imxdpuv1_soc *dpu, uint32_t offset)
{
	return __raw_readl(dpu->base + offset);
}
#endif

#ifdef ENABLE_IMXDPUV1_TRACE_REG
void _imxdpuv1_write(struct imxdpuv1_soc *dpu, uint32_t value, uint32_t offset, char *file, int line);
#define imxdpuv1_write(_inst_, _value_, _offset_) _imxdpuv1_write(_inst_, _value_, _offset_, __FILE__, __LINE__)
#else
static inline void imxdpuv1_write(struct imxdpuv1_soc *dpu, uint32_t offset, uint32_t value)
{
	__raw_writel(value, dpu->base + offset);
}
#endif

#ifdef ENABLE_IMXDPUV1_TRACE_IRQ_WRITE
void _imxdpuv1_write_irq(struct imxdpuv1_soc *dpu, uint32_t value, uint32_t offset, char *file, int line);
#define imxdpuv1_write_irq(_inst_, _value_, _offset_) _imxdpuv1_write_irq(_inst_, _value_, _offset_, __FILE__, __LINE__)
#else
static inline void imxdpuv1_write_irq(struct imxdpuv1_soc *dpu, uint32_t offset, uint32_t value)
{
	__raw_writel(value, dpu->base + offset);
}
#endif

void _imxdpuv1_write_block(struct imxdpuv1_soc *imxdpu, uint32_t offset, void *values, uint32_t cnt, char *file, int line);
#define imxdpuv1_write_block(_inst_, _values_, _offset_, _cnt_) _imxdpuv1_write_block(_inst_, _values_, _offset_, _cnt_, __FILE__, __LINE__)

/* mapping of RGB, Tcon, or static values to output */
#define IMXDPUV1_TCON_MAPBIT__RGB(_x_)   ((_x_))
#define IMXDPUV1_TCON_MAPBIT__Tsig(_x_)  ((_x_) + 30)
#define IMXDPUV1_TCON_MAPBIT__HIGH 42U
#define IMXDPUV1_TCON_MAPBIT__LOW  43U

/* these match the bit definitions for the shadlow load
   request registers
 */
typedef enum {
	IMXDPUV1_SHLDREQID_FETCHDECODE9 = 0,
	IMXDPUV1_SHLDREQID_FETCHPERSP9,
	IMXDPUV1_SHLDREQID_FETCHECO9,
	IMXDPUV1_SHLDREQID_CONSTFRAME0,
	IMXDPUV1_SHLDREQID_CONSTFRAME4,
	IMXDPUV1_SHLDREQID_CONSTFRAME1,
	IMXDPUV1_SHLDREQID_CONSTFRAME5,
#ifdef IMXDPUV1_VERSION_0
	IMXDPUV1_SHLDREQID_EXTSRC4,
	IMXDPUV1_SHLDREQID_EXTSRC5,
	IMXDPUV1_SHLDREQID_FETCHDECODE2,
	IMXDPUV1_SHLDREQID_FETCHDECODE3,
#endif
	IMXDPUV1_SHLDREQID_FETCHWARP2,
	IMXDPUV1_SHLDREQID_FETCHECO2,
	IMXDPUV1_SHLDREQID_FETCHDECODE0,
	IMXDPUV1_SHLDREQID_FETCHECO0,
	IMXDPUV1_SHLDREQID_FETCHDECODE1,
	IMXDPUV1_SHLDREQID_FETCHECO1,
	IMXDPUV1_SHLDREQID_FETCHLAYER0,
#ifdef IMXDPUV1_VERSION_0
	IMXDPUV1_SHLDREQID_FETCHLAYER1,
	IMXDPUV1_SHLDREQID_EXTSRC0,
	IMXDPUV1_SHLDREQID_EXTSRC1
#endif
} imxdpuv1_shadow_load_req_t;

#define IMXDPUV1_PIXENGCFG_DIVIDER_RESET 0x80

#endif /* IMXDPUV1_PRIVATE_H */

