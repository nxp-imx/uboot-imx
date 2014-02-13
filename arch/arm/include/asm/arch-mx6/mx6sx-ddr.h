/*
 * Copyright (C) 2014 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#ifndef __ASM_ARCH_MX6SX_DDR_H__
#define __ASM_ARCH_MX6SX_DDR_H__

#ifndef CONFIG_MX6SX
#error "wrong CPU"
#endif

#define MX6_IOM_DRAM_DQM0	0x020e02ec
#define MX6_IOM_DRAM_DQM1	0x020e02f0
#define MX6_IOM_DRAM_DQM2	0x020e02f4
#define MX6_IOM_DRAM_DQM3	0x020e02f8

#define MX6_IOM_DRAM_RAS	0x020e02fc
#define MX6_IOM_DRAM_CAS	0x020e0300
#define MX6_IOM_DRAM_SDODT0	0x020e0310
#define MX6_IOM_DRAM_SDODT1	0x020e0314
#define MX6_IOM_DRAM_SDBA2	0x020e0320
#define MX6_IOM_DRAM_SDCKE0	0x020e0324
#define MX6_IOM_DRAM_SDCKE1	0x020e0328
#define MX6_IOM_DRAM_SDCLK_0	0x020e032c
#define MX6_IOM_DRAM_RESET	0x020e0340

#define MX6_IOM_DRAM_SDQS0	0x020e0330
#define MX6_IOM_DRAM_SDQS1	0x020e0334
#define MX6_IOM_DRAM_SDQS2	0x020e0338
#define MX6_IOM_DRAM_SDQS3	0x020e033c

#define MX6_IOM_GRP_ADDDS	0x020e05f4
#define MX6_IOM_DDRMODE_CTL	0x020e05f8
#define MX6_IOM_GRP_DDRPKE	0x020e05fc
#define MX6_IOM_GRP_DDRMODE	0x020e0608
#define MX6_IOM_GRP_B0DS	0x020e060c
#define MX6_IOM_GRP_B1DS	0x020e0610
#define MX6_IOM_GRP_CTLDS	0x020e0614
#define MX6_IOM_GRP_DDR_TYPE	0x020e0618
#define MX6_IOM_GRP_B2DS	0x020e061c
#define MX6_IOM_GRP_B3DS	0x020e0620

#endif	/*__ASM_ARCH_MX6SX_DDR_H__ */
