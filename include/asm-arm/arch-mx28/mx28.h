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
#ifndef __MX28_H
#define __MX28_H

#ifndef __ASSEMBLER__
enum mxc_clock {
	MXC_ARM_CLK = 0,
	MXC_AHB_CLK,
	MXC_IPG_CLK,
	MXC_GPMI_CLK,
};

unsigned int mxc_get_clock(enum mxc_clock clk);
void enet_board_init(void);
#endif

/*
 * Most of i.MX28 SoC registers are associated with four addresses
 * used for different operations - read/write, set, clear and toggle bits.
 *
 * Some of registers do not implement such feature and, thus, should be
 * accessed/manipulated via single address in common way.
 */
#define REG_RD(base, reg) \
	(*(volatile unsigned int *)((base) + (reg)))
#define REG_WR(base, reg, value) \
	((*(volatile unsigned int *)((base) + (reg))) = (value))
#define REG_SET(base, reg, value) \
	((*(volatile unsigned int *)((base) + (reg ## _SET))) = (value))
#define REG_CLR(base, reg, value) \
	((*(volatile unsigned int *)((base) + (reg ## _CLR))) = (value))
#define REG_TOG(base, reg, value) \
	((*(volatile unsigned int *)((base) + (reg ## _TOG))) = (value))

#define REG_RD_ADDR(addr) \
	(*(volatile unsigned int *)((addr)))
#define REG_WR_ADDR(addr, value) \
	((*(volatile unsigned int *)((addr))) = (value))
#define REG_SET_ADDR(addr, value) \
	((*(volatile unsigned int *)((addr) + 0x4)) = (value))
#define REG_CLR_ADDR(addr, value) \
	((*(volatile unsigned int *)((addr) + 0x8)) = (value))
#define REG_TOG_ADDR(addr, value) \
	((*(volatile unsigned int *)((addr) + 0xc)) = (value))

/*
 * Register base address
 */
#define REGS_ICOL_BASE		(0x80000000)
#define REGS_HSADC_BASE		(0x80002000)
#define REGS_APBH_BASE		(0x80004000)
#define REGS_PERFMON_BASE	(0x80006000)
#define REGS_BCH_BASE		(0x8000A000)
#define REGS_GPMI_BASE		(0x8000C000)
#define REGS_SSP0_BASE		(0x80010000)
#define REGS_SSP1_BASE		(0x80012000)
#define REGS_SSP2_BASE		(0x80014000)
#define REGS_SSP3_BASE		(0x80016000)
#define REGS_PINCTRL_BASE	(0x80018000)
#define REGS_DIGCTL_BASE	(0x8001C000)
#define REGS_ETM_BASE		(0x80022000)
#define REGS_APBX_BASE		(0x80024000)
#define REGS_DCP_BASE		(0x80028000)
#define REGS_PXP_BASE		(0x8002A000)
#define REGS_OCOTP_BASE		(0x8002C000)
#define REGS_AXI_AHB0_BASE	(0x8002E000)
#define REGS_LCDIF_BASE		(0x80030000)
#define REGS_CAN0_BASE		(0x80032000)
#define REGS_CAN1_BASE		(0x80034000)
#define REGS_SIMDBG_BASE	(0x8003C000)
#define REGS_SIMGPMISEL_BASE	(0x8003C200)
#define REGS_SIMSSPSEL_BASE	(0x8003C300)
#define REGS_SIMMEMSEL_BASE	(0x8003C400)
#define REGS_GPIOMON_BASE	(0x8003C500)
#define REGS_SIMENET_BASE	(0x8003C700)
#define REGS_ARMJTAG_BASE	(0x8003C800)
#define REGS_CLKCTRL_BASE	(0x80040000)
#define REGS_SAIF0_BASE		(0x80042000)
#define REGS_POWER_BASE		(0x80044000)
#define REGS_SAIF1_BASE		(0x80046000)
#define REGS_LRADC_BASE		(0x80050000)
#define REGS_SPDIF_BASE		(0x80054000)
#define REGS_RTC_BASE		(0x80056000)
#define REGS_I2C0_BASE		(0x80058000)
#define REGS_I2C1_BASE		(0x8005A000)
#define REGS_PWM_BASE		(0x80064000)
#define REGS_TIMROT_BASE	(0x80068000)
#define REGS_UARTAPP0_BASE	(0x8006A000)
#define REGS_UARTAPP1_BASE	(0x8006C000)
#define REGS_UARTAPP2_BASE	(0x8006E000)
#define REGS_UARTAPP3_BASE	(0x80070000)
#define REGS_UARTAPP4_BASE	(0x80072000)
#define REGS_UARTDBG_BASE	(0x80074000)
#define REGS_USBPHY0_BASE	(0x8007C000)
#define REGS_USBPHY1_BASE	(0x8007E000)
#define REGS_USBCTRL0_BASE	(0x80080000)
#define REGS_USBCTRL1_BASE	(0x80090000)
#define REGS_DFLPT_BASE		(0x800C0000)
#define REGS_DRAM_BASE		(0x800E0000)
#define REGS_ENET_BASE		(0x800F0000)

#define BCH_BASE_ADDR REGS_BCH_BASE
#define GPMI_BASE_ADDR REGS_GPMI_BASE
#define ABPHDMA_BASE_ADDR  REGS_APBH_BASE
#endif /* __MX28_H */
