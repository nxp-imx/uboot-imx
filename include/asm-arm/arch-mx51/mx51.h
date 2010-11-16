/*
 * Copyright 2009-2010 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#ifndef __ASM_ARCH_MXC_MX51_H__
#define __ASM_ARCH_MXC_MX51_H__

#define __REG(x)	(*((volatile u32 *)(x)))
#define __REG16(x)	(*((volatile u16 *)(x)))
#define __REG8(x)	(*((volatile u8 *)(x)))
/*
 * IRAM
 */
#define IRAM_BASE_ADDR		0x1FFE8000	/* internal ram */
/*
 * Graphics Memory of GPU
 */
#define GPU_BASE_ADDR		0x20000000
#define GPU_CTRL_BASE_ADDR	0x30000000
#define IPU_CTRL_BASE_ADDR	0x40000000
/*
 * Debug
 */
#define DEBUG_BASE_ADDR		0x60000000
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
#define SPBA0_BASE_ADDR 	0x70000000

#define MMC_SDHC1_BASE_ADDR	(SPBA0_BASE_ADDR + 0x00004000)
#define MMC_SDHC2_BASE_ADDR	(SPBA0_BASE_ADDR + 0x00008000)
#define UART3_BASE_ADDR 	(SPBA0_BASE_ADDR + 0x0000C000)
#define CSPI1_BASE_ADDR 	(SPBA0_BASE_ADDR + 0x00010000)
#define SSI2_BASE_ADDR		(SPBA0_BASE_ADDR + 0x00014000)
#define MMC_SDHC3_BASE_ADDR	(SPBA0_BASE_ADDR + 0x00020000)
#define MMC_SDHC4_BASE_ADDR	(SPBA0_BASE_ADDR + 0x00024000)
#define SPDIF_BASE_ADDR		(SPBA0_BASE_ADDR + 0x00028000)
#define ATA_DMA_BASE_ADDR	(SPBA0_BASE_ADDR + 0x00030000)
#define SLIM_DMA_BASE_ADDR	(SPBA0_BASE_ADDR + 0x00034000)
#define HSI2C_DMA_BASE_ADDR	(SPBA0_BASE_ADDR + 0x00038000)
#define SPBA_CTRL_BASE_ADDR	(SPBA0_BASE_ADDR + 0x0003C000)

/*
 * AIPS 1
 */
#define AIPS1_BASE_ADDR 	0x73F00000

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
#define SRC_BASE_ADDR		(AIPS1_BASE_ADDR + 0x000D0000)
#define CCM_BASE_ADDR		(AIPS1_BASE_ADDR + 0x000D4000)
#define GPC_BASE_ADDR		(AIPS1_BASE_ADDR + 0x000D8000)

/*
 * AIPS 2
 */
#define AIPS2_BASE_ADDR	0x83F00000

#define PLL1_BASE_ADDR		(AIPS2_BASE_ADDR + 0x00080000)
#define PLL2_BASE_ADDR		(AIPS2_BASE_ADDR + 0x00084000)
#define PLL3_BASE_ADDR		(AIPS2_BASE_ADDR + 0x00088000)
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
#define M4IF_BASE_ADDR		(AIPS2_BASE_ADDR + 0x000D8000)
#define ESDCTL_BASE_ADDR	(AIPS2_BASE_ADDR + 0x000D9000)
#define WEIM_BASE_ADDR	(AIPS2_BASE_ADDR + 0x000DA000)
#define NFC_BASE_ADDR		(AIPS2_BASE_ADDR + 0x000DB000)
#define EMI_BASE_ADDR		(AIPS2_BASE_ADDR + 0x000DBF00)
#define MIPI_HSC_BASE_ADDR	(AIPS2_BASE_ADDR + 0x000DC000)
#define ATA_BASE_ADDR		(AIPS2_BASE_ADDR + 0x000E0000)
#define SIM_BASE_ADDR		(AIPS2_BASE_ADDR + 0x000E4000)
#define SSI3BASE_ADDR		(AIPS2_BASE_ADDR + 0x000E8000)
#define FEC_BASE_ADDR		(AIPS2_BASE_ADDR + 0x000EC000)
#define TVE_BASE_ADDR		(AIPS2_BASE_ADDR + 0x000F0000)
#define VPU_BASE_ADDR		(AIPS2_BASE_ADDR + 0x000F4000)
#define SAHARA_BASE_ADDR	(AIPS2_BASE_ADDR + 0x000F8000)

