/*
 * (C) Copyright 2009
 * Stefano Babic, DENX Software Engineering, sbabic@denx.de.
 *
 * (C) Copyright 2009-2015 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _SYS_PROTO_H_
#define _SYS_PROTO_H_

#include <asm/imx-common/regs-common.h>
#include "../arch-imx/cpu.h"

#define soc_rev() (get_cpu_rev() & 0xFF)
#define is_soc_rev(rev)        (int)(soc_rev() - rev)

u32 get_nr_cpus(void);
u32 get_cpu_rev(void);

/* returns MXC_CPU_ value */
#define cpu_type(rev) (((rev) >> 12)&0xff)

/* both macros return/take MXC_CPU_ constants */
#define get_cpu_type()	(cpu_type(get_cpu_rev()))
#define is_cpu_type(cpu) (get_cpu_type() == cpu)

const char *get_imx_type(u32 imxtype);
unsigned imx_ddr_size(void);
void set_chipselect_size(int const);
void set_wdog_reset(struct wdog_regs *wdog);

#ifdef CONFIG_LDO_BYPASS_CHECK
int check_ldo_bypass(void);
int check_1_2G(void);
int set_anatop_bypass(int wdog_reset_pin);
void ldo_mode_set(int ldo_bypass);
void prep_anatop_bypass(void);
void finish_anatop_bypass(void);
#endif

#ifdef CONFIG_MX6SX
int arch_auxiliary_core_up(u32 core_id, u32 boot_private_data);
int arch_auxiliary_core_check_up(u32 core_id);
#endif

#define is_mx6dqp() ((is_cpu_type(MXC_CPU_MX6Q) || is_cpu_type(MXC_CPU_MX6D)) \
	&& (is_soc_rev(CHIP_REV_2_0) >= 0))

/*
 * Initializes on-chip ethernet controllers.
 * to override, implement board_eth_init()
 */

int fecmxc_initialize(bd_t *bis);
u32 get_ahb_clk(void);
u32 get_periph_clk(void);

int mxs_reset_block(struct mxs_register_32 *reg);
int mxs_wait_mask_set(struct mxs_register_32 *reg,
		       uint32_t mask,
		       unsigned int timeout);
int mxs_wait_mask_clr(struct mxs_register_32 *reg,
		       uint32_t mask,
		       unsigned int timeout);
#endif
