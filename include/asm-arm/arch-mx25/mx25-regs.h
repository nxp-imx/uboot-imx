/*
 *
 * (c) 2007 Pengutronix, Sascha Hauer <s.hauer@pengutronix.de>
 *
 * (C) Copyright 2009 Freescale Semiconductor
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

#ifndef __ASM_ARCH_MX25_REGS_H
#define __ASM_ARCH_MX25_REGS_H

#define __REG(x)	(*((volatile u32 *)(x)))
#define __REG16(x)	(*((volatile u16 *)(x)))
#define __REG8(x)	(*((volatile u8 *)(x)))

/*
 * AIPS 1
 */
#define AIPS1_BASE		0x43F00000
#define AIPS1_CTRL_BASE		AIPS1_BASE
#define MAX_BASE		0x43F04000
#define CLKCTL_BASE		0x43F08000
#define ETB_SLOT4_BASE		0x43F0C000
#define ETB_SLOT5_BASE		0x43F1000l
#define ECT_CTIO_BASE		0x43F18000
#define I2C1_BASE		0x43F80000
#define I2C3_BASE		0x43F84000
#define CAN1_BASE		0x43F88000
#define CAN2_BASE		0x43F8C000
#define UART1_BASE		0x43F90000
#define UART2_BASE		0x43F94000
#define I2C2_BASE		0x43F98000
#define OWIRE_BASE		0x43F9C000
#define CSPI1_BASE		0x43FA4000
#define KPP_BASE		0x43FA8000
#define IOMUXC_BASE		0x43FAC000
#define AUDMUX_BASE		0x43FB0000
#define ECT_IP1_BASE		0x43FB8000
#define ECT_IP2_BASE		0x43FBC000

/*
 * SPBA
 */
#define SPBA_BASE		0x50000000
#define CSPI3_BASE		0x50040000
#define UART4_BASE		0x50008000
#define UART3_BASE		0x5000C000
#define CSPI2_BASE		0x50010000
#define SSI2_BASE		0x50014000
#define ESAI_BASE		0x50018000
#define ATA_DMA_BASE		0x50020000
#define SIM1_BASE		0x50024000
#define SIM2_BASE		0x50028000
#define UART5_BASE		0x5002C000
#define TSC_BASE		0x50030000
#define SSI1_BASE		0x50034000
#define FEC_BASE		0x50038000
#define SOC_FEC			FEC_BASE
#define SPBA_CTRL_BASE		0x5003C000

/*
 * AIPS 2
 */
#define AIPS2_BASE		0x53F00000
#define AIPS2_CTRL_BASE		AIPS2_BASE
#define CCM_BASE		0x53F80000
#define GPT4_BASE		0x53F84000
#define GPT3_BASE		0x53F88000
#define GPT2_BASE		0x53F8C000
#define GPT1_BASE		0x53F90000
#define EPIT1_BASE		0x53F94000
#define EPIT2_BASE		0x53F98000
#define GPIO4_BASE		0x53F9C000
#define PWM2_BASE		0x53FA0000
#define GPIO3_BASE		0x53FA4000
#define PWM3_BASE		0x53FA8000
#define SCC_BASE		0x53FAC000
#define SCM_BASE		0x53FAE000
#define SMN_BASE		0x53FAF000
#define RNGD_BASE		0x53FB0000
#define MMC_SDHC1_BASE		0x53FB4000
#define MMC_SDHC2_BASE		0x53FB8000
#define ESDHC1_REG_BASE		MMC_SDHC1_BASE
#define LCDC_BASE		0x53FBC000
#define SLCDC_BASE		0x53FC0000
#define PWM4_BASE		0x53FC8000
#define GPIO1_BASE		0x53FCC000
#define GPIO2_BASE		0x53FD0000
#define SDMA_BASE		0x53FD4000
#define WDOG_BASE		0x53FDC000
#define PWM1_BASE		0x53FE0000
#define RTIC_BASE		0x53FEC000
#define IIM_BASE		0x53FF0000
#define USB_BASE		0x53FF4000
#define CSI_BASE		0x53FF8000
#define DRYICE_BASE		0x53FFC000

/*
 * ROMPATCH and ASIC
 */
