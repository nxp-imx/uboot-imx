/*
 * Copyright 2015-2017 Freescale Semiconductor, Inc.
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <linux/types.h>

#include "imxdpuv1_private.h"
#include "imxdpuv1_registers.h"
#include "imxdpuv1_events.h"

#include "imxdpuv1_be.h"

#define ptr_to_uint32(__ptr__) ((uint32_t)((uint64_t)(__ptr__)))

/* Private data*/
static struct imxdpuv1_soc imxdpuv1_array[IMXDPUV1_MAX_NUM];

typedef struct {
	uint8_t len;
	uint8_t buffers;
} imxdpuv1_burst_entry_t;

static const imxdpuv1_burst_entry_t burst_param[] = {
	{ 0, 0 },     /* IMXDPUV1_SCAN_DIR_UNKNOWN */
	{ 8, 32 },    /* IMXDPUV1_SCAN_DIR_LEFT_RIGHT_DOWN */
	{ 16, 16 },   /* IMXDPUV1_SCAN_DIR_HORIZONTAL */
	{ 8, 32 },    /* IMXDPUV1_SCAN_DIR_VERTICAL possibly 8/32 here */
	{ 8, 32 },    /* IMXDPUV1_SCAN_DIR_FREE */
};

typedef struct {
	uint32_t extdst;
	uint32_t sub;
} trigger_entry_t;

static const trigger_entry_t trigger_list[IMXDPUV1_SHDLD_IDX_MAX] = {
	/*  IMXDPUV1_SHDLD_* extdst,          sub */
	/* _DISP0    */{ 1, 0 },
	/* _DISP1    */{ 1, 0 },
	/* _CONST0   */{ IMXDPUV1_SHDLD_CONSTFRAME0, 0 },
	/* _CONST1   */{ IMXDPUV1_SHDLD_CONSTFRAME1, 0 },
	/* _CHAN_00  */{ IMXDPUV1_SHDLD_FETCHDECODE2, 0 },
	/* _CHAN_01  */{ IMXDPUV1_SHDLD_FETCHDECODE0, 0 },
	/* _CHAN_02  */{ IMXDPUV1_SHDLD_FETCHLAYER0, IMXDPUV1_SUB_1 },
	/* _CHAN_03  */{ IMXDPUV1_SHDLD_FETCHLAYER0, IMXDPUV1_SUB_2 },
	/* _CHAN_04  */{ IMXDPUV1_SHDLD_FETCHLAYER0, IMXDPUV1_SUB_3 },
	/* _CHAN_05  */{ IMXDPUV1_SHDLD_FETCHLAYER0, IMXDPUV1_SUB_4 },
	/* _CHAN_06  */{ IMXDPUV1_SHDLD_FETCHLAYER0, IMXDPUV1_SUB_5 },
	/* _CHAN_07  */{ IMXDPUV1_SHDLD_FETCHLAYER0, IMXDPUV1_SUB_6 },
	/* _CHAN_08  */{ IMXDPUV1_SHDLD_FETCHLAYER0, IMXDPUV1_SUB_7 },
	/* _CHAN_09  */{ IMXDPUV1_SHDLD_FETCHLAYER0, IMXDPUV1_SUB_8 },
	/* _CHAN_10  */{ IMXDPUV1_SHDLD_FETCHWARP2, IMXDPUV1_SUB_1 << 16 },
	/* _CHAN_11  */{ IMXDPUV1_SHDLD_FETCHWARP2, IMXDPUV1_SUB_2 << 16 },
	/* _CHAN_12  */{ IMXDPUV1_SHDLD_FETCHWARP2, IMXDPUV1_SUB_3 << 16 },
	/* _CHAN_13  */{ IMXDPUV1_SHDLD_FETCHWARP2, IMXDPUV1_SUB_4 << 16 },
	/* _CHAN_14  */{ IMXDPUV1_SHDLD_FETCHWARP2, IMXDPUV1_SUB_5 << 16 },
	/* _CHAN_15  */{ IMXDPUV1_SHDLD_FETCHWARP2, IMXDPUV1_SUB_6 << 16 },
	/* _CHAN_16  */{ IMXDPUV1_SHDLD_FETCHWARP2, IMXDPUV1_SUB_7 << 16 },
	/* _CHAN_17  */{ IMXDPUV1_SHDLD_FETCHWARP2, IMXDPUV1_SUB_8 << 16 },
	/* _CHAN_18  */{ IMXDPUV1_SHDLD_FETCHDECODE3, 0 },
	/* _CHAN_19  */{ IMXDPUV1_SHDLD_FETCHDECODE1, 0 },
	/* _CHAN_20  */{ IMXDPUV1_SHDLD_FETCHLAYER1, IMXDPUV1_SUB_1 << 8 },
	/* _CHAN_21  */{ IMXDPUV1_SHDLD_FETCHLAYER1, IMXDPUV1_SUB_2 << 8 },
	/* _CHAN_22  */{ IMXDPUV1_SHDLD_FETCHLAYER1, IMXDPUV1_SUB_3 << 8 },
	/* _CHAN_23  */{ IMXDPUV1_SHDLD_FETCHLAYER1, IMXDPUV1_SUB_4 << 8 },
	/* _CHAN_24  */{ IMXDPUV1_SHDLD_FETCHLAYER1, IMXDPUV1_SUB_5 << 8 },
	/* _CHAN_25  */{ IMXDPUV1_SHDLD_FETCHLAYER1, IMXDPUV1_SUB_6 << 8 },
	/* _CHAN_26  */{ IMXDPUV1_SHDLD_FETCHLAYER1, IMXDPUV1_SUB_7 << 8 },
	/* _CHAN_27  */{ IMXDPUV1_SHDLD_FETCHLAYER1, IMXDPUV1_SUB_8 << 8 },
	/* _CHAN_28  */{ IMXDPUV1_SHDLD_FETCHECO0, 0 },
	/* _CHAN_29  */{ IMXDPUV1_SHDLD_FETCHECO1, 0 },
	/* _CHAN_30  */{ IMXDPUV1_SHDLD_FETCHECO2, 0 }
};

#ifdef ENABLE_IMXDPUV1_TRACE_REG
uint32_t _imxdpuv1_read(struct imxdpuv1_soc *imxdpu, uint32_t offset, char *file,
	int line)
{
	uint32_t val = 0;
	val = __raw_readl(imxdpu->base + offset);
	IMXDPUV1_TRACE_REG("%s:%d R reg 0x%08x --> val 0x%08x\n", file, line,
		(uint32_t)offset, (uint32_t)val);
	return val;
}

void _imxdpuv1_write(struct imxdpuv1_soc *imxdpu, uint32_t offset, uint32_t value,
	char *file, int line)
{
	__raw_writel(value, imxdpu->base + offset);
	IMXDPUV1_TRACE_REG("%s:%d W reg 0x%08x <-- val 0x%08x\n", file, line,
		(uint32_t)offset, (uint32_t)value);
}

#endif

void _imxdpuv1_write_block(struct imxdpuv1_soc *imxdpu, uint32_t offset,
	void *values, uint32_t cnt, char *file, int line)
{
	int i;
	uint32_t *dest = (uint32_t *)(imxdpu->base + offset);
	uint32_t *src = (uint32_t *)values;
	IMXDPUV1_TRACE_REG("%s:%d W reg 0x%08x <-- cnt 0x%08x\n", file, line,
		(uint32_t)offset, (uint32_t)cnt);
	for (i = 0; i < cnt; i++) {
		dest[i] = src[i];
		IMXDPUV1_TRACE_REG("%s:%d WB reg 0x%08x <-- val 0x%08x\n", file, line,
		(uint32_t) ((uint64_t)(&dest[i])), (uint32_t)(src[i]));

	}
}

#ifdef ENABLE_IMXDPUV1_TRACE_IRQ_READ
uint32_t _imxdpuv1_read_irq(struct imxdpuv1_soc *imxdpu, uint32_t offset,
	char *file, int line)
{
	uint32_t val = 0;
	val = __raw_readl(imxdpu->base + offset);
	IMXDPUV1_TRACE_IRQ("%s:%d IRQ R reg 0x%08x --> val 0x%08x\n", file, line,
		(uint32_t)offset, (uint32_t)val);
	return val;
}
#endif

#ifdef ENABLE_IMXDPUV1_TRACE_IRQ_WRITE
void _imxdpuv1_write_irq(struct imxdpuv1_soc *imxdpu, uint32_t offset,
	uint32_t value, char *file, int line)
{
	__raw_writel(value, imxdpu->base + offset);
	IMXDPUV1_TRACE_IRQ("%s:%d IRQ W reg 0x%08x <-- val 0x%08x\n", file, line,
		(uint32_t)offset, (uint32_t)value);
}
#endif

/* static prototypes */
int imxdpuv1_dump_channel(int8_t imxdpuv1_id, imxdpuv1_chan_t chan);
static int imxdpuv1_disp_start_shadow_loads(int8_t imxdpuv1_id, int8_t disp);
void imxdpuv1_dump_pixencfg_status(int8_t imxdpuv1_id);
static bool imxdpuv1_is_yuv(uint32_t fmt);
bool imxdpuv1_is_rgb(uint32_t fmt);

/*!
 * Returns IMXDPUV1_TRUE for a valid channel
 *
 * @param	channel to test
 *
 * @return      This function returns IMXDPUV1_TRUE on success or
 *		IMXDPUV1_FALSE if the test fails.
 */
static int is_chan(imxdpuv1_chan_t chan)
{
	imxdpuv1_chan_idx_t chan_idx = get_channel_idx(chan);

	if ((chan_idx >= IMXDPUV1_CHAN_IDX_IN_FIRST) &&
		(chan_idx < IMXDPUV1_CHAN_IDX_IN_MAX))
		return IMXDPUV1_TRUE;
	if ((chan_idx >= IMXDPUV1_CHAN_IDX_OUT_FIRST) &&
		(chan_idx < IMXDPUV1_CHAN_IDX_OUT_MAX))
		return IMXDPUV1_TRUE;
	return IMXDPUV1_FALSE;
}

/*!
 * Returns IMXDPUV1_TRUE for a valid store channel
 *
 * @param	channel to test
 *
 * @return      This function returns IMXDPUV1_TRUE on success or
 *		IMXDPUV1_FALSE if the test fails.
 */
static int is_store_chan(imxdpuv1_chan_t chan)
{
#ifdef IMXDPUV1_VERSION_0
	imxdpuv1_id_t blk_id = get_channel_blk(chan);
	if ((blk_id == IMXDPUV1_ID_STORE4) || (blk_id == IMXDPUV1_ID_STORE4))
		return IMXDPUV1_TRUE;
#endif
	return IMXDPUV1_FALSE;
}

/*!
 * Returns IMXDPUV1_TRUE for a valid fetch channel
 *
 * @param	channel to test
 *
 * @return      This function returns IMXDPUV1_TRUE on success or
 *		IMXDPUV1_FALSE if the test fails.
 */
static int is_fetch_eco_chan(imxdpuv1_chan_t chan)
{
	imxdpuv1_id_t blk_id = get_channel_blk(chan);
	if ((blk_id == IMXDPUV1_ID_FETCHECO0) ||
		(blk_id == IMXDPUV1_ID_FETCHECO1) ||
		(blk_id == IMXDPUV1_ID_FETCHECO2))
		return IMXDPUV1_TRUE;
	return IMXDPUV1_FALSE;
}

/*!
 * Returns IMXDPUV1_TRUE for a valid fetch decode channel
 *
 * @param	channel to test
 *
 * @return      This function returns IMXDPUV1_TRUE on success or
 *		IMXDPUV1_FALSE if the test fails.
 */
static int is_fetch_decode_chan(imxdpuv1_chan_t chan)
{
	imxdpuv1_id_t blk_id = get_channel_blk(chan);
	if ((blk_id == IMXDPUV1_ID_FETCHDECODE0) ||
		(blk_id == IMXDPUV1_ID_FETCHDECODE1)
#ifdef IMXDPUV1_VERSION_0
	    || (blk_id == IMXDPUV1_ID_FETCHDECODE2)
	    || (blk_id == IMXDPUV1_ID_FETCHDECODE3)
#endif
	    )
	    return IMXDPUV1_TRUE;
	return IMXDPUV1_FALSE;
}

/*!
 * Returns IMXDPUV1_TRUE if a fetch channel has an eco fetch
 *
 * @param	channel to test
 *
 * @return      This function returns IMXDPUV1_TRUE on success or
 *		IMXDPUV1_FALSE if the test fails.
 */
static int has_fetch_eco_chan(imxdpuv1_chan_t chan)
{
	imxdpuv1_id_t blk_id = get_channel_blk(chan);
	if ((blk_id == IMXDPUV1_ID_FETCHDECODE0) ||
		(blk_id == IMXDPUV1_ID_FETCHDECODE1) ||
		(blk_id == IMXDPUV1_ID_FETCHWARP2))
		return IMXDPUV1_TRUE;
	return IMXDPUV1_FALSE;
}

/*!
 * Returns IMXDPUV1_TRUE for a valid fetch warp channel
 *
 * @param	channel to test
 *
 * @return      This function returns IMXDPUV1_TRUE on success or
 *		IMXDPUV1_FALSE if the test fails.
 */
static int is_fetch_warp_chan(imxdpuv1_chan_t chan)
{
	imxdpuv1_id_t blk_id = get_channel_blk(chan);
	if ((blk_id == IMXDPUV1_ID_FETCHWARP2))
		return IMXDPUV1_TRUE;
	return IMXDPUV1_FALSE;
}

/*!
 * Returns IMXDPUV1_TRUE for a valid fetch layer channel
 *
 * @param	channel to test
 *
 * @return      This function returns IMXDPUV1_TRUE on success or
 *		IMXDPUV1_FALSE if the test fails.
 */
static int is_fetch_layer_chan(imxdpuv1_chan_t chan)
{
	imxdpuv1_id_t blk_id = get_channel_blk(chan);
	if ((blk_id == IMXDPUV1_ID_FETCHLAYER0)
#ifdef IMXDPUV1_VERSION_0
	    || (blk_id == IMXDPUV1_ID_FETCHLAYER1)
#endif
	    )
	    return IMXDPUV1_TRUE;
	return IMXDPUV1_FALSE;
}

/*!
 * Returns IMXDPUV1_TRUE for a valid layer sub1 channel
 *
 * @param	channel to test
 *
 * @return      This function returns IMXDPUV1_TRUE on success or
 *		IMXDPUV1_FALSE if the test fails.
 */
static int is_fetch_layer_sub_chan1(imxdpuv1_chan_t chan)
{
	imxdpuv1_id_t blk_id = get_channel_blk(chan);
	if ((blk_id == IMXDPUV1_ID_FETCHLAYER0) ||
#ifdef IMXDPUV1_VERSION_0
		(blk_id == IMXDPUV1_ID_FETCHLAYER1) ||
#endif
		(blk_id == IMXDPUV1_ID_FETCHWARP2))
		if (get_channel_sub(chan) == IMXDPUV1_SUB_1)
			return IMXDPUV1_TRUE;
	return IMXDPUV1_FALSE;
}

/*!
 * Returns subindex of a channel
 *
 * @param	channel
 *
 * @return      returns the subindex of a channel
 */
static int imxdpuv1_get_channel_subindex(imxdpuv1_chan_t chan)
{
	switch (get_channel_sub(chan)) {
	case IMXDPUV1_SUB_2:
		return 1;
	case IMXDPUV1_SUB_3:
		return 2;
	case IMXDPUV1_SUB_4:
		return 3;
	case IMXDPUV1_SUB_5:
		return 4;
	case IMXDPUV1_SUB_6:
		return 5;
	case IMXDPUV1_SUB_7:
		return 6;
	case IMXDPUV1_SUB_8:
		return 7;
	case IMXDPUV1_SUB_1:
	case IMXDPUV1_SUBWINDOW_NONE:
	default:
		return 0;
	}
}

/*!
 * Returns returns the eco channel for a channel index
 *
 * @param       chan
 *
 * @return      returns number of bits per pixel or zero
 *      	if the format is not matched.
 */
imxdpuv1_chan_t imxdpuv1_get_eco(imxdpuv1_chan_t chan)
{
	switch (get_eco_idx(chan)) {
	case get_channel_idx(IMXDPUV1_CHAN_28):
		return IMXDPUV1_CHAN_28;
	case get_channel_idx(IMXDPUV1_CHAN_29):
		return IMXDPUV1_CHAN_29;
	case get_channel_idx(IMXDPUV1_CHAN_30):
		return IMXDPUV1_CHAN_30;
	default:
		return 0;
	}
}
/*!
 * Returns the start address offset for a given block ID
 *
 * @param	block id
 *
 * @return      This function returns the address offset if the block id
 *		matches a valid block. Otherwise, IMXDPUV1_OFFSET_INVALID
 *		is returned.
 */
uint32_t id2blockoffset(imxdpuv1_id_t block_id)
{
	switch (block_id) {
	/*case IMXDPUV1_ID_NONE:         return IMXDPUV1_NONE_LOCKUNLOCK; */
	case IMXDPUV1_ID_FETCHDECODE9:
		return IMXDPUV1_FETCHDECODE9_LOCKUNLOCK;
#ifdef IMXDPUV1_VERSION_0
	case IMXDPUV1_ID_FETCHPERSP9:
		return IMXDPUV1_FETCHPERSP9_LOCKUNLOCK;
#else
	case IMXDPUV1_ID_FETCHWARP9:
		return IMXDPUV1_FETCHWARP9_LOCKUNLOCK;
#endif
	case IMXDPUV1_ID_FETCHECO9:
		return IMXDPUV1_FETCHECO9_LOCKUNLOCK;
	case IMXDPUV1_ID_ROP9:
		return IMXDPUV1_ROP9_LOCKUNLOCK;
	case IMXDPUV1_ID_CLUT9:
		return IMXDPUV1_CLUT9_LOCKUNLOCK;
	case IMXDPUV1_ID_MATRIX9:
		return IMXDPUV1_MATRIX9_LOCKUNLOCK;
	case IMXDPUV1_ID_HSCALER9:
		return IMXDPUV1_HSCALER9_LOCKUNLOCK;
	case IMXDPUV1_ID_VSCALER9:
		return IMXDPUV1_VSCALER9_LOCKUNLOCK;
	case IMXDPUV1_ID_FILTER9:
		return IMXDPUV1_FILTER9_LOCKUNLOCK;
	case IMXDPUV1_ID_BLITBLEND9:
		return IMXDPUV1_BLITBLEND9_LOCKUNLOCK;
	case IMXDPUV1_ID_STORE9:
		return IMXDPUV1_STORE9_LOCKUNLOCK;
	case IMXDPUV1_ID_CONSTFRAME0:
		return IMXDPUV1_CONSTFRAME0_LOCKUNLOCK;
	case IMXDPUV1_ID_EXTDST0:
		return IMXDPUV1_EXTDST0_LOCKUNLOCK;
	case IMXDPUV1_ID_CONSTFRAME4:
		return IMXDPUV1_CONSTFRAME4_LOCKUNLOCK;
	case IMXDPUV1_ID_EXTDST4:
		return IMXDPUV1_EXTDST4_LOCKUNLOCK;
	case IMXDPUV1_ID_CONSTFRAME1:
		return IMXDPUV1_CONSTFRAME1_LOCKUNLOCK;
	case IMXDPUV1_ID_EXTDST1:
		return IMXDPUV1_EXTDST1_LOCKUNLOCK;
	case IMXDPUV1_ID_CONSTFRAME5:
		return IMXDPUV1_CONSTFRAME5_LOCKUNLOCK;
	case IMXDPUV1_ID_EXTDST5:
		return IMXDPUV1_EXTDST5_LOCKUNLOCK;
#ifdef IMXDPUV1_VERSION_0
	case IMXDPUV1_ID_EXTSRC4:
		return IMXDPUV1_EXTSRC4_LOCKUNLOCK;
	case IMXDPUV1_ID_STORE4:
		return IMXDPUV1_STORE4_LOCKUNLOCK;
	case IMXDPUV1_ID_EXTSRC5:
		return IMXDPUV1_EXTSRC5_LOCKUNLOCK;
	case IMXDPUV1_ID_STORE5:
		return IMXDPUV1_STORE5_LOCKUNLOCK;
	case IMXDPUV1_ID_FETCHDECODE2:
		return IMXDPUV1_FETCHDECODE2_LOCKUNLOCK;
	case IMXDPUV1_ID_FETCHDECODE3:
		return IMXDPUV1_FETCHDECODE3_LOCKUNLOCK;
#endif
	case IMXDPUV1_ID_FETCHWARP2:
		return IMXDPUV1_FETCHWARP2_LOCKUNLOCK;
	case IMXDPUV1_ID_FETCHECO2:
		return IMXDPUV1_FETCHECO2_LOCKUNLOCK;
	case IMXDPUV1_ID_FETCHDECODE0:
		return IMXDPUV1_FETCHDECODE0_LOCKUNLOCK;
	case IMXDPUV1_ID_FETCHECO0:
		return IMXDPUV1_FETCHECO0_LOCKUNLOCK;
	case IMXDPUV1_ID_FETCHDECODE1:
		return IMXDPUV1_FETCHDECODE1_LOCKUNLOCK;
	case IMXDPUV1_ID_FETCHECO1:
		return IMXDPUV1_FETCHECO1_LOCKUNLOCK;
	case IMXDPUV1_ID_FETCHLAYER0:
		return IMXDPUV1_FETCHLAYER0_LOCKUNLOCK;
#ifdef IMXDPUV1_VERSION_0
	case IMXDPUV1_ID_FETCHLAYER1:
		return IMXDPUV1_FETCHLAYER1_LOCKUNLOCK;
	case IMXDPUV1_ID_GAMMACOR4:
		return IMXDPUV1_GAMMACOR4_LOCKUNLOCK;
#endif
	case IMXDPUV1_ID_MATRIX4:
		return IMXDPUV1_MATRIX4_LOCKUNLOCK;
	case IMXDPUV1_ID_HSCALER4:
		return IMXDPUV1_HSCALER4_LOCKUNLOCK;
	case IMXDPUV1_ID_VSCALER4:
		return IMXDPUV1_VSCALER4_LOCKUNLOCK;
#ifdef IMXDPUV1_VERSION_0
	case IMXDPUV1_ID_HISTOGRAM4:
		return IMXDPUV1_HISTOGRAM4_CONTROL;
	case IMXDPUV1_ID_GAMMACOR5:
		return IMXDPUV1_GAMMACOR5_LOCKUNLOCK;
#endif
	case IMXDPUV1_ID_MATRIX5:
		return IMXDPUV1_MATRIX5_LOCKUNLOCK;
	case IMXDPUV1_ID_HSCALER5:
		return IMXDPUV1_HSCALER5_LOCKUNLOCK;
	case IMXDPUV1_ID_VSCALER5:
		return IMXDPUV1_VSCALER5_LOCKUNLOCK;
#ifdef IMXDPUV1_VERSION_0
	case IMXDPUV1_ID_HISTOGRAM5:
		return IMXDPUV1_HISTOGRAM5_CONTROL;
#endif
	case IMXDPUV1_ID_LAYERBLEND0:
		return IMXDPUV1_LAYERBLEND0_LOCKUNLOCK;
	case IMXDPUV1_ID_LAYERBLEND1:
		return IMXDPUV1_LAYERBLEND1_LOCKUNLOCK;
	case IMXDPUV1_ID_LAYERBLEND2:
		return IMXDPUV1_LAYERBLEND2_LOCKUNLOCK;
	case IMXDPUV1_ID_LAYERBLEND3:
		return IMXDPUV1_LAYERBLEND3_LOCKUNLOCK;
#ifdef IMXDPUV1_VERSION_0
	case IMXDPUV1_ID_LAYERBLEND4:
		return IMXDPUV1_LAYERBLEND4_LOCKUNLOCK;
	case IMXDPUV1_ID_LAYERBLEND5:
		return IMXDPUV1_LAYERBLEND5_LOCKUNLOCK;
	case IMXDPUV1_ID_LAYERBLEND6:
		return IMXDPUV1_LAYERBLEND6_LOCKUNLOCK;
	case IMXDPUV1_ID_EXTSRC0:
		return IMXDPUV1_EXTSRC0_LOCKUNLOCK;
	case IMXDPUV1_ID_EXTSRC1:
		return IMXDPUV1_EXTSRC1_LOCKUNLOCK;
#endif
	case IMXDPUV1_ID_DISENGCFG:
		return IMXDPUV1_DISENGCFG_LOCKUNLOCK0;
	case IMXDPUV1_ID_FRAMEGEN0:
		return IMXDPUV1_FRAMEGEN0_LOCKUNLOCK;
	case IMXDPUV1_ID_MATRIX0:
		return IMXDPUV1_MATRIX0_LOCKUNLOCK;
	case IMXDPUV1_ID_GAMMACOR0:
		return IMXDPUV1_GAMMACOR0_LOCKUNLOCK;
	case IMXDPUV1_ID_DITHER0:
		return IMXDPUV1_DITHER0_LOCKUNLOCK;
	case IMXDPUV1_ID_TCON0:
		return IMXDPUV1_TCON0_LOCKUNLOCK;
	case IMXDPUV1_ID_SIG0:
		return IMXDPUV1_SIG0_LOCKUNLOCK;
	case IMXDPUV1_ID_FRAMEGEN1:
		return IMXDPUV1_FRAMEGEN1_LOCKUNLOCK;
	case IMXDPUV1_ID_MATRIX1:
		return IMXDPUV1_MATRIX1_LOCKUNLOCK;
	case IMXDPUV1_ID_GAMMACOR1:
		return IMXDPUV1_GAMMACOR1_LOCKUNLOCK;
	case IMXDPUV1_ID_DITHER1:
		return IMXDPUV1_DITHER1_LOCKUNLOCK;
	case IMXDPUV1_ID_TCON1:
		return IMXDPUV1_TCON1_LOCKUNLOCK;
	case IMXDPUV1_ID_SIG1:
		return IMXDPUV1_SIG1_LOCKUNLOCK;
#ifdef IMXDPUV1_VERSION_0
	case IMXDPUV1_ID_FRAMECAP4:
		return IMXDPUV1_FRAMECAP4_LOCKUNLOCK;
	case IMXDPUV1_ID_FRAMECAP5:
		return IMXDPUV1_FRAMECAP5_LOCKUNLOCK;
#endif
	default:
		return IMXDPUV1_OFFSET_INVALID;
	}
}

/*!
 * Returns the start address offset for the dynamic configuraiton for
 * a given block ID
 *
 * @param	block id
 *
 * @return      This function returns the address offset if the block id
 *		matches a valid block. Otherwise, IMXDPUV1_OFFSET_INVALID
 *		is returned.
 */
uint32_t id2dynamicoffset(imxdpuv1_id_t block_id)
{
	switch (block_id) {
	case IMXDPUV1_ID_FETCHDECODE9:
		return IMXDPUV1_PIXENGCFG_FETCHDECODE9_DYNAMIC;

#ifdef IMXDPUV1_VERSION_0
	case IMXDPUV1_ID_FETCHPERSP9:
		return IMXDPUV1_PIXENGCFG_FETCHPERSP9_DYNAMIC;
#else
	case IMXDPUV1_ID_FETCHWARP9:
		return IMXDPUV1_PIXENGCFG_FETCHWARP9_DYNAMIC;
#endif
	case IMXDPUV1_ID_ROP9:
		return IMXDPUV1_PIXENGCFG_ROP9_DYNAMIC;
	case IMXDPUV1_ID_CLUT9:
		return IMXDPUV1_PIXENGCFG_CLUT9_DYNAMIC;
	case IMXDPUV1_ID_MATRIX9:
		return IMXDPUV1_PIXENGCFG_MATRIX9_DYNAMIC;
	case IMXDPUV1_ID_HSCALER9:
		return IMXDPUV1_PIXENGCFG_HSCALER9_DYNAMIC;
	case IMXDPUV1_ID_VSCALER9:
		return IMXDPUV1_PIXENGCFG_VSCALER9_DYNAMIC;
	case IMXDPUV1_ID_FILTER9:
		return IMXDPUV1_PIXENGCFG_FILTER9_DYNAMIC;
	case IMXDPUV1_ID_BLITBLEND9:
		return IMXDPUV1_PIXENGCFG_BLITBLEND9_DYNAMIC;
	case IMXDPUV1_ID_STORE9:
		return IMXDPUV1_PIXENGCFG_STORE9_DYNAMIC;
	case IMXDPUV1_ID_EXTDST0:
		return IMXDPUV1_PIXENGCFG_EXTDST0_DYNAMIC;
	case IMXDPUV1_ID_EXTDST4:
		return IMXDPUV1_PIXENGCFG_EXTDST4_DYNAMIC;
	case IMXDPUV1_ID_EXTDST1:
		return IMXDPUV1_PIXENGCFG_EXTDST1_DYNAMIC;
#ifdef IMXDPUV1_VERSION_0
	case IMXDPUV1_ID_EXTDST5:
		return IMXDPUV1_PIXENGCFG_EXTDST5_DYNAMIC;
	case IMXDPUV1_ID_STORE4:
		return IMXDPUV1_PIXENGCFG_STORE4_DYNAMIC;
	case IMXDPUV1_ID_STORE5:
		return IMXDPUV1_PIXENGCFG_STORE5_DYNAMIC;
	case IMXDPUV1_ID_FETCHDECODE2:
		return IMXDPUV1_PIXENGCFG_FETCHDECODE2_DYNAMIC;
	case IMXDPUV1_ID_FETCHDECODE3:
		return IMXDPUV1_PIXENGCFG_FETCHDECODE3_DYNAMIC;
#endif
	case IMXDPUV1_ID_FETCHWARP2:
		return IMXDPUV1_PIXENGCFG_FETCHWARP2_DYNAMIC;
	case IMXDPUV1_ID_FETCHDECODE0:
		return IMXDPUV1_PIXENGCFG_FETCHDECODE0_DYNAMIC;
	case IMXDPUV1_ID_FETCHDECODE1:
		return IMXDPUV1_PIXENGCFG_FETCHDECODE1_DYNAMIC;
#ifdef IMXDPUV1_VERSION_0
	case IMXDPUV1_ID_GAMMACOR4:
		return IMXDPUV1_PIXENGCFG_GAMMACOR4_DYNAMIC;
#endif
	case IMXDPUV1_ID_MATRIX4:
		return IMXDPUV1_PIXENGCFG_MATRIX4_DYNAMIC;
	case IMXDPUV1_ID_HSCALER4:
		return IMXDPUV1_PIXENGCFG_HSCALER4_DYNAMIC;
	case IMXDPUV1_ID_VSCALER4:
		return IMXDPUV1_PIXENGCFG_VSCALER4_DYNAMIC;
#ifdef IMXDPUV1_VERSION_0
	case IMXDPUV1_ID_HISTOGRAM4:
		return IMXDPUV1_PIXENGCFG_HISTOGRAM4_DYNAMIC;
	case IMXDPUV1_ID_GAMMACOR5:
		return IMXDPUV1_PIXENGCFG_GAMMACOR5_DYNAMIC;
#endif
	case IMXDPUV1_ID_MATRIX5:
		return IMXDPUV1_PIXENGCFG_MATRIX5_DYNAMIC;
	case IMXDPUV1_ID_HSCALER5:
		return IMXDPUV1_PIXENGCFG_HSCALER5_DYNAMIC;
	case IMXDPUV1_ID_VSCALER5:
		return IMXDPUV1_PIXENGCFG_VSCALER5_DYNAMIC;
#ifdef IMXDPUV1_VERSION_0
	case IMXDPUV1_ID_HISTOGRAM5:
		return IMXDPUV1_PIXENGCFG_HISTOGRAM5_DYNAMIC;
#endif
	case IMXDPUV1_ID_LAYERBLEND0:
		return IMXDPUV1_PIXENGCFG_LAYERBLEND0_DYNAMIC;
	case IMXDPUV1_ID_LAYERBLEND1:
		return IMXDPUV1_PIXENGCFG_LAYERBLEND1_DYNAMIC;
	case IMXDPUV1_ID_LAYERBLEND2:
		return IMXDPUV1_PIXENGCFG_LAYERBLEND2_DYNAMIC;
	case IMXDPUV1_ID_LAYERBLEND3:
		return IMXDPUV1_PIXENGCFG_LAYERBLEND3_DYNAMIC;
#ifdef IMXDPUV1_VERSION_0
	case IMXDPUV1_ID_LAYERBLEND4:
		return IMXDPUV1_PIXENGCFG_LAYERBLEND4_DYNAMIC;
	case IMXDPUV1_ID_LAYERBLEND5:
		return IMXDPUV1_PIXENGCFG_LAYERBLEND5_DYNAMIC;
	case IMXDPUV1_ID_LAYERBLEND6:
		return IMXDPUV1_PIXENGCFG_LAYERBLEND6_DYNAMIC;
#endif
	default:
		return IMXDPUV1_OFFSET_INVALID;
	}
}

/*!
 * Returns the start address offset for a given shadow index
 *
 * @param	block id
 *
 * @return      This function returns the address offset if the shadow
 *		index matches a valid block. Otherwise, IMXDPUV1_OFFSET_INVALID
 *		is returned.
 */
imxdpuv1_chan_t shadowindex2channel(imxdpuv1_shadow_load_index_t shadow_index)
{
	switch (shadow_index) {
	case IMXDPUV1_SHDLD_IDX_CHAN_00:
		return IMXDPUV1_CHAN_00;
	case IMXDPUV1_SHDLD_IDX_CHAN_01:
		return IMXDPUV1_CHAN_01;
	case IMXDPUV1_SHDLD_IDX_CHAN_02:
		return IMXDPUV1_CHAN_02;
	case IMXDPUV1_SHDLD_IDX_CHAN_03:
		return IMXDPUV1_CHAN_03;
	case IMXDPUV1_SHDLD_IDX_CHAN_04:
		return IMXDPUV1_CHAN_04;
	case IMXDPUV1_SHDLD_IDX_CHAN_05:
		return IMXDPUV1_CHAN_05;
	case IMXDPUV1_SHDLD_IDX_CHAN_06:
		return IMXDPUV1_CHAN_06;
	case IMXDPUV1_SHDLD_IDX_CHAN_07:
		return IMXDPUV1_CHAN_07;
	case IMXDPUV1_SHDLD_IDX_CHAN_08:
		return IMXDPUV1_CHAN_08;
	case IMXDPUV1_SHDLD_IDX_CHAN_09:
		return IMXDPUV1_CHAN_09;
	case IMXDPUV1_SHDLD_IDX_CHAN_10:
		return IMXDPUV1_CHAN_10;
	case IMXDPUV1_SHDLD_IDX_CHAN_11:
		return IMXDPUV1_CHAN_11;
	case IMXDPUV1_SHDLD_IDX_CHAN_12:
		return IMXDPUV1_CHAN_12;
	case IMXDPUV1_SHDLD_IDX_CHAN_13:
		return IMXDPUV1_CHAN_13;
	case IMXDPUV1_SHDLD_IDX_CHAN_14:
		return IMXDPUV1_CHAN_14;
	case IMXDPUV1_SHDLD_IDX_CHAN_15:
		return IMXDPUV1_CHAN_15;
	case IMXDPUV1_SHDLD_IDX_CHAN_16:
		return IMXDPUV1_CHAN_16;
	case IMXDPUV1_SHDLD_IDX_CHAN_17:
		return IMXDPUV1_CHAN_17;
	case IMXDPUV1_SHDLD_IDX_CHAN_18:
		return IMXDPUV1_CHAN_18;
	case IMXDPUV1_SHDLD_IDX_CHAN_19:
		return IMXDPUV1_CHAN_19;
	case IMXDPUV1_SHDLD_IDX_CHAN_20:
		return IMXDPUV1_CHAN_20;
	case IMXDPUV1_SHDLD_IDX_CHAN_21:
		return IMXDPUV1_CHAN_21;
	case IMXDPUV1_SHDLD_IDX_CHAN_22:
		return IMXDPUV1_CHAN_22;
	case IMXDPUV1_SHDLD_IDX_CHAN_23:
		return IMXDPUV1_CHAN_23;
	case IMXDPUV1_SHDLD_IDX_CHAN_24:
		return IMXDPUV1_CHAN_24;
	case IMXDPUV1_SHDLD_IDX_CHAN_25:
		return IMXDPUV1_CHAN_25;
	case IMXDPUV1_SHDLD_IDX_CHAN_26:
		return IMXDPUV1_CHAN_26;
	case IMXDPUV1_SHDLD_IDX_CHAN_27:
		return IMXDPUV1_CHAN_27;
	case IMXDPUV1_SHDLD_IDX_CHAN_28:
		return IMXDPUV1_CHAN_28;
	case IMXDPUV1_SHDLD_IDX_CHAN_29:
		return IMXDPUV1_CHAN_29;
	case IMXDPUV1_SHDLD_IDX_CHAN_30:
		return IMXDPUV1_CHAN_30;
	default:
		return IMXDPUV1_CHANNEL_INVALID;
	}
}


/*!
 * This function returns the pointer to the imxdpu structutre
 *
 * @param	imxdpuv1_id	id of the diplay unit
 * @param       disp		id of the diplay output pipe
 *
 * @return      This function returns the pointer to the imxdpu structutre
 *      	return a NULL pointer for a failure.
 */
struct imxdpuv1_soc *imxdpuv1_get_soc(int8_t imxdpuv1_id)
{
	/* imxdpuv1_id starts from 0 */
	if (!((imxdpuv1_id >= 0) && (imxdpuv1_id < IMXDPUV1_MAX_NUM))) {
		return NULL;
	}
	return &(imxdpuv1_array[imxdpuv1_id]);
}

/*!
 * This function enables the interrupt for the specified interrupt line.
 * The interrupt lines are defined in imxdpuv1_events.h.
 *
 * @param	imxdpu		imxdpu instance
 * @param       irq     	Interrupt line to enable interrupt for.
 *
 * @return      This function returns 0 on success or negative error code on
 *      	fail.
 */
