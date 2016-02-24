/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2017-2018 NXP
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

/* SRC_DDR_SELF_REF_CTRL bit fields */
#define SRC_DDR_EN_SELF_REF_CTRL_DDR0_EN_SLF_REF_RST		BIT(2)
#define SRC_DDR_EN_SELF_REF_CTRL_DDR1_EN_SLF_REF_RST		BIT(3)
#define SRC_DDR_EN_SELF_REF_CTRL_DDR0_SLF_REF_CLR		BIT(1)
#define SRC_DDR_EN_SELF_REF_CTRL_DDR1_SLF_REF_CLR		1

/* SRC registers values */
#define SRC_DDR_EN_SLF_REF_VALUE \
	(SRC_DDR_EN_SELF_REF_CTRL_DDR0_EN_SLF_REF_RST | \
	 SRC_DDR_EN_SELF_REF_CTRL_DDR1_EN_SLF_REF_RST)

#endif  /* __ASM_ARCH_SRC_H__ */
