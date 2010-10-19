/*
 * (C) Copyright 2007
 * Sascha Hauer, Pengutronix
 *
 * Copyright (C) 2010 Freescale Semiconductor, Inc.
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
#include <asm/arch/mx53.h>
#include <div64.h>

/* General purpose timers registers */
#define GPTCR   __REG(GPT1_BASE_ADDR)	/* Control register */
#define GPTPR  	__REG(GPT1_BASE_ADDR + 0x4)	/* Prescaler register */
#define GPTSR   __REG(GPT1_BASE_ADDR + 0x8)	/* Status register */
#define GPTCNT 	__REG(GPT1_BASE_ADDR + 0x24)	/* Counter register */

/* General purpose timers bitfields */
#define GPTCR_SWR       (1<<15)	/* Software reset */
#define GPTCR_FRR       (1<<9)	/* Freerun / restart */
#define GPTCR_CLKSOURCE_32	(4<<6)	/* Clock source */
#define GPTCR_TEN       (1)	/* Timer enable */

static ulong timestamp;
static ulong lastinc;

static inline void setup_gpt(void)
{
	int i;
	static int init_done;

	if (init_done)
		return;

	init_done = 1;

	/* setup GP Timer 1 */
	GPTCR = GPTCR_SWR;
	for (i = 0; i < 100; i++)
		GPTCR = 0;      	/* We have no udelay by now */
	GPTPR = 0;	/* 32KHz */
	/* Freerun Mode, CLK32 input */
	GPTCR = GPTCR | GPTCR_CLKSOURCE_32 | GPTCR_TEN;
	reset_timer_masked();
}

int timer_init(void)
{
	setup_gpt();

	return 0;
}

void reset_timer_masked(void)
{
	/* capture current incrementer value time */
	lastinc = GPTCNT / (CONFIG_MX53_CLK32 / CONFIG_SYS_HZ);
	timestamp = 0; /* start "advancing" time stamp from 0 */
}

void reset_timer(void)
{
	reset_timer_masked();
}

inline ulong get_timer_masked(void)
{
	ulong val = GPTCNT;
	val /= (CONFIG_MX53_CLK32 / CONFIG_SYS_HZ);
	if (val >= lastinc)
		timestamp += (val - lastinc);
	else
		timestamp += ((0xFFFFFFFF / (CONFIG_MX53_CLK32 / CONFIG_SYS_HZ))
			- lastinc) + val;
	lastinc = val;
	return timestamp;
}

ulong get_timer(ulong base)
{
	return get_timer_masked() - base;
}

void set_timer(ulong t)
{
	timestamp = t;
}

/* delay x useconds AND perserve advance timstamp value */
/* GPTCNT is now supposed to tick 1 by 1 us. */
void udelay(unsigned long usec)
{
	unsigned long now, start, tmo;
	setup_gpt();

	tmo = usec * (CONFIG_MX53_CLK32 / 1000) / 1000;
	if (!tmo)
		tmo = 1;

	now = start = GPTCNT;

	while ((now - start) < tmo)
		now = GPTCNT;
}
