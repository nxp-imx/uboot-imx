/*
 *
 * (c) 2007 Pengutronix, Sascha Hauer <s.hauer@pengutronix.de>
 *
 * (C) Copyright 2008-2009 Freescale Semiconductor, Inc.
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

#ifndef __ASM_ARCH_MX35_H
#define __ASM_ARCH_MX35_H

#define __REG(x)     (*((volatile u32 *)(x)))
#define __REG16(x)   (*((volatile u16 *)(x)))
#define __REG8(x)    (*((volatile u8 *)(x)))

#define L2CC_BASE_ADDR	0x30000000

/*
 * AIPS 1
 */
#define AIPS1_BASE_ADDR         0x43F00000
#define AIPS1_CTRL_BASE_ADDR    AIPS1_BASE_ADDR
#define MAX_BASE_ADDR           0x43F04000
#define EVTMON_BASE_ADDR        0x43F08000
#define CLKCTL_BASE_ADDR        0x43F0C000
#define I2C_BASE_ADDR           0x43F80000
#define I2C3_BASE_ADDR          0x43F84000
#define ATA_BASE_ADDR           0x43F8C000
#define UART1_BASE_ADDR         0x43F90000
#define UART2_BASE_ADDR         0x43F94000
#define I2C2_BASE_ADDR          0x43F98000
#define CSPI1_BASE_ADDR         0x43FA4000
#define IOMUXC_BASE_ADDR        0x43FAC000

/*
 * SPBA
 */
#define SPBA_BASE_ADDR          0x50000000
#define UART3_BASE_ADDR         0x5000C000
#define CSPI2_BASE_ADDR         0x50010000
#define ATA_DMA_BASE_ADDR       0x50020000
#define FEC_BASE_ADDR           0x50038000
#define SPBA_CTRL_BASE_ADDR     0x5003C000

/*
 * AIPS 2
 */
#define AIPS2_BASE_ADDR         0x53F00000
#define AIPS2_CTRL_BASE_ADDR    AIPS2_BASE_ADDR
#define CCM_BASE_ADDR           0x53F80000
#define GPT1_BASE_ADDR          0x53F90000
#define EPIT1_BASE_ADDR         0x53F94000
#define EPIT2_BASE_ADDR         0x53F98000
#define GPIO3_BASE_ADDR         0x53FA4000
#define IPU_CTRL_BASE_ADDR      0x53FC0000
#define GPIO3_BASE_ADDR         0x53FA4000
#define GPIO1_BASE_ADDR         0x53FCC000
#define GPIO2_BASE_ADDR         0x53FD0000
#define SDMA_BASE_ADDR          0x53FD4000
#define RTC_BASE_ADDR           0x53FD8000
#define WDOG_BASE_ADDR          0x53FDC000
#define PWM_BASE_ADDR           0x53FE0000
#define RTIC_BASE_ADDR          0x53FEC000
#define IIM_BASE_ADDR           0x53FF0000

/*
 * ROMPATCH and AVIC
 */
#define ROMPATCH_BASE_ADDR      0x60000000
#define AVIC_BASE_ADDR          0x68000000

/*
 * NAND, SDRAM, WEIM, M3IF, EMI controllers
 */
#define EXT_MEM_CTRL_BASE       0xB8000000
#define ESDCTL_BASE_ADDR        0xB8001000
#define WEIM_BASE_ADDR          0xB8002000
#define WEIM_CTRL_CS0           WEIM_BASE_ADDR
#define WEIM_CTRL_CS1           (WEIM_BASE_ADDR + 0x10)
#define WEIM_CTRL_CS2           (WEIM_BASE_ADDR + 0x20)
#define WEIM_CTRL_CS3           (WEIM_BASE_ADDR + 0x30)
#define WEIM_CTRL_CS4           (WEIM_BASE_ADDR + 0x40)
#define WEIM_CTRL_CS5           (WEIM_BASE_ADDR + 0x50)
#define M3IF_BASE_ADDR		0xB8003000
#define EMI_BASE_ADDR		0xB8004000

#define NFC_BASE_ADDR		0xBB000000

/*
 * Memory regions and CS
 */