int imxdpuv1_enable_irq(int8_t imxdpuv1_id, uint32_t irq)
{
	int ret = 0;
	struct imxdpuv1_soc *imxdpu;

	IMXDPUV1_TRACE("%s()\n", __func__);

#ifdef DEBUG_IMXDPUV1_IRQ_ERROR
	if (irq == 0)
		panic("Trying to enable irq 0!");
#endif
	/* imxdpuv1_id starts from 0 */
	if (!((imxdpuv1_id >= 0) && (imxdpuv1_id < IMXDPUV1_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpuv1_array[imxdpuv1_id];

	imxdpuv1_clear_irq(imxdpuv1_id, irq);
	if (irq < IMXDPUV1_INTERRUPT_MAX) {
		if (irq < 32) {
			imxdpu->enabled_int[0] |= INTSTAT0_BIT(irq);
			imxdpuv1_write_irq(imxdpu,
				IMXDPUV1_COMCTRL_USERINTERRUPTENABLE0,
				imxdpu->enabled_int[0]);
		} else if (irq < 64) {
			imxdpu->enabled_int[1] |= INTSTAT1_BIT(irq);
			imxdpuv1_write_irq(imxdpu,
				IMXDPUV1_COMCTRL_USERINTERRUPTENABLE1,
				imxdpu->enabled_int[1]);
#ifdef IMXDPUV1_VERSION_0
		} else {
			imxdpu->enabled_int[2] |= INTSTAT2_BIT(irq);
			imxdpuv1_write_irq(imxdpu,
				IMXDPUV1_COMCTRL_USERINTERRUPTENABLE2,
				imxdpu->enabled_int[2]);
#endif
		}
	} else {
		return -EINVAL;
	}

	return ret;
}

/*!
 * This function disables the interrupt for the specified interrupt line.g
 * The interrupt lines are defined in imxdpuv1_events.h.
 *
 * @param	imxdpu		imxdpu instance
 * @param       irq     	Interrupt line to disable interrupt for.
 *
 */
int imxdpuv1_disable_irq(int8_t imxdpuv1_id, uint32_t irq)
{
	int ret = 0;
	struct imxdpuv1_soc *imxdpu;

	IMXDPUV1_TRACE("%s()\n", __func__);

	/* imxdpuv1_id starts from 0 */
	if (!((imxdpuv1_id >= 0) && (imxdpuv1_id < IMXDPUV1_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpuv1_array[imxdpuv1_id];

	if (irq < IMXDPUV1_INTERRUPT_MAX) {
		if (irq < 32) {
			imxdpu->enabled_int[0] &= ~INTSTAT0_BIT(irq);
			imxdpuv1_write_irq(imxdpu,
				IMXDPUV1_COMCTRL_USERINTERRUPTENABLE0,
				imxdpu->enabled_int[0]);
		} else if (irq < 64) {
			imxdpu->enabled_int[1] &= ~INTSTAT1_BIT(irq);
			imxdpuv1_write_irq(imxdpu,
				IMXDPUV1_COMCTRL_USERINTERRUPTENABLE1,
				imxdpu->enabled_int[1]);
#ifdef IMXDPUV1_VERSION_0
		} else {
			imxdpu->enabled_int[2] &= ~INTSTAT2_BIT(irq);
			imxdpuv1_write_irq(imxdpu,
				IMXDPUV1_COMCTRL_USERINTERRUPTENABLE2,
				imxdpu->enabled_int[2]);
#endif
		}
	} else {
		return -EINVAL;
	}

	return ret;
}

/*!
 * This function clears all interrupts.
 *
 * @param	imxdpu		imxdpu instance
 *
 */
int imxdpuv1_clear_all_irqs(int8_t imxdpuv1_id)
{
	int ret = 0;
	struct imxdpuv1_soc *imxdpu;

	IMXDPUV1_TRACE("%s()\n", __func__);

	/* imxdpuv1_id starts from 0 */
	if (!((imxdpuv1_id >= 0) && (imxdpuv1_id < IMXDPUV1_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpuv1_array[imxdpuv1_id];

	imxdpuv1_write_irq(imxdpu,
		IMXDPUV1_COMCTRL_USERINTERRUPTCLEAR0,
		IMXDPUV1_COMCTRL_USERINTERRUPTCLEAR0_USERINTERRUPTCLEAR0_MASK);
	imxdpuv1_write_irq(imxdpu,
		IMXDPUV1_COMCTRL_USERINTERRUPTCLEAR1,
		IMXDPUV1_COMCTRL_USERINTERRUPTCLEAR1_USERINTERRUPTCLEAR1_MASK);
#ifdef IMXDPUV1_VERSION_0
	imxdpuv1_write_irq(imxdpu,
		IMXDPUV1_COMCTRL_USERINTERRUPTCLEAR2,
		IMXDPUV1_COMCTRL_USERINTERRUPTCLEAR2_USERINTERRUPTCLEAR2_MASK);
#endif
#if 1
	imxdpuv1_write_irq(imxdpu,
		IMXDPUV1_COMCTRL_INTERRUPTCLEAR0,
		IMXDPUV1_COMCTRL_INTERRUPTCLEAR0_INTERRUPTCLEAR0_MASK);
	imxdpuv1_write_irq(imxdpu,
		IMXDPUV1_COMCTRL_INTERRUPTCLEAR1,
		IMXDPUV1_COMCTRL_INTERRUPTCLEAR1_INTERRUPTCLEAR1_MASK);
#ifdef IMXDPUV1_VERSION_0
	imxdpuv1_write_irq(imxdpu,
		IMXDPUV1_COMCTRL_INTERRUPTCLEAR2,
		IMXDPUV1_COMCTRL_INTERRUPTCLEAR2_INTERRUPTCLEAR2_MASK);
#endif
#endif
	return ret;
}

/*!
 * This function disables all interrupts.
 *
 * @param	imxdpu		imxdpu instance
 *
 */
int imxdpuv1_disable_all_irqs(int8_t imxdpuv1_id)
{
	int ret = 0;
	struct imxdpuv1_soc *imxdpu;

	IMXDPUV1_TRACE("%s()\n", __func__);

	/* imxdpuv1_id starts from 0 */
	if (!((imxdpuv1_id >= 0) && (imxdpuv1_id < IMXDPUV1_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpuv1_array[imxdpuv1_id];

	imxdpuv1_write_irq(imxdpu, IMXDPUV1_COMCTRL_USERINTERRUPTENABLE0, 0);
	imxdpuv1_write_irq(imxdpu, IMXDPUV1_COMCTRL_USERINTERRUPTENABLE1, 0);
#ifdef IMXDPUV1_VERSION_0
	imxdpuv1_write_irq(imxdpu, IMXDPUV1_COMCTRL_USERINTERRUPTENABLE2, 0);
#endif

#if 1
	imxdpuv1_write_irq(imxdpu, IMXDPUV1_COMCTRL_INTERRUPTENABLE0, 0);
	imxdpuv1_write_irq(imxdpu, IMXDPUV1_COMCTRL_INTERRUPTENABLE1, 0);
#ifdef IMXDPUV1_VERSION_0
	imxdpuv1_write_irq(imxdpu, IMXDPUV1_COMCTRL_INTERRUPTENABLE2, 0);
#endif
#endif

	imxdpu->enabled_int[0] = 0;
	imxdpu->enabled_int[1] = 0;
#ifdef IMXDPUV1_VERSION_0
	imxdpu->enabled_int[2] = 0;
#endif
	return ret;
}

/*!
 * This function clears the interrupt for the specified interrupt line.
 * The interrupt lines are defined in ipu_irq_line enum.
 *
 * @param	imxdpu  	imxdpu instance
 * @param       irq     	Interrupt line to clear interrupt for.
 *
 */
int imxdpuv1_clear_irq(int8_t imxdpuv1_id, uint32_t irq)
{
	int ret = 0;
	struct imxdpuv1_soc *imxdpu;

	IMXDPUV1_TRACE("%s()\n", __func__);

	/* imxdpuv1_id starts from 0 */
	if (!((imxdpuv1_id >= 0) && (imxdpuv1_id < IMXDPUV1_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpuv1_array[imxdpuv1_id];

	if (irq < IMXDPUV1_INTERRUPT_MAX) {
		if (irq < 32) {
			imxdpuv1_write_irq(imxdpu,
				IMXDPUV1_COMCTRL_USERINTERRUPTCLEAR0,
				1U << irq);
		}
		if (irq < 64) {
			imxdpuv1_write_irq(imxdpu,
				IMXDPUV1_COMCTRL_USERINTERRUPTCLEAR1,
				1U << (irq - 32));
#ifdef IMXDPUV1_VERSION_0
		} else {
			imxdpuv1_write_irq(imxdpu,
				IMXDPUV1_COMCTRL_USERINTERRUPTCLEAR2,
				1U << (irq - 64));
#endif
		}
	} else {
		return -EINVAL;
	}

	return ret;
}

/*!
 * This function initializes the imxdpu interrupts
 *
 * @param	imxdpu  	imxdpu instance
 *
 */
int imxdpuv1_init_irqs(int8_t imxdpuv1_id)
{
	int ret = 0;
	struct imxdpuv1_soc *imxdpu;

	IMXDPUV1_TRACE("%s()\n", __func__);

	/* imxdpuv1_id starts from 0 */
	if (!((imxdpuv1_id >= 0) && (imxdpuv1_id < IMXDPUV1_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpuv1_array[imxdpuv1_id];

	imxdpuv1_disable_all_irqs(imxdpuv1_id);
	imxdpuv1_clear_all_irqs(imxdpuv1_id);

	/* Set all irq to user mode */
	imxdpuv1_write_irq(imxdpu,
		IMXDPUV1_COMCTRL_USERINTERRUPTMASK0,
		IMXDPUV1_COMCTRL_USERINTERRUPTMASK0_USERINTERRUPTMASK0_MASK);
	imxdpuv1_write_irq(imxdpu,
		IMXDPUV1_COMCTRL_USERINTERRUPTMASK1,
		IMXDPUV1_COMCTRL_USERINTERRUPTMASK1_USERINTERRUPTMASK1_MASK);
#ifdef IMXDPUV1_VERSION_0
	imxdpuv1_write_irq(imxdpu,
		IMXDPUV1_COMCTRL_USERINTERRUPTMASK2,
		IMXDPUV1_COMCTRL_USERINTERRUPTMASK2_USERINTERRUPTMASK2_MASK);
#endif
	/* enable needed interupts */
	imxdpuv1_enable_irq(imxdpuv1_id, IMXDPUV1_EXTDST0_SHDLOAD_IRQ);
	imxdpuv1_enable_irq(imxdpuv1_id, IMXDPUV1_EXTDST1_SHDLOAD_IRQ);
	imxdpuv1_enable_irq(imxdpuv1_id, IMXDPUV1_EXTDST0_FRAMECOMPLETE_IRQ);
	imxdpuv1_enable_irq(imxdpuv1_id, IMXDPUV1_EXTDST1_FRAMECOMPLETE_IRQ);

#ifdef IMXDPUV1_VERSION_0
	imxdpuv1_enable_irq(imxdpuv1_id, IMXDPUV1_STORE4_SHDLOAD_IRQ);
	imxdpuv1_enable_irq(imxdpuv1_id, IMXDPUV1_STORE5_SHDLOAD_IRQ);
	imxdpuv1_enable_irq(imxdpuv1_id, IMXDPUV1_STORE4_SEQCOMPLETE_IRQ);
	imxdpuv1_enable_irq(imxdpuv1_id, IMXDPUV1_STORE5_SEQCOMPLETE_IRQ);
	imxdpuv1_enable_irq(imxdpuv1_id, IMXDPUV1_STORE4_FRAMECOMPLETE_IRQ);
	imxdpuv1_enable_irq(imxdpuv1_id, IMXDPUV1_STORE5_FRAMECOMPLETE_IRQ);
#endif
	/* enable the frame interrupts as IMXDPUV1_IRQF_ONESHOT */
	imxdpuv1_enable_irq(imxdpuv1_id, IMXDPUV1_FRAMEGEN0_INT0_IRQ);
	imxdpuv1_enable_irq(imxdpuv1_id, IMXDPUV1_FRAMEGEN1_INT0_IRQ);

	imxdpuv1_enable_irq(imxdpuv1_id, IMXDPUV1_COMCTRL_SW0_IRQ);
	imxdpuv1_enable_irq(imxdpuv1_id, IMXDPUV1_COMCTRL_SW1_IRQ);

	imxdpuv1_enable_irq(imxdpuv1_id, IMXDPUV1_DISENGCFG_SHDLOAD0_IRQ);
	imxdpuv1_enable_irq(imxdpuv1_id, IMXDPUV1_DISENGCFG_SHDLOAD1_IRQ);

	IMXDPUV1_TRACE("%s() enabled_int[0] 0x%08x\n", __func__,
		imxdpu->enabled_int[0]);
	IMXDPUV1_TRACE("%s() enabled_int[1] 0x%08x\n", __func__,
		imxdpu->enabled_int[1]);
#ifdef IMXDPUV1_VERSION_0
	IMXDPUV1_TRACE("%s() enabled_int[2] 0x%08x\n", __func__,
		imxdpu->enabled_int[2]);
#endif
	return ret;
}

/*!
 * This function checks pending shadow loads
 *
 * @param	imxdpuv1_id	id of the diplay unit
 * @param       disp		id of the diplay output pipe
 *
 * @return      This function returns 0 on success or negative error code on
 *      	fail.
 */
int imxdpuv1_disp_check_shadow_loads(int8_t imxdpuv1_id, int8_t disp)
{
	int ret = 0;
	uint32_t addr_extdst = IMXDPUV1_OFFSET_INVALID; /* address for extdst */
	uint32_t extdst = 0;
	uint32_t extdst_stat = 0;
	uint32_t fgen = 1;
	uint32_t fgen_stat = 0;
	uint32_t sub = 0;
	uint32_t sub_stat = 0;
	uint32_t stat;

	int32_t i;

	struct imxdpuv1_soc *imxdpu;

	IMXDPUV1_TRACE_IRQ("%s()\n", __func__);

	if (!((imxdpuv1_id >= 0) && (imxdpuv1_id < IMXDPUV1_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpuv1_array[imxdpuv1_id];

	stat = imxdpuv1_read_irq(imxdpu, IMXDPUV1_COMCTRL_USERINTERRUPTSTATUS0);
	if (disp == 0) {
		addr_extdst = IMXDPUV1_PIXENGCFG_EXTDST0_REQUEST;
		if (stat & IMXDPUV1_DISENGCFG_SHDLOAD0_IRQ) {
			fgen = 0;
		}
	} else if (disp == 1) {
		addr_extdst = IMXDPUV1_PIXENGCFG_EXTDST1_REQUEST;
		if (stat & IMXDPUV1_DISENGCFG_SHDLOAD1_IRQ) {
			fgen = 0;
		}
	} else {
		return -EINVAL;
	}

	sub |= (imxdpuv1_read(imxdpu, IMXDPUV1_FETCHLAYER0_TRIGGERENABLE)) & 0xff;
#ifdef IMXDPUV1_VERSION_0
	sub |= (imxdpuv1_read(imxdpu, IMXDPUV1_FETCHLAYER1_TRIGGERENABLE) << 8) & 0xff00;
#endif
	sub |= (imxdpuv1_read(imxdpu, IMXDPUV1_FETCHWARP2_TRIGGERENABLE) << 16) & 0xff0000;
	extdst = imxdpuv1_read(imxdpu, addr_extdst);

	/* this loop may need to be optimized */
	for (i = 0; i < IMXDPUV1_SHDLD_IDX_CHAN_00; i++) {
		if (imxdpu->shadow_load_state[disp][i].state.complete) {
			if (imxdpu->shadow_load_state[disp][i].state.trys > 0) {
				IMXDPUV1_TRACE_IRQ
					("shadow index complete after retry: index %d trys %d\n",
					i,
					imxdpu->shadow_load_state[disp][i].
					state.trys);
			} else {
				IMXDPUV1_TRACE_IRQ("shadow index complete: index %d\n", i);
			}
			imxdpu->shadow_load_state[disp][i].word = 0;
		} else if (imxdpu->shadow_load_state[disp][i].state.processing) {
			if (i > IMXDPUV1_SHDLD_IDX_CONST1) {
				if (!(extdst & trigger_list[i].extdst) && !fgen) {
					imxdpu->shadow_load_state[disp][i].
						state.complete = 1;
				} else {
					extdst_stat |= trigger_list[i].extdst;
					fgen_stat |= 1 << i;
				}
			} else if (!(extdst & trigger_list[i].extdst)) {
				imxdpu->shadow_load_state[disp][i].
					state.complete = 1;
			} else {
				imxdpu->shadow_load_state[disp][i].state.trys++;
				extdst |= trigger_list[i].extdst;
				IMXDPUV1_TRACE_IRQ
					("shadow index retry: index %d trys %d\n",
					i,
					imxdpu->shadow_load_state[disp][i].
					state.trys);
			}
		}
	}


	for (i = IMXDPUV1_SHDLD_IDX_CHAN_00; i < IMXDPUV1_SHDLD_IDX_MAX; i++) {
		if (imxdpu->shadow_load_state[disp][i].state.complete) {

			if (imxdpu->shadow_load_state[disp][i].state.trys > 0) {
				IMXDPUV1_TRACE_IRQ
					("shadow index complete after retry: index %d trys %d\n",
					i,
					imxdpu->shadow_load_state[disp][i].
					state.trys);
			} else {
				IMXDPUV1_TRACE_IRQ("shadow index complete: index %d\n", i);
			}
			imxdpu->shadow_load_state[disp][i].word = 0;
		} else if (imxdpu->shadow_load_state[disp][i].state.processing) {
			/* fetch layer and fetchwarp */
			if ((trigger_list[i].extdst != 0) &&
				(trigger_list[i].sub != 0)) {
				if (!(extdst & trigger_list[i].extdst) &&
					!(sub & trigger_list[i].sub)) {
					imxdpu->shadow_load_state[disp][i].
						state.complete = 1;
				} else {
					extdst_stat |= trigger_list[i].extdst;
					sub_stat |= trigger_list[i].sub;
				}
			} else if (!(extdst & trigger_list[i].extdst)) {
				imxdpu->shadow_load_state[disp][i].
					state.complete = 1;
			} else {
				imxdpu->shadow_load_state[disp][i].state.trys++;
				extdst_stat |= trigger_list[i].extdst;
				IMXDPUV1_TRACE_IRQ
					("shadow index retry: index %d trys %d\n",
					i,
					imxdpu->shadow_load_state[disp][i].
					state.trys);
			}
		}
	}

	if ((extdst_stat == 0) && (sub_stat == 0) && (fgen_stat == 0)) {
		/* clear interrupt */
		IMXDPUV1_TRACE_IRQ("shadow requests are complete.\n");
	} else {
		IMXDPUV1_TRACE_IRQ
			("shadow requests are not complete: extdst 0x%08x, sub 0x%08x, fgen 0x%08x\n",
			extdst, sub, fgen);
		IMXDPUV1_TRACE_IRQ
			("shadow requests are not complete: extdst_stat 0x%08x, sub_stat 0x%08x, fgen_stat 0x%08x\n",
			extdst_stat, sub_stat, fgen_stat);
	}

	return ret;
}

/*!
 * This function starts pending shadow loads
 *
 * @param	imxdpuv1_id	id of the diplay unit
 * @param       disp		id of the diplay output pipe
 *
 * @return      This function returns 0 on success or negative error code on
 *      	fail.
 */
static int imxdpuv1_disp_start_shadow_loads(int8_t imxdpuv1_id, int8_t disp)
{
	int ret = 0;
	uint32_t addr_extdst;   /* address for extdst */
	uint32_t addr_fgen; /* address for frame generator */
	uint32_t extdst = 0;
	uint32_t fgen = 0;
	uint32_t sub = 0;
	int32_t i;

	struct imxdpuv1_soc *imxdpu;

	IMXDPUV1_TRACE_IRQ("%s()\n", __func__);

	if (!((imxdpuv1_id >= 0) && (imxdpuv1_id < IMXDPUV1_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpuv1_array[imxdpuv1_id];

	if (disp == 0) {
		addr_fgen = IMXDPUV1_FRAMEGEN0_FGSLR;
		addr_extdst = IMXDPUV1_PIXENGCFG_EXTDST0_REQUEST;

	} else if (disp == 1) {
		addr_fgen = IMXDPUV1_FRAMEGEN1_FGSLR;
		addr_extdst = IMXDPUV1_PIXENGCFG_EXTDST1_REQUEST;
	} else {
		return -EINVAL;
	}

	/* this loop may need to be optimized */
	for (i = 0; i < IMXDPUV1_SHDLD_IDX_CHAN_00; i++) {
		if (imxdpu->shadow_load_state[disp][i].state.request &&
			(imxdpu->shadow_load_state[disp][i].state.processing == 0)) {
			imxdpu->shadow_load_state[disp][i].state.processing = 1;
			extdst |= trigger_list[i].extdst;
			/* only trigger frame generator for const frames*/
			if (i >= IMXDPUV1_SHDLD_IDX_CONST0) {
				fgen |= 1;
			}
		}
	}
	for (i = IMXDPUV1_SHDLD_IDX_CHAN_00; i < IMXDPUV1_SHDLD_IDX_MAX; i++) {
		if (imxdpu->shadow_load_state[disp][i].state.request &&
			(imxdpu->shadow_load_state[disp][i].state.processing == 0)) {
			imxdpu->shadow_load_state[disp][i].state.processing = 1;
			/*todo: need a completion handler */
			extdst |= trigger_list[i].extdst;
			sub |= trigger_list[i].sub;
		}
	}

	if (sub) {
		IMXDPUV1_TRACE_IRQ("Fetch layer shadow request 0x%08x\n", sub);
		if (sub & 0xff) {   /* FETCHLAYER0 */
			imxdpuv1_write(imxdpu, IMXDPUV1_FETCHLAYER0_TRIGGERENABLE,
				sub & 0xff);
		}
#ifdef IMXDPUV1_VERSION_0
		if (sub & 0xff00) { /* FETCHLAYER1 */
			imxdpuv1_write(imxdpu, IMXDPUV1_FETCHLAYER1_TRIGGERENABLE,
				(sub >> 8) & 0xff);
		}
#endif
		if (sub & 0xff0000) {   /* FETCHWARP2 */
			imxdpuv1_write(imxdpu, IMXDPUV1_FETCHWARP2_TRIGGERENABLE,
				(sub >> 16) & 0xff);
		}
	}

	if (extdst) {
		IMXDPUV1_TRACE_IRQ("Extdst shadow request  0x%08x\n", extdst);
		imxdpuv1_write(imxdpu, addr_extdst, extdst);
	}

	if (fgen) {
		IMXDPUV1_TRACE_IRQ("Fgen shadow request  0x%08x\n", fgen);
		imxdpuv1_write(imxdpu, addr_fgen, fgen);
	}

	return ret;
}

/*!
 * This function handles the VYNC interrupt for a display
 *
 * @param	imxdpu		imxdpu instance
 * @param	disp		display index
 *
 */
static void imxdpuv1_disp_vsync_handler(int8_t imxdpuv1_id, int8_t disp)
{
	IMXDPUV1_TRACE_IRQ("%s()\n", __func__);

	if (!((imxdpuv1_id >= 0) && (imxdpuv1_id < IMXDPUV1_MAX_NUM))) {
		return;
	}
	if (!((disp == 0) || (disp == 1)))
		return;

	/* send notifications
	   shadow load finished
	 */

	imxdpuv1_disp_start_shadow_loads(imxdpuv1_id, disp);
	imxdpuv1_disp_update_fgen_status(imxdpuv1_id, disp);

	return;

}

/*!
 * This function calls a register handler for an interrupt
 *
 * @param	imxdpu		imxdpu instance
 * @param	irq		interrupt line
 *
 */
static void imxdpuv1_handle_registered_irq(int8_t imxdpuv1_id, int8_t irq)
{
	struct imxdpuv1_soc *imxdpu;

	IMXDPUV1_TRACE_IRQ("%s()\n", __func__);

	if (!((imxdpuv1_id >= 0) && (imxdpuv1_id < IMXDPUV1_MAX_NUM))) {
		return;
	}
	imxdpu = &imxdpuv1_array[imxdpuv1_id];

	if ((irq < 0) || (irq >= IMXDPUV1_INTERRUPT_MAX))
		return;

	if (imxdpu->irq_list[irq].handler == NULL)
		return;

	imxdpu->irq_list[irq].handler(irq, imxdpu->irq_list[irq].data);

	if ((imxdpu->irq_list[irq].flags & IMXDPUV1_IRQF_ONESHOT) != 0) {
		imxdpuv1_disable_irq(imxdpuv1_id, irq);
		imxdpuv1_clear_irq(imxdpuv1_id, irq);
	}
	return;

}

/* todo: this irq handler assumes all irq are ORed together.
     The irqs may be grouped so this function can be
     optimized if that is the case*/
/*!
 * This function processes all IRQs for the IMXDPU
 *
 * @param	data	pointer to the imxdpu structure
 *
 */
int imxdpuv1_handle_irq(int32_t imxdpuv1_id)
{
	uint32_t int_stat[3];
	uint32_t int_temp[3];
	struct imxdpuv1_soc *imxdpu;

	IMXDPUV1_TRACE_IRQ("%s()\n", __func__);


	/* imxdpuv1_id starts from 0 */
	if (!((imxdpuv1_id >= 0) && (imxdpuv1_id < IMXDPUV1_MAX_NUM))) {
		IMXDPUV1_TRACE_IRQ("%s(): invalid imxdpuv1_id\n", __func__);
#ifdef DEBUG_IMXDPUV1_IRQ_ERROR
		panic("wrong imxdpuv1_id");
#endif
		return IMXDPUV1_FALSE;
	}
	imxdpu = &imxdpuv1_array[imxdpuv1_id];

	imxdpu->irq_count++;

#ifdef DEBUG_IMXDPUV1_IRQ_ERROR
	{
		uint32_t int_enable0;
		int_enable0 = imxdpuv1_read_irq(imxdpu,
			IMXDPUV1_COMCTRL_USERINTERRUPTENABLE0);
		if (int_enable0 & 1) {
			panic("IRQ0 enabled\n");
		}
		if (imxdpu->enabled_int[0] & 1) {
			panic("IRQ0 in enabled_int is set\n");
		}
	}
#endif
	/* Get and clear interrupt status */
	int_temp[0] =
		imxdpuv1_read_irq(imxdpu, IMXDPUV1_COMCTRL_USERINTERRUPTSTATUS0);
	int_stat[0] = imxdpu->enabled_int[0] & int_temp[0];
	int_temp[1] =
		imxdpuv1_read_irq(imxdpu, IMXDPUV1_COMCTRL_USERINTERRUPTSTATUS1);
	int_stat[1] = imxdpu->enabled_int[1] & int_temp[1];
#ifdef IMXDPUV1_VERSION_0
#ifdef IMXDPUV1_ENABLE_INTSTAT2
	/* Enable this  (IMXDPUV1_ENABLE_INTSTAT2) if intstat2 interrupts
	   are needed */
	int_temp[2] =
		imxdpuv1_read_irq(imxdpu, IMXDPUV1_COMCTRL_USERINTERRUPTSTATUS2);
	int_stat[2] = imxdpu->enabled_int[2] & int_temp[2];
#endif
#endif
	/* No interrupts are pending */
	if ((int_temp[0] == 0) && (int_temp[1] == 0)
#ifdef IMXDPUV1_VERSION_0
#ifdef IMXDPUV1_ENABLE_INTSTAT2
		&& (int_temp[2] == 0)
#endif
#endif
		) {
	}

	/* No enabled interrupts are pending */
	if ((int_stat[0] == 0) && (int_stat[1] == 0)
#ifdef IMXDPUV1_ENABLE_INTSTAT2
		&& (int_stat[2] == 0)
#endif
		) {
		IMXDPUV1_TRACE_IRQ
			("Error: No enabled interrupts, 0x%08x 0x%08x\n",
			int_temp[0] & ~imxdpu->enabled_int[0],
			int_temp[1] & ~imxdpu->enabled_int[1]);
#ifdef DEBUG_IMXDPUV1_IRQ_ERROR
		panic("no enabled IMXDPU interrupts");
#endif

		return IMXDPUV1_FALSE;
	}

	/* Clear the enabled interrupts */
	if (int_stat[0]) {
		imxdpuv1_write_irq(imxdpu,
			IMXDPUV1_COMCTRL_USERINTERRUPTCLEAR0,
			int_stat[0]);
	}
	if (int_stat[1]) {
		imxdpuv1_write_irq(imxdpu,
			IMXDPUV1_COMCTRL_USERINTERRUPTCLEAR1,
			int_stat[1]);
	}
#ifdef IMXDPUV1_ENABLE_INTSTAT2
	if (int_stat[2]) {
		imxdpuv1_write_irq(imxdpu,
			IMXDPUV1_COMCTRL_USERINTERRUPTCLEAR2,
			int_stat[2]);
	}
#endif

#ifdef IMXDPUV1_ENABLE_INTSTAT2
	if (int_stat[1] != 0) {
		/* add int_stat[2] if needed */
	}
#endif
#ifdef IMXDPUV1_VERSION_0
       /* now handle the interrupts that are pending */
	if (int_stat[0] != 0) {
		if (int_stat[0] & 0xff) {
			if (int_stat[0] &
				INTSTAT0_BIT(IMXDPUV1_STORE9_SHDLOAD_IRQ)) {
				IMXDPUV1_TRACE_IRQ
				    ("IMXDPUV1_STORE9_SHDLOAD_IRQ irq\n");
				imxdpuv1_be_irq_handler(imxdpuv1_id,
							     IMXDPUV1_STORE9_SHDLOAD_IRQ);
				imxdpuv1_handle_registered_irq(imxdpuv1_id,
							     IMXDPUV1_STORE9_SHDLOAD_IRQ);
			}
			if (int_stat[0] &
				INTSTAT0_BIT(IMXDPUV1_STORE9_FRAMECOMPLETE_IRQ)) {
				IMXDPUV1_TRACE_IRQ
				    ("IMXDPUV1_STORE9_FRAMECOMPLETE_IRQ irq\n");
				imxdpuv1_be_irq_handler(imxdpuv1_id,
							     IMXDPUV1_STORE9_FRAMECOMPLETE_IRQ);
				imxdpuv1_handle_registered_irq(imxdpuv1_id,
							     IMXDPUV1_STORE9_FRAMECOMPLETE_IRQ);
			}
			if (int_stat[0] &
				INTSTAT0_BIT(IMXDPUV1_STORE9_SEQCOMPLETE_IRQ)) {
				IMXDPUV1_TRACE_IRQ
				    ("IMXDPUV1_STORE9_SEQCOMPLETE_IRQ irq\n");
				imxdpuv1_be_irq_handler(imxdpuv1_id,
							     IMXDPUV1_STORE9_SEQCOMPLETE_IRQ);
				imxdpuv1_handle_registered_irq(imxdpuv1_id,
							     IMXDPUV1_STORE9_SEQCOMPLETE_IRQ);
			}
			if (int_stat[0] &
				INTSTAT0_BIT(IMXDPUV1_EXTDST0_SHDLOAD_IRQ)) {
				IMXDPUV1_TRACE_IRQ
					("IMXDPUV1_EXTDST0_SHDLOAD_IRQ irq\n");
				imxdpuv1_handle_registered_irq(imxdpuv1_id,
					IMXDPUV1_EXTDST0_SHDLOAD_IRQ);
			}
			if (int_stat[0] &
				INTSTAT0_BIT(IMXDPUV1_EXTDST0_FRAMECOMPLETE_IRQ)) {
				IMXDPUV1_TRACE_IRQ
					("IMXDPUV1_EXTDST0_FRAMECOMPLETE_IRQ\n");
				/* todo: move */
				imxdpuv1_disp_check_shadow_loads(imxdpuv1_id, 0);

				imxdpuv1_handle_registered_irq(imxdpuv1_id,
					IMXDPUV1_EXTDST0_FRAMECOMPLETE_IRQ);
			}
		}
		if (int_stat[0] & 0xff00) {
			if (int_stat[0] &
				INTSTAT0_BIT(IMXDPUV1_EXTDST1_SHDLOAD_IRQ)) {
				IMXDPUV1_TRACE_IRQ(
					"IMXDPUV1_EXTDST1_SHDLOAD_IRQ irq\n");
				imxdpuv1_handle_registered_irq(imxdpuv1_id,
					IMXDPUV1_EXTDST1_SHDLOAD_IRQ);
			}
			if (int_stat[0] &
				INTSTAT0_BIT(
					IMXDPUV1_EXTDST1_FRAMECOMPLETE_IRQ)) {
				IMXDPUV1_TRACE_IRQ(
					"IMXDPUV1_EXTDST1_FRAMECOMPLETE_IRQ\n");
				/* todo: move */
				imxdpuv1_disp_check_shadow_loads(imxdpuv1_id, 1);
				imxdpuv1_handle_registered_irq(imxdpuv1_id,
					IMXDPUV1_EXTDST1_FRAMECOMPLETE_IRQ);
			}
			if (int_stat[0] &
				INTSTAT0_BIT(IMXDPUV1_STORE4_SHDLOAD_IRQ)) {
				IMXDPUV1_TRACE_IRQ_CAPTURE("IMXDPUV1_STORE4_SHDLOAD_IRQ\n");
				imxdpuv1_handle_registered_irq(imxdpuv1_id,
					IMXDPUV1_STORE4_SHDLOAD_IRQ);
			}
		}
		if (int_stat[0] & 0xff0000) {
			if (int_stat[0] &
				INTSTAT0_BIT(IMXDPUV1_STORE4_FRAMECOMPLETE_IRQ)) {
				IMXDPUV1_TRACE_IRQ_CAPTURE(
					"IMXDPUV1_STORE4_FRAMECOMPLETE_IRQ\n");
				imxdpuv1_handle_registered_irq(imxdpuv1_id,
					IMXDPUV1_STORE4_FRAMECOMPLETE_IRQ);
			}
			if (int_stat[0] &
				INTSTAT0_BIT(IMXDPUV1_STORE4_SEQCOMPLETE_IRQ)) {
				IMXDPUV1_TRACE_IRQ_CAPTURE(
					"IMXDPUV1_STORE4_SEQCOMPLETE_IRQ\n");
				imxdpuv1_handle_registered_irq(imxdpuv1_id,
					IMXDPUV1_STORE4_SEQCOMPLETE_IRQ);
			}
			if (int_stat[0] &
				INTSTAT0_BIT(IMXDPUV1_HISTOGRAM4_VALID_IRQ)) {
				IMXDPUV1_TRACE_IRQ(
					"IMXDPUV1_HISTOGRAM4_VALID_IRQ\n");
				imxdpuv1_handle_registered_irq(imxdpuv1_id,
					IMXDPUV1_HISTOGRAM4_VALID_IRQ);
			}
		}
		if (int_stat[0] & 0xff000000) {
			if (int_stat[0] &
				INTSTAT0_BIT(IMXDPUV1_HISTOGRAM5_VALID_IRQ)) {
				IMXDPUV1_TRACE_IRQ(
					"IMXDPUV1_HISTOGRAM5_VALID_IRQ\n");
				imxdpuv1_handle_registered_irq(imxdpuv1_id,
					IMXDPUV1_HISTOGRAM5_VALID_IRQ);
			}
			if (int_stat[1] &
				INTSTAT0_BIT(IMXDPUV1_DISENGCFG_SHDLOAD0_IRQ)) {
				IMXDPUV1_PRINT
					("IMXDPUV1_DISENGCFG_SHDLOAD0_IRQ irq\n");
				imxdpuv1_disp_check_shadow_loads(imxdpuv1_id, 0);
				imxdpuv1_handle_registered_irq(imxdpuv1_id,
					IMXDPUV1_DISENGCFG_SHDLOAD0_IRQ);
			}
			if (int_stat[0] &
				INTSTAT0_BIT(IMXDPUV1_DISENGCFG_FRAMECOMPLETE0_IRQ)) {
				IMXDPUV1_TRACE_IRQ(
					"IMXDPUV1_DISENGCFG_FRAMECOMPLETE0_IRQ\n");
				imxdpuv1_handle_registered_irq(imxdpuv1_id,
					IMXDPUV1_DISENGCFG_FRAMECOMPLETE0_IRQ);
			}
			if (int_stat[0] &
				INTSTAT0_BIT(IMXDPUV1_FRAMEGEN0_INT0_IRQ)) {
				IMXDPUV1_TRACE_IRQ(
					"IMXDPUV1_FRAMEGEN0_INT0_IRQ\n");
				imxdpuv1_disp_vsync_handler(imxdpuv1_id, 0);
				imxdpuv1_handle_registered_irq(imxdpuv1_id,
					IMXDPUV1_FRAMEGEN0_INT0_IRQ);
			}
			if (int_stat[0] &
				INTSTAT0_BIT(IMXDPUV1_FRAMEGEN0_INT1_IRQ)) {
				IMXDPUV1_TRACE_IRQ(
					"IMXDPUV1_FRAMEGEN0_INT1_IRQ\n");
				imxdpuv1_handle_registered_irq(imxdpuv1_id,
					IMXDPUV1_FRAMEGEN0_INT1_IRQ);
			}
		}
	}

	if (int_stat[1] != 0) {
		if (int_stat[1] & 0xff) {

		}
		if (int_stat[1] & 0xff00) {
			if (int_stat[1] &
				INTSTAT1_BIT(IMXDPUV1_FRAMEGEN1_INT0_IRQ)) {
				IMXDPUV1_TRACE_IRQ(
					"IMXDPUV1_FRAMEGEN1_INT0_IRQ\n");
				imxdpuv1_disp_vsync_handler(imxdpuv1_id, 1);
				imxdpuv1_handle_registered_irq(imxdpuv1_id,
					IMXDPUV1_FRAMEGEN1_INT0_IRQ);
			}
		}
		if (int_stat[0] & 0xff0000) {
			if (int_stat[0] &
				INTSTAT1_BIT(IMXDPUV1_COMCTRL_SW0_IRQ)) {
				IMXDPUV1_TRACE_IRQ("IMXDPUV1_COMCTRL_SW0_IRQ\n");
				imxdpuv1_handle_registered_irq(imxdpuv1_id,
					IMXDPUV1_COMCTRL_SW0_IRQ);
			}
			if (int_stat[1] & INTSTAT1_BIT(IMXDPUV1_COMCTRL_SW2_IRQ)) {
				IMXDPUV1_TRACE_IRQ("IMXDPUV1_COMCTRL_SW2_IRQ\n");
				imxdpuv1_handle_registered_irq(imxdpuv1_id,
					IMXDPUV1_COMCTRL_SW2_IRQ);
			}
			if (int_stat[1] & INTSTAT1_BIT(IMXDPUV1_COMCTRL_SW3_IRQ)) {
				IMXDPUV1_TRACE_IRQ("IMXDPUV1_COMCTRL_SW3_IRQ\n");
				imxdpuv1_handle_registered_irq(imxdpuv1_id,
					IMXDPUV1_COMCTRL_SW3_IRQ);
			}

		}
	}
#else
       /* now handle the interrupts that are pending */
	if (int_stat[0] != 0) {
		if (int_stat[0] & 0xff) {
			if (int_stat[0] &
				INTSTAT0_BIT(IMXDPUV1_STORE9_SHDLOAD_IRQ)) {
				IMXDPUV1_TRACE_IRQ
				    ("IMXDPUV1_STORE9_SHDLOAD_IRQ irq\n");
				imxdpuv1_be_irq_handler(imxdpuv1_id,
							     IMXDPUV1_STORE9_SHDLOAD_IRQ);
				imxdpuv1_handle_registered_irq(imxdpuv1_id,
							     IMXDPUV1_STORE9_SHDLOAD_IRQ);
			}
			if (int_stat[0] &
				INTSTAT0_BIT(IMXDPUV1_STORE9_FRAMECOMPLETE_IRQ)) {
				IMXDPUV1_TRACE_IRQ
				    ("IMXDPUV1_STORE9_FRAMECOMPLETE_IRQ irq\n");
				imxdpuv1_be_irq_handler(imxdpuv1_id,
							     IMXDPUV1_STORE9_FRAMECOMPLETE_IRQ);
				imxdpuv1_handle_registered_irq(imxdpuv1_id,
							     IMXDPUV1_STORE9_FRAMECOMPLETE_IRQ);
			}
			if (int_stat[0] &
				INTSTAT0_BIT(IMXDPUV1_STORE9_SEQCOMPLETE_IRQ)) {
				IMXDPUV1_TRACE_IRQ
				    ("IMXDPUV1_STORE9_SEQCOMPLETE_IRQ irq\n");
				imxdpuv1_be_irq_handler(imxdpuv1_id,
							     IMXDPUV1_STORE9_SEQCOMPLETE_IRQ);
				imxdpuv1_handle_registered_irq(imxdpuv1_id,
							     IMXDPUV1_STORE9_SEQCOMPLETE_IRQ);
			}
			if (int_stat[0] &
				INTSTAT0_BIT(IMXDPUV1_EXTDST0_SHDLOAD_IRQ)) {
				IMXDPUV1_TRACE_IRQ
					("IMXDPUV1_EXTDST0_SHDLOAD_IRQ irq\n");
				imxdpuv1_handle_registered_irq(imxdpuv1_id,
					IMXDPUV1_EXTDST0_SHDLOAD_IRQ);
			}
			if (int_stat[0] &
				INTSTAT0_BIT(IMXDPUV1_EXTDST0_FRAMECOMPLETE_IRQ)) {
				IMXDPUV1_TRACE_IRQ
					("IMXDPUV1_EXTDST0_FRAMECOMPLETE_IRQ\n");
				/* todo: move */
				imxdpuv1_disp_check_shadow_loads(imxdpuv1_id, 0);

				imxdpuv1_handle_registered_irq(imxdpuv1_id,
					IMXDPUV1_EXTDST0_FRAMECOMPLETE_IRQ);
			}
		}
		if (int_stat[0] & 0xff00) {
			if (int_stat[0] &
				INTSTAT0_BIT(IMXDPUV1_EXTDST1_SHDLOAD_IRQ)) {
				IMXDPUV1_TRACE_IRQ(
					"IMXDPUV1_EXTDST1_SHDLOAD_IRQ irq\n");
				imxdpuv1_handle_registered_irq(imxdpuv1_id,
					IMXDPUV1_EXTDST1_SHDLOAD_IRQ);
			}
			if (int_stat[0] &
				INTSTAT0_BIT(
					IMXDPUV1_EXTDST1_FRAMECOMPLETE_IRQ)) {
				IMXDPUV1_TRACE_IRQ(
					"IMXDPUV1_EXTDST1_FRAMECOMPLETE_IRQ\n");
				/* todo: move */
				imxdpuv1_disp_check_shadow_loads(imxdpuv1_id, 1);
				imxdpuv1_handle_registered_irq(imxdpuv1_id,
					IMXDPUV1_EXTDST1_FRAMECOMPLETE_IRQ);
			}
		}
		if (int_stat[0] & 0xff0000) {
			if (int_stat[0] &
				INTSTAT0_BIT(IMXDPUV1_FRAMEGEN0_INT0_IRQ)) {
				IMXDPUV1_TRACE_IRQ(
					"IMXDPUV1_FRAMEGEN0_INT0_IRQ\n");
				imxdpuv1_disp_vsync_handler(imxdpuv1_id, 0);
				imxdpuv1_handle_registered_irq(imxdpuv1_id,
					IMXDPUV1_FRAMEGEN0_INT0_IRQ);
			}

		}
		if (int_stat[0] & 0xff000000) {
			if (int_stat[1] &
			    INTSTAT0_BIT(IMXDPUV1_FRAMEGEN1_INT0_IRQ)) {
				IMXDPUV1_TRACE_IRQ(
					"IMXDPUV1_FRAMEGEN1_INT0_IRQ\n");
				imxdpuv1_disp_vsync_handler(imxdpuv1_id, 1);
				imxdpuv1_handle_registered_irq(imxdpuv1_id,
					IMXDPUV1_FRAMEGEN1_INT0_IRQ);
			}
		}
	}

	if (int_stat[1] != 0) {
		if (int_stat[1] & 0xff) {
			if (int_stat[0] &
			    INTSTAT1_BIT(IMXDPUV1_COMCTRL_SW0_IRQ)) {
				IMXDPUV1_TRACE_IRQ("IMXDPUV1_COMCTRL_SW0_IRQ\n");
				imxdpuv1_handle_registered_irq(imxdpuv1_id,
					IMXDPUV1_COMCTRL_SW0_IRQ);
			}
			if (int_stat[1] & INTSTAT1_BIT(IMXDPUV1_COMCTRL_SW2_IRQ)) {
				IMXDPUV1_TRACE_IRQ("IMXDPUV1_COMCTRL_SW2_IRQ\n");
				imxdpuv1_handle_registered_irq(imxdpuv1_id,
					IMXDPUV1_COMCTRL_SW2_IRQ);
			}
		}
		if (int_stat[1] & 0xff00) {
			if (int_stat[1] & INTSTAT1_BIT(IMXDPUV1_COMCTRL_SW3_IRQ)) {
				IMXDPUV1_TRACE_IRQ("IMXDPUV1_COMCTRL_SW3_IRQ\n");
				imxdpuv1_handle_registered_irq(imxdpuv1_id,
					IMXDPUV1_COMCTRL_SW3_IRQ);
			}
		}
		if (int_stat[0] & 0xff0000) {
			/* Reserved for command sequencer debug */
		}
	}
#endif
	return IMXDPUV1_TRUE;
}

/*!
 * This function registers an interrupt handler function for the specified
 * irq line. The interrupt lines are defined in imxdpuv1_events.h
 *
 * @param	imxdpu		imxdpu instance
 * @param       irq     	Interrupt line to get status for.
 *
 * @param       handler 	Input parameter for address of the handler
 *      			function.
 *
 * @param       irq_flags       Flags for interrupt mode. Currently not used.
 *
 * @param       devname 	Input parameter for string name of driver
 *      			registering the handler.
 *
 * @param       data    	Input parameter for pointer of data to be
 *      			passed to the handler.
 *
 * @return      This function returns 0 on success or negative error code on
 *      	fail.
 */
int imxdpuv1_request_irq(int8_t imxdpuv1_id,
	uint32_t irq,
	int (*handler)(int, void *),
	uint32_t irq_flags, const char *devname, void *data)
{
	int ret = 0;
	struct imxdpuv1_soc *imxdpu;

	IMXDPUV1_TRACE("%s()\n", __func__);

	/* imxdpuv1_id starts from 0 */
	if (!((imxdpuv1_id >= 0) && (imxdpuv1_id < IMXDPUV1_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpuv1_array[imxdpuv1_id];

	if (imxdpu->irq_list[irq].handler != NULL) {
		IMXDPUV1_TRACE("handler already installed on irq %d\n", irq);
		ret = -EINVAL;
		goto out;
	}

	imxdpu->irq_list[irq].handler = handler;
	imxdpu->irq_list[irq].flags = irq_flags;
	imxdpu->irq_list[irq].data = data;
	imxdpu->irq_list[irq].name = devname;

	/* Clear and enable the IRQ */
	imxdpuv1_clear_irq(imxdpuv1_id, irq);
	/* Don't enable if a one shot */
	if ((imxdpu->irq_list[irq].flags & IMXDPUV1_IRQF_ONESHOT) == 0)
		imxdpuv1_enable_irq(imxdpuv1_id, irq);
out:
	return ret;
}

/*!
 * This function unregisters an interrupt handler for the specified interrupt
 * line. The interrupt lines are defined in imxdpuv1_events.h
 *
 * @param       imxdpu		imxdpu instance
 * @param       irq     	Interrupt line to get status for.
 *
 * @param       data          Input parameter for pointer of data to be passed
 *      			to the handler. This must match value passed to
 *      			ipu_request_irq().
 *
 */
int imxdpuv1_free_irq(int8_t imxdpuv1_id, uint32_t irq, void *data)
{
	int ret = 0;
	struct imxdpuv1_soc *imxdpu;

	IMXDPUV1_TRACE("%s()\n", __func__);

	/* imxdpuv1_id starts from 0 */
	if (!((imxdpuv1_id >= 0) && (imxdpuv1_id < IMXDPUV1_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpuv1_array[imxdpuv1_id];

	imxdpuv1_disable_irq(imxdpuv1_id, irq);
	imxdpuv1_clear_irq(imxdpuv1_id, irq);
	if (imxdpu->irq_list[irq].data == data)
		memset(&imxdpu->irq_list[irq], 0, sizeof(imxdpu->irq_list[irq]));

	return ret;
}

/*!
 * This function un-initializes the imxdpu interrupts
 *
 * @param	imxdpu  	imxdpu instance
 *
 */
int imxdpuv1_uninit_interrupts(int8_t imxdpuv1_id)
{
	int ret = 0;
	struct imxdpuv1_soc *imxdpu;

	IMXDPUV1_TRACE("%s()\n", __func__);

	/* imxdpuv1_id starts from 0 */
	if (!((imxdpuv1_id >= 0) && (imxdpuv1_id < IMXDPUV1_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpuv1_array[imxdpuv1_id];

	imxdpu->enabled_int[0] = 0;
	imxdpu->enabled_int[1] = 0;
#ifdef IMXDPUV1_VERSION_0
	imxdpu->enabled_int[2] = 0;
#endif
	imxdpuv1_clear_all_irqs(imxdpuv1_id);

	/* Set all interrupt to user mode */
	imxdpuv1_write(imxdpu,
		IMXDPUV1_COMCTRL_USERINTERRUPTMASK0,
		IMXDPUV1_COMCTRL_USERINTERRUPTMASK0_USERINTERRUPTMASK0_MASK);
	imxdpuv1_write(imxdpu,
		IMXDPUV1_COMCTRL_USERINTERRUPTMASK1,
		IMXDPUV1_COMCTRL_USERINTERRUPTMASK1_USERINTERRUPTMASK1_MASK);
#ifdef IMXDPUV1_VERSION_0
	imxdpuv1_write(imxdpu,
		IMXDPUV1_COMCTRL_USERINTERRUPTMASK2,
		IMXDPUV1_COMCTRL_USERINTERRUPTMASK2_USERINTERRUPTMASK2_MASK);
#endif
	/* Set all interrupts to user mode. this will to change to
	   enable panic mode */
	imxdpuv1_write(imxdpu, IMXDPUV1_COMCTRL_USERINTERRUPTENABLE0, 0);
	imxdpuv1_write(imxdpu, IMXDPUV1_COMCTRL_USERINTERRUPTENABLE1, 0);
#ifdef IMXDPUV1_VERSION_0
	imxdpuv1_write(imxdpu, IMXDPUV1_COMCTRL_USERINTERRUPTENABLE2, 0);
#endif
	/* enable needed interupts */
	return ret;
}

/*!
 * This function initializes the imxdpu and the required data structures
 *
 * @param	imxdpuv1_id	id of the diplay unit
 *
 * @return      This function returns 0 on success or negative error code on
 *      	fail.
 */
/* todo: replace with probe function or call from probe
   use device tree as needed */
int imxdpuv1_init(int8_t imxdpuv1_id)
{
	int ret = 0;
	int i;
	struct imxdpuv1_soc *imxdpu;

	IMXDPUV1_TRACE("%s()\n", __func__);

	/* todo: add resource mapping for xrdc, layers, blit, display, ... */

	/* imxdpuv1_id starts from 0 */
	if (!((imxdpuv1_id >= 0) && (imxdpuv1_id < IMXDPUV1_MAX_NUM))) {
		return -EINVAL;
	}

	/* Map the channels to display streams
	   todo:
	   make this mapping dynamic
	   add channel features
	   map capture channels
	 */
	for (i = IMXDPUV1_CHAN_IDX_IN_FIRST; i < IMXDPUV1_CHAN_IDX_MAX; i++) {
		if (i <= IMXDPUV1_CHAN_IDX_17)
			imxdpuv1_array[imxdpuv1_id].chan_data[i].disp_id = 0;
		else if (i < IMXDPUV1_CHAN_IDX_IN_MAX)
			imxdpuv1_array[imxdpuv1_id].chan_data[i].disp_id = 1;
		else if (i < IMXDPUV1_CHAN_IDX_OUT_FIRST)
			imxdpuv1_array[imxdpuv1_id].chan_data[i].disp_id = 0;
		else if (i < IMXDPUV1_CHAN_IDX_OUT_MAX)
			imxdpuv1_array[imxdpuv1_id].chan_data[i].disp_id = 1;
		else
			imxdpuv1_array[imxdpuv1_id].chan_data[i].disp_id = 0;
	}

	imxdpu = &imxdpuv1_array[imxdpuv1_id];
	imxdpu->irq_count = 0;

	if (imxdpuv1_id == 0) {
		imxdpu->base = (void __iomem *)IMXDPUV1_REGS_BASE_PHY0;
		IMXDPUV1_TRACE("%s(): virtual base address is 0x%p (0x%08x physical)\n",
			__func__, imxdpu->base, IMXDPUV1_REGS_BASE_PHY0);

	} else if (imxdpuv1_id == 1) {
		imxdpu->base = (void __iomem *)IMXDPUV1_REGS_BASE_PHY1;
		IMXDPUV1_TRACE("%s(): virtual base address is 0x%p (0x%08x physical)\n",
			__func__, imxdpu->base, IMXDPUV1_REGS_BASE_PHY1);

	} else {
		return -ENOMEM;
	}

	/* todo: may need to check resource allocaiton/ownership for these */
	imxdpuv1_write(imxdpu, IMXDPUV1_FETCHLAYER0_LAYERPROPERTY0,
		IMXDPUV1_FETCHLAYER0_LAYERPROPERTY0_RESET_VALUE);
	imxdpuv1_write(imxdpu, IMXDPUV1_FETCHLAYER0_LAYERPROPERTY1,
		IMXDPUV1_FETCHLAYER0_LAYERPROPERTY1_RESET_VALUE);
	imxdpuv1_write(imxdpu, IMXDPUV1_FETCHLAYER0_LAYERPROPERTY2,
		IMXDPUV1_FETCHLAYER0_LAYERPROPERTY2_RESET_VALUE);
	imxdpuv1_write(imxdpu, IMXDPUV1_FETCHLAYER0_LAYERPROPERTY3,
		IMXDPUV1_FETCHLAYER0_LAYERPROPERTY3_RESET_VALUE);
	imxdpuv1_write(imxdpu, IMXDPUV1_FETCHLAYER0_LAYERPROPERTY4,
		IMXDPUV1_FETCHLAYER0_LAYERPROPERTY4_RESET_VALUE);
	imxdpuv1_write(imxdpu, IMXDPUV1_FETCHLAYER0_LAYERPROPERTY5,
		IMXDPUV1_FETCHLAYER0_LAYERPROPERTY5_RESET_VALUE);
	imxdpuv1_write(imxdpu, IMXDPUV1_FETCHLAYER0_LAYERPROPERTY6,
		IMXDPUV1_FETCHLAYER0_LAYERPROPERTY6_RESET_VALUE);
	imxdpuv1_write(imxdpu, IMXDPUV1_FETCHLAYER0_LAYERPROPERTY7,
		IMXDPUV1_FETCHLAYER0_LAYERPROPERTY7_RESET_VALUE);
	imxdpuv1_write(imxdpu, IMXDPUV1_FETCHLAYER0_TRIGGERENABLE,
		IMXDPUV1_FETCHLAYER0_TRIGGERENABLE_RESET_VALUE);
#ifdef IMXDPUV1_VERSION_0
	imxdpuv1_write(imxdpu, IMXDPUV1_FETCHLAYER1_LAYERPROPERTY0,
		IMXDPUV1_FETCHLAYER1_LAYERPROPERTY0_RESET_VALUE);
	imxdpuv1_write(imxdpu, IMXDPUV1_FETCHLAYER1_LAYERPROPERTY1,
		IMXDPUV1_FETCHLAYER1_LAYERPROPERTY1_RESET_VALUE);
	imxdpuv1_write(imxdpu, IMXDPUV1_FETCHLAYER1_LAYERPROPERTY2,
		IMXDPUV1_FETCHLAYER1_LAYERPROPERTY2_RESET_VALUE);
	imxdpuv1_write(imxdpu, IMXDPUV1_FETCHLAYER1_LAYERPROPERTY3,
		IMXDPUV1_FETCHLAYER1_LAYERPROPERTY3_RESET_VALUE);
	imxdpuv1_write(imxdpu, IMXDPUV1_FETCHLAYER1_LAYERPROPERTY4,
		IMXDPUV1_FETCHLAYER1_LAYERPROPERTY4_RESET_VALUE);
	imxdpuv1_write(imxdpu, IMXDPUV1_FETCHLAYER1_LAYERPROPERTY5,
		IMXDPUV1_FETCHLAYER1_LAYERPROPERTY5_RESET_VALUE);
	imxdpuv1_write(imxdpu, IMXDPUV1_FETCHLAYER1_LAYERPROPERTY6,
		IMXDPUV1_FETCHLAYER1_LAYERPROPERTY6_RESET_VALUE);
	imxdpuv1_write(imxdpu, IMXDPUV1_FETCHLAYER1_LAYERPROPERTY7,
		IMXDPUV1_FETCHLAYER1_LAYERPROPERTY7_RESET_VALUE);
	imxdpuv1_write(imxdpu, IMXDPUV1_FETCHLAYER1_TRIGGERENABLE,
		IMXDPUV1_FETCHLAYER1_TRIGGERENABLE_RESET_VALUE);
#endif
	imxdpuv1_write(imxdpu, IMXDPUV1_FETCHWARP2_LAYERPROPERTY0,
		IMXDPUV1_FETCHWARP2_LAYERPROPERTY0_RESET_VALUE);
	imxdpuv1_write(imxdpu, IMXDPUV1_FETCHWARP2_LAYERPROPERTY1,
		IMXDPUV1_FETCHWARP2_LAYERPROPERTY1_RESET_VALUE);
	imxdpuv1_write(imxdpu, IMXDPUV1_FETCHWARP2_LAYERPROPERTY2,
		IMXDPUV1_FETCHWARP2_LAYERPROPERTY2_RESET_VALUE);
	imxdpuv1_write(imxdpu, IMXDPUV1_FETCHWARP2_LAYERPROPERTY3,
		IMXDPUV1_FETCHWARP2_LAYERPROPERTY3_RESET_VALUE);
	imxdpuv1_write(imxdpu, IMXDPUV1_FETCHWARP2_LAYERPROPERTY4,
		IMXDPUV1_FETCHWARP2_LAYERPROPERTY4_RESET_VALUE);
	imxdpuv1_write(imxdpu, IMXDPUV1_FETCHWARP2_LAYERPROPERTY5,
		IMXDPUV1_FETCHWARP2_LAYERPROPERTY5_RESET_VALUE);
	imxdpuv1_write(imxdpu, IMXDPUV1_FETCHWARP2_LAYERPROPERTY6,
		IMXDPUV1_FETCHWARP2_LAYERPROPERTY6_RESET_VALUE);
	imxdpuv1_write(imxdpu, IMXDPUV1_FETCHWARP2_LAYERPROPERTY7,
		IMXDPUV1_FETCHWARP2_LAYERPROPERTY7_RESET_VALUE);
	imxdpuv1_write(imxdpu, IMXDPUV1_FETCHWARP2_TRIGGERENABLE,
		IMXDPUV1_FETCHWARP2_TRIGGERENABLE_RESET_VALUE);

	/* Initial StaticControl configuration - reset values */
	/* IMXDPUV1_FETCHDECODE9_STATICCONTROL  */
	imxdpuv1_write(imxdpu, IMXDPUV1_FETCHDECODE9_STATICCONTROL,
		IMXDPUV1_FETCHDECODE9_STATICCONTROL_RESET_VALUE);
#ifdef IMXDPUV1_VERSION_0
	/* IMXDPUV1_FETCHPERSP9_STATICCONTROL   */
	imxdpuv1_write(imxdpu, IMXDPUV1_FETCHPERSP9_STATICCONTROL,
		IMXDPUV1_FETCHPERSP9_STATICCONTROL_RESET_VALUE);
#else
	/* IMXDPUV1_FETCHPERSP9_STATICCONTROL   */
	imxdpuv1_write(imxdpu, IMXDPUV1_FETCHWARP9_STATICCONTROL,
		IMXDPUV1_FETCHWARP9_STATICCONTROL_RESET_VALUE);
#endif

	/* IMXDPUV1_FETCHECO9_STATICCONTROL     */
	imxdpuv1_write(imxdpu, IMXDPUV1_FETCHECO9_STATICCONTROL,
		IMXDPUV1_FETCHECO9_STATICCONTROL_RESET_VALUE);

	/* IMXDPUV1_ROP9_STATICCONTROL          */
	imxdpuv1_write(imxdpu, IMXDPUV1_ROP9_STATICCONTROL,
		IMXDPUV1_ROP9_STATICCONTROL_RESET_VALUE);

	/* IMXDPUV1_CLUT9_STATICCONTROL         */
	imxdpuv1_write(imxdpu, IMXDPUV1_CLUT9_STATICCONTROL,
		IMXDPUV1_CLUT9_STATICCONTROL_RESET_VALUE);

	/* IMXDPUV1_MATRIX9_STATICCONTROL       */
	imxdpuv1_write(imxdpu, IMXDPUV1_MATRIX9_STATICCONTROL,
		IMXDPUV1_MATRIX9_STATICCONTROL_RESET_VALUE);

	/* IMXDPUV1_HSCALER9_STATICCONTROL      */
	imxdpuv1_write(imxdpu, IMXDPUV1_HSCALER9_STATICCONTROL,
		IMXDPUV1_HSCALER9_STATICCONTROL_RESET_VALUE);

	/* IMXDPUV1_VSCALER9_STATICCONTROL      */
	imxdpuv1_write(imxdpu, IMXDPUV1_VSCALER9_STATICCONTROL,
		IMXDPUV1_VSCALER9_STATICCONTROL_RESET_VALUE);

	/* IMXDPUV1_FILTER9_STATICCONTROL       */
	imxdpuv1_write(imxdpu, IMXDPUV1_FILTER9_STATICCONTROL,
		IMXDPUV1_FILTER9_STATICCONTROL_RESET_VALUE);

	/* IMXDPUV1_BLITBLEND9_STATICCONTROL    */
	imxdpuv1_write(imxdpu, IMXDPUV1_BLITBLEND9_STATICCONTROL,
		IMXDPUV1_BLITBLEND9_STATICCONTROL_RESET_VALUE);

	/* IMXDPUV1_STORE9_STATICCONTROL        */
	imxdpuv1_write(imxdpu, IMXDPUV1_STORE9_STATICCONTROL,
		IMXDPUV1_STORE9_STATICCONTROL_RESET_VALUE);

	/* IMXDPUV1_CONSTFRAME0_STATICCONTROL    */
	imxdpuv1_write(imxdpu, IMXDPUV1_CONSTFRAME0_STATICCONTROL,
		IMXDPUV1_CONSTFRAME0_STATICCONTROL_RESET_VALUE);

	/* IMXDPUV1_EXTDST0_STATICCONTROL        */
	imxdpuv1_write(imxdpu, IMXDPUV1_EXTDST0_STATICCONTROL,
		IMXDPUV1_EXTDST0_STATICCONTROL_RESET_VALUE);

	/* IMXDPUV1_EXTDST4_STATICCONTROL        */
	imxdpuv1_write(imxdpu, IMXDPUV1_EXTDST4_STATICCONTROL,
		IMXDPUV1_EXTDST4_STATICCONTROL_RESET_VALUE);

	/* todo: IMXDPUV1_CONSTFRAME4_STATICCONTROL    */

	/* IMXDPUV1_CONSTFRAME1_STATICCONTROL    */
	imxdpuv1_write(imxdpu, IMXDPUV1_CONSTFRAME1_STATICCONTROL,
		IMXDPUV1_CONSTFRAME1_STATICCONTROL_RESET_VALUE);

	/* IMXDPUV1_EXTDST1_STATICCONTROL        */
	imxdpuv1_write(imxdpu, IMXDPUV1_EXTDST1_STATICCONTROL,
		IMXDPUV1_EXTDST1_STATICCONTROL_RESET_VALUE);

	/* IMXDPUV1_EXTDST5_STATICCONTROL        */
	imxdpuv1_write(imxdpu, IMXDPUV1_EXTDST5_STATICCONTROL,
		IMXDPUV1_EXTDST5_STATICCONTROL_RESET_VALUE);

	/* todo: IMXDPUV1_CONSTFRAME5_STATICCONTROL    */
#ifdef IMXDPUV1_VERSION_0
	/* IMXDPUV1_EXTSRC4_STATICCONTROL        */
	imxdpuv1_write(imxdpu, IMXDPUV1_EXTSRC4_STATICCONTROL,
		IMXDPUV1_EXTSRC4_STATICCONTROL_RESET_VALUE);

	/* IMXDPUV1_STORE4_STATICCONTROL         */
	imxdpuv1_write(imxdpu, IMXDPUV1_STORE4_STATICCONTROL,
		IMXDPUV1_STORE4_STATICCONTROL_RESET_VALUE);

	/* IMXDPUV1_EXTSRC5_STATICCONTROL        */
	imxdpuv1_write(imxdpu, IMXDPUV1_EXTSRC5_STATICCONTROL,
		IMXDPUV1_EXTSRC5_STATICCONTROL_RESET_VALUE);

	/* IMXDPUV1_STORE5_STATICCONTROL         */
	imxdpuv1_write(imxdpu, IMXDPUV1_STORE5_STATICCONTROL,
		IMXDPUV1_STORE5_STATICCONTROL_RESET_VALUE);

	/* IMXDPUV1_FETCHDECODE2_STATICCONTROL   */
	imxdpuv1_write(imxdpu, IMXDPUV1_FETCHDECODE2_STATICCONTROL,
		IMXDPUV1_FETCHDECODE2_STATICCONTROL_RESET_VALUE);

	/* IMXDPUV1_FETCHDECODE3_STATICCONTROL   */
	imxdpuv1_write(imxdpu, IMXDPUV1_FETCHDECODE3_STATICCONTROL,
		IMXDPUV1_FETCHDECODE3_STATICCONTROL_RESET_VALUE);
#endif
	/* IMXDPUV1_FETCHWARP2_STATICCONTROL     */
	imxdpuv1_write(imxdpu, IMXDPUV1_FETCHWARP2_STATICCONTROL,
		IMXDPUV1_FETCHWARP2_STATICCONTROL_RESET_VALUE);

	/* IMXDPUV1_FETCHECO2_STATICCONTROL      */
	imxdpuv1_write(imxdpu, IMXDPUV1_FETCHECO9_STATICCONTROL,
		IMXDPUV1_FETCHECO9_STATICCONTROL_RESET_VALUE);

	/* IMXDPUV1_FETCHDECODE0_STATICCONTROL   */
	imxdpuv1_write(imxdpu, IMXDPUV1_FETCHDECODE0_STATICCONTROL,
		IMXDPUV1_FETCHDECODE0_STATICCONTROL_RESET_VALUE);

	/* IMXDPUV1_FETCHECO0_STATICCONTROL      */
	imxdpuv1_write(imxdpu, IMXDPUV1_FETCHECO0_STATICCONTROL,
		IMXDPUV1_FETCHECO0_STATICCONTROL_RESET_VALUE);

	/* IMXDPUV1_FETCHDECODE1_STATICCONTROL   */
	imxdpuv1_write(imxdpu, IMXDPUV1_FETCHDECODE1_STATICCONTROL,
		IMXDPUV1_FETCHDECODE1_STATICCONTROL_RESET_VALUE);

	/* IMXDPUV1_FETCHECO1_STATICCONTROL      */
	imxdpuv1_write(imxdpu, IMXDPUV1_FETCHECO1_STATICCONTROL,
		IMXDPUV1_FETCHECO1_STATICCONTROL_RESET_VALUE);

	/* todo: IMXDPUV1_MATRIX5_STATICCONTROL        */
	/* todo: IMXDPUV1_HSCALER5_STATICCONTROL       */
	/* todo: IMXDPUV1_VSCALER5_STATICCONTROL       */
	/* IMXDPUV1_LAYERBLEND0_STATICCONTROL    */
	imxdpuv1_write(imxdpu, IMXDPUV1_LAYERBLEND0_STATICCONTROL,
		IMXDPUV1_LAYERBLEND0_STATICCONTROL_RESET_VALUE);

	/* IMXDPUV1_LAYERBLEND1_STATICCONTROL    */
	imxdpuv1_write(imxdpu, IMXDPUV1_LAYERBLEND1_STATICCONTROL,
		IMXDPUV1_LAYERBLEND1_STATICCONTROL_RESET_VALUE);

	/* IMXDPUV1_LAYERBLEND2_STATICCONTROL    */
	imxdpuv1_write(imxdpu, IMXDPUV1_LAYERBLEND2_STATICCONTROL,
		IMXDPUV1_LAYERBLEND2_STATICCONTROL_RESET_VALUE);

	/* IMXDPUV1_LAYERBLEND3_STATICCONTROL    */
	imxdpuv1_write(imxdpu, IMXDPUV1_LAYERBLEND3_STATICCONTROL,
		IMXDPUV1_LAYERBLEND3_STATICCONTROL_RESET_VALUE);
#ifdef IMXDPUV1_VERSION_0
	/* IMXDPUV1_LAYERBLEND4_STATICCONTROL    */
	imxdpuv1_write(imxdpu, IMXDPUV1_LAYERBLEND4_STATICCONTROL,
		IMXDPUV1_LAYERBLEND4_STATICCONTROL_RESET_VALUE);

	/* IMXDPUV1_LAYERBLEND5_STATICCONTROL    */
	imxdpuv1_write(imxdpu, IMXDPUV1_LAYERBLEND5_STATICCONTROL,
		IMXDPUV1_LAYERBLEND5_STATICCONTROL_RESET_VALUE);

	/* IMXDPUV1_LAYERBLEND6_STATICCONTROL    */
	imxdpuv1_write(imxdpu, IMXDPUV1_LAYERBLEND6_STATICCONTROL,
		IMXDPUV1_LAYERBLEND6_STATICCONTROL_RESET_VALUE);
#endif
	/* Dynamic config */
	imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_FETCHDECODE9_DYNAMIC,
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_SRC_SEL,
			IMXDPUV1_PIXENGCFG_SRC_SEL__DISABLE));

#ifdef IMXDPUV1_VERSION_0
	imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_FETCHPERSP9_DYNAMIC,
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_SRC_SEL,
			IMXDPUV1_PIXENGCFG_SRC_SEL__DISABLE));
#else
	imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_FETCHWARP9_DYNAMIC,
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_SRC_SEL,
			IMXDPUV1_PIXENGCFG_SRC_SEL__DISABLE));
#endif

	imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_ROP9_DYNAMIC,
		IMXDPUV1_SET_FIELD
		(IMXDPUV1_PIXENGCFG_ROP9_DYNAMIC_ROP9_PRIM_SEL,
			IMXDPUV1_PIXENGCFG_SRC_SEL__DISABLE) |
		IMXDPUV1_SET_FIELD
		(IMXDPUV1_PIXENGCFG_ROP9_DYNAMIC_ROP9_SEC_SEL,
			IMXDPUV1_PIXENGCFG_SRC_SEL__DISABLE) |
		IMXDPUV1_SET_FIELD
		(IMXDPUV1_PIXENGCFG_ROP9_DYNAMIC_ROP9_TERT_SEL,
			IMXDPUV1_PIXENGCFG_SRC_SEL__DISABLE) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_CLKEN,
			IMXDPUV1_PIXENGCFG_CLKEN__AUTOMATIC));

	imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_CLUT9_DYNAMIC,
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_SRC_SEL,
			IMXDPUV1_PIXENGCFG_SRC_SEL__DISABLE));

	imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_MATRIX9_DYNAMIC,
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_SRC_SEL,
			IMXDPUV1_PIXENGCFG_SRC_SEL__DISABLE) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_CLKEN,
			IMXDPUV1_PIXENGCFG_CLKEN__AUTOMATIC));

	imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_HSCALER9_DYNAMIC,
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_SRC_SEL,
			IMXDPUV1_PIXENGCFG_SRC_SEL__DISABLE) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_CLKEN,
			IMXDPUV1_PIXENGCFG_CLKEN__AUTOMATIC));

	imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_VSCALER9_DYNAMIC,
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_SRC_SEL,
			IMXDPUV1_PIXENGCFG_SRC_SEL__DISABLE) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_CLKEN,
			IMXDPUV1_PIXENGCFG_CLKEN__AUTOMATIC));

	imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_FILTER9_DYNAMIC,
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_SRC_SEL,
			IMXDPUV1_PIXENGCFG_SRC_SEL__DISABLE) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_CLKEN,
			IMXDPUV1_PIXENGCFG_CLKEN__AUTOMATIC));

	imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_BLITBLEND9_DYNAMIC,
		IMXDPUV1_SET_FIELD
		(IMXDPUV1_PIXENGCFG_BLITBLEND9_DYNAMIC_BLITBLEND9_PRIM_SEL,
			IMXDPUV1_PIXENGCFG_SRC_SEL__DISABLE) |
		IMXDPUV1_SET_FIELD
		(IMXDPUV1_PIXENGCFG_BLITBLEND9_DYNAMIC_BLITBLEND9_SEC_SEL,
			IMXDPUV1_PIXENGCFG_SRC_SEL__DISABLE) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_CLKEN,
			IMXDPUV1_PIXENGCFG_CLKEN__AUTOMATIC));

	imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_STORE9_DYNAMIC,
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_SRC_SEL,
			IMXDPUV1_PIXENGCFG_SRC_SEL__DISABLE));

	imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_EXTDST0_DYNAMIC,
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_SRC_SEL,
			IMXDPUV1_PIXENGCFG_SRC_SEL__DISABLE));

	imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_EXTDST4_DYNAMIC,
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_SRC_SEL,
			IMXDPUV1_PIXENGCFG_SRC_SEL__DISABLE));

	imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_EXTDST1_DYNAMIC,
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_SRC_SEL,
			IMXDPUV1_PIXENGCFG_SRC_SEL__DISABLE));

	imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_EXTDST5_DYNAMIC,
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_SRC_SEL,
			IMXDPUV1_PIXENGCFG_SRC_SEL__DISABLE));
