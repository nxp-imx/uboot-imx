/*
 * Copyright (c) 2012-2016, Freescale Semiconductor, Inc.
 * All rights reserved.
 * Copyright 2018 NXP
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __CAAM_INTERNAL_H__
#define __CAAM_INTERNAL_H__

/* 4kbyte pages */
#define CAAM_SEC_RAM_START_ADDR CAAM_ARB_BASE_ADDR

#define SEC_MEM_PAGE0       CAAM_SEC_RAM_START_ADDR
#define SEC_MEM_PAGE1       (CAAM_SEC_RAM_START_ADDR + 0x1000)
#define SEC_MEM_PAGE2       (CAAM_SEC_RAM_START_ADDR + 0x2000)
#define SEC_MEM_PAGE3       (CAAM_SEC_RAM_START_ADDR + 0x3000)

/* Configuration and special key registers */
#define CAAM_MCFGR          (CONFIG_SYS_FSL_SEC_ADDR + 0x0004)
#define CAAM_SCFGR          (CONFIG_SYS_FSL_SEC_ADDR + 0x000c)
#define CAAM_JR0MIDR        (CONFIG_SYS_FSL_SEC_ADDR + 0x0010)
#define CAAM_JR1MIDR        (CONFIG_SYS_FSL_SEC_ADDR + 0x0018)
#define CAAM_DECORR         (CONFIG_SYS_FSL_SEC_ADDR + 0x009c)
#define CAAM_DECO0MID       (CONFIG_SYS_FSL_SEC_ADDR + 0x00a0)
#define CAAM_DAR            (CONFIG_SYS_FSL_SEC_ADDR + 0x0120)
#define CAAM_DRR            (CONFIG_SYS_FSL_SEC_ADDR + 0x0124)
#define CAAM_JDKEKR         (CONFIG_SYS_FSL_SEC_ADDR + 0x0400)
#define CAAM_TDKEKR         (CONFIG_SYS_FSL_SEC_ADDR + 0x0420)
#define CAAM_TDSKR          (CONFIG_SYS_FSL_SEC_ADDR + 0x0440)
#define CAAM_SKNR           (CONFIG_SYS_FSL_SEC_ADDR + 0x04e0)
#define CAAM_SMSTA          (CONFIG_SYS_FSL_SEC_ADDR + 0x0FB4)
#define CAAM_STA            (CONFIG_SYS_FSL_SEC_ADDR + 0x0FD4)
#define CAAM_SMPO_0         (CONFIG_SYS_FSL_SEC_ADDR + 0x1FBC)
#define CAAM_CHAVID_LS      (CONFIG_SYS_FSL_SEC_ADDR + 0x0FEC)
#define CAAM_FAR            (CONFIG_SYS_FSL_SEC_ADDR + 0x0FC0)
#define CAAM_FAMR           (CONFIG_SYS_FSL_SEC_ADDR + 0x0FC8)
#define CAAM_FADR           (CONFIG_SYS_FSL_SEC_ADDR + 0x0FCC)

/* RNG registers */
#define CAAM_RTMCTL         (CONFIG_SYS_FSL_SEC_ADDR + 0x0600)
#define CAAM_RTSCMISC       (CONFIG_SYS_FSL_SEC_ADDR + 0x0604)
#define CAAM_RTPKRRNG       (CONFIG_SYS_FSL_SEC_ADDR + 0x0608)
#define CAAM_RTPKRMAX       (CONFIG_SYS_FSL_SEC_ADDR + 0x060C)
#define CAAM_RTSDCTL        (CONFIG_SYS_FSL_SEC_ADDR + 0x0610)
#define CAAM_RTFRQMIN       (CONFIG_SYS_FSL_SEC_ADDR + 0x0618)
#define CAAM_RTFRQMAX       (CONFIG_SYS_FSL_SEC_ADDR + 0x061C)
#define CAAM_RTSCML         (CONFIG_SYS_FSL_SEC_ADDR + 0x0620)
#define CAAM_RTSCR1L        (CONFIG_SYS_FSL_SEC_ADDR + 0x0624)
#define CAAM_RTSCR2L        (CONFIG_SYS_FSL_SEC_ADDR + 0x0628)
#define CAAM_RTSCR3L        (CONFIG_SYS_FSL_SEC_ADDR + 0x062C)
#define CAAM_RTSCR4L        (CONFIG_SYS_FSL_SEC_ADDR + 0x0630)
#define CAAM_RTSCR5L        (CONFIG_SYS_FSL_SEC_ADDR + 0x0634)
#define CAAM_RTSCR6PL       (CONFIG_SYS_FSL_SEC_ADDR + 0x0638)
#define CAAM_RTSTATUS       (CONFIG_SYS_FSL_SEC_ADDR + 0x063C)
#define CAAM_RDSTA          (CONFIG_SYS_FSL_SEC_ADDR + 0x06C0)

