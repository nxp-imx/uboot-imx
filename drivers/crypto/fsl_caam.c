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

#include <common.h>
#include <malloc.h>
#include <memalign.h>
#include <asm/io.h>
#ifndef CONFIG_ARCH_MX7ULP
#include <asm/arch/crm_regs.h>
#else
#include <asm/arch/pcc.h>
#endif /* CONFIG_ARCH_MX7ULP */
#include "fsl_caam_internal.h"
#include "fsl/desc_constr.h"
#include <fsl_caam.h>
#include <cpu_func.h>

DECLARE_GLOBAL_DATA_PTR;

static int do_cfg_jrqueue(void);
static int do_job(u32 *desc);
#ifndef CONFIG_ARCH_IMX8
static void rng_init(void);
static void caam_clock_enable(void);
static int jr_reset(void);
#endif
#ifdef CONFIG_CAAM_KB_SELF_TEST
static void caam_test(void);
#endif

/*
 * Structures
 */
/* Definition of input ring object */
struct inring_entry {
	u32 desc; /* Pointer to input descriptor */
};

/* Definition of output ring object */
struct outring_entry {
	u32 desc;   /* Pointer to output descriptor */
	u32 status; /* Status of the Job Ring       */
};

/* Main job ring data structure */
struct jr_data_st {
	struct inring_entry  *inrings;
	struct outring_entry *outrings;
	u32 status;  /* Ring buffers init status */
	u32 *desc;   /* Pointer to output descriptor */
	u32 raw_addr[DESC_MAX_SIZE * 2];
};

/*
 * Global variables
 */
#if defined(CONFIG_SPL_BUILD)
static struct jr_data_st g_jrdata = {0};
#else
static struct jr_data_st g_jrdata = {0, 0, 0xFFFFFFFF};
#endif

static u8 skeymod[] = {
	0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08,
	0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00
};

/*
 * Local functions
 */
static void dump_error(void)
{
	int i;

	debug("Dump CAAM Error\n");
	debug("MCFGR 0x%08X\n", __raw_readl(CAAM_MCFGR));
	debug("FAR  0x%08X\n", __raw_readl(CAAM_FAR));
	debug("FAMR 0x%08X\n", __raw_readl(CAAM_FAMR));
	debug("FADR 0x%08X\n", __raw_readl(CAAM_FADR));
	debug("CSTA 0x%08X\n", __raw_readl(CAAM_STA));
	debug("RTMCTL 0x%X\n", __raw_readl(CAAM_RTMCTL));
	debug("RTSTATUS 0x%X\n", __raw_readl(CAAM_RTSTATUS));
	debug("RDSTA 0x%X\n", __raw_readl(CAAM_RDSTA));

	for (i = 0; i < desc_len(g_jrdata.desc); i++)
		debug("desc[%d]: 0x%08x\n", i, g_jrdata.desc[i]);
}

/*!
 * Secure memory run command.
 *
 * @param   sec_mem_cmd  Secure memory command register
 * @return  cmd_status  Secure memory command status register
 */
u32 secmem_set_cmd_1(u32 sec_mem_cmd)
{
	u32 temp_reg;
	__raw_writel(sec_mem_cmd, CAAM_SMCJR0);
	do {
		temp_reg = __raw_readl(CAAM_SMCSJR0);
	} while (temp_reg & CMD_COMPLETE);

	return temp_reg;
}


/*!
 * Use CAAM to decapsulate a blob to secure memory.
 * Such blob of secret key cannot be read once decrypted,
 * but can still be used for enc/dec operation of user's data.
 *
 * @param   blob_addr  Location address of the blob.
 *
 * @return  SUCCESS or ERROR_XXX
 */
