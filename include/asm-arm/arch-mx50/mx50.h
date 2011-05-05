/*
 * Copyright (C) 2010-2011 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __ASM_ARCH_MXC_MX50_H__
#define __ASM_ARCH_MXC_MX50_H__

#define __REG(x)        (*((volatile u32 *)(x)))
#define __REG16(x)      (*((volatile u16 *)(x)))
#define __REG8(x)       (*((volatile u8 *)(x)))

 /*
 * IRAM
 */
#define IRAM_BASE_ADDR		0xF8000000	/* internal ram */
#define IRAM_PARTITIONS		16
#define IRAM_SIZE		(IRAM_PARTITIONS*SZ_8K)	/* 128KB */

#define TZIC_BASE_ADDR		0x0FFFC000
#define DATABAHN_BASE_ADDR	0x14000000

#define DEBUG_BASE_ADDR		0x40000000
#define ETB_BASE_ADDR		(DEBUG_BASE_ADDR + 0x00001000)
#define ETM_BASE_ADDR		(DEBUG_BASE_ADDR + 0x00002000)
#define TPIU_BASE_ADDR		(DEBUG_BASE_ADDR + 0x00003000)
#define CTI0_BASE_ADDR		(DEBUG_BASE_ADDR + 0x00004000)
#define CTI1_BASE_ADDR		(DEBUG_BASE_ADDR + 0x00005000)
#define CTI2_BASE_ADDR		(DEBUG_BASE_ADDR + 0x00006000)
#define CTI3_BASE_ADDR		(DEBUG_BASE_ADDR + 0x00007000)
#define CORTEX_DBG_BASE_ADDR	(DEBUG_BASE_ADDR + 0x00008000)
#define ABPHDMA_BASE_ADDR	(DEBUG_BASE_ADDR + 0x01000000)
#define OCOTP_CTRL_BASE_ADDR	(DEBUG_BASE_ADDR + 0x01002000)
#define DIGCTL_BASE_ADDR	(DEBUG_BASE_ADDR + 0x01004000)
#define GPMI_BASE_ADDR	(DEBUG_BASE_ADDR + 0x01006000)
#define BCH_BASE_ADDR		(DEBUG_BASE_ADDR + 0x01008000)
#define EPDC_BASE_ADDR		(DEBUG_BASE_ADDR + 0x01010000)

/*
 * SPBA global module enabled #0
 */
#define SPBA0_BASE_ADDR 	0x50000000

#define MMC_SDHC1_BASE_ADDR	(SPBA0_BASE_ADDR + 0x00004000)
#define MMC_SDHC2_BASE_ADDR	(SPBA0_BASE_ADDR + 0x00008000)
#define UART3_BASE_ADDR 	(SPBA0_BASE_ADDR + 0x0000C000)
#define CSPI1_BASE_ADDR 	(SPBA0_BASE_ADDR + 0x00010000)
#define SSI2_BASE_ADDR		(SPBA0_BASE_ADDR + 0x00014000)
#define MMC_SDHC3_BASE_ADDR	(SPBA0_BASE_ADDR + 0x00020000)
#define MMC_SDHC4_BASE_ADDR	(SPBA0_BASE_ADDR + 0x00024000)
#define SPBA_CTRL_BASE_ADDR	(SPBA0_BASE_ADDR + 0x0003C000)

/*
 * defines for SPBA modules
 */
#define SPBA_SDHC1	0x04
#define SPBA_SDHC2	0x08
#define SPBA_UART3	0x0C
#define SPBA_CSPI1	0x10
#define SPBA_SSI2	0x14
#define SPBA_ESAI	0x18
#define SPBA_SDHC3	0x20
#define SPBA_SDHC4	0x24
#define SPBA_SPDIF	0x28
#define SPBA_ASRC	0x2C
#define SPBA_ATA	0x30
#define SPBA_CTRL	0x3C

/*
 * AIPS 1
 */
#define AIPS1_BASE_ADDR 	0x53F00000

