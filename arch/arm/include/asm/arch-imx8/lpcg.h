/*
 * Copyright 2018-2020 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __ASM_ARCH_IMX8_LPCG_H__
#define __ASM_ARCH_IMX8_LPCG_H__

#if defined(CONFIG_IMX8QM)
#include "imx8qm_lpcg.h"
#elif defined(CONFIG_IMX8QXP)
#include "imx8qxp_lpcg.h"
#elif defined(CONFIG_IMX8DXL)
#include "imx8qxp_lpcg.h"
#else
#error "No lpcg header"
#endif

void lpcg_clock_off(u32 lpcg_addr, u8 clk);
void lpcg_clock_on(u32 lpcg_addr, u8 clk);
void lpcg_clock_autogate(u32 lpcg_addr, u8 clk);
bool lpcg_is_clock_on(u32 lpcg_addr, u8 clk);
void lpcg_all_clock_off(u32 lpcg_addr);
void lpcg_all_clock_on(u32 lpcg_addr);
void lpcg_all_clock_autogate(u32 lpcg_addr);

#endif	/* __ASM_ARCH_IMX8_LPCG_H__ */