u32 caam_decap_blob(u32 plain_text, u32 blob_addr, u32 size)
{
	u32 ret = SUCCESS;
	u32 key_sz = sizeof(skeymod);
	u32 *decap_desc = g_jrdata.desc;

	/* prepare job descriptor */
	init_job_desc(decap_desc, 0);
	append_load(decap_desc, PTR2CAAMDMA(skeymod), key_sz,
		    LDST_CLASS_2_CCB | LDST_SRCDST_BYTE_KEY);
	append_seq_in_ptr_intlen(decap_desc, blob_addr, size + CAAM_PAD_LEN, 0);
	append_seq_out_ptr_intlen(decap_desc, plain_text, size, 0);
	append_operation(decap_desc, OP_TYPE_DECAP_PROTOCOL | OP_PCLID_BLOB);

	flush_dcache_range((uintptr_t)blob_addr & ALIGN_MASK,
			   ((uintptr_t)blob_addr & ALIGN_MASK)
			   + ROUND(size + CAAM_PAD_LEN, ARCH_DMA_MINALIGN));
	flush_dcache_range((uintptr_t)plain_text & ALIGN_MASK,
			   (plain_text & ALIGN_MASK)
			   + ROUND(size, ARCH_DMA_MINALIGN));

	/* Run descriptor with result written to blob buffer */
	ret = do_job(decap_desc);

	if (ret != SUCCESS) {
		printf("Error: blob decap job failed 0x%x\n", ret);
	}

	return ret;
}

/*!
 * Use CAAM to generate a blob.
 *
 * @param   plain_data_addr  Location address of the plain data.
 * @param   blob_addr  Location address of the blob.
 *
 * @return  SUCCESS or ERROR_XXX
 */
u32 caam_gen_blob(u32 plain_data_addr, u32 blob_addr, u32 size)
{
	u32 ret = SUCCESS;
	u32 key_sz = sizeof(skeymod);
	u32 *encap_desc = g_jrdata.desc;
	/* Buffer to hold the resulting blob */
	u8 *blob = (u8 *)CAAMDMA2PTR(blob_addr);

	/* initialize the blob array */
	memset(blob,0,size);

	/* prepare job descriptor */
	init_job_desc(encap_desc, 0);
	append_load(encap_desc, PTR2CAAMDMA(skeymod), key_sz,
		    LDST_CLASS_2_CCB | LDST_SRCDST_BYTE_KEY);
	append_seq_in_ptr_intlen(encap_desc, plain_data_addr, size, 0);
	append_seq_out_ptr_intlen(encap_desc, PTR2CAAMDMA(blob), size + CAAM_PAD_LEN, 0);
	append_operation(encap_desc, OP_TYPE_ENCAP_PROTOCOL | OP_PCLID_BLOB);

	flush_dcache_range((uintptr_t)plain_data_addr & ALIGN_MASK,
			   (plain_data_addr & ALIGN_MASK)
			   + ROUND(size, ARCH_DMA_MINALIGN));
	flush_dcache_range((uintptr_t)blob & ALIGN_MASK,
			   ((uintptr_t)blob & ALIGN_MASK)
			   + ROUND(size + CAAM_PAD_LEN, ARCH_DMA_MINALIGN));

	ret = do_job(encap_desc);

	if (ret != SUCCESS) {
		printf("Error: blob encap job failed 0x%x\n", ret);
	}

	return ret;
}

u32 caam_hwrng(u8 *output_ptr, u32 output_len)
{
	u32 ret = SUCCESS;
	u32 *hwrng_desc = g_jrdata.desc;
	/* Buffer to hold the resulting output*/
	u8 *output = (u8 *)output_ptr;

	/* initialize the output array */
	memset(output,0,output_len);

	/* prepare job descriptor */
	init_job_desc(hwrng_desc, 0);
	append_operation(hwrng_desc, OP_ALG_ALGSEL_RNG | OP_TYPE_CLASS1_ALG);
	append_fifo_store(hwrng_desc, PTR2CAAMDMA(output),
			  output_len, FIFOST_TYPE_RNGSTORE);

	/* flush cache */
	flush_dcache_range((uintptr_t)hwrng_desc & ALIGN_MASK,
			   ((uintptr_t)hwrng_desc & ALIGN_MASK)
			   + ROUND(DESC_MAX_SIZE, ARCH_DMA_MINALIGN));

	flush_dcache_range((uintptr_t)output & ALIGN_MASK,
			   ((uintptr_t)output & ALIGN_MASK)
			   + ROUND(2 * output_len, ARCH_DMA_MINALIGN));

	ret = do_job(hwrng_desc);

	if (ret != SUCCESS) {
		printf("Error: RNG generate failed 0x%x\n", ret);
	}

	return ret;
}

