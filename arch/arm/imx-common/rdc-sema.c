/*
 * Copyright (C) 2014 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:  GPL-2.0+
 */
#include <common.h>
#include <asm/io.h>
#include <asm/arch/imx-regs.h>
#include <asm/imx-common/rdc-sema.h>
#include <asm/arch/imx-rdc.h>

#define SEMA_GATES_NUM 64

#define RDC_MDA_DID_SHIFT 0
#define RDC_MDA_DID_MASK (0x3 << RDC_MDA_DID_SHIFT)
#define RDC_MDA_LCK_SHIFT 31
#define RDC_MDA_LCK_MASK (0x1 << RDC_MDA_LCK_SHIFT)

#define RDC_PDAP_D0W_SHIFT 0
#define RDC_PDAP_D0W_MASK (0x1 << RDC_PDAP_D0W_SHIFT)
#define RDC_PDAP_D0R_SHIFT 1
#define RDC_PDAP_D0R_MASK (0x1 << RDC_PDAP_D0R_SHIFT)
#define RDC_PDAP_D1W_SHIFT 2
#define RDC_PDAP_D1W_MASK (0x1 << RDC_PDAP_D1W_SHIFT)
#define RDC_PDAP_D1R_SHIFT 3
#define RDC_PDAP_D1R_MASK (0x1 << RDC_PDAP_D1R_SHIFT)
#define RDC_PDAP_D2W_SHIFT 4
#define RDC_PDAP_D2W_MASK (0x1 << RDC_PDAP_D2W_SHIFT)
#define RDC_PDAP_D2R_SHIFT 5
#define RDC_PDAP_D2R_MASK (0x1 << RDC_PDAP_D2R_SHIFT)
#define RDC_PDAP_D3W_SHIFT 6
#define RDC_PDAP_D3W_MASK (0x1 << RDC_PDAP_D3W_SHIFT)
#define RDC_PDAP_D3R_SHIFT 7
#define RDC_PDAP_D3R_MASK (0x1 << RDC_PDAP_D3R_SHIFT)
#define RDC_PDAP_SREQ_SHIFT 30
#define RDC_PDAP_SREQ_MASK (0x1 << RDC_PDAP_SREQ_SHIFT)
#define RDC_PDAP_LCK_SHIFT 31
#define RDC_PDAP_LCK_MASK (0x1 << RDC_PDAP_LCK_SHIFT)

#define RDC_MRSA_SADR_SHIFT 7
#define RDC_MRSA_SADR_MASK (0x1ffffff << RDC_MRSA_SADR_SHIFT)

#define RDC_MREA_EADR_SHIFT 7
#define RDC_MREA_EADR_MASK (0x1ffffff << RDC_MREA_EADR_SHIFT)

#define RDC_MRC_D0W_SHIFT 0
#define RDC_MRC_D0W_MASK (0x1 << RDC_MRC_D0W_SHIFT)
#define RDC_MRC_D0R_SHIFT 1
#define RDC_MRC_D0R_MASK (0x1 << RDC_MRC_D0R_SHIFT)
#define RDC_MRC_D1W_SHIFT 2
#define RDC_MRC_D1W_MASK (0x1 << RDC_MRC_D1W_SHIFT)
#define RDC_MRC_D1R_SHIFT 3
#define RDC_MRC_D1R_MASK (0x1 << RDC_MRC_D1R_SHIFT)
#define RDC_MRC_D2W_SHIFT 4
#define RDC_MRC_D2W_MASK (0x1 << RDC_MRC_D2W_SHIFT)
#define RDC_MRC_D2R_SHIFT 5
#define RDC_MRC_D2R_MASK (0x1 << RDC_MRC_D2R_SHIFT)
#define RDC_MRC_D3W_SHIFT 6
#define RDC_MRC_D3W_MASK (0x1 << RDC_MRC_D3W_SHIFT)
#define RDC_MRC_D3R_SHIFT 7
#define RDC_MRC_D3R_MASK (0x1 << RDC_MRC_D3R_SHIFT)
#define RDC_MRC_ENA_SHIFT 30
#define RDC_MRC_ENA_MASK (0x1 << RDC_MRC_ENA_SHIFT)
#define RDC_MRC_LCK_SHIFT 31
#define RDC_MRC_LCK_MASK (0x1 << RDC_MRC_LCK_SHIFT)

#define RDC_MRVS_VDID_SHIFT 0
#define RDC_MRVS_VDID_MASK (0x3 << RDC_MRVS_VDID_SHIFT)
#define RDC_MRVS_AD_SHIFT 4
#define RDC_MRVS_AD_MASK (0x1 << RDC_MRVS_AD_SHIFT)
#define RDC_MRVS_VADDR_SHIFT 5
#define RDC_MRVS_VADDR_MASK (0x7ffffff << RDC_MRVS_VADDR_SHIFT)

#define RDC_SEMA_GATE_GTFSM_SHIFT 0
#define RDC_SEMA_GATE_GTFSM_MASK (0xf << RDC_SEMA_GATE_GTFSM_SHIFT)
#define RDC_SEMA_GATE_LDOM_SHIFT 5
#define RDC_SEMA_GATE_LDOM_MASK (0x3 << RDC_SEMA_GATE_LDOM_SHIFT)

#define RDC_SEMA_RSTGT_RSTGDP_SHIFT 0
#define RDC_SEMA_RSTGT_RSTGDP_MASK (0xff << RDC_SEMA_RSTGT_RSTGDP_SHIFT)
#define RDC_SEMA_RSTGT_RSTGSM_SHIFT 2
#define RDC_SEMA_RSTGT_RSTGSM_MASK (0x3 << RDC_SEMA_RSTGT_RSTGSM_SHIFT)
#define RDC_SEMA_RSTGT_RSTGMS_SHIFT 4
#define RDC_SEMA_RSTGT_RSTGMS_MASK (0xf << RDC_SEMA_RSTGT_RSTGMS_SHIFT)
#define RDC_SEMA_RSTGT_RSTGTN_SHIFT 8
#define RDC_SEMA_RSTGT_RSTGTN_MASK (0xff << RDC_SEMA_RSTGT_RSTGTN_SHIFT)

