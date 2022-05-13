/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2022 NXP
 */

#ifndef __ASM_ARCH_IMX9_REGS_H__
#define __ASM_ARCH_IMX9_REGS_H__

#define ARCH_MXC
#define FEC_QUIRK_ENET_MAC

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

#define FSB_BASE_ADDR   0x47510000

#define BLK_CTRL_WAKEUPMIX_BASE_ADDR 0x42420000
#if !(defined(__KERNEL_STRICT_NAMES) || defined(__ASSEMBLY__))
#include <asm/types.h>
#include <stdbool.h>

#define BCTRL_GPR_ENET_QOS_INTF_MODE_MASK        GENMASK(3, 1)
#define BCTRL_GPR_ENET_QOS_INTF_SEL_MII          (0x0 << 1)
#define BCTRL_GPR_ENET_QOS_INTF_SEL_RMII         (0x4 << 1)
#define BCTRL_GPR_ENET_QOS_INTF_SEL_RGMII        (0x1 << 1)
#define BCTRL_GPR_ENET_QOS_CLK_GEN_EN            (0x1 << 0)

struct blk_ctrl_wakeupmix_regs {
	u32 upper_addr;
	u32 ipg_debug_cm33;
	u32 reserved[2];
	u32 qch_dis;
	u32 ssi;
	u32 reserved1[1];
	u32 dexsc_err;
	u32 mqs_setting;
	u32 sai_clk_sel;
	u32 eqos_gpr;
	u32 enet_clk_sel;
	u32 reserved2[1];
	u32 volt_detect;
	u32 i3c2_wakeup;
	u32 ipg_debug_ca55c0;
	u32 ipg_debug_ca55c1;
	u32 axi_attr_cfg;
	u32 i3c2_sda_irq;
};

struct mu_type {
	u32 ver;
	u32 par;
	u32 cr;
	u32 sr;
	u32 reserved0[60];
	u32 fcr;
	u32 fsr;
	u32 reserved1[2];
	u32 gier;
	u32 gcr;
	u32 gsr;
	u32 reserved2;
	u32 tcr;
	u32 tsr;
	u32 rcr;
	u32 rsr;
	u32 reserved3[52];
	u32 tr[16];
	u32 reserved4[16];
	u32 rr[16];
	u32 reserved5[14];
	u32 mu_attr;
};

bool is_usb_boot(void);
void disconnect_from_pc(void);
#define is_boot_from_usb  is_usb_boot

#endif

#endif