#ifdef IMXDPUV1_VERSION_0
	imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_STORE4_DYNAMIC,
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_SRC_SEL,
			IMXDPUV1_PIXENGCFG_SRC_SEL__DISABLE));

	imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_STORE5_DYNAMIC,
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_SRC_SEL,
			IMXDPUV1_PIXENGCFG_SRC_SEL__DISABLE));

	imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_FETCHDECODE2_DYNAMIC,
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_SRC_SEL,
			IMXDPUV1_PIXENGCFG_SRC_SEL__DISABLE));

	imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_FETCHDECODE3_DYNAMIC,
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_SRC_SEL,
			IMXDPUV1_PIXENGCFG_SRC_SEL__DISABLE));
#endif
	imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_FETCHWARP2_DYNAMIC,
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_SRC_SEL,
			IMXDPUV1_PIXENGCFG_SRC_SEL__DISABLE));

	imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_FETCHDECODE0_DYNAMIC,
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_SRC_SEL,
			IMXDPUV1_PIXENGCFG_SRC_SEL__DISABLE));

	imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_FETCHDECODE1_DYNAMIC,
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_SRC_SEL,
			IMXDPUV1_PIXENGCFG_SRC_SEL__DISABLE));
#ifdef IMXDPUV1_VERSION_0
	imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_GAMMACOR4_DYNAMIC,
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_SRC_SEL,
			IMXDPUV1_PIXENGCFG_SRC_SEL__DISABLE));
#endif
	imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_MATRIX4_DYNAMIC,
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_SRC_SEL,
			IMXDPUV1_PIXENGCFG_SRC_SEL__DISABLE) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_CLKEN,
			IMXDPUV1_PIXENGCFG_CLKEN__AUTOMATIC));

	imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_HSCALER4_DYNAMIC,
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_SRC_SEL,
			IMXDPUV1_PIXENGCFG_SRC_SEL__DISABLE) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_CLKEN,
			IMXDPUV1_PIXENGCFG_CLKEN__AUTOMATIC));

	imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_VSCALER4_DYNAMIC,
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_SRC_SEL,
			IMXDPUV1_PIXENGCFG_SRC_SEL__DISABLE) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_CLKEN,
			IMXDPUV1_PIXENGCFG_CLKEN__AUTOMATIC));
#ifdef IMXDPUV1_VERSION_0
	imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_HISTOGRAM4_DYNAMIC,
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_SRC_SEL,
			IMXDPUV1_PIXENGCFG_SRC_SEL__DISABLE) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_CLKEN,
			IMXDPUV1_PIXENGCFG_CLKEN__AUTOMATIC));

	imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_GAMMACOR5_DYNAMIC,
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_SRC_SEL,
			IMXDPUV1_PIXENGCFG_SRC_SEL__DISABLE));
#endif
	imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_MATRIX5_DYNAMIC,
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_SRC_SEL,
			IMXDPUV1_PIXENGCFG_SRC_SEL__DISABLE) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_CLKEN,
			IMXDPUV1_PIXENGCFG_CLKEN__AUTOMATIC));

	imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_HSCALER5_DYNAMIC,
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_SRC_SEL,
			IMXDPUV1_PIXENGCFG_SRC_SEL__DISABLE) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_CLKEN,
			IMXDPUV1_PIXENGCFG_CLKEN__AUTOMATIC));

	imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_VSCALER5_DYNAMIC,
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_SRC_SEL,
			IMXDPUV1_PIXENGCFG_SRC_SEL__DISABLE) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_CLKEN,
			IMXDPUV1_PIXENGCFG_CLKEN__AUTOMATIC));
#ifdef IMXDPUV1_VERSION_0
	imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_HISTOGRAM5_DYNAMIC,
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_SRC_SEL,
			IMXDPUV1_PIXENGCFG_SRC_SEL__DISABLE) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_CLKEN,
			IMXDPUV1_PIXENGCFG_CLKEN__AUTOMATIC));
#endif
	imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_LAYERBLEND0_DYNAMIC,
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_LAYERBLEND_PRIM_SEL,
			IMXDPUV1_PIXENGCFG_LAYERBLEND_PRIM_SEL__DISABLE)
		| IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_LAYERBLEND_SEC_SEL,
			IMXDPUV1_PIXENGCFG_LAYERBLEND_SEC_SEL__DISABLE)
		| IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_CLKEN,
			IMXDPUV1_PIXENGCFG_CLKEN__AUTOMATIC));

	imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_LAYERBLEND1_DYNAMIC,
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_LAYERBLEND_PRIM_SEL,
			IMXDPUV1_PIXENGCFG_LAYERBLEND_PRIM_SEL__DISABLE)
		| IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_LAYERBLEND_SEC_SEL,
			IMXDPUV1_PIXENGCFG_LAYERBLEND_SEC_SEL__DISABLE)
		| IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_CLKEN,
			IMXDPUV1_PIXENGCFG_CLKEN__AUTOMATIC));

	imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_LAYERBLEND2_DYNAMIC,
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_LAYERBLEND_PRIM_SEL,
			IMXDPUV1_PIXENGCFG_LAYERBLEND_PRIM_SEL__DISABLE)
		| IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_LAYERBLEND_SEC_SEL,
			IMXDPUV1_PIXENGCFG_LAYERBLEND_SEC_SEL__DISABLE)
		| IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_CLKEN,
			IMXDPUV1_PIXENGCFG_CLKEN__AUTOMATIC));

	imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_LAYERBLEND3_DYNAMIC,
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_LAYERBLEND_PRIM_SEL,
			IMXDPUV1_PIXENGCFG_LAYERBLEND_PRIM_SEL__DISABLE)
		| IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_LAYERBLEND_SEC_SEL,
			IMXDPUV1_PIXENGCFG_LAYERBLEND_SEC_SEL__DISABLE)
		| IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_CLKEN,
			IMXDPUV1_PIXENGCFG_CLKEN__AUTOMATIC));
