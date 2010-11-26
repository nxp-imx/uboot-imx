/*
 * Copyright (C) 2010 Freescale Semiconductor, Inc. All Rights Reserved.
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

#ifndef __ASM_ARCH_MXC_MX53_H__
#define __ASM_ARCH_MXC_MX53_H__

#define __REG(x)        (*((volatile u32 *)(x)))
#define __REG16(x)      (*((volatile u16 *)(x)))
#define __REG8(x)       (*((volatile u8 *)(x)))

/*
  * ROM address which denotes silicon rev
  */
#define ROM_SI_REV	0x48

/*
 * SATA
 */
#define SATA_BASE_ADDR		0x10000000

/*
 * IRAM
 */
#define IRAM_BASE_ADDR		0xF8000000	/* internal ram */
#define IRAM_PARTITIONS		16
#define IRAM_SIZE		(IRAM_PARTITIONS*SZ_8K)	/* 128KB */

/*
 * NFC
 */
#define NFC_BASE_ADDR_AXI		0xF7FF0000	/* NAND flash AXI */
#define NFC_AXI_SIZE		SZ_64K

#define TZIC_BASE_ADDR		0x0FFFC000

#define DEBUG_BASE_ADDR	0x40000000
#define ETB_BASE_ADDR		(DEBUG_BASE_ADDR + 0x00001000)
#define ETM_BASE_ADDR		(DEBUG_BASE_ADDR + 0x00002000)
#define TPIU_BASE_ADDR		(DEBUG_BASE_ADDR + 0x00003000)
#define CTI0_BASE_ADDR		(DEBUG_BASE_ADDR + 0x00004000)
#define CTI1_BASE_ADDR		(DEBUG_BASE_ADDR + 0x00005000)
#define CTI2_BASE_ADDR		(DEBUG_BASE_ADDR + 0x00006000)
#define CTI3_BASE_ADDR		(DEBUG_BASE_ADDR + 0x00007000)
#define CORTEX_DBG_BASE_ADDR	(DEBUG_BASE_ADDR + 0x00008000)

/*
 * SPBA global module enabled #0
 */
#define SPBA0_BASE_ADDR 	0x50000000

#define MMC_SDHC1_BASE_ADDR	(SPBA0_BASE_ADDR + 0x00004000)
#define MMC_SDHC2_BASE_ADDR	(SPBA0_BASE_ADDR + 0x00008000)
#define UART3_BASE_ADDR 	(SPBA0_BASE_ADDR + 0x0000C000)
#define CSPI1_BASE_ADDR 	(SPBA0_BASE_ADDR + 0x00010000)
#define SSI2_BASE_ADDR		(SPBA0_BASE_ADDR + 0x00014000)
#define ESAI_BASE_ADDR		(SPBA0_BASE_ADDR + 0x00018000)
#define MMC_SDHC3_BASE_ADDR	(SPBA0_BASE_ADDR + 0x00020000)
#define MMC_SDHC4_BASE_ADDR	(SPBA0_BASE_ADDR + 0x00024000)
#define SPDIF_BASE_ADDR		(SPBA0_BASE_ADDR + 0x00028000)
#define ASRC_BASE_ADDR		(SPBA0_BASE_ADDR + 0x0002C000)
#define ATA_DMA_BASE_ADDR	(SPBA0_BASE_ADDR + 0x00030000)
#define SPBA_CTRL_BASE_ADDR	(SPBA0_BASE_ADDR + 0x0003C000)

