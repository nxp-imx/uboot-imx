/*
 * (C) Copyright 2007
 * Sascha Hauer, Pengutronix
 *
 * Copyright (C) 2010-2011 Freescale Semiconductor, Inc.
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
#include <asm/io.h>
#include <asm/arch/mx53.h>
#include <asm/arch/iomux.h>

/* nothing really to do with interrupts, just starts up a counter. */
int interrupt_init(void)
{
	return 0;
}

void reset_cpu(ulong addr)
{
#if defined(CONFIG_MX53_SMD)
	unsigned int reg;
#endif

	/* de-select SS0 of instance: eCSPI1 */
	mxc_request_iomux(MX53_PIN_EIM_EB2, IOMUX_CONFIG_ALT1);
	/* de-select SS1 of instance: eCSPI1 */
	mxc_request_iomux(MX53_PIN_EIM_D19, IOMUX_CONFIG_ALT1);

#if defined(CONFIG_MX53_SMD)
	/* GPIO_1_9 */
	reg = readl(GPIO1_BASE_ADDR + 0x4);
	reg |= (0x1 << 9);
	writel(reg, GPIO1_BASE_ADDR + 0x4);

	reg = readl(GPIO1_BASE_ADDR);
	reg &= ~0x200;
	writel(reg, GPIO1_BASE_ADDR);
#else
	__REG16(WDOG1_BASE_ADDR) = 4;
#endif
}