#define OTG_BASE_ADDR		(AIPS1_BASE_ADDR + 0x00080000)
#define GPIO1_BASE_ADDR		(AIPS1_BASE_ADDR + 0x00084000)
#define GPIO2_BASE_ADDR		(AIPS1_BASE_ADDR + 0x00088000)
#define GPIO3_BASE_ADDR		(AIPS1_BASE_ADDR + 0x0008C000)
#define GPIO4_BASE_ADDR		(AIPS1_BASE_ADDR + 0x00090000)
#define KPP_BASE_ADDR		(AIPS1_BASE_ADDR + 0x00094000)
#define WDOG1_BASE_ADDR		(AIPS1_BASE_ADDR + 0x00098000)
#define GPT1_BASE_ADDR		(AIPS1_BASE_ADDR + 0x000A0000)
#define SRTC_BASE_ADDR		(AIPS1_BASE_ADDR + 0x000A4000)
#define IOMUXC_BASE_ADDR	(AIPS1_BASE_ADDR + 0x000A8000)
#define EPIT1_BASE_ADDR		(AIPS1_BASE_ADDR + 0x000AC000)
#define PWM1_BASE_ADDR		(AIPS1_BASE_ADDR + 0x000B4000)
#define PWM2_BASE_ADDR		(AIPS1_BASE_ADDR + 0x000B8000)
#define UART1_BASE_ADDR		(AIPS1_BASE_ADDR + 0x000BC000)
#define UART2_BASE_ADDR		(AIPS1_BASE_ADDR + 0x000C0000)
#define USBOH1_BASE_ADDR        (AIPS1_BASE_ADDR + 0x000C4000)
#define SRC_BASE_ADDR		(AIPS1_BASE_ADDR + 0x000D0000)
#define CCM_BASE_ADDR		(AIPS1_BASE_ADDR + 0x000D4000)
#define GPC_BASE_ADDR		(AIPS1_BASE_ADDR + 0x000D8000)
#define GPIO5_BASE_ADDR		(AIPS1_BASE_ADDR + 0x000DC000)
#define GPIO6_BASE_ADDR		(AIPS1_BASE_ADDR + 0x000E0000)
#define GPIO7_BASE_ADDR		(AIPS1_BASE_ADDR + 0x000E4000)
#define ATA_BASE_ADDR		(AIPS1_BASE_ADDR + 0x000E8000)
#define I2C3_BASE_ADDR		(AIPS1_BASE_ADDR + 0x000EC000)
#define UART4_BASE_ADDR		(AIPS1_BASE_ADDR + 0x000F0000)
#define MSHC_BASE_ADDR		(AIPS1_BASE_ADDR + 0x000F4000)
#define RNGB_BASE_ADDR          (AIPS1_BASE_ADDR + 0x000F8000)

/*
 * AIPS 2
 */
#define AIPS2_BASE_ADDR		0x63F00000

#define PLL1_BASE_ADDR		(AIPS2_BASE_ADDR + 0x00080000)
#define PLL2_BASE_ADDR		(AIPS2_BASE_ADDR + 0x00084000)
#define PLL3_BASE_ADDR		(AIPS2_BASE_ADDR + 0x00088000)
#define UART5_BASE_ADDR		(AIPS2_BASE_ADDR + 0x00090000)
#define AHBMAX_BASE_ADDR	(AIPS2_BASE_ADDR + 0x00094000)
#define ARM_BASE_ADDR		(AIPS2_BASE_ADDR + 0x000A0000)
#define OWIRE_BASE_ADDR 	(AIPS2_BASE_ADDR + 0x000A4000)
#define CSPI2_BASE_ADDR		(AIPS2_BASE_ADDR + 0x000AC000)
#define SDMA_BASE_ADDR		(AIPS2_BASE_ADDR + 0x000B0000)
#define ROMCP_BASE_ADDR		(AIPS2_BASE_ADDR + 0x000B8000)
#define CSPI3_BASE_ADDR		(AIPS2_BASE_ADDR + 0x000C0000)
#define I2C2_BASE_ADDR		(AIPS2_BASE_ADDR + 0x000C4000)
#define I2C1_BASE_ADDR		(AIPS2_BASE_ADDR + 0x000C8000)
#define SSI1_BASE_ADDR		(AIPS2_BASE_ADDR + 0x000CC000)
#define AUDMUX_BASE_ADDR	(AIPS2_BASE_ADDR + 0x000D0000)
#define M4IF_BASE_ADDR		(AIPS2_BASE_ADDR + 0x000D8000)
#define ESDCTL_BASE_ADDR	(AIPS2_BASE_ADDR + 0x000D9000)
#define WEIM_BASE_ADDR		(AIPS2_BASE_ADDR + 0x000DA000)
#define NFC_BASE_ADDR		(AIPS2_BASE_ADDR + 0x000DB000)
#define EMI_BASE_ADDR		(AIPS2_BASE_ADDR + 0x000DBF00)
#define FEC_BASE_ADDR		(AIPS2_BASE_ADDR + 0x000EC000)