#define TZIC_BASE_ADDR		0x8FFFC000

/*
 * Memory regions and CS
 */
#define CSD0_BASE_ADDR		0x90000000
#define CSD1_BASE_ADDR		0xA0000000
#define CS0_BASE_ADDR		0xB0000000
#define CS1_BASE_ADDR		0xB8000000
#define CS2_BASE_ADDR		0xC0000000
#define CS3_BASE_ADDR		0xC8000000
#define CS4_BASE_ADDR		0xCC000000
#define CS5_BASE_ADDR		0xCE000000

/*
 * NFC
 */
#define NFC_BASE_ADDR_AXI	0xCFFF0000	/* NAND flash AXI */

/*!
 * defines for SPBA modules
 */
#define SPBA_SDHC1	0x04
#define SPBA_SDHC2	0x08
#define SPBA_UART3	0x0C
#define SPBA_CSPI1	0x10
#define SPBA_SSI2	0x14
#define SPBA_SDHC3	0x20
#define SPBA_SDHC4	0x24
#define SPBA_SPDIF	0x28
#define SPBA_ATA	0x30
#define SPBA_SLIM	0x34
#define SPBA_HSI2C	0x38
#define SPBA_CTRL	0x3C

/*
 * Interrupt numbers
 */
#define MXC_INT_BASE		0
#define MXC_INT_RESV0		0
#define MXC_INT_MMC_SDHC1	1
#define MXC_INT_MMC_SDHC2	2
#define MXC_INT_MMC_SDHC3	3
#define MXC_INT_MMC_SDHC4		4
#define MXC_INT_RESV5		5
#define MXC_INT_SDMA		6
#define MXC_INT_IOMUX		7
#define MXC_INT_NFC			8
#define MXC_INT_VPU		9
#define MXC_INT_IPU_ERR		10
#define MXC_INT_IPU_SYN		11
#define MXC_INT_GPU			12
#define MXC_INT_RESV13		13
#define MXC_INT_USB_H1			14
#define MXC_INT_EMI		15
#define MXC_INT_USB_H2			16
#define MXC_INT_USB_H3			17
#define MXC_INT_USB_OTG		18
#define MXC_INT_SAHARA_H0		19
#define MXC_INT_SAHARA_H1		20
#define MXC_INT_SCC_SMN		21
#define MXC_INT_SCC_STZ		22
#define MXC_INT_SCC_SCM		23
#define MXC_INT_SRTC_NTZ	24
#define MXC_INT_SRTC_TZ		25
#define MXC_INT_RTIC		26
#define MXC_INT_CSU		27
#define MXC_INT_SLIM_B			28
#define MXC_INT_SSI1		29
#define MXC_INT_SSI2		30
#define MXC_INT_UART1		31
#define MXC_INT_UART2		32
#define MXC_INT_UART3		33
#define MXC_INT_RESV34		34
#define MXC_INT_RESV35		35
#define MXC_INT_CSPI1		36
#define MXC_INT_CSPI2		37
#define MXC_INT_CSPI			38
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
#define MXC_INT_GPIO4_LOW		56
#define MXC_INT_GPIO4_HIGH		57
#define MXC_INT_WDOG1		58
#define MXC_INT_WDOG2		59
#define MXC_INT_KPP		60
#define MXC_INT_PWM1			61
#define MXC_INT_I2C1			62
#define MXC_INT_I2C2		63
#define MXC_INT_HS_I2C			64
#define MXC_INT_RESV65			65
#define MXC_INT_RESV66		66
#define MXC_INT_SIM_IPB			67
#define MXC_INT_SIM_DAT		68
#define MXC_INT_IIM		69
#define MXC_INT_ATA		70
#define MXC_INT_CCM1		71
#define MXC_INT_CCM2		72
#define MXC_INT_GPC1		73
#define MXC_INT_GPC2		74
#define MXC_INT_SRC		75
#define MXC_INT_NM			76
#define MXC_INT_PMU			77
#define MXC_INT_CTI_IRQ			78
#define MXC_INT_CTI1_TG0		79
#define MXC_INT_CTI1_TG1		80
#define MXC_INT_MCG_ERR		81
#define MXC_INT_MCG_TMR		82
#define MXC_INT_MCG_FUNC		83
#define MXC_INT_RESV84		84
#define MXC_INT_RESV85		85
#define MXC_INT_RESV86		86
#define MXC_INT_FEC		87
#define MXC_INT_OWIRE		88
#define MXC_INT_CTI1_TG2		89
#define MXC_INT_SJC			90
#define MXC_INT_SPDIF		91
#define MXC_INT_TVE			92
#define MXC_INT_FIRI			93
#define MXC_INT_PWM2			94
#define MXC_INT_SLIM_EXP		95
#define MXC_INT_SSI3			96
#define MXC_INT_RESV97			97
#define MXC_INT_CTI1_TG3		98
#define MXC_INT_SMC_RX			99
#define MXC_INT_VPU_IDLE		100
#define MXC_INT_RESV101		101
#define MXC_INT_GPU_IDLE		102

