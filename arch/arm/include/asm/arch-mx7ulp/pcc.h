/*
 * Copyright (C) 2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _ASM_ARCH_PCC_H
#define _ASM_ARCH_PCC_H

#include <common.h>
#include <asm/arch/scg.h>

/*------------------------------------------------------------------------------
  * PCC2
  *------------------------------------------------------------------------------
*/

/* On-Platform (32 entries) */

#define RSVD0_PCC2_SLOT				(0)
#define RSVD1_PCC2_SLOT				(1)
#define CA7_GIC_PCC2_SLOT			(2)
#define RSVD3_PCC2_SLOT				(3)
#define RSVD4_PCC2_SLOT				(4)
#define RSVD5_PCC2_SLOT				(5)
#define RSVD6_PCC2_SLOT				(6)
#define RSVD7_PCC2_SLOT				(7)
#define DMA1_PCC2_SLOT				(8)
#define RSVD9_PCC2_SLOT				(9)
#define RSVD10_PCC2_SLOT			(10)
#define RSVD11_PCC2_SLOT			(11)
#define RSVD12_PCC2_SLOT			(12)
#define RSVD13_PCC2_SLOT			(13)
#define RSVD14_PCC2_SLOT			(14)
#define RGPIO1_PCC2_SLOT			(15)
#define FLEXBUS0_PCC2_SLOT			(16)
#define RSVD17_PCC2_SLOT			(17)
#define RSVD18_PCC2_SLOT			(18)
#define RSVD19_PCC2_SLOT			(19)
#define RSVD20_PCC2_SLOT			(20)
#define RSVD21_PCC2_SLOT			(21)
#define RSVD22_PCC2_SLOT			(22)
#define RSVD23_PCC2_SLOT			(23)
#define RSVD24_PCC2_SLOT			(24)
#define RSVD25_PCC2_SLOT			(25)
#define RSVD26_PCC2_SLOT			(26)
#define SEMA42_1_PCC2_SLOT			(27)
#define RSVD28_PCC2_SLOT			(28)
#define RSVD29_PCC2_SLOT			(29)
#define RSVD30_PCC2_SLOT			(30)
#define RSVD31_PCC2_SLOT			(31)

/* Off-Platform (96 entries) */

