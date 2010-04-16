/*
 * Copyright (C) 2008 Embedded Alley Solutions Inc.
 *
 * (C) Copyright 2009-2010 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef MX23_H
#define MX23_H

/*
 * Most of 378x SoC registers are associated with four addresses
 * used for different operations - read/write, set, clear and toggle bits.
 *
 * Some of registers do not implement such feature and, thus, should be
 * accessed/manipulated via single address in common way.
 */
#define REG_RD(x)	(*(volatile unsigned int *)(x))
#define REG_WR(x, v)	((*(volatile unsigned int *)(x)) = (v))
#define REG_SET(x, v)	((*(volatile unsigned int *)((x) + 0x04)) = (v))
#define REG_CLR(x, v)	((*(volatile unsigned int *)((x) + 0x08)) = (v))
#define REG_TOG(x, v)	((*(volatile unsigned int *)((x) + 0x0c)) = (v))

#define MX23_OCRAM_BASE	0x00000000
#define MX23_SDRAM_BASE	0x40000000
#define MX23_REGS_BASE	0x80000000

#endif /* STMP378X_H */
