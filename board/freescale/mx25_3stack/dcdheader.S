/*
 *  Copyright (c) 2009  Freescale Semiconductor
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


#include <config.h>
#include <version.h>

.extern reset

#define DCDGEN(i,type, addr, data) \
dcd_##i:			;\
    .long type			;\
    .long addr			;\
    .long data

.globl _initheader
_initheader:
	b	reset
	.org 0x400
app_code_jump_v:	.long reset
app_code_barker:	.long 0xB1
app_code_csf:		.long 0
hwcfg_ptr_ptr:		.long hwcfg_ptr
super_root_key:		.long 0
hwcfg_ptr:		.long dcd_data
app_dest_ptr:		.long TEXT_BASE
dcd_data:		.long 0xB17219E9

#ifdef MXC_MEMORY_MDDR
dcd_len:		.long 12*15
#else
dcd_len:		.long 12*24
#endif

/* WEIM config-CS5 init -- CPLD */
DCDGEN( 1, 4, 0xB8002050, 0x0000D843) /* CS5_CSCRU */
DCDGEN( 2, 4, 0xB8002054, 0x22252521) /* CS5_CSCRL */
DCDGEN( 3, 4, 0xB8002058, 0x22220A00) /* CS5_CSCRA */
#ifdef MXC_MEMORY_MDDR
/* MDDR init */
DCDGEN( 4, 4, 0xB8001010, 0x00000004) /* enable mDDR */
DCDGEN( 5, 4, 0xB8001000, 0x92100000) /* precharge command */
DCDGEN( 6, 1, 0x80000400, 0x12344321) /* precharge all dummy write */
DCDGEN( 7, 4, 0xB8001000, 0xA2100000) /* auto-refresh command */
DCDGEN( 8, 4, 0x80000000, 0x12344321) /* dummy write for refresh */
DCDGEN( 9, 4, 0x80000000, 0x12344321) /* dummy write for refresh */
DCDGEN(10, 4, 0xB8001000, 0xB2100000) /* Load Mode Reg command - cas=3 bl=8 */
DCDGEN(11, 1, 0x80000033, 0xda)	/* dummy write -- address has the mode bits */
DCDGEN(12, 1, 0x81000000, 0xff) /* dummy write -- address has the mode bits */
DCDGEN(13, 4, 0xB8001000, 0x82216880)
DCDGEN(14, 4, 0xB8001004, 0x00295729)
#else
/* DDR2 init */
DCDGEN( 4, 4, 0xB8001004, 0x0076E83A)	/* initial value for ESDCFG0 */
DCDGEN( 5, 4, 0xB8001010, 0x00000204)	/* ESD_MISC */
DCDGEN( 6, 4, 0xB8001000, 0x92210000)	/* CS0 precharge command */
DCDGEN( 7, 4, 0x80000f00, 0x12344321)	/* precharge all dummy write */
DCDGEN( 8, 4, 0xB8001000, 0xB2210000)	/* Load Mode Register command */
DCDGEN( 9, 1, 0x82000000, 0xda)		/* dummy write Load EMR2 */
DCDGEN(10, 1, 0x83000000, 0xda)		/* dummy write Load EMR3 */
DCDGEN(11, 1, 0x81000400, 0xda)		/* dummy write Load EMR1; enable DLL */
DCDGEN(12, 1, 0x80000333, 0xda)		/* dummy write Load MR; reset DLL */

DCDGEN(13, 4, 0xB8001000, 0x92210000)	/* CS0 precharge command */
DCDGEN(14, 1, 0x80000400, 0x12345678)	/* precharge all dummy write */

DCDGEN(15, 4, 0xB8001000, 0xA2210000)	/* select manual refresh mode */
DCDGEN(16, 4, 0x80000000, 0x87654321)	/* manual refresh */
DCDGEN(17, 4, 0x80000000, 0x87654321)	/* manual refresh twice */

DCDGEN(18, 4, 0xB8001000, 0xB2210000)	/* Load Mode Register command */
DCDGEN(19, 1, 0x80000233, 0xda)		/* Load MR; CL=3, BL=8, end DLL reset */
DCDGEN(20, 1, 0x81000780, 0xda)		/* Load EMR1; OCD default */
DCDGEN(21, 1, 0x81000400, 0xda)		/* Load EMR1; OCD exit */
DCDGEN(22, 4, 0xB8001000, 0x82216080)	/* normal mode */
DCDGEN(23, 4, 0x43FAC454, 0x00001000)	/* IOMUXC_SW_PAD_CTL_GRP_DDRTYPE(1-5) */
#endif

DCDGEN(99, 4, 0x53F80008, 0x20034000) /* CLKCTL ARM=400 AHB=133 */
card_cfg:	.long UBOOT_IMAGE_SIZE
