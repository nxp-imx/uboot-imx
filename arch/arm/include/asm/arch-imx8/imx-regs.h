/*
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
 */

#ifndef __ASM_ARCH_IMX_REGS_H__
#define __ASM_ARCH_IMX_REGS_H__

#define MU_BASE_ADDR(id)	((0x5D1B0000UL + (id*0x10000)))

#define LPUART_BASE			0x5A060000

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

#ifdef CONFIG_LPUART
#define LPUART_BASE		SCU_LPUART_BASE
#endif

#define ROM_SW_INFO_ADDR	0x00000890

#define USB_BASE_ADDR		0x5b0d0000
#define USB_PHY0_BASE_ADDR	0x5b100000

#if !(defined(__KERNEL_STRICT_NAMES) || defined(__ASSEMBLY__))
#include <asm/types.h>

/*
 * If ROM fail back to USB recover mode, USBPH0_PWD will be clear to use USB
 * If boot from the other mode, USB0_PWD will keep reset value
 */
#define	is_boot_from_usb(void) (!(readl(USB_PHY0_BASE_ADDR) & (1<<20)))
#define	disconnect_from_pc(void) writel(0x0, USB_BASE_ADDR + 0x140)

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

#endif /* __ASM_ARCH_IMX_REGS_H__ */