/* Job Ring 0 registers */
#define CAAM_IRBAR0         (CONFIG_SYS_FSL_SEC_ADDR + 0x1004)
#define CAAM_IRSR0          (CONFIG_SYS_FSL_SEC_ADDR + 0x100c)
#define CAAM_IRSAR0         (CONFIG_SYS_FSL_SEC_ADDR + 0x1014)
#define CAAM_IRJAR0         (CONFIG_SYS_FSL_SEC_ADDR + 0x101c)
#define CAAM_ORBAR0         (CONFIG_SYS_FSL_SEC_ADDR + 0x1024)
#define CAAM_ORSR0          (CONFIG_SYS_FSL_SEC_ADDR + 0x102c)
#define CAAM_ORJRR0         (CONFIG_SYS_FSL_SEC_ADDR + 0x1034)
#define CAAM_ORSFR0         (CONFIG_SYS_FSL_SEC_ADDR + 0x103c)
#define CAAM_JRSTAR0        (CONFIG_SYS_FSL_SEC_ADDR + 0x1044)
#define CAAM_JRINTR0        (CONFIG_SYS_FSL_SEC_ADDR + 0x104c)
#define CAAM_JRCFGR0_MS     (CONFIG_SYS_FSL_SEC_ADDR + 0x1050)
#define CAAM_JRCFGR0_LS     (CONFIG_SYS_FSL_SEC_ADDR + 0x1054)
#define CAAM_IRRIR0         (CONFIG_SYS_FSL_SEC_ADDR + 0x105c)
#define CAAM_ORWIR0         (CONFIG_SYS_FSL_SEC_ADDR + 0x1064)
#define CAAM_JRCR0          (CONFIG_SYS_FSL_SEC_ADDR + 0x106c)
#define CAAM_SMCJR0         (CONFIG_SYS_FSL_SEC_ADDR + 0x10f4)
#define CAAM_SMCSJR0        (CONFIG_SYS_FSL_SEC_ADDR + 0x10fc)
#define CAAM_SMAPJR0(y)     (CONFIG_SYS_FSL_SEC_ADDR + 0x1104 + y*16)
#define CAAM_SMAG2JR0(y)    (CONFIG_SYS_FSL_SEC_ADDR + 0x1108 + y*16)
#define CAAM_SMAG1JR0(y)    (CONFIG_SYS_FSL_SEC_ADDR + 0x110C + y*16)
#define CAAM_SMAPJR0_PRTN1  (CONFIG_SYS_FSL_SEC_ADDR + 0x1114)
#define CAAM_SMAG2JR0_PRTN1 (CONFIG_SYS_FSL_SEC_ADDR + 0x1118)
#define CAAM_SMAG1JR0_PRTN1 (CONFIG_SYS_FSL_SEC_ADDR + 0x111c)
#define CAAM_SMPO           (CONFIG_SYS_FSL_SEC_ADDR + 0x1fbc)

/* JR0 and JR1 will be assigned to seco in imx8, so we need to
 * use JR3 instead.
 */
#ifdef CONFIG_ARCH_IMX8
#define CAAM_IRBAR3         (CONFIG_SYS_FSL_SEC_ADDR + 0x40004)
#define CAAM_IRSR3          (CONFIG_SYS_FSL_SEC_ADDR + 0x4000c)
#define CAAM_IRSAR3         (CONFIG_SYS_FSL_SEC_ADDR + 0x40014)
#define CAAM_IRJAR3         (CONFIG_SYS_FSL_SEC_ADDR + 0x4001c)
#define CAAM_ORBAR3         (CONFIG_SYS_FSL_SEC_ADDR + 0x40024)
#define CAAM_ORSR3          (CONFIG_SYS_FSL_SEC_ADDR + 0x4002c)
#define CAAM_ORSFR3         (CONFIG_SYS_FSL_SEC_ADDR + 0x4003c)
#define CAAM_JRINTR3        (CONFIG_SYS_FSL_SEC_ADDR + 0x4004c)
#define CAAM_ORJRR3         (CONFIG_SYS_FSL_SEC_ADDR + 0x40034)
#define CAAM_JRCFGR3_LS     (CONFIG_SYS_FSL_SEC_ADDR + 0x40054)
#define CAAM_JRCR3          (CONFIG_SYS_FSL_SEC_ADDR + 0x4006c)
#endif

