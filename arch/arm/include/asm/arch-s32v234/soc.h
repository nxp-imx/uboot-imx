/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2017 NXP.
 */

#include <asm/arch/imx-regs.h>
#include <asm/arch/clock.h>
#include <asm/arch/mc_cgm_regs.h>
#include <asm/arch/mc_me_regs.h>
#include <asm/arch/mc_rgm_regs.h>
#include <asm/arch/src.h>
#include <asm/arch/mmdc.h>
#include <asm/arch/ddr.h>
#if defined(CONFIG_S32V234_LPDDR2)
#include <asm/arch/lpddr2.h>
#else
#error "Please define the DDR type for S32V234 board!"
#endif

