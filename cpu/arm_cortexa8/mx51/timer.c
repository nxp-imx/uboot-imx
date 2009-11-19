/*
 * (C) Copyright 2007
 * Sascha Hauer, Pengutronix
 *
 * (C) Copyright 2008-2009 Freescale Semiconductor, Inc.
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
#include <asm/arch/mx51.h>

/* General purpose timers registers */
#define GPTCR   __REG(GPT1_BASE_ADDR)	/* Control register */
#define GPTPR  	__REG(GPT1_BASE_ADDR + 0x4)	/* Prescaler register */
#define GPTSR   __REG(GPT1_BASE_ADDR + 0x8)	/* Status register */
#define GPTCNT 	__REG(GPT1_BASE_ADDR + 0x24)	/* Counter register */

/* General purpose timers bitfields */
#define GPTCR_SWR       (1<<15)	/* Software reset */
#define GPTCR_FRR       (1<<9)	/* Freerun / restart */
#define GPTCR_CLKSOURCE_32   (0x100<<6)	/* Clock source */
#define GPTCR_CLKSOURCE_IPG (0x001<<6)	/* Clock source */
#define GPTCR_TEN       (1)	/* Timer enable */
#define GPTPR_VAL	(66)

static inline void setup_gpt()
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
	GPTPR = GPTPR_VAL;	/* 66Mhz / 66 */
	/* Freerun Mode, PERCLK1 input */
	GPTCR |= GPTCR_CLKSOURCE_IPG | GPTCR_TEN;
}

int timer_init(void)
{
	setup_gpt();

	return 0;
}

void reset_timer_masked(void)
{
	GPTCR = 0;
	/* Freerun Mode, PERCLK1 input */
	GPTCR = GPTCR_CLKSOURCE_IPG | GPTCR_TEN;
}

inline ulong get_timer_masked(void)
{
	ulong val = GPTCNT;

	return val;
}

void reset_timer(void)
{
	reset_timer_masked();
}

ulong get_timer(ulong base)
{
	ulong tmp;

	tmp = get_timer_masked();

	if (tmp <= (base * 1000)) {
		/* Overflow */
		tmp += (0xffffffff - base);
	}

	return (tmp / 1000) - base;
}

void set_timer(ulong t)
{
}

/* delay x useconds AND perserve advance timstamp value */
/* GPTCNT is now supposed to tick 1 by 1 us. */
void udelay(unsigned long usec)
{
	ulong tmp;

	setup_gpt();

	tmp = get_timer_masked();	/* get current timestamp */

	/* if setting this forward will roll time stamp */
	if ((usec + tmp + 1) < tmp) {
		/* reset "advancing" timestamp to 0, set lastinc value */
		reset_timer_masked();
	} else {
		/* else, set advancing stamp wake up time */
		tmp += usec;
	}

	while (get_timer_masked() < tmp)	/* loop till event */
		 /*NOP*/;
}