/*!
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

#define OTG_BASE_ADDR	(AIPS1_BASE_ADDR + 0x00080000)
#define GPIO1_BASE_ADDR	(AIPS1_BASE_ADDR + 0x00084000)
#define GPIO2_BASE_ADDR	(AIPS1_BASE_ADDR + 0x00088000)
#define GPIO3_BASE_ADDR	(AIPS1_BASE_ADDR + 0x0008C000)
#define GPIO4_BASE_ADDR	(AIPS1_BASE_ADDR + 0x00090000)
#define KPP_BASE_ADDR		(AIPS1_BASE_ADDR + 0x00094000)
#define WDOG1_BASE_ADDR	(AIPS1_BASE_ADDR + 0x00098000)
#define WDOG2_BASE_ADDR	(AIPS1_BASE_ADDR + 0x0009C000)
#define GPT1_BASE_ADDR		(AIPS1_BASE_ADDR + 0x000A0000)
#define SRTC_BASE_ADDR		(AIPS1_BASE_ADDR + 0x000A4000)
#define IOMUXC_BASE_ADDR	(AIPS1_BASE_ADDR + 0x000A8000)
#define EPIT1_BASE_ADDR	(AIPS1_BASE_ADDR + 0x000AC000)
#define EPIT2_BASE_ADDR	(AIPS1_BASE_ADDR + 0x000B0000)
#define PWM1_BASE_ADDR	(AIPS1_BASE_ADDR + 0x000B4000)
#define PWM2_BASE_ADDR	(AIPS1_BASE_ADDR + 0x000B8000)
#define UART1_BASE_ADDR	(AIPS1_BASE_ADDR + 0x000BC000)
#define UART2_BASE_ADDR	(AIPS1_BASE_ADDR + 0x000C0000)
#define CAN1_BASE_ADDR		(AIPS1_BASE_ADDR + 0x000C8000)
#define CAN2_BASE_ADDR		(AIPS1_BASE_ADDR + 0x000CC000)
#define SRC_BASE_ADDR		(AIPS1_BASE_ADDR + 0x000D0000)
#define CCM_BASE_ADDR		(AIPS1_BASE_ADDR + 0x000D4000)
#define GPC_BASE_ADDR		(AIPS1_BASE_ADDR + 0x000D8000)
#define GPIO5_BASE_ADDR	(AIPS1_BASE_ADDR + 0x000DC000)
#define GPIO6_BASE_ADDR	(AIPS1_BASE_ADDR + 0x000E0000)
#define GPIO7_BASE_ADDR	(AIPS1_BASE_ADDR + 0x000E4000)
#define ATA_BASE_ADDR		(AIPS1_BASE_ADDR + 0x000E8000)
#define I2C3_BASE_ADDR		(AIPS1_BASE_ADDR + 0x000EC000)
#define UART4_BASE_ADDR	(AIPS1_BASE_ADDR + 0x000F0000)

/*
 * AIPS 2
 */
#define AIPS2_BASE_ADDR	0x63F00000

