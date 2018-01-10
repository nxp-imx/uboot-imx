/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2017-2018,2020 NXP
 */

#ifndef __ASM_ARCH_SRC_H__
#define __ASM_ARCH_SRC_H__

/*
 * SRC_BMR1 bit fields
 */
#define SRC_BMR1_CFG1_MASK					(0xC0 << 0x0)
#define SRC_BMR1_CFG1_BOOT_SHIFT				(6)
#define SRC_BMR1_CFG1_QuadSPI					(0x0)
#define SRC_BMR1_CFG1_SD					(0x2)
#define SRC_BMR1_CFG1_eMMC					(0x3)

/*
 * SRC_GPR1 bit fields
 */
#define SRC_GPR1_PLL_SOURCE(pll, val) (((val) & SRC_GPR1_PLL_SOURCE_MASK) << \
					(SRC_GPR1_PLL_OFFSET + (pll)))
#define SRC_GPR1_PLL_SOURCE_MASK	(0x1)

#define SRC_GPR1_PLL_OFFSET			(27)
#define SRC_GPR1_FIRC_CLK_SOURCE	(0x0)
#define SRC_GPR1_XOSC_CLK_SOURCE	(0x1)

/* SRC_DDR_SELF_REF_CTRL bit fields */
#define SRC_DDR_EN_SELF_REF_CTRL_DDR0_EN_SLF_REF_RST		BIT(2)
#define SRC_DDR_EN_SELF_REF_CTRL_DDR1_EN_SLF_REF_RST		BIT(3)
#define SRC_DDR_EN_SELF_REF_CTRL_DDR0_SLF_REF_CLR		BIT(1)
#define SRC_DDR_EN_SELF_REF_CTRL_DDR1_SLF_REF_CLR		1

/* SRC registers values */
#define SRC_DDR_EN_SLF_REF_VALUE \
	(SRC_DDR_EN_SELF_REF_CTRL_DDR0_EN_SLF_REF_RST | \
	 SRC_DDR_EN_SELF_REF_CTRL_DDR1_EN_SLF_REF_RST)

/* System Reset Controller (SRC) */
struct src {
	u32 bmr1;
	u32 bmr2;
	u32 gpr1_boot;
	u32 reserved_0x00C[61];
	u32 gpr1;
	u32 gpr2;
	u32 gpr3;
	u32 gpr4;
	u32 gpr5;
	u32 gpr6;
	u32 gpr7;		/* TREERUNNER_GENERATION_1 specific */
	u32 gpr8;		/* TREERUNNER_GENERATION_2 specific */
	u32 reserved_0x120[1];
	u32 gpr10;
	u32 gpr11;
	u32 gpr12;
	u32 gpr13;
	u32 gpr14;
	u32 gpr15;
	u32 gpr16;
	u32 reserved_0x140[1];
	u32 gpr18;
	u32 gpr19;
	u32 gpr20;
	u32 gpr21;
	u32 gpr22;
	u32 gpr23;
	u32 gpr24;
	u32 gpr25;
	u32 gpr26;
	u32 gpr27;
	u32 reserved_0x16C[5];
	u32 pcie_config1;
	u32 ddr_self_ref_ctrl;
	u32 pcie_config0;
	u32 reserved_0x18C[3];
	u32 soc_misc_config2;
};

#endif	/* __ASM_ARCH_SRC_H__ */