#define RSVD32_PCC2_SLOT			(32)
#define DMA1_CH_MUX0_PCC2_SLOT		(33)
#define MU_B_PCC2_SLOT				(34)
#define SNVS_PCC2_SLOT				(35)
#define CAAM_PCC2_SLOT				(36)
#define LPTPM4_PCC2_SLOT			(37)
#define LPTPM5_PCC2_SLOT			(38)
#define LPIT1_PCC2_SLOT				(39)
#define RSVD40_PCC2_SLOT			(40)
#define LPSPI2_PCC2_SLOT			(41)
#define LPSPI3_PCC2_SLOT			(42)
#define LPI2C4_PCC2_SLOT			(43)
#define LPI2C5_PCC2_SLOT			(44)
#define LPUART4_PCC2_SLOT			(45)
#define LPUART5_PCC2_SLOT			(46)
#define RSVD47_PCC2_SLOT			(47)
#define RSVD48_PCC2_SLOT			(48)
#define FLEXIO1_PCC2_SLOT			(49)
#define RSVD50_PCC2_SLOT			(50)
#define USBOTG0_PCC2_SLOT			(51)
#define USBOTG1_PCC2_SLOT			(52)
#define USBPHY_PCC2_SLOT			(53)
#define USB_PL301_PCC2_SLOT			(54)
#define USDHC0_PCC2_SLOT			(55)
#define USDHC1_PCC2_SLOT			(56)
#define RSVD57_PCC2_SLOT			(57)
#define TRGMUX1_PCC2_SLOT			(58)
#define RSVD59_PCC2_SLOT			(59)
#define RSVD60_PCC2_SLOT			(60)
#define WDG1_PCC2_SLOT				(61)
#define SCG1_PCC2_SLOT				(62)
#define PCC2_PCC2_SLOT				(63)
#define PMC1_PCC2_SLOT				(64)
#define SMC1_PCC2_SLOT				(65)
#define RCM1_PCC2_SLOT				(66)
#define WDG2_PCC2_SLOT				(67)
#define RSVD68_PCC2_SLOT			(68)
#define TEST_SPACE1_PCC2_SLOT		(69)
#define TEST_SPACE2_PCC2_SLOT		(70)
#define TEST_SPACE3_PCC2_SLOT		(71)
#define RSVD72_PCC2_SLOT			(72)
#define RSVD73_PCC2_SLOT			(73)
#define RSVD74_PCC2_SLOT			(74)
#define RSVD75_PCC2_SLOT			(75)
#define RSVD76_PCC2_SLOT			(76)
#define RSVD77_PCC2_SLOT			(77)
#define RSVD78_PCC2_SLOT			(78)
#define RSVD79_PCC2_SLOT			(79)
#define RSVD80_PCC2_SLOT			(80)
#define RSVD81_PCC2_SLOT			(81)
#define RSVD82_PCC2_SLOT			(82)
#define RSVD83_PCC2_SLOT			(83)
#define RSVD84_PCC2_SLOT			(84)
#define RSVD85_PCC2_SLOT			(85)
#define RSVD86_PCC2_SLOT			(86)
#define RSVD87_PCC2_SLOT			(87)
#define RSVD88_PCC2_SLOT			(88)
#define RSVD89_PCC2_SLOT			(89)
#define RSVD90_PCC2_SLOT			(90)
#define RSVD91_PCC2_SLOT			(91)
#define RSVD92_PCC2_SLOT			(92)
#define RSVD93_PCC2_SLOT			(93)
#define RSVD94_PCC2_SLOT			(94)
#define RSVD95_PCC2_SLOT			(95)
#define RSVD96_PCC2_SLOT			(96)
#define RSVD97_PCC2_SLOT			(97)
#define RSVD98_PCC2_SLOT			(98)
#define RSVD99_PCC2_SLOT			(99)
#define RSVD100_PCC2_SLOT			(100)
#define RSVD101_PCC2_SLOT			(101)
#define RSVD102_PCC2_SLOT			(102)
#define RSVD103_PCC2_SLOT			(103)
#define RSVD104_PCC2_SLOT			(104)
#define RSVD105_PCC2_SLOT			(105)
#define RSVD106_PCC2_SLOT			(106)
#define RSVD107_PCC2_SLOT			(107)
#define RSVD108_PCC2_SLOT			(108)
#define RSVD109_PCC2_SLOT			(109)
#define RSVD110_PCC2_SLOT			(110)
#define RSVD111_PCC2_SLOT			(111)
#define RSVD112_PCC2_SLOT			(112)
#define RSVD113_PCC2_SLOT			(113)
#define RSVD114_PCC2_SLOT			(114)
#define RSVD115_PCC2_SLOT			(115)
#define RSVD116_PCC2_SLOT			(116)
#define RSVD117_PCC2_SLOT			(117)
#define RSVD118_PCC2_SLOT			(118)
#define RSVD119_PCC2_SLOT			(119)
#define RSVD120_PCC2_SLOT			(120)
#define RSVD121_PCC2_SLOT			(121)
#define RSVD122_PCC2_SLOT			(122)
#define RSVD123_PCC2_SLOT			(123)
#define RSVD124_PCC2_SLOT			(124)
#define RSVD125_PCC2_SLOT			(125)
#define RSVD126_PCC2_SLOT			(126)
#define RSVD127_PCC2_SLOT			(127)

/*------------------------------------------------------------------------------
  * PCC3
  *------------------------------------------------------------------------------
*/

/* On-Platform (32 entries) */

#define RSVD0_PCC3_SLOT				(0)
#define RSVD1_PCC3_SLOT				(1)
#define RSVD2_PCC3_SLOT				(2)
#define RSVD3_PCC3_SLOT				(3)
#define RSVD4_PCC3_SLOT				(4)
#define RSVD5_PCC3_SLOT				(5)
#define RSVD6_PCC3_SLOT				(6)
#define RSVD7_PCC3_SLOT				(7)
#define RSVD8_PCC3_SLOT				(8)
#define RSVD9_PCC3_SLOT				(9)
#define RSVD10_PCC3_SLOT			(10)
#define RSVD11_PCC3_SLOT			(11)
#define RSVD12_PCC3_SLOT			(12)
#define RSVD13_PCC3_SLOT			(13)
#define RSVD14_PCC3_SLOT			(14)
#define RSVD15_PCC3_SLOT			(15)
#define ROMCP1_PCC3_SLOT			(16)
#define RSVD17_PCC3_SLOT			(17)
#define RSVD18_PCC3_SLOT			(18)
#define RSVD19_PCC3_SLOT			(19)
#define RSVD20_PCC3_SLOT			(20)
#define RSVD21_PCC3_SLOT			(21)
#define RSVD22_PCC3_SLOT			(22)
#define RSVD23_PCC3_SLOT			(23)
#define RSVD24_PCC3_SLOT			(24)
#define RSVD25_PCC3_SLOT			(25)
#define RSVD26_PCC3_SLOT			(26)
#define RSVD27_PCC3_SLOT			(27)
#define RSVD28_PCC3_SLOT			(28)
#define RSVD29_PCC3_SLOT			(29)
#define RSVD30_PCC3_SLOT			(30)
#define RSVD31_PCC3_SLOT			(31)

