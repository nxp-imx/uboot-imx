/*
 * Copyright (C) 2008 Embedded Alley Solutions Inc.
 *
 * (C) Copyright 2009-2010 Freescale Semiconductor, Inc.
 *
 * Timers and rotary encoder register definitions
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
#ifndef TIMROT_H
#define TIMROT_H

#include <asm/arch/mx23.h>

#define TIMROT_BASE	(MX23_REGS_BASE + 0x00068000)

/* Timer and rotary encoder register offsets */
#define ROTCTRL		0x0
#define ROTCOUNT	0x10
#define TIMCTRL0	0x20
#define TIMCOUNT0	0x30
#define TIMCTRL1	0x40
#define TIMCOUNT1	0x50
#define TIMCTRL2	0x60
#define TIMCOUNT2	0x70
#define TIMCTRL3	0x80
#define TIMCTRL3	0x90

/* TIMCTRL bits, bit fields and values */
#define TIMCTRL_SELECT		0
#define TIMCTRL_PRESCALE	4
#define TIMCTRL_RELOAD		(1 << 6)
#define TIMCTRL_UPDATE		(1 << 7)
#define TIMCTRL_POLARITY	(1 << 8)
#define TIMCTRL_IRQEN		(1 << 14)
#define TIMCTRL_IRQ		(1 << 15)

#define TIMCTRL_SELECT_PWM0	(0x1 << TIMCTRL_SELECT)
#define TIMCTRL_SELECT_PWM1	(0x2 << TIMCTRL_SELECT)
#define TIMCTRL_SELECT_PWM2	(0x3 << TIMCTRL_SELECT)
#define TIMCTRL_SELECT_PWM3	(0x4 << TIMCTRL_SELECT)
#define TIMCTRL_SELECT_PWM4	(0x5 << TIMCTRL_SELECT)
#define TIMCTRL_SELECT_ROTARYA	(0x6 << TIMCTRL_SELECT)
#define TIMCTRL_SELECT_ROTARYB	(0x7 << TIMCTRL_SELECT)
#define TIMCTRL_SELECT_32KHZ	(0x8 << TIMCTRL_SELECT)
#define TIMCTRL_SELECT_8KHZ	(0x9 << TIMCTRL_SELECT)
#define TIMCTRL_SELECT_4KHZ	(0xa << TIMCTRL_SELECT)
#define TIMCTRL_SELECT_1KHZ	(0xb << TIMCTRL_SELECT)
#define TIMCTRL_SELECT_ALWAYS	(0xc << TIMCTRL_SELECT)

#endif /* TIMROT_H */