#ifdef IMXDPUV1_VERSION_0
	imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_LAYERBLEND4_DYNAMIC,
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_LAYERBLEND_PRIM_SEL,
			IMXDPUV1_PIXENGCFG_LAYERBLEND_PRIM_SEL__DISABLE)
		| IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_LAYERBLEND_SEC_SEL,
			IMXDPUV1_PIXENGCFG_LAYERBLEND_SEC_SEL__DISABLE)
		| IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_CLKEN,
			IMXDPUV1_PIXENGCFG_CLKEN__AUTOMATIC));

	imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_LAYERBLEND5_DYNAMIC,
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_LAYERBLEND_PRIM_SEL,
			IMXDPUV1_PIXENGCFG_LAYERBLEND_PRIM_SEL__DISABLE)
		| IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_LAYERBLEND_SEC_SEL,
			IMXDPUV1_PIXENGCFG_LAYERBLEND_SEC_SEL__DISABLE)
		| IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_CLKEN,
			IMXDPUV1_PIXENGCFG_CLKEN__AUTOMATIC));

	imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_LAYERBLEND6_DYNAMIC,
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_LAYERBLEND_PRIM_SEL,
			IMXDPUV1_PIXENGCFG_LAYERBLEND_PRIM_SEL__DISABLE)
		| IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_LAYERBLEND_SEC_SEL,
			IMXDPUV1_PIXENGCFG_LAYERBLEND_SEC_SEL__DISABLE)
		| IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_CLKEN,
			IMXDPUV1_PIXENGCFG_CLKEN__AUTOMATIC));
#endif
	/* Static configuration - reset values */
	imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_STORE9_STATIC,
		IMXDPUV1_PIXENGCFG_STORE9_STATIC_RESET_VALUE);

	imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_EXTDST0_STATIC,
		IMXDPUV1_PIXENGCFG_EXTDST0_STATIC_RESET_VALUE);

	imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_EXTDST4_STATIC,
		IMXDPUV1_PIXENGCFG_EXTDST4_STATIC_RESET_VALUE);

	imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_EXTDST1_STATIC,
		IMXDPUV1_PIXENGCFG_EXTDST1_STATIC_RESET_VALUE);

	imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_EXTDST5_STATIC,
		IMXDPUV1_PIXENGCFG_EXTDST5_STATIC_RESET_VALUE);
#ifdef IMXDPUV1_VERSION_0
	imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_STORE4_STATIC,
		IMXDPUV1_PIXENGCFG_STORE4_STATIC_RESET_VALUE);

	imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_STORE5_STATIC,
		IMXDPUV1_PIXENGCFG_STORE5_STATIC_RESET_VALUE);
#endif
	/* Static configuration - initial settings */
	imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_STORE9_STATIC,
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_PIXENGCFG_STORE9_STATIC_STORE9_SHDEN, 1) |
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_PIXENGCFG_STORE9_STATIC_STORE9_POWERDOWN,
			IMXDPUV1_FALSE) |
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_PIXENGCFG_STORE9_STATIC_STORE9_SYNC_MODE,
			IMXDPUV1_PIXENGCFG_STORE9_STATIC_STORE9_SYNC_MODE__SINGLE) |
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_PIXENGCFG_STORE9_STATIC_STORE9_SW_RESET,
			IMXDPUV1_PIXENGCFG_STORE9_STATIC_STORE9_SW_RESET__OPERATION) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_STORE9_STATIC_STORE9_DIV,
			IMXDPUV1_PIXENGCFG_DIVIDER_RESET));

	imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_EXTDST0_STATIC,
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_PIXENGCFG_EXTDST0_STATIC_EXTDST0_SHDEN, 1) |
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_PIXENGCFG_EXTDST0_STATIC_EXTDST0_POWERDOWN,
			IMXDPUV1_FALSE) |
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_PIXENGCFG_EXTDST0_STATIC_EXTDST0_SYNC_MODE,
			IMXDPUV1_PIXENGCFG_EXTDST0_STATIC_EXTDST0_SYNC_MODE__AUTO) |
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_PIXENGCFG_EXTDST0_STATIC_EXTDST0_SW_RESET,
			IMXDPUV1_PIXENGCFG_EXTDST0_STATIC_EXTDST0_SW_RESET__OPERATION) |
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_PIXENGCFG_EXTDST0_STATIC_EXTDST0_DIV,
			IMXDPUV1_PIXENGCFG_DIVIDER_RESET));

	/* todo: IMXDPUV1_PIXENGCFG_EXTDST4_STATIC_OFFSET */

	imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_EXTDST1_STATIC,
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_PIXENGCFG_EXTDST1_STATIC_EXTDST1_SHDEN, 1) |
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_PIXENGCFG_EXTDST1_STATIC_EXTDST1_POWERDOWN,
			IMXDPUV1_FALSE) |
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_PIXENGCFG_EXTDST1_STATIC_EXTDST1_SYNC_MODE,
			IMXDPUV1_PIXENGCFG_EXTDST1_STATIC_EXTDST1_SYNC_MODE__AUTO) |
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_PIXENGCFG_EXTDST1_STATIC_EXTDST1_SW_RESET,
			IMXDPUV1_PIXENGCFG_EXTDST1_STATIC_EXTDST1_SW_RESET__OPERATION) |
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_PIXENGCFG_EXTDST1_STATIC_EXTDST1_DIV,
			IMXDPUV1_PIXENGCFG_DIVIDER_RESET));

	/* todo: IMXDPUV1_PIXENGCFG_EXTDST5_STATIC_OFFSET */
#ifdef IMXDPUV1_VERSION_0
	imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_STORE4_STATIC,
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_PIXENGCFG_STORE4_STATIC_STORE4_SHDEN, 1) |
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_PIXENGCFG_STORE4_STATIC_STORE4_POWERDOWN,
			IMXDPUV1_FALSE) |
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_PIXENGCFG_STORE4_STATIC_STORE4_SYNC_MODE,
			IMXDPUV1_PIXENGCFG_STORE4_STATIC_STORE4_SYNC_MODE__SINGLE) |
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_PIXENGCFG_STORE4_STATIC_STORE4_SW_RESET,
			IMXDPUV1_PIXENGCFG_STORE4_STATIC_STORE4_SW_RESET__OPERATION) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_STORE4_STATIC_STORE4_DIV,
			IMXDPUV1_PIXENGCFG_DIVIDER_RESET));
#endif
	/* todo: IMXDPUV1_PIXENGCFG_STORE4_STATIC */
	/* Static Control configuration */
	/* IMXDPUV1_FETCHDECODE9_STATICCONTROL  */
	imxdpuv1_write(imxdpu, IMXDPUV1_FETCHDECODE9_STATICCONTROL,
		IMXDPUV1_SET_FIELD(IMXDPUV1_FETCHDECODE9_STATICCONTROL_SHDEN, 1) |
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_FETCHDECODE9_STATICCONTROL_BASEADDRESSAUTOUPDATE, 0));
#ifdef IMXDPUV1_VERSION_0
	/* IMXDPUV1_FETCHPERSP9_STATICCONTROL   */
	imxdpuv1_write(imxdpu, IMXDPUV1_FETCHPERSP9_STATICCONTROL,
		IMXDPUV1_SET_FIELD(IMXDPUV1_FETCHPERSP9_STATICCONTROL_SHDEN, 1) |
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_FETCHPERSP9_STATICCONTROL_BASEADDRESSAUTOUPDATE, 0));
#else
	/* IMXDPUV1_FETCHWARP9_STATICCONTROL   */
	imxdpuv1_write(imxdpu, IMXDPUV1_FETCHWARP9_STATICCONTROL,
		IMXDPUV1_SET_FIELD(IMXDPUV1_FETCHWARP9_STATICCONTROL_SHDEN, 1) |
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_FETCHWARP9_STATICCONTROL_BASEADDRESSAUTOUPDATE, 0));
#endif
	/* IMXDPUV1_FETCHECO9_STATICCONTROL     */
	imxdpuv1_write(imxdpu, IMXDPUV1_FETCHECO9_STATICCONTROL,
		IMXDPUV1_SET_FIELD(IMXDPUV1_FETCHECO9_STATICCONTROL_SHDEN, 1) |
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_FETCHECO9_STATICCONTROL_BASEADDRESSAUTOUPDATE, 0));

	/* IMXDPUV1_ROP9_STATICCONTROL          */
	imxdpuv1_write(imxdpu, IMXDPUV1_ROP9_STATICCONTROL,
		IMXDPUV1_SET_FIELD(IMXDPUV1_ROP9_STATICCONTROL_SHDEN, 1));

	/* IMXDPUV1_CLUT9_STATICCONTROL         */
	imxdpuv1_write(imxdpu, IMXDPUV1_CLUT9_STATICCONTROL,
		IMXDPUV1_SET_FIELD(IMXDPUV1_CLUT9_STATICCONTROL_SHDEN, 1));

	imxdpuv1_write(imxdpu, IMXDPUV1_CLUT9_UNSHADOWEDCONTROL,
		IMXDPUV1_SET_FIELD(IMXDPUV1_CLUT9_UNSHADOWEDCONTROL_B_EN,
			IMXDPUV1_CLUT9_UNSHADOWEDCONTROL_B_EN__ENABLE)
		| IMXDPUV1_SET_FIELD(IMXDPUV1_CLUT9_UNSHADOWEDCONTROL_G_EN,
			IMXDPUV1_CLUT9_UNSHADOWEDCONTROL_G_EN__ENABLE)
		| IMXDPUV1_SET_FIELD(IMXDPUV1_CLUT9_UNSHADOWEDCONTROL_R_EN,
			IMXDPUV1_CLUT9_UNSHADOWEDCONTROL_R_EN__ENABLE));

	/* IMXDPUV1_MATRIX9_STATICCONTROL       */
	imxdpuv1_write(imxdpu, IMXDPUV1_MATRIX9_STATICCONTROL,
		IMXDPUV1_SET_FIELD(IMXDPUV1_MATRIX9_STATICCONTROL_SHDEN, 1));

	/* IMXDPUV1_HSCALER9_STATICCONTROL      */
	imxdpuv1_write(imxdpu, IMXDPUV1_HSCALER9_STATICCONTROL,
		IMXDPUV1_SET_FIELD(IMXDPUV1_HSCALER9_STATICCONTROL_SHDEN, 1));

	/* IMXDPUV1_VSCALER9_STATICCONTROL      */
	imxdpuv1_write(imxdpu, IMXDPUV1_VSCALER9_STATICCONTROL,
		IMXDPUV1_SET_FIELD(IMXDPUV1_VSCALER9_STATICCONTROL_SHDEN, 1));

	/* IMXDPUV1_FILTER9_STATICCONTROL       */
	imxdpuv1_write(imxdpu, IMXDPUV1_FILTER9_STATICCONTROL,
		IMXDPUV1_SET_FIELD(IMXDPUV1_FILTER9_STATICCONTROL_SHDEN, 1));

	/* IMXDPUV1_BLITBLEND9_STATICCONTROL    */
	imxdpuv1_write(imxdpu, IMXDPUV1_BLITBLEND9_STATICCONTROL,
		IMXDPUV1_SET_FIELD(IMXDPUV1_BLITBLEND9_STATICCONTROL_SHDEN, 1));

	/* IMXDPUV1_STORE9_STATICCONTROL        */
	imxdpuv1_write(imxdpu, IMXDPUV1_STORE9_STATICCONTROL,
		IMXDPUV1_SET_FIELD(IMXDPUV1_STORE9_STATICCONTROL_SHDEN, 1) |
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_STORE9_STATICCONTROL_BASEADDRESSAUTOUPDATE, 1));

	/* IMXDPUV1_CONSTFRAME0_STATICCONTROL    */
	imxdpuv1_write(imxdpu, IMXDPUV1_CONSTFRAME0_STATICCONTROL,
		IMXDPUV1_SET_FIELD(IMXDPUV1_CONSTFRAME0_STATICCONTROL_SHDEN, 1));

	/* IMXDPUV1_EXTDST0_STATICCONTROL        */
	imxdpuv1_write(imxdpu, IMXDPUV1_EXTDST0_STATICCONTROL,
		IMXDPUV1_SET_FIELD(IMXDPUV1_EXTDST0_STATICCONTROL_SHDEN, 1) |
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_EXTDST0_STATICCONTROL_PERFCOUNTMODE, 0) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_EXTDST0_STATICCONTROL_KICK_MODE,
			IMXDPUV1_EXTDST0_STATICCONTROL_KICK_MODE__EXTERNAL));

	/* todo: IMXDPUV1_CONSTFRAME4_STATICCONTROL    */
	/* todo: IMXDPUV1_EXTDST4_STATICCONTROL        */

	/* IMXDPUV1_CONSTFRAME1_STATICCONTROL    */
	imxdpuv1_write(imxdpu, IMXDPUV1_CONSTFRAME1_STATICCONTROL,
		IMXDPUV1_SET_FIELD(IMXDPUV1_CONSTFRAME1_STATICCONTROL_SHDEN, 1));

	/* IMXDPUV1_EXTDST1_STATICCONTROL        */
	imxdpuv1_write(imxdpu, IMXDPUV1_EXTDST1_STATICCONTROL,
		IMXDPUV1_SET_FIELD(IMXDPUV1_EXTDST1_STATICCONTROL_SHDEN, 1) |
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_EXTDST1_STATICCONTROL_PERFCOUNTMODE, 0) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_EXTDST1_STATICCONTROL_KICK_MODE,
			IMXDPUV1_EXTDST1_STATICCONTROL_KICK_MODE__EXTERNAL));

	/* todo: IMXDPUV1_CONSTFRAME5_STATICCONTROL    */
	/* todo: IMXDPUV1_EXTDST5_STATICCONTROL        */
#ifdef IMXDPUV1_VERSION_0
	/* IMXDPUV1_EXTSRC4_STATICCONTROL        */
	imxdpuv1_write(imxdpu, IMXDPUV1_EXTSRC4_STATICCONTROL,
		IMXDPUV1_SET_FIELD(IMXDPUV1_EXTSRC4_STATICCONTROL_SHDEN, 1) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_EXTSRC4_STATICCONTROL_STARTSEL,
			IMXDPUV1_EXTSRC4_STATICCONTROL_STARTSEL__LOCAL));

	/* IMXDPUV1_STORE4_STATICCONTROL         */
	imxdpuv1_write(imxdpu, IMXDPUV1_STORE4_STATICCONTROL,
		IMXDPUV1_SET_FIELD(IMXDPUV1_STORE4_STATICCONTROL_SHDEN, 1) |
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_STORE4_STATICCONTROL_BASEADDRESSAUTOUPDATE, 1));

	/* IMXDPUV1_EXTSRC5_STATICCONTROL        */
	imxdpuv1_write(imxdpu, IMXDPUV1_EXTSRC5_STATICCONTROL,
		IMXDPUV1_SET_FIELD(IMXDPUV1_EXTSRC5_STATICCONTROL_SHDEN, 1) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_EXTSRC5_STATICCONTROL_STARTSEL,
			IMXDPUV1_EXTSRC5_STATICCONTROL_STARTSEL__LOCAL));

	/* IMXDPUV1_STORE5_STATICCONTROL         */
	imxdpuv1_write(imxdpu, IMXDPUV1_STORE5_STATICCONTROL,
		IMXDPUV1_SET_FIELD(IMXDPUV1_STORE5_STATICCONTROL_SHDEN, 1) |
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_STORE5_STATICCONTROL_BASEADDRESSAUTOUPDATE, 1));

	/* IMXDPUV1_FETCHDECODE2_STATICCONTROL   */
	imxdpuv1_write(imxdpu, IMXDPUV1_FETCHDECODE2_STATICCONTROL,
		IMXDPUV1_SET_FIELD(IMXDPUV1_FETCHDECODE2_STATICCONTROL_SHDEN, 1) |
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_FETCHDECODE2_STATICCONTROL_BASEADDRESSAUTOUPDATE, 0));

	/* IMXDPUV1_FETCHDECODE3_STATICCONTROL   */
	imxdpuv1_write(imxdpu, IMXDPUV1_FETCHDECODE3_STATICCONTROL,
		IMXDPUV1_SET_FIELD(IMXDPUV1_FETCHDECODE3_STATICCONTROL_SHDEN, 1) |
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_FETCHDECODE3_STATICCONTROL_BASEADDRESSAUTOUPDATE, 0));
#endif
	/* IMXDPUV1_FETCHWARP2_STATICCONTROL     */
	imxdpuv1_write(imxdpu, IMXDPUV1_FETCHWARP2_STATICCONTROL,
		IMXDPUV1_SET_FIELD(IMXDPUV1_FETCHWARP2_STATICCONTROL_SHDEN, 1) |
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_FETCHWARP2_STATICCONTROL_BASEADDRESSAUTOUPDATE, 0) |
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_FETCHWARP2_STATICCONTROL_SHDLDREQSTICKY, 0));

	/* IMXDPUV1_FETCHECO2_STATICCONTROL      */
	imxdpuv1_write(imxdpu, IMXDPUV1_FETCHECO9_STATICCONTROL,
		IMXDPUV1_SET_FIELD(IMXDPUV1_FETCHECO9_STATICCONTROL_SHDEN, 1) |
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_FETCHECO9_STATICCONTROL_BASEADDRESSAUTOUPDATE, 0));

	/* IMXDPUV1_FETCHDECODE0_STATICCONTROL   */
	imxdpuv1_write(imxdpu, IMXDPUV1_FETCHDECODE0_STATICCONTROL,
		IMXDPUV1_SET_FIELD(IMXDPUV1_FETCHDECODE0_STATICCONTROL_SHDEN, 1) |
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_FETCHDECODE0_STATICCONTROL_BASEADDRESSAUTOUPDATE,
			0));

	/* IMXDPUV1_FETCHECO0_STATICCONTROL      */
	imxdpuv1_write(imxdpu, IMXDPUV1_FETCHECO0_STATICCONTROL,
		IMXDPUV1_SET_FIELD(IMXDPUV1_FETCHECO0_STATICCONTROL_SHDEN, 1) |
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_FETCHECO0_STATICCONTROL_BASEADDRESSAUTOUPDATE, 0));

	/* IMXDPUV1_FETCHDECODE1_STATICCONTROL   */
	imxdpuv1_write(imxdpu, IMXDPUV1_FETCHDECODE1_STATICCONTROL,
		IMXDPUV1_SET_FIELD(IMXDPUV1_FETCHDECODE1_STATICCONTROL_SHDEN, 1) |
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_FETCHDECODE1_STATICCONTROL_BASEADDRESSAUTOUPDATE, 0));

	/* IMXDPUV1_FETCHECO1_STATICCONTROL      */
	imxdpuv1_write(imxdpu, IMXDPUV1_FETCHECO1_STATICCONTROL,
		IMXDPUV1_SET_FIELD(IMXDPUV1_FETCHECO1_STATICCONTROL_SHDEN, 1) |
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_FETCHECO1_STATICCONTROL_BASEADDRESSAUTOUPDATE, 0));

	/* IMXDPUV1_FETCHLAYER0_STATICCONTROL    */
	imxdpuv1_write(imxdpu, IMXDPUV1_FETCHLAYER0_STATICCONTROL,
		IMXDPUV1_SET_FIELD(IMXDPUV1_FETCHLAYER0_STATICCONTROL_SHDEN, 1) |
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_FETCHLAYER0_STATICCONTROL_BASEADDRESSAUTOUPDATE, 0) |
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_FETCHLAYER0_STATICCONTROL_SHDLDREQSTICKY, 0));
#ifdef IMXDPUV1_VERSION_0
	/* IMXDPUV1_FETCHLAYER1_STATICCONTROL    */
	imxdpuv1_write(imxdpu, IMXDPUV1_FETCHLAYER1_STATICCONTROL,
		IMXDPUV1_SET_FIELD(IMXDPUV1_FETCHLAYER1_STATICCONTROL_SHDEN, 1) |
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_FETCHLAYER1_STATICCONTROL_BASEADDRESSAUTOUPDATE, 0) |
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_FETCHLAYER1_STATICCONTROL_SHDLDREQSTICKY, 0));

	/* IMXDPUV1_GAMMACOR4_STATICCONTROL      */
	imxdpuv1_write(imxdpu, IMXDPUV1_GAMMACOR4_STATICCONTROL,
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_GAMMACOR4_STATICCONTROL_BLUEWRITEENABLE, 1) |
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_GAMMACOR4_STATICCONTROL_GREENWRITEENABLE, 1) |
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_GAMMACOR4_STATICCONTROL_REDWRITEENABLE, 1));
#endif
	/* todo: IMXDPUV1_MATRIX4_STATICCONTROL        */
	/* todo: IMXDPUV1_HSCALER4_STATICCONTROL       */
	/* todo: IMXDPUV1_VSCALER4_STATICCONTROL       */
#ifdef IMXDPUV1_VERSION_0
	/* IMXDPUV1_GAMMACOR5_STATICCONTROL      */
	imxdpuv1_write(imxdpu, IMXDPUV1_GAMMACOR5_STATICCONTROL,
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_GAMMACOR5_STATICCONTROL_BLUEWRITEENABLE, 1) |
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_GAMMACOR5_STATICCONTROL_GREENWRITEENABLE, 1) |
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_GAMMACOR5_STATICCONTROL_REDWRITEENABLE, 1));
#endif
	/* todo: IMXDPUV1_MATRIX5_STATICCONTROL        */
	/* todo: IMXDPUV1_HSCALER5_STATICCONTROL       */
	/* todo: IMXDPUV1_VSCALER5_STATICCONTROL       */

	/* IMXDPUV1_LAYERBLEND0_STATICCONTROL    */
	imxdpuv1_write(imxdpu, IMXDPUV1_LAYERBLEND0_STATICCONTROL,
		IMXDPUV1_SET_FIELD(IMXDPUV1_LAYERBLEND0_STATICCONTROL_SHDEN, 1) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_LAYERBLEND0_STATICCONTROL_SHDLDSEL,
			IMXDPUV1_LAYERBLEND0_STATICCONTROL_SHDLDSEL__SECONDARY) |
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_LAYERBLEND0_STATICCONTROL_SHDTOKSEL,
			IMXDPUV1_LAYERBLEND0_STATICCONTROL_SHDTOKSEL__BOTH));

	/* IMXDPUV1_LAYERBLEND1_STATICCONTROL    */
	imxdpuv1_write(imxdpu, IMXDPUV1_LAYERBLEND1_STATICCONTROL,
		IMXDPUV1_SET_FIELD(IMXDPUV1_LAYERBLEND1_STATICCONTROL_SHDEN, 1) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_LAYERBLEND1_STATICCONTROL_SHDLDSEL,
			IMXDPUV1_LAYERBLEND1_STATICCONTROL_SHDLDSEL__SECONDARY) |
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_LAYERBLEND1_STATICCONTROL_SHDTOKSEL,
			IMXDPUV1_LAYERBLEND1_STATICCONTROL_SHDTOKSEL__BOTH));

	/* IMXDPUV1_LAYERBLEND2_STATICCONTROL    */
	imxdpuv1_write(imxdpu, IMXDPUV1_LAYERBLEND2_STATICCONTROL,
		IMXDPUV1_SET_FIELD(IMXDPUV1_LAYERBLEND2_STATICCONTROL_SHDEN, 1) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_LAYERBLEND2_STATICCONTROL_SHDLDSEL,
			IMXDPUV1_LAYERBLEND2_STATICCONTROL_SHDLDSEL__SECONDARY) |
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_LAYERBLEND2_STATICCONTROL_SHDTOKSEL,
			IMXDPUV1_LAYERBLEND2_STATICCONTROL_SHDTOKSEL__BOTH));

	/* IMXDPUV1_LAYERBLEND3_STATICCONTROL    */
	imxdpuv1_write(imxdpu, IMXDPUV1_LAYERBLEND3_STATICCONTROL,
		IMXDPUV1_SET_FIELD(IMXDPUV1_LAYERBLEND3_STATICCONTROL_SHDEN, 1) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_LAYERBLEND3_STATICCONTROL_SHDLDSEL,
			IMXDPUV1_LAYERBLEND3_STATICCONTROL_SHDLDSEL__SECONDARY) |
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_LAYERBLEND3_STATICCONTROL_SHDTOKSEL,
			IMXDPUV1_LAYERBLEND3_STATICCONTROL_SHDTOKSEL__BOTH));

#ifdef IMXDPUV1_VERSION_0
	/* IMXDPUV1_LAYERBLEND4_STATICCONTROL    */
	imxdpuv1_write(imxdpu, IMXDPUV1_LAYERBLEND4_STATICCONTROL,
		IMXDPUV1_SET_FIELD(IMXDPUV1_LAYERBLEND4_STATICCONTROL_SHDEN, 1) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_LAYERBLEND4_STATICCONTROL_SHDLDSEL,
			IMXDPUV1_LAYERBLEND4_STATICCONTROL_SHDLDSEL__SECONDARY) |
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_LAYERBLEND4_STATICCONTROL_SHDTOKSEL,
			IMXDPUV1_LAYERBLEND4_STATICCONTROL_SHDTOKSEL__BOTH));

	/* IMXDPUV1_LAYERBLEND4_STATICCONTROL    */
	imxdpuv1_write(imxdpu, IMXDPUV1_LAYERBLEND4_STATICCONTROL,
		IMXDPUV1_SET_FIELD(IMXDPUV1_LAYERBLEND4_STATICCONTROL_SHDEN, 0) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_LAYERBLEND4_STATICCONTROL_SHDLDSEL,
			IMXDPUV1_LAYERBLEND4_STATICCONTROL_SHDLDSEL__SECONDARY) |
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_LAYERBLEND4_STATICCONTROL_SHDTOKSEL,
			IMXDPUV1_LAYERBLEND4_STATICCONTROL_SHDTOKSEL__BOTH));

	/* IMXDPUV1_LAYERBLEND5_STATICCONTROL    */
	imxdpuv1_write(imxdpu, IMXDPUV1_LAYERBLEND5_STATICCONTROL,
		IMXDPUV1_SET_FIELD(IMXDPUV1_LAYERBLEND5_STATICCONTROL_SHDEN, 1) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_LAYERBLEND5_STATICCONTROL_SHDLDSEL,
			IMXDPUV1_LAYERBLEND5_STATICCONTROL_SHDLDSEL__SECONDARY) |
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_LAYERBLEND5_STATICCONTROL_SHDTOKSEL,
			IMXDPUV1_LAYERBLEND5_STATICCONTROL_SHDTOKSEL__BOTH));

	/* IMXDPUV1_LAYERBLEND6_STATICCONTROL    */
	imxdpuv1_write(imxdpu, IMXDPUV1_LAYERBLEND6_STATICCONTROL,
		IMXDPUV1_SET_FIELD(IMXDPUV1_LAYERBLEND6_STATICCONTROL_SHDEN, 1) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_LAYERBLEND6_STATICCONTROL_SHDLDSEL,
			IMXDPUV1_LAYERBLEND6_STATICCONTROL_SHDLDSEL__SECONDARY) |
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_LAYERBLEND6_STATICCONTROL_SHDTOKSEL,
			IMXDPUV1_LAYERBLEND6_STATICCONTROL_SHDTOKSEL__BOTH));
#endif
	/* todo: IMXDPUV1_EXTSRC0_STATICCONTROL        */
	/* todo: IMXDPUV1_EXTSRC1_STATICCONTROL        */
	/* todo: IMXDPUV1_MATRIX0_STATICCONTROL        */
	/* IMXDPUV1_GAMMACOR0_STATICCONTROL      */
	imxdpuv1_write(imxdpu, IMXDPUV1_GAMMACOR0_STATICCONTROL,
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_GAMMACOR1_STATICCONTROL_BLUEWRITEENABLE, 1) |
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_GAMMACOR1_STATICCONTROL_GREENWRITEENABLE, 1) |
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_GAMMACOR1_STATICCONTROL_REDWRITEENABLE, 1));
	/* todo: IMXDPUV1_SIG0_STATICCONTROL           */
	/* todo: IMXDPUV1_MATRIX1_STATICCONTROL        */
	/* IMXDPUV1_GAMMACOR1_STATICCONTROL      */
	imxdpuv1_write(imxdpu, IMXDPUV1_GAMMACOR1_STATICCONTROL,
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_GAMMACOR1_STATICCONTROL_BLUEWRITEENABLE, 1) |
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_GAMMACOR1_STATICCONTROL_GREENWRITEENABLE, 1) |
		IMXDPUV1_SET_FIELD(
			IMXDPUV1_GAMMACOR1_STATICCONTROL_REDWRITEENABLE, 1));
	/* IMXDPUV1_SIG1_STATICCONTROL           */

	imxdpuv1_init_irqs(imxdpuv1_id);

	return ret;
}

int imxdpuv1_init_sync_panel(int8_t imxdpuv1_id,
	int8_t disp,
	uint32_t pixel_fmt, struct imxdpuv1_videomode mode)
{
	int ret = 0;
	IMXDPUV1_TRACE("%s()\n", __func__);
	return ret;
}

int imxdpuv1_uninit_sync_panel(int8_t imxdpuv1_id, int8_t disp)
{
	int ret = 0;
	IMXDPUV1_TRACE("%s()\n", __func__);
	return ret;
}

int imxdpuv1_reset_disp_panel(int8_t imxdpuv1_id, int8_t disp)
{
	int ret = 0;
	IMXDPUV1_TRACE("%s()\n", __func__);
	return ret;
}

/*!
 * This function initializes the display
 *
 * @param	imxdpuv1_id	id of the diplay unit
 * @param       disp		id of the diplay output pipe
 *
 * @return      This function returns 0 on success or negative error code on
 *      	fail.
 */
int imxdpuv1_disp_init(int8_t imxdpuv1_id, int8_t disp)
{
	int ret = 0;
	struct imxdpuv1_soc *imxdpu;
	struct imxdpuv1_videomode *mode;
	int reg = 0;
	IMXDPUV1_TRACE("%s()\n", __func__);

	if (!((imxdpuv1_id >= 0) && (imxdpuv1_id < IMXDPUV1_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpuv1_array[imxdpuv1_id];
	mode = &imxdpu->video_mode[disp];
	/*imxdpuv1_disp_dump_mode(&imxdpu->video_mode[disp]);*/

	if (disp == 0) {
#ifdef IMXDPUV1_TCON0_MAP_24BIT_0_23
		/* Static  24-bit TCON bit mapping for FPGA */
		imxdpuv1_write(imxdpu, IMXDPUV1_TCON0_MAPBIT7_4, 0x1d1c1b1a);
		imxdpuv1_write(imxdpu, IMXDPUV1_TCON0_MAPBIT3_0, 0x19181716);
		imxdpuv1_write(imxdpu, IMXDPUV1_TCON0_MAPBIT15_12, 0x13121110);
		imxdpuv1_write(imxdpu, IMXDPUV1_TCON0_MAPBIT11_8, 0x0f0e0d0c);
		imxdpuv1_write(imxdpu, IMXDPUV1_TCON0_MAPBIT23_20, 0x09080706);
		imxdpuv1_write(imxdpu, IMXDPUV1_TCON0_MAPBIT19_16, 0x05040302);
#else
		/*  tcon mapping
		  *  RR RRRR RRRR GGGG GGGG GGBB BBBB BBBB
		  *  98 7654 3210 9876 5432 1098 7654 3210
		  *  bits
		  *  00 0000 0000 1111 1111 1122 2222 2222
		  *  98 7654 3210 8765 5432 1098 7654 3210
		  */
		/* 30-bit timing controller setup */
		imxdpuv1_write(imxdpu, IMXDPUV1_TCON0_MAPBIT31_28, 0x00000908);
		imxdpuv1_write(imxdpu, IMXDPUV1_TCON0_MAPBIT27_24, 0x07060504);
		imxdpuv1_write(imxdpu, IMXDPUV1_TCON0_MAPBIT23_20, 0x03020100);
		imxdpuv1_write(imxdpu, IMXDPUV1_TCON0_MAPBIT19_16, 0x13121110);
		imxdpuv1_write(imxdpu, IMXDPUV1_TCON0_MAPBIT15_12, 0x0f0e0d0c);
		imxdpuv1_write(imxdpu, IMXDPUV1_TCON0_MAPBIT11_8,  0x0b0a1d1c);
		imxdpuv1_write(imxdpu, IMXDPUV1_TCON0_MAPBIT7_4,   0x1b1a1918);
		imxdpuv1_write(imxdpu, IMXDPUV1_TCON0_MAPBIT3_0,   0x17161514);

#endif

		/* set data enable polarity */
		if (mode->flags & IMXDPUV1_MODE_FLAGS_HSYNC_POL)
			reg = IMXDPUV1_SET_FIELD(
				IMXDPUV1_DISENGCFG_POLARITYCTRL0_POLHS0,
				IMXDPUV1_DISENGCFG_POLARITYCTRL0_POLHS0__HIGH);
		else
			reg = IMXDPUV1_SET_FIELD(
				IMXDPUV1_DISENGCFG_POLARITYCTRL0_POLHS0,
				IMXDPUV1_DISENGCFG_POLARITYCTRL0_POLHS0__LOW);

		if (mode->flags & IMXDPUV1_MODE_FLAGS_VSYNC_POL)
			reg |= IMXDPUV1_SET_FIELD(
				IMXDPUV1_DISENGCFG_POLARITYCTRL0_POLVS0,
				IMXDPUV1_DISENGCFG_POLARITYCTRL0_POLVS0__HIGH);
		else
			reg |= IMXDPUV1_SET_FIELD(
				IMXDPUV1_DISENGCFG_POLARITYCTRL0_POLVS0,
				IMXDPUV1_DISENGCFG_POLARITYCTRL0_POLVS0__LOW);

		if (mode->flags & IMXDPUV1_MODE_FLAGS_DE_POL)
			reg |= IMXDPUV1_SET_FIELD(
				IMXDPUV1_DISENGCFG_POLARITYCTRL0_POLEN0,
				IMXDPUV1_DISENGCFG_POLARITYCTRL0_POLEN0__HIGH);
		else
			reg |= IMXDPUV1_SET_FIELD(
				IMXDPUV1_DISENGCFG_POLARITYCTRL0_POLEN0,
				IMXDPUV1_DISENGCFG_POLARITYCTRL0_POLEN0__LOW);

		imxdpuv1_write(imxdpu, IMXDPUV1_DISENGCFG_POLARITYCTRL0, reg);
		/* printf("polreg=0x%x\n", imxdpuv1_read(imxdpu, IMXDPUV1_DISENGCFG_POLARITYCTRL0)); */

	} else if (disp == 1) {
#ifdef IMXDPUV1_TCON1_MAP_24BIT_0_23
		/* Static TCON bit mapping */
		imxdpuv1_write(imxdpu, IMXDPUV1_TCON1_MAPBIT7_4, 0x1d1c1b1a);
		imxdpuv1_write(imxdpu, IMXDPUV1_TCON1_MAPBIT3_0, 0x19181716);
		imxdpuv1_write(imxdpu, IMXDPUV1_TCON1_MAPBIT15_12, 0x13121110);
		imxdpuv1_write(imxdpu, IMXDPUV1_TCON1_MAPBIT11_8, 0x0f0e0d0c);
		imxdpuv1_write(imxdpu, IMXDPUV1_TCON1_MAPBIT23_20, 0x09080706);
		imxdpuv1_write(imxdpu, IMXDPUV1_TCON1_MAPBIT19_16, 0x05040302);
#else
		/*  tcon mapping
		  *  RR RRRR RRRR GGGG GGGG GGBB BBBB BBBB
		  *  98 7654 3210 9876 5432 1098 7654 3210
		  *  bits
		  *  00 0000 0000 1111 1111 1122 2222 2222
		  *  98 7654 3210 8765 5432 1098 7654 3210
		  */
		/* 30-bit timing controller setup */
		imxdpuv1_write(imxdpu, IMXDPUV1_TCON1_MAPBIT31_28, 0x00000908);
		imxdpuv1_write(imxdpu, IMXDPUV1_TCON1_MAPBIT27_24, 0x07060504);
		imxdpuv1_write(imxdpu, IMXDPUV1_TCON1_MAPBIT23_20, 0x03020100);
		imxdpuv1_write(imxdpu, IMXDPUV1_TCON1_MAPBIT19_16, 0x13121110);
		imxdpuv1_write(imxdpu, IMXDPUV1_TCON1_MAPBIT15_12, 0x0f0e0d0c);
		imxdpuv1_write(imxdpu, IMXDPUV1_TCON1_MAPBIT11_8,  0x0b0a1d1c);
		imxdpuv1_write(imxdpu, IMXDPUV1_TCON1_MAPBIT7_4,   0x1b1a1918);
		imxdpuv1_write(imxdpu, IMXDPUV1_TCON1_MAPBIT3_0,   0x17161514);
#endif
		/* set data enable polarity */
		if (mode->flags & IMXDPUV1_MODE_FLAGS_HSYNC_POL)
			reg = IMXDPUV1_SET_FIELD(
				IMXDPUV1_DISENGCFG_POLARITYCTRL1_POLHS1,
				IMXDPUV1_DISENGCFG_POLARITYCTRL1_POLHS1__HIGH);
		else
			reg = IMXDPUV1_SET_FIELD(
				IMXDPUV1_DISENGCFG_POLARITYCTRL1_POLHS1,
				IMXDPUV1_DISENGCFG_POLARITYCTRL1_POLHS1__LOW);

		if (mode->flags & IMXDPUV1_MODE_FLAGS_VSYNC_POL)
			reg |= IMXDPUV1_SET_FIELD(
				IMXDPUV1_DISENGCFG_POLARITYCTRL1_POLVS1,
				IMXDPUV1_DISENGCFG_POLARITYCTRL1_POLVS1__HIGH);
		else
			reg |= IMXDPUV1_SET_FIELD(
				IMXDPUV1_DISENGCFG_POLARITYCTRL1_POLVS1,
				IMXDPUV1_DISENGCFG_POLARITYCTRL1_POLVS1__LOW);

		if (mode->flags & IMXDPUV1_MODE_FLAGS_DE_POL)
			reg |= IMXDPUV1_SET_FIELD(
				IMXDPUV1_DISENGCFG_POLARITYCTRL1_POLEN1,
				IMXDPUV1_DISENGCFG_POLARITYCTRL1_POLEN1__HIGH);
		else
			reg |= IMXDPUV1_SET_FIELD(
				IMXDPUV1_DISENGCFG_POLARITYCTRL1_POLEN1,
				IMXDPUV1_DISENGCFG_POLARITYCTRL1_POLEN1__LOW);

		imxdpuv1_write(imxdpu, IMXDPUV1_DISENGCFG_POLARITYCTRL1, reg);
		/* printf("polreg=0x%x\n", imxdpuv1_read(imxdpu, IMXDPUV1_DISENGCFG_POLARITYCTRL1)); */

	} else {
		return -EINVAL;
	}
	/* todo: initialize prefetch */

	return ret;
}

int imxdpuv1_disp_setup_tcon_bypass_mode(
	int8_t imxdpuv1_id,
	int8_t disp,
	const struct imxdpuv1_videomode *mode)
{
	struct imxdpuv1_soc *imxdpu;
	uint32_t b_off;     /* block offset for tcon generator */

	IMXDPUV1_TRACE("%s()\n", __func__);

	if (!((imxdpuv1_id >= 0) && (imxdpuv1_id < IMXDPUV1_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpuv1_array[imxdpuv1_id];

	if (disp == 0) {
		b_off = IMXDPUV1_TCON0_LOCKUNLOCK;
	} else if (disp == 1) {
		b_off = IMXDPUV1_TCON1_LOCKUNLOCK;
	} else {
		return -EINVAL;
	}

	imxdpuv1_write(imxdpu, b_off + IMXDPUV1_TCON0_TCON_CTRL_OFFSET,
		IMXDPUV1_SET_FIELD(IMXDPUV1_TCON0_TCON_CTRL_LVDS_BALANCE,
			IMXDPUV1_TCON0_TCON_CTRL_LVDS_BALANCE__BALANCED) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_TCON0_TCON_CTRL_MINILVDS_OPCODE,
			IMXDPUV1_TCON0_TCON_CTRL_MINILVDS_OPCODE__MODE_4PAIRS) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_TCON0_TCON_CTRL_SPLITPOSITION,
			0x140));
	/* setup hsync */
	imxdpuv1_write(imxdpu, b_off + IMXDPUV1_TCON0_SPG0POSON_OFFSET,
		IMXDPUV1_SET_FIELD(IMXDPUV1_TCON0_SPG0POSON_SPGPSON_X0, mode->hlen + mode->hfp));
	imxdpuv1_write(imxdpu, b_off + IMXDPUV1_TCON0_SPG0MASKON_OFFSET, 0xffff);
	imxdpuv1_write(imxdpu, b_off + IMXDPUV1_TCON0_SPG0POSOFF_OFFSET,
		IMXDPUV1_SET_FIELD(IMXDPUV1_TCON0_SPG0POSOFF_SPGPSOFF_X0, mode->hlen + mode->hfp + mode->hsync));
	imxdpuv1_write(imxdpu, b_off + IMXDPUV1_TCON0_SPG0MASKOFF_OFFSET, 0xffff);

	imxdpuv1_write(imxdpu, b_off + IMXDPUV1_TCON0_SMX0SIGS_OFFSET,
		IMXDPUV1_SET_FIELD(IMXDPUV1_TCON0_SMX0SIGS_SMX0SIGS_S0, 2));
	imxdpuv1_write(imxdpu, b_off + IMXDPUV1_TCON0_SMX0FCTTABLE_OFFSET, 1);

	/* Setup Vsync */
	imxdpuv1_write(imxdpu, b_off + IMXDPUV1_TCON0_SPG1POSON_OFFSET,
		IMXDPUV1_SET_FIELD(IMXDPUV1_TCON0_SPG1POSON_SPGPSON_X1, mode->hlen + mode->hfp + mode->hsync) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_TCON0_SPG1POSON_SPGPSON_Y1, mode->vlen + mode->vfp - 1));
	imxdpuv1_write(imxdpu, b_off + IMXDPUV1_TCON0_SPG1MASKON_OFFSET, 0);
	imxdpuv1_write(imxdpu, b_off + IMXDPUV1_TCON0_SPG1POSOFF_OFFSET,
		IMXDPUV1_SET_FIELD(IMXDPUV1_TCON0_SPG1POSOFF_SPGPSOFF_X1, mode->hlen + mode->hfp + mode->hsync)|
		IMXDPUV1_SET_FIELD(IMXDPUV1_TCON0_SPG1POSOFF_SPGPSOFF_Y1, mode->vlen + mode->vfp + mode->vsync - 1));
	imxdpuv1_write(imxdpu, b_off + IMXDPUV1_TCON0_SPG1MASKOFF_OFFSET, 0);

	imxdpuv1_write(imxdpu, b_off + IMXDPUV1_TCON0_SMX1SIGS_OFFSET,
		IMXDPUV1_SET_FIELD(IMXDPUV1_TCON0_SMX1SIGS_SMX1SIGS_S0, 3));
	imxdpuv1_write(imxdpu, b_off + IMXDPUV1_TCON0_SMX1FCTTABLE_OFFSET, 1);

	/* data enable horizontal */
	imxdpuv1_write(imxdpu, b_off + IMXDPUV1_TCON0_SPG2POSON_OFFSET, 0);
	imxdpuv1_write(imxdpu, b_off + IMXDPUV1_TCON0_SPG2MASKON_OFFSET, 0xffff);
	imxdpuv1_write(imxdpu, b_off + IMXDPUV1_TCON0_SPG2POSOFF_OFFSET,
		IMXDPUV1_SET_FIELD(IMXDPUV1_TCON0_SPG2POSOFF_SPGPSOFF_X2, mode->hlen));
	imxdpuv1_write(imxdpu, b_off + IMXDPUV1_TCON0_SPG2MASKOFF_OFFSET, 0xffff);
	/* data enable vertical  */
	imxdpuv1_write(imxdpu, b_off + IMXDPUV1_TCON0_SPG3POSON_OFFSET, 0);
	imxdpuv1_write(imxdpu, b_off + IMXDPUV1_TCON0_SPG3MASKON_OFFSET, 0x7fff0000);
	imxdpuv1_write(imxdpu, b_off + IMXDPUV1_TCON0_SPG3POSOFF_OFFSET,
		IMXDPUV1_SET_FIELD(IMXDPUV1_TCON0_SPG3POSOFF_SPGPSOFF_X3, 0) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_TCON0_SPG3POSOFF_SPGPSOFF_Y3, mode->vlen));
	imxdpuv1_write(imxdpu, b_off + IMXDPUV1_TCON0_SPG3MASKOFF_OFFSET, 0x7fff0000);

	/* use both SPG2 and SPG3 to generate data enable */
	imxdpuv1_write(imxdpu, b_off + IMXDPUV1_TCON0_SMX2SIGS_OFFSET,
		IMXDPUV1_SET_FIELD(IMXDPUV1_TCON0_SMX2SIGS_SMX2SIGS_S0, 4)|
		IMXDPUV1_SET_FIELD(IMXDPUV1_TCON0_SMX2SIGS_SMX2SIGS_S1, 5));
	imxdpuv1_write(imxdpu, b_off + IMXDPUV1_TCON0_SMX2FCTTABLE_OFFSET, 8);

	/* shadow load trigger (aka kachunk) */
	imxdpuv1_write(imxdpu, b_off + IMXDPUV1_TCON0_SPG4POSON_OFFSET,
		IMXDPUV1_SET_FIELD(IMXDPUV1_TCON0_SPG4POSON_SPGPSON_X4, 10) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_TCON0_SPG4POSON_SPGPSON_Y4, mode->vlen));
	imxdpuv1_write(imxdpu, b_off + IMXDPUV1_TCON0_SPG4MASKON_OFFSET, 0);
	imxdpuv1_write(imxdpu, b_off + IMXDPUV1_TCON0_SPG4POSOFF_OFFSET,
		IMXDPUV1_SET_FIELD(IMXDPUV1_TCON0_SPG4POSOFF_SPGPSOFF_X4, 26) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_TCON0_SPG4POSOFF_SPGPSOFF_Y4, mode->vlen));
	imxdpuv1_write(imxdpu, b_off + IMXDPUV1_TCON0_SPG4MASKOFF_OFFSET, 0);

	imxdpuv1_write(imxdpu, b_off + IMXDPUV1_TCON0_SMX3SIGS_OFFSET,
		IMXDPUV1_SET_FIELD(IMXDPUV1_TCON0_SMX3SIGS_SMX3SIGS_S0, 6));
	imxdpuv1_write(imxdpu, b_off + IMXDPUV1_TCON0_SMX3FCTTABLE_OFFSET, 2);

	return 0;
}

