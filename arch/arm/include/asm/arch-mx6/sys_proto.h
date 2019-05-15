/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2009
 * Stefano Babic, DENX Software Engineering, sbabic@denx.de.
 * Copyright (C) 2015-2016 Freescale Semiconductor, Inc.
 * Copyright 2018 NXP
 */

#ifndef __SYS_PROTO_IMX6_
#define __SYS_PROTO_IMX6_

#include <asm/mach-imx/sys_proto.h>
#include <asm/arch/iomux.h>
#include <asm/arch/module_fuse.h>

#define USBPHY_PWD		0x00000000

#define USBPHY_PWD_RXPWDRX	(1 << 20) /* receiver block power down */

#define is_usbotg_phy_active(void) (!(readl(USB_PHY0_BASE_ADDR + USBPHY_PWD) & \
				   USBPHY_PWD_RXPWDRX))

int imx6_pcie_toggle_power(void);
int imx6_pcie_toggle_reset(void);

enum ldo_reg {
	LDO_ARM,
	LDO_SOC,
	LDO_PU,
};

int set_ldo_voltage(enum ldo_reg ldo, u32 mv);

/**
 * iomuxc_set_rgmii_io_voltage - set voltage level of RGMII/USB pins
 *
 * @param io_vol - the voltage IO level of pins
 */
static inline void iomuxc_set_rgmii_io_voltage(int io_vol)
{
	__raw_writel(io_vol, IOMUXC_SW_PAD_CTL_GRP_DDR_TYPE_RGMII);
}

void set_wdog_reset(struct wdog_regs *wdog);
enum boot_device get_boot_device(void);

#ifdef CONFIG_LDO_BYPASS_CHECK
int check_ldo_bypass(void);
int check_1_2G(void);
int set_anatop_bypass(int wdog_reset_pin);
void ldo_mode_set(int ldo_bypass);
void prep_anatop_bypass(void);
void finish_anatop_bypass(void);
#endif

#endif /* __SYS_PROTO_IMX6_ */
