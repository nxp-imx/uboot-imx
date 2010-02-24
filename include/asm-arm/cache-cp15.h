/*
 * (C) Copyright 2008-2010 Freescale Semiconductor, Inc.
 *
 * (C) Copyright 2002
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
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
#include <asm/system.h>

#if !(defined(CONFIG_SYS_NO_ICACHE) && defined(CONFIG_SYS_NO_DCACHE))
#define cp_delay()	\
{	\
	volatile int i;	\
	/* copro seems to need some delay between reading and writing */	\
	for (i = 0; i < 100; i++)	\
		nop();	\
}

/* cache_bit must be either CR_I or CR_C */
#define cache_enable(cache_bit)	\
{	\
	uint32_t reg;	\
	reg = get_cr();	/* get control reg. */	\
	set_cr(reg | cache_bit);	\
	cp_delay();	\
}

/* cache_bit must be either CR_I or CR_C */
#define cache_disable(cache_bit)	\
{	\
	uint32_t reg;	\
	reg = get_cr();	\
	set_cr(reg & ~cache_bit);	\
	cp_delay();	\
}

#endif

#ifdef CONFIG_SYS_NO_ICACHE
#define icache_enable()

#define icache_disable()

#define icache_status()
#else
#define icache_enable()		(cache_enable(CR_I))

#define icache_disable()	(cache_disable(CR_I))

#define icache_status()		((get_cr() & CR_I) != 0)
#endif

#ifdef CONFIG_SYS_NO_DCACHE
#define dcache_enable()

#define dcache_disable()

#define dcache_status()
#else
#define dcache_enable()		(cache_enable(CR_C))

#define dcache_disable()	\
{	\
	cache_disable(CR_C);	\
}

#define dcache_status()	((get_cr() & CR_C) != 0)

#endif