#define MXC_MAX_INT_LINES       128

#define	MXC_GPIO_INT_BASE	(MXC_MAX_INT_LINES)

/*!
 * Number of GPIO port as defined in the IC Spec
 */
#define GPIO_PORT_NUM		4
/*!
 * Number of GPIO pins per port
 */
#define GPIO_NUM_PIN            32

#define MXC_GPIO_SPLIT_IRQ_2

#define IIM_SREV	0x24
#define ROM_SI_REV	0x48

#define NFC_BUF_SIZE	0x1000

/* WEIM registers */
#define CSGCR1	0x00
#define CSGCR2	0x04
#define CSRCR1	0x08
#define CSRCR2	0x0C
#define CSWCR1	0x10

/* M4IF */
#define M4IF_FBPM0	0x40
#define M4IF_FIDBP	0x48

/* ESDCTL */
#define ESDCTL_ESDCTL0                  0x00
#define ESDCTL_ESDCFG0                  0x04
#define ESDCTL_ESDCTL1                  0x08
#define ESDCTL_ESDCFG1                  0x0C
#define ESDCTL_ESDMISC                  0x10
#define ESDCTL_ESDSCR                   0x14
#define ESDCTL_ESDCDLY1                 0x20
#define ESDCTL_ESDCDLY2                 0x24
#define ESDCTL_ESDCDLY3                 0x28
#define ESDCTL_ESDCDLY4                 0x2C
#define ESDCTL_ESDCDLY5                 0x30
#define ESDCTL_ESDCDLYGD                0x34

/* CCM */
#define CLKCTL_CCR              0x00
#define CLKCTL_CCDR             0x04
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
#define CLKCTL_CHSCCDR          0x34
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
#define CLKCTL_CMEOR            0x84

/* DPLL */
#define PLL_DP_CTL	0x00
#define PLL_DP_CONFIG	0x04
#define PLL_DP_OP	0x08
#define PLL_DP_MFD	0x0C
#define PLL_DP_MFN	0x10
#define PLL_DP_MFNMINUS	0x14
#define PLL_DP_MFNPLUS	0x18
#define PLL_DP_HFS_OP	0x1C
#define PLL_DP_HFS_MFD	0x20
#define PLL_DP_HFS_MFN	0x24
#define PLL_DP_TOGC	0x28
#define PLL_DP_DESTAT	0x2C

/* Assuming 24MHz input clock with doubler ON */
/*                            MFI         PDF */
#define DP_OP_850	((8 << 4) + ((1 - 1)  << 0))
#define DP_MFD_850	(48 - 1)
#define DP_MFN_850	41

#define DP_OP_800	((8 << 4) + ((1 - 1)  << 0))
#define DP_MFD_800	(3 - 1)
#define DP_MFN_800	1