/*
 * Some of i.MX50 SoC registers are associated with four addresses
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
 * Memory regions and CS
 */
#define CSD0_BASE_ADDR		0x70000000
#define CSD1_BASE_ADDR		0xB0000000

/* gpio and gpio based interrupt handling */
#define GPIO_DR                 0x00
#define GPIO_GDIR               0x04
#define GPIO_PSR                0x08
#define GPIO_ICR1               0x0C
#define GPIO_ICR2               0x10
#define GPIO_IMR                0x14
#define GPIO_ISR                0x18
#define GPIO_INT_LOW_LEV        0x0
#define GPIO_INT_HIGH_LEV       0x1
#define GPIO_INT_RISE_EDGE      0x2
#define GPIO_INT_FALL_EDGE      0x3
#define GPIO_INT_NONE           0x4

#define CLKCTL_CCR              0x00
#define	CLKCTL_CCDR             0x04
#define CLKCTL_CSR              0x08
#define CLKCTL_CCSR             0x0C
#define CLKCTL_CACRR            0x10
#define CLKCTL_CBCDR            0x14
#define CLKCTL_CBCMR            0x18
#define CLKCTL_CSCMR1           0x1C
#define CLKCTL_CSCMR2           0x20
#define CLKCTL_CSCDR1           0x24
#define CLKCTL_CS1CDR           0x28
#define CLKCTL_CS2CDR           0x2C
#define CLKCTL_CDCDR            0x30
#define CLKCTL_CHSCDR           0x34
#define CLKCTL_CSCDR2           0x38
#define CLKCTL_CSCDR3           0x3C
#define CLKCTL_CSCDR4           0x40
#define CLKCTL_CWDR             0x44
#define CLKCTL_CDHIPR           0x48
#define CLKCTL_CDCR             0x4C
#define CLKCTL_CTOR             0x50
#define CLKCTL_CLPCR            0x54
#define CLKCTL_CISR             0x58
#define CLKCTL_CIMR             0x5C
#define CLKCTL_CCOSR            0x60
#define CLKCTL_CGPR             0x64
#define CLKCTL_CCGR0            0x68
#define CLKCTL_CCGR1            0x6C
#define CLKCTL_CCGR2            0x70
#define CLKCTL_CCGR3            0x74
#define CLKCTL_CCGR4            0x78
#define CLKCTL_CCGR5            0x7C
#define CLKCTL_CCGR6            0x80
#define CLKCTL_CCGR7            0x84
#define CLKCTL_CMEOR            0x88

#define CLKCTL_CSR2            0x8C
#define CLKCTL_CLKSEQ_BYPASS   0x90
#define CLKCTL_CLK_SYS         0x94
#define CLKCTL_CLK_DDR         0x98

#define CHIP_REV_1_0            0x10
#define CHIP_REV_1_1_1         0x11
#define PLATFORM_ICGC           0x14
/* ROM ID as the indicator of SOC rev */
#define ROM_SI_REV	0x48