/*!
 * This function sets up the frame generator
 *
 * @param	imxdpuv1_id	id of the diplay unit
 * @param       disp		id of the diplay output pipe
 * @param       enable  	state to set frame generator to
 * @param	mode    	to set the display to
 * @param       cc_red		constant color red
 * @param       cc_green	constant color green
 * @param       cc_blue		constant color blue
 * @param       cc_alpha	constant color alpha
*
 * @return      This function returns 0 on success or negative error code on
 *      	fail.
 */
int imxdpuv1_disp_setup_frame_gen(
	int8_t imxdpuv1_id,
	int8_t disp,
	const struct imxdpuv1_videomode *mode,
	uint16_t cc_red,    /* 10 bits */
	uint16_t cc_green,  /* 10 bits */
	uint16_t cc_blue,   /* 10 bits */
	uint8_t cc_alpha,
	bool test_mode_enable)
{           /* 1 bits, yes 1 bit */
	int ret = 0;
	uint32_t b_off;     /* block offset for frame generator */
	uint32_t reg;
	struct imxdpuv1_soc *imxdpu;

	IMXDPUV1_TRACE("%s()\n", __func__);

	if (!((imxdpuv1_id >= 0) && (imxdpuv1_id < IMXDPUV1_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpuv1_array[imxdpuv1_id];

	if (disp == 0) {
		b_off = IMXDPUV1_FRAMEGEN0_LOCKUNLOCK;
	} else if (disp == 1) {
		b_off = IMXDPUV1_FRAMEGEN1_LOCKUNLOCK;
	} else {
		return -EINVAL;
	}

	/* todo:
	   add video mode sanity check here
	   check if LRSYNC is required
	 */

	if (mode->flags & IMXDPUV1_MODE_FLAGS_LRSYNC) {
		/* todo: here we need to use two outputs to make one */
		if (disp == 0) {
			reg = IMXDPUV1_SET_FIELD(
				IMXDPUV1_FRAMEGEN0_FGSTCTRL_FGSYNCMODE,
				IMXDPUV1_FRAMEGEN0_FGSTCTRL_FGSYNCMODE__MASTER);
		} else {
			reg = IMXDPUV1_SET_FIELD(
				IMXDPUV1_FRAMEGEN1_FGSTCTRL_FGSYNCMODE,
				IMXDPUV1_FRAMEGEN1_FGSTCTRL_FGSYNCMODE__SLAVE_CYC);
		}
	} else {
		reg = IMXDPUV1_SET_FIELD(
			IMXDPUV1_FRAMEGEN0_FGSTCTRL_FGSYNCMODE,
			IMXDPUV1_FRAMEGEN0_FGSTCTRL_FGSYNCMODE__OFF);
	}
	imxdpuv1_write(imxdpu, b_off + IMXDPUV1_FRAMEGEN0_FGSTCTRL_OFFSET, reg);

	reg = IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMEGEN0_HTCFG1_HACT, mode->hlen) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMEGEN0_HTCFG1_HTOTAL,
		(mode->hlen + mode->hfp + mode->hbp + mode->hsync - 1));
	imxdpuv1_write(imxdpu, b_off + IMXDPUV1_FRAMEGEN0_HTCFG1_OFFSET, reg);

	reg = IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMEGEN0_HTCFG2_HSYNC,
		mode->hsync - 1) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMEGEN0_HTCFG2_HSBP,
		mode->hbp + mode->hsync - 1) |
		/* shadow enable */
		IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMEGEN0_HTCFG2_HSEN, 1);
	imxdpuv1_write(imxdpu, b_off + IMXDPUV1_FRAMEGEN0_HTCFG2_OFFSET, reg);

	reg = IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMEGEN0_VTCFG1_VACT, mode->vlen) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMEGEN0_VTCFG1_VTOTAL,
		(mode->vlen + mode->vfp + mode->vbp + mode->vsync -
			1));
	imxdpuv1_write(imxdpu, b_off + IMXDPUV1_FRAMEGEN0_VTCFG1_OFFSET, reg);

	reg = IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMEGEN0_VTCFG2_VSYNC,
		mode->vsync - 1) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMEGEN0_VTCFG2_VSBP,
		mode->vbp + mode->vsync - 1) |
		/* shadow enable */
		IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMEGEN0_VTCFG2_VSEN, 1);
	imxdpuv1_write(imxdpu, b_off + IMXDPUV1_FRAMEGEN0_VTCFG2_OFFSET, reg);

	/* Interupt at position (0, vlen - 3) for end of frame interrupt */
	reg = IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMEGEN0_INT0CONFIG_INT0COL, 1) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMEGEN0_INT0CONFIG_INT0HSEN, 0) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMEGEN0_INT0CONFIG_INT0ROW,
		mode->vlen - 3) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMEGEN0_INT0CONFIG_INT0EN, 1);
	imxdpuv1_write(imxdpu, b_off + IMXDPUV1_FRAMEGEN0_INT0CONFIG_OFFSET, reg);

	/* Interupt at position 1, mode->vlen  */
	reg = IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMEGEN0_INT1CONFIG_INT1COL, 1) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMEGEN0_INT1CONFIG_INT1HSEN, 0) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMEGEN0_INT1CONFIG_INT1ROW,
		mode->vlen) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMEGEN0_INT1CONFIG_INT1EN, 1);
	imxdpuv1_write(imxdpu, b_off + IMXDPUV1_FRAMEGEN0_INT1CONFIG_OFFSET, reg);

	/* Interupt at position 2, mode->vlen */
	reg = IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMEGEN0_INT2CONFIG_INT2COL, 2) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMEGEN0_INT2CONFIG_INT2HSEN, 0) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMEGEN0_INT2CONFIG_INT2ROW,
		mode->vlen) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMEGEN0_INT2CONFIG_INT2EN, 1);
	imxdpuv1_write(imxdpu, b_off + IMXDPUV1_FRAMEGEN0_INT2CONFIG_OFFSET, reg);

	/* Interupt at position 3, mode->vlen */
	reg = IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMEGEN0_INT3CONFIG_INT3COL, 3) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMEGEN0_INT3CONFIG_INT3HSEN, 0) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMEGEN0_INT3CONFIG_INT3ROW,
		mode->vlen) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMEGEN0_INT3CONFIG_INT3EN, 1);
	imxdpuv1_write(imxdpu, b_off + IMXDPUV1_FRAMEGEN0_INT3CONFIG_OFFSET, reg);

	/* todo: these need to be checked
	   _SKICKCOL for verification: =(FW - 40) , for ref driver = 1 ?
	   _SKICKROW for verif.  =(FH - 1), ref driver = vlen-2
	 */
	reg = IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMEGEN0_SKICKCONFIG_SKICKCOL,
		mode->hlen - 40) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMEGEN0_SKICKCONFIG_SKICKINT1EN, 0) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMEGEN0_SKICKCONFIG_SKICKROW,
		mode->vlen + 1) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMEGEN0_SKICKCONFIG_SKICKEN, 1);
	imxdpuv1_write(imxdpu, b_off + IMXDPUV1_FRAMEGEN0_SKICKCONFIG_OFFSET, reg);

	reg = IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMEGEN0_PACFG_PSTARTX, 1) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMEGEN0_PACFG_PSTARTY, 1);
	imxdpuv1_write(imxdpu, b_off + IMXDPUV1_FRAMEGEN0_PACFG_OFFSET, reg);

	reg = IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMEGEN0_SACFG_SSTARTX, 1) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMEGEN0_SACFG_SSTARTY, 1);
	imxdpuv1_write(imxdpu, b_off + IMXDPUV1_FRAMEGEN0_SACFG_OFFSET, reg);

	if (IMXDPUV1_ENABLE == test_mode_enable) {
		reg = IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMEGEN0_FGINCTRL_FGDM,
			IMXDPUV1_FRAMEGEN0_FGINCTRL_FGDM__TEST);
	} else {
		reg = IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMEGEN0_FGINCTRL_FGDM,
			IMXDPUV1_FRAMEGEN0_FGINCTRL_FGDM__SEC) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMEGEN0_FGINCTRL_ENPRIMALPHA, 0) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMEGEN0_FGINCTRL_ENSECALPHA, 0);
	}
	imxdpuv1_write(imxdpu, b_off + IMXDPUV1_FRAMEGEN0_FGINCTRL_OFFSET, reg);

	reg = IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMEGEN0_FGINCTRLPANIC_FGDMPANIC,
		IMXDPUV1_FRAMEGEN0_FGINCTRLPANIC_FGDMPANIC__CONSTCOL) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMEGEN0_FGINCTRLPANIC_ENPRIMALPHAPANIC, 0) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMEGEN0_FGINCTRLPANIC_ENSECALPHAPANIC, 0);
	imxdpuv1_write(imxdpu, b_off +
		IMXDPUV1_FRAMEGEN0_FGINCTRLPANIC_OFFSET, reg);

	/* Set the constant color - ARGB 1-10-10-10 */
	reg = IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMEGEN0_FGCCR_CCRED, cc_red) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMEGEN0_FGCCR_CCBLUE, cc_blue) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMEGEN0_FGCCR_CCGREEN, cc_green) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMEGEN0_FGCCR_CCALPHA, cc_alpha);
	imxdpuv1_write(imxdpu, b_off + IMXDPUV1_FRAMEGEN0_FGCCR_OFFSET, reg);


	imxdpuv1_disp_setup_tcon_bypass_mode(imxdpuv1_id, disp, mode);

	/* save the mode */
	imxdpu->video_mode[disp] = *mode;

	/* imxdpuv1_disp_dump_mode(&imxdpu->video_mode[disp]); */

	return ret;
}

/*!
 * This function updates the frame generator status
 *
 * @param	imxdpuv1_id	id of the diplay unit
 * @param       disp		id of the diplay output pipe
 *
 * @return      This function returns 0 on success or negative error code on
 *      	fail.
 */
int imxdpuv1_disp_update_fgen_status(int8_t imxdpuv1_id, int8_t disp)
{
	int ret = 0;
	uint32_t b_off;     /* block offset for frame generator */
	uint32_t reg;
	uint32_t temp;
	struct imxdpuv1_soc *imxdpu;
	static uint32_t fcount[IMXDPUV1_NUM_DI_MAX] = { 0, 0 };

	IMXDPUV1_TRACE_IRQ("%s()\n", __func__);

	if (!((imxdpuv1_id >= 0) && (imxdpuv1_id < IMXDPUV1_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpuv1_array[imxdpuv1_id];

	if (disp == 0) {
		b_off = IMXDPUV1_FRAMEGEN0_LOCKUNLOCK;
	} else if (disp == 1) {
		b_off = IMXDPUV1_FRAMEGEN1_LOCKUNLOCK;
	} else {
		return -EINVAL;
	}

	/* todo:
	   add video mode sanity check here
	   check if LRSYNC is required
	 */

	reg = imxdpuv1_read_irq(imxdpu, b_off + IMXDPUV1_FRAMEGEN0_FGTIMESTAMP_OFFSET);
	IMXDPUV1_TRACE_IRQ("DISP %d: findex %d, lindex %d\n", disp,
		IMXDPUV1_GET_FIELD
		(IMXDPUV1_FRAMEGEN0_FGTIMESTAMP_FRAMEINDEX, reg),
		IMXDPUV1_GET_FIELD
		(IMXDPUV1_FRAMEGEN0_FGTIMESTAMP_LINEINDEX, reg));

	temp = IMXDPUV1_GET_FIELD(IMXDPUV1_FRAMEGEN0_FGTIMESTAMP_FRAMEINDEX, reg);
	if (temp != fcount[disp]) {
		fcount[disp] = temp;
		/* Just increment we assume this is called one per frame */
		imxdpu->fgen_stats[disp].frame_count++;
	}

	reg = imxdpuv1_read_irq(imxdpu, b_off + IMXDPUV1_FRAMEGEN0_FGCHSTAT_OFFSET);
	temp = IMXDPUV1_GET_FIELD(IMXDPUV1_FRAMEGEN0_FGCHSTAT_SECSYNCSTAT, reg);

	/* Sync status bits should be set */
	if ((temp != imxdpu->fgen_stats[disp].sec_sync_state) && (temp == 1)) {
		imxdpu->fgen_stats[disp].sec_sync_count++;
		IMXDPUV1_TRACE_IRQ("DISP %d: sec in sync\n", disp);
	}
	if ((temp != imxdpu->fgen_stats[disp].sec_sync_state) && (temp == 0)) {
		IMXDPUV1_TRACE_IRQ("DISP %d: sec out of sync\n", disp);
	}
	imxdpu->fgen_stats[disp].sec_sync_state = temp;
	temp = IMXDPUV1_GET_FIELD(IMXDPUV1_FRAMEGEN0_FGCHSTAT_PRIMSYNCSTAT, reg);

	/* Sync status bits should be set */
	if ((temp != imxdpu->fgen_stats[disp].prim_sync_state) &&
		(temp == 1)) {
		imxdpu->fgen_stats[disp].prim_sync_count++;
		IMXDPUV1_TRACE_IRQ("DISP %d: prim in sync\n", disp);
	}
	if ((temp != imxdpu->fgen_stats[disp].prim_sync_state) &&
		(temp == 0)) {
		IMXDPUV1_TRACE_IRQ("DISP %d: prim out of sync\n", disp);
	}
	imxdpu->fgen_stats[disp].prim_sync_state = temp;

	/* primary fifo bit should be clear if in use (panic stream) */
	if (IMXDPUV1_GET_FIELD(IMXDPUV1_FRAMEGEN0_FGCHSTAT_PFIFOEMPTY, reg)) {
		IMXDPUV1_TRACE_IRQ("DISP %d: primary fifo empty\n", disp);
		imxdpuv1_write_irq(imxdpu,
			b_off + IMXDPUV1_FRAMEGEN0_FGCHSTATCLR_OFFSET,
			IMXDPUV1_FRAMEGEN0_FGCHSTATCLR_CLRPRIMSTAT_MASK);
		imxdpu->fgen_stats[disp].prim_fifo_empty_count++;
	}
	/* secondary fifo and skew error bits should be clear
	   if in use (content stream) */
	if (IMXDPUV1_GET_FIELD(IMXDPUV1_FRAMEGEN0_FGCHSTAT_SFIFOEMPTY, reg) ||
		IMXDPUV1_GET_FIELD(IMXDPUV1_FRAMEGEN0_FGCHSTAT_SKEWRANGEERR, reg)) {
		if (IMXDPUV1_GET_FIELD(IMXDPUV1_FRAMEGEN0_FGCHSTAT_SFIFOEMPTY, reg)) {
			IMXDPUV1_TRACE_IRQ("DISP %d: secondary fifo empty\n",
				disp);
			imxdpu->fgen_stats[disp].sec_fifo_empty_count++;
		}
		if (IMXDPUV1_GET_FIELD
			(IMXDPUV1_FRAMEGEN0_FGCHSTAT_SKEWRANGEERR, reg)) {
			IMXDPUV1_TRACE_IRQ("DISP %d: secondary skew error\n",
				disp);
			imxdpu->fgen_stats[disp].skew_error_count++;
		}
		imxdpuv1_write_irq(imxdpu,
			b_off + IMXDPUV1_FRAMEGEN0_FGCHSTATCLR_OFFSET,
			IMXDPUV1_FRAMEGEN0_FGCHSTATCLR_CLRSECSTAT_MASK);
	}
	return ret;
}
/*!
 * This function sets up the frame capture
 *
 * @param	imxdpuv1_id	id of the diplay unit
 * @param       src_id		id of the capture source block
 * @param       dest_id		id of the capture dest block
 * @param       sync_count	number of valid required to aquire sync
 * @param	cap_mode	mode of the video input
 *
 * @return      This function returns 0 on success or negative error code on
 *      	fail.
 */
int imxdpuv1_cap_setup_frame(
	int8_t imxdpuv1_id,
	int8_t src_id,
	int8_t dest_id,
	int8_t sync_count,
	const struct imxdpuv1_videomode *cap_mode)
{
#ifndef IMXDPUV1_VERSION_0
	return -EINVAL;
#else
	int ret = 0;
	uint32_t b_off_frame;   /* block offset for capture source */
	uint32_t b_off_extsrc;  /* block offset for extsrc */

	int8_t cap_id;
	struct imxdpuv1_soc *imxdpu;

	IMXDPUV1_TRACE("%s()\n", __func__);

	if (!((imxdpuv1_id >= 0) && (imxdpuv1_id < IMXDPUV1_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpuv1_array[imxdpuv1_id];

	if (src_id == IMXDPUV1_ID_FRAMECAP4) {
		cap_id = 0;
		b_off_frame = IMXDPUV1_FRAMECAP4_LOCKUNLOCK;
		b_off_extsrc = IMXDPUV1_EXTSRC4_LOCKUNLOCK;
	} else if (src_id == IMXDPUV1_ID_FRAMECAP5) {
		cap_id = 1;
		b_off_frame = IMXDPUV1_FRAMECAP5_LOCKUNLOCK;
		b_off_extsrc = IMXDPUV1_EXTSRC5_LOCKUNLOCK;
	} else if (src_id == IMXDPUV1_ID_FRAMEDUMP0) {
		cap_id = 0;
		b_off_frame = IMXDPUV1_FRAMEDUMP0_CONTROL;
		b_off_extsrc = IMXDPUV1_EXTSRC0_LOCKUNLOCK;
	} else if (src_id == IMXDPUV1_ID_FRAMEDUMP1) {
		cap_id = 1;
		b_off_frame = IMXDPUV1_FRAMEDUMP1_CONTROL;
		b_off_extsrc = IMXDPUV1_EXTSRC4_LOCKUNLOCK;
	} else {
		return -EINVAL;
	}

	if (dest_id == IMXDPUV1_ID_STORE4) {
		imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_STORE4_DYNAMIC,
			IMXDPUV1_PIXENGCFG_STORE4_DYNAMIC_STORE4_SRC_SEL__EXTSRC4);
	} else if (dest_id == IMXDPUV1_ID_STORE5) {
		imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_STORE5_DYNAMIC,
			IMXDPUV1_PIXENGCFG_STORE5_DYNAMIC_STORE5_SRC_SEL__EXTSRC5);
	} else if (dest_id == IMXDPUV1_ID_EXTDST0) {
		imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_EXTDST0_DYNAMIC,
			IMXDPUV1_PIXENGCFG_EXTDST0_DYNAMIC_EXTDST0_SRC_SEL__EXTSRC4);
	} else if (dest_id == IMXDPUV1_ID_EXTDST1) {
		imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_EXTDST1_DYNAMIC,
			IMXDPUV1_PIXENGCFG_EXTDST1_DYNAMIC_EXTDST1_SRC_SEL__EXTSRC5);
	} else {
		return -EINVAL;
	}

	imxdpuv1_write(imxdpu,
		b_off_extsrc +  IMXDPUV1_EXTSRC4_STATICCONTROL_OFFSET,
		 IMXDPUV1_SET_FIELD(IMXDPUV1_EXTSRC4_STATICCONTROL_STARTSEL,
			 IMXDPUV1_EXTSRC4_STATICCONTROL_STARTSEL__LOCAL) |
		 IMXDPUV1_EXTSRC4_STATICCONTROL_SHDEN_MASK);
	imxdpuv1_write(imxdpu,
		b_off_extsrc + IMXDPUV1_EXTSRC4_CONSTANTCOLOR_OFFSET, 0);

	if (cap_mode->format == IMXDPUV1_PIX_FMT_BGR24) {
		 imxdpuv1_write(imxdpu,
			 b_off_extsrc + IMXDPUV1_EXTSRC4_COLORCOMPONENTBITS_OFFSET,
			 IMXDPUV1_SET_FIELD(IMXDPUV1_EXTSRC4_COLORCOMPONENTBITS_COMPONENTBITSRED, 0x8) |
			 IMXDPUV1_SET_FIELD(IMXDPUV1_EXTSRC4_COLORCOMPONENTBITS_COMPONENTBITSGREEN, 0x8) |
			 IMXDPUV1_SET_FIELD(IMXDPUV1_EXTSRC4_COLORCOMPONENTBITS_COMPONENTBITSBLUE, 0x8));
		 imxdpuv1_write(imxdpu,
			  b_off_extsrc + IMXDPUV1_EXTSRC4_COLORCOMPONENTSHIFT_OFFSET,
			 IMXDPUV1_SET_FIELD(IMXDPUV1_EXTSRC4_COLORCOMPONENTSHIFT_COMPONENTSHIFTRED, 0x10) |
			 IMXDPUV1_SET_FIELD(IMXDPUV1_EXTSRC4_COLORCOMPONENTSHIFT_COMPONENTSHIFTGREEN, 0x08) |
			 IMXDPUV1_SET_FIELD(IMXDPUV1_EXTSRC4_COLORCOMPONENTSHIFT_COMPONENTSHIFTBLUE, 0x00));

		 /* fixme: handle all cases for control */
		 imxdpuv1_write(imxdpu,
			b_off_extsrc + IMXDPUV1_EXTSRC4_CONTROL_OFFSET,
			IMXDPUV1_SET_FIELD(IMXDPUV1_EXTSRC4_CONTROL_YUVCONVERSIONMODE,
			IMXDPUV1_EXTSRC4_CONTROL_YUVCONVERSIONMODE__ITU601) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_EXTSRC4_CONTROL_RASTERMODE,
			IMXDPUV1_EXTSRC4_CONTROL_RASTERMODE__YUV422) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_EXTSRC4_CONTROL_YUV422UPSAMPLINGMODE,
			IMXDPUV1_EXTSRC4_CONTROL_YUV422UPSAMPLINGMODE__REPLICATE) |
			IMXDPUV1_EXTSRC4_CONTROL_CLIPWINDOWENABLE_MASK);

	} else if (cap_mode->format == IMXDPUV1_PIX_FMT_YUYV) {

		 imxdpuv1_write(imxdpu,
			b_off_extsrc + IMXDPUV1_EXTSRC4_COLORCOMPONENTBITS_OFFSET,

			IMXDPUV1_SET_FIELD(IMXDPUV1_EXTSRC4_COLORCOMPONENTBITS_COMPONENTBITSRED, 0x8) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_EXTSRC4_COLORCOMPONENTBITS_COMPONENTBITSGREEN, 0x8) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_EXTSRC4_COLORCOMPONENTBITS_COMPONENTBITSBLUE, 0x8));
		 imxdpuv1_write(imxdpu,
			b_off_extsrc + IMXDPUV1_EXTSRC4_COLORCOMPONENTSHIFT_OFFSET,
			IMXDPUV1_SET_FIELD(IMXDPUV1_EXTSRC4_COLORCOMPONENTSHIFT_COMPONENTSHIFTRED, 0x8) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_EXTSRC4_COLORCOMPONENTSHIFT_COMPONENTSHIFTGREEN, 0x0) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_EXTSRC4_COLORCOMPONENTSHIFT_COMPONENTSHIFTBLUE, 0x0));

		 /* fixme: handle all cases for control */
		 imxdpuv1_write(imxdpu,
			b_off_extsrc + IMXDPUV1_EXTSRC4_CONTROL_OFFSET,
			IMXDPUV1_SET_FIELD(IMXDPUV1_EXTSRC4_CONTROL_RASTERMODE,
			IMXDPUV1_EXTSRC4_CONTROL_RASTERMODE__YUV422) |
			IMXDPUV1_EXTSRC4_CONTROL_CLIPWINDOWENABLE_MASK);

	} else {
		IMXDPUV1_PRINT("%s(): invalid capture interface format\n", __func__);
		return -EINVAL;
	}


	if ((src_id == IMXDPUV1_ID_FRAMECAP4) || (src_id == IMXDPUV1_ID_FRAMECAP5)) {
		/* setup cature */
		uint8_t capture_interface_mode;
		/* Fixme:  change these mode bits to an enumeration */
		if ((cap_mode->flags & IMXDPUV1_MODE_FLAGS_32BIT) != 0) {
			capture_interface_mode = IMXDPUV1_CAPENGCFG_CAPTUREINPUT1_CAPTUREMODE1__ENHSVS_32BIT;
		} else if ((cap_mode->flags & IMXDPUV1_MODE_FLAGS_BT656_10BIT) != 0) {
			capture_interface_mode = IMXDPUV1_CAPENGCFG_CAPTUREINPUT1_CAPTUREMODE1__ITU656_10BIT;
		} else if ((cap_mode->flags & IMXDPUV1_MODE_FLAGS_BT656_8BIT) != 0) {
			capture_interface_mode = IMXDPUV1_CAPENGCFG_CAPTUREINPUT1_CAPTUREMODE1__ITU656_8BIT;
		} else {
			return -EINVAL;
		}

		if (cap_id == 0) {
			imxdpuv1_write(imxdpu, IMXDPUV1_CAPENGCFG_CAPTUREINPUT0,
				 IMXDPUV1_SET_FIELD(IMXDPUV1_CAPENGCFG_CAPTUREINPUT0_CAPTUREMODE0,
					 capture_interface_mode));
		} else {
			imxdpuv1_write(imxdpu, IMXDPUV1_CAPENGCFG_CAPTUREINPUT1,
				 IMXDPUV1_SET_FIELD(IMXDPUV1_CAPENGCFG_CAPTUREINPUT1_CAPTUREMODE1,
					 capture_interface_mode));
		}

		imxdpuv1_write(imxdpu, b_off_frame + IMXDPUV1_FRAMECAP4_FDR_OFFSET,
		       IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMECAP4_FDR_HEIGHT, cap_mode->vlen - 1) |
		       IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMECAP4_FDR_WIDTH, cap_mode->hlen - 1));

		imxdpuv1_write(imxdpu,
			b_off_frame + IMXDPUV1_FRAMECAP4_FDR1_OFFSET,
			IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMECAP4_FDR_HEIGHT, cap_mode->vlen1 - 1));

		imxdpuv1_write(imxdpu,
			b_off_frame + IMXDPUV1_FRAMECAP4_SCR_OFFSET, sync_count);


		imxdpuv1_write(imxdpu,
			b_off_frame + IMXDPUV1_FRAMECAP4_KCR_OFFSET, 0);
		if ((cap_mode->clip_height != 0) && (cap_mode->clip_width != 0)) {
			imxdpuv1_write(imxdpu, b_off_extsrc  + IMXDPUV1_EXTSRC4_CLIPWINDOWDIMENSION_OFFSET,
				IMXDPUV1_SET_FIELD(IMXDPUV1_EXTSRC4_CLIPWINDOWDIMENSION_CLIPWINDOWHEIGHT, cap_mode->clip_height - 1) |
				IMXDPUV1_SET_FIELD(IMXDPUV1_EXTSRC4_CLIPWINDOWDIMENSION_CLIPWINDOWWIDTH, cap_mode->clip_width - 1));

			imxdpuv1_write(imxdpu, b_off_extsrc  + IMXDPUV1_EXTSRC4_CLIPWINDOWOFFSET_OFFSET,
				IMXDPUV1_SET_FIELD(IMXDPUV1_EXTSRC4_CLIPWINDOWOFFSET_CLIPWINDOWXOFFSET, cap_mode->clip_left) |
				IMXDPUV1_SET_FIELD(IMXDPUV1_EXTSRC4_CLIPWINDOWOFFSET_CLIPWINDOWYOFFSET, cap_mode->clip_top));
		}

		imxdpuv1_write(imxdpu,
			b_off_frame +  IMXDPUV1_FRAMECAP4_SPR_OFFSET,

			/* low is active low, high is active high */
			IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMECAP4_SPR_POLHS,
				((cap_mode->flags & IMXDPUV1_MODE_FLAGS_HSYNC_POL) != 0)) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMECAP4_SPR_POLVS,
				((cap_mode->flags & IMXDPUV1_MODE_FLAGS_VSYNC_POL) != 0)) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMECAP4_SPR_POLEN,
				((cap_mode->flags & IMXDPUV1_MODE_FLAGS_DE_POL) == 0))
		);


		/* fixme: may need to move this mapping */
		if (src_id == IMXDPUV1_ID_FRAMECAP4) {
			imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_STORE4_DYNAMIC,
				IMXDPUV1_PIXENGCFG_STORE4_DYNAMIC_STORE4_SRC_SEL__EXTSRC4);
		} else if (src_id == IMXDPUV1_ID_FRAMECAP5) {
			imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_STORE5_DYNAMIC,
				IMXDPUV1_PIXENGCFG_STORE5_DYNAMIC_STORE5_SRC_SEL__EXTSRC5);
		}
	}

	if ((src_id == IMXDPUV1_ID_FRAMEDUMP0) || (src_id == IMXDPUV1_ID_FRAMEDUMP1)) {
		/* todo */
	}

	/* save the mode */
	imxdpu->capture_mode[cap_id] = *cap_mode;
	/* imxdpuv1_disp_dump_mode(cap_mode); */
	return ret;
#endif
}

/*!
 * This function sets up the frame capture
 *
 * @param	imxdpuv1_id	id of the diplay unit
 * @param       cap		id of the capture inpute
 * @param       sync_count	number of valid required to aquire sync
 * @param	cap_mode	mode of the video input
 *
 * @return      This function returns 0 on success or negative error code on
 *      	fail.
 */
