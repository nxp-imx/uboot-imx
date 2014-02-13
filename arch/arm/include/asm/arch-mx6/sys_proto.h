/*
 * (C) Copyright 2009
 * Stefano Babic, DENX Software Engineering, sbabic@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef _SYS_PROTO_H_
#define _SYS_PROTO_H_

#include <asm/imx-common/regs-common.h>

#define MXC_CPU_MX51		0x51
#define MXC_CPU_MX53		0x53
#define MXC_CPU_MX6SL		0x60
#define MXC_CPU_MX6DL		0x61
#define MXC_CPU_MX6SX		0x62
#define MXC_CPU_MX6Q		0x63
#define MXC_CPU_MX6SOLO		0x66 /* dummy */

#define is_soc_rev(rev)		((int)((get_cpu_rev() & 0xFF) - rev))
#define is_soc(soc)		(((get_cpu_rev() >> 12) & 0xFF) ==  (soc))
#define is_mx6dq()		is_soc(MXC_CPU_MX6Q)
#define is_mx6dl()		is_soc(MXC_CPU_MX6DL)
#define is_mx6solo()		is_soc(MXC_CPU_MX6SOLO)
#define is_mx6dlsolo()		(is_mx6dl() || is_mx6solo())
#define is_mx6sl()		is_soc(MXC_CPU_MX6SL)
#define is_mx6sx()		is_soc(MXC_CPU_MX6SX)

u32 get_cpu_rev(void);
const char *get_imx_type(u32 imxtype);
unsigned imx_ddr_size(void);

void set_vddsoc(u32 mv);
#ifdef CONFIG_LDO_BYPASS_CHECK
int check_ldo_bypass(void);
int check_1_2G(void);
void set_anatop_bypass(void);
void ldo_mode_set(int ldo_bypass);
#endif

/*
 * Initializes on-chip ethernet controllers.
 * to override, implement board_eth_init()
 */

int fecmxc_initialize(bd_t *bis);
u32 get_ahb_clk(void);
u32 get_periph_clk(void);
int get_hab_status(void);

int mxs_reset_block(struct mxs_register_32 *reg);
int mxs_wait_mask_set(struct mxs_register_32 *reg,
		       uint32_t mask,
		       unsigned int timeout);
int mxs_wait_mask_clr(struct mxs_register_32 *reg,
		       uint32_t mask,
		       unsigned int timeout);
#endif