/* Off-Platform (96 entries) */

#define RSVD32_PCC3_SLOT			(32)
#define LPTPM6_PCC3_SLOT			(33)
#define LPTPM7_PCC3_SLOT			(34)
#define RSVD35_PCC3_SLOT			(35)
#define LPI2C6_PCC3_SLOT			(36)
#define LPI2C7_PCC3_SLOT			(37)
#define LPUART6_PCC3_SLOT			(38)
#define LPUART7_PCC3_SLOT			(39)
#define VIU0_PCC3_SLOT				(40)
#define DSI0_PCC3_SLOT				(41)
#define LCDIF0_PCC3_SLOT			(42)
#define MMDC0_PCC3_SLOT				(43)
#define IOMUXC1_PCC3_SLOT			(44)
#define IOMUXC_DDR_PCC3_SLOT		(45)
#define PORTC_PCC3_SLOT				(46)
#define PORTD_PCC3_SLOT				(47)
#define PORTE_PCC3_SLOT				(48)
#define PORTF_PCC3_SLOT				(49)
#define RSVD50_PCC3_SLOT			(50)
#define PCC3_PCC3_SLOT				(51)
#define RSVD52_PCC3_SLOT			(52)
#define WKPU_PCC3_SLOT				(53)
#define RSVD54_PCC3_SLOT			(54)
#define RSVD55_PCC3_SLOT			(55)
#define RSVD56_PCC3_SLOT			(56)
#define RSVD57_PCC3_SLOT			(57)
#define RSVD58_PCC3_SLOT			(58)
#define RSVD59_PCC3_SLOT			(59)
#define RSVD60_PCC3_SLOT			(60)
#define RSVD61_PCC3_SLOT			(61)
#define RSVD62_PCC3_SLOT			(62)
#define RSVD63_PCC3_SLOT			(63)
#define RSVD64_PCC3_SLOT			(64)
#define RSVD65_PCC3_SLOT			(65)
#define RSVD66_PCC3_SLOT			(66)
#define RSVD67_PCC3_SLOT			(67)
#define RSVD68_PCC3_SLOT			(68)
#define RSVD69_PCC3_SLOT			(69)
#define RSVD70_PCC3_SLOT			(70)
#define RSVD71_PCC3_SLOT			(71)
#define RSVD72_PCC3_SLOT			(72)
#define RSVD73_PCC3_SLOT			(73)
#define RSVD74_PCC3_SLOT			(74)
#define RSVD75_PCC3_SLOT			(75)
#define RSVD76_PCC3_SLOT			(76)
#define RSVD77_PCC3_SLOT			(77)
#define RSVD78_PCC3_SLOT			(78)
#define RSVD79_PCC3_SLOT			(79)
#define RSVD80_PCC3_SLOT			(80)
#define GPU3D_PCC3_SLOT				(81)
#define GPU2D_PCC3_SLOT				(82)
#define RSVD83_PCC3_SLOT			(83)
#define RSVD84_PCC3_SLOT			(84)
#define RSVD85_PCC3_SLOT			(85)
#define RSVD86_PCC3_SLOT			(86)
#define RSVD87_PCC3_SLOT			(87)
#define RSVD88_PCC3_SLOT			(88)
#define RSVD89_PCC3_SLOT			(89)
#define RSVD90_PCC3_SLOT			(90)
#define RSVD91_PCC3_SLOT			(91)
#define RSVD92_PCC3_SLOT			(92)
#define RSVD93_PCC3_SLOT			(93)
#define RSVD94_PCC3_SLOT			(94)
#define RSVD95_PCC3_SLOT			(95)
#define RSVD96_PCC3_SLOT			(96)
#define RSVD97_PCC3_SLOT			(97)
#define RSVD98_PCC3_SLOT			(98)
#define RSVD99_PCC3_SLOT			(99)
#define RSVD100_PCC3_SLOT			(100)
#define RSVD101_PCC3_SLOT			(101)
#define RSVD102_PCC3_SLOT			(102)
#define RSVD103_PCC3_SLOT			(103)
#define RSVD104_PCC3_SLOT			(104)
#define RSVD105_PCC3_SLOT			(105)
#define RSVD106_PCC3_SLOT			(106)
#define RSVD107_PCC3_SLOT			(107)
#define RSVD108_PCC3_SLOT			(108)
#define RSVD109_PCC3_SLOT			(109)
#define RSVD110_PCC3_SLOT			(110)
#define RSVD111_PCC3_SLOT			(111)
#define RSVD112_PCC3_SLOT			(112)
#define RSVD113_PCC3_SLOT			(113)
#define RSVD114_PCC3_SLOT			(114)
#define RSVD115_PCC3_SLOT			(115)
#define RSVD116_PCC3_SLOT			(116)
#define RSVD117_PCC3_SLOT			(117)
#define RSVD118_PCC3_SLOT			(118)
#define RSVD119_PCC3_SLOT			(119)
#define RSVD120_PCC3_SLOT			(120)
#define RSVD121_PCC3_SLOT			(121)
#define RSVD122_PCC3_SLOT			(122)
#define RSVD123_PCC3_SLOT			(123)
#define RSVD124_PCC3_SLOT			(124)
#define RSVD125_PCC3_SLOT			(125)
#define RSVD126_PCC3_SLOT			(126)
#define RSVD127_PCC3_SLOT			(127)