int imxdpuv1_cap_setup_crop(
	int8_t imxdpuv1_id,
	int8_t src_id,
	int16_t  clip_top,
	int16_t  clip_left,
	uint16_t clip_width,
	uint16_t clip_height)
{
#ifndef IMXDPUV1_VERSION_0
	return -EINVAL;
#else
	int ret = 0;
	uint32_t b_off_extsrc;  /* block offset for extsrc */
#if 0
	uint32_t b_off_dest;    /* block offset for destination */
#endif
	int8_t cap_id;
	struct imxdpuv1_soc *imxdpu;

	IMXDPUV1_TRACE("%s()\n", __func__);

	if (!((imxdpuv1_id >= 0) && (imxdpuv1_id < IMXDPUV1_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpuv1_array[imxdpuv1_id];

	if (src_id == IMXDPUV1_ID_FRAMECAP4) {
		cap_id = 0;
		b_off_extsrc = IMXDPUV1_EXTSRC4_LOCKUNLOCK;
	} else if (src_id == IMXDPUV1_ID_FRAMECAP5) {
		cap_id = 1;
		b_off_extsrc = IMXDPUV1_EXTSRC5_LOCKUNLOCK;
	} else if (src_id == IMXDPUV1_ID_FRAMEDUMP0) {
		cap_id = 0;
		b_off_extsrc = IMXDPUV1_EXTSRC0_LOCKUNLOCK;
	} else if (src_id == IMXDPUV1_ID_FRAMEDUMP1) {
		cap_id = 1;
		b_off_extsrc = IMXDPUV1_EXTSRC4_LOCKUNLOCK;
	} else {
		return -EINVAL;
	}

	if ((src_id == IMXDPUV1_ID_FRAMECAP4) || (src_id == IMXDPUV1_ID_FRAMECAP5)) {
		if ((clip_height != 0) && (clip_width != 0)) {
			imxdpuv1_write(imxdpu, b_off_extsrc  + IMXDPUV1_EXTSRC4_CLIPWINDOWDIMENSION_OFFSET,
				IMXDPUV1_SET_FIELD(IMXDPUV1_EXTSRC4_CLIPWINDOWDIMENSION_CLIPWINDOWHEIGHT, clip_height - 1) |
				IMXDPUV1_SET_FIELD(IMXDPUV1_EXTSRC4_CLIPWINDOWDIMENSION_CLIPWINDOWWIDTH, clip_width - 1));

			imxdpuv1_write(imxdpu, b_off_extsrc  + IMXDPUV1_EXTSRC4_CLIPWINDOWOFFSET_OFFSET,
				IMXDPUV1_SET_FIELD(IMXDPUV1_EXTSRC4_CLIPWINDOWOFFSET_CLIPWINDOWXOFFSET, clip_left) |
				IMXDPUV1_SET_FIELD(IMXDPUV1_EXTSRC4_CLIPWINDOWOFFSET_CLIPWINDOWYOFFSET, clip_top));
			/* save the clip data */
			imxdpu->capture_mode[cap_id].clip_height = clip_height;
			imxdpu->capture_mode[cap_id].clip_width  = clip_width;
			imxdpu->capture_mode[cap_id].clip_top    = clip_top;
			imxdpu->capture_mode[cap_id].clip_left   = clip_left;
		}
	}

	if ((src_id == IMXDPUV1_ID_FRAMEDUMP0) || (src_id == IMXDPUV1_ID_FRAMEDUMP1)) {
		/* todo */
	}
	/* imxdpuv1_disp_dump_mode(&imxdpu->video_mode[cap_id]); */
	return ret;
#endif
}
/*!
 * This function enables the frame capture
 *
 * @param	imxdpuv1_id	id of the display unit
 * @param       cap		id of the capture output pipe
 * @param 	enable 		state to set frame generator to
 *
 * @return 	This function returns 0 on success or negative error code on
 *              fail.
 */
int imxdpuv1_cap_enable(int8_t imxdpuv1_id, int8_t cap, bool enable)
{
#ifndef IMXDPUV1_VERSION_0
	return -EINVAL;
#else
	int ret = 0;
	uint32_t b_off;
	uint32_t reg;
	struct imxdpuv1_soc *imxdpu;

	IMXDPUV1_TRACE("%s()\n", __func__);

	if (!((imxdpuv1_id >= 0) && (imxdpuv1_id < IMXDPUV1_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpuv1_array[imxdpuv1_id];

	if (cap == 0) {
		b_off = IMXDPUV1_FRAMECAP4_LOCKUNLOCK;
	} else {
		return -EINVAL;
	}

	if (enable) {
		/* imxdpuv1_dump_pixencfg_status(imxdpuv1_id); */
		printf("%s(): %s:%d stubbed feature\n", __func__, __FILE__, __LINE__);
		/* imxdpuv1_dump_pixencfg_status(imxdpuv1_id); */
	}
	reg = enable ? IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMECAP4_CTR_CEN, 1) :
		       IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMECAP4_CTR_CEN, 0);


	imxdpuv1_write(imxdpu, b_off + IMXDPUV1_FRAMECAP4_CTR_OFFSET, reg);

	return ret;
#endif
}

/*!
 * This function triggers a shadow load
 *
 * @param	imxdpuv1_id	id of the diplay unit
 * @param       dest_id		id of the capture dest block
 *
 * @return      This function returns 0 on success or negative error code on
 *      	fail.
 */
int imxdpuv1_cap_request_shadow_load(int8_t imxdpuv1_id, int8_t dest_id, uint32_t mask)
{
#ifndef IMXDPUV1_VERSION_0
	return -EINVAL;
#else
	int ret = 0;
	struct imxdpuv1_soc *imxdpu;

	IMXDPUV1_TRACE("%s()\n", __func__);

	if (!((imxdpuv1_id >= 0) && (imxdpuv1_id < IMXDPUV1_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpuv1_array[imxdpuv1_id];

	switch (dest_id) {
	case IMXDPUV1_ID_STORE4:
		imxdpuv1_write(imxdpu,
			IMXDPUV1_PIXENGCFG_STORE4_REQUEST,
			mask);
		imxdpuv1_write(imxdpu,
			IMXDPUV1_PIXENGCFG_STORE4_TRIGGER,
			IMXDPUV1_PIXENGCFG_STORE4_TRIGGER_STORE4_SYNC_TRIGGER_MASK);
		break;
	case IMXDPUV1_ID_STORE5:
		imxdpuv1_write(imxdpu,
			IMXDPUV1_PIXENGCFG_STORE5_REQUEST,
			mask);
		imxdpuv1_write(imxdpu,
			IMXDPUV1_PIXENGCFG_STORE5_TRIGGER,
			IMXDPUV1_PIXENGCFG_STORE5_TRIGGER_STORE5_SYNC_TRIGGER_MASK);
		break;

	default:
		return -EINVAL;

	}
	return ret;
#endif
}

/*!
 * This function requests a shadow loads
 *
 * @param	imxdpuv1_id	id of the diplay unit
 * @param       disp		id of the diplay output pipe
 * @param       shadow_load_idx  index of the shadow load requested
 * @return      This function returns 0 on success or negative error code on
 *      	fail.
 */
int imxdpuv1_disp_request_shadow_load(int8_t imxdpuv1_id,
	int8_t disp,
	imxdpuv1_shadow_load_index_t shadow_load_idx)
{
	int ret = 0;
	struct imxdpuv1_soc *imxdpu;

	IMXDPUV1_TRACE("%s(): imxdpuv1_id %d, disp %d, shadow_load_idx %d\n",
		__func__, imxdpuv1_id, disp, shadow_load_idx);

	if (!((imxdpuv1_id >= 0) && (imxdpuv1_id < IMXDPUV1_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpuv1_array[imxdpuv1_id];
	/* trigger configuration of the pipeline */

	if ((disp == 0) || (disp == 1)) {
		/* last request was complete or no request in progress,
		   then start a new request */
		if (imxdpu->shadow_load_state[disp][shadow_load_idx].word == 0) {
			imxdpu->shadow_load_state[disp][shadow_load_idx].state.
				request = IMXDPUV1_TRUE;
		} else {    /* check ifg the request is busy */
			IMXDPUV1_TRACE("%s(): shadow load not complete.", __func__);
			return -EBUSY;
		}
	} else {
		return -EINVAL;
	}

	return ret;
}

/*!
 * This function force a shadow loads
 *
 * @param	imxdpuv1_id	id of the diplay unit
 * @param       disp		id of the diplay output pipe
 * @param       shadow_load_idx  index of the shadow load requested
 * @return      This function returns 0 on success or negative error code on
 *      	fail.
 */
int imxdpuv1_disp_force_shadow_load(int8_t imxdpuv1_id,
	int8_t disp,
	uint64_t mask)
{
	int ret = 0;
	uint32_t addr_extdst;   /* address for extdst */
	uint32_t addr_fgen; /* address for frame generator */
	uint32_t extdst = 0;
	uint32_t fgen = 0;
	uint32_t sub = 0;
	struct imxdpuv1_soc *imxdpu;
	int i;
	uint64_t temp_mask;

	IMXDPUV1_TRACE_IRQ("%s()\n", __func__);

	if (!((imxdpuv1_id >= 0) && (imxdpuv1_id < IMXDPUV1_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpuv1_array[imxdpuv1_id];

	if (!((imxdpuv1_id >= 0) && (imxdpuv1_id < IMXDPUV1_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpuv1_array[imxdpuv1_id];

	if (!((disp == 0) || (disp == 1))) {
		return -EINVAL;
	}

	if (mask == 0) {
		return -EINVAL;
	}

	if (disp == 0) {
		addr_fgen = IMXDPUV1_FRAMEGEN0_FGSLR;
		addr_extdst = IMXDPUV1_PIXENGCFG_EXTDST0_REQUEST;
	} else if (disp == 1) {
		addr_fgen = IMXDPUV1_FRAMEGEN1_FGSLR;
		addr_extdst = IMXDPUV1_PIXENGCFG_EXTDST1_REQUEST;
	} else {
		return -EINVAL;
	}

	for (i = 0; i <  IMXDPUV1_SHDLD_IDX_MAX; i++) {
		temp_mask = 1ULL << i;
		if ((mask & temp_mask) == 0)
			continue;

		extdst |= trigger_list[i].extdst;
		sub |= trigger_list[i].sub;

		if ((i == IMXDPUV1_SHDLD_IDX_CONST0) ||
			(i == IMXDPUV1_SHDLD_IDX_CONST1)) {
			fgen |= 1;
		}
		mask &= ~temp_mask;
	}

	if (sub) {
		IMXDPUV1_TRACE_IRQ("Fetch layer shadow request 0x%08x\n", sub);
		if (sub & 0xff) {   /* FETCHLAYER0 */
			imxdpuv1_write(imxdpu, IMXDPUV1_FETCHLAYER0_TRIGGERENABLE,
				sub & 0xff);
		}
#ifdef IMXDPUV1_VERSION_0
		if (sub & 0xff00) { /* FETCHLAYER1 */
			imxdpuv1_write(imxdpu, IMXDPUV1_FETCHLAYER1_TRIGGERENABLE,
				(sub >> 8) & 0xff);
		}
#endif
		if (sub & 0xff0000) {   /* FETCHWARP2 */
			imxdpuv1_write(imxdpu, IMXDPUV1_FETCHWARP2_TRIGGERENABLE,
				(sub >> 16) & 0xff);
		}
	}

	if (extdst) {
		IMXDPUV1_TRACE_IRQ("Extdst shadow request  0x%08x\n", extdst);
		imxdpuv1_write(imxdpu, addr_extdst, extdst);
	}

	if (fgen) {
		IMXDPUV1_TRACE_IRQ("Fgen shadow request  0x%08x\n", fgen);
		imxdpuv1_write(imxdpu, addr_fgen, fgen);
	}

	return ret;
}

/*!
 * This function shows the frame generators status
 *
 * @param	imxdpuv1_id	id of the diplay unit
 *
 * @return      This function returns 0 on success or negative error code on
 *      	fail.
 */
int imxdpuv1_disp_show_fgen_status(int8_t imxdpuv1_id)
{
#ifndef ENABLE_IMXDPUV1_TRACE
	return 0;
#else
	int ret = 0;
	struct imxdpuv1_soc *imxdpu;

	IMXDPUV1_TRACE_IRQ("%s()\n", __func__);

	if (!((imxdpuv1_id >= 0) && (imxdpuv1_id < IMXDPUV1_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpuv1_array[imxdpuv1_id];

	IMXDPUV1_PRINT("IMXDPU %d stat    			   fg0  	      fg1\n"
		"prim_sync_state:            %10d         %10d\n"
		"sec_sync_state:	     %10d         %10d\n"
		"prim_sync_count:            %10d         %10d\n"
		"sec_sync_count:	     %10d         %10d\n"
		"skew_error_count:           %10d         %10d\n"
		"prim_fifo_empty_count:      %10d         %10d\n"
		"sec_fifo_empty_count:       %10d         %10d\n"
		"frame_count:   	     %10d         %10d\n"
		"irq_count:     	     %10u\n\n",
		imxdpuv1_id,
		imxdpu->fgen_stats[0].prim_sync_state,
		imxdpu->fgen_stats[1].prim_sync_state,
		imxdpu->fgen_stats[0].sec_sync_state,
		imxdpu->fgen_stats[1].sec_sync_state,
		imxdpu->fgen_stats[0].prim_sync_count,
		imxdpu->fgen_stats[1].prim_sync_count,
		imxdpu->fgen_stats[0].sec_sync_count,
		imxdpu->fgen_stats[1].sec_sync_count,
		imxdpu->fgen_stats[0].skew_error_count,
		imxdpu->fgen_stats[1].skew_error_count,
		imxdpu->fgen_stats[0].prim_fifo_empty_count,
		imxdpu->fgen_stats[1].prim_fifo_empty_count,
		imxdpu->fgen_stats[0].sec_fifo_empty_count,
		imxdpu->fgen_stats[1].sec_fifo_empty_count,
		imxdpu->fgen_stats[0].frame_count,
		imxdpu->fgen_stats[1].frame_count,
		imxdpu->irq_count);

	return ret;
#endif
}

/*!
 * This function enables the frame generator
 *
 * @param	imxdpuv1_id	id of the diplay unit
 * @param       disp		id of the diplay output pipe
 * @param       enable  	state to set frame generator to
 *
 * @return      This function returns 0 on success or negative error code on
 *      	fail.
 */
int imxdpuv1_disp_enable_frame_gen(int8_t imxdpuv1_id, int8_t disp, bool enable)
{
	int ret = 0;
	uint32_t b_off;
	uint32_t reg;
	struct imxdpuv1_soc *imxdpu;

	IMXDPUV1_TRACE("%s()\n", __func__);

	if (!((imxdpuv1_id >= 0) && (imxdpuv1_id < IMXDPUV1_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpuv1_array[imxdpuv1_id];

	if (disp == 0) {
		b_off = IMXDPUV1_FRAMEGEN0_LOCKUNLOCK;
	} else if (disp == 1) {
		b_off = IMXDPUV1_FRAMEGEN1_LOCKUNLOCK;
	} else {
		return -EINVAL;
	}

	imxdpuv1_disp_start_shadow_loads(imxdpuv1_id, disp);

	reg = enable ? IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMEGEN0_FGENABLE_FGEN, 1) :
		       IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMEGEN0_FGENABLE_FGEN, 0);
	imxdpuv1_write(imxdpu, b_off + IMXDPUV1_FRAMEGEN0_FGENABLE_OFFSET, reg);

	return ret;
}

/*!
 * This function sets up the constframe generator
 *
 * @param	imxdpuv1_id	id of the diplay unit
 * @param       disp		id of the diplay output pipe
 * @param       bg_red		background red
 * @param       bg_green	background green
 * @param       bg_blue		background blue
 * @param       bg_alpha	background alpha
 *
 * @return      This function returns 0 on success or negative error code on
 *      	fail.
 */
int imxdpuv1_disp_setup_constframe(
	int8_t imxdpuv1_id,
	int8_t disp,
	uint8_t bg_red,
	uint8_t bg_green,
	uint8_t bg_blue,
	uint8_t bg_alpha)
{
	int ret = 0;
	uint32_t b_off;
	uint32_t reg;
	struct imxdpuv1_soc *imxdpu;
	imxdpuv1_shadow_load_index_t shadow_idx;
	IMXDPUV1_TRACE("%s()\n", __func__);

	if (!((imxdpuv1_id >= 0) && (imxdpuv1_id < IMXDPUV1_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpuv1_array[imxdpuv1_id];

	/* todo: add constfram4 and constframe5 */
	if (disp == 0) {
		b_off = IMXDPUV1_CONSTFRAME0_LOCKUNLOCK;
		shadow_idx = IMXDPUV1_SHDLD_IDX_CONST0;
	} else if (disp == 1) {
		b_off = IMXDPUV1_CONSTFRAME1_LOCKUNLOCK;
		shadow_idx = IMXDPUV1_SHDLD_IDX_CONST1;
	} else {
		return -EINVAL;
	}

	if (imxdpu->video_mode[disp].flags & IMXDPUV1_MODE_FLAGS_LRSYNC) {
		/* todo: need to handle sync display case */
	}

	reg = IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMEHEIGHT,
		imxdpu->video_mode[disp].vlen - 1) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMEWIDTH,
		imxdpu->video_mode[disp].hlen - 1);
	imxdpuv1_write(imxdpu,
		b_off + IMXDPUV1_CONSTFRAME0_FRAMEDIMENSIONS_OFFSET, reg);

	/* todo: add linear light correction if needed */
	imxdpuv1_write(imxdpu, b_off + IMXDPUV1_CONSTFRAME0_CONSTANTCOLOR_OFFSET,
		IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_CONSTRED, bg_red) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_CONSTGREEN, bg_green) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_CONSTBLUE, bg_blue) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_CONSTALPHA, bg_alpha));

	imxdpuv1_disp_request_shadow_load(imxdpuv1_id, disp, shadow_idx);

	/* todo: add linear light correction if needed */
	return ret;
}

/*!
 * This function sets up a layer
 *
 * @param	imxdpuv1_id	id of the diplay unit
 * @param       layer   	layer data to use
 * @param	layer_idx       layer index  to use
 *
 * @return      This function returns 0 on success or negative error code on
 *      	fail.
 */
int imxdpuv1_disp_setup_layer(int8_t imxdpuv1_id,
	const imxdpuv1_layer_t *layer,
			    imxdpuv1_layer_idx_t layer_idx,
	bool is_top_layer)
{
	int ret = 0;
	uint32_t dynamic_offset;
	uint32_t static_offset;
	uint32_t reg;
	struct imxdpuv1_soc *imxdpu;

	IMXDPUV1_TRACE("%s()\n", __func__);

	if (!((imxdpuv1_id >= 0) && (imxdpuv1_id < IMXDPUV1_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpuv1_array[imxdpuv1_id];

	IMXDPUV1_TRACE("%s():  enable %d, primary %d, secondary %d, stream 0x%08x\n", __func__,
		layer->enable,
		layer->primary,
		layer->secondary,
		layer->stream);
	imxdpu->blend_layer[layer_idx] = *layer;

	dynamic_offset = id2dynamicoffset(layer_idx + IMXDPUV1_ID_LAYERBLEND0);
	if (dynamic_offset == IMXDPUV1_OFFSET_INVALID) {
		return -EINVAL;
	}

	static_offset = id2blockoffset(layer_idx + IMXDPUV1_ID_LAYERBLEND0);
	if (static_offset == IMXDPUV1_OFFSET_INVALID) {
		return -EINVAL;
	}

	reg =
		IMXDPUV1_SET_FIELD(
		IMXDPUV1_PIXENGCFG_LAYERBLEND0_DYNAMIC_LAYERBLEND0_PRIM_SEL,
		imxdpu->blend_layer[layer_idx].primary) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_LAYERBLEND0_DYNAMIC_LAYERBLEND0_SEC_SEL,
		imxdpu->blend_layer[layer_idx].secondary) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_LAYERBLEND0_DYNAMIC_LAYERBLEND0_CLKEN,
		IMXDPUV1_PIXENGCFG_LAYERBLEND0_DYNAMIC_LAYERBLEND0_CLKEN__AUTOMATIC);
	imxdpuv1_write(imxdpu, dynamic_offset, reg);

	if (imxdpu->blend_layer[layer_idx].stream & IMXDPUV1_DISPLAY_STREAM_0) {

		IMXDPUV1_TRACE("%s():  IMXDPUV1_DISPLAY_STREAM_0\n", __func__);
		if (is_top_layer) {
		reg = IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_EXTDST0_DYNAMIC_EXTDST0_SRC_SEL,
			layer_idx + IMXDPUV1_ID_LAYERBLEND0);
		imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_EXTDST0_DYNAMIC, reg);
		}

		/* trigger configuration of the pipeline */
		imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_EXTDST0_TRIGGER,
			IMXDPUV1_PIXENGCFG_EXTDST0_TRIGGER_EXTDST0_SYNC_TRIGGER_MASK);
		imxdpuv1_disp_request_shadow_load(imxdpuv1_id, 0,
			IMXDPUV1_SHDLD_IDX_DISP0);
	}
	if (imxdpu->blend_layer[layer_idx].stream & IMXDPUV1_DISPLAY_STREAM_1) {
		IMXDPUV1_TRACE_IRQ("%s():  IMXDPUV1_DISPLAY_STREAM_1\n", __func__);
		if (is_top_layer) {
		reg =
			IMXDPUV1_SET_FIELD(IMXDPUV1_PIXENGCFG_EXTDST0_DYNAMIC_EXTDST0_SRC_SEL,
			layer_idx + IMXDPUV1_ID_LAYERBLEND0);
		imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_EXTDST1_DYNAMIC, reg);

		}
		/* trigger configuration of the pipeline */
		imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_EXTDST1_TRIGGER,
			IMXDPUV1_PIXENGCFG_EXTDST1_TRIGGER_EXTDST1_SYNC_TRIGGER_MASK);
		imxdpuv1_disp_request_shadow_load(imxdpuv1_id, 1,
			IMXDPUV1_SHDLD_IDX_DISP1);
	}

	/* todo: add code to disable a layer */
	return ret;
}

/*!
 * This function sets global alpha for a blend layer
 *
 * @param	imxdpuv1_id	id of the diplay unit
 * @param	layer_idx       layer index  to use
 * @param       alpha   	global alpha
 *
 * @return      This function returns 0 on success or negative error code on
 *      	fail.
 */
int imxdpuv1_disp_set_layer_global_alpha(int8_t imxdpuv1_id,
	imxdpuv1_layer_idx_t layer_idx,
	uint8_t alpha)
{
	int ret = 0;
	uint32_t offset;
	uint32_t reg;
	struct imxdpuv1_soc *imxdpu;

	IMXDPUV1_TRACE("%s()\n", __func__);

	if (!((imxdpuv1_id >= 0) && (imxdpuv1_id < IMXDPUV1_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpuv1_array[imxdpuv1_id];

	/* update imxdpu */

	offset = id2blockoffset(layer_idx + IMXDPUV1_ID_LAYERBLEND0);
	if (offset == IMXDPUV1_OFFSET_INVALID) {
		return -EINVAL;
	}

	reg = IMXDPUV1_SET_FIELD(IMXDPUV1_LAYERBLEND0_BLENDCONTROL_BLENDALPHA,
		alpha)
		| IMXDPUV1_SET_FIELD(IMXDPUV1_LAYERBLEND0_BLENDCONTROL_PRIM_C_BLD_FUNC,
		IMXDPUV1_LAYERBLEND0_BLENDCONTROL_PRIM_C_BLD_FUNC__ONE_MINUS_SEC_ALPHA)
		| IMXDPUV1_SET_FIELD(IMXDPUV1_LAYERBLEND0_BLENDCONTROL_SEC_C_BLD_FUNC,
		IMXDPUV1_LAYERBLEND0_BLENDCONTROL_SEC_C_BLD_FUNC__CONST_ALPHA)
		| IMXDPUV1_SET_FIELD(IMXDPUV1_LAYERBLEND0_BLENDCONTROL_PRIM_A_BLD_FUNC,
		IMXDPUV1_LAYERBLEND0_BLENDCONTROL_PRIM_A_BLD_FUNC__ONE_MINUS_SEC_ALPHA)
		| IMXDPUV1_SET_FIELD(IMXDPUV1_LAYERBLEND0_BLENDCONTROL_SEC_A_BLD_FUNC,
		IMXDPUV1_LAYERBLEND0_BLENDCONTROL_SEC_A_BLD_FUNC__ONE);
	imxdpuv1_write(imxdpu, offset + IMXDPUV1_LAYERBLEND0_BLENDCONTROL_OFFSET,
		reg);

	reg =
		IMXDPUV1_SET_FIELD(IMXDPUV1_LAYERBLEND0_CONTROL_MODE,
		IMXDPUV1_LAYERBLEND0_CONTROL_MODE__BLEND) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_LAYERBLEND0_CONTROL_ALPHAMASKENABLE,
		IMXDPUV1_DISABLE);

	imxdpuv1_write(imxdpu, offset + IMXDPUV1_LAYERBLEND0_CONTROL_OFFSET, reg);

	return ret;
}

/*!
 * This function sets the position of the a blend layer secondary input
 *
 * @param	imxdpuv1_id	id of the diplay unit
 * @param	layer_idx       layer index  to use
 * @param       x       	x position
 * @param       y       	y position
 *
 * @return      This function returns 0 on success or negative error code on
 *      	fail.
 */
int imxdpuv1_disp_set_layer_position(int8_t imxdpuv1_id,
	imxdpuv1_layer_idx_t layer_idx,
	int16_t x, int16_t y)
{
	int ret = 0;
	uint32_t offset;
	uint32_t reg;
	struct imxdpuv1_soc *imxdpu;

	IMXDPUV1_TRACE("%s()\n", __func__);

	if (!((imxdpuv1_id >= 0) && (imxdpuv1_id < IMXDPUV1_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpuv1_array[imxdpuv1_id];

	/* update imxdpu */

	offset = id2blockoffset(layer_idx + IMXDPUV1_ID_LAYERBLEND0);
	if (offset == IMXDPUV1_OFFSET_INVALID) {
		return -EINVAL;
	}

	reg = IMXDPUV1_SET_FIELD(IMXDPUV1_LAYERBLEND0_POSITION_XPOS, x) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_LAYERBLEND0_POSITION_YPOS, y);
	imxdpuv1_write(imxdpu, offset + IMXDPUV1_LAYERBLEND0_POSITION_OFFSET, reg);

	return ret;
}

/*!
 * This function sets the position of the a channel (window) layer
 *
 * @param	imxdpuv1_id	id of the diplay unit
 * @param	layer_idx       layer index  to use
 * @param       x       	x position
 * @param       y       	y position
 *
 * @return      This function returns 0 on success or negative error code on
 *      	fail.
 */
int imxdpuv1_disp_set_chan_position(int8_t imxdpuv1_id,
	imxdpuv1_chan_t chan, int16_t x, int16_t y)
{
	int ret = 0;
	uint32_t offset;
	int idx;
	int sub_idx;
	struct imxdpuv1_soc *imxdpu;

	IMXDPUV1_TRACE("%s()\n", __func__);

	if (!((imxdpuv1_id >= 0) && (imxdpuv1_id < IMXDPUV1_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpuv1_array[imxdpuv1_id];

	/* update imxdpu */

	offset = id2blockoffset(get_channel_blk(chan));
	if (offset == IMXDPUV1_OFFSET_INVALID) {
		return -EINVAL;
	}

	idx = get_channel_idx(chan);
	if ((idx >= IMXDPUV1_CHAN_IDX_IN_MAX) || (idx < 0)) {
		return -EINVAL;
	}

	sub_idx = imxdpuv1_get_channel_subindex(chan);

	imxdpu->chan_data[idx].dest_top = y;
	imxdpu->chan_data[idx].dest_left = x;

	imxdpu->chan_data[idx].fetch_layer_prop.layeroffset0 =
		IMXDPUV1_SET_FIELD(IMXDPUV1_FETCHDECODE0_LAYEROFFSET0_LAYERXOFFSET0,
		imxdpu->chan_data[idx].dest_left) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_FETCHDECODE0_LAYEROFFSET0_LAYERYOFFSET0,
		imxdpu->chan_data[idx].dest_top);

	if (is_fetch_layer_chan(chan) || is_fetch_warp_chan(chan)) {
		IMXDPUV1_TRACE("%s(): fetch layer or warp\n", __func__);
		imxdpuv1_write(imxdpu,
			offset + IMXDPUV1_FETCHLAYER0_LAYEROFFSET0_OFFSET +
			((IMXDPUV1_SUBCHAN_LAYER_OFFSET * sub_idx)),
			imxdpu->chan_data[idx].fetch_layer_prop.layeroffset0);

	} else if (is_fetch_decode_chan(chan)) {
		if (imxdpu->chan_data[idx].use_eco_fetch) {
			imxdpuv1_disp_set_chan_position(imxdpuv1_id,
				imxdpuv1_get_eco(chan),
				x, y);
		}
		imxdpuv1_write(imxdpu,
			offset + IMXDPUV1_FETCHDECODE0_LAYEROFFSET0_OFFSET,
			imxdpu->chan_data[idx].fetch_layer_prop.layeroffset0);
	} else if (is_fetch_eco_chan(chan)) {
		imxdpuv1_write(imxdpu,
			offset + IMXDPUV1_FETCHECO0_LAYEROFFSET0_OFFSET,
			imxdpu->chan_data[idx].fetch_layer_prop.layeroffset0);
	} else {
		return -EINVAL;
	}

	imxdpuv1_disp_request_shadow_load(imxdpuv1_id,
		imxdpu->chan_data[idx].disp_id,
		IMXDPUV1_SHDLD_IDX_CHAN_00 + idx);

	return ret;
}

/*!
 * This function sets the source and destination crop
 * position of the a channel (window) layer
 *
 * @param	imxdpuv1_id	id of the diplay unit
 * @param	chan    	chan to use
 * @param       clip_top	source y position
 * @param       clip_left	source x position
 * @param       clip_width	source width
 * @param       clip_height	source height
 * @param       dest_top	destination y
 * @param       dest_left	destination x
 * @param       dest_width	destination width
 * @param       dest_height	destination height
 *
 * @return      This function returns 0 on success or negative error code on
 *      	fail.
 */
int imxdpuv1_disp_set_chan_crop(
	int8_t imxdpuv1_id,
	imxdpuv1_chan_t chan,
	int16_t  clip_top,
	int16_t  clip_left,
	uint16_t clip_width,
	uint16_t clip_height,
	int16_t  dest_top,
	int16_t  dest_left,
	uint16_t dest_width,
	uint16_t dest_height)
{
	int ret = 0;
	uint32_t offset;
	int idx;
	int sub_idx;
	struct imxdpuv1_soc *imxdpu;

	IMXDPUV1_TRACE("%s()\n", __func__);

	if (!((imxdpuv1_id >= 0) && (imxdpuv1_id < IMXDPUV1_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpuv1_array[imxdpuv1_id];

	offset = id2blockoffset(get_channel_blk(chan));
	if (offset == IMXDPUV1_OFFSET_INVALID) {
		return -EINVAL;
	}

	idx = get_channel_idx(chan);
	if ((idx >= IMXDPUV1_CHAN_IDX_IN_MAX) || (idx < 0)) {
		return -EINVAL;
	}

	sub_idx = imxdpuv1_get_channel_subindex(chan);

	imxdpu->chan_data[idx].dest_top    = dest_top;
	imxdpu->chan_data[idx].dest_left   = dest_left;
	imxdpu->chan_data[idx].dest_width  = IMXDPUV1_MIN(dest_width, clip_width);
	imxdpu->chan_data[idx].dest_height = IMXDPUV1_MIN(dest_height, clip_height);
	imxdpu->chan_data[idx].clip_top    = clip_top;
	imxdpu->chan_data[idx].clip_left   = clip_left;
	imxdpu->chan_data[idx].clip_width  = IMXDPUV1_MIN(dest_width, clip_width);
	imxdpu->chan_data[idx].clip_height = IMXDPUV1_MIN(dest_height, clip_height);

	/* Need to check more cases here */
	if ((imxdpu->chan_data[idx].clip_height != 0) &&
		(imxdpu->chan_data[idx].clip_width != 0)) {
		imxdpu->chan_data[idx].fetch_layer_prop.layerproperty0 |=
			IMXDPUV1_SET_FIELD(IMXDPUV1_LAYERPROPERTY_CLIPWINDOWENABLE,
			IMXDPUV1_ENABLE);
		imxdpu->chan_data[idx].fetch_layer_prop.clipwindowdimensions0 =
			IMXDPUV1_SET_FIELD(IMXDPUV1_CLIP_HEIGHT,
			imxdpu->chan_data[idx].clip_height - 1) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_CLIP_WIDTH,
			imxdpu->chan_data[idx].clip_width - 1);
	} else {
		imxdpu->chan_data[idx].fetch_layer_prop.layerproperty0 &=
			~IMXDPUV1_LAYERPROPERTY_CLIPWINDOWENABLE_MASK;
		imxdpu->chan_data[idx].fetch_layer_prop.clipwindowdimensions0 = 0;
	}
	imxdpu->chan_data[idx].fetch_layer_prop.layeroffset0 =
		IMXDPUV1_SET_FIELD(IMXDPUV1_LAYER_XOFFSET,
		imxdpu->chan_data[idx].dest_left - imxdpu->chan_data[idx].clip_left) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_LAYER_YOFFSET,
		imxdpu->chan_data[idx].dest_top - imxdpu->chan_data[idx].clip_top);
	imxdpu->chan_data[idx].fetch_layer_prop.clipwindowoffset0 =
		IMXDPUV1_SET_FIELD(IMXDPUV1_CLIP_XOFFSET,
		imxdpu->chan_data[idx].dest_left) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_CLIP_YOFFSET,
		imxdpu->chan_data[idx].dest_top);

	if (is_fetch_layer_chan(chan) || is_fetch_warp_chan(chan)) {
		imxdpuv1_write_block(imxdpu,
			offset +
			IMXDPUV1_FETCHLAYER0_LAYEROFFSET0_OFFSET +
			((IMXDPUV1_SUBCHAN_LAYER_OFFSET * sub_idx)),
			(void *)&imxdpu->chan_data[idx].fetch_layer_prop.layeroffset0,
			5);

	} else if (is_fetch_decode_chan(chan)) {
		if (imxdpu->chan_data[idx].use_eco_fetch) {
			imxdpuv1_disp_set_chan_crop(imxdpuv1_id,
				imxdpuv1_get_eco(chan),
				clip_top,
				clip_left,
				clip_width,
				clip_height,
				dest_top,
				dest_left,
				dest_width,
				dest_height);
		}
		imxdpuv1_write_block(imxdpu,
			offset +
			IMXDPUV1_FETCHDECODE0_LAYEROFFSET0_OFFSET,
			(void *)&imxdpu->chan_data[idx].fetch_layer_prop.layeroffset0,
			5);
	} else if (is_fetch_eco_chan(chan)) {
		imxdpuv1_write_block(imxdpu,
			offset + IMXDPUV1_FETCHECO0_LAYEROFFSET0_OFFSET,
			(void *)&imxdpu->chan_data[idx].fetch_layer_prop.layeroffset0,
			5);

	} else {
		return -EINVAL;
	}
	imxdpuv1_disp_request_shadow_load(imxdpuv1_id,
		imxdpu->chan_data[idx].disp_id,
		IMXDPUV1_SHDLD_IDX_CHAN_00 + idx);

	return ret;
}

/*!
 * This function sets initializes a channel and buffer
 *
 * @param	imxdpuv1_id	id of the diplay unit
 * @param	chan    	chan to use
 * @param       src_pixel_fmt   source pixel format
 * @param       clip_top	source y position
 * @param       clip_left	source x position
 * @param       clip_width	source width
 * @param       clip_height	source height
 * @param       stride		stride of the buffer
 * @param       disp_id		display id
 * @param       dest_top	destination y
 * @param       dest_left	destination x
 * @param       dest_width	destination width
 * @param       dest_height	destination height
 * @param       const_color     constant color for clip region
 * @param       disp_addr	display buffer physical address
 *
 * @return      This function returns 0 on success or negative error code on
 *      	fail.
 */
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
	unsigned int disp_addr)
{
	int ret = 0;
	imxdpuv1_channel_params_t channel;
	uint32_t uv_offset = 0;

	IMXDPUV1_TRACE("%s(): "
		"imxdpuv1_id     %d\n"
		"chan_t chan   %x\n"
		"src_pixel_fmt 0x%x\n"
		"src_width     %d\n"
		"src_height    %d\n"
		"clip_top      %d\n"
		"clip_left     %d\n"
		"clip_width    %d\n"
		"clip_height   %d\n"
		"stride        %d\n"
		"disp_id       %d\n"
		"dest_top      %d\n"
		"dest_left     %d\n"
		"dest_width    %d\n"
		"dest_height   %d\n"
		"const_color   0x%x\n"
		"disp_addr     0x%x\n",
		__func__,
		imxdpuv1_id,
		chan,
		src_pixel_fmt,
		src_width,
		src_height,
		clip_top,
		clip_left,
		clip_width,
		clip_height,
		stride,
		disp_id,
		dest_top,
		dest_left,
		dest_width,
		dest_height,
		const_color,
		disp_addr);

	channel.common.chan = chan;
	channel.common.src_pixel_fmt = src_pixel_fmt;
	channel.common.src_width = src_width;
	channel.common.src_height = src_height;
	channel.common.clip_top = clip_top;
	channel.common.clip_left = clip_left;
	channel.common.clip_width = clip_width;
	channel.common.clip_height = clip_height;
	channel.common.stride = stride;
	channel.common.disp_id = disp_id;
	channel.common.dest_top = dest_top;
	channel.common.dest_left = dest_left;
	channel.common.dest_width = dest_width;
	channel.common.dest_height = dest_height;
	channel.common.const_color = const_color;
	channel.common.use_global_alpha = use_global_alpha;
	channel.common.use_local_alpha = use_local_alpha;

	if (imxdpuv1_get_planes(src_pixel_fmt) == 2) {
		uv_offset = src_width * src_height; /* works for NV12 and NV16*/
	}
	ret = imxdpuv1_init_channel(imxdpuv1_id, &channel);

	ret = imxdpuv1_init_channel_buffer(imxdpuv1_id, channel.common.chan, channel.common.stride, IMXDPUV1_ROTATE_NONE,
		disp_addr,
		uv_offset,
		0);

	ret = imxdpuv1_disp_set_chan_crop(imxdpuv1_id,
		channel.common.chan,
		channel.common.clip_top,
		channel.common.clip_left,
		channel.common.clip_width,
		channel.common.clip_height,
		channel.common.dest_top,
		channel.common.dest_left,
		channel.common.dest_width,
		channel.common.dest_height);

#ifdef DEBUG
	{
		imxdpuv1_chan_t eco_chan;
		imxdpuv1_dump_channel(imxdpuv1_id, channel.common.chan);
		eco_chan = imxdpuv1_get_eco(channel.common.chan);
		if (eco_chan != 0) {
			imxdpuv1_dump_channel(imxdpuv1_id, eco_chan);
		}
	}
#endif
	return ret;
}

/*!
 * This function prints the video mode passed as a parameter
 *
 * @param	*mode	pointer to video mode struct to show
 */
void imxdpuv1_disp_dump_mode(const struct imxdpuv1_videomode *mode)
{
	IMXDPUV1_PRINT("%s():\n", __func__);
	IMXDPUV1_PRINT("\thlen   %4d\n", mode->hlen);
	IMXDPUV1_PRINT("\thfp    %4d\n", mode->hfp);
	IMXDPUV1_PRINT("\thbp    %4d\n", mode->hbp);
	IMXDPUV1_PRINT("\thsync  %4d\n", mode->hsync);
	IMXDPUV1_PRINT("\tvlen   %4d\n", mode->vlen);
	IMXDPUV1_PRINT("\tvfp    %4d\n", mode->vfp);
	IMXDPUV1_PRINT("\tvbp    %4d\n", mode->vbp);
	IMXDPUV1_PRINT("\tvsync  %4d\n", mode->vsync);
	IMXDPUV1_PRINT("\tvlen1  %4d\n", mode->vlen1);
	IMXDPUV1_PRINT("\tvfp1   %4d\n", mode->vfp1);
	IMXDPUV1_PRINT("\tvbp1   %4d\n", mode->vbp1);
	IMXDPUV1_PRINT("\tvsync1 %4d\n", mode->vsync1);

	IMXDPUV1_PRINT("\tflags 0x%08x:\n", mode->flags);

	if (mode->flags & IMXDPUV1_MODE_FLAGS_HSYNC_POL)
		IMXDPUV1_PRINT("\t\tIMXDPUV1_MODE_FLAGS_HSYNC_POL is high\n");
	else
		IMXDPUV1_PRINT("\t\tIMXDPUV1_MODE_FLAGS_HSYNC_POL is low\n");
	if (mode->flags & IMXDPUV1_MODE_FLAGS_VSYNC_POL)
		IMXDPUV1_PRINT("\t\tIMXDPUV1_MODE_FLAGS_VSYNC_POL is high\n");
	else
		IMXDPUV1_PRINT("\t\tIMXDPUV1_MODE_FLAGS_VSYNC_POL is low\n");
	if (mode->flags & IMXDPUV1_MODE_FLAGS_DE_POL)
		IMXDPUV1_PRINT("\t\tIMXDPUV1_MODE_FLAGS_DE_POL is high\n");
	else
		IMXDPUV1_PRINT("\t\tIMXDPUV1_MODE_FLAGS_DE_POL is low\n");

	if (mode->flags & IMXDPUV1_MODE_FLAGS_INTERLACED)
		IMXDPUV1_PRINT("\t\tIMXDPUV1_MODE_FLAGS_INTERLACED is set\n");
	if (mode->flags & IMXDPUV1_MODE_FLAGS_LRSYNC)
		IMXDPUV1_PRINT("\t\tIMXDPUV1_MODE_FLAGS_LRSYNC is set\n");
	if (mode->flags & IMXDPUV1_MODE_FLAGS_SPLIT)
		IMXDPUV1_PRINT("\t\tIMXDPUV1_MODE_FLAGS_SPLIT is set\n");
	if (mode->flags & IMXDPUV1_MODE_FLAGS_32BIT)
		IMXDPUV1_PRINT("\t\tIMXDPUV1_MODE_FLAGS_32BIT is set\n");
	if (mode->flags & IMXDPUV1_MODE_FLAGS_BT656_10BIT)
		IMXDPUV1_PRINT("\t\tIMXDPUV1_MODE_FLAGS_BT656_10BIT is set\n");
	if (mode->flags & IMXDPUV1_MODE_FLAGS_BT656_8BIT)
		IMXDPUV1_PRINT("\t\tIMXDPUV1_MODE_FLAGS_BT656_8BIT is set\n");
}

/*!
 * Returns the bytes per pixel
 *
 * @param	pixel format
 *
 * @return      returns number of bytes per pixel or zero
 *      	if the format is not matched.
 */
int imxdpuv1_bytes_per_pixel(uint32_t fmt)
{
	IMXDPUV1_TRACE("%s():\n", __func__);
	switch (fmt) {
	/* todo add NV12, and NV16 */
	case IMXDPUV1_PIX_FMT_NV12:
		return 1; /* luma */

	case IMXDPUV1_PIX_FMT_RGB565:
	case IMXDPUV1_PIX_FMT_YUYV:
	case IMXDPUV1_PIX_FMT_UYVY:
		return 2;
		break;
	case IMXDPUV1_PIX_FMT_BGR24:
	case IMXDPUV1_PIX_FMT_RGB24:
	case IMXDPUV1_PIX_FMT_YUV444:
		return 3;
		break;
	case IMXDPUV1_PIX_FMT_GENERIC_32:
	case IMXDPUV1_PIX_FMT_BGR32:
	case IMXDPUV1_PIX_FMT_BGRA32:
	case IMXDPUV1_PIX_FMT_RGB32:
	case IMXDPUV1_PIX_FMT_RGBA32:
	case IMXDPUV1_PIX_FMT_ABGR32:
	case IMXDPUV1_PIX_FMT_AYUV:
		return 4;
		break;
	default:
		IMXDPUV1_TRACE("%s(): unsupported pixel format", __func__);
		return 0;
	}
}

/*!
 * Returns the number of bits per color component for the color
 * component bits register
 *
 * @param	pixel format
 *
 * @return      Returns the number of bits per color component for
 *      	the color component bits register.
 */
uint32_t imxdpuv1_get_colorcomponentbits(uint32_t fmt)
{
	IMXDPUV1_TRACE("%s():\n", __func__);
	switch (fmt) {
	/* todo add NV12, NV16, YUYV, and  UYVY */
	case IMXDPUV1_PIX_FMT_YUYV:
	case IMXDPUV1_PIX_FMT_UYVY:
		return
		IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_BITSRED0, 0x08) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_BITSGREEN0, 0x08) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_BITSBLUE0, 0x08) |
		IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_BITSALPHA0, 0x00);
	case IMXDPUV1_PIX_FMT_NV12:
		return
			IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_BITSRED0, 0x08) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_BITSGREEN0, 0x00) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_BITSBLUE0, 0x00) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_BITSALPHA0, 0x00);

	case IMXDPUV1_PIX_FMT_RGB565:
		return IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_BITSRED0, 0) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_BITSGREEN0, 5) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_BITSBLUE0, 11) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_BITSALPHA0, 0);

	case IMXDPUV1_PIX_FMT_BGR24:
	case IMXDPUV1_PIX_FMT_RGB24:
	case IMXDPUV1_PIX_FMT_YUV444:
	case IMXDPUV1_PIX_FMT_BGR32:
	case IMXDPUV1_PIX_FMT_RGB32:
		return IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_BITSRED0, 0x08) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_BITSGREEN0, 0x08) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_BITSBLUE0, 0x08) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_BITSALPHA0, 0x0);

	case IMXDPUV1_PIX_FMT_GENERIC_32:
	case IMXDPUV1_PIX_FMT_BGRA32:
	case IMXDPUV1_PIX_FMT_RGBA32:
	case IMXDPUV1_PIX_FMT_ABGR32:
	case IMXDPUV1_PIX_FMT_ARGB32:
	case IMXDPUV1_PIX_FMT_AYUV:
		return
			IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_BITSRED0, 0x08) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_BITSGREEN0, 0x08) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_BITSBLUE0, 0x08) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_BITSALPHA0, 0x08);
	default:
		IMXDPUV1_TRACE("%s(): unsupported pixel format 0x%08x", __func__, fmt);
		return 0;
	}
	return 0;
}

