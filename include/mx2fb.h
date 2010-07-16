/*
 * Copyright (C) 2004-2010 Freescale Semiconductor, Inc.
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

/*!
 * @file mx2fb.h
 *
 * @brief MX 25 LCD controller header file.
 *
 *
 */

#ifndef __MX2FB_H__
#define __MX2FB_H__


/* LCDC register settings */

#define LCDC_LSCR 0x00120300

#define LCDC_LRMCR 0x00000000

#define LCDC_LDCR 0x00020010

#define LCDC_LPCCR 0x00a9037f

#define LCDC_LPCR 0xFA008B80

#define LCDC_LPCR_PCD 0x4

#define FB_SYNC_OE_LOW_ACT	0x80000000
#define FB_SYNC_CLK_LAT_FALL	0x40000000
#define FB_SYNC_DATA_INVERT	0x20000000
#define FB_SYNC_CLK_IDLE_EN	0x10000000
#define FB_SYNC_SHARP_MODE	0x08000000
#define FB_SYNC_SWAP_RGB	0x04000000

#endif				/* __MX2FB_H__ */
