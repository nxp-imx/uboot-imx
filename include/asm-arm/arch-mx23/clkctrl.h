/*
 * Copyright (C) 2008 Embedded Alley Solutions Inc.
 *
 * (C) Copyright 2009-2010 Freescale Semiconductor, Inc.
 *
 * Clock control register descriptions
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
#ifndef CLKCTRL_H
#define CLKCTRL_H

#include <asm/arch/mx23.h>

#define CLKCTRL_BASE	(MX23_REGS_BASE + 0x40000)

#define	CLKCTRL_PLLCTRL0	0x000
#define	CLKCTRL_PLLCTRL1	0x010
#define	CLKCTRL_CPU		0x020
#define CLKCTRL_HBUS		0x030
#define	CLKCTRL_XBUS		0x040
#define	CLKCTRL_XTAL		0x050
#define CLKCTRL_PIX		0x060
#define	CLKCTRL_SSP		0x070
#define	CLKCTRL_GPMI		0x080
#define CLKCTRL_SPDIF		0x090
#define	CLKCTRL_EMI		0x0a0
#define	CLKCTRL_IR		0x0b0
#define CLKCTRL_SAIF		0x0c0
#define	CLKCTRL_TV		0x0d0
#define	CLKCTRL_ETM		0x0e0
#define	CLKCTRL_FRAC		0x0f0
#define	CLKCTRL_FRAC1		0x100
#define CLKCTRL_CLKSEQ		0x110
#define	CLKCTRL_RESET		0x120
#define	CLKCTRL_STATUS		0x130
#define CLKCTRL_VERSION		0x140

/* CLKCTRL_SSP register bits, bit fields and values */
#define	SSP_CLKGATE	(1 << 31)
#define	SSP_BUSY	(1 << 29)
#define	SSP_DIV_FRAC_EN	(1 << 9)
#define SSP_DIV		0

/* CLKCTRL_FRAC register bits, bit fields and values */
#define	FRAC_CLKGATEIO	(1 << 31)
#define FRAC_IOFRAC	24

/* CLKCTRL_FRAC register bits, bit fields and values */
#define CLKSEQ_BYPASS_SSP	(1 << 5)

#endif /* CLKCTRL_H */
