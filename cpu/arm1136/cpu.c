/*
 * (C) Copyright 2004 Texas Insturments
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * (C) Copyright 2002
 * Gary Jennejohn, DENX Software Engineering, <garyj@denx.de>
 *
 * (C) Copyright 2008-2010 Freescale Semiconductor, Inc.
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

/*
 * CPU specific code
 */

#include <common.h>
#include <command.h>
#include <asm/system.h>
#include <asm/cache-cp15.h>
#include <asm/mmu.h>

#define dcache_invalidate_all_l1()	\
{	\
	int i = 0;	\
	/* Clean and Invalidate Entire Data Cache */       \
	asm volatile ("mcr p15, 0, %0, c7, c14, 0;"	\
			:	\
			: "r"(i)	\
			: "memory");	\
	asm volatile ("mcr p15, 0, %0, c8, c7, 0;"	\
			:	\
			: "r"(i)	\
			: "memory"); /* Invalidate i+d-TLBs */	\
}

#define dcache_disable_l1()	\
{	\
	int i = 0;	\
	asm volatile ("mcr p15, 0, %0, c7, c6, 0;"	\
			:	\
			: "r"(i)); /* clear data cache */	\
	asm volatile ("mrc p15, 0, %0, c1, c0, 0;"	\
			: "=r"(i));	\
	i &= (~0x0004);	/* disable DCache */	\
			/* but not MMU and alignment faults */     \
	asm volatile ("mcr p15, 0, %0, c1, c0, 0;"	\
			:	\
			: "r"(i));	\
}

#define icache_invalidate_all_l1()        \
{	\
	/* this macro can discard dirty cache lines (N/A for ICache) */	\
	int i = 0;	\
	asm volatile ("mcr p15, 0, %0, c7, c5, 0;"	\
			:	\
			: "r"(i)); /* flush ICache */	\
	asm volatile ("mcr p15, 0, %0, c8, c5, 0;"	\
			:	\
			: "r"(i)); /* flush ITLB only */	\
	asm volatile ("mcr p15, 0, %0, c7, c5, 4;"	\
			:	\
			: "r"(i)); /* flush prefetch buffer */	\
	asm (	\
	"nop;" /* next few instructions may be via cache */	\
	"nop;"	\
	"nop;"	\
	"nop;"	\
	"nop;"	\
	"nop;");	\
}

#define cache_flush()	\
{	\
	dcache_invalidate_all_l1();	\
	icache_invalidate_all_l1();	\
}

int cleanup_before_linux (void)
{
	/*
	 * this function is called just before we call linux
	 * it prepares the processor for linux
	 *
	 * we turn off caches etc ...
	 */

	disable_interrupts ();

#ifdef CONFIG_LCD
	{
		extern void lcd_disable(void);
		extern void lcd_panel_disable(void);

		lcd_disable(); /* proper disable of lcd & panel */
		lcd_panel_disable();
	}
#endif
	/* flush I/D-cache */
	cache_flush();

	/* turn off I/D-cache */
	icache_disable();
	dcache_disable();

	/* MMU Off */
	MMU_OFF();

/*Workaround to enable L2CC during kernel decompressing*/
#ifdef fixup_before_linux
	fixup_before_linux;
#endif
	return 0;
}

