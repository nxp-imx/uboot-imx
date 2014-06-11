/*
 * Copyright (C) 2014 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:  GPL-2.0+
 */

#ifndef __RDC_SEMA_H__
#define __RDC_SEMA_H__

typedef u32 rdc_peri_cfg_t;
typedef u32 rdc_ma_cfg_t;

#define RDC_PERI_MASK 0xFF
#define RDC_PERI_SHIFT 0

#define RDC_DOMAIN_SHIFT_BASE   16
#define RDC_DOMAIN_MASK 0xFF0000
#define RDC_DOMAIN_SHIFT(x)   (RDC_DOMAIN_SHIFT_BASE + ((x << 1)))
#define RDC_DOMAIN(x)	((rdc_peri_cfg_t)(0x3 << RDC_DOMAIN_SHIFT(x)))

#define RDC_MASTER_SHIFT   8
#define RDC_MASTER_MASK 0xFF00
#define RDC_MASTER_CFG(master_id, domain_id) (rdc_ma_cfg_t)((master_id << 8) | (domain_id << RDC_DOMAIN_SHIFT_BASE))

int imx_rdc_check_permission(int per_id);
int imx_rdc_sema_lock(int per_id);
int imx_rdc_sema_unlock(int per_id);
int imx_rdc_setup_peri(rdc_peri_cfg_t p);
int imx_rdc_setup_peripherals(rdc_peri_cfg_t const *peripherals_list,
	unsigned count);
int imx_rdc_setup_ma(rdc_ma_cfg_t p);
int imx_rdc_setup_masters(rdc_ma_cfg_t const *masters_list,
	unsigned count);

#endif	/* __RDC_SEMA_H__*/
