/* Copyright 2009-2010 Freescale Semiconductor, Inc.
 *
 * On-Chip OTP register descriptions
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
#ifndef OCOTP_H
#define OCOTP_H

#include <asm/arch/mx23.h>

#define OCOTP_BASE	(MX23_REGS_BASE + 0x2c000)

#define	OCOTP_CTRL		0x000
#define	OCOTP_CTRL_SET		0x004
#define	OCOTP_CTRL_CLR		0x008
#define	OCOTP_CTRL_TOG		0x00c
#define	OCOTP_DATA		0x010
#define	OCOTP_CUST0		0x020
#define	OCOTP_CUST1		0x030
#define	OCOTP_CUST2		0x040
#define	OCOTP_CUST3		0x050
#define	OCOTP_CRYPTO1		0x070
#define	OCOTP_CRYPTO2		0x080
#define	OCOTP_CRYPTO3		0x090
#define	OCOTP_HWCAP0		0x0a0
#define	OCOTP_HWCAP1		0x0b0
#define	OCOTP_HWCAP2		0x0c0
#define	OCOTP_HWCAP3		0x0d0
#define	OCOTP_HWCAP4		0x0e0
#define	OCOTP_HWCAP5		0x0f0
#define	OCOTP_SWCAP		0x100
#define	OCOTP_CUSTCAP		0x110
#define	OCOTP_LOCK		0x120
#define	OCOTP_OPS0		0x130
#define	OCOTP_OPS1		0x140
#define	OCOTP_OPS2		0x150
#define	OCOTP_OPS3		0x160
#define	OCOTP_UN0		0x170
#define	OCOTP_UN1		0x180
#define	OCOTP_UN2		0x190
#define	OCOTP_ROM0		0x1a0
#define	OCOTP_ROM1		0x1b0
#define	OCOTP_ROM2		0x1c0
#define	OCOTP_ROM3		0x1d0
#define	OCOTP_ROM4		0x1e0
#define	OCOTP_ROM5		0x1f0
#define	OCOTP_ROM6		0x200
#define	OCOTP_ROM7		0x210
#define	OCOTP_VERSION		0x220


/* OCOTP_CTRL register bits, bit fields and values */
#define CTRL_RD_BANK_OPEN	(1 << 12)
#define CTRL_BUSY		(8 << 12)

#endif /* OCOTP_H */