#define PLL1_BASE_ADDR		(AIPS2_BASE_ADDR + 0x00080000)
#define PLL2_BASE_ADDR		(AIPS2_BASE_ADDR + 0x00084000)
#define PLL3_BASE_ADDR		(AIPS2_BASE_ADDR + 0x00088000)
#define PLL4_BASE_ADDR		(AIPS2_BASE_ADDR + 0x0008C000)
#define UART5_BASE_ADDR	(AIPS2_BASE_ADDR + 0x00090000)
#define AHBMAX_BASE_ADDR	(AIPS2_BASE_ADDR + 0x00094000)
#define IIM_BASE_ADDR		(AIPS2_BASE_ADDR + 0x00098000)
#define CSU_BASE_ADDR		(AIPS2_BASE_ADDR + 0x0009C000)
#define ARM_BASE_ADDR		(AIPS2_BASE_ADDR + 0x000A0000)
#define OWIRE_BASE_ADDR 	(AIPS2_BASE_ADDR + 0x000A4000)
#define FIRI_BASE_ADDR		(AIPS2_BASE_ADDR + 0x000A8000)
#define CSPI2_BASE_ADDR	(AIPS2_BASE_ADDR + 0x000AC000)
#define SDMA_BASE_ADDR	(AIPS2_BASE_ADDR + 0x000B0000)
#define SCC_BASE_ADDR		(AIPS2_BASE_ADDR + 0x000B4000)
#define ROMCP_BASE_ADDR	(AIPS2_BASE_ADDR + 0x000B8000)
#define RTIC_BASE_ADDR		(AIPS2_BASE_ADDR + 0x000BC000)
#define CSPI3_BASE_ADDR	(AIPS2_BASE_ADDR + 0x000C0000)
#define I2C2_BASE_ADDR		(AIPS2_BASE_ADDR + 0x000C4000)
#define I2C1_BASE_ADDR		(AIPS2_BASE_ADDR + 0x000C8000)
#define SSI1_BASE_ADDR		(AIPS2_BASE_ADDR + 0x000CC000)
#define AUDMUX_BASE_ADDR	(AIPS2_BASE_ADDR + 0x000D0000)
#define RTC_BASE_ADDR		(AIPS2_BASE_ADDR + 0x000D4000)
#define M4IF_BASE_ADDR		(AIPS2_BASE_ADDR + 0x000D8000)
#define ESDCTL_BASE_ADDR	(AIPS2_BASE_ADDR + 0x000D9000)
#define WEIM_BASE_ADDR		(AIPS2_BASE_ADDR + 0x000DA000)
#define NFC_BASE_ADDR		(AIPS2_BASE_ADDR + 0x000DB000)
#define EMI_BASE_ADDR		(AIPS2_BASE_ADDR + 0x000DBF00)
#define MLB_BASE_ADDR		(AIPS2_BASE_ADDR + 0x000E4000)
#define SSI3_BASE_ADDR		(AIPS2_BASE_ADDR + 0x000E8000)
#define FEC_BASE_ADDR		(AIPS2_BASE_ADDR + 0x000EC000)
#define TVE_BASE_ADDR		(AIPS2_BASE_ADDR + 0x000F0000)
#define VPU_BASE_ADDR		(AIPS2_BASE_ADDR + 0x000F4000)
#define SAHARA_BASE_ADDR	(AIPS2_BASE_ADDR + 0x000F8000)
#define PTP_BASE_ADDR		(AIPS2_BASE_ADDR + 0x000FC000)

/*
 * Memory regions and CS
 */
#define CSD0_BASE_ADDR		0x70000000
#define CSD1_BASE_ADDR		0xB0000000

#define CS0_BASE_ADDR		0xF0000000
#define CS1_BASE_ADDR		0xF4000000
#define CS2_BASE_ADDR		0xF8000000
#define CS3_BASE_ADDR		0xFC000000
#define CS4_BASE_ADDR		0xFE000000
/*
 * Interrupt numbers
 */
