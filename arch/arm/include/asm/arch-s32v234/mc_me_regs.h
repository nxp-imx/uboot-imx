/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2015-2016 Freescale Semiconductor, Inc.
 * (C) Copyright 2020 NXP
 *
 */

#ifndef __ARCH_ARM_MACH_S32V234_MCME_REGS_H__
#define __ARCH_ARM_MACH_S32V234_MCME_REGS_H__

#ifndef __ASSEMBLY__

/* MC_ME registers definitions */

/* MC_ME_GS */
#define MC_ME_GS						(MC_ME_BASE_ADDR + 0x00000000)

#define MC_ME_GS_S_SYSCLK_FIRC			(0x0 << 0)
#define MC_ME_GS_S_SYSCLK_FXOSC			(0x1 << 0)
#define MC_ME_GS_S_SYSCLK_ARMPLL		(0x2 << 0)
#define MC_ME_GS_S_STSCLK_DISABLE		(0xF << 0)
#define MC_ME_GS_S_FIRC				BIT(4)
#define MC_ME_GS_S_XOSC				BIT(5)
#define MC_ME_GS_S_ARMPLL			BIT(6)
#define MC_ME_GS_S_PERPLL			BIT(7)
#define MC_ME_GS_S_ENETPLL			BIT(8)
#define MC_ME_GS_S_DDRPLL			BIT(9)
#define MC_ME_GS_S_VIDEOPLL			BIT(10)
#define MC_ME_GS_S_MVR				BIT(20)
#define MC_ME_GS_S_PDO				BIT(23)
#define MC_ME_GS_S_MTRANS			BIT(27)
#define MC_ME_GS_S_CRT_MODE_RESET		(0x0 << 28)
#define MC_ME_GS_S_CRT_MODE_TEST		(0x1 << 28)
#define MC_ME_GS_S_CRT_MODE_DRUN		(0x3 << 28)
#define MC_ME_GS_S_CRT_MODE_RUN0		(0x4 << 28)
#define MC_ME_GS_S_CRT_MODE_RUN1		(0x5 << 28)
#define MC_ME_GS_S_CRT_MODE_RUN2		(0x6 << 28)
#define MC_ME_GS_S_CRT_MODE_RUN3		(0x7 << 28)

/* MC_ME_MCTL */
#define MC_ME_MCTL						(MC_ME_BASE_ADDR + 0x00000004)

#define MC_ME_MCTL_KEY				(0x00005AF0)
#define MC_ME_MCTL_INVERTEDKEY			(0x0000A50F)
#define MC_ME_MCTL_RESET			(0x0 << 28)
#define MC_ME_MCTL_TEST				(0x1 << 28)
#define MC_ME_MCTL_DRUN				(0x3 << 28)
#define MC_ME_MCTL_RUN0				(0x4 << 28)
#define MC_ME_MCTL_RUN1				(0x5 << 28)
#define MC_ME_MCTL_RUN2				(0x6 << 28)
#define MC_ME_MCTL_RUN3				(0x7 << 28)

/* MC_ME_ME */
#define MC_ME_ME						(MC_ME_BASE_ADDR + 0x00000008)

#define MC_ME_ME_RESET_FUNC			BIT(0)
#define MC_ME_ME_TEST				BIT(1)
#define MC_ME_ME_DRUN				BIT(3)
#define MC_ME_ME_RUN0				BIT(4)
#define MC_ME_ME_RUN1				BIT(5)
#define MC_ME_ME_RUN2				BIT(6)
#define MC_ME_ME_RUN3				BIT(7)

/* MC_ME_RUN_PCn */
#define MC_ME_RUN_PCn(n)			(MC_ME_BASE_ADDR + \
						 0x00000080 + 0x4 * (n))

#define MC_ME_RUN_PCn_RESET			BIT(0)
#define MC_ME_RUN_PCn_TEST			BIT(1)
#define MC_ME_RUN_PCn_DRUN			BIT(3)
#define MC_ME_RUN_PCn_RUN0			BIT(4)
#define MC_ME_RUN_PCn_RUN1			BIT(5)
#define MC_ME_RUN_PCn_RUN2			BIT(6)
#define MC_ME_RUN_PCn_RUN3			BIT(7)

/*
 * MC_ME_RESET_MC/MC_ME_TEST_MC
 * MC_ME_DRUN_MC
 * MC_ME_RUNn_MC
 */
#define MC_ME_RESET_MC		(MC_ME_BASE_ADDR + 0x00000020)
#define MC_ME_TEST_MC		(MC_ME_BASE_ADDR + 0x00000024)
#define MC_ME_DRUN_MC		(MC_ME_BASE_ADDR + 0x0000002C)
#define MC_ME_RUNn_MC(n)	(MC_ME_BASE_ADDR + 0x00000030 + 0x4 * (n))

#define MC_ME_RUNMODE_MC_SYSCLK(val)	(MC_ME_RUNMODE_MC_SYSCLK_MASK & (val))
#define MC_ME_RUNMODE_MC_SYSCLK_MASK		(0x0000000F)
#define MC_ME_RUNMODE_MC_FIRCON			(1 << 4)
#define MC_ME_RUNMODE_MC_XOSCON			(1 << 5)
#define MC_ME_RUNMODE_MC_PLL(pll)		(1 << (6 + (pll)))
#define MC_ME_RUNMODE_MC_MVRON			(1 << 20)
#define MC_ME_RUNMODE_MC_PDO			(1 << 23)
#define MC_ME_RUNMODE_MC_PWRLVL0		(1 << 28)
#define MC_ME_RUNMODE_MC_PWRLVL1		(1 << 29)
#define MC_ME_RUNMODE_MC_PWRLVL2		(1 << 30)

