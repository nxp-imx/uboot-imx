/*
 * (C) Copyright 2008 Texas Insturments
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * (C) Copyright 2002
 * Gary Jennejohn, DENX Software Engineering, <garyj@denx.de>
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
#include <asm/cache.h>
#include <asm/cache-cp15.h>
#include <asm/mmu.h>

#ifndef CONFIG_L2_OFF
#ifndef CONFIG_MXC
#include <asm/arch/sys_proto.h>
#endif
#endif

#define cache_flush(void)	\
{	\
	asm volatile (	\
		"stmfd sp!, {r0-r5, r7, r9-r11};"	\
		"mrc        p15, 1, r0, c0, c0, 1;" /*@ read clidr*/	\
		/* @ extract loc from clidr */       \
		"ands       r3, r0, #0x7000000;"	\
		/* @ left align loc bit field*/      \
		"mov        r3, r3, lsr #23;"	\
		/* @ if loc is 0, then no need to clean*/    \
		"beq        555f;" /* finished;" */	\
		/* @ start clean at cache level 0*/  \
		"mov        r10, #0;"	\
		"111:" /*"loop1: */	\
		/* @ work out 3x current cache level */	\
		"add        r2, r10, r10, lsr #1;"	\
		/* @ extract cache type bits from clidr */    \
		"mov        r1, r0, lsr r2;"	\
		/* @ mask of the bits for current cache only */	\
		"and        r1, r1, #7;"	\
		/* @ see what cache we have at this level*/  \
		"cmp        r1, #2;"	\
		/* @ skip if no cache, or just i-cache*/	\
		"blt        444f;" /* skip;" */	\
		/* @ select current cache level in cssr*/   \
		"mcr        p15, 2, r10, c0, c0, 0;"	\
		/* @ isb to sych the new cssr&csidr */	\
		"mcr        p15, 0, r10, c7, c5, 4;"	\
		/* @ read the new csidr */    \
		"mrc        p15, 1, r1, c0, c0, 0;"	\
		/* @ extract the length of the cache lines */ \
		"and        r2, r1, #7;"	\
		/* @ add 4 (line length offset) */   \
		"add        r2, r2, #4;"	\
		"ldr        r4, =0x3ff;"	\
		/* @ find maximum number on the way size*/   \
		"ands       r4, r4, r1, lsr #3;"	\
		/*"clz  r5, r4;" @ find bit position of way size increment*/ \
		".word 0xE16F5F14;"	\
		"ldr        r7, =0x7fff;"	\
		/* @ extract max number of the index size*/  \
		"ands       r7, r7, r1, lsr #13;"	\
		"222:" /* loop2:"  */	\
		/* @ create working copy of max way size*/   \
		"mov        r9, r4;"	\
		"333:" /* loop3:"  */	\
		/* @ factor way and cache number into r11*/  \
		"orr        r11, r10, r9, lsl r5;"	\
		/* @ factor index number into r11*/  \
		"orr        r11, r11, r7, lsl r2;"	\
		/* @ clean & invalidate by set/way */	\
		"mcr        p15, 0, r11, c7, c14, 2;"	\
		/* @ decrement the way */	\
		"subs       r9, r9, #1;"	\
		"bge        333b;" /* loop3;" */	\
		/* @ decrement the index */	\
		"subs       r7, r7, #1;"	\
		"bge        222b;" /* loop2;" */	\
		"444:" /* skip: */	\
		/*@ increment cache number */	\
		"add        r10, r10, #2;"	\
		"cmp        r3, r10;" 	\
		"bgt        111b;" /* loop1; */	\
		"555:" /* "finished:" */	\
		/* @ swith back to cache level 0 */	\
		"mov        r10, #0;"	\
		/* @ select current cache level in cssr */	\
		"mcr        p15, 2, r10, c0, c0, 0;"	\
		/* @ isb to sych the new cssr&csidr */	\
		"mcr        p15, 0, r10, c7, c5, 4;" 	\
		"ldmfd 	    sp!, {r0-r5, r7, r9-r11};"	\
		"666:" /* iflush:" */	\
		"mov        r0, #0x0;"	\
		/* @ invalidate I+BTB */	\
		"mcr        p15, 0, r0, c7, c5, 0;" 	\
		/* @ drain WB */	\
		"mcr        p15, 0, r0, c7, c10, 4;"	\
		:	\
		:	\
		: "r0" /* Clobber list */	\
	);	\
}

int cleanup_before_linux(void)
{
	unsigned int i;

	/*
	 * this function is called just before we call linux
	 * it prepares the processor for linux
	 *
	 * we turn off caches etc ...
	 */
	disable_interrupts();

	/* flush cache */
	cache_flush();

	/* turn off I/D-cache */
	icache_disable();
	/* invalidate D-cache */
	dcache_disable();

#ifndef CONFIG_L2_OFF
	/* turn off L2 cache */
	l2_cache_disable();
	/* invalidate L2 cache also */
	v7_flush_dcache_all(get_device_type());
#endif
	i = 0;
	/* mem barrier to sync up things */
	asm("mcr p15, 0, %0, c7, c10, 4": :"r"(i));

	/* turn off MMU */
	MMU_OFF();

#ifndef CONFIG_L2_OFF
	l2_cache_enable();
#endif

	return 0;
}