/*!
 * Returns the number of planes for the pixel format
 *
 * @param	pixel format
 *
 * @return      returns number of bytes per pixel or zero
 *      	if the format is not matched.
 */
uint32_t imxdpuv1_get_planes(uint32_t fmt)
{
	IMXDPUV1_TRACE("%s():\n", __func__);
	switch (fmt) {
	case IMXDPUV1_PIX_FMT_NV16:
	case IMXDPUV1_PIX_FMT_NV12:
		return  2;

	case IMXDPUV1_PIX_FMT_RGB565:
	case IMXDPUV1_PIX_FMT_YUYV:
	case IMXDPUV1_PIX_FMT_UYVY:
	case IMXDPUV1_PIX_FMT_BGRA32:
	case IMXDPUV1_PIX_FMT_RGBA32:
	case IMXDPUV1_PIX_FMT_ABGR32:
	case IMXDPUV1_PIX_FMT_AYUV:
	case IMXDPUV1_PIX_FMT_BGR24:
	case IMXDPUV1_PIX_FMT_RGB24:
	case IMXDPUV1_PIX_FMT_YUV444:
	case IMXDPUV1_PIX_FMT_BGR32:
	case IMXDPUV1_PIX_FMT_RGB32:
	case IMXDPUV1_PIX_FMT_ARGB32:
		return 1;
	default:
		return 0;
		IMXDPUV1_TRACE("%s(): unsupported pixel format", __func__);
	}
}

/*!
 * Returns the color component bit position shifts
 *
 * @param	pixel format
 *
 * @return      returns the register setting for the
  *		colorcomponentshift register
  *
 */
uint32_t imxdpuv1_get_colorcomponentshift(uint32_t fmt)
{
	IMXDPUV1_TRACE("%s():\n", __func__);
	switch (fmt) {

	case IMXDPUV1_PIX_FMT_NV12:
		return IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_SHIFTRED0, 0x0) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_SHIFTGREEN0, 0x0) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_SHIFTBLUE0, 0x0) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_SHIFTALPHA0, 0x0);

	case IMXDPUV1_PIX_FMT_RGB565:
		return IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_SHIFTRED0, 5) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_SHIFTGREEN0, 6) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_SHIFTBLUE0, 5) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_SHIFTALPHA0, 0);
	case IMXDPUV1_PIX_FMT_YUYV:
		return IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_SHIFTRED0, 0x0) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_SHIFTGREEN0, 0x8) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_SHIFTBLUE0, 0x8) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_SHIFTALPHA0, 0x0);
	case IMXDPUV1_PIX_FMT_UYVY:
		return IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_SHIFTRED0, 0x8) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_SHIFTGREEN0, 0x0) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_SHIFTBLUE0, 0x0) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_SHIFTALPHA0, 0x0);

	case IMXDPUV1_PIX_FMT_BGR24:
	case IMXDPUV1_PIX_FMT_BGR32:
	case IMXDPUV1_PIX_FMT_BGRA32:
		/* 0xaaRRGGBB */
		return IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_SHIFTRED0, 0x10) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_SHIFTGREEN0, 0x08) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_SHIFTBLUE0, 0x00) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_SHIFTALPHA0, 0x18);
	case IMXDPUV1_PIX_FMT_AYUV:
		/* 0xVVUUYYAA  */
		return IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_SHIFTRED0, 0x08) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_SHIFTGREEN0, 0x10) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_SHIFTBLUE0, 0x18) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_SHIFTALPHA0, 0x00);

	case IMXDPUV1_PIX_FMT_ABGR32:
		/* 0xRRGGBBAA */
		return IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_SHIFTRED0, 0x18) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_SHIFTGREEN0, 0x10) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_SHIFTBLUE0, 0x08) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_SHIFTALPHA0, 0x00);

	case IMXDPUV1_PIX_FMT_ARGB32:
		/* 0xBBGGRRAA */
		return IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_SHIFTRED0, 0x08) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_SHIFTGREEN0, 0x10) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_SHIFTBLUE0, 0x18) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_SHIFTALPHA0, 0x00);
	case IMXDPUV1_PIX_FMT_GENERIC_32:
	case IMXDPUV1_PIX_FMT_RGB24:
	case IMXDPUV1_PIX_FMT_YUV444:
	case IMXDPUV1_PIX_FMT_RGB32:
	case IMXDPUV1_PIX_FMT_RGBA32:
		/* 0xaaBBGGRR or 0xaaUUVVYY */
		return IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_SHIFTRED0, 0x00) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_SHIFTGREEN0, 0x08) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_SHIFTBLUE0, 0x10) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_SHIFTALPHA0, 0x18);
	default:
		return 0;
		IMXDPUV1_TRACE("%s(): unsupported pixel format", __func__);
	}
}

/*!
 * Returns true is the format has local alpha
 *
 * @param	pixel format
 *
 * @return      Returns true is the format has local alpha
 */
uint32_t imxdpuv1_has_localalpha(uint32_t fmt)
{
	IMXDPUV1_TRACE("%s():\n", __func__);
	switch (fmt) {
	case IMXDPUV1_PIX_FMT_BGRA32:
	case IMXDPUV1_PIX_FMT_AYUV:
	case IMXDPUV1_PIX_FMT_RGBA32:
		return IMXDPUV1_TRUE;
	default:
		return IMXDPUV1_FALSE;
	}
}

/*!
 * Returns the bits per pixel
 *
 * @param	pixel format
 *
 * @return      returns number of bits per pixel or zero
 *      	if the format is not matched.
 */
int imxdpuv1_bits_per_pixel(uint32_t fmt)
{
	int ret = 0;
	switch (fmt) {
	case IMXDPUV1_PIX_FMT_NV12:
		ret = 8;
		break;
	case IMXDPUV1_PIX_FMT_NV16:
	case IMXDPUV1_PIX_FMT_RGB565:
	case IMXDPUV1_PIX_FMT_YUYV:
	case IMXDPUV1_PIX_FMT_UYVY:
	case IMXDPUV1_PIX_FMT_YVYU:
		ret = 16;
		break;
	case IMXDPUV1_PIX_FMT_BGR24:
	case IMXDPUV1_PIX_FMT_RGB24:
	case IMXDPUV1_PIX_FMT_YUV444:
		ret = 24;
		break;

	case IMXDPUV1_PIX_FMT_GENERIC_32:
	case IMXDPUV1_PIX_FMT_BGR32:
	case IMXDPUV1_PIX_FMT_BGRA32:
	case IMXDPUV1_PIX_FMT_RGB32:
	case IMXDPUV1_PIX_FMT_RGBA32:
	case IMXDPUV1_PIX_FMT_ABGR32:
	case IMXDPUV1_PIX_FMT_ARGB32:
	case IMXDPUV1_PIX_FMT_AYUV:
		ret = 32;
		break;
	default:
		IMXDPUV1_TRACE("%s(): unsupported pixel format\n", __func__);
		ret = 1;
		break;
	}
	IMXDPUV1_TRACE("%s(): fmt 0x%08x, ret %d\n", __func__, fmt, ret);

	return ret;
}

/*!
 * Tests for YUV
 *
 * @param	pixel format
 *
 * @return      returns true if the format is YUV.
 */
static bool imxdpuv1_is_yuv(uint32_t fmt)
{
	int ret = IMXDPUV1_FALSE;
	switch (fmt) {
	case IMXDPUV1_PIX_FMT_AYUV:
	case IMXDPUV1_PIX_FMT_NV12:
	case IMXDPUV1_PIX_FMT_NV16:
	case IMXDPUV1_PIX_FMT_YUYV:
	case IMXDPUV1_PIX_FMT_UYVY:
	case IMXDPUV1_PIX_FMT_YUV444:
		ret = IMXDPUV1_TRUE;
		break;
	case IMXDPUV1_PIX_FMT_GENERIC_32:
	case IMXDPUV1_PIX_FMT_BGR32:
	case IMXDPUV1_PIX_FMT_BGRA32:
	case IMXDPUV1_PIX_FMT_RGB32:
	case IMXDPUV1_PIX_FMT_RGBA32:
	case IMXDPUV1_PIX_FMT_ABGR32:
	case IMXDPUV1_PIX_FMT_ARGB32:
	case IMXDPUV1_PIX_FMT_RGB565:
	case IMXDPUV1_PIX_FMT_BGR24:
	case IMXDPUV1_PIX_FMT_RGB24:
		ret = IMXDPUV1_FALSE;
		break;

	default:
		IMXDPUV1_TRACE("%s(): unsupported pixel format", __func__);
		ret = IMXDPUV1_FALSE;
		break;
	}
	IMXDPUV1_TRACE("%s(): fmt 0x%08x, ret %d\n", __func__, fmt, ret);

	return ret;
}

/*!
 * Tests for RGB formats
 *
 * @param	pixel format
 *
 * @return 	returns true if the format is any supported RGB
 */
bool imxdpuv1_is_rgb(uint32_t fmt)
{
	int ret = IMXDPUV1_FALSE;
	switch (fmt) {
	case IMXDPUV1_PIX_FMT_AYUV:
	case IMXDPUV1_PIX_FMT_NV12:
	case IMXDPUV1_PIX_FMT_NV16:
	case IMXDPUV1_PIX_FMT_YUYV:
	case IMXDPUV1_PIX_FMT_UYVY:
	case IMXDPUV1_PIX_FMT_YUV444:
	case IMXDPUV1_PIX_FMT_GENERIC_32:
		ret = IMXDPUV1_FALSE;
		break;
	case IMXDPUV1_PIX_FMT_BGR32:
	case IMXDPUV1_PIX_FMT_BGRA32:
	case IMXDPUV1_PIX_FMT_RGB32:
	case IMXDPUV1_PIX_FMT_RGBA32:
	case IMXDPUV1_PIX_FMT_ABGR32:
	case IMXDPUV1_PIX_FMT_ARGB32:
	case IMXDPUV1_PIX_FMT_RGB565:
	case IMXDPUV1_PIX_FMT_BGR24:
	case IMXDPUV1_PIX_FMT_RGB24:
		ret = IMXDPUV1_TRUE;
		break;

	default:
		IMXDPUV1_TRACE("%s(): unsupported pixel format", __func__);
		ret = IMXDPUV1_FALSE;
		break;
	}
	IMXDPUV1_TRACE("%s(): fmt 0x%08x, ret %d\n", __func__, fmt, ret);

	return ret;
}

/*!
 * Intializes buffers to be used for a channel
 *
 * @param       imxdpuv1_id       id of the diplay unit
 * @param       chan    	channel to use for this buffer
 * @param       stride  	total width in the buffer in pixels
 * @param       rot_mode	rotatation mode
 * @param       phyaddr_0       buffer 0 address
 * @param       u_offset	U offset
 * @param       v_offset	V offset
 *
 * @return      This function returns 0 on success or negative error code on
 *      	fail.
 */
int imxdpuv1_init_channel_buffer(
	int8_t imxdpuv1_id,
	imxdpuv1_chan_t chan,
	uint32_t stride,
	imxdpuv1_rotate_mode_t rot_mode,
	dma_addr_t phyaddr_0,
	uint32_t u_offset,
	uint32_t v_offset)
{
	int ret = 0;
	uint32_t b_off;
	struct imxdpuv1_soc *imxdpu;
	imxdpuv1_chan_idx_t chan_idx = get_channel_idx(chan);
	int sub_idx = imxdpuv1_get_channel_subindex(chan);
	bool enable_clip = IMXDPUV1_FALSE;
	bool enable_buffer = IMXDPUV1_TRUE;
	uint8_t enable_yuv = IMXDPUV1_LAYERPROPERTY_YUVCONVERSIONMODE__OFF;
	uint8_t input_select = IMXDPUV1_FETCHDECODE0_CONTROL_INPUTSELECT__INACTIVE;
	uint32_t fwidth;
	uint32_t fheight;

	IMXDPUV1_TRACE("%s()\n", __func__);

	if (!((imxdpuv1_id >= 0) && (imxdpuv1_id < IMXDPUV1_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpuv1_array[imxdpuv1_id];

	if (!is_chan(chan)) {
		return -EINVAL;
	}

	b_off = id2blockoffset(get_channel_blk(chan));
	if (b_off == IMXDPUV1_OFFSET_INVALID) {
		return -EINVAL;
	}

	imxdpu->chan_data[chan_idx].phyaddr_0 = phyaddr_0;
	imxdpu->chan_data[chan_idx].u_offset = u_offset;
	imxdpu->chan_data[chan_idx].v_offset = v_offset;

	/* update stride if provided */
	if (stride != 0) {
		/* todo: check stride range */
		imxdpu->chan_data[chan_idx].stride = stride;
	}

	/* common fetch setup */
	if (!is_store_chan(chan)) {
		/* default horizontal scan
		 * todo: add support for vertical and warp scans
		 */
		if (sub_idx == 0) {
			imxdpuv1_write(imxdpu,
				b_off +
				IMXDPUV1_FETCHDECODE0_BURSTBUFFERMANAGEMENT_OFFSET,
				IMXDPUV1_SET_FIELD(
					IMXDPUV1_FETCHDECODE0_BURSTBUFFERMANAGEMENT_SETBURSTLENGTH,
					burst_param[IMXDPUV1_BURST_HORIZONTAL].
					len) |
				IMXDPUV1_SET_FIELD(
					IMXDPUV1_FETCHDECODE0_BURSTBUFFERMANAGEMENT_SETNUMBUFFERS,
					burst_param[IMXDPUV1_BURST_HORIZONTAL].buffers));
		}
		/* todo: Add range checking here */
		imxdpu->chan_data[chan_idx].fetch_layer_prop.baseaddress0 = phyaddr_0;
		imxdpu->chan_data[chan_idx].fetch_layer_prop.sourcebufferattributes0 =
			IMXDPUV1_SET_FIELD(IMXDPUV1_BUFF_ATTR_BITSPERPIXEL,
			imxdpuv1_bits_per_pixel(
				imxdpu->chan_data[chan_idx].src_pixel_fmt)) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_BUFF_ATTR_STRIDE,
			imxdpu->chan_data[chan_idx].stride - 1);
		imxdpu->chan_data[chan_idx].fetch_layer_prop.sourcebufferdimension0 =
			IMXDPUV1_SET_FIELD(IMXDPUV1_BUFF_DIMEN_LINECOUNT,
			imxdpu->chan_data[chan_idx].src_height - 1) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_BUFF_DIMEN_LINEWIDTH,
			imxdpu->chan_data[chan_idx].src_width - 1);
		imxdpu->chan_data[chan_idx].fetch_layer_prop.colorcomponentbits0 =
			imxdpuv1_get_colorcomponentbits(
			imxdpu->chan_data[chan_idx].src_pixel_fmt);
		imxdpu->chan_data[chan_idx].fetch_layer_prop.colorcomponentshift0 =
			imxdpuv1_get_colorcomponentshift(
			imxdpu->chan_data[chan_idx].src_pixel_fmt);

		imxdpu->chan_data[chan_idx].fetch_layer_prop.layeroffset0 =
			IMXDPUV1_SET_FIELD(IMXDPUV1_LAYER_XOFFSET,
			imxdpu->chan_data[chan_idx].dest_left) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_LAYER_YOFFSET,
			imxdpu->chan_data[chan_idx].dest_top);
		imxdpu->chan_data[chan_idx].fetch_layer_prop.clipwindowoffset0 =
			IMXDPUV1_SET_FIELD(IMXDPUV1_CLIP_XOFFSET,
			imxdpu->chan_data[chan_idx].clip_left) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_CLIP_YOFFSET,
			imxdpu->chan_data[chan_idx].clip_top);
		imxdpu->chan_data[chan_idx].fetch_layer_prop.clipwindowdimensions0 =
			IMXDPUV1_SET_FIELD(IMXDPUV1_CLIP_HEIGHT,
			imxdpu->chan_data[chan_idx].clip_height - 1) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_CLIP_WIDTH,
			imxdpu->chan_data[chan_idx].clip_width - 1);
		if ((imxdpu->chan_data[chan_idx].clip_height != 0) &&
			(imxdpu->chan_data[chan_idx].clip_width != 0)) {
			imxdpu->chan_data[chan_idx].fetch_layer_prop.clipwindowdimensions0 =
				IMXDPUV1_SET_FIELD(IMXDPUV1_CLIP_HEIGHT,
				imxdpu->chan_data[chan_idx].clip_height - 1) |
				IMXDPUV1_SET_FIELD(IMXDPUV1_CLIP_WIDTH,
				imxdpu->chan_data[chan_idx].clip_width - 1);

			enable_clip = IMXDPUV1_ENABLE;
		} else {
			imxdpu->chan_data[chan_idx].fetch_layer_prop.clipwindowdimensions0 = 0;
		}

		imxdpu->chan_data[chan_idx].fetch_layer_prop.constantcolor0 =
			imxdpu->chan_data[chan_idx].const_color;

		if (imxdpu->chan_data[chan_idx].phyaddr_0 == 0) {
			enable_buffer = IMXDPUV1_FALSE;
		}
		if (imxdpuv1_is_yuv(imxdpu->chan_data[chan_idx].src_pixel_fmt)) {
			/* TODO: need to get correct encoding range */
			enable_yuv = IMXDPUV1_LAYERPROPERTY_YUVCONVERSIONMODE__ITU601;
		}
	}


	if (is_fetch_decode_chan(chan)) {
		IMXDPUV1_TRACE("%s(): fetch decode channel\n", __func__);
		if (imxdpu->chan_data[chan_idx].use_eco_fetch) {
			input_select = IMXDPUV1_FETCHDECODE0_CONTROL_INPUTSELECT__COMPPACK;
			if (chan == IMXDPUV1_CHAN_01) {
				imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_FETCHDECODE0_DYNAMIC,
					IMXDPUV1_SET_FIELD(
						IMXDPUV1_PIXENGCFG_SRC_SEL,
						IMXDPUV1_PIXENGCFG_FETCHDECODE0_DYNAMIC_FETCHDECODE0_SRC_SEL__FETCHECO0));
			} else if (chan == IMXDPUV1_CHAN_19) {
				imxdpuv1_write(imxdpu, IMXDPUV1_PIXENGCFG_FETCHDECODE1_DYNAMIC,
					IMXDPUV1_SET_FIELD(
						IMXDPUV1_PIXENGCFG_SRC_SEL,
						IMXDPUV1_PIXENGCFG_FETCHDECODE1_DYNAMIC_FETCHDECODE1_SRC_SEL__FETCHECO1));
			}
			imxdpuv1_init_channel_buffer(imxdpuv1_id,
				imxdpuv1_get_eco(chan),
				stride,
				rot_mode,
				phyaddr_0,
				u_offset, v_offset);

			imxdpu->chan_data[chan_idx].fetch_layer_prop.colorcomponentbits0 =
				(0x08 << IMXDPUV1_FETCHDECODE0_COLORCOMPONENTBITS0_COMPONENTBITSRED0_SHIFT);
			imxdpu->chan_data[chan_idx].fetch_layer_prop.colorcomponentshift0 =
				(0x00 << IMXDPUV1_FETCHDECODE0_COLORCOMPONENTSHIFT0_COMPONENTSHIFTRED0_SHIFT);

		} /* else need to handle Alpha, Warp, CLUT ... */

		imxdpu->chan_data[chan_idx].fetch_layer_prop.layerproperty0 =
			IMXDPUV1_SET_FIELD(IMXDPUV1_LAYERPROPERTY_SOURCEBUFFERENABLE,
			enable_buffer) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_LAYERPROPERTY_YUVCONVERSIONMODE,
			enable_yuv) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_LAYERPROPERTY_CLIPWINDOWENABLE,
				enable_clip) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_LAYERPROPERTY_ALPHACONSTENABLE,
				imxdpu->chan_data[chan_idx].use_global_alpha) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_LAYERPROPERTY_ALPHASRCENABLE,
				imxdpu->chan_data[chan_idx].use_local_alpha);

		/* todo: handle all cases for control register */
		imxdpuv1_write(imxdpu,
			b_off + IMXDPUV1_FETCHDECODE0_CONTROL_OFFSET,
			IMXDPUV1_SET_FIELD(IMXDPUV1_FETCHDECODE0_CONTROL_YUV422UPSAMPLINGMODE,
				IMXDPUV1_FETCHDECODE0_CONTROL_YUV422UPSAMPLINGMODE__INTERPOLATE) |
			IMXDPUV1_FETCHDECODE0_CONTROL_PALETTEIDXWIDTH_MASK | /* needed ?*/
			IMXDPUV1_SET_FIELD(IMXDPUV1_FETCHDECODE0_CONTROL_CLIPCOLOR, 1) |  /*needed for clip */
			IMXDPUV1_SET_FIELD(IMXDPUV1_FETCHDECODE0_CONTROL_INPUTSELECT, input_select)); /*needed for eco */

		imxdpuv1_write(imxdpu,
			b_off + IMXDPUV1_FETCHDECODE0_FRAMEDIMENSIONS_OFFSET,
			IMXDPUV1_SET_FIELD
			(IMXDPUV1_FETCHDECODE0_FRAMEDIMENSIONS_FRAMEHEIGHT,
				imxdpu->chan_data[chan_idx].dest_height -
				1 /*fheight-1 */) |
			IMXDPUV1_SET_FIELD
			(IMXDPUV1_FETCHDECODE0_FRAMEDIMENSIONS_FRAMEWIDTH,
				imxdpu->chan_data[chan_idx].dest_width -
				1 /*fwidth-1 */));

		imxdpuv1_write_block(imxdpu,
			b_off + IMXDPUV1_FETCHDECODE0_BASEADDRESS0_OFFSET,
			(void *)&imxdpu->chan_data[chan_idx].
			fetch_layer_prop,
			sizeof(fetch_layer_setup_t) / 4);
		imxdpuv1_disp_request_shadow_load(imxdpuv1_id,
			imxdpu->chan_data[chan_idx].
			disp_id,
			IMXDPUV1_SHDLD_IDX_CHAN_00 +
			chan_idx);
	} else if (is_fetch_layer_chan(chan)) {
		IMXDPUV1_TRACE("%s(): fetch layer channel\n", __func__);
		/* here the frame is shared for all sub layers so we use
		   the video mode dimensions.
		   fetch layer sub 1 must be setup first
		   todo:  add a check so that any sub layer can set this */
		if (is_fetch_layer_sub_chan1(chan)) {
			IMXDPUV1_TRACE("%s(): fetch layer sub channel 1\n",
				__func__);
			fwidth =
				imxdpuv1_array[imxdpuv1_id].
				video_mode[imxdpuv1_array[imxdpuv1_id].
				chan_data[chan_idx].disp_id].hlen;
			fheight =
				imxdpuv1_array[imxdpuv1_id].
				video_mode[imxdpuv1_array[imxdpuv1_id].
				chan_data[chan_idx].disp_id].vlen;

			imxdpuv1_write(imxdpu,
				b_off + IMXDPUV1_FETCHLAYER0_CONTROL_OFFSET,
				IMXDPUV1_FETCHDECODE0_CONTROL_PALETTEIDXWIDTH_MASK | /* needed ?*/
				IMXDPUV1_SET_FIELD(IMXDPUV1_FETCHDECODE0_CONTROL_CLIPCOLOR, 1)
				); /*needed for eco */

			imxdpuv1_write(imxdpu,
				b_off +
				IMXDPUV1_FETCHLAYER0_FRAMEDIMENSIONS_OFFSET,
				IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMEHEIGHT,
					/*imxdpu->chan_data[chan_idx].dest_height-1 */
					fheight - 1) |
				IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMEWIDTH,
					/*imxdpu->chan_data[chan_idx].dest_width-1 */
					fwidth - 1));
		}
		imxdpu->chan_data[chan_idx].fetch_layer_prop.layerproperty0 =
			IMXDPUV1_SET_FIELD(IMXDPUV1_LAYERPROPERTY_SOURCEBUFFERENABLE,
			enable_buffer) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_LAYERPROPERTY_YUVCONVERSIONMODE,
			enable_yuv) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_LAYERPROPERTY_CLIPWINDOWENABLE,
				enable_clip) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_LAYERPROPERTY_ALPHACONSTENABLE,
				imxdpu->chan_data[chan_idx].use_global_alpha) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_LAYERPROPERTY_ALPHASRCENABLE,
				imxdpu->chan_data[chan_idx].use_local_alpha);

		imxdpuv1_write_block(imxdpu,
			b_off +
			IMXDPUV1_FETCHLAYER0_BASEADDRESS0_OFFSET +
			((IMXDPUV1_SUBCHAN_LAYER_OFFSET * sub_idx)),
			(void *)&imxdpu->chan_data[chan_idx].
			fetch_layer_prop,
			sizeof(fetch_layer_setup_t) / 4);
		imxdpuv1_write(imxdpu,
			b_off + IMXDPUV1_FETCHLAYER0_TRIGGERENABLE_OFFSET,
			get_channel_sub(chan));
		imxdpuv1_disp_request_shadow_load(imxdpuv1_id,
			imxdpu->chan_data[chan_idx].
			disp_id,
			IMXDPUV1_SHDLD_IDX_CHAN_00 +
			chan_idx);
	} else if (is_fetch_warp_chan(chan)) {
		/* here the frame is shared for all sub layers so we use
		   the video mode dimensions.
		   fetch layer sub 1 must be setup first
		   todo:  add a check so that any sub layer can set this */
		if (is_fetch_layer_sub_chan1(chan)) {
			IMXDPUV1_TRACE("%s(): fetch layer sub channel 1\n",
				__func__);
			fwidth =
				imxdpuv1_array[imxdpuv1_id].
				video_mode[imxdpuv1_array[imxdpuv1_id].
				chan_data[chan_idx].disp_id].hlen;
			fheight =
				imxdpuv1_array[imxdpuv1_id].
				video_mode[imxdpuv1_array[imxdpuv1_id].
				chan_data[chan_idx].disp_id].vlen;

			imxdpuv1_write(imxdpu,
				b_off + IMXDPUV1_FETCHWARP2_CONTROL_OFFSET, 0x700);

			imxdpuv1_write(imxdpu,
				b_off +
				IMXDPUV1_FETCHLAYER0_FRAMEDIMENSIONS_OFFSET,
				IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMEHEIGHT,
					/*imxdpu->chan_data[chan_idx].dest_height-1 */
					fheight - 1) |
				IMXDPUV1_SET_FIELD(IMXDPUV1_FRAMEWIDTH,
					/*imxdpu->chan_data[chan_idx].dest_width-1 */
					fwidth - 1));
		}
		imxdpu->chan_data[chan_idx].fetch_layer_prop.layerproperty0 =
			IMXDPUV1_SET_FIELD(IMXDPUV1_LAYERPROPERTY_SOURCEBUFFERENABLE,
			enable_buffer) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_LAYERPROPERTY_YUVCONVERSIONMODE,
			enable_yuv) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_LAYERPROPERTY_CLIPWINDOWENABLE,
				enable_clip) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_LAYERPROPERTY_ALPHACONSTENABLE,
				imxdpu->chan_data[chan_idx].use_global_alpha) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_LAYERPROPERTY_ALPHASRCENABLE,
				imxdpu->chan_data[chan_idx].use_local_alpha);

		imxdpuv1_write_block(imxdpu,
			b_off +
			IMXDPUV1_FETCHWARP2_BASEADDRESS0_OFFSET +
			(IMXDPUV1_SUBCHAN_LAYER_OFFSET * sub_idx),
			(void *)&imxdpu->chan_data[chan_idx].
			fetch_layer_prop,
			sizeof(fetch_layer_setup_t) / 4);
		imxdpuv1_write(imxdpu,
			b_off + IMXDPUV1_FETCHWARP2_TRIGGERENABLE_OFFSET,
			get_channel_sub(chan));
		imxdpuv1_disp_request_shadow_load(imxdpuv1_id,
			imxdpu->chan_data[chan_idx].
			disp_id,
			IMXDPUV1_SHDLD_IDX_CHAN_00 +
			chan_idx);
	} else if (is_fetch_eco_chan(chan)) {
		IMXDPUV1_TRACE("%s(): fetch eco setup\n", __func__);
		if (imxdpu->chan_data[chan_idx].src_pixel_fmt == IMXDPUV1_PIX_FMT_NV12) {
			imxdpu->chan_data[chan_idx].fetch_layer_prop.baseaddress0 =  phyaddr_0 + u_offset;
			imxdpu->chan_data[chan_idx].fetch_layer_prop.sourcebufferattributes0 =
				IMXDPUV1_SET_FIELD(IMXDPUV1_BUFF_ATTR_BITSPERPIXEL, 16) |
				IMXDPUV1_SET_FIELD(IMXDPUV1_BUFF_ATTR_STRIDE,
				imxdpu->chan_data[chan_idx].stride - 1);

			/* chroma resolution*/
			imxdpu->chan_data[chan_idx].fetch_layer_prop.sourcebufferdimension0 =
				IMXDPUV1_SET_FIELD(IMXDPUV1_BUFF_DIMEN_LINECOUNT,
				imxdpu->chan_data[chan_idx].src_height / 2 - 1) |
				IMXDPUV1_SET_FIELD(IMXDPUV1_BUFF_DIMEN_LINEWIDTH,
				imxdpu->chan_data[chan_idx].src_width / 2 - 1);

			imxdpu->chan_data[chan_idx].fetch_layer_prop.colorcomponentbits0 =
				IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_BITSRED0, 0x0) |
				IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_BITSGREEN0, 0x8) |
				IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_BITSBLUE0, 0x8) |
				IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_BITSALPHA0, 0x0);

			imxdpu->chan_data[chan_idx].fetch_layer_prop.colorcomponentshift0 =
				IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_SHIFTRED0, 0x0) |
				IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_SHIFTGREEN0, 0x0) |
				IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_SHIFTBLUE0, 0x8) |
				IMXDPUV1_SET_FIELD(IMXDPUV1_COLOR_SHIFTALPHA0, 0x0);
			imxdpu->chan_data[chan_idx].fetch_layer_prop.layerproperty0 =
				IMXDPUV1_SET_FIELD(IMXDPUV1_LAYERPROPERTY_SOURCEBUFFERENABLE,
				enable_buffer) |
				IMXDPUV1_SET_FIELD(IMXDPUV1_LAYERPROPERTY_CLIPWINDOWENABLE,
				enable_clip);

			imxdpuv1_write(imxdpu,
				b_off + IMXDPUV1_FETCHECO0_FRAMERESAMPLING_OFFSET,
				IMXDPUV1_SET_FIELD(IMXDPUV1_FETCHECO0_FRAMERESAMPLING_DELTAX, 0x2) |
				IMXDPUV1_SET_FIELD(IMXDPUV1_FETCHECO0_FRAMERESAMPLING_DELTAY, 0x2)
				);

			/* todo: handle all cases for control register */
			imxdpuv1_write(imxdpu,
				b_off + IMXDPUV1_FETCHECO0_CONTROL_OFFSET,
				IMXDPUV1_SET_FIELD(IMXDPUV1_FETCHECO0_CONTROL_CLIPCOLOR, 1));

			/* luma resolution */
			imxdpuv1_write(imxdpu,
				b_off + IMXDPUV1_FETCHECO0_FRAMEDIMENSIONS_OFFSET,
				IMXDPUV1_SET_FIELD
				(IMXDPUV1_FETCHECO0_FRAMEDIMENSIONS_FRAMEHEIGHT,
					imxdpu->chan_data[chan_idx].dest_height -
					1 /*fheight-1 */) |
				IMXDPUV1_SET_FIELD
				(IMXDPUV1_FETCHECO0_FRAMEDIMENSIONS_FRAMEWIDTH,
					imxdpu->chan_data[chan_idx].dest_width -
					1 /*fwidth-1 */));

		} /* else need to handle Alpha, Warp, CLUT ... */

		imxdpu->chan_data[chan_idx].fetch_layer_prop.layerproperty0 =
			IMXDPUV1_SET_FIELD(IMXDPUV1_LAYERPROPERTY_SOURCEBUFFERENABLE,
			enable_buffer) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_LAYERPROPERTY_CLIPWINDOWENABLE,
			enable_clip);

		imxdpuv1_write_block(imxdpu,
			b_off + IMXDPUV1_FETCHECO0_BASEADDRESS0_OFFSET,
			(void *)&imxdpu->chan_data[chan_idx].
			fetch_layer_prop,
			sizeof(fetch_layer_setup_t) / 4);

		imxdpuv1_disp_request_shadow_load(imxdpuv1_id,
			imxdpu->chan_data[chan_idx].
			disp_id,
			IMXDPUV1_SHDLD_IDX_CHAN_00 +
			chan_idx);

	} else if (is_store_chan(chan)) {
		imxdpu->chan_data[chan_idx].store_layer_prop.baseaddress0 = phyaddr_0;
		imxdpu->chan_data[chan_idx].store_layer_prop.destbufferattributes0 =
			IMXDPUV1_SET_FIELD(
				IMXDPUV1_STORE9_DESTINATIONBUFFERATTRIBUTES_BITSPERPIXEL,
			imxdpuv1_bits_per_pixel(
				imxdpu->chan_data[chan_idx].dest_pixel_fmt)) |
			IMXDPUV1_SET_FIELD(
				IMXDPUV1_STORE9_DESTINATIONBUFFERATTRIBUTES_STRIDE,
			imxdpu->chan_data[chan_idx].stride-1);
		imxdpu->chan_data[chan_idx].store_layer_prop.destbufferdimension0 =
			IMXDPUV1_SET_FIELD(
				IMXDPUV1_STORE9_DESTINATIONBUFFERDIMENSION_LINECOUNT,
				imxdpu->chan_data[chan_idx].dest_height - 1) |
			IMXDPUV1_SET_FIELD(
				IMXDPUV1_STORE9_DESTINATIONBUFFERDIMENSION_LINEWIDTH,
			imxdpu->chan_data[chan_idx].dest_width - 1);
		imxdpu->chan_data[chan_idx].store_layer_prop.colorcomponentbits0 =
			imxdpuv1_get_colorcomponentbits(
			imxdpu->chan_data[chan_idx].dest_pixel_fmt);
		imxdpu->chan_data[chan_idx].store_layer_prop.colorcomponentshift0 =
			imxdpuv1_get_colorcomponentshift(
			imxdpu->chan_data[chan_idx].dest_pixel_fmt);
		imxdpu->chan_data[chan_idx].store_layer_prop.frameoffset0 =
			IMXDPUV1_SET_FIELD(IMXDPUV1_STORE9_FRAMEOFFSET_FRAMEXOFFSET,
			-imxdpu->chan_data[chan_idx].dest_left) |
			IMXDPUV1_SET_FIELD(IMXDPUV1_STORE9_FRAMEOFFSET_FRAMEYOFFSET,
			-imxdpu->chan_data[chan_idx].dest_top);


		imxdpuv1_write_block(imxdpu,
			b_off + IMXDPUV1_STORE9_BASEADDRESS_OFFSET,
			(void *)&imxdpu->chan_data[chan_idx].
			store_layer_prop,
			sizeof(store_layer_setup_t) / 4);

		if ((imxdpu->chan_data[chan_idx].dest_pixel_fmt == IMXDPUV1_PIX_FMT_YUYV) ||
		    (imxdpu->chan_data[chan_idx].dest_pixel_fmt == IMXDPUV1_PIX_FMT_YVYU) ||
		    (imxdpu->chan_data[chan_idx].dest_pixel_fmt == IMXDPUV1_PIX_FMT_UYVY)) {
			imxdpuv1_write(imxdpu,
				b_off + IMXDPUV1_STORE9_CONTROL_OFFSET,
				IMXDPUV1_SET_FIELD(IMXDPUV1_STORE9_CONTROL_RASTERMODE,
					IMXDPUV1_STORE9_CONTROL_RASTERMODE__YUV422));
		}

	}

	/* imxdpuv1_dump_channel(imxdpuv1_id, chan); */

	return ret;
}

