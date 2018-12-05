/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __ASM_ARCH_IMX8_LPCG_H__
#define __ASM_ARCH_IMX8_LPCG_H__

#if defined(CONFIG_IMX8QM)
#include "imx8qm_lpcg.h"
#elif defined(CONFIG_IMX8QXP)
#include "imx8qxp_lpcg.h"
#else
#error "No lpcg header"
#endif

void LPCG_ClockOff(u32 lpcg_addr, u8 clk);
void LPCG_ClockOn(u32 lpcg_addr, u8 clk);
void LPCG_ClockAutoGate(u32 lpcg_addr, u8 clk);
void LPCG_AllClockOff(u32 lpcg_addr);
void LPCG_AllClockOn(u32 lpcg_addr);
void LPCG_AllClockAutoGate(u32 lpcg_addr);

#endif	/* __ASM_ARCH_IMX8_LPCG_H__ */