/*!
 * Initialize the CAAM.
 *
 */
void caam_open(void)
{
	int ret;

	/* switch on the clock */
	/* for imx8, the CAAM initialization should have been done
	 * in seco, so we should skip this part.
	 */
#ifndef CONFIG_ARCH_IMX8
	u32 temp_reg;
	u32 init_mask;

	caam_clock_enable();

	/* reset the CAAM */
	temp_reg = __raw_readl(CAAM_MCFGR) |
			CAAM_MCFGR_DMARST | CAAM_MCFGR_SWRST;
	__raw_writel(temp_reg,  CAAM_MCFGR);
	while (__raw_readl(CAAM_MCFGR) & CAAM_MCFGR_DMARST)
		;

	jr_reset();

	ret = do_cfg_jrqueue();

	if (ret != SUCCESS) {
		printf("Error CAAM JR initialization\n");
		return;
	}

	/* Check if the RNG is already instantiated */
	temp_reg = __raw_readl(CAAM_RDSTA);
	init_mask = RDSTA_IF0 | RDSTA_IF1 | RDSTA_SKVN;
	if ((temp_reg & init_mask) == init_mask) {
		printf("RNG already instantiated 0x%X\n", temp_reg);
		return;
	}
	rng_init();
#else
	ret = do_cfg_jrqueue();

	if (ret != SUCCESS) {
		printf("Error CAAM JR initialization\n");
		return;
	}
#endif

#ifdef CONFIG_CAAM_KB_SELF_TEST
	caam_test();
#endif
}

/*
 *  Descriptors to instantiate SH0, SH1, load the keys
 */
#ifndef CONFIG_ARCH_IMX8
static const u32 rng_inst_sh0_desc[] = {
	/* Header, don't setup the size */
	CAAM_HDR_CTYPE | CAAM_HDR_ONE | CAAM_HDR_START_INDEX(0),
	/* Operation instantiation (sh0) */
	CAAM_PROTOP_CTYPE | CAAM_C1_RNG | ALGO_RNG_SH(0) | ALGO_RNG_PR |
		ALGO_RNG_INSTANTIATE,
};

static const u32 rng_inst_sh1_desc[] = {
	/* wait for done - Jump to next entry */
	CAAM_C1_JUMP | CAAM_JUMP_LOCAL | CAAM_JUMP_TST_ALL_COND_TRUE
		| CAAM_JUMP_OFFSET(1),
	/* Clear written register (write 1) */
	CAAM_C0_LOAD_IMM | CAAM_DST_CLEAR_WRITTEN | sizeof(u32),
	0x00000001,
	/* Operation instantiation (sh1) */
	CAAM_PROTOP_CTYPE | CAAM_C1_RNG | ALGO_RNG_SH(1) | ALGO_RNG_PR
		| ALGO_RNG_INSTANTIATE,
};

static const u32 rng_inst_load_keys[] = {
	/* wait for done - Jump to next entry */
	CAAM_C1_JUMP | CAAM_JUMP_LOCAL | CAAM_JUMP_TST_ALL_COND_TRUE
		| CAAM_JUMP_OFFSET(1),
	/* Clear written register (write 1) */
	CAAM_C0_LOAD_IMM | CAAM_DST_CLEAR_WRITTEN | sizeof(u32),
	0x00000001,
	/* Generate the Key */
	CAAM_PROTOP_CTYPE | CAAM_C1_RNG | BM_ALGO_RNG_SK | ALGO_RNG_GENERATE,
};
#endif