/* Assuming 24MHz input clock with doubler ON */
/*                            MFI         PDF */
#define DP_OP_850       ((8 << 4) + ((1 - 1)  << 0))
#define DP_MFD_850      (48 - 1)
#define DP_MFN_850      41

#define DP_OP_800       ((8 << 4) + ((1 - 1)  << 0))
#define DP_MFD_800      (3 - 1)
#define DP_MFN_800      1

#define DP_OP_700       ((7 << 4) + ((1 - 1)  << 0))
#define DP_MFD_700      (24 - 1)
#define DP_MFN_700      7

#define DP_OP_600       ((6 << 4) + ((1 - 1)  << 0))
#define DP_MFD_600      (4 - 1)
#define DP_MFN_600      1

#define DP_OP_665       ((6 << 4) + ((1 - 1)  << 0))
#define DP_MFD_665      (96 - 1)
#define DP_MFN_665      89

#define DP_OP_532       ((5 << 4) + ((1 - 1)  << 0))
#define DP_MFD_532      (24 - 1)
#define DP_MFN_532      13

#define DP_OP_400       ((8 << 4) + ((2 - 1)  << 0))
#define DP_MFD_400      (3 - 1)
#define DP_MFN_400      1

#define DP_OP_216       ((6 << 4) + ((3 - 1)  << 0))
#define DP_MFD_216      (4 - 1)
#define DP_MFN_216      3

#define PLL_DP_CTL      0x00
#define PLL_DP_CONFIG   0x04
#define PLL_DP_OP       0x08
#define PLL_DP_MFD      0x0C
#define PLL_DP_MFN      0x10
#define PLL_DP_MFNMINUS 0x14
#define PLL_DP_MFNPLUS  0x18
#define PLL_DP_HFS_OP   0x1C
#define PLL_DP_HFS_MFD  0x20
#define PLL_DP_HFS_MFN  0x24
#define PLL_DP_TOGC     0x28
#define PLL_DP_DESTAT   0x2C

#ifndef __ASSEMBLER__

enum boot_device {
	WEIM_NOR_BOOT,
	ONE_NAND_BOOT,
	PATA_BOOT,
	SATA_BOOT,
	I2C_BOOT,
	SPI_NOR_BOOT,
	SD_BOOT,
	MMC_BOOT,
	NAND_BOOT,
	UNKNOWN_BOOT,
	BOOT_DEV_NUM = UNKNOWN_BOOT,
};

enum mxc_clock {
	MXC_ARM_CLK = 0,
	MXC_PER_CLK,
	MXC_AHB_CLK,
	MXC_IPG_CLK,
	MXC_IPG_PERCLK,
	MXC_UART_CLK,
	MXC_CSPI_CLK,
	MXC_AXI_A_CLK,
	MXC_AXI_B_CLK,
	MXC_EMI_SLOW_CLK,
	MXC_DDR_CLK,
	MXC_ESDHC_CLK,
	MXC_ESDHC2_CLK,
	MXC_ESDHC3_CLK,
	MXC_ESDHC4_CLK,
	MXC_GPMI_CLK,
	MXC_BCH_CLK,
};

enum mxc_peri_clocks {
	MXC_UART1_BAUD,
	MXC_UART2_BAUD,
	MXC_UART3_BAUD,
	MXC_SSI1_BAUD,
	MXC_SSI2_BAUD,
	MXC_CSI_BAUD,
	MXC_MSTICK1_CLK,
	MXC_MSTICK2_CLK,
	MXC_SPI1_CLK,
	MXC_SPI2_CLK,
};

extern unsigned int mxc_get_clock(enum mxc_clock clk);
extern unsigned int get_board_rev(void);
extern int is_soc_rev(int rev);
extern enum boot_device get_boot_device(void);

extern void set_usboh3_clk(void);
extern void set_usb_phy1_clk(void);
extern void enable_usboh3_clk(unsigned char enable);
extern void enable_usb_phy1_clk(unsigned char enable);

#endif /* __ASSEMBLER__*/

#endif				/*  __ASM_ARCH_MXC_MX50_H__ */