#define ROMPATCH_BASE		0x60000000
#define ROMPATCH_REV		0x40
#define ASIC_BASE		0x68000000

#define RAM_BASE		0x78000000

/*
 * NAND, SDRAM, WEIM, M3IF, EMI controllers
 */
#define EXT_MEM_CTRL_BASE	0xB8000000
#define ESDCTL_BASE		0xB8001000
#define WEIM_BASE		0xB8002000
#define WEIM_CTRL_CS0		WEIM_BASE
#define WEIM_CTRL_CS1		(WEIM_BASE + 0x10)
#define WEIM_CTRL_CS2		(WEIM_BASE + 0x20)
#define WEIM_CTRL_CS3		(WEIM_BASE + 0x30)
#define WEIM_CTRL_CS4		(WEIM_BASE + 0x40)
#define WEIM_CTRL_CS5		(WEIM_BASE + 0x50)
#define M3IF_BASE		0xB8003000
#define EMI_BASE		0xB8004000

#define NFC_BASE_ADDR		0xBB000000
/*
 * Memory regions and CS
 */
#define CSD0_BASE		0x80000000
#define CSD1_BASE		0x90000000
#define CS0_BASE		0xA0000000
#define CS1_BASE		0xA8000000
#define CS2_BASE		0xB0000000
#define CS3_BASE		0xB2000000
#define CS4_BASE		0xB4000000
#define CS5_BASE		0xB6000000

/* CCM */
#define CCM_MPCTL			(CCM_BASE + 0x00)
#define CCM_UPCTL			(CCM_BASE + 0x04)
#define CCM_CCTL			(CCM_BASE + 0x08)
#define CCM_CGR0			(CCM_BASE + 0x0C)
#define CCM_CGR1			(CCM_BASE + 0x10)
#define CCM_CGR2			(CCM_BASE + 0x14)
#define CCM_PCDR0			(CCM_BASE + 0x18)
#define CCM_PCDR1			(CCM_BASE + 0x1C)
#define CCM_PCDR2			(CCM_BASE + 0x20)
#define CCM_PCDR3			(CCM_BASE + 0x24)
#define CCM_RCSR			(CCM_BASE + 0x28)
#define CCM_CRDR			(CCM_BASE + 0x2C)
#define CCM_DCVR0			(CCM_BASE + 0x30)
#define CCM_DCVR1			(CCM_BASE + 0x34)
#define CCM_DCVR2			(CCM_BASE + 0x38)
#define CCM_DCVR3			(CCM_BASE + 0x3C)
#define CCM_LTR0			(CCM_BASE + 0x40)
#define CCM_LTR1			(CCM_BASE + 0x44)
#define CCM_LTR2			(CCM_BASE + 0x48)
#define CCM_LTR3			(CCM_BASE + 0x4C)
#define CCM_LTBR0			(CCM_BASE + 0x50)
#define CCM_LTBR1			(CCM_BASE + 0x54)
#define CCM_PCMR0			(CCM_BASE + 0x58)
#define CCM_PCMR1			(CCM_BASE + 0x5C)
#define CCM_PCMR2			(CCM_BASE + 0x60)
#define CCM_MCR				(CCM_BASE + 0x64)
#define CCM_LPIMR0			(CCM_BASE + 0x68)
#define CCM_LPIMR1			(CCM_BASE + 0x6C)

#define CRM_CCTL_ARM_SRC		(1 << 14)
#define CRM_CCTL_AHB_OFFSET		28


#define FREQ_24MHZ			24000000
#define PLL_REF_CLK			FREQ_24MHZ

/*
 * FIXME - Constants verified up to this point.
 * Offsets and derived constants below should be confirmed.
 */

#define CLKMODE_AUTO		0
#define CLKMODE_CONSUMER	1

/* WEIM - CS0 */
#define CSCRU				0x00
#define CSCRL				0x04
#define CSCRA				0x08

#define CHIP_REV_1_0		0x0	/* PASS 1.0 */
#define CHIP_REV_1_1		0x1	/* PASS 1.1 */
#define CHIP_REV_2_0		0x2	/* PASS 2.0 */
#define CHIP_LATEST		CHIP_REV_1_1

