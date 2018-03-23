/*
 * Copyright (C) 2015 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>

#define GPR0           0x0
#define GPR1           0x4
#define GPR2           0x8
#define GPR3           0xC
#define GPR4           0x10
#define GPR5           0x14
#define GPR6           0x18
#define GPR7           0x1C
#define GPR8           0x20
#define GPR9           0x24
#define GPR10          0x28
#define GPR11          0x2C

#define GPR0_CTRL_CLK_EN_LOCK	(1 << 31)
#define GPR0_CTRL_CLK_EN	(1 << 15)
#define GPR0_CTRL_SFTRST_N_LOCK	(1 << 30)
#define GPR0_CTRL_SFTRST	(0 << 14)
#define GPR0_CTRL_SFTRST_N	(1 << 14)
#define GPR0_CTRL_AES_MODE_LOCK	(1 << 29)
#define GPR0_CTRL_AES_MODE_ECB	(0 << 13)
#define GPR0_CTRL_AES_MODE_CTR	(1 << 13)
#define GPR0_SEC_LEVEL_LOCK	(3 << 24)
#define GPR0_SEC_LEVEL		(3 << 8)
#define GPR0_AES_KEY_SEL_LOCK	(1 << 20)
#define GPR0_AES_KEY_SEL_SNVS	(0 << 4)
#define GPR0_AES_KEY_SEL_SOFT	(1 << 4)
#define GPR0_BEE_ENABLE_LOCK	(1 << 16)
#define GPR0_BEE_ENABLE		(1 << 0)

/*
 * SECURITY LEVEL
 *        Non-Secure User |  Non-Secure Spvr | Secure User | Secure Spvr
 * Level
 * (0)00      RD + WR           RD + WR          RD + WR       RD + WR
 * (1)01      None              RD + WR          RD + WR       RD + WR
 * (2)10      None              None             RD + WR       RD + WR
 * (3)11      None              None             None          RD + WR
 */
#define GPR0_SEC_LEVEL_0	(0 << 8)
#define GPR0_SEC_LEVEL_1	(1 << 8)
#define GPR0_SEC_LEVEL_2	(2 << 8)
#define GPR0_SEC_LEVEL_3	(3 << 8)
