/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2022 NXP
 */

#ifndef __ASM_ARCH_IMX9_REGS_H__
#define __ASM_ARCH_IMX9_REGS_H__

#define ARCH_MXC

#define IOMUXC_BASE_ADDR	0x443C0000UL
#define CCM_BASE_ADDR		0x44450000UL
#define CCM_CCGR_BASE_ADDR	0x44458000UL
#define SYSCNT_CTRL_BASE_ADDR	0x44290000

#define WDG3_BASE_ADDR      0x42490000UL
#define WDG4_BASE_ADDR      0x424a0000UL
#define WDG5_BASE_ADDR      0x424b0000UL

#define ANATOP_BASE_ADDR    0x44480000UL

#define USB1_BASE_ADDR		0x4c100000
#define USB2_BASE_ADDR		0x4c200000

#if !(defined(__KERNEL_STRICT_NAMES) || defined(__ASSEMBLY__))
#include <asm/types.h>

#include <stdbool.h>

bool is_usb_boot(void);
void disconnect_from_pc(void);
#define is_boot_from_usb  is_usb_boot

#endif

#endif