static int do_job(u32 *desc)
{
	int ret;
	phys_addr_t p_desc = virt_to_phys(desc);

	/* for imx8, JR0 and JR1 will be assigned to seco, so we use
	 * the JR3 instead.
	 */
#ifndef CONFIG_ARCH_IMX8
	if (__raw_readl(CAAM_IRSAR0) == 0)
#else
	if (__raw_readl(CAAM_IRSAR3) == 0)
#endif
		return ERROR_ANY;
	g_jrdata.inrings[0].desc = p_desc;

	flush_dcache_range((uintptr_t)g_jrdata.inrings & ALIGN_MASK,
			   ((uintptr_t)g_jrdata.inrings & ALIGN_MASK)
			   + ROUND(DESC_MAX_SIZE, ARCH_DMA_MINALIGN));
	flush_dcache_range((uintptr_t)desc & ALIGN_MASK,
			   ((uintptr_t)desc & ALIGN_MASK)
			   + ROUND(DESC_MAX_SIZE, ARCH_DMA_MINALIGN));

	flush_dcache_range((uintptr_t)g_jrdata.outrings & ALIGN_MASK,
			  ((uintptr_t)g_jrdata.outrings & ALIGN_MASK)
			  + ROUND(DESC_MAX_SIZE, ARCH_DMA_MINALIGN));

	/* Inform HW that a new JR is available */
#ifndef CONFIG_ARCH_IMX8
	__raw_writel(1, CAAM_IRJAR0);
	while (__raw_readl(CAAM_ORSFR0) == 0)
		;
#else
	__raw_writel(1, CAAM_IRJAR3);
	while (__raw_readl(CAAM_ORSFR3) == 0)
		;
#endif

	if (PTR2CAAMDMA(desc) == g_jrdata.outrings[0].desc) {
		ret = g_jrdata.outrings[0].status;
	} else {
		dump_error();
		ret = ERROR_ANY;
	}

	/* Acknowledge interrupt */
#ifndef CONFIG_ARCH_IMX8
	setbits_le32(CAAM_JRINTR0, JRINTR_JRI);
	/* Remove the JR from the output list even if no JR caller found */
	__raw_writel(1, CAAM_ORJRR0);
#else
	setbits_le32(CAAM_JRINTR3, JRINTR_JRI);
	/* Remove the JR from the output list even if no JR caller found */
	__raw_writel(1, CAAM_ORJRR3);
#endif

	return ret;
}

