/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2018 NXP
 */

#ifndef __ASM_ARCH_IMX8_REGS_H__
#define __ASM_ARCH_IMX8_REGS_H__

#include <asm/mach-imx/regs-lcdif.h>

#define ARCH_MXC

#define LPUART_BASE		0x5A060000

#define GPT1_BASE_ADDR		0x5D140000
#define SCU_LPUART_BASE		0x33220000
#define GPIO1_BASE_ADDR		0x5D080000
#define GPIO2_BASE_ADDR		0x5D090000
#define GPIO3_BASE_ADDR		0x5D0A0000
#define GPIO4_BASE_ADDR		0x5D0B0000
#define GPIO5_BASE_ADDR		0x5D0C0000
#define GPIO6_BASE_ADDR		0x5D0D0000
#define GPIO7_BASE_ADDR		0x5D0E0000
#define GPIO8_BASE_ADDR		0x5D0F0000
#define LPI2C1_BASE_ADDR	0x5A800000
#define LPI2C2_BASE_ADDR	0x5A810000
#define LPI2C3_BASE_ADDR	0x5A820000
#define LPI2C4_BASE_ADDR	0x5A830000
#define LPI2C5_BASE_ADDR	0x5A840000

#ifdef CONFIG_IMX8QXP
#define LVDS0_PHYCTRL_BASE	0x56221000
#define LVDS1_PHYCTRL_BASE	0x56241000
#define MIPI0_SS_BASE		0x56220000
#define MIPI1_SS_BASE		0x56240000
#endif

#ifdef CONFIG_IMX8QM
#define LVDS0_PHYCTRL_BASE 0x56241000
#define LVDS1_PHYCTRL_BASE 0x57241000
#define MIPI0_SS_BASE 0x56220000
#define MIPI1_SS_BASE 0x57220000
#endif

#define APBH_DMA_ARB_BASE_ADDR	0x5B810000
#define APBH_DMA_ARB_END_ADDR	0x5B81FFFF
#define MXS_APBH_BASE		APBH_DMA_ARB_BASE_ADDR

#define MXS_GPMI_BASE		(APBH_DMA_ARB_BASE_ADDR + 0x02000)
#define MXS_BCH_BASE		(APBH_DMA_ARB_BASE_ADDR + 0x04000)

#define PASS_OVER_INFO_ADDR	0x0010fe00

#define USB_BASE_ADDR		0x5b0d0000
#define USB_PHY0_BASE_ADDR	0x5b100000
#define USB_PHY1_BASE_ADDR	0x5b110000

#define CAAM_ARB_BASE_ADDR	(0x31800000)
#define CONFIG_SYS_FSL_SEC_ADDR (0x31400000)

#if !(defined(__KERNEL_STRICT_NAMES) || defined(__ASSEMBLY__))
#include <asm/types.h>
#include <stdbool.h>

bool is_usb_boot(void);
void disconnect_from_pc(void);
#define is_boot_from_usb  is_usb_boot

struct usbphy_regs {
	u32	usbphy_pwd;			/* 0x000 */
	u32	usbphy_pwd_set;			/* 0x004 */
	u32	usbphy_pwd_clr;			/* 0x008 */
	u32	usbphy_pwd_tog;			/* 0x00c */
	u32	usbphy_tx;			/* 0x010 */
	u32	usbphy_tx_set;			/* 0x014 */
	u32	usbphy_tx_clr;			/* 0x018 */
	u32	usbphy_tx_tog;			/* 0x01c */
	u32	usbphy_rx;			/* 0x020 */
	u32	usbphy_rx_set;			/* 0x024 */
	u32	usbphy_rx_clr;			/* 0x028 */
	u32	usbphy_rx_tog;			/* 0x02c */
	u32	usbphy_ctrl;			/* 0x030 */
	u32	usbphy_ctrl_set;		/* 0x034 */
	u32	usbphy_ctrl_clr;		/* 0x038 */
	u32	usbphy_ctrl_tog;		/* 0x03c */
	u32	usbphy_status;			/* 0x040 */
	u32	reserved0[3];
	u32	usbphy_debug0;			/* 0x050 */
	u32	usbphy_debug0_set;		/* 0x054 */
	u32	usbphy_debug0_clr;		/* 0x058 */
	u32	usbphy_debug0_tog;		/* 0x05c */
	u32	reserved1[4];
	u32	usbphy_debug1;			/* 0x070 */
	u32	usbphy_debug1_set;		/* 0x074 */
	u32	usbphy_debug1_clr;		/* 0x078 */
	u32	usbphy_debug1_tog;		/* 0x07c */
	u32	usbphy_version;			/* 0x080 */
	u32	reserved2[7];
	u32	usb1_pll_480_ctrl;		/* 0x0a0 */
	u32	usb1_pll_480_ctrl_set;		/* 0x0a4 */
	u32	usb1_pll_480_ctrl_clr;		/* 0x0a8 */
	u32	usb1_pll_480_ctrl_tog;		/* 0x0ac */
	u32	reserved3[4];
	u32	usb1_vbus_detect;		/* 0xc0 */
	u32	usb1_vbus_detect_set;		/* 0xc4 */
	u32	usb1_vbus_detect_clr;		/* 0xc8 */
	u32	usb1_vbus_detect_tog;		/* 0xcc */
	u32	usb1_vbus_det_stat;		/* 0xd0 */
	u32	reserved4[3];
	u32	usb1_chrg_detect;		/* 0xe0 */
	u32	usb1_chrg_detect_set;		/* 0xe4 */
	u32	usb1_chrg_detect_clr;		/* 0xe8 */
	u32	usb1_chrg_detect_tog;		/* 0xec */
	u32	usb1_chrg_det_stat;		/* 0xf0 */
	u32	reserved5[3];
	u32	usbphy_anactrl;			/* 0x100 */
	u32	usbphy_anactrl_set;		/* 0x104 */
	u32	usbphy_anactrl_clr;		/* 0x108 */
	u32	usbphy_anactrl_tog;		/* 0x10c */
	u32	usb1_loopback;			/* 0x110 */
	u32	usb1_loopback_set;		/* 0x114 */
	u32	usb1_loopback_clr;		/* 0x118 */
	u32	usb1_loopback_tog;		/* 0x11c */
	u32	usb1_loopback_hsfscnt;		/* 0x120 */
	u32	usb1_loopback_hsfscnt_set;	/* 0x124 */
	u32	usb1_loopback_hsfscnt_clr;	/* 0x128 */
	u32	usb1_loopback_hsfscnt_tog;	/* 0x12c */
	u32	usphy_trim_override_en;		/* 0x130 */
	u32	usphy_trim_override_en_set;	/* 0x134 */
	u32	usphy_trim_override_en_clr;	/* 0x138 */
	u32	usphy_trim_override_en_tog;	/* 0x13c */
	u32	usb1_pfda_ctrl1;		/* 0x140 */
	u32	usb1_pfda_ctrl1_set;		/* 0x144 */
	u32	usb1_pfda_ctrl1_clr;		/* 0x148 */
	u32	usb1_pfda_ctrl1_tog;		/* 0x14c */
};
#endif

#endif /* __ASM_ARCH_IMX8_REGS_H__ */
