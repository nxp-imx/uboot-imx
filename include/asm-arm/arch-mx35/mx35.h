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
#define MMC_SDHC1_BASE_ADDR 	0x53FB4000
#define MMC_SDHC2_BASE_ADDR 	0x53FB8000
#define MMC_SDHC3_BASE_ADDR 	0x53FBC000
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

#define IIM_SREV	0x24
#define ROMPATCH_REV	0x40

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

#define NFC_BUF_SIZE   0x1000
#define NFC_BUFSIZE_REG_OFF             (0 + 0x00)
#define RAM_BUFFER_ADDRESS_REG_OFF      (0 + 0x04)
#define NAND_FLASH_ADD_REG_OFF          (0 + 0x06)
#define NAND_FLASH_CMD_REG_OFF          (0 + 0x08)
#define NFC_CONFIGURATION_REG_OFF       (0 + 0x0A)
#define ECC_STATUS_RESULT_REG_OFF       (0 + 0x0C)
#define ECC_RSLT_MAIN_AREA_REG_OFF      (0 + 0x0E)
#define ECC_RSLT_SPARE_AREA_REG_OFF     (0 + 0x10)
#define NF_WR_PROT_REG_OFF              (0 + 0x12)
#define NAND_FLASH_WR_PR_ST_REG_OFF     (0 + 0x18)
#define NAND_FLASH_CONFIG1_REG_OFF      (0 + 0x1A)
#define NAND_FLASH_CONFIG2_REG_OFF      (0 + 0x1C)
#define UNLOCK_START_BLK_ADD_REG_OFF    (0 + 0x20)
#define UNLOCK_END_BLK_ADD_REG_OFF      (0 + 0x22)
#define RAM_BUFFER_ADDRESS_RBA_3        0x3
#define NFC_BUFSIZE_1KB                 0x0
#define NFC_BUFSIZE_2KB                 0x1
#define NFC_CONFIGURATION_UNLOCKED      0x2
#define ECC_STATUS_RESULT_NO_ERR        0x0
#define ECC_STATUS_RESULT_1BIT_ERR      0x1
#define ECC_STATUS_RESULT_2BIT_ERR      0x2
#define NF_WR_PROT_UNLOCK               0x4
#define NAND_FLASH_CONFIG1_FORCE_CE     (1 << 7)
#define NAND_FLASH_CONFIG1_RST          (1 << 6)
#define NAND_FLASH_CONFIG1_BIG          (1 << 5)
#define NAND_FLASH_CONFIG1_INT_MSK      (1 << 4)
#define NAND_FLASH_CONFIG1_ECC_EN       (1 << 3)
#define NAND_FLASH_CONFIG1_SP_EN        (1 << 2)
#define NAND_FLASH_CONFIG2_INT_DONE     (1 << 15)
#define NAND_FLASH_CONFIG2_FDO_PAGE     (0 << 3)
#define NAND_FLASH_CONFIG2_FDO_ID       (2 << 3)
#define NAND_FLASH_CONFIG2_FDO_STATUS   (4 << 3)
#define NAND_FLASH_CONFIG2_FDI_EN       (1 << 2)
#define NAND_FLASH_CONFIG2_FADD_EN      (1 << 1)
#define NAND_FLASH_CONFIG2_FCMD_EN      (1 << 0)
#define FDO_PAGE_SPARE_VAL              0x8
#define NAND_BUF_NUM    8

#define CHIP_REV_1_0		0x10
#define CHIP_REV_2_0		0x20

#define BOARD_REV_1_0		0x0
#define BOARD_REV_2_0		0x1

#ifndef __ASSEMBLER__


enum mxc_clock {
MXC_ARM_CLK = 0,
MXC_AHB_CLK,
MXC_IPG_CLK,
MXC_IPG_PERCLK,
MXC_UART_CLK,
MXC_ESDHC_CLK,
MXC_USB_CLK,
};

enum plls {
	MCU_PLL = CCM_BASE_ADDR + CLKCTL_MPCTL,
	PER_PLL = CCM_BASE_ADDR + CLKCTL_PPCTL,
};

enum mxc_main_clocks {
	CPU_CLK,
	AHB_CLK,
	IPG_CLK,
	IPG_PER_CLK,
	NFC_CLK,
	USB_CLK,
	HSP_CLK,
};

enum mxc_peri_clocks {
	UART1_BAUD,
	UART2_BAUD,
	UART3_BAUD,
	SSI1_BAUD,
	SSI2_BAUD,
	CSI_BAUD,
	MSHC_CLK,
	ESDHC1_CLK,
	ESDHC2_CLK,
	ESDHC3_CLK,
	SPDIF_CLK,
	SPI1_CLK,
	SPI2_CLK,
};
/*!
 * NFMS bit in RCSR register for pagesize of nandflash
 */
#define NFMS            (*((volatile u32 *)(CCM_BASE_ADDR+0x18)))
#define NFMS_BIT                8
#define NFMS_NF_DWIDTH          14
#define NFMS_NF_PG_SZ           8


extern unsigned int mxc_get_clock(enum mxc_clock clk);
extern unsigned int get_board_rev(void);
extern int is_soc_rev(int rev);
extern int sdhc_init(void);

#define fixup_before_linux	\
	{		\
		volatile unsigned long *l2cc_ctl = (unsigned long *)0x30000100;\
		if (is_soc_rev(CHIP_REV_2_0) < 0) \
			*l2cc_ctl = 1;\
	}
#endif /* __ASSEMBLER__*/
#endif /* __ASM_ARCH_MX35_H */
