/*
 * Copyright (c) 2015-2016 Freescale Semiconductor, Inc.
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef IMXDPUV1_H
#define IMXDPUV1_H

#include <linux/types.h>
#include <errno.h>

/* these will be removed */
#undef IMXDPUV1_VERSION_0
#define IMXDPUV1_VERSION_1

/* #define DEBUG */
/* #define ENABLE_IMXDPUV1_TRACE */
/* #define ENABLE_IMXDPUV1_TRACE_REG */
/* #define ENABLE_IMXDPUV1_TRACE_IRQ */
/* #define ENABLE_IMXDPUV1_TRACE_IRQ_READ */
/* #define ENABLE_IMXDPUV1_TRACE_IRQ_WRITE */

#ifdef ENABLE_IMXDPUV1_TRACE
#define IMXDPUV1_TRACE(fmt, ...) \
printf((fmt), ##__VA_ARGS__)
#else
#define IMXDPUV1_TRACE(fmt, ...) do {} while (0)
#endif

#ifdef ENABLE_IMXDPUV1_TRACE_IRQ
#define IMXDPUV1_TRACE_IRQ(fmt, ...) \
printf((fmt), ##__VA_ARGS__)
#else
#define IMXDPUV1_TRACE_IRQ(fmt, ...) do {} while (0)
#endif

#ifdef ENABLE_IMXDPUV1_TRACE_REG
#define IMXDPUV1_TRACE_REG(fmt, ...) \
printf((fmt), ##__VA_ARGS__)
#else
#define IMXDPUV1_TRACE_REG(fmt, ...) do {} while (0)
#endif

#define IMXDPUV1_PRINT(fmt, ...) \
printf((fmt), ##__VA_ARGS__)

/* #define IMXDPUV1_TCON0_MAP_24BIT_0_23 */
/* #define IMXDPUV1_TCON1_MAP_24BIT_0_23 */

/* todo: this need to come from device tree */
#define IMXDPUV1_NUM_DI_MAX 2
#define IMXDPUV1_MAX_NUM		2
#define IMXDPUV1_NUM_DI           2
#define IMXDPUV1_NUM_CI           2
#define IMXDPUV1_REGS_BASE_PHY0	0x56180000
#define IMXDPUV1_REGS_BASE_PHY1	0x57180000
#define IMXDPUV1_REGS_BASE_SIZE	0x14000

#ifdef IMXDPUV1_VERSION_0
#define IMXDPUV1_ENABLE_INTSTAT2
#endif
#define IMXDPUV1_SET_FIELD(field, value) (((value) << (field ## _SHIFT)) & (field ## _MASK))
#define IMXDPUV1_GET_FIELD(field, reg) (((reg)&(field ## _MASK)) >> (field  ## _SHIFT))

/*
	IMXDPU windows, planes, layers, streams

	IMXDPU hardware documentation confuses the meaning of layers and
		planes. These are software usages of these terms.

	window - a logical buffer of pixels in a rectangular arrangment.
		Image, Integral and video planes suport one window.
		Fractional and warp plane support 8 windows. Blending is not
		supported between the sub-windows of a fractional or warp plane.

	sub-window - one of the eight logical windows of a fractional or warp
		plane.

	channel - the logical DMA configuration for etiher a fetch or store unit

	plane - a plane is a hardware supported feature. There are four types
		of display planes:

		video x2
		fractional x2
		intergral x2
		warp

	layer - each of the 7 planes is fed to a layer blender. Full Alpha
		blending is supported for all of the planes fed to the layer
		blender.

	streams - the layer bleder produces four streams: two normal streams
		(0 and 1) and two panic streams (4 and 5).

		In normal mode, streams 0 and 1 are fed to the displays.
		In panic mode, streams 4 and 5 are fed to the displays.
*/


/*!
 * Enumeration of IMXDPU blend mode flags
 */
typedef enum {
	IMXDPUV1_PLANE_CLUT               = 1 << 0,	/* Color lookup */
	IMXDPUV1_PLANE_DECODE             = 1 << 1,	/* Decode compressed bufers */
	IMXDPUV1_PLANE_ETERNAL_ALPHA      = 1 << 2,	/* supports external alpha buffer  */
	IMXDPUV1_PLANE_VIDEO_PROC         = 1 << 2,	/* Gamma, Matrix, Scaler, histogram  */
	IMXDPUV1_PLANE_PLANAR             = 1 << 3,	/* Support Planar pixel buffers*/
	IMXDPUV1_PLANE_WARP               = 1 << 4,	/* Warping */
	IMXDPUV1_PLANE_MULTIWINDOW        = 1 << 5,	/* Support multiple buffers per plane */
	IMXDPUV1_PLANE_CAPTURE            = 1 << 6,	/* Video capture */
} imxdpuv1_plane_features_t;

/*!
 * Enumeration of IMXDPU layer blend mode flags
 */
typedef enum {
	IMXDPUV1_LAYER_NONE               = 1 << 0,	/* Disable blending */
	IMXDPUV1_LAYER_TRANSPARENCY       = 1 << 1,	/* Transparency */
	IMXDPUV1_LAYER_GLOBAL_ALPHA       = 1 << 2,	/* Global alpha mode */
	IMXDPUV1_LAYER_LOCAL_ALPHA        = 1 << 3,	/* Alpha contained in source buffer */
	IMXDPUV1_LAYER_EXTERN_ALPHA       = 1 << 4,	/* Alpha is contained in a separate plane */
	IMXDPUV1_LAYER_PRE_MULITPLY       = 1 << 5,	/* Pre-multiply alpha mode */
} imxdpuv1_layer_blend_modes_t;

/*!
 * Enumeration of IMXDPU layers
 */
typedef enum {
	IMXDPUV1_LAYER_0 = 0,
	IMXDPUV1_LAYER_1,
	IMXDPUV1_LAYER_2,
	IMXDPUV1_LAYER_3,
	IMXDPUV1_LAYER_4,
#ifdef IMXDPUV1_VERSION_0
	IMXDPUV1_LAYER_5,
	IMXDPUV1_LAYER_6,
#endif
	IMXDPUV1_LAYER_MAX,
} imxdpuv1_layer_idx_t;

/*!
 * Enumeration of IMXDPU sub-windows
 */
typedef enum {
	IMXDPUV1_SUBWINDOW_NONE = 0,
	IMXDPUV1_SUBWINDOW_1,
	IMXDPUV1_SUBWINDOW_2,
	IMXDPUV1_SUBWINDOW_3,
	IMXDPUV1_SUBWINDOW_4,
	IMXDPUV1_SUBWINDOW_5,
	IMXDPUV1_SUBWINDOW_6,
	IMXDPUV1_SUBWINDOW_7,
	IMXDPUV1_SUBWINDOW_8,
} imxdpuv1_subwindow_id_t;

/*!
 * Enumeration of IMXDPU display streams
 */
typedef enum {
	IMXDPUV1_DISPLAY_STREAM_NONE = (0),
	IMXDPUV1_DISPLAY_STREAM_0 = (1U<<0),
	IMXDPUV1_DISPLAY_STREAM_1 = (1U<<1),
	IMXDPUV1_DISPLAY_STREAM_4 = (1U<<4),
	IMXDPUV1_DISPLAY_STREAM_5 = (1U<<5),
} imxdpuv1_display_stream_t;

/*!
 * Enumeration of IMXDPU rotation modes
 */
typedef enum {
	/* todo: these need to aligh to imxdpu scan direction */
	IMXDPUV1_ROTATE_NONE = 0,
	IMXDPUV1_ROTATE_VERT_FLIP = 1,
	IMXDPUV1_ROTATE_HORIZ_FLIP = 2,
	IMXDPUV1_ROTATE_180 = 3,
	IMXDPUV1_ROTATE_90_RIGHT = 4,
	IMXDPUV1_ROTATE_90_RIGHT_VFLIP = 5,
	IMXDPUV1_ROTATE_90_RIGHT_HFLIP = 6,
	IMXDPUV1_ROTATE_90_LEFT = 7,
} imxdpuv1_rotate_mode_t;


/*!
 * Enumeration of types of buffers for a logical channel.
 */
typedef enum {
	IMXDPUV1_OUTPUT_BUFFER = 0,	/*!< Buffer for output from IMXDPU BLIT or capture */
	IMXDPUV1_ALPHA_IN_BUFFER = 1,	/*!< Buffer for alpha input to IMXDPU */
	IMXDPUV1_GRAPH_IN_BUFFER = 2,	/*!< Buffer for graphics input to IMXDPU */
	IMXDPUV1_VIDEO_IN_BUFFER = 3,	/*!< Buffer for video input to IMXDPU */
} imxdpuv1_buffer_t;

#ifdef IMXDPUV1_VERSION_0
/*!
 * Enumeration of IMXDPU logical block ids
 * NOTE: these match the hardware layout and are not arbitrary
 */
typedef enum {
	IMXDPUV1_ID_NONE = 0,
	IMXDPUV1_ID_FETCHDECODE9,
	IMXDPUV1_ID_FETCHPERSP9,
	IMXDPUV1_ID_FETCHECO9,
	IMXDPUV1_ID_ROP9,
	IMXDPUV1_ID_CLUT9,
	IMXDPUV1_ID_MATRIX9,
	IMXDPUV1_ID_HSCALER9,
	IMXDPUV1_ID_VSCALER9,
	IMXDPUV1_ID_FILTER9,
	IMXDPUV1_ID_BLITBLEND9,
	IMXDPUV1_ID_STORE9,
	IMXDPUV1_ID_CONSTFRAME0,
	IMXDPUV1_ID_EXTDST0,
	IMXDPUV1_ID_CONSTFRAME4,
	IMXDPUV1_ID_EXTDST4,
	IMXDPUV1_ID_CONSTFRAME1,
	IMXDPUV1_ID_EXTDST1,
	IMXDPUV1_ID_CONSTFRAME5,
	IMXDPUV1_ID_EXTDST5,
	IMXDPUV1_ID_EXTSRC4,
	IMXDPUV1_ID_STORE4,
	IMXDPUV1_ID_EXTSRC5,
	IMXDPUV1_ID_STORE5,
	IMXDPUV1_ID_FETCHDECODE2,
	IMXDPUV1_ID_FETCHDECODE3,
	IMXDPUV1_ID_FETCHWARP2,
	IMXDPUV1_ID_FETCHECO2,
	IMXDPUV1_ID_FETCHDECODE0,
	IMXDPUV1_ID_FETCHECO0,
	IMXDPUV1_ID_FETCHDECODE1,
	IMXDPUV1_ID_FETCHECO1,
	IMXDPUV1_ID_FETCHLAYER0,
	IMXDPUV1_ID_FETCHLAYER1,
	IMXDPUV1_ID_GAMMACOR4,
	IMXDPUV1_ID_MATRIX4,
	IMXDPUV1_ID_HSCALER4,
	IMXDPUV1_ID_VSCALER4,
	IMXDPUV1_ID_HISTOGRAM4,
	IMXDPUV1_ID_GAMMACOR5,
	IMXDPUV1_ID_MATRIX5,
	IMXDPUV1_ID_HSCALER5,
	IMXDPUV1_ID_VSCALER5,
	IMXDPUV1_ID_HISTOGRAM5,
	IMXDPUV1_ID_LAYERBLEND0,
	IMXDPUV1_ID_LAYERBLEND1,
	IMXDPUV1_ID_LAYERBLEND2,
	IMXDPUV1_ID_LAYERBLEND3,
	IMXDPUV1_ID_LAYERBLEND4,
	IMXDPUV1_ID_LAYERBLEND5,
	IMXDPUV1_ID_LAYERBLEND6,
	IMXDPUV1_ID_EXTSRC0,
	IMXDPUV1_ID_EXTSRC1,
	IMXDPUV1_ID_DISENGCFG,
	IMXDPUV1_ID_FRAMEDUMP0,
	IMXDPUV1_ID_FRAMEDUMP1,
	IMXDPUV1_ID_FRAMEGEN0,
	IMXDPUV1_ID_MATRIX0,
	IMXDPUV1_ID_GAMMACOR0,
	IMXDPUV1_ID_DITHER0,
	IMXDPUV1_ID_TCON0,
	IMXDPUV1_ID_SIG0,
	IMXDPUV1_ID_FRAMEGEN1,
	IMXDPUV1_ID_MATRIX1,
	IMXDPUV1_ID_GAMMACOR1,
	IMXDPUV1_ID_DITHER1,
	IMXDPUV1_ID_TCON1,
	IMXDPUV1_ID_SIG1,
	IMXDPUV1_ID_CAPENGCFG,
	IMXDPUV1_ID_FRAMECAP4,
	IMXDPUV1_ID_FRAMECAP5,
	IMXDPUV1_ID_ANALYSER4,
	IMXDPUV1_ID_ANALYSER5,
	/* the following are added arbitrarily */
	IMXDPUV1_ID_DPUXPC,

} imxdpuv1_id_t;
#else
/*!
 * Enumeration of IMXDPU logical block ids
 * NOTE: these match the hardware layout and are not arbitrary
 */
typedef enum {
	IMXDPUV1_ID_NONE = 0,
	IMXDPUV1_ID_FETCHDECODE9,
	IMXDPUV1_ID_FETCHWARP9,
	IMXDPUV1_ID_FETCHECO9,
	IMXDPUV1_ID_ROP9,
	IMXDPUV1_ID_CLUT9,
	IMXDPUV1_ID_MATRIX9,
	IMXDPUV1_ID_HSCALER9,
	IMXDPUV1_ID_VSCALER9,
	IMXDPUV1_ID_FILTER9,
	IMXDPUV1_ID_BLITBLEND9,
	IMXDPUV1_ID_STORE9,
	IMXDPUV1_ID_CONSTFRAME0,
	IMXDPUV1_ID_EXTDST0,
	IMXDPUV1_ID_CONSTFRAME4,
	IMXDPUV1_ID_EXTDST4,
	IMXDPUV1_ID_CONSTFRAME1,
	IMXDPUV1_ID_EXTDST1,
	IMXDPUV1_ID_CONSTFRAME5,
	IMXDPUV1_ID_EXTDST5,
	IMXDPUV1_ID_FETCHWARP2,
	IMXDPUV1_ID_FETCHECO2,
	IMXDPUV1_ID_FETCHDECODE0,
	IMXDPUV1_ID_FETCHECO0,
	IMXDPUV1_ID_FETCHDECODE1,
	IMXDPUV1_ID_FETCHECO1,
	IMXDPUV1_ID_FETCHLAYER0,
	IMXDPUV1_ID_MATRIX4,
	IMXDPUV1_ID_HSCALER4,
	IMXDPUV1_ID_VSCALER4,
	IMXDPUV1_ID_MATRIX5,
	IMXDPUV1_ID_HSCALER5,
	IMXDPUV1_ID_VSCALER5,
	IMXDPUV1_ID_LAYERBLEND0,
	IMXDPUV1_ID_LAYERBLEND1,
	IMXDPUV1_ID_LAYERBLEND2,
	IMXDPUV1_ID_LAYERBLEND3,
	IMXDPUV1_ID_DISENGCFG,
	IMXDPUV1_ID_FRAMEGEN0,
	IMXDPUV1_ID_MATRIX0,
	IMXDPUV1_ID_GAMMACOR0,
	IMXDPUV1_ID_DITHER0,
	IMXDPUV1_ID_TCON0,
	IMXDPUV1_ID_SIG0,
	IMXDPUV1_ID_FRAMEGEN1,
	IMXDPUV1_ID_MATRIX1,
	IMXDPUV1_ID_GAMMACOR1,
	IMXDPUV1_ID_DITHER1,
	IMXDPUV1_ID_TCON1,
	IMXDPUV1_ID_SIG1,
	IMXDPUV1_ID_DPUXPC,
} imxdpuv1_id_t;
#endif

#ifdef IMXDPUV1_VERSION_0
typedef enum {
	IMXDPUV1_SHDLD_CONSTFRAME0  =  1U << 4,
	IMXDPUV1_SHDLD_CONSTFRAME4  =  1U << 5,
	IMXDPUV1_SHDLD_CONSTFRAME1  =  1U << 6,
	IMXDPUV1_SHDLD_CONSTFRAME5  =  1U << 7,
	IMXDPUV1_SHDLD_EXTSRC4      =  1U << 8,
	IMXDPUV1_SHDLD_EXTSRC5      =  1U << 9,
	IMXDPUV1_SHDLD_FETCHDECODE2 =  1U << 10,
	IMXDPUV1_SHDLD_FETCHDECODE3 =  1U << 11,
	IMXDPUV1_SHDLD_FETCHWARP2   =  1U << 12,
	IMXDPUV1_SHDLD_FETCHECO2    =  1U << 13,
	IMXDPUV1_SHDLD_FETCHDECODE0 =  1U << 14,
	IMXDPUV1_SHDLD_FETCHECO0    =  1U << 15,
	IMXDPUV1_SHDLD_FETCHDECODE1 =  1U << 16,
	IMXDPUV1_SHDLD_FETCHECO1    =  1U << 17,
	IMXDPUV1_SHDLD_FETCHLAYER0  =  1U << 18,
	IMXDPUV1_SHDLD_FETCHLAYER1  =  1U << 19,
	IMXDPUV1_SHDLD_EXTSRC0      =  1U << 20,
	IMXDPUV1_SHDLD_EXTSRC1      =  1U << 21,
} imxdpuv1_shadow_load_req_id_t;
#else
typedef enum {
	IMXDPUV1_SHDLD_CONSTFRAME0  =  1U << 4,
	IMXDPUV1_SHDLD_CONSTFRAME4  =  1U << 5,
	IMXDPUV1_SHDLD_CONSTFRAME1  =  1U << 6,
	IMXDPUV1_SHDLD_CONSTFRAME5  =  1U << 7,
	IMXDPUV1_SHDLD_FETCHWARP2   =  1U << 8,
	IMXDPUV1_SHDLD_FETCHECO2    =  1U << 9,
	IMXDPUV1_SHDLD_FETCHDECODE0 =  1U << 10,
	IMXDPUV1_SHDLD_FETCHECO0    =  1U << 11,
	IMXDPUV1_SHDLD_FETCHDECODE1 =  1U << 12,
	IMXDPUV1_SHDLD_FETCHECO1    =  1U << 13,
	IMXDPUV1_SHDLD_FETCHLAYER0  =  1U << 14,

	IMXDPUV1_SHDLD_EXTSRC4      =  0,
	IMXDPUV1_SHDLD_EXTSRC5      =  0,
	IMXDPUV1_SHDLD_FETCHDECODE2 =  0,
	IMXDPUV1_SHDLD_FETCHDECODE3 =  0,
	IMXDPUV1_SHDLD_FETCHLAYER1  =  0,
	IMXDPUV1_SHDLD_EXTSRC0      =  0,
	IMXDPUV1_SHDLD_EXTSRC1      =  0,

} imxdpuv1_shadow_load_req_id_t;


#endif
typedef struct {
	imxdpuv1_id_t primary;
	imxdpuv1_id_t secondary;
	imxdpuv1_display_stream_t stream;
	bool enable;
} imxdpuv1_layer_t;

typedef enum {
	/* Fetch Channels */
	IMXDPUV1_CHAN_IDX_IN_FIRST = 0,
	IMXDPUV1_CHAN_IDX_00 = 0,	/* IMXDPUV1_ID_SRC_FETCHDECODE2 */
	IMXDPUV1_CHAN_IDX_01,	/* IMXDPUV1_ID_SRC_FETCHDECODE0 */
	IMXDPUV1_CHAN_IDX_02,	/* IMXDPUV1_ID_SRC_FETCHLAYER0 */
	IMXDPUV1_CHAN_IDX_03,	/* IMXDPUV1_ID_SRC_FETCHLAYER0 */
	IMXDPUV1_CHAN_IDX_04,	/* IMXDPUV1_ID_SRC_FETCHLAYER0 */
	IMXDPUV1_CHAN_IDX_05,	/* IMXDPUV1_ID_SRC_FETCHLAYER0 */
	IMXDPUV1_CHAN_IDX_06,	/* IMXDPUV1_ID_SRC_FETCHLAYER0 */
	IMXDPUV1_CHAN_IDX_07,	/* IMXDPUV1_ID_SRC_FETCHLAYER0 */
	IMXDPUV1_CHAN_IDX_08,	/* IMXDPUV1_ID_SRC_FETCHLAYER0 */
	IMXDPUV1_CHAN_IDX_09,	/* IMXDPUV1_ID_SRC_FETCHLAYER0 */
	IMXDPUV1_CHAN_IDX_10,	/* IMXDPUV1_ID_SRC_FETCHWARP2 */
	IMXDPUV1_CHAN_IDX_11,	/* IMXDPUV1_ID_SRC_FETCHWARP2 */
	IMXDPUV1_CHAN_IDX_12,	/* IMXDPUV1_ID_SRC_FETCHWARP2 */
	IMXDPUV1_CHAN_IDX_13,	/* IMXDPUV1_ID_SRC_FETCHWARP2 */
	IMXDPUV1_CHAN_IDX_14,	/* IMXDPUV1_ID_SRC_FETCHWARP2 */
	IMXDPUV1_CHAN_IDX_15,	/* IMXDPUV1_ID_SRC_FETCHWARP2 */
	IMXDPUV1_CHAN_IDX_16,	/* IMXDPUV1_ID_SRC_FETCHWARP2 */
	IMXDPUV1_CHAN_IDX_17,	/* IMXDPUV1_ID_SRC_FETCHWARP2 */
	IMXDPUV1_CHAN_IDX_18,	/* IMXDPUV1_ID_SRC_FETCHDECODE3 */
	IMXDPUV1_CHAN_IDX_19,	/* IMXDPUV1_ID_SRC_FETCHDECODE1 */
	IMXDPUV1_CHAN_IDX_20,	/* IMXDPUV1_ID_SRC_FETCHLAYER1 */
	IMXDPUV1_CHAN_IDX_21,	/* IMXDPUV1_ID_SRC_FETCHLAYER1 */
	IMXDPUV1_CHAN_IDX_22,	/* IMXDPUV1_ID_SRC_FETCHLAYER1 */
	IMXDPUV1_CHAN_IDX_23,	/* IMXDPUV1_ID_SRC_FETCHLAYER1 */
	IMXDPUV1_CHAN_IDX_24,	/* IMXDPUV1_ID_SRC_FETCHLAYER1 */
	IMXDPUV1_CHAN_IDX_25,	/* IMXDPUV1_ID_SRC_FETCHLAYER1 */
	IMXDPUV1_CHAN_IDX_26,	/* IMXDPUV1_ID_SRC_FETCHLAYER1 */
	IMXDPUV1_CHAN_IDX_27,	/* IMXDPUV1_ID_SRC_FETCHLAYER1 */
	IMXDPUV1_CHAN_IDX_28,	/* IMXDPUV1_ID_SRC_ECO0 */
	IMXDPUV1_CHAN_IDX_29,	/* IMXDPUV1_ID_SRC_ECO1 */
	IMXDPUV1_CHAN_IDX_30,	/* IMXDPUV1_ID_SRC_ECO2 */
	IMXDPUV1_CHAN_IDX_IN_MAX,	/* Last fetch channel + 1 */

	/* Store Channels */
	IMXDPUV1_CHAN_IDX_OUT_FIRST = 32,
	IMXDPUV1_CHAN_IDX_32 = 32,/* IMXDPUV1_ID_DST_STORE4 */
	IMXDPUV1_CHAN_IDX_33,	/* IMXDPUV1_ID_DST_STORE5 */
	IMXDPUV1_CHAN_IDX_OUT_MAX,/* Last fetch channel + 1 */
	IMXDPUV1_CHAN_IDX_MAX = IMXDPUV1_CHAN_IDX_OUT_MAX,
} imxdpuv1_chan_idx_t;

typedef enum {
	IMXDPUV1_SUB_NONE = 0,
	IMXDPUV1_SUB_1 = 1U << 0,	/* IMXDPUV1_ID_FETCHLAYER0, layer 1 */
	IMXDPUV1_SUB_2 = 1U << 1,	/* IMXDPUV1_ID_FETCHLAYER0, layer 2 */
	IMXDPUV1_SUB_3 = 1U << 2,	/* IMXDPUV1_ID_FETCHLAYER0, layer 3 */
	IMXDPUV1_SUB_4 = 1U << 3,	/* IMXDPUV1_ID_FETCHLAYER0, layer 4 */
	IMXDPUV1_SUB_5 = 1U << 4,	/* IMXDPUV1_ID_FETCHLAYER0, layer 5 */
	IMXDPUV1_SUB_6 = 1U << 5,	/* IMXDPUV1_ID_FETCHLAYER0, layer 6 */
	IMXDPUV1_SUB_7 = 1U << 6,	/* IMXDPUV1_ID_FETCHLAYER0, layer 7 */
	IMXDPUV1_SUB_8 = 1U << 7,	/* IMXDPUV1_ID_FETCHLAYER0, layer 8 */
} imxdpuv1_chan_sub_idx_t;

/*  IMXDPU Channel
 *	Consistist of four fields
 *	src - block id of source or destination
 *	sec - block id of secondary source for fetcheco
 *	sub - sub index of block for fetchlayer or fetchwarp
 *	idx - logical channel index
 *
 */
#define make_channel(__blk_id, __eco_id, __sub, __idx) \
(((__u32)(__idx)<<0)|((__u32)(__eco_id)<<8)|((__u32)(__sub)<<16)|((__u32)(__blk_id)<<24))

#define get_channel_blk(chan) (((__u32)(chan) >> 24) & 0xff)
#define get_channel_sub(chan) (((__u32)(chan) >> 16) & 0xff)
#define get_eco_idx(chan)     (((__u32)(chan) >> 8) & 0xff)
#define get_channel_idx(chan) (((__u32)(chan) >> 0) & 0xff)
#define IMXDPUV1_SUBCHAN_LAYER_OFFSET 0x28

typedef enum {
#ifdef IMXDPUV1_VERSION_0
	/* Fetch Channels */
	IMXDPUV1_CHAN_00 = make_channel(IMXDPUV1_ID_FETCHDECODE2, IMXDPUV1_ID_NONE, IMXDPUV1_SUB_NONE, 0),
	IMXDPUV1_CHAN_01 = make_channel(IMXDPUV1_ID_FETCHDECODE0,             28, IMXDPUV1_SUB_NONE, 1),
	IMXDPUV1_CHAN_02 = make_channel(IMXDPUV1_ID_FETCHLAYER0,  IMXDPUV1_ID_NONE, IMXDPUV1_SUB_1, 2),
	IMXDPUV1_CHAN_03 = make_channel(IMXDPUV1_ID_FETCHLAYER0,  IMXDPUV1_ID_NONE, IMXDPUV1_SUB_2, 3),
	IMXDPUV1_CHAN_04 = make_channel(IMXDPUV1_ID_FETCHLAYER0,  IMXDPUV1_ID_NONE, IMXDPUV1_SUB_3, 4),
	IMXDPUV1_CHAN_05 = make_channel(IMXDPUV1_ID_FETCHLAYER0,  IMXDPUV1_ID_NONE, IMXDPUV1_SUB_4, 5),
	IMXDPUV1_CHAN_06 = make_channel(IMXDPUV1_ID_FETCHLAYER0,  IMXDPUV1_ID_NONE, IMXDPUV1_SUB_5, 6),
	IMXDPUV1_CHAN_07 = make_channel(IMXDPUV1_ID_FETCHLAYER0,  IMXDPUV1_ID_NONE, IMXDPUV1_SUB_6, 7),
	IMXDPUV1_CHAN_08 = make_channel(IMXDPUV1_ID_FETCHLAYER0,  IMXDPUV1_ID_NONE, IMXDPUV1_SUB_7, 8),
	IMXDPUV1_CHAN_09 = make_channel(IMXDPUV1_ID_FETCHLAYER0,  IMXDPUV1_ID_NONE, IMXDPUV1_SUB_8, 9),
	IMXDPUV1_CHAN_10 = make_channel(IMXDPUV1_ID_FETCHWARP2,               30, IMXDPUV1_SUB_1, 10),
	IMXDPUV1_CHAN_11 = make_channel(IMXDPUV1_ID_FETCHWARP2,               30, IMXDPUV1_SUB_2, 11),
	IMXDPUV1_CHAN_12 = make_channel(IMXDPUV1_ID_FETCHWARP2,               30, IMXDPUV1_SUB_3, 12),
	IMXDPUV1_CHAN_13 = make_channel(IMXDPUV1_ID_FETCHWARP2,               30, IMXDPUV1_SUB_4, 13),
	IMXDPUV1_CHAN_14 = make_channel(IMXDPUV1_ID_FETCHWARP2,               30, IMXDPUV1_SUB_5, 14),
	IMXDPUV1_CHAN_15 = make_channel(IMXDPUV1_ID_FETCHWARP2,               30, IMXDPUV1_SUB_6, 15),
	IMXDPUV1_CHAN_16 = make_channel(IMXDPUV1_ID_FETCHWARP2,               30, IMXDPUV1_SUB_7, 16),
	IMXDPUV1_CHAN_17 = make_channel(IMXDPUV1_ID_FETCHWARP2,               30, IMXDPUV1_SUB_8, 17),
	IMXDPUV1_CHAN_18 = make_channel(IMXDPUV1_ID_FETCHDECODE3,             30, IMXDPUV1_SUB_NONE, 18),
	IMXDPUV1_CHAN_19 = make_channel(IMXDPUV1_ID_FETCHDECODE1,             29, IMXDPUV1_SUB_NONE, 19),
	IMXDPUV1_CHAN_20 = make_channel(IMXDPUV1_ID_FETCHLAYER1,  IMXDPUV1_ID_NONE, IMXDPUV1_SUB_1, 20),
	IMXDPUV1_CHAN_21 = make_channel(IMXDPUV1_ID_FETCHLAYER1,  IMXDPUV1_ID_NONE, IMXDPUV1_SUB_2, 21),
	IMXDPUV1_CHAN_22 = make_channel(IMXDPUV1_ID_FETCHLAYER1,  IMXDPUV1_ID_NONE, IMXDPUV1_SUB_3, 22),
	IMXDPUV1_CHAN_23 = make_channel(IMXDPUV1_ID_FETCHLAYER1,  IMXDPUV1_ID_NONE, IMXDPUV1_SUB_4, 23),
	IMXDPUV1_CHAN_24 = make_channel(IMXDPUV1_ID_FETCHLAYER1,  IMXDPUV1_ID_NONE, IMXDPUV1_SUB_5, 24),
	IMXDPUV1_CHAN_25 = make_channel(IMXDPUV1_ID_FETCHLAYER1,  IMXDPUV1_ID_NONE, IMXDPUV1_SUB_6, 25),
	IMXDPUV1_CHAN_26 = make_channel(IMXDPUV1_ID_FETCHLAYER1,  IMXDPUV1_ID_NONE, IMXDPUV1_SUB_7, 26),
	IMXDPUV1_CHAN_27 = make_channel(IMXDPUV1_ID_FETCHLAYER1,  IMXDPUV1_ID_NONE, IMXDPUV1_SUB_8, 27),
	IMXDPUV1_CHAN_28 = make_channel(IMXDPUV1_ID_FETCHECO0,    IMXDPUV1_ID_NONE, IMXDPUV1_SUB_NONE, 28),
	IMXDPUV1_CHAN_29 = make_channel(IMXDPUV1_ID_FETCHECO1,    IMXDPUV1_ID_NONE, IMXDPUV1_SUB_NONE, 29),
	IMXDPUV1_CHAN_30 = make_channel(IMXDPUV1_ID_FETCHECO2,    IMXDPUV1_ID_NONE, IMXDPUV1_SUB_NONE, 30),
	/* Store Channels */
	IMXDPUV1_CHAN_32 = make_channel(IMXDPUV1_ID_STORE4, IMXDPUV1_ID_NONE, IMXDPUV1_SUB_NONE, 32),
	IMXDPUV1_CHAN_33 = make_channel(IMXDPUV1_ID_STORE5, IMXDPUV1_ID_NONE, IMXDPUV1_SUB_NONE, 33),
#else
	/* Fetch Channels */
	IMXDPUV1_CHAN_00 = make_channel(0, 0, 0, 0),
	IMXDPUV1_CHAN_01 = make_channel(IMXDPUV1_ID_FETCHDECODE0,             28, IMXDPUV1_SUB_NONE, 1),
	IMXDPUV1_CHAN_02 = make_channel(IMXDPUV1_ID_FETCHLAYER0,  IMXDPUV1_ID_NONE, IMXDPUV1_SUB_1, 2),
	IMXDPUV1_CHAN_03 = make_channel(IMXDPUV1_ID_FETCHLAYER0,  IMXDPUV1_ID_NONE, IMXDPUV1_SUB_2, 3),
	IMXDPUV1_CHAN_04 = make_channel(IMXDPUV1_ID_FETCHLAYER0,  IMXDPUV1_ID_NONE, IMXDPUV1_SUB_3, 4),
	IMXDPUV1_CHAN_05 = make_channel(IMXDPUV1_ID_FETCHLAYER0,  IMXDPUV1_ID_NONE, IMXDPUV1_SUB_4, 5),
	IMXDPUV1_CHAN_06 = make_channel(IMXDPUV1_ID_FETCHLAYER0,  IMXDPUV1_ID_NONE, IMXDPUV1_SUB_5, 6),
	IMXDPUV1_CHAN_07 = make_channel(IMXDPUV1_ID_FETCHLAYER0,  IMXDPUV1_ID_NONE, IMXDPUV1_SUB_6, 7),
	IMXDPUV1_CHAN_08 = make_channel(IMXDPUV1_ID_FETCHLAYER0,  IMXDPUV1_ID_NONE, IMXDPUV1_SUB_7, 8),
	IMXDPUV1_CHAN_09 = make_channel(IMXDPUV1_ID_FETCHLAYER0,  IMXDPUV1_ID_NONE, IMXDPUV1_SUB_8, 9),
	IMXDPUV1_CHAN_10 = make_channel(IMXDPUV1_ID_FETCHWARP2,               30, IMXDPUV1_SUB_1, 10),
	IMXDPUV1_CHAN_11 = make_channel(IMXDPUV1_ID_FETCHWARP2,               30, IMXDPUV1_SUB_2, 11),
	IMXDPUV1_CHAN_12 = make_channel(IMXDPUV1_ID_FETCHWARP2,               30, IMXDPUV1_SUB_3, 12),
	IMXDPUV1_CHAN_13 = make_channel(IMXDPUV1_ID_FETCHWARP2,               30, IMXDPUV1_SUB_4, 13),
	IMXDPUV1_CHAN_14 = make_channel(IMXDPUV1_ID_FETCHWARP2,               30, IMXDPUV1_SUB_5, 14),
	IMXDPUV1_CHAN_15 = make_channel(IMXDPUV1_ID_FETCHWARP2,               30, IMXDPUV1_SUB_6, 15),
	IMXDPUV1_CHAN_16 = make_channel(IMXDPUV1_ID_FETCHWARP2,               30, IMXDPUV1_SUB_7, 16),
	IMXDPUV1_CHAN_17 = make_channel(IMXDPUV1_ID_FETCHWARP2,               30, IMXDPUV1_SUB_8, 17),
	IMXDPUV1_CHAN_18 = make_channel(0, 0, 0, 0),
	IMXDPUV1_CHAN_19 = make_channel(IMXDPUV1_ID_FETCHDECODE1,             29, IMXDPUV1_SUB_NONE, 19),
	IMXDPUV1_CHAN_20 = make_channel(0, 0, 0, 0),
	IMXDPUV1_CHAN_21 = make_channel(0, 0, 0, 0),
	IMXDPUV1_CHAN_22 = make_channel(0, 0, 0, 0),
	IMXDPUV1_CHAN_23 = make_channel(0, 0, 0, 0),
	IMXDPUV1_CHAN_24 = make_channel(0, 0, 0, 0),
	IMXDPUV1_CHAN_25 = make_channel(0, 0, 0, 0),
	IMXDPUV1_CHAN_26 = make_channel(0, 0, 0, 0),
	IMXDPUV1_CHAN_27 = make_channel(0, 0, 0, 0),
	IMXDPUV1_CHAN_28 = make_channel(IMXDPUV1_ID_FETCHECO0,    IMXDPUV1_ID_NONE, IMXDPUV1_SUB_NONE, 28),
	IMXDPUV1_CHAN_29 = make_channel(IMXDPUV1_ID_FETCHECO1,    IMXDPUV1_ID_NONE, IMXDPUV1_SUB_NONE, 29),
	IMXDPUV1_CHAN_30 = make_channel(IMXDPUV1_ID_FETCHECO2,    IMXDPUV1_ID_NONE, IMXDPUV1_SUB_NONE, 30),
	/* Store Channels */
	IMXDPUV1_CHAN_32 = make_channel(0, 0, 0, 0),
	IMXDPUV1_CHAN_33 = make_channel(0, 0, 0, 0),
#endif
} imxdpuv1_chan_t;

/* Aliases for Channels */
#define IMXDPUV1_CHAN_VIDEO_0        IMXDPUV1_CHAN_01
#define IMXDPUV1_CHAN_VIDEO_1        IMXDPUV1_CHAN_19

#define IMXDPUV1_CHAN_INTEGRAL_0    IMXDPUV1_CHAN_00
#define IMXDPUV1_CHAN_INTEGRAL_1    IMXDPUV1_CHAN_18

#define IMXDPUV1_CHAN_FRACTIONAL_0_1 IMXDPUV1_CHAN_02
#define IMXDPUV1_CHAN_FRACTIONAL_0_2 IMXDPUV1_CHAN_03
#define IMXDPUV1_CHAN_FRACTIONAL_0_3 IMXDPUV1_CHAN_04
#define IMXDPUV1_CHAN_FRACTIONAL_0_4 IMXDPUV1_CHAN_05
#define IMXDPUV1_CHAN_FRACTIONAL_0_5 IMXDPUV1_CHAN_06
#define IMXDPUV1_CHAN_FRACTIONAL_0_6 IMXDPUV1_CHAN_07
#define IMXDPUV1_CHAN_FRACTIONAL_0_7 IMXDPUV1_CHAN_08
#define IMXDPUV1_CHAN_FRACTIONAL_0_8 IMXDPUV1_CHAN_09

#define IMXDPUV1_CHAN_FRACTIONAL_1_1 IMXDPUV1_CHAN_20
#define IMXDPUV1_CHAN_FRACTIONAL_1_2 IMXDPUV1_CHAN_21
#define IMXDPUV1_CHAN_FRACTIONAL_1_3 IMXDPUV1_CHAN_22
#define IMXDPUV1_CHAN_FRACTIONAL_1_4 IMXDPUV1_CHAN_23
#define IMXDPUV1_CHAN_FRACTIONAL_1_5 IMXDPUV1_CHAN_24
#define IMXDPUV1_CHAN_FRACTIONAL_1_6 IMXDPUV1_CHAN_25
#define IMXDPUV1_CHAN_FRACTIONAL_1_7 IMXDPUV1_CHAN_26
#define IMXDPUV1_CHAN_FRACTIONAL_1_8 IMXDPUV1_CHAN_27

#define IMXDPUV1_CHAN_WARP_2_1 IMXDPUV1_CHAN_10
#define IMXDPUV1_CHAN_WARP_2_2 IMXDPUV1_CHAN_11
#define IMXDPUV1_CHAN_WARP_2_3 IMXDPUV1_CHAN_12
#define IMXDPUV1_CHAN_WARP_2_4 IMXDPUV1_CHAN_13
#define IMXDPUV1_CHAN_WARP_2_5 IMXDPUV1_CHAN_14
#define IMXDPUV1_CHAN_WARP_2_6 IMXDPUV1_CHAN_15
#define IMXDPUV1_CHAN_WARP_2_7 IMXDPUV1_CHAN_16
#define IMXDPUV1_CHAN_WARP_2_8 IMXDPUV1_CHAN_17

#define IMXDPUV1_CHAN_CAPTURE_0        IMXDPUV1_CHAN_32
#define IMXDPUV1_CHAN_CAPTURE_1        IMXDPUV1_CHAN_33


/*  IMXDPU Pixel format definitions */
/*  Four-character-code (FOURCC) */
#ifdef fourcc
#warning "fourcc is already defined ... redeifining it here!"
#undef  fourcc
#endif
#define fourcc(a, b, c, d)\
	 (((__u32)(a)<<0)|((__u32)(b)<<8)|((__u32)(c)<<16)|((__u32)(d)<<24))


/*! @} */
/*! @name Generic Formats */
/*! @{ */
#define IMXDPUV1_PIX_FMT_GENERIC fourcc('D', 'P', 'U', '0')	/*!< IPU Generic Data */
#define IMXDPUV1_PIX_FMT_GENERIC_32 fourcc('D', 'P', 'U', '1')	/*!< IPU Generic Data */
#define IMXDPUV1_PIX_FMT_GENERIC_16 fourcc('D', 'P', 'U', '2')	/*!< IPU Generic Data */

/*! @} */
/*! @name RGB Formats */
/*! @{ */
#define IMXDPUV1_PIX_FMT_RGB332  fourcc('R', 'G', 'B', '1')	/*!<  8  RGB-3-3-2    */
#define IMXDPUV1_PIX_FMT_RGB555  fourcc('R', 'G', 'B', 'O')	/*!< 16  RGB-5-5-5    */
#define IMXDPUV1_PIX_FMT_RGB565  fourcc('R', 'G', 'B', 'P')	/*!< 16  RGB-5-6-5    */
#define IMXDPUV1_PIX_FMT_BGRA4444 fourcc('4', '4', '4', '4')	/*!< 16  RGBA-4-4-4-4 */
#define IMXDPUV1_PIX_FMT_BGRA5551 fourcc('5', '5', '5', '1')	/*!< 16  RGBA-5-5-5-1 */
#define IMXDPUV1_PIX_FMT_RGB666  fourcc('R', 'G', 'B', '6')	/*!< 18  RGB-6-6-6    */
#define IMXDPUV1_PIX_FMT_BGR666  fourcc('B', 'G', 'R', '6')	/*!< 18  BGR-6-6-6    */
#define IMXDPUV1_PIX_FMT_BGR24   fourcc('B', 'G', 'R', '3')	/*!< 24  BGR-8-8-8    */
#define IMXDPUV1_PIX_FMT_RGB24   fourcc('R', 'G', 'B', '3')	/*!< 24  RGB-8-8-8    */
#define IMXDPUV1_PIX_FMT_GBR24   fourcc('G', 'B', 'R', '3')	/*!< 24  GBR-8-8-8    */
#define IMXDPUV1_PIX_FMT_BGR32   fourcc('B', 'G', 'R', '4')	/*!< 32  BGR-8-8-8-8  */
#define IMXDPUV1_PIX_FMT_BGRA32  fourcc('B', 'G', 'R', 'A')	/*!< 32  BGR-8-8-8-8  */
#define IMXDPUV1_PIX_FMT_RGB32   fourcc('R', 'G', 'B', '4')	/*!< 32  RGB-8-8-8-8  */
#define IMXDPUV1_PIX_FMT_RGBA32  fourcc('R', 'G', 'B', 'A')	/*!< 32  RGB-8-8-8-8  */
#define IMXDPUV1_PIX_FMT_ABGR32  fourcc('A', 'B', 'G', 'R')	/*!< 32  ABGR-8-8-8-8 */
#define IMXDPUV1_PIX_FMT_ARGB32  fourcc('A', 'R', 'G', 'B')	/*!< 32  ARGB-8-8-8-8 */

/*! @} */
/*! @name YUV Interleaved Formats */
/*! @{ */
#define IMXDPUV1_PIX_FMT_YUYV    fourcc('Y', 'U', 'Y', 'V')	/*!< 16 YUV 4:2:2 */
#define IMXDPUV1_PIX_FMT_UYVY    fourcc('U', 'Y', 'V', 'Y')	/*!< 16 YUV 4:2:2 */
#define IMXDPUV1_PIX_FMT_YVYU    fourcc('Y', 'V', 'Y', 'U')	/*!< 16 YVYU 4:2:2 */
#define IMXDPUV1_PIX_FMT_VYUY    fourcc('V', 'Y', 'U', 'Y')	/*!< 16 VYYU 4:2:2 */
#define IMXDPUV1_PIX_FMT_Y41P    fourcc('Y', '4', '1', 'P')	/*!< 12 YUV 4:1:1 */
#define IMXDPUV1_PIX_FMT_YUV444  fourcc('Y', '4', '4', '4')	/*!< 24 YUV 4:4:4 */
#define IMXDPUV1_PIX_FMT_VYU444  fourcc('V', '4', '4', '4')	/*!< 24 VYU 4:4:4 */
#define IMXDPUV1_PIX_FMT_AYUV    fourcc('A', 'Y', 'U', 'V')	/*!< 32 AYUV 4:4:4:4 */

/* two planes -- one Y, one Cb + Cr interleaved  */
#define IMXDPUV1_PIX_FMT_NV12    fourcc('N', 'V', '1', '2')	/* 12  Y/CbCr 4:2:0  */
#define IMXDPUV1_PIX_FMT_NV16    fourcc('N', 'V', '1', '6')	/* 16  Y/CbCr 4:2:2  */

#define IMXDPUV1_CAP_FMT_RGB24   fourcc('R', 'G', 'B', '3')
#define IMXDPUV1_CAP_FMT_BT656   fourcc('B', '6', '5', '6')
#define IMXDPUV1_CAP_FMT_YUYV    fourcc('Y', 'U', 'Y', 'V')

struct imxdpuv1_soc;
/*!
 * Definition of IMXDPU rectangle structure
 */
typedef struct {
	int16_t top;		/* y coordinate of top/left pixel */
	int16_t left;		/* x coordinate top/left pixel */
	int16_t width;
	int16_t height;
} imxdpuv1_rect_t;


/*!
 * Union of initialization parameters for a logical channel.
 */
typedef union {
	struct {
		imxdpuv1_chan_t chan;
		uint32_t src_pixel_fmt;
		uint16_t src_width;
		uint16_t src_height;
		int16_t clip_top;
		int16_t clip_left;
		uint16_t clip_width;
		uint16_t clip_height;
		uint16_t stride;
		uint32_t dest_pixel_fmt;
		uint8_t blend_mode;
		uint8_t blend_layer;
		uint8_t disp_id; /* capture id */
		int16_t dest_top;
		int16_t dest_left;
		uint16_t dest_width;
		uint16_t dest_height;
		uint32_t const_color;
		bool use_global_alpha;
		bool use_local_alpha;
	} common;
	struct {
		imxdpuv1_chan_t chan;
		uint32_t src_pixel_fmt;
		uint16_t src_width;
		uint16_t src_height;
		int16_t clip_top;
		int16_t clip_left;
		uint16_t clip_width;
		uint16_t clip_height;
		uint16_t stride;
		uint32_t dest_pixel_fmt;
		uint8_t blend_mode;
		uint8_t blend_layer;
		uint8_t capture_id; /* disp_id/capture id */
		int16_t dest_top;
		int16_t dest_left;
		uint16_t dest_width;
		uint16_t dest_height;
		uint32_t const_color;
		bool use_global_alpha;
		bool use_local_alpha;
		uint32_t h_scale_factor;    /* downscaling  out/in */
		uint32_t h_phase;
		uint32_t v_scale_factor;    /* downscaling  out/in */
		uint32_t v_phase[2][2];
		bool use_video_proc;
		bool interlaced;
	} store;
	struct {
		imxdpuv1_chan_t chan;
		uint32_t src_pixel_fmt;
		uint16_t src_width;
		uint16_t src_height;
		int16_t clip_top;
		int16_t clip_left;
		uint16_t clip_width;
		uint16_t clip_height;
		uint16_t stride;
		uint32_t dest_pixel_fmt;
		uint8_t blend_mode;
		uint8_t blend_layer;
		uint8_t disp_id;
		int16_t dest_top;
		int16_t dest_left;
		uint16_t dest_width;
		uint16_t dest_height;
		uint32_t const_color;
		bool use_global_alpha;
		bool use_local_alpha;
		uint32_t h_scale_factor;    /* downscaling  out/in */
		uint32_t h_phase;
		uint32_t v_scale_factor;    /* downscaling  out/in */
		uint32_t v_phase[2][2];
		bool use_video_proc;
		bool interlaced;
	} fetch_decode;
	struct {
		imxdpuv1_chan_t chan;
		uint32_t src_pixel_fmt;
		uint16_t src_width;
		uint16_t src_height;
		int16_t clip_top;
		int16_t clip_left;
		uint16_t clip_width;
		uint16_t clip_height;
		uint16_t stride;
		uint32_t dest_pixel_fmt;
		uint8_t blend_mode;
		uint8_t blend_layer;
		uint8_t disp_id; /* capture id */
		int16_t dest_top;
		int16_t dest_left;
		uint16_t dest_width;
		uint16_t dest_height;
		uint32_t const_color;
		bool use_global_alpha;
		bool use_local_alpha;
	} fetch_layer;
	struct {
		imxdpuv1_chan_t chan;
		uint32_t src_pixel_fmt;
		uint16_t src_width;
		uint16_t src_height;
		int16_t clip_top;
		int16_t clip_left;
		uint16_t clip_width;
		uint16_t clip_height;
		uint16_t stride;
		uint32_t dest_pixel_fmt;
		uint8_t blend_mode;
		uint8_t blend_layer;
		uint8_t disp_id; /* capture id */
		int16_t dest_top;
		int16_t dest_left;
		uint16_t dest_width;
		uint16_t dest_height;
		uint32_t const_color;
		bool use_global_alpha;
		bool use_local_alpha;
	} fetch_warp;
} imxdpuv1_channel_params_t;

/*!
 * Enumeration of IMXDPU video mode flags
 */
enum imxdpuv1_mode_flags {
	/* 1 is active high 0 is active low */
	IMXDPUV1_MODE_FLAGS_HSYNC_POL     = 1 << 0,
	IMXDPUV1_MODE_FLAGS_VSYNC_POL     = 1 << 1,
	IMXDPUV1_MODE_FLAGS_DE_POL        = 1 << 2,

	/* drive data on positive .edge */
	IMXDPUV1_MODE_FLAGS_CLK_POL       = 1 << 3,

	IMXDPUV1_MODE_FLAGS_INTERLACED    = 1 << 4 ,

	/* Left/Right Synchronous display mode,  both display pipe are
	   combined to make one display. All mode timings are divided by
	   two for each half screen.
	   Note: This may not be needed we may force this for any width
	   over ~2048
	 */
	IMXDPUV1_MODE_FLAGS_LRSYNC        = 1 << 8,

	/* Split mode each pipe is split into two displays  */
	IMXDPUV1_MODE_FLAGS_SPLIT         = 1 << 9,

	IMXDPUV1_MODE_FLAGS_32BIT          = 1 << 16,
	IMXDPUV1_MODE_FLAGS_BT656_10BIT    = 1 << 17,
	IMXDPUV1_MODE_FLAGS_BT656_8BIT     = 1 << 18,
};

struct imxdpuv1_videomode {
	char name[64];		/* may not be needed */

	uint32_t pixelclock;	/* Hz */

	/* htotal (pixels) = hlen + hfp + hsync + hbp */
	uint32_t hlen;
	uint32_t hfp;
	uint32_t hbp;
	uint32_t hsync;

	/* field0 - vtotal (lines) = vlen + vfp + vsync + vbp */
	uint32_t vlen;
	uint32_t vfp;
	uint32_t vbp;
	uint32_t vsync;

	/* field1  */
	uint32_t vlen1;
	uint32_t vfp1;
	uint32_t vbp1;
	uint32_t vsync1;

	uint32_t flags;
	uint32_t format;
	uint32_t dest_format; /*buffer format for capture*/
	int16_t  clip_top;
	int16_t  clip_left;
	uint16_t clip_width;
	uint16_t clip_height;

};

#define IMXDPUV1_ENABLE  1
#define IMXDPUV1_DISABLE 0

#define IMXDPUV1_TRUE    1
#define IMXDPUV1_FALSE   0
#define IMXDPUV1_OFFSET_INVALID 0x10000000	/* this should force an access error */
#define IMXDPUV1_CHANNEL_INVALID 0x0	/* this should force an access error */

#define IMXDPUV1_MIN(_X, _Y) ((_X) < (_Y) ? (_X) : (_Y))

/* Native color type */
#define IMXDPUV1_COLOR_CONSTALPHA_MASK 0xFFU
#define IMXDPUV1_COLOR_CONSTALPHA_SHIFT 0U
#define IMXDPUV1_COLOR_CONSTBLUE_MASK 0xFF00U
#define IMXDPUV1_COLOR_CONSTBLUE_SHIFT 8U
#define IMXDPUV1_COLOR_CONSTGREEN_MASK 0xFF0000U
#define IMXDPUV1_COLOR_CONSTGREEN_SHIFT 16U
#define IMXDPUV1_COLOR_CONSTRED_MASK  0xFF000000U
#define IMXDPUV1_COLOR_CONSTRED_SHIFT 24U

#define IMXDPUV1_IRQF_NONE    0x0
#define IMXDPUV1_IRQF_ONESHOT 0x1
#define IMXDPUV1_INTERRUPT_MAX  (66 + 1)	/* IMXDPUV1_FRAMECAP5_SYNC_OFF_IRQ
					   (66) is last interrupt */

int imxdpuv1_enable_irq(int8_t imxdpuv1_id, uint32_t irq);
int imxdpuv1_disable_irq(int8_t imxdpuv1_id, uint32_t irq);
int imxdpuv1_clear_all_irqs(int8_t imxdpuv1_id);
int imxdpuv1_clear_irq(int8_t imxdpuv1_id, uint32_t irq);
int imxdpuv1_init_irqs(int8_t imxdpuv1_id);
int imxdpuv1_request_irq(int8_t imxdpuv1_id,
	uint32_t irq,
	int(*handler) (int, void *),
	uint32_t irq_flags,
	const char *devname, void *data) ;
int imxdpuv1_free_irq(int8_t imxdpuv1_id, uint32_t irq, void *data);
int imxdpuv1_uninit_interrupts(int8_t imxdpuv1_id);
int imxdpuv1_handle_irq(int32_t imxdpuv1_id);
struct imxdpuv1_soc *imxdpuv1_get_soc(int8_t imxdpuv1_id);
int imxdpuv1_init(int8_t imxdpuv1_id);
int imxdpuv1_init_sync_panel(int8_t imxdpuv1_id, int8_t disp,
	uint32_t pixel_fmt,
	struct imxdpuv1_videomode mode);
int imxdpuv1_uninit_sync_panel(int8_t imxdpuv1_id, int8_t disp);
int imxdpuv1_reset_disp_panel(int8_t imxdpuv1_id, int8_t disp);
int imxdpuv1_disp_init(int8_t imxdpuv1_id, int8_t disp);
int imxdpuv1_disp_setup_frame_gen(
	int8_t imxdpuv1_id,
	int8_t disp,
	const struct imxdpuv1_videomode *mode,
	uint16_t cc_red,	/* 10 bits */
	uint16_t cc_green,	/* 10 bits */
	uint16_t cc_blue,	/* 10 bits */
	uint8_t cc_alpha,
	bool test_mode_enable);
int imxdpuv1_disp_enable_frame_gen(int8_t imxdpuv1_id,
	int8_t disp,
	bool enable);
int imxdpuv1_disp_setup_constframe(int8_t imxdpuv1_id,
	int8_t disp,
	uint8_t bg_red,
	uint8_t bg_green,
	uint8_t bg_blue,
	uint8_t bg_alpha);
int imxdpuv1_disp_setup_layer(int8_t imxdpuv1_id,
	const imxdpuv1_layer_t *layer,
			    imxdpuv1_layer_idx_t layer_idx,
			    bool is_top_layer);
void imxdpuv1_disp_dump_mode(const struct imxdpuv1_videomode *mode);
int imxdpuv1_bytes_per_pixel(uint32_t fmt);
int imxdpuv1_init_channel_buffer(int8_t imxdpuv1_id,
	imxdpuv1_chan_t chan,
	uint32_t stride,
	imxdpuv1_rotate_mode_t rot_mode,
	dma_addr_t phyaddr_0,
	uint32_t u_offset,
	uint32_t v_offset);
int32_t imxdpuv1_update_channel_buffer(int8_t imxdpuv1_id,
	imxdpuv1_chan_t chan,
	dma_addr_t phyaddr_0);
int imxdpuv1_init_channel(int8_t imxdpuv1_id,
	imxdpuv1_channel_params_t *params);
int imxdpuv1_disp_set_layer_global_alpha(int8_t imxdpuv1_id,
	imxdpuv1_layer_idx_t layer_idx,
	uint8_t alpha);
int imxdpuv1_disp_set_layer_position(int8_t imxdpuv1_id,
	imxdpuv1_layer_idx_t layer_idx,
	int16_t x, int16_t y);
int imxdpuv1_disp_set_chan_position(int8_t imxdpuv1_id,
	imxdpuv1_chan_t chan,
	int16_t x, int16_t y);
int imxdpuv1_disp_update_fgen_status(int8_t imxdpuv1_id, int8_t disp);
int imxdpuv1_disp_show_fgen_status(int8_t imxdpuv1_id);
void imxdpuv1_dump_int_stat(int8_t imxdpuv1_id);
void imxdpuv1_dump_layerblend(int8_t imxdpuv1_id);
int imxdpuv1_disp_force_shadow_load(int8_t imxdpuv1_id,
	int8_t disp,
	uint64_t mask);
int imxdpuv1_disp_set_chan_crop(int8_t imxdpuv1_id,
	imxdpuv1_chan_t chan,
	int16_t  clip_top,
	int16_t  clip_left,
	uint16_t clip_width,
	uint16_t clip_height,
	int16_t  dest_top,
	int16_t  dest_left,
	uint16_t dest_width,
	uint16_t dest_height);
void imxdpuv1_dump_pixencfg_status(int8_t imxdpuv1_id);
int imxdpuv1_dump_channel(int8_t imxdpuv1_id, imxdpuv1_chan_t chan);
uint32_t imxdpuv1_get_planes(uint32_t fmt);

int imxdpuv1_disp_setup_channel(int8_t imxdpuv1_id,
			      imxdpuv1_chan_t chan,
			      uint32_t src_pixel_fmt,
			      uint16_t src_width,
			      uint16_t src_height,
			      int16_t clip_top,
			      int16_t clip_left,
			      uint16_t clip_width,
			      uint16_t clip_height,
			      uint16_t stride,
			      uint8_t disp_id,
			      int16_t dest_top,
			      int16_t dest_left,
			      uint16_t dest_width,
			      uint16_t dest_height,
			      uint32_t const_color,
			      bool use_global_alpha,
			      bool use_local_alpha,
			      unsigned int disp_addr);
int imxdpuv1_disp_check_shadow_loads(int8_t imxdpuv1_id, int8_t disp);

int imxdpuv1_cap_setup_frame(
	int8_t imxdpuv1_id,
	int8_t src_id,
	int8_t dest_id,
	int8_t sync_count,
	const struct imxdpuv1_videomode *cap_mode);
int imxdpuv1_cap_setup_crop(
	int8_t imxdpuv1_id,
	int8_t src_id,
	int16_t  clip_top,
	int16_t  clip_left,
	uint16_t clip_width,
	uint16_t clip_height);

int imxdpuv1_cap_enable(int8_t imxdpuv1_id, int8_t cap, bool enable);
int imxdpuv1_cap_request_shadow_load(int8_t imxdpuv1_id, int8_t dest_id, uint32_t mask);

/* FIXME: add api if needed */
static inline int32_t imxdpuv1_csi_enable_mclk_if(int8_t imxdpuv1_id, int src, uint32_t cap,
		bool flag, bool wait)
{
	printf("%s(): %s:%d stubbed feature\n", __func__, __FILE__, __LINE__);
	return 0;
}
#endif				/* IMXDPUV1_H */