#define IIM_STAT		0x00
#define IIM_STAT_BUSY		(1 << 7)
#define IIM_STAT_PRGD		(1 << 1)
#define IIM_STAT_SNSD		(1 << 0)
#define IIM_STATM		0x04
#define IIM_ERR			0x08
#define IIM_ERR_PRGE		(1 << 7)
#define IIM_ERR_WPE		(1 << 6)
#define IIM_ERR_OPE		(1 << 5)
#define IIM_ERR_RPE		(1 << 4)
#define IIM_ERR_WLRE		(1 << 3)
#define IIM_ERR_SNSE		(1 << 2)
#define IIM_ERR_PARITYE		(1 << 1)
#define IIM_EMASK		0x0C
#define IIM_FCTL		0x10
#define IIM_UAF			0x14
#define IIM_LA			0x18
#define IIM_SDAT		0x1C
#define IIM_PREV		0x20
#define IIM_SREV		0x24
#define IIM_PREG_P		0x28
#define IIM_SCS0		0x2C
#define IIM_SCS1		0x30
#define IIM_SCS2		0x34
#define IIM_SCS3		0x38

#define EPIT_BASE		EPIT1_BASE
#define EPITCR			0x00
#define EPITSR			0x04
#define EPITLR			0x08
#define EPITCMPR		0x0C
#define EPITCNR			0x10

#define GPT_BASE		GPT1_BASE
/*#define GPTCR			0x00
#define GPTPR			0x04
#define GPTSR			0x08
#define GPTIR			0x0C
#define GPTOCR1			0x10
#define GPTOCR2			0x14
#define GPTOCR3			0x18
#define GPTICR1			0x1C
#define GPTICR2			0x20
#define GPTCNT			0x24*/

/* ESDCTL */
#define ESDCTL_ESDCTL0		0x00
#define ESDCTL_ESDCFG0		0x04
#define ESDCTL_ESDCTL1		0x08
#define ESDCTL_ESDCFG1		0x0C
#define ESDCTL_ESDMISC		0x10

/* DRYICE */
#define DRYICE_DTCMR		0x00
#define DRYICE_DTCLR		0x04
#define DRYICE_DCAMR		0x08
#define DRYICE_DCALR		0x0C
#define DRYICE_DCR		0x10
#define DRYICE_DSR		0x14
#define DRYICE_DIER		0x18
#define DRYICE_DMCR		0x1C
#define DRYICE_DKSR		0x20
#define DRYICE_DKCR		0x24
#define DRYICE_DTCR		0x28
#define DRYICE_DACR		0x2C
#define DRYICE_DGPR		0x3C
#define DRYICE_DPKR0		0x40
#define DRYICE_DPKR1		0x44
#define DRYICE_DPKR2		0x48
#define DRYICE_DPKR3		0x4C
#define DRYICE_DPKR4		0x50
#define DRYICE_DPKR5		0x54
#define DRYICE_DPKR6		0x58
#define DRYICE_DPKR7		0x5C
#define DRYICE_DRKR0		0x60
#define DRYICE_DRKR1		0x64
#define DRYICE_DRKR2		0x68
#define DRYICE_DRKR3		0x6C
#define DRYICE_DRKR4		0x70
#define DRYICE_DRKR5		0x74
#define DRYICE_DRKR6		0x78
#define DRYICE_DRKR7		0x7C

/* GPIO */
#define GPIO_DR			0x00
#define GPIO_GDIR		0x04
#define GPIO_PSR0		0x08
#define GPIO_ICR1		0x0C
#define GPIO_ICR2		0x10
#define GPIO_IMR		0x14
#define GPIO_ISR		0x18
#define GPIO_EDGE_SEL		0x1C


#if (PLL_REF_CLK != 24000000)
#error Wrong PLL reference clock! The following macros will not work.
#endif

/* Assuming 24MHz input clock */
/*                           PD             MFD              MFI          MFN */
#define MPCTL_PARAM_399    (((1-1) << 26) + ((16-1) << 16) + \
				(8  << 10) + (5 << 0))
#define MPCTL_PARAM_532     ((1 << 31) + ((1-1) << 26) + \
				((12-1) << 16) + (11  << 10) + (1 << 0))
#define MPCTL_PARAM_665     (((1-1) << 26) + ((48-1) << 16) + \
				(13  << 10) + (41 << 0))

