/*
 * (C) Copyright 2007
 * Sascha Hauer, Pengutronix
 *
 * (C) Copyright 2009-2010 Freescale Semiconductor
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

#include <common.h>
#include <asm/errno.h>
#include <asm/arch/mx25-regs.h>

static u32 mx25_decode_pll(u32 reg)
{
	u32 mfi = (reg >> 10) & 0xf;
	u32 mfn = reg & 0x3ff;
	u32 mfd = (reg >> 16) & 0x3ff;
	u32 pd =  (reg >> 26) & 0xf;

	u32 ref_clk = PLL_REF_CLK;

	mfi = mfi <= 5 ? 5 : mfi;
	mfd += 1;
	pd += 1;

	return ((2 * (ref_clk >> 10) * (mfi * mfd + mfn)) /
		(mfd * pd)) << 10;
}

static u32 mx25_get_mcu_main_clk(void)
{
	u32 cctl = __REG(CCM_CCTL);
	u32 ret_val = mx25_decode_pll(__REG(CCM_MPCTL));

	if (cctl & CRM_CCTL_ARM_SRC) {
		ret_val *= 3;
		ret_val /= 4;
	}

	return ret_val;
}

static u32 mx25_get_ahb_clk(void)
{
	u32 cctl = __REG(CCM_CCTL);
	u32 ahb_div = ((cctl >> CRM_CCTL_AHB_OFFSET) & 3) + 1;

	return mx25_get_mcu_main_clk()/ahb_div;
}

unsigned int mx25_get_ipg_clk(void)
{
	return mx25_get_ahb_clk()/2;
}

unsigned int mx25_get_cspi_clk(void)
{
	return mx25_get_ipg_clk();
}

void mx25_dump_clocks(void)
{
	u32 cpufreq = mx25_get_mcu_main_clk();
	printf("mx25 cpu clock: %dMHz\n", cpufreq / 1000000);
	printf("ipg clock     : %dHz\n", mx25_get_ipg_clk());
}

unsigned int mxc_get_clock(enum mxc_clock clk)
{
	switch (clk) {
	case MXC_ARM_CLK:
		return mx25_get_mcu_main_clk();
	case MXC_AHB_CLK:
		return mx25_get_ahb_clk();
		break;
	case MXC_IPG_PERCLK:
	case MXC_IPG_CLK:
		return mx25_get_ipg_clk();
	case MXC_CSPI_CLK:
		return mx25_get_cspi_clk();
	case MXC_UART_CLK:
		break;
	case MXC_ESDHC_CLK:
		return mx25_get_ipg_clk();
		break;
	}
	return -1;
}

#if defined(CONFIG_DISPLAY_CPUINFO)
int print_cpuinfo(void)
{
	printf("CPU:   Freescale i.MX25 at %d MHz\n",
		mx25_get_mcu_main_clk() / 1000000);
	mx25_dump_clocks();
	return 0;
}
#endif

#if defined(CONFIG_MXC_FEC)
extern int mxc_fec_initialize(bd_t *bis);
extern void mxc_fec_set_mac_from_env(char *mac_addr);
#endif

/*
 * Initializes on-chip ethernet controllers.
 * to override, implement board_eth_init()
 */
int cpu_eth_init(bd_t *bis)
{
	int rc = -ENODEV;

#if defined(CONFIG_MXC_FEC)
	rc = mxc_fec_initialize(bis);
#endif

	return rc;
}

