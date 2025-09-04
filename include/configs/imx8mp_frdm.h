/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2025 NXP
 */

#ifndef __IMX8MP_FRDM_H
#define __IMX8MP_FRDM_H

#include <linux/sizes.h>
#include <linux/stringify.h>
#include <asm/arch/imx-regs.h>
#include <env/nxp/imx_env.h>

#if defined(CONFIG_CMD_NET)
#define CFG_FEC_MXC_PHYADDR          1

#endif

/* Link Definitions */

#define CFG_SYS_INIT_RAM_ADDR	0x40000000
#define CFG_SYS_INIT_RAM_SIZE	0x80000

/* Totally 6GB DDR */
#define CFG_SYS_SDRAM_BASE		0x40000000
#define PHYS_SDRAM			0x40000000
#define PHYS_SDRAM_SIZE			0xC0000000	/* 3 GB */
#define PHYS_SDRAM_2			0x100000000
#define PHYS_SDRAM_2_SIZE		0x40000000	/* 1 GB */

#define CFG_MXC_UART_BASE		UART2_BASE_ADDR

#define CFG_SYS_NAND_BASE           0x20000000

#ifdef CONFIG_ANDROID_SUPPORT
#include "imx8mp_frdm_android.h"
#endif

#endif