/* PCC registers */
#define PCC_PR_OFFSET	31
#define PCC_PR_MASK		(0x1 << PCC_PR_OFFSET)
#define PCC_CGC_OFFSET	30
#define PCC_CGC_MASK	(0x1 << PCC_CGC_OFFSET)
#define PCC_INUSE_OFFSET	29
#define PCC_INUSE_MASK		(0x1 << PCC_INUSE_OFFSET)
#define PCC_PCS_OFFSET	24
#define PCC_PCS_MASK	(0x7 << PCC_PCS_OFFSET)
#define PCC_FRAC_OFFSET	3
#define PCC_FRAC_MASK	(0x1 << PCC_FRAC_OFFSET)
#define PCC_PCD_OFFSET	0
#define PCC_PCD_MASK	(0x7 << PCC_PCD_OFFSET)


enum pcc_clksrc_type {
	CLKSRC_PER_PLAT = 0,
	CLKSRC_PER_BUS = 1,
	CLKSRC_NO_PCS = 2,
};

enum pcc_div_type {
	PCC_HAS_DIV,
	PCC_NO_DIV,
};

/* All peripheral clocks on A7 PCCs */
enum pcc_clk {

	/*PCC2 clocks*/
	PER_CLK_DMA1 = 0,
	PER_CLK_RGPIO2P1,
	PER_CLK_FLEXBUS,
	PER_CLK_SEMA42_1,
	PER_CLK_DMA_MUX1,
	PER_CLK_SNVS,
	PER_CLK_CAAM,
	PER_CLK_LPTPM4,
	PER_CLK_LPTPM5,
	PER_CLK_LPIT1,
	PER_CLK_LPSPI2,
	PER_CLK_LPSPI3,
	PER_CLK_LPI2C4,
	PER_CLK_LPI2C5,
	PER_CLK_LPUART4,
	PER_CLK_LPUART5,
	PER_CLK_FLEXIO1,
	PER_CLK_USB0,
	PER_CLK_USB1,
	PER_CLK_USB_PHY,
	PER_CLK_USB_PL301,
	PER_CLK_USDHC0,
	PER_CLK_USDHC1,
	PER_CLK_WDG1,
	PER_CLK_WDG2,

	/*PCC3 clocks*/
	PER_CLK_LPTPM6,
	PER_CLK_LPTPM7,
	PER_CLK_LPI2C6,
	PER_CLK_LPI2C7,
	PER_CLK_LPUART6,
	PER_CLK_LPUART7,
	PER_CLK_VIU,
	PER_CLK_DSI,
	PER_CLK_LCDIF,
	PER_CLK_MMDC,
	PER_CLK_PCTLC,
	PER_CLK_PCTLD,
	PER_CLK_PCTLE,
	PER_CLK_PCTLF,
	PER_CLK_GPU3D,
	PER_CLK_GPU2D,
};


/* This structure keeps info for each pcc slot */
struct pcc_entry {
	u32 pcc_base;
	u32 pcc_slot;
	enum pcc_clksrc_type clksrc;
	enum pcc_div_type div;
};

int pcc_clock_enable(enum pcc_clk clk, bool enable);
int pcc_clock_sel(enum pcc_clk clk, enum scg_clk src);
int pcc_clock_div_config(enum pcc_clk clk, bool frac, u8 div);
bool pcc_clock_is_enable(enum pcc_clk clk);
int pcc_clock_get_clksrc(enum pcc_clk clk, enum scg_clk *src);
u32 pcc_clock_get_rate(enum pcc_clk clk);

#endif
