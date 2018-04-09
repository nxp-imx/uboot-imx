/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2018 NXP
 */

#ifndef __DMA_MACROS_H__
#define __DMA_MACROS_H__

/* eDMA controller */
#define DMA_CHANNEL_1			1
#define DMA_TCD_BASE_ADDRESS	(EDMA_BASE_ADDR + 0x1000)
#define DMA_CHANNEL(channel)	(DMA_TCD_BASE_ADDRESS + 0x20 * (channel))
#define DMA_CR				(EDMA_BASE_ADDR)
#define DMA_ES				(EDMA_BASE_ADDR + 0x4)
#define DMA_ERR				(EDMA_BASE_ADDR + 0x2C)
#define DMA_TCD_N_SADDR(channel)	(DMA_CHANNEL(channel))
#define DMA_TCD_N_SOFF(channel)		(DMA_CHANNEL(channel) + 0x4)
#define DMA_TCD_N_NBYTES_MLNO(channel)	(DMA_CHANNEL(channel) + 0x8)
#define DMA_TCD_N_DADDR(channel)	(DMA_CHANNEL(channel) + 0x10)
#define DMA_TCD_N_DOFF(channel)		(DMA_CHANNEL(channel) + 0x14)
#define DMA_TCD_N_CITER_ELINKNO(channel)(DMA_CHANNEL(channel) + 0x16)
#define DMA_TCD_N_CSR(channel)		(DMA_CHANNEL(channel) + 0x1C)
#define DMA_TCD_N_BITER_ELINKNO(channel)(DMA_CHANNEL(channel) + 0x1E)

#ifdef __INCLUDE_ASSEMBLY_MACROS__
.macro check_done_bit
	ldr x9, =DMA_TCD_N_CSR(DMA_CHANNEL_1)
	ldr w10, [x9]
	/* Check transfer done */
	and w10, w10, #0x0080
	sub w10, w10, #0x0080
.endm

.macro clear_done_bit
	ldr x9, =DMA_TCD_N_CSR(DMA_CHANNEL_1)
	ldr w10, =0x0
	strb w10, [x9]
.endm

.macro clear_channel_err
	/* DMA_ERR */
	ldr x9, =DMA_ERR
	/* Clear error bit for channel */
	ldr x10, =0x00000002
	str w10, [x9]
.endm
#endif
#endif /* __DMA_MACROS_H__ */
