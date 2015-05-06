/*
 * Copyright (C) 2012-2015 Freescale Semiconductor, Inc.
 * Copyright (c) 2013, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __MX6_COMMON_H
#define __MX6_COMMON_H

#ifndef CONFIG_MX6UL
#define CONFIG_ARM_ERRATA_743622
#if (defined(CONFIG_MX6QP) || defined(CONFIG_MX6Q) ||\
defined(CONFIG_MX6DL)) && !defined(CONFIG_MX6SOLO)
#define CONFIG_ARM_ERRATA_751472
#define CONFIG_ARM_ERRATA_794072
#define CONFIG_ARM_ERRATA_761320
#define CONFIG_ARM_ERRATA_845369
#endif

#ifndef CONFIG_SYS_L2CACHE_OFF
#define CONFIG_SYS_L2_PL310
#define CONFIG_SYS_PL310_BASE	L2_PL310_BASE
#endif

#define CONFIG_MP
#define CONFIG_GPT_TIMER
#else
#define CONFIG_SYSCOUNTER_TIMER
#define CONFIG_SC_TIMER_CLK 8000000 /* 8Mhz */
#endif /* CONFIG_MX6UL */

#define CONFIG_BOARD_POSTCLK_INIT
#define CONFIG_LDO_BYPASS_CHECK
#define CONFIG_MXC_GPT_HCLK
#ifdef CONFIG_MX6QP
#define CONFIG_MX6Q
#endif

#define CONFIG_IMX_THERMAL

#endif
