/*
 * (C) Copyright 2009
 * Stefano Babic, DENX Software Engineering, sbabic@denx.de.
 *
 * Copyright (C) 2016 Freescale Semiconductor, Inc.
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _SYS_PROTO_H_
#define _SYS_PROTO_H_

#include <asm/imx-common/regs-common.h>
#include <common.h>
#include "../arch-imx/cpu.h"

#define soc_rev() (get_cpu_rev() & 0xFF)
#define is_soc_rev(rev) (soc_rev() == rev)

/* returns MXC_CPU_ value */
#define cpu_type(rev) (((rev) >> 12) & 0xff)
#define soc_type(rev) (((rev) >> 12) & 0xf0)
/* both macros return/take MXC_CPU_ constants */
#define get_cpu_type() (cpu_type(get_cpu_rev()))
#define get_soc_type() (soc_type(get_cpu_rev()))
#define is_cpu_type(cpu) (get_cpu_type() == cpu)
#define is_soc_type(soc) (get_soc_type() == soc)

#define is_mx6() (is_soc_type(MXC_SOC_MX6))
#define is_mx7() (is_soc_type(MXC_SOC_MX7))
#define is_imx8() (is_soc_type(MXC_SOC_IMX8))
#define is_imx8m() (is_soc_type(MXC_SOC_IMX8M))

#define is_mx6dqp() (is_cpu_type(MXC_CPU_MX6QP) || is_cpu_type(MXC_CPU_MX6DP))
#define is_mx6dq() (is_cpu_type(MXC_CPU_MX6Q) || is_cpu_type(MXC_CPU_MX6D))
#define is_mx6sdl() (is_cpu_type(MXC_CPU_MX6SOLO) || is_cpu_type(MXC_CPU_MX6DL))
#define is_mx6dl() (is_cpu_type(MXC_CPU_MX6DL))
#define is_mx6sx() (is_cpu_type(MXC_CPU_MX6SX))
#define is_mx6sl() (is_cpu_type(MXC_CPU_MX6SL))
#define is_mx6solo() (is_cpu_type(MXC_CPU_MX6SOLO))
#define is_mx6ul() (is_cpu_type(MXC_CPU_MX6UL))
#define is_mx6ull() (is_cpu_type(MXC_CPU_MX6ULL))
#define is_mx6sll() (is_cpu_type(MXC_CPU_MX6SLL))

#define is_mx7ulp() (is_cpu_type(MXC_CPU_MX7ULP))

#define is_imx8mq() (is_cpu_type(MXC_CPU_IMX8MQ))
#define is_imx8mm() (is_cpu_type(MXC_CPU_IMX8MM))
#define is_imx8qm() (is_cpu_type(MXC_CPU_IMX8QM))
#define is_imx8qxp() (is_cpu_type(MXC_CPU_IMX8QXP))
#define is_imx8dx() (is_cpu_type(MXC_CPU_IMX8DX))

u32 get_nr_cpus(void);
u32 get_cpu_rev(void);
u32 get_cpu_speed_grade_hz(void);
u32 get_cpu_temp_grade(int *minc, int *maxc);
const char *get_imx_type(u32 imxtype);
u32 imx_ddr_size(void);
void sdelay(unsigned long);
void set_chipselect_size(int const);

void init_aips(void);
void init_src(void);
void imx_set_wdog_powerdown(bool enable);

/*
 * Initializes on-chip ethernet controllers.
 * to override, implement board_eth_init()
 */
int fecmxc_initialize(bd_t *bis);
u32 get_ahb_clk(void);
u32 get_periph_clk(void);

void lcdif_power_down(void);

int mxs_reset_block(struct mxs_register_32 *reg);
int mxs_wait_mask_set(struct mxs_register_32 *reg, u32 mask, u32 timeout);
int mxs_wait_mask_clr(struct mxs_register_32 *reg, u32 mask, u32 timeout);

int mmc_get_env_dev(void);
void board_late_mmc_env_init(void);

void vadc_power_up(void);
void vadc_power_down(void);

void pcie_power_up(void);
void pcie_power_off(void);
int arch_auxiliary_core_up(u32 core_id, ulong boot_private_data);
int arch_auxiliary_core_check_up(u32 core_id);

#ifdef CONFIG_ARM64
unsigned long call_imx_sip(unsigned long id, unsigned long reg0, unsigned long reg1, unsigned long reg2, unsigned long reg3);
unsigned long call_imx_sip_ret2(unsigned long id, unsigned long reg0, unsigned long *reg1, unsigned long reg2, unsigned long reg3);
#endif
#endif