#define DRUN_MC_RESETVAL			(0x00100010)
#define SYSCLK_FXOSC				(1 << 0)
#define SYSCLK_ARM_PLL_DFS_1			BIT(1)

/* MC_ME_DRUN_SEC_CC_I */
#define MC_ME_DRUN_SEC_CC_I			(MC_ME_BASE_ADDR + 0x260)
/* MC_ME_RUNn_SEC_CC_I */
#define MC_ME_RUNn_SEC_CC_I(n)	(MC_ME_BASE_ADDR + 0x270 + (n) * 0x10)
#define MC_ME_RUNMODE_SEC_CC_I_SYSCLK(val,offset)	((MC_ME_RUNMODE_SEC_CC_I_SYSCLK_MASK & (val)) << offset)
#define MC_ME_RUNMODE_SEC_CC_I_SYSCLK1_OFFSET	(4)
#define MC_ME_RUNMODE_SEC_CC_I_SYSCLK2_OFFSET	(8)
#define MC_ME_RUNMODE_SEC_CC_I_SYSCLK3_OFFSET	(12)
#define MC_ME_RUNMODE_SEC_CC_I_SYSCLK_MASK	(0x3)

/*
 * ME_PCTLn
 * Please note that these registers are 8 bits width, so
 * the operations over them should be done using 8 bits operations.
 */
#define MC_ME_PCTLn_RUNPCm(n)		((n) & MC_ME_PCTLn_RUNPCm_MASK)
#define MC_ME_PCTLn_RUNPCm_MASK		(0x7)

#define MC_ME_PCTLn(n)		(MC_ME_BASE_ADDR + 0xC0 + 4 * ((n) >> 2) \
				 + (3 - (n) % 4))

/* Peripherals PCTL indexes */
#define DEC200_PCTL     39
#define DCU_PCTL        40
#define CSI0_PCTL       48
#define DMACHMUX0_PCTL  49
#define ENET_PCTL       50
#define FRAY_PCTL       52
#define MMDC0_PCTL      54
#define PIT0_PCTL       58
#define SARADC0_PCTL    77
#define FlexTIMER0_PCTL 79
#define IIC0_PCTL       81
#define LINFLEX0_PCTL   83
#define CANFD0_PCTL     85
#define DSPI0_PCTL      87
#define DSPI2_PCTL      89
#define CRC0_PCTL       91
#define SDHC_PCTL       93
#define VIU0_PCTL       100
#define HPSMI_PCTL      104
#define SIPI_PCTL       116
#define LFAST_PCTL      120
#define CSI1_PCTL       160
#define DMACHMUX1_PCTL  161
#define MMDC1_PCTL      162
#define QUADSPI0_PCTL   166
#define PIT1_PCTL       170
#define FlexTIMER1_PCTL 182
#define IIC1_PCTL       184
#define IIC2_PCTL       186
#define LINFLEX1_PCTL   188
#define CANFD1_PCTL     190
#define DSPI1_PCTL      192
#define DSPI3_PCTL      194
#define CRC1_PCTL       204
#define TSENS_PCTL      206
#define VIU1_PCTL       208
#define JPEG_PCTL       212
#define H264_DEC_PCTL   216
#define H264_ENC_PCTL   220
#define MBIST_PCTL      236

/* Core status register */
#define MC_ME_CS               (MC_ME_BASE_ADDR + 0x000001C0)

/* Cortex-M4 Core Control Register */
#define MC_MC_CCTL0            (MC_ME_BASE_ADDR + 0x000001C6)
/* Cortex-A53 - Core 0 Core Control Register */
#define MC_ME_CCTL1            (MC_ME_BASE_ADDR + 0x000001C4)
/* Cortex-A53 - Core 1 Core Control Register */
#define MC_ME_CCTL2            (MC_ME_BASE_ADDR + 0x000001CA)
/* Cortex-A53 - Core 2 Core Control Register */
#define MC_ME_CCTL3            (MC_ME_BASE_ADDR + 0x000001C8)
/* Cortex-A53 - Core 3 Control Register */
#define MC_ME_CCTL4            (MC_ME_BASE_ADDR + 0x000001CE)

#define MC_ME_CCTL_DEASSERT_CORE       (0xFA)

/* Cortex-M4 Core Address Register */
#define MC_ME_CADDR0   (MC_ME_BASE_ADDR + 0x000001E0)
/* Cortex-A53 Core 0 - Core Address Register */
#define MC_ME_CADDR1   (MC_ME_BASE_ADDR + 0x000001E4)
/* Cortex-A53 Core 1 - Core Address Register */
#define MC_ME_CADDR2   (MC_ME_BASE_ADDR + 0x000001E8)
/* Cortex-A53 Core 2 - Core Address Register */
#define MC_ME_CADDR3   (MC_ME_BASE_ADDR + 0x000001EC)
/* Cortex-A53 Core 3 - Core Address Register */
#define MC_ME_CADDR4   (MC_ME_BASE_ADDR + 0x000001F0)

#define MC_ME_CADDRn_ADDR_EN	BIT(0)

/* Default used values */
#define CFG_RUN_PC	MC_ME_PCTLn_RUNPCm(1)

#endif

#endif /*__ARCH_ARM_MACH_S32V234_MCME_REGS_H__ */