#define DP_OP_700	((7 << 4) + ((1 - 1)  << 0))
#define DP_MFD_700	(24 - 1)
#define DP_MFN_700	7

#define DP_OP_665	((6 << 4) + ((1 - 1)  << 0))
#define DP_MFD_665	(96 - 1)
#define DP_MFN_665	89

#define DP_OP_532	((5 << 4) + ((1 - 1)  << 0))
#define DP_MFD_532	(24 - 1)
#define DP_MFN_532	13

#define DP_OP_400	((8 << 4) + ((2 - 1)  << 0))
#define DP_MFD_400	(3 - 1)
#define DP_MFN_400	1

#define DP_OP_216	((6 << 4) + ((3 - 1)  << 0))
#define DP_MFD_216	(4 - 1)
#define DP_MFN_216	3

/* IIM */
#define IIM_STAT_OFF            0x00
#define IIM_STAT_BUSY           (1 << 7)
#define IIM_STAT_PRGD           (1 << 1)
#define IIM_STAT_SNSD           (1 << 0)
#define IIM_STATM_OFF           0x04
#define IIM_ERR_OFF             0x08
#define IIM_ERR_PRGE            (1 << 7)
#define IIM_ERR_WPE         (1 << 6)
#define IIM_ERR_OPE         (1 << 5)
#define IIM_ERR_RPE         (1 << 4)
#define IIM_ERR_WLRE        (1 << 3)
#define IIM_ERR_SNSE        (1 << 2)
#define IIM_ERR_PARITYE     (1 << 1)
#define IIM_EMASK_OFF           0x0C
#define IIM_FCTL_OFF            0x10
#define IIM_UA_OFF              0x14
#define IIM_LA_OFF              0x18
#define IIM_SDAT_OFF            0x1C
#define IIM_PREV_OFF            0x20
#define IIM_SREV_OFF            0x24
#define IIM_PREG_P_OFF          0x28
#define IIM_SCS0_OFF            0x2C
#define IIM_SCS1_P_OFF          0x30
#define IIM_SCS2_OFF            0x34
#define IIM_SCS3_P_OFF          0x38

#define IIM_PROD_REV_SH         3
#define IIM_PROD_REV_LEN        5
#define IIM_SREV_REV_SH         4
#define IIM_SREV_REV_LEN        4
#define PROD_SIGNATURE_MX51     0x1

#define CHIP_REV_1_0            0x10
#define CHIP_REV_1_1            0x11
#define CHIP_REV_2_0            0x20
#define CHIP_REV_2_5		0x25
#define CHIP_REV_3_0            0x30

#define BOARD_REV_1_0           0x0
#define BOARD_REV_2_0           0x1

#define BOARD_VER_OFFSET	0x8

#define NAND_FLASH_BOOT		0x10000000
#define SPI_NOR_FLASH_BOOT	0x80000000
#define MMC_FLASH_BOOT		0x40000000

#ifndef __ASSEMBLER__

enum boot_device {
	UNKNOWN_BOOT = -1,
	NAND_BOOT = 0,
	SPI_NOR_BOOT,
	MMC_BOOT,
	BOOT_DEV_NUM,
};

enum mxc_clock {
	MXC_ARM_CLK = 0,
	MXC_PER_CLK,
	MXC_AHB_CLK,
	MXC_IPG_CLK,
	MXC_IPG_PERCLK,
	MXC_UART_CLK,
	MXC_CSPI_CLK,
	MXC_FEC_CLK,
	MXC_ESDHC_CLK,
	MXC_AXI_A_CLK,
	MXC_AXI_B_CLK,
	MXC_EMI_SLOW_CLK,
	MXC_DDR_CLK
};

/*
enum mxc_main_clocks {
	MXC_CPU_CLK,
	MXC_AHB_CLK,
	MXC_IPG_CLK,
	MXC_IPG_PER_CLK,
	MXC_DDR_CLK,
	MXC_NFC_CLK,
	MXC_USB_CLK,
};
*/

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

#endif				/*  __ASM_ARCH_MXC_MX51_H__ */
