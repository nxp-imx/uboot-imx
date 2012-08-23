/*
 * Copyright (C) 2012 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef	_IMX_WDOG_H
#define	_IMX_WDOG_H

#include <asm/io.h>

#define WDOGx_WMCR	0x8
static inline void wdog_preconfig(int base_addr)
{
	/*
	 * From IC spec:
	 * The Power down Counter inside WDOG-1 will be enabled out of reset.
	 * This counter has a fixed time-out value of 16 seconds, after which
	 * it will drive the WDOG-1 signal low.
	 *
	 * To prevent this, the software must disable this counter by clearing
	 * the PDE bit of Watchdog Miscellaneous Control Register (WDOG_WMCR).
	 */
	writew(0, base_addr + WDOGx_WMCR);
}

#endif