static int do_cfg_jrqueue(void)
{
	u32 value = 0;
	phys_addr_t ip_base;
	phys_addr_t op_base;

	/* check if already configured after relocation */
	if (g_jrdata.status == RING_RELOC_INIT)
		return 0;

	/*
	 * jr configuration needs to be updated once, after relocation to ensure
	 * using the right buffers.
	 * When buffers are updated after relocation the flag RING_RELOC_INIT
	 * is used to prevent extra updates
	 */
	if (gd->flags & GD_FLG_RELOC) {
		g_jrdata.inrings  = (struct inring_entry *)
				    memalign(ARCH_DMA_MINALIGN,
					     ARCH_DMA_MINALIGN);
		g_jrdata.outrings = (struct outring_entry *)
				    memalign(ARCH_DMA_MINALIGN,
					     ARCH_DMA_MINALIGN);
		g_jrdata.desc = (u32 *)
				memalign(ARCH_DMA_MINALIGN, ARCH_DMA_MINALIGN);
		g_jrdata.status = RING_RELOC_INIT;
	} else {
		u32 align_idx = 0;

		/* Ensure 64bits buffers addresses alignment */
		if ((uintptr_t)g_jrdata.raw_addr & 0x7)
			align_idx = 1;
		g_jrdata.inrings  = (struct inring_entry *)
				    (&g_jrdata.raw_addr[align_idx]);
		g_jrdata.outrings = (struct outring_entry *)
				    (&g_jrdata.raw_addr[align_idx + 2]);
		g_jrdata.desc = (u32 *)(&g_jrdata.raw_addr[align_idx + 4]);
		g_jrdata.status = RING_EARLY_INIT;
	}

	if (!g_jrdata.inrings || !g_jrdata.outrings)
		return ERROR_ANY;

	/* Configure the HW Job Rings */
	ip_base = virt_to_phys((void *)g_jrdata.inrings);
	op_base = virt_to_phys((void *)g_jrdata.outrings);

	/* for imx8, JR0 and JR1 will be assigned to seco, so we use
	 * the JR3 instead.
	 */
#ifndef CONFIG_ARCH_IMX8
	__raw_writel(ip_base, CAAM_IRBAR0);
	__raw_writel(1, CAAM_IRSR0);

	__raw_writel(op_base, CAAM_ORBAR0);
	__raw_writel(1, CAAM_ORSR0);

	setbits_le32(CAAM_JRINTR0, JRINTR_JRI);
#else
	__raw_writel(ip_base, CAAM_IRBAR3);
	__raw_writel(1, CAAM_IRSR3);

	__raw_writel(op_base, CAAM_ORBAR3);
	__raw_writel(1, CAAM_ORSR3);

	setbits_le32(CAAM_JRINTR3, JRINTR_JRI);
#endif

	/*
	 * Configure interrupts but disable it:
	 * Optimization to generate an interrupt either when there are
	 * half of the job done or when there is a job done and
	 * 10 clock cycles elapse without new job complete
	 */
	value = 10 << BS_JRCFGR_LS_ICTT;
	value |= (1 << BS_JRCFGR_LS_ICDCT) & BM_JRCFGR_LS_ICDCT;
	value |= BM_JRCFGR_LS_ICEN;
	value |= BM_JRCFGR_LS_IMSK;
#ifndef CONFIG_ARCH_IMX8
	__raw_writel(value, CAAM_JRCFGR0_LS);

	/* Enable deco watchdog */
	setbits_le32(CAAM_MCFGR, BM_MCFGR_WDE);
#else
	__raw_writel(value, CAAM_JRCFGR3_LS);
#endif

	return 0;
}

#ifndef CONFIG_ARCH_IMX8
static void do_clear_rng_error(void)
{
	u32 val;

	val = __raw_readl(CAAM_RTMCTL);

	if (val & (RTMCTL_ERR | RTMCTL_FCT_FAIL)) {
		setbits_le32(CAAM_RTMCTL, RTMCTL_ERR);
	val = __raw_readl(CAAM_RTMCTL);
	}
}

static void do_inst_desc(u32 *desc, u32 status)
{
	u32 *pdesc = desc;
	u8  desc_len;
	bool add_sh0   = false;
	bool add_sh1   = false;
	bool load_keys = false;

	/*
	 * Modify the the descriptor to remove if necessary:
	 *  - The key loading
	 *  - One of the SH already instantiated
	 */
	desc_len = RNG_DESC_SH0_SIZE;
	if ((status & RDSTA_IF0) != RDSTA_IF0)
		add_sh0 = true;

	if ((status & RDSTA_IF1) != RDSTA_IF1) {
		add_sh1 = true;
		if (add_sh0)
			desc_len += RNG_DESC_SH1_SIZE;
	}

	if ((status & RDSTA_SKVN) != RDSTA_SKVN) {
		load_keys = true;
		desc_len += RNG_DESC_KEYS_SIZE;
	}

	/* Copy the SH0 descriptor anyway */
	memcpy(pdesc, rng_inst_sh0_desc, sizeof(rng_inst_sh0_desc));
	pdesc += RNG_DESC_SH0_SIZE;

	if (load_keys) {
		debug("RNG - Load keys\n");
		memcpy(pdesc, rng_inst_load_keys, sizeof(rng_inst_load_keys));
		pdesc += RNG_DESC_KEYS_SIZE;
	}

	if (add_sh1) {
		if (add_sh0) {
			debug("RNG - Instantiation of SH0 and SH1\n");
			/* Add the sh1 descriptor */
			memcpy(pdesc, rng_inst_sh1_desc,
				sizeof(rng_inst_sh1_desc));
		} else {
			debug("RNG - Instantiation of SH1 only\n");
			/* Modify the SH0 descriptor to instantiate only SH1 */
			desc[1] &= ~BM_ALGO_RNG_SH;
			desc[1] |= ALGO_RNG_SH(1);
		}
	}

	/* Setup the descriptor size */
	desc[0] &= ~(0x3F);
	desc[0] |= CAAM_HDR_DESCLEN(desc_len);
}

