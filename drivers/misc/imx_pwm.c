/*
 * Porting to u-boot:
 * Linux IMX PWM driver
 *
 * Copyright (C) 2011 Freescale Semiconductor, Inc.
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

#include <linux/types.h>
#include <asm/io.h>
#include <asm/imx_pwm.h>
#include <common.h>
#include <div64.h>

#define MX_PWMCR                 0x00    /* PWM Control Register */
#define MX_PWMSAR                0x0C    /* PWM Sample Register */
#define MX_PWMPR                 0x10    /* PWM Period Register */
#define MX_PWMCR_PRESCALER(x)    (((x - 1) & 0xFFF) << 4)
#define MX_PWMCR_CLKSRC_IPG_HIGH (2 << 16)
#define MX_PWMCR_CLKSRC_IPG      (1 << 16)
#define MX_PWMCR_EN              (1 << 0)

#define MX_PWMCR_STOPEN		(1 << 25)
#define MX_PWMCR_DOZEEN		(1 << 24)
#define MX_PWMCR_WAITEN		(1 << 23)
#define MX_PWMCR_DBGEN		(1 << 22)
#define MX_PWMCR_CLKSRC_IPG	(1 << 16)
#define MX_PWMCR_CLKSRC_IPG_32k	(3 << 16)

int imx_pwm_config(struct pwm_device pwm, int duty_ns, int period_ns)
{
	unsigned long long c;
	unsigned long period_cycles, duty_cycles, prescale;
	u32 cr;

	if (period_ns == 0 || duty_ns > period_ns)
		return -1;

	pwm.mmio_base = pwm.pwm_id ? (unsigned long)IMX_PWM2_BASE:
				(unsigned long)IMX_PWM1_BASE;

	if (pwm.pwmo_invert)
		duty_ns = period_ns - duty_ns;

	c = mxc_get_clock(MXC_IPG_PERCLK);
	c = c * period_ns;
	do_div(c, 1000000000);
	period_cycles = c;

	prescale = period_cycles / 0x10000 + 1;

	period_cycles /= prescale;
	c = (unsigned long long)period_cycles * duty_ns;
	do_div(c, period_ns);
	duty_cycles = c;

	writel(duty_cycles, pwm.mmio_base + MX_PWMSAR);
	writel(period_cycles, pwm.mmio_base + MX_PWMPR);

	cr = MX_PWMCR_PRESCALER(prescale) |
		MX_PWMCR_STOPEN | MX_PWMCR_DOZEEN |
		MX_PWMCR_WAITEN | MX_PWMCR_DBGEN;

	cr |= MX_PWMCR_CLKSRC_IPG_HIGH;

	writel(cr, pwm.mmio_base + MX_PWMCR);

	return 0;
}

int imx_pwm_enable(struct pwm_device pwm)
{
	unsigned long reg;
	int rc = 0;

	if (pwm.enable_pwm_clk)
		pwm.enable_pwm_clk();

	pwm.mmio_base = pwm.pwm_id ? (unsigned long)IMX_PWM2_BASE:
				(unsigned long)IMX_PWM1_BASE;

	reg = readl(pwm.mmio_base + MX_PWMCR);
	reg |= MX_PWMCR_EN;
	writel(reg, pwm.mmio_base + MX_PWMCR);

	if (pwm.enable_pwm_pad)
		pwm.enable_pwm_pad();

	return rc;
}

int imx_pwm_disable(struct pwm_device pwm)
{
	if (pwm.disable_pwm_pad)
		pwm.disable_pwm_pad();

	pwm.mmio_base = pwm.pwm_id ? (unsigned long)IMX_PWM2_BASE:
				(unsigned long)IMX_PWM1_BASE;

	writel(0, pwm.mmio_base + MX_PWMCR);

	if (pwm.disable_pwm_clk)
		pwm.disable_pwm_clk();

	return 0;
}