/* UPCTL                     PD             MFD              MFI          MFN */
#define UPCTL_PARAM_300     (((1-1) << 26) + ((4-1) << 16) + \
				(6  << 10) + (1  << 0))

#define NFC_V1_1

#define NAND_REG_BASE			(NFC_BASE + 0x1E00)
#define NFC_BUFSIZE_REG_OFF		(0 + 0x00)
#define RAM_BUFFER_ADDRESS_REG_OFF	(0 + 0x04)
#define NAND_FLASH_ADD_REG_OFF		(0 + 0x06)
#define NAND_FLASH_CMD_REG_OFF		(0 + 0x08)
#define NFC_CONFIGURATION_REG_OFF	(0 + 0x0A)
#define ECC_STATUS_RESULT_REG_OFF	(0 + 0x0C)
#define ECC_RSLT_MAIN_AREA_REG_OFF	(0 + 0x0E)
#define ECC_RSLT_SPARE_AREA_REG_OFF	(0 + 0x10)
#define NF_WR_PROT_REG_OFF		(0 + 0x12)
#define NAND_FLASH_WR_PR_ST_REG_OFF	(0 + 0x18)
#define NAND_FLASH_CONFIG1_REG_OFF	(0 + 0x1A)
#define NAND_FLASH_CONFIG2_REG_OFF	(0 + 0x1C)
#define UNLOCK_START_BLK_ADD_REG_OFF	(0 + 0x20)
#define UNLOCK_END_BLK_ADD_REG_OFF	(0 + 0x22)
#define RAM_BUFFER_ADDRESS_RBA_3	0x3
#define NFC_BUFSIZE_1KB			0x0
#define NFC_BUFSIZE_2KB			0x1
#define NFC_CONFIGURATION_UNLOCKED	0x2
#define ECC_STATUS_RESULT_NO_ERR	0x0
#define ECC_STATUS_RESULT_1BIT_ERR	0x1
#define ECC_STATUS_RESULT_2BIT_ERR	0x2
#define NF_WR_PROT_UNLOCK		0x4
#define NAND_FLASH_CONFIG1_FORCE_CE	(1 << 7)
#define NAND_FLASH_CONFIG1_RST		(1 << 6)
#define NAND_FLASH_CONFIG1_BIG		(1 << 5)
#define NAND_FLASH_CONFIG1_INT_MSK	(1 << 4)
#define NAND_FLASH_CONFIG1_ECC_EN	(1 << 3)
#define NAND_FLASH_CONFIG1_SP_EN	(1 << 2)
#define NAND_FLASH_CONFIG2_INT_DONE	(1 << 15)
#define NAND_FLASH_CONFIG2_FDO_PAGE	(0 << 3)
#define NAND_FLASH_CONFIG2_FDO_ID	(2 << 3)
#define NAND_FLASH_CONFIG2_FDO_STATUS	(4 << 3)
#define NAND_FLASH_CONFIG2_FDI_EN	(1 << 2)
#define NAND_FLASH_CONFIG2_FADD_EN	(1 << 1)
#define NAND_FLASH_CONFIG2_FCMD_EN	(1 << 0)
#define FDO_PAGE_SPARE_VAL		0x8
#define NAND_BUF_NUM			8

#define MXC_NAND_BASE_DUMMY		0x00000000
#define MXC_MMC_BASE_DUMMY		0x00000000
#define NOR_FLASH_BOOT			0
#define NAND_FLASH_BOOT			0x10000000
#define SDRAM_NON_FLASH_BOOT		0x20000000
#define MMC_FLASH_BOOT			0x40000000
#define MXCBOOT_FLAG_REG		(CSI_BASE_ADDR + 0x28)
#define MXCFIS_NOTHING			0x00000000
#define MXCFIS_NAND			0x10000000
#define MXCFIS_NOR			0x20000000
#define MXCFIS_MMC			0x40000000
#define MXCFIS_FLAG_REG			(CSI_BASE_ADDR + 0x2C)

/*!
 *  * NFMS bit in RCSR register for pagesize of nandflash
 *   */
#define NFMS		(*((volatile u32 *)(CCM_BASE+0x28)))
#define NFMS_BIT	8
#define NFMS_NF_DWIDTH	14
#define NFMS_NF_PG_SZ	8

#endif
