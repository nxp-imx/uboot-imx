/*
 * Copyright 2009 Freescale Semiconductor, Inc.
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
#include <asm/arch/mx35.h>
#include "board-mx35_3stack.h"

#ifdef	CONFIG_FLASH_HEADER
#ifndef CONFIG_FLASH_HEADER_OFFSET
# error "Must define the offset of flash header"
#endif

/* Flash header setup */
#define DCDGEN(i,type, addr, data) \
dcd_##i:                         ;\
    .long type                   ;\
    .long addr                   ;\
    .long data

#define GEN_FHEADERADDR(x) (x)

.section ".text.flasheader", "x"
	b _start
	.org CONFIG_FLASH_HEADER_OFFSET
app_code_jump_v:    .long GEN_FHEADERADDR(_start)
app_code_barker:    .long CONFIG_FLASH_HEADER_BARKER
app_code_csf:       .long 0
hwcfg_ptr_ptr:      .long GEN_FHEADERADDR(hwcfg_ptr)
super_root_key:     .long 0
hwcfg_ptr:          .long GEN_FHEADERADDR(dcd_data)
app_dest_ptr:       .long TEXT_BASE
dcd_data:           .long 0xB17219E9
#ifdef MEMORY_MDDR_ENABLE
                    .long (dcd_data_end - dcd_data - 8)

//WEIM config-CS5 init
DCDGEN(1, 4, 0xB8002054, 0x444a4541)
DCDGEN(1_1, 4, 0xB8002050, 0x0000dcf6)
DCDGEN(1_2, 4, 0xB8002058, 0x44443302)
//MDDR init
//enable mDDR
DCDGEN(2, 4, 0xB8001010, 0x00000004)
//reset delay time
DCDGEN(3, 4, 0xB8001010, 0x0000000C)
DCDGEN(4, 4, 0xB800100C, 0x007ffc3f)
DCDGEN(5, 4, 0xB800100C, 0x007ffc3f)
DCDGEN(6, 4, 0xB8001004, 0x007ffc3f)
DCDGEN(7, 4, 0xB8001000, 0x92220000)
DCDGEN(8, 1, 0x80000400, 0xda)
DCDGEN(9, 4, 0xB8001000, 0xA2220000)
DCDGEN(10, 4, 0x80000000, 0x87654321)
DCDGEN(11, 4, 0x80000000, 0x87654321)
DCDGEN(12, 4, 0xB8001000, 0xB2220000)
DCDGEN(13, 1, 0x80000033, 0xda)
DCDGEN(14, 1, 0x82000000, 0xda)
DCDGEN(15, 4, 0xB8001000, 0x82226080)
DCDGEN(16, 4, 0xB8001010, 0x00000004)
DCDGEN(17, 4, 0xB8001008, 0x00002000)

#else
                    .long 240

//WEIM config-CS5 init
DCDGEN(1, 4, 0xB8002050, 0x0000d843)
DCDGEN(1_1, 4, 0xB8002054, 0x22252521)
DCDGEN(1_2, 4, 0xB8002058, 0x22220a00)

//DDR2 init
DCDGEN(2, 4, 0xB8001010, 0x00000304)
DCDGEN(3, 4, 0xB8001010, 0x0000030C)
DCDGEN(4, 4, 0xB8001004, 0x007ffc3f)
DCDGEN(5, 4, 0xB8001000, 0x92220000)
DCDGEN(6, 4, 0x80000400, 0x12345678)
DCDGEN(7, 4, 0xB8001000, 0xA2220000)
DCDGEN(8, 4, 0x80000000, 0x87654321)
DCDGEN(9, 4, 0x80000000, 0x87654321)
DCDGEN(10, 4, 0xB8001000, 0xB2220000)
DCDGEN(11, 1, 0x80000233, 0xda)
DCDGEN(12, 1, 0x82000780, 0xda)
DCDGEN(13, 1, 0x82000400, 0xda)
DCDGEN(14, 4, 0xB8001000, 0x82226080)
DCDGEN(15, 4, 0xB8001004, 0x007ffc3f)
DCDGEN(16, 4, 0xB800100C, 0x007ffc3f)
DCDGEN(17, 4, 0xB8001010, 0x00000304)
DCDGEN(18, 4, 0xB8001008, 0x00002000)

#endif
dcd_data_end:

//CARD_FLASH_CFG_PARMS_T---length
card_cfg:           .long 0x100000
#endif