/*!
 * Intializes a channel
 *
 * @param       imxdpuv1_id       id of the diplay unit
 * @param	chan    	channel to update
 * @param	phyaddr_0       physical address
 *
 * @return      This function returns 0 on success or negative error code on
 *      	fail.
 */
int32_t imxdpuv1_update_channel_buffer(
	int8_t imxdpuv1_id,
	imxdpuv1_chan_t chan,
	dma_addr_t phyaddr_0)
{
	int ret = 0;
	uint32_t b_off;     /* block offset for frame generator */
	struct imxdpuv1_soc *imxdpu;
	imxdpuv1_chan_idx_t chan_idx = get_channel_idx(chan);

	IMXDPUV1_TRACE_IRQ("%s()\n", __func__);

	if (!((imxdpuv1_id >= 0) && (imxdpuv1_id < IMXDPUV1_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpuv1_array[imxdpuv1_id];

	if (!is_chan(chan)) {
		return -EINVAL;
	}

	b_off = id2blockoffset(get_channel_blk(chan));
	if (b_off == IMXDPUV1_OFFSET_INVALID) {
		return -EINVAL;
	}

	if (imxdpu->chan_data[chan_idx].use_eco_fetch == IMXDPUV1_FALSE) {
		imxdpu->chan_data[chan_idx].phyaddr_0 = phyaddr_0;
		imxdpu->chan_data[chan_idx].fetch_layer_prop.baseaddress0 = phyaddr_0;
	}
#ifdef IMXDPUV1_VERSION_0
	if (is_store_chan(chan)) {
		IMXDPUV1_TRACE_IRQ("%s(): store channel\n", __func__);
		imxdpuv1_write_irq(imxdpu,
			b_off + IMXDPUV1_STORE4_BASEADDRESS_OFFSET,
			imxdpu->chan_data[chan_idx].fetch_layer_prop.baseaddress0);

		/* fixme: need to handle all pipline elements */
		imxdpuv1_write_irq(imxdpu, IMXDPUV1_PIXENGCFG_STORE4_REQUEST, 1);

		return ret;
	}
#endif
	if (is_fetch_decode_chan(chan)) {
		IMXDPUV1_TRACE_IRQ("%s(): fetch decode channel\n", __func__);
		if (imxdpu->chan_data[chan_idx].use_eco_fetch) {
			imxdpuv1_update_channel_buffer(imxdpuv1_id,
				imxdpuv1_get_eco(chan),
				phyaddr_0);
		}
		imxdpuv1_write_irq(imxdpu,
			b_off + IMXDPUV1_FETCHDECODE0_BASEADDRESS0_OFFSET,
			imxdpu->chan_data[chan_idx].fetch_layer_prop.baseaddress0);
		imxdpuv1_write_irq(imxdpu,
			b_off + IMXDPUV1_FETCHDECODE0_CONTROLTRIGGER_OFFSET,
			IMXDPUV1_FETCHDECODE0_CONTROLTRIGGER_SHDTOKGEN_MASK);
	} else if (is_fetch_layer_chan(chan)) {
		IMXDPUV1_TRACE_IRQ("%s(): fetch layer channel\n", __func__);
		imxdpuv1_write_irq(imxdpu,
			b_off + IMXDPUV1_FETCHLAYER0_BASEADDRESS0_OFFSET,
			imxdpu->chan_data[chan_idx].fetch_layer_prop.baseaddress0);
		imxdpuv1_write_irq(imxdpu,
			b_off + IMXDPUV1_FETCHLAYER0_TRIGGERENABLE_OFFSET,
			get_channel_sub(chan));
		imxdpuv1_write_irq(imxdpu,
			b_off + IMXDPUV1_FETCHLAYER0_CONTROLTRIGGER_OFFSET,
			IMXDPUV1_FETCHLAYER0_CONTROLTRIGGER_SHDTOKGEN_MASK);
	} else if (is_fetch_warp_chan(chan)) {
		IMXDPUV1_TRACE_IRQ("%s(): fetch warp channel\n", __func__);
		imxdpuv1_write_irq(imxdpu,
			b_off + IMXDPUV1_FETCHWARP2_BASEADDRESS0_OFFSET,
			imxdpu->chan_data[chan_idx].fetch_layer_prop.baseaddress0);
		imxdpuv1_write_irq(imxdpu,
			b_off + IMXDPUV1_FETCHWARP2_TRIGGERENABLE_OFFSET,
			get_channel_sub(chan));
		imxdpuv1_write_irq(imxdpu,
			b_off + IMXDPUV1_FETCHWARP2_CONTROLTRIGGER_OFFSET,
			IMXDPUV1_FETCHWARP2_CONTROLTRIGGER_SHDTOKGEN_MASK);
	} else if (is_fetch_eco_chan(chan))  {
		IMXDPUV1_TRACE_IRQ("%s(): fetch eco channel\n", __func__);

		imxdpu->chan_data[chan_idx].phyaddr_0 = phyaddr_0 + imxdpu->chan_data[chan_idx].u_offset;
		imxdpu->chan_data[chan_idx].fetch_layer_prop.baseaddress0 = imxdpu->chan_data[chan_idx].phyaddr_0;

		imxdpuv1_write_irq(imxdpu,
			b_off + IMXDPUV1_FETCHDECODE0_BASEADDRESS0_OFFSET,
			imxdpu->chan_data[chan_idx].fetch_layer_prop.baseaddress0);
		imxdpuv1_write_irq(imxdpu,
			b_off + IMXDPUV1_FETCHECO0_CONTROLTRIGGER_OFFSET,
			IMXDPUV1_FETCHECO0_CONTROLTRIGGER_SHDTOKGEN_MASK);
	}

	return ret;
}

/*!
 * Intializes a channel
 *
 * @param       imxdpuv1_id       id of the diplay unit
 * @param	params  	pointer to channel parameters
 *
 * @return      This function returns 0 on success or negative error code on
 *      	fail.
 */
int imxdpuv1_init_channel(int8_t imxdpuv1_id, imxdpuv1_channel_params_t *params)
{
	int ret = 0;
	struct imxdpuv1_soc *imxdpu;
	imxdpuv1_chan_t chan = params->common.chan;
	imxdpuv1_chan_idx_t chan_idx = get_channel_idx(chan);
	/* here we use the video mode for channel frame width, todo: we may need to
	   add a paramter for this */

	IMXDPUV1_TRACE("%s()\n", __func__);

	if (!((imxdpuv1_id >= 0) && (imxdpuv1_id < IMXDPUV1_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpuv1_array[imxdpuv1_id];

	if (!is_chan(chan)) {
		return -EINVAL;
	}
	imxdpu->chan_data[chan_idx].chan = chan;

	memset(&imxdpu->chan_data[chan_idx].fetch_layer_prop, 0,
		sizeof(fetch_layer_setup_t));
	imxdpu->chan_data[chan_idx].use_eco_fetch = IMXDPUV1_FALSE;

	if (is_fetch_decode_chan(chan)) {
		IMXDPUV1_TRACE("%s(): decode channel setup\n", __func__);
		imxdpu->chan_data[chan_idx].src_pixel_fmt =
			params->fetch_decode.src_pixel_fmt;
		imxdpu->chan_data[chan_idx].src_width =
			params->fetch_decode.src_width;
		imxdpu->chan_data[chan_idx].src_height =
			params->fetch_decode.src_height;
		imxdpu->chan_data[chan_idx].clip_top =
			params->fetch_decode.clip_top;
		imxdpu->chan_data[chan_idx].clip_left =
			params->fetch_decode.clip_left;
		imxdpu->chan_data[chan_idx].clip_width =
			params->fetch_decode.clip_width;
		imxdpu->chan_data[chan_idx].clip_height =
			params->fetch_decode.clip_height;
		imxdpu->chan_data[chan_idx].stride =
			params->fetch_decode.stride;
		imxdpu->chan_data[chan_idx].dest_pixel_fmt =
			params->fetch_decode.dest_pixel_fmt;
		imxdpu->chan_data[chan_idx].dest_top =
			params->fetch_decode.dest_top;
		imxdpu->chan_data[chan_idx].dest_left =
			params->fetch_decode.dest_left;
		imxdpu->chan_data[chan_idx].dest_width =
			params->fetch_decode.dest_width;
		imxdpu->chan_data[chan_idx].dest_height =
			params->fetch_decode.dest_height;
		imxdpu->chan_data[chan_idx].const_color =
			params->fetch_decode.const_color;
		imxdpu->chan_data[chan_idx].use_global_alpha =
			params->fetch_decode.use_global_alpha;
		imxdpu->chan_data[chan_idx].use_local_alpha =
			params->fetch_decode.use_local_alpha;
		imxdpu->chan_data[chan_idx].disp_id =
			params->fetch_decode.disp_id;

		if (imxdpu->chan_data[chan_idx].use_video_proc ==
				IMXDPUV1_TRUE) {
			imxdpu->chan_data[chan_idx].h_scale_factor =
				params->fetch_decode.h_scale_factor;
			imxdpu->chan_data[chan_idx].h_phase =
				params->fetch_decode.h_phase;
			imxdpu->chan_data[chan_idx].v_scale_factor =
				params->fetch_decode.v_scale_factor;
			imxdpu->chan_data[chan_idx].v_phase[0][0] =
				params->fetch_decode.v_phase[0][0];
			imxdpu->chan_data[chan_idx].v_phase[0][1] =
				params->fetch_decode.v_phase[0][1];
			imxdpu->chan_data[chan_idx].v_phase[1][0] =
				params->fetch_decode.v_phase[1][0];
			imxdpu->chan_data[chan_idx].v_phase[1][1] =
				params->fetch_decode.v_phase[1][1];
		}

		if (imxdpuv1_get_planes(imxdpu->chan_data[chan_idx].src_pixel_fmt) == 2) {
			if (has_fetch_eco_chan(chan)) {
				imxdpuv1_channel_params_t temp_params = *params;

				imxdpu->chan_data[chan_idx].use_eco_fetch = IMXDPUV1_TRUE;
				temp_params.fetch_decode.chan = imxdpuv1_get_eco(params->fetch_decode.chan);
				imxdpuv1_init_channel(imxdpuv1_id, &temp_params);
			} else {
				return -EINVAL;
			}
		}
	} else if (is_fetch_layer_chan(chan)) {
		IMXDPUV1_TRACE("%s(): layer channel setup\n", __func__);
		imxdpu->chan_data[chan_idx].src_pixel_fmt =
			params->fetch_layer.src_pixel_fmt;
		imxdpu->chan_data[chan_idx].src_width =
			params->fetch_layer.src_width;
		imxdpu->chan_data[chan_idx].src_height =
			params->fetch_layer.src_height;
		imxdpu->chan_data[chan_idx].clip_top =
			params->fetch_layer.clip_top;
		imxdpu->chan_data[chan_idx].clip_left =
			params->fetch_layer.clip_left;
		imxdpu->chan_data[chan_idx].clip_width =
			params->fetch_layer.clip_width;
		imxdpu->chan_data[chan_idx].clip_height =
			params->fetch_layer.clip_height;
		imxdpu->chan_data[chan_idx].stride =
			params->fetch_layer.stride;
		imxdpu->chan_data[chan_idx].dest_pixel_fmt =
			params->fetch_layer.dest_pixel_fmt;
		imxdpu->chan_data[chan_idx].dest_top =
			params->fetch_layer.dest_top;
		imxdpu->chan_data[chan_idx].dest_left =
			params->fetch_layer.dest_left;
		imxdpu->chan_data[chan_idx].dest_width =
			params->fetch_layer.dest_width;
		imxdpu->chan_data[chan_idx].dest_height =
			params->fetch_layer.dest_height;
		imxdpu->chan_data[chan_idx].const_color =
			params->fetch_layer.const_color;
		imxdpu->chan_data[chan_idx].use_global_alpha =
			params->fetch_layer.use_global_alpha;
		imxdpu->chan_data[chan_idx].use_local_alpha =
			params->fetch_layer.use_local_alpha;
		imxdpu->chan_data[chan_idx].disp_id =
			params->fetch_layer.disp_id;

	} else if (is_fetch_warp_chan(chan)) {
		IMXDPUV1_TRACE("%s(): warp channel setup\n", __func__);

		imxdpu->chan_data[chan_idx].src_pixel_fmt =
			params->fetch_warp.src_pixel_fmt;
		imxdpu->chan_data[chan_idx].src_width =
			params->fetch_warp.src_width;
		imxdpu->chan_data[chan_idx].src_height =
			params->fetch_warp.src_height;
		imxdpu->chan_data[chan_idx].clip_top =
			params->fetch_warp.clip_top;
		imxdpu->chan_data[chan_idx].clip_left =
			params->fetch_warp.clip_left;
		imxdpu->chan_data[chan_idx].clip_width =
			params->fetch_warp.clip_width;
		imxdpu->chan_data[chan_idx].clip_height =
			params->fetch_warp.clip_height;
		imxdpu->chan_data[chan_idx].stride =
			params->fetch_warp.stride;
		imxdpu->chan_data[chan_idx].dest_pixel_fmt =
			params->fetch_warp.dest_pixel_fmt;
		imxdpu->chan_data[chan_idx].dest_top =
			params->fetch_warp.dest_top;
		imxdpu->chan_data[chan_idx].dest_left =
			params->fetch_warp.dest_left;
		imxdpu->chan_data[chan_idx].dest_width =
			params->fetch_warp.dest_width;
		imxdpu->chan_data[chan_idx].dest_height =
			params->fetch_warp.dest_height;
		imxdpu->chan_data[chan_idx].const_color =
			params->fetch_warp.const_color;
		imxdpu->chan_data[chan_idx].use_global_alpha =
			params->fetch_warp.use_global_alpha;
		imxdpu->chan_data[chan_idx].use_local_alpha =
			params->fetch_warp.use_local_alpha;
		imxdpu->chan_data[chan_idx].disp_id =
			params->fetch_warp.disp_id;

	} else if (is_fetch_eco_chan(chan)) {

		IMXDPUV1_TRACE("%s(): fetch eco channel setup\n", __func__);
		imxdpu->chan_data[chan_idx].src_pixel_fmt =
			params->fetch_decode.src_pixel_fmt;
		imxdpu->chan_data[chan_idx].src_width =
			params->fetch_decode.src_width;
		imxdpu->chan_data[chan_idx].src_height =
			params->fetch_decode.src_height;
		imxdpu->chan_data[chan_idx].clip_top =
			params->fetch_decode.clip_top;
		imxdpu->chan_data[chan_idx].clip_left =
			params->fetch_decode.clip_left;
		imxdpu->chan_data[chan_idx].clip_width =
			params->fetch_decode.clip_width;
		imxdpu->chan_data[chan_idx].clip_height =
			params->fetch_decode.clip_height;
		imxdpu->chan_data[chan_idx].stride =
			params->fetch_decode.stride;
		imxdpu->chan_data[chan_idx].dest_pixel_fmt =
			params->fetch_decode.dest_pixel_fmt;
		imxdpu->chan_data[chan_idx].dest_top =
			params->fetch_decode.dest_top;
		imxdpu->chan_data[chan_idx].dest_left =
			params->fetch_decode.dest_left;
		imxdpu->chan_data[chan_idx].dest_width =
			params->fetch_decode.dest_width;
		imxdpu->chan_data[chan_idx].dest_height =
			params->fetch_decode.dest_height;
		imxdpu->chan_data[chan_idx].const_color =
			params->fetch_decode.const_color;
		imxdpu->chan_data[chan_idx].use_global_alpha =
			params->fetch_decode.use_global_alpha;
		imxdpu->chan_data[chan_idx].use_local_alpha =
			params->fetch_decode.use_local_alpha;
		imxdpu->chan_data[chan_idx].disp_id =
			params->fetch_decode.disp_id;

		if (imxdpu->chan_data[chan_idx].use_video_proc ==
				IMXDPUV1_TRUE) {
			imxdpu->chan_data[chan_idx].h_scale_factor =
				params->fetch_decode.h_scale_factor;
			imxdpu->chan_data[chan_idx].h_phase =
				params->fetch_decode.h_phase;
			imxdpu->chan_data[chan_idx].v_scale_factor =
				params->fetch_decode.v_scale_factor;
			imxdpu->chan_data[chan_idx].v_phase[0][0] =
				params->fetch_decode.v_phase[0][0];
			imxdpu->chan_data[chan_idx].v_phase[0][1] =
				params->fetch_decode.v_phase[0][1];
			imxdpu->chan_data[chan_idx].v_phase[1][0] =
				params->fetch_decode.v_phase[1][0];
			imxdpu->chan_data[chan_idx].v_phase[1][1] =
				params->fetch_decode.v_phase[1][1];
		}

	} else if (is_store_chan(chan)) {
		IMXDPUV1_TRACE("%s(): store setup\n", __func__);
		imxdpu->chan_data[chan_idx].src_pixel_fmt =
			params->store.src_pixel_fmt;
		imxdpu->chan_data[chan_idx].src_width =
			params->store.src_width;
		imxdpu->chan_data[chan_idx].src_height =
			params->store.src_height;
		imxdpu->chan_data[chan_idx].clip_top =
			params->store.clip_top;
		imxdpu->chan_data[chan_idx].clip_left =
			params->store.clip_left;
		imxdpu->chan_data[chan_idx].clip_width =
			params->store.clip_width;
		imxdpu->chan_data[chan_idx].clip_height =
			params->store.clip_height;
		imxdpu->chan_data[chan_idx].stride =
			params->store.stride;
		imxdpu->chan_data[chan_idx].dest_pixel_fmt =
			params->store.dest_pixel_fmt;
		imxdpu->chan_data[chan_idx].dest_top =
			params->store.dest_top;
		imxdpu->chan_data[chan_idx].dest_left =
			params->store.dest_left;
		imxdpu->chan_data[chan_idx].dest_width =
			params->store.dest_width;
		imxdpu->chan_data[chan_idx].dest_height =
			params->store.dest_height;
		imxdpu->chan_data[chan_idx].const_color =
			params->store.const_color;
		imxdpu->chan_data[chan_idx].source_id =
			params->store.capture_id;

		if (imxdpu->chan_data[chan_idx].use_video_proc ==
				IMXDPUV1_TRUE) {
			imxdpu->chan_data[chan_idx].h_scale_factor =
				params->store.h_scale_factor;
			imxdpu->chan_data[chan_idx].h_phase =
				params->store.h_phase;
			imxdpu->chan_data[chan_idx].v_scale_factor =
				params->store.v_scale_factor;
			imxdpu->chan_data[chan_idx].v_phase[0][0] =
				params->store.v_phase[0][0];
			imxdpu->chan_data[chan_idx].v_phase[0][1] =
				params->store.v_phase[0][1];
			imxdpu->chan_data[chan_idx].v_phase[1][0] =
				params->store.v_phase[1][0];
			imxdpu->chan_data[chan_idx].v_phase[1][1] =
				params->store.v_phase[1][1];
		}

	} else {
		IMXDPUV1_TRACE("%s(): ERROR, invalid channel type!\n", __func__);
		return -EINVAL;
	}

	/* imxdpuv1_dump_channel(imxdpuv1_id, chan); */

	return ret;
}

/*!
 * Dumps the fetch layer properties structure for a channel.
 *
 * @param       layer   	id of the diplay unit
 *
 * @return      This function returns 0 on success or negative error code on
 *      	fail.
 */
void imxdpuv1_dump_fetch_layer(fetch_layer_setup_t *layer)
{
	IMXDPUV1_PRINT("baseaddress             0x%08x\n"
		"sourcebufferattributes  0x%08x\n"
		"sourcebufferdimension   h %d  w %d\n"
		"colorcomponentbits      0x%08x\n"
		"colorcomponentshift     0x%08x\n"
		"layeroffset    	 y(top) %d  x(left) %d\n"
		"clipwindowoffset        y(top) %d  x(left) %d\n"
		"clipwindowdimensions    h %d  w %d\n"
		"constantcolor  	 0x%08x\n"
		"layerproperty  	 0x%08x\n",
		layer->baseaddress0,
		layer->sourcebufferattributes0,
		layer->sourcebufferdimension0 >> 16,
		layer->sourcebufferdimension0 & 0x3fff,
		layer->colorcomponentbits0, layer->colorcomponentshift0,
		layer->layeroffset0 >> 16, layer->layeroffset0 & 0x3fff,
		layer->clipwindowoffset0 >> 16,
		layer->clipwindowoffset0 & 0x3fff,
		layer->clipwindowdimensions0 >> 16,
		layer->clipwindowdimensions0 & 0x3fff,
		layer->constantcolor0, layer->layerproperty0);
	return;
}
/*!
 * Dumps the store layer properties structure for a channel.
 *
 * @param       layer   	id of the diplay unit
 *
 * @return      This function returns 0 on success or negative error code on
 *      	fail.
 */
void imxdpuv1_dump_store_layer(store_layer_setup_t  *layer)
{
	IMXDPUV1_TRACE(
		"baseaddress0             0x%08x\n"
		"destbufferattributes0    0x%08x\n"
		"destbufferdimension0     h %d  w %d\n"
		"frameoffset0             %d\n"
		"colorcomponentbits0      0x%08x\n"
		"colorcomponentshift0     0x%08x\n",
		layer->baseaddress0,
		layer->destbufferattributes0,
		layer->destbufferdimension0 >> 16, layer->destbufferdimension0 & 0x3fff,
		layer->frameoffset0,
		layer->colorcomponentbits0,
		layer->colorcomponentshift0);
	return;
}

/*!
 * Dumps the pixel engine configuration status
 *
 * @param       imxdpuv1_id       id of the diplay unit
 *
 * @return      This function returns 0 on success or negative error code on
 *      	fail.
 */
void imxdpuv1_dump_layerblend(int8_t imxdpuv1_id)
{
	uint32_t reg;
	struct imxdpuv1_soc *imxdpu;

	IMXDPUV1_TRACE("%s()\n", __func__);

	if (!((imxdpuv1_id >= 0) && (imxdpuv1_id < IMXDPUV1_MAX_NUM))) {
		return;
	}
	imxdpu = &imxdpuv1_array[imxdpuv1_id];

	reg = imxdpuv1_read(imxdpu, IMXDPUV1_PIXENGCFG_LAYERBLEND0_STATUS);
	IMXDPUV1_TRACE("LAYERBLEND0_STATUS:     0x%08x\n", reg);
	reg = imxdpuv1_read(imxdpu, IMXDPUV1_PIXENGCFG_LAYERBLEND0_LOCKSTATUS);
	IMXDPUV1_PRINT("LAYERBLEND0_LOCKSTATUS: 0x%08x\n", reg);

	reg = imxdpuv1_read(imxdpu, IMXDPUV1_PIXENGCFG_LAYERBLEND1_STATUS);
	IMXDPUV1_PRINT("LAYERBLEND1_STATUS:     0x%08x\n", reg);
	reg = imxdpuv1_read(imxdpu, IMXDPUV1_PIXENGCFG_LAYERBLEND1_LOCKSTATUS);
	IMXDPUV1_PRINT("LAYERBLEND1_LOCKSTATUS: 0x%08x\n", reg);

	reg = imxdpuv1_read(imxdpu, IMXDPUV1_PIXENGCFG_LAYERBLEND2_STATUS);
	IMXDPUV1_PRINT("LAYERBLEND2_STATUS:     0x%08x\n", reg);
	reg = imxdpuv1_read(imxdpu, IMXDPUV1_PIXENGCFG_LAYERBLEND2_LOCKSTATUS);
	IMXDPUV1_PRINT("LAYERBLEND2_LOCKSTATUS: 0x%08x\n", reg);

	reg = imxdpuv1_read(imxdpu, IMXDPUV1_PIXENGCFG_LAYERBLEND3_STATUS);
	IMXDPUV1_PRINT("LAYERBLEND3_STATUS:     0x%08x\n", reg);
	reg = imxdpuv1_read(imxdpu, IMXDPUV1_PIXENGCFG_LAYERBLEND3_LOCKSTATUS);
	IMXDPUV1_PRINT("LAYERBLEND3_LOCKSTATUS: 0x%08x\n", reg);
#ifdef IMXDPUV1_VERSION_0
	reg = imxdpuv1_read(imxdpu, IMXDPUV1_PIXENGCFG_LAYERBLEND4_STATUS);
	IMXDPUV1_PRINT("LAYERBLEND4_STATUS:     0x%08x\n", reg);
	reg = imxdpuv1_read(imxdpu, IMXDPUV1_PIXENGCFG_LAYERBLEND4_LOCKSTATUS);
	IMXDPUV1_PRINT("LAYERBLEND4_LOCKSTATUS: 0x%08x\n", reg);

	reg = imxdpuv1_read(imxdpu, IMXDPUV1_PIXENGCFG_LAYERBLEND5_STATUS);
	IMXDPUV1_PRINT("LAYERBLEND5_STATUS:     0x%08x\n", reg);
	reg = imxdpuv1_read(imxdpu, IMXDPUV1_PIXENGCFG_LAYERBLEND5_LOCKSTATUS);
	IMXDPUV1_PRINT("LAYERBLEND5_LOCKSTATUS: 0x%08x\n", reg);

	reg = imxdpuv1_read(imxdpu, IMXDPUV1_PIXENGCFG_LAYERBLEND6_STATUS);
	IMXDPUV1_PRINT("LAYERBLEND6_STATUS:     0x%08x\n", reg);
	reg = imxdpuv1_read(imxdpu, IMXDPUV1_PIXENGCFG_LAYERBLEND6_LOCKSTATUS);
	IMXDPUV1_PRINT("LAYERBLEND6_LOCKSTATUS: 0x%08x\n", reg);
#endif
	return;
}

/*!
 * Dumps the pixel engine configuration status
 *
 * @param       imxdpuv1_id       id of the diplay unit
 *
 * @return      This function returns 0 on success or negative error code on
 *      	fail.
 */
void imxdpuv1_dump_pixencfg_status(int8_t imxdpuv1_id)
{
	uint32_t reg;
	struct imxdpuv1_soc *imxdpu;

	IMXDPUV1_TRACE("%s()\n", __func__);

	if (!((imxdpuv1_id >= 0) && (imxdpuv1_id < IMXDPUV1_MAX_NUM))) {
		return;
	}
	imxdpu = &imxdpuv1_array[imxdpuv1_id];

	reg = imxdpuv1_read(imxdpu, IMXDPUV1_PIXENGCFG_EXTDST0_REQUEST);
	IMXDPUV1_PRINT("EXTDST0_REQUEST:     0x%08x\n", reg);
	reg = imxdpuv1_read(imxdpu, IMXDPUV1_PIXENGCFG_EXTDST1_REQUEST);
	IMXDPUV1_PRINT("EXTDST1_REQUEST:     0x%08x\n", reg);
	reg = imxdpuv1_read(imxdpu, IMXDPUV1_PIXENGCFG_EXTDST4_REQUEST);
	IMXDPUV1_PRINT("EXTDST4_REQUEST:     0x%08x\n", reg);
	reg = imxdpuv1_read(imxdpu, IMXDPUV1_PIXENGCFG_EXTDST5_REQUEST);
	IMXDPUV1_PRINT("EXTDST5_REQUEST:     0x%08x\n", reg);

	reg = imxdpuv1_read(imxdpu, IMXDPUV1_PIXENGCFG_EXTDST0_STATUS);
	IMXDPUV1_PRINT("EXTDST0_STATUS:     0x%08x\n", reg);
	reg = imxdpuv1_read(imxdpu, IMXDPUV1_PIXENGCFG_EXTDST1_STATUS);
	IMXDPUV1_PRINT("EXTDST1_STATUS:     0x%08x\n", reg);
	reg = imxdpuv1_read(imxdpu, IMXDPUV1_PIXENGCFG_EXTDST4_STATUS);
	IMXDPUV1_PRINT("EXTDST4_STATUS:     0x%08x\n", reg);
	reg = imxdpuv1_read(imxdpu, IMXDPUV1_PIXENGCFG_EXTDST5_STATUS);
	IMXDPUV1_PRINT("EXTDST5_STATUS:     0x%08x\n", reg);
#ifdef IMXDPUV1_VERSION_0
	reg = imxdpuv1_read(imxdpu, IMXDPUV1_PIXENGCFG_FETCHDECODE2_STATUS);
	IMXDPUV1_PRINT("FETCHDECODE2_STATUS:     0x%08x\n", reg);
	reg = imxdpuv1_read(imxdpu, IMXDPUV1_PIXENGCFG_FETCHDECODE3_STATUS);
	IMXDPUV1_PRINT("FETCHDECODE3_STATUS:     0x%08x\n", reg);
#endif
	reg = imxdpuv1_read(imxdpu, IMXDPUV1_PIXENGCFG_FETCHWARP2_STATUS);
	IMXDPUV1_PRINT("FETCHWARP2_STATUS:     0x%08x\n", reg);
	reg = imxdpuv1_read(imxdpu, IMXDPUV1_PIXENGCFG_FETCHECO2_STATUS);
	IMXDPUV1_PRINT("FETCHECO2_STATUS:     0x%08x\n", reg);

	reg = imxdpuv1_read(imxdpu, IMXDPUV1_PIXENGCFG_FETCHDECODE0_STATUS);
	IMXDPUV1_PRINT("FETCHDECODE0_STATUS:     0x%08x\n", reg);
	reg = imxdpuv1_read(imxdpu, IMXDPUV1_PIXENGCFG_FETCHECO0_STATUS);
	IMXDPUV1_PRINT("FETCHECO0_STATUS:     0x%08x\n", reg);
	reg = imxdpuv1_read(imxdpu, IMXDPUV1_PIXENGCFG_FETCHDECODE1_STATUS);
	IMXDPUV1_PRINT("FETCHDECODE1_STATUS:     0x%08x\n", reg);
	reg = imxdpuv1_read(imxdpu, IMXDPUV1_PIXENGCFG_FETCHECO1_STATUS);
	IMXDPUV1_PRINT("FETCHECO1_STATUS:     0x%08x\n", reg);

	reg = imxdpuv1_read(imxdpu, IMXDPUV1_PIXENGCFG_FETCHLAYER0_STATUS);
	IMXDPUV1_PRINT("FETCHLAYER0_STATUS:     0x%08x\n", reg);
#ifdef IMXDPUV1_VERSION_0
	reg = imxdpuv1_read(imxdpu, IMXDPUV1_PIXENGCFG_FETCHLAYER1_STATUS);
	IMXDPUV1_PRINT("FETCHLAYER1_STATUS:     0x%08x\n", reg);
#endif
	return;
}

/*!
 * Dumps the channel data
 *
 * @param       imxdpuv1_id       id of the diplay unit
 * @param       chan    	channel to dump
 *
 * @return      This function returns 0 on success or negative error code on
 *      	fail.
 */
int imxdpuv1_dump_channel(int8_t imxdpuv1_id, imxdpuv1_chan_t chan)
{
	int ret = 0;
	struct imxdpuv1_soc *imxdpu;
	imxdpuv1_chan_idx_t chan_idx = get_channel_idx(chan);

	IMXDPUV1_TRACE("%s()\n", __func__);

	if (!((imxdpuv1_id >= 0) && (imxdpuv1_id < IMXDPUV1_MAX_NUM))) {
		return -EINVAL;
	}

	imxdpu = &imxdpuv1_array[imxdpuv1_id];

	if (!is_chan(chan)) {
		return -EINVAL;
	}
	if (is_store_chan(chan)) {
		IMXDPUV1_PRINT("chan_id        0x%x\n"
			"src_pixel_fmt  0x%08x\n"
			"src_width      %d\n"
			"src_height     %d\n"
			"clip_top       %d(0x%04x)\n"
			"clip_left      %d(0x%04x)\n"
			"clip_width     %d\n"
			"clip_height    %d\n"
			"stride         %d\n"
			"dest_pixel_fmt 0x%08x\n"
			"dest_top       %d(0x%04x)\n"
			"dest_left      %d(0x%04x)\n"
			"dest_width     %d\n"
			"dest_height    %d\n",
			(uint32_t)imxdpu->chan_data[chan_idx].chan,
			imxdpu->chan_data[chan_idx].src_pixel_fmt,
			imxdpu->chan_data[chan_idx].src_width,
			imxdpu->chan_data[chan_idx].src_height,
			imxdpu->chan_data[chan_idx].clip_top,
			imxdpu->chan_data[chan_idx].clip_top,
			imxdpu->chan_data[chan_idx].clip_left,
			imxdpu->chan_data[chan_idx].clip_left,
			imxdpu->chan_data[chan_idx].clip_width,
			imxdpu->chan_data[chan_idx].clip_height,
			imxdpu->chan_data[chan_idx].stride,
			imxdpu->chan_data[chan_idx].dest_pixel_fmt,
			imxdpu->chan_data[chan_idx].dest_top,
			imxdpu->chan_data[chan_idx].dest_top,
			imxdpu->chan_data[chan_idx].dest_left,
			imxdpu->chan_data[chan_idx].dest_left,
			imxdpu->chan_data[chan_idx].dest_width,
			imxdpu->chan_data[chan_idx].dest_height);

		IMXDPUV1_PRINT(
			"use_video_proc %d\n"
			"use_eco_fetch  %d\n"
			"interlaced     %d\n"
			"phyaddr_0      0x%08x\n"
			"rot_mode       %d\n"
			"in_use         %d\n"
			"use_global_alpha %d\n"
			"use_local_alpha  %d\n",
			imxdpu->chan_data[chan_idx].use_video_proc,
			imxdpu->chan_data[chan_idx].use_eco_fetch,
			imxdpu->chan_data[chan_idx].interlaced,
			ptr_to_uint32(imxdpu->chan_data[chan_idx].phyaddr_0),
			imxdpu->chan_data[chan_idx].rot_mode,
			imxdpu->chan_data[chan_idx].in_use,
			imxdpu->chan_data[chan_idx].use_global_alpha,
			imxdpu->chan_data[chan_idx].use_local_alpha
			);

		imxdpuv1_dump_store_layer(&imxdpu->chan_data[chan_idx].store_layer_prop);

	} else {
		IMXDPUV1_PRINT("chan_id        0x%x\n"
			"src_pixel_fmt  0x%08x\n"
			"src_width      %d\n"
			"src_height     %d\n"
			"clip_top       %d(0x%04x)\n"
			"clip_left      %d(0x%04x)\n"
			"clip_width     %d\n"
			"clip_height    %d\n"
			"stride 	%d\n"
			"dest_pixel_fmt 0x%08x\n"
			"dest_top       %d(0x%04x)\n"
			"dest_left      %d(0x%04x)\n"
			"dest_width     %d\n"
			"dest_height    %d\n",
			(uint32_t)imxdpu->chan_data[chan_idx].chan,
			imxdpu->chan_data[chan_idx].src_pixel_fmt,
			imxdpu->chan_data[chan_idx].src_width,
			imxdpu->chan_data[chan_idx].src_height,
			imxdpu->chan_data[chan_idx].clip_top,
			imxdpu->chan_data[chan_idx].clip_top,
			imxdpu->chan_data[chan_idx].clip_left,
			imxdpu->chan_data[chan_idx].clip_left,
			imxdpu->chan_data[chan_idx].clip_width,
			imxdpu->chan_data[chan_idx].clip_height,
			imxdpu->chan_data[chan_idx].stride,
			imxdpu->chan_data[chan_idx].dest_pixel_fmt,
			imxdpu->chan_data[chan_idx].dest_top,
			imxdpu->chan_data[chan_idx].dest_top,
			imxdpu->chan_data[chan_idx].dest_left,
			imxdpu->chan_data[chan_idx].dest_left,
			imxdpu->chan_data[chan_idx].dest_width,
			imxdpu->chan_data[chan_idx].dest_height);


		IMXDPUV1_PRINT(
			"use_video_proc %d\n"
			"use_eco_fetch  %d\n"
			"interlaced     %d\n"
			"phyaddr_0      0x%08x\n"
			"u_offset       0x%08x\n"
			"v_offset       0x%08x\n"
			"rot_mode       %d\n"
			"in_use         %d\n"
			"use_global_alpha %d\n"
			"use_local_alpha  %d\n",
			imxdpu->chan_data[chan_idx].use_video_proc,
			imxdpu->chan_data[chan_idx].use_eco_fetch,
			imxdpu->chan_data[chan_idx].interlaced,
			ptr_to_uint32(imxdpu->chan_data[chan_idx].phyaddr_0),
			imxdpu->chan_data[chan_idx].u_offset,
			imxdpu->chan_data[chan_idx].v_offset,
			imxdpu->chan_data[chan_idx].rot_mode,
			imxdpu->chan_data[chan_idx].in_use,
			imxdpu->chan_data[chan_idx].use_global_alpha,
			imxdpu->chan_data[chan_idx].use_local_alpha
			);

		imxdpuv1_dump_fetch_layer(&imxdpu->chan_data[chan_idx].fetch_layer_prop);
	}
	return ret;
}

/*!
 * Shows the interrupt status registers
 *
 * @param   id of the diplay unit
 *
 */
void imxdpuv1_dump_int_stat(int8_t imxdpuv1_id)
{
	int i;
	struct imxdpuv1_soc *imxdpu;
	uint32_t reg;

	IMXDPUV1_TRACE("%s()\n", __func__);

	if (!((imxdpuv1_id >= 0) && (imxdpuv1_id < IMXDPUV1_MAX_NUM))) {
		return;
	}

	imxdpu = &imxdpuv1_array[imxdpuv1_id];

	for (i = 0; i < 3; i++) {
		reg = imxdpuv1_read_irq(imxdpu,
			IMXDPUV1_COMCTRL_USERINTERRUPTMASK0 +
			(i * 4));
		IMXDPUV1_PRINT("USERINTERRUPTMASK%d:   0x%08x\n", i, reg);
	}
	for (i = 0; i < 3; i++) {
		reg = imxdpuv1_read_irq(imxdpu,
			IMXDPUV1_COMCTRL_USERINTERRUPTENABLE0 +
			(i * 4));
		IMXDPUV1_PRINT("USERINTERRUPTENABLE%d: 0x%08x\n", i, reg);
	}
	for (i = 0; i < 3; i++) {
		reg = imxdpuv1_read_irq(imxdpu,
			IMXDPUV1_COMCTRL_USERINTERRUPTSTATUS0 +
			(i * 4));
		IMXDPUV1_PRINT("USERINTERRUPTSTATUS%d: 0x%08x\n", i, reg);
	}
	for (i = 0; i < 3; i++) {
		reg = imxdpuv1_read_irq(imxdpu,
			IMXDPUV1_COMCTRL_INTERRUPTENABLE0 + (i * 4));
		IMXDPUV1_PRINT("INTERRUPTENABLE%i:     0x%08x\n", i, reg);
	}
	for (i = 0; i < 3; i++) {
		reg = imxdpuv1_read_irq(imxdpu,
			IMXDPUV1_COMCTRL_INTERRUPTSTATUS0 + (i * 4));
		IMXDPUV1_PRINT("INTERRUPTSTATUS%i:     0x%08x\n", i, reg);
	}
}