#define DESC_MAX_SIZE       (0x40)        /* Descriptor max size */
#define JRCFG_LS_IMSK       (0x01)        /* Interrupt Mask */
#define JR_MID              (0x02)        /* Matches ROM configuration */
#define KS_G1               BIT(JR_MID)   /* CAAM only */
#define PERM                (0x0000B008)  /* Clear on release, lock SMAP,
					   * lock SMAG and group 1 Blob
					   */

#define CMD_PAGE_ALLOC      (0x1)
#define CMD_PAGE_DEALLOC    (0x2)
#define CMD_PART_DEALLOC    (0x3)
#define CMD_INQUIRY         (0x5)
#define PAGE(x)             (x << 16)
#define PARTITION(x)        (x << 8)

#define SMCSJR_AERR         (3 << 12)
#define SMCSJR_CERR         (3 << 14)
#define CMD_COMPLETE        (3 << 14)

#define SMCSJR_PO           (3 << 6)
#define PAGE_AVAILABLE      (0)
#define PAGE_OWNED          (3 << 6)

#define PARTITION_OWNER(x)  (0x3 << (x*2))

#define CAAM_BUSY_MASK      (0x00000001) /* BUSY from status reg */
#define CAAM_IDLE_MASK      (0x00000002) /* IDLE from status reg */
#define CAAM_MCFGR_SWRST    BIT(31)      /* CAAM SW reset */
#define CAAM_MCFGR_DMARST   BIT(28)      /* CAAM DMA reset */

#define JOB_RING_ENTRIES    (1)
#define JOB_RING_STS        (0xF << 28)

/** OSC_DIV in RNG trim fuses */
#define RNG_TRIM_OSC_DIV    (0)
/** ENT_DLY multiplier in RNG trim fuses */
#define TRNG_SDCTL_ENT_DLY_MIN (3200)
#define TRNG_SDCTL_ENT_DLY_MAX (4800)

#define RTMCTL_PGM       BIT(16)
#define RTMCTL_ERR       BIT(12)
#define RTMCTL_RST       BIT(6)
#define RTMCTL_ACC       BIT(5)
#define RDSTA_IF0        (1)
#define RDSTA_IF1        (2)
#define RDSTA_SKVN       BIT(30)
#define JRCR_RESET       (1)
#define RTMCTL_FCT_FAIL  BIT(8)

#define BS_TRNG_ENT_DLY     (16)
#define BM_TRNG_ENT_DLY     (0xffff << BS_TRNG_ENT_DLY)
#define BM_TRNG_SAMP_MODE   (3)
#define TRNG_SAMP_MODE_RAW_ES_SC (1)
#define BS_JRINTR_HALT      (2)
#define BM_JRINTR_HALT      (0x3 << BS_JRINTR_HALT)
#define JRINTR_HALT_ONGOING (0x1 << BS_JRINTR_HALT)
#define JRINTR_HALT_DONE    (0x2 << BS_JRINTR_HALT)
#define JRINTR_JRI          (0x1)
#define BS_JRCFGR_LS_ICTT   (16)
#define BM_JRCFGR_LS_ICTT   (0xFFFF << BS_JRCFGR_LS_ICTT)
#define BS_JRCFGR_LS_ICDCT  (8)
#define BM_JRCFGR_LS_ICDCT  (0xFF << BS_JRCFGR_LS_ICDCT)
#define BS_JRCFGR_LS_ICEN   (1)
#define BM_JRCFGR_LS_ICEN   (0x1 << BS_JRCFGR_LS_ICEN)
#define BS_JRCFGR_LS_IMSK   (0)
#define BM_JRCFGR_LS_IMSK   (0x1 << BS_JRCFGR_LS_IMSK)
#define BS_CHAVID_LS_RNGVID (16)
#define BM_CHAVID_LS_RNGVID (0xF << BS_CHAVID_LS_RNGVID)
#define BS_MCFGR_WDE        (30)
#define BM_MCFGR_WDE        (0x1 << BS_MCFGR_WDE)

