/*
 * Copyright (C) 2004-2010 Freescale Semiconductor, Inc.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
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