static void kick_trng(u32 ent_delay)
{
	u32 samples  = 512; /* number of bits to generate and test */
	u32 mono_min = 195;
	u32 mono_max = 317;
	u32 mono_range  = mono_max - mono_min;
	u32 poker_min = 1031;
	u32 poker_max = 1600;
	u32 poker_range = poker_max - poker_min + 1;
	u32 retries    = 2;
	u32 lrun_max   = 32;
	s32 run_1_min   = 27;
	s32 run_1_max   = 107;
	s32 run_1_range = run_1_max - run_1_min;
	s32 run_2_min   = 7;
	s32 run_2_max   = 62;
	s32 run_2_range = run_2_max - run_2_min;
	s32 run_3_min   = 0;
	s32 run_3_max   = 39;
	s32 run_3_range = run_3_max - run_3_min;
	s32 run_4_min   = -1;
	s32 run_4_max   = 26;
	s32 run_4_range = run_4_max - run_4_min;
	s32 run_5_min   = -1;
	s32 run_5_max   = 18;
	s32 run_5_range = run_5_max - run_5_min;
	s32 run_6_min   = -1;
	s32 run_6_max   = 17;
	s32 run_6_range = run_6_max - run_6_min;
	u32 val;

	/* Put RNG in program mode */
	/* Setting both RTMCTL:PRGM and RTMCTL:TRNG_ACC causes TRNG to
	 * properly invalidate the entropy in the entropy register and
	 * force re-generation.
	 */
	setbits_le32(CAAM_RTMCTL, RTMCTL_PGM | RTMCTL_ACC);

	/* Configure the RNG Entropy Delay
	 * Performance-wise, it does not make sense to
	 * set the delay to a value that is lower
	 * than the last one that worked (i.e. the state handles
	 * were instantiated properly. Thus, instead of wasting
	 * time trying to set the values controlling the sample
	 * frequency, the function simply returns.
	 */
	val = __raw_readl(CAAM_RTSDCTL);
	val &= BM_TRNG_ENT_DLY;
	val >>= BS_TRNG_ENT_DLY;
	if (ent_delay < val) {
		/* Put RNG4 into run mode */
		clrbits_le32(CAAM_RTMCTL, RTMCTL_PGM | RTMCTL_ACC);
		return;
	}

	val = (ent_delay << BS_TRNG_ENT_DLY) | samples;
	__raw_writel(val, CAAM_RTSDCTL);

	/*
	 * Recommended margins (min,max) for freq. count:
	 *   freq_mul = RO_freq / TRNG_clk_freq
	 *   rtfrqmin = (ent_delay x freq_mul) >> 1;
	 *   rtfrqmax = (ent_delay x freq_mul) << 3;
	 * Given current deployments of CAAM in i.MX SoCs, and to simplify
	 * the configuration, we consider [1,16] to be a safe interval
	 * for the freq_mul and the limits of the interval are used to compute
	 * rtfrqmin, rtfrqmax
	 */
	__raw_writel(ent_delay >> 1, CAAM_RTFRQMIN);
	__raw_writel(ent_delay << 7, CAAM_RTFRQMAX);

	__raw_writel((retries << 16) | lrun_max, CAAM_RTSCMISC);
	__raw_writel(poker_max, CAAM_RTPKRMAX);
	__raw_writel(poker_range, CAAM_RTPKRRNG);
	__raw_writel((mono_range << 16) | mono_max, CAAM_RTSCML);
	__raw_writel((run_1_range << 16) | run_1_max, CAAM_RTSCR1L);
	__raw_writel((run_2_range << 16) | run_2_max, CAAM_RTSCR2L);
	__raw_writel((run_3_range << 16) | run_3_max, CAAM_RTSCR3L);
	__raw_writel((run_4_range << 16) | run_4_max, CAAM_RTSCR4L);
	__raw_writel((run_5_range << 16) | run_5_max, CAAM_RTSCR5L);
	__raw_writel((run_6_range << 16) | run_6_max, CAAM_RTSCR6PL);

	val = __raw_readl(CAAM_RTMCTL);
	/*
	 * Select raw sampling in both entropy shifter
	 * and statistical checker
	 */
	val &= ~BM_TRNG_SAMP_MODE;
	val |= TRNG_SAMP_MODE_RAW_ES_SC;
	/* Put RNG4 into run mode */
	val &= ~(RTMCTL_PGM | RTMCTL_ACC);
/*test with sample mode only */
	__raw_writel(val, CAAM_RTMCTL);

	/* Clear the ERR bit in RTMCTL if set. The TRNG error can occur when the
	 * RNG clock is not within 1/2x to 8x the system clock.
	 * This error is possible if ROM code does not initialize the system PLLs
	 * immediately after PoR.
	 */
	/* setbits_le32(CAAM_RTMCTL, RTMCTL_ERR); */
}