#define IPU_MEM_BASE_ADDR	0x70000000
#define CSD0_BASE_ADDR	0x80000000
#define CSD1_BASE_ADDR	0x90000000
#define CS0_BASE_ADDR	0xA0000000
#define CS1_BASE_ADDR	0xA8000000
#define CS2_BASE_ADDR	0xB0000000
#define CS3_BASE_ADDR	0xB2000000
#define CS4_BASE_ADDR	0xB4000000
#define CS5_BASE_ADDR	0xB6000000

/*
 * IRQ Controller Register Definitions.
 */
#define AVIC_NIMASK   	0x04
#define AVIC_INTTYPEH   0x18
#define AVIC_INTTYPEL   0x1C

/* L210 */
#define L2CC_BASE_ADDR                  0x30000000
#define L2_CACHE_LINE_SIZE              32
#define L2_CACHE_CTL_REG                0x100
#define L2_CACHE_AUX_CTL_REG            0x104
#define L2_CACHE_SYNC_REG               0x730
#define L2_CACHE_INV_LINE_REG           0x770
#define L2_CACHE_INV_WAY_REG            0x77C
#define L2_CACHE_CLEAN_LINE_REG         0x7B0
#define L2_CACHE_CLEAN_INV_LINE_REG     0x7F0
#define L2_CACHE_DBG_CTL_REG            0xF40

/* CCM */
#define CLKCTL_CCMR                     0x00
#define CLKCTL_PDR0                     0x04
#define CLKCTL_PDR1                     0x08
#define CLKCTL_PDR2                     0x0C
#define CLKCTL_PDR3                     0x10
#define CLKCTL_PDR4                     0x14
#define CLKCTL_RCSR                     0x18
#define CLKCTL_MPCTL                    0x1C
#define CLKCTL_PPCTL                    0x20
#define CLKCTL_ACMR                     0x24
#define CLKCTL_COSR                     0x28
#define CLKCTL_CGR0                     0x2C
#define CLKCTL_CGR1                     0x30
#define CLKCTL_CGR2                     0x34
#define CLKCTL_CGR3                     0x38

#define CLKMODE_AUTO            0
#define CLKMODE_CONSUMER        1

#define PLL_PD(x)		(((x) & 0xf) << 26)
#define PLL_MFD(x)		(((x) & 0x3ff) << 16)
#define PLL_MFI(x)		(((x) & 0xf) << 10)
#define PLL_MFN(x)		(((x) & 0x3ff) << 0)

#define CSCR_U(x)	(WEIM_CTRL_CS#x + 0)
#define CSCR_L(x)	(WEIM_CTRL_CS#x + 4)
#define CSCR_A(x)	(WEIM_CTRL_CS#x + 8)

#define IPU_CONF	IPU_CTRL_BASE_ADDR

#define IPU_CONF_PXL_ENDIAN	(1<<8)
#define IPU_CONF_DU_EN		(1<<7)
#define IPU_CONF_DI_EN		(1<<6)
#define IPU_CONF_ADC_EN		(1<<5)
#define IPU_CONF_SDC_EN		(1<<4)
#define IPU_CONF_PF_EN		(1<<3)
#define IPU_CONF_ROT_EN		(1<<2)
#define IPU_CONF_IC_EN		(1<<1)
#define IPU_CONF_SCI_EN		(1<<0)

#define GPIO_PORT_NUM	3
#define GPIO_NUM_PIN	32

#ifndef __ASSEMBLER__

enum mxc_clock {
MXC_ARM_CLK = 0,
MXC_AHB_CLK,
MXC_IPG_CLK,
MXC_IPG_PERCLK,
MXC_UART_CLK,
};

/*!
 * NFMS bit in RCSR register for pagesize of nandflash
 */
#define NFMS            (*((volatile u32 *)(CCM_BASE_ADDR+0x18)))
#define NFMS_BIT                8
#define NFMS_NF_DWIDTH          14
#define NFMS_NF_PG_SZ           8

extern unsigned int mxc_get_clock(enum mxc_clock clk);

#define fixup_before_linux	\
	{		\
		volatile unsigned long *l2cc_ctl = (unsigned long *)0x30000100;\
		*l2cc_ctl = 1;\
	}
#endif /* __ASSEMBLER__*/
#endif /* __ASM_ARCH_MX35_H */