/*
 * Check the peripheral read / write access permission on Domain 0.
 * (Always assume the main CPU is in Domain 0)
 */
int imx_rdc_check_permission(int per_id)
{
	struct rdc_regs *imx_rdc = (struct rdc_regs *)RDC_BASE_ADDR;
	u32 reg;

	reg = readl(&imx_rdc->pdap[per_id]);
	if (!(reg & (RDC_PDAP_D0W_MASK | RDC_PDAP_D0R_MASK)))
		return 1;  /*No access*/
	return 0;
}

/*
 * Check if the RDC Semaphore is required for this peripheral.
 */
static inline int imx_rdc_check_sema_required(int per_id)
{
	struct rdc_regs *imx_rdc = (struct rdc_regs *)RDC_BASE_ADDR;
	u32 reg;

	reg = readl(&imx_rdc->pdap[per_id]);
	if (!(reg & RDC_PDAP_SREQ_MASK))
		return 1;  /*No semaphore*/
	return 0;
}

/*
 * Lock up the RDC semaphore for this peripheral if semaphore is required.
 */
int imx_rdc_sema_lock(int per_id)
{
	struct rdc_sema_regs *imx_rdc_sema;
	u8 reg;

	if (imx_rdc_check_sema_required(per_id))
		return 1;

	if (per_id < SEMA_GATES_NUM)
		imx_rdc_sema  = (struct rdc_sema_regs *)SEMAPHORE1_BASE_ADDR;
	else
		imx_rdc_sema  = (struct rdc_sema_regs *)SEMAPHORE2_BASE_ADDR;

	do {
		writeb(RDC_SEMA_PROC_ID, &imx_rdc_sema->gate[per_id % SEMA_GATES_NUM]);
		reg = readb(&imx_rdc_sema->gate[per_id % SEMA_GATES_NUM]);
		if ((reg & RDC_SEMA_GATE_GTFSM_MASK) == RDC_SEMA_PROC_ID)
			break;  /* Get the Semaphore*/
	} while (1);

	return 0;
}

/*
 * Unlock the RDC semaphore for this peripheral if main CPU is the semaphore owner.
 */
int imx_rdc_sema_unlock(int per_id)
{
	struct rdc_sema_regs *imx_rdc_sema;
	u8 reg;

	if (imx_rdc_check_sema_required(per_id))
		return 1;

	if (per_id < SEMA_GATES_NUM)
		imx_rdc_sema  = (struct rdc_sema_regs *)SEMAPHORE1_BASE_ADDR;
	else
		imx_rdc_sema  = (struct rdc_sema_regs *)SEMAPHORE2_BASE_ADDR;

	reg = readb(&imx_rdc_sema->gate[per_id % SEMA_GATES_NUM]);
	if ((reg & RDC_SEMA_GATE_GTFSM_MASK) != RDC_SEMA_PROC_ID)
		return 1;	/*Not the semaphore owner */

	writeb(0x0, &imx_rdc_sema->gate[per_id % SEMA_GATES_NUM]);

	return 0;
}

/*
 * Setup RDC setting for one peripheral
 */
int imx_rdc_setup_peri(rdc_peri_cfg_t p)
{
	struct rdc_regs *imx_rdc = (struct rdc_regs *)RDC_BASE_ADDR;
	u32 reg = 0;
	u32 share_count = 0;
	u32 peri_id = p & RDC_PERI_MASK;
	u32 domain = (p & RDC_DOMAIN_MASK) >> RDC_DOMAIN_SHIFT_BASE;

	if (domain == 0)
		return 1;

	reg |= domain;

	share_count = (domain & 0x3)
		+ ((domain >> 2) & 0x3)
		+ ((domain >> 4) & 0x3)
		+ ((domain >> 6) & 0x3);

	if (share_count > 0x3)
		reg |= RDC_PDAP_SREQ_MASK;

	writel(reg, &imx_rdc->pdap[peri_id]);

	return 0;
}

/*
 * Setup RDC settings for multiple peripherals
 */
int imx_rdc_setup_peripherals(rdc_peri_cfg_t const *peripherals_list,
				     unsigned count)
{
	rdc_peri_cfg_t const *p = peripherals_list;
	int i;
	int ret;

	for (i = 0; i < count; i++) {
		ret = imx_rdc_setup_peri(*p);
		if (ret)
			return ret;
		p++;
	}
	return 0;
}

/*
 * Setup RDC setting for one master
 */
int imx_rdc_setup_ma(rdc_ma_cfg_t p)
{
	struct rdc_regs *imx_rdc = (struct rdc_regs *)RDC_BASE_ADDR;
	u32 master_id = (p & RDC_MASTER_MASK) >> RDC_MASTER_SHIFT;
	u32 domain = (p & RDC_DOMAIN_MASK) >> RDC_DOMAIN_SHIFT_BASE;

	writel((domain & RDC_MDA_DID_MASK), &imx_rdc->mda[master_id]);

	return 0;
}

/*
 * Setup RDC settings for multiple masters
 */
int imx_rdc_setup_masters(rdc_ma_cfg_t const *masters_list,
				     unsigned count)
{
	rdc_ma_cfg_t const *p = masters_list;
	int i;
	int ret;

	for (i = 0; i < count; i++) {
		ret = imx_rdc_setup_ma(*p);
		if (ret)
			return ret;
		p++;
	}
	return 0;
}