static int do_instantiation(void)
{
	int ret = ERROR_ANY;
	u32 cha_vid_ls;
	u32 ent_delay;
	u32 status;

	if (!g_jrdata.desc) {
		printf("%d: CAAM Descriptor allocation error\n", __LINE__);
		return ERROR_ANY;
	}

	cha_vid_ls = __raw_readl(CAAM_CHAVID_LS);

	/*
	 * If SEC has RNG version >= 4 and RNG state handle has not been
	 * already instantiated, do RNG instantiation
	 */
	if (((cha_vid_ls & BM_CHAVID_LS_RNGVID) >> BS_CHAVID_LS_RNGVID) < 4) {
		printf("%d: RNG already instantiated\n", __LINE__);
		return 0;
	}

	ent_delay = TRNG_SDCTL_ENT_DLY_MIN;

	do {
		/* Read the CAAM RNG status */
		status = __raw_readl(CAAM_RDSTA);

		if ((status & RDSTA_IF0) != RDSTA_IF0) {
			/* Configure the RNG entropy delay */
			kick_trng(ent_delay);
			ent_delay += 400;
		}

		do_clear_rng_error();

		if ((status & (RDSTA_IF0 | RDSTA_IF1)) !=
				(RDSTA_IF0 | RDSTA_IF1)) {
			/* Prepare the instantiation descriptor */
			do_inst_desc(g_jrdata.desc, status);

			/* Run Job */
			ret = do_job(g_jrdata.desc);

			if (ret == ERROR_ANY) {
				/* CAAM JR failure ends here */
				printf("RNG Instantiation error\n");
				goto end_instantation;
			}
		} else {
			ret = SUCCESS;
			printf("RNG instantiation done (%d)\n", ent_delay);
			goto end_instantation;
		}
	} while (ent_delay < TRNG_SDCTL_ENT_DLY_MAX);

	printf("RNG Instantation Failure - Entropy delay (%d)\n", ent_delay);
	ret = ERROR_ANY;

end_instantation:
	return ret;
}

static void rng_init(void)
{
	int  ret;

	ret = jr_reset();
	if (ret != SUCCESS) {
		printf("Error CAAM JR reset\n");
		return;
	}

	ret = do_instantiation();

	if (ret != SUCCESS)
		printf("Error do_instantiation\n");

	jr_reset();

	return;
}

