/*
 * (C) Copyright 2010 Freescale Semiconductor, Inc.
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
#include <asm/arch/mx28.h>
#include <asm/arch/regs-clkctrl.h>

#if defined(CONFIG_ARCH_CPU_INIT)
int arch_cpu_init(void)
{
	icache_enable();
	dcache_enable();
	return 0;
}
#endif

#if defined(CONFIG_DISPLAY_CPUINFO)
int print_cpuinfo(void)
{
	const u32 xtal = 24, ref = 480;
	u32 cpu, bus, emi;
	u32 clkfrac, clkdeq, clkctrl;
	u32 frac, div;

	clkfrac = REG_RD(REGS_CLKCTRL_BASE, HW_CLKCTRL_FRAC0);
	clkdeq = REG_RD(REGS_CLKCTRL_BASE, HW_CLKCTRL_CLKSEQ);

	/* CPU */
	clkctrl = REG_RD(REGS_CLKCTRL_BASE, HW_CLKCTRL_CPU);
	if (clkctrl & (BM_CLKCTRL_CPU_DIV_XTAL_FRAC_EN |
		BM_CLKCTRL_CPU_DIV_CPU_FRAC_EN)) {
		/* No support of fractional divider calculation */
		cpu = 0;
	} else {
		if (clkdeq & BM_CLKCTRL_CLKSEQ_BYPASS_CPU) {
			/* xtal path */
			div = (clkctrl & BM_CLKCTRL_CPU_DIV_XTAL) >>
				BP_CLKCTRL_CPU_DIV_XTAL;
			cpu = xtal / div;
		} else {
			/* ref path */
			frac = (clkfrac & BM_CLKCTRL_FRAC0_CPUFRAC) >>
				BP_CLKCTRL_FRAC0_CPUFRAC;
			div = (clkctrl & BM_CLKCTRL_CPU_DIV_CPU) >>
				BP_CLKCTRL_CPU_DIV_CPU;
			cpu =  (ref * 18 / frac) / div;
		}
	}

	/* BUS */
	clkctrl = REG_RD(REGS_CLKCTRL_BASE, HW_CLKCTRL_HBUS);
	if (clkctrl & BM_CLKCTRL_HBUS_DIV_FRAC_EN) {
		/* No support of fractional divider calculation */
		bus = 0;
	} else {
		div = (clkctrl & BM_CLKCTRL_HBUS_DIV) >>
			BP_CLKCTRL_HBUS_DIV;
		bus = cpu / div;
	}

	/* EMI */
	clkctrl = REG_RD(REGS_CLKCTRL_BASE, HW_CLKCTRL_EMI);
	if (clkdeq & BM_CLKCTRL_CLKSEQ_BYPASS_EMI) {
		/* xtal path */
		div = (clkctrl & BM_CLKCTRL_EMI_DIV_XTAL) >>
			BP_CLKCTRL_EMI_DIV_XTAL;
		emi = xtal / div;
	} else {
		/* ref path */
		frac = (clkfrac & BM_CLKCTRL_FRAC0_EMIFRAC) >>
			BP_CLKCTRL_FRAC0_EMIFRAC;
		div = (clkctrl & BM_CLKCTRL_EMI_DIV_EMI) >>
			BP_CLKCTRL_EMI_DIV_EMI;
		emi =  (ref * 18 / frac) / div;
	}

	/* Print */
	printf("Freescale i.MX28 family\n");
	printf("CPU:   %d MHz\n", cpu);
	printf("BUS:   %d MHz\n", bus);
	printf("EMI:   %d MHz\n", emi);

	return 0;
}
#endif

/*
 * Initializes on-chip MMC controllers.
 */
#if defined(CONFIG_MXC_ENET)
int imx_ssp_mmc_initialize(bd_t *bis);
#endif
int cpu_mmc_init(bd_t *bis)
{
	int rc = ENODEV;
#ifdef CONFIG_IMX_SSP_MMC
	rc = imx_ssp_mmc_initialize(bis);
#endif
	return rc;
}

/*
 * Initializes on-chip ethernet controllers.
 */
#if defined(CONFIG_MXC_ENET)
int mxc_enet_initialize(bd_t *bis);
#endif
int cpu_eth_init(bd_t *bis)
{
	int rc = ENODEV;
#if defined(CONFIG_MXC_ENET)
	rc = mxc_enet_initialize(bis);
#endif
	return rc;
}