typedef enum {
    PAGE_0,
    PAGE_1,
    PAGE_2,
    PAGE_3,
} page_num_e;

typedef enum {
    PARTITION_0,
    PARTITION_1,
    PARTITION_2,
    PARTITION_3,
    PARTITION_4,
    PARTITION_5,
    PARTITION_6,
    PARTITION_7,
} partition_num_e;


/*
 * Local defines
 */
/* arm v7 need 64 align */
#define ALIGN_MASK     ~(ARCH_DMA_MINALIGN - 1)
/* caam dma and pointer conversion for arm and arm64 architectures */
#ifdef CONFIG_IMX_CONFIG
  #define PTR2CAAMDMA(x)  (u32)((uintptr_t)(x) & 0xffffffff)
  #define CAAMDMA2PTR(x)  (uintptr_t)((x) & 0xffffffff)
#else
  #define PTR2CAAMDMA(x)  (uintptr_t)(x)
  #define CAAMDMA2PTR(x)  (uintptr_t)(x)
#endif
#define RING_EARLY_INIT   (0x01)
#define RING_RELOC_INIT   (0x02)

#define CAAM_HDR_CTYPE            (0x16u << 27)
#define CAAM_HDR_ONE              BIT(23)
#define CAAM_HDR_START_INDEX(x)   (((x) & 0x3F) << 16)
#define CAAM_HDR_DESCLEN(x)       ((x) & 0x3F)
#define CAAM_PROTOP_CTYPE         (0x10u << 27)

/* State Handle */
#define BS_ALGO_RNG_SH            (4)
#define BM_ALGO_RNG_SH            (0x3 << BS_ALGO_RNG_SH)
#define ALGO_RNG_SH(id)           (((id) << BS_ALGO_RNG_SH) & BM_ALGO_RNG_SH)

/* Secure Key */
#define BS_ALGO_RNG_SK            (12)
#define BM_ALGO_RNG_SK            BIT(BS_ALGO_RNG_SK)

/* State */
#define BS_ALGO_RNG_AS            (2)
#define BM_ALGO_RNG_AS            (0x3 << BS_ALGO_RNG_AS)
#define ALGO_RNG_GENERATE         (0x0 << BS_ALGO_RNG_AS)
#define ALGO_RNG_INSTANTIATE      BIT(BS_ALGO_RNG_AS)

/* Prediction Resistance */
#define ALGO_RNG_PR		BIT(1)

#define CAAM_C1_RNG               ((0x50 << 16) | (2 << 24))

#define BS_JUMP_LOCAL_OFFSET      (0)
#define BM_JUMP_LOCAL_OFFSET      (0xFF << BS_JUMP_LOCAL_OFFSET)

#define CAAM_C1_JUMP              ((0x14u << 27) | (1 << 25))
#define CAAM_JUMP_LOCAL           (0 << 20)
#define CAAM_JUMP_TST_ALL_COND_TRUE (0 << 16)
#define CAAM_JUMP_OFFSET(off)     (((off) << BS_JUMP_LOCAL_OFFSET) \
				& BM_JUMP_LOCAL_OFFSET)

#define CAAM_C0_LOAD_IMM          ((0x2 << 27) | (1 << 23))
#define CAAM_DST_CLEAR_WRITTEN    (0x8 << 16)

#define RNG_DESC_SH0_SIZE   (ARRAY_SIZE(rng_inst_sh0_desc))
#define RNG_DESC_SH1_SIZE   (ARRAY_SIZE(rng_inst_sh1_desc))
#define RNG_DESC_KEYS_SIZE  (ARRAY_SIZE(rng_inst_load_keys))
#define RNG_DESC_MAX_SIZE   (RNG_DESC_SH0_SIZE + \
			RNG_DESC_SH1_SIZE + \
			RNG_DESC_KEYS_SIZE)

#define CAAM_PAD_LEN 48

#endif /* __CAAM_INTERNAL_H__ */
