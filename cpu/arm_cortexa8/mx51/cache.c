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
#include <asm/cache.h>

void l2_cache_enable(void)
{
	asm("mrc 15, 0, r0, c1, c0, 1");
	asm("orr r0, r0, #0x2");
	asm("mcr 15, 0, r0, c1, c0, 1");
}

void l2_cache_disable(void)
{
	asm("mrc 15, 0, r0, c1, c0, 1");
	asm("bic r0, r0, #0x2");
	asm("mcr 15, 0, r0, c1, c0, 1");
}

/*dummy function for L2 ON*/
u32 get_device_type(void)
{
	return 0;
}