static void caam_clock_enable(void)
{
#if defined(CONFIG_ARCH_MX6)
	struct mxc_ccm_reg *mxc_ccm = (struct mxc_ccm_reg *)CCM_BASE_ADDR;
	u32 reg;

	reg = __raw_readl(&mxc_ccm->CCGR0);

	reg |= (MXC_CCM_CCGR0_CAAM_SECURE_MEM_MASK |
		MXC_CCM_CCGR0_CAAM_WRAPPER_ACLK_MASK |
		MXC_CCM_CCGR0_CAAM_WRAPPER_IPG_MASK);

	__raw_writel(reg, &mxc_ccm->CCGR0);

#ifndef CONFIG_MX6UL
	/* EMI slow clk */
	reg = __raw_readl(&mxc_ccm->CCGR6);
	reg |= MXC_CCM_CCGR6_EMI_SLOW_MASK;

	__raw_writel(reg, &mxc_ccm->CCGR6);
#endif

#elif defined(CONFIG_ARCH_MX7)
	HW_CCM_CCGR_SET(36, MXC_CCM_CCGR36_CAAM_DOMAIN0_MASK);
#elif defined(CONFIG_ARCH_MX7ULP)
	pcc_clock_enable(PER_CLK_CAAM, true);
#endif
}

static int jr_reset(void)
{
	/*
	 * Function reset the Job Ring HW
	 * Reset is done in 2 steps:
	 *  - Flush all pending jobs (Set RESET bit)
	 *  - Reset the Job Ring (Set RESET bit second time)
	 */
	u16 timeout = 10000;
	u32 reg_val;

	/* Mask interrupts to poll for reset completion status */
	setbits_le32(CAAM_JRCFGR0_LS, BM_JRCFGR_LS_IMSK);

	/* Initiate flush (required prior to reset) */
	__raw_writel(JRCR_RESET, CAAM_JRCR0);
	do {
		reg_val = __raw_readl(CAAM_JRINTR0);
		reg_val &= BM_JRINTR_HALT;
	} while ((reg_val == JRINTR_HALT_ONGOING) && --timeout);

	if (!timeout  || reg_val != JRINTR_HALT_DONE) {
		printf("Failed to flush job ring\n");
		return ERROR_ANY;
	}

	/* Initiate reset */
	timeout = 100;
	__raw_writel(JRCR_RESET, CAAM_JRCR0);
	do {
		reg_val = __raw_readl(CAAM_JRCR0);
	} while ((reg_val & JRCR_RESET) && --timeout);

	if (!timeout) {
		printf("Failed to reset job ring\n");
		return ERROR_ANY;
	}

	return 0;
}

#endif /* !CONFIG_ARCH_IMX8 */

#ifdef CONFIG_CAAM_KB_SELF_TEST
static void caam_hwrng_test(void)
{
	ALLOC_CACHE_ALIGN_BUFFER(uint8_t, out1, 32);
	ALLOC_CACHE_ALIGN_BUFFER(uint8_t, out2, 32);

	memset(out1, 0x00, sizeof(out1));
	memset(out2, 0x00, sizeof(out2));

	caam_hwrng(out1, sizeof(out1));
	caam_hwrng(out2, sizeof(out2));

	if (memcmp(out1, out2, sizeof(out1)))
		printf("caam hwrng test pass!\n");
	else
		printf("caam hwrng test fail!\n");
}

static void caam_blob_test(void)
{
	ALLOC_CACHE_ALIGN_BUFFER(uint8_t, plain, 32);
	ALLOC_CACHE_ALIGN_BUFFER(uint8_t, blob, 128);
	ALLOC_CACHE_ALIGN_BUFFER(uint8_t, plain_bak, 32);

	memset(plain, 0x00, sizeof(plain));
	memset(plain_bak, 0xff, sizeof(plain_bak));

	/* encapsulate blob */
	caam_gen_blob((ulong)plain, (ulong)blob, sizeof(plain));

	/* decapsulate blob */
	caam_decap_blob((ulong)plain_bak, (ulong)blob, sizeof(plain_bak));

	if (memcmp(plain, plain_bak, sizeof(plain)))
		printf("caam blob test fail!\n");
	else
		printf("caam blob test pass!\n");
}

static void caam_test(void)
{
	caam_hwrng_test();
	caam_blob_test();
}
#endif /* CONFIG_CAAM_KB_SELF_TEST */