#define MXC_INT_BASE		0
#define MXC_INT_RESV0		0
#define MXC_INT_MMC_SDHC1	1
#define MXC_INT_MMC_SDHC2	2
#define MXC_INT_MMC_SDHC3	3
#define MXC_INT_MMC_SDHC4	4
#define MXC_INT_DAP		5
#define MXC_INT_SDMA		6
#define MXC_INT_IOMUX		7
#define MXC_INT_NFC		8
#define MXC_INT_VPU		9
#define MXC_INT_IPU_ERR	10
#define MXC_INT_IPU_SYN	11
#define MXC_INT_GPU		12
#define MXC_INT_UART4		13
#define MXC_INT_USB_H1		14
#define MXC_INT_EMI		15
#define MXC_INT_USB_H2		16
#define MXC_INT_USB_H3		17
#define MXC_INT_USB_OTG	18
#define MXC_INT_SAHARA_H0	19
#define MXC_INT_SAHARA_H1	20
#define MXC_INT_SCC_SMN	21
#define MXC_INT_SCC_STZ	22
#define MXC_INT_SCC_SCM	23
#define MXC_INT_SRTC_NTZ	24
#define MXC_INT_SRTC_TZ	25
#define MXC_INT_RTIC		26
#define MXC_INT_CSU		27
#define MXC_INT_SATA		28
#define MXC_INT_SSI1		29
#define MXC_INT_SSI2		30
#define MXC_INT_UART1		31
#define MXC_INT_UART2		32
#define MXC_INT_UART3		33
#define MXC_INT_RTC			34
#define MXC_INT_PTP		35
#define MXC_INT_CSPI1		36
#define MXC_INT_CSPI2		37
#define MXC_INT_CSPI		38
#define MXC_INT_GPT		39
#define MXC_INT_EPIT1		40
#define MXC_INT_EPIT2		41
#define MXC_INT_GPIO1_INT7	42
#define MXC_INT_GPIO1_INT6	43
#define MXC_INT_GPIO1_INT5	44
#define MXC_INT_GPIO1_INT4	45
#define MXC_INT_GPIO1_INT3	46
#define MXC_INT_GPIO1_INT2	47
#define MXC_INT_GPIO1_INT1	48
#define MXC_INT_GPIO1_INT0	49
#define MXC_INT_GPIO1_LOW	50
#define MXC_INT_GPIO1_HIGH	51
#define MXC_INT_GPIO2_LOW	52
#define MXC_INT_GPIO2_HIGH	53
#define MXC_INT_GPIO3_LOW	54
#define MXC_INT_GPIO3_HIGH	55
#define MXC_INT_GPIO4_LOW	56
#define MXC_INT_GPIO4_HIGH	57
#define MXC_INT_WDOG1		58
#define MXC_INT_WDOG2		59
#define MXC_INT_KPP		60
#define MXC_INT_PWM1		61
#define MXC_INT_I2C1		62
#define MXC_INT_I2C2		63
#define MXC_INT_I2C3		64
#define MXC_INT_MLB		65
#define MXC_INT_ASRC		66
#define MXC_INT_SPDIF		67
#define MXC_INT_RESV1		68
#define MXC_INT_IIM		69
#define MXC_INT_ATA		70
#define MXC_INT_CCM1		71
#define MXC_INT_CCM2		72
#define MXC_INT_GPC1		73
#define MXC_INT_GPC2		74
#define MXC_INT_SRC		75
#define MXC_INT_NM		76
#define MXC_INT_PMU		77
#define MXC_INT_CTI_IRQ		78
#define MXC_INT_CTI1_TG0	79
#define MXC_INT_CTI1_TG1	80
#define MXC_INT_ESAI		81
#define MXC_INT_CAN1		82
#define MXC_INT_CAN2		83
#define MXC_INT_GPU2_IRQ	84
#define MXC_INT_GPU2_BUSY	85
#define MXC_INT_UART5		86
#define MXC_INT_FEC		87
#define MXC_INT_OWIRE		88
#define MXC_INT_CTI1_TG2	89
#define MXC_INT_SJC		90
#define MXC_INT_RESV2		91
#define MXC_INT_TVE		92
#define MXC_INT_FIRI		93
#define MXC_INT_PWM2		94
#define MXC_INT_RESV3		95
#define MXC_INT_SSI3		96
#define MXC_INT_RESV4		97
#define MXC_INT_CTI1_TG3	98
#define MXC_INT_RESV5		99
#define MXC_INT_VPU_IDLE	100
#define MXC_INT_EMI_NFC	101
#define MXC_INT_GPU_IDLE	102
#define MXC_INT_GPIO5_LOW	103
#define MXC_INT_GPIO5_HIGH	104
#define MXC_INT_GPIO6_LOW	105
#define MXC_INT_GPIO6_HIGH	106
#define MXC_INT_GPIO7_LOW	107
#define MXC_INT_GPIO7_HIGH	108

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

#define CHIP_REV_1_0            0x10
#define CHIP_REV_2_0            0x20
#define PLATFORM_ICGC           0x14

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
	MXC_SATA_CLK,
	MXC_NFC_CLK,
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

#endif				/*  __ASM_ARCH_MXC_MX53_H__ */
