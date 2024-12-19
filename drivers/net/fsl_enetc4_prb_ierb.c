// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2023 NXP
 */

#include <asm/io.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <dm/device_compat.h>

#include "fsl_enetc4.h"

#define CFG_LINK_IO_VAR		0xc
#define IO_VAR_16FF_16G_SERDES	0x1
#define IO_VAR(port, var)	(((var) & 0xf) << ((port) << 2))
#define CFG_LINK_MII_PROT	0x10
#define MII_PROT_MII		0x0
#define MII_PROT_RMII		0x1
#define MII_PROT_RGMII		0x2
#define MII_PROT_SERIAL		0x3
#define MII_PROT(port, prot)	(((prot) & 0xf) << ((port) << 2))
#define CFG_LINK_PCS_PROT_0	0x14
#define CFG_LINK_PCS_PROT_1	0x18
#define CFG_LINK_PCS_PROT_2	0x1c
#define PCS_PROT_1G_SGMII	BIT(0)
#define PCS_PROT_2500M_SGMII	BIT(1)
#define PCS_PROT_XFI		BIT(3)
#define PCS_PROT_SFI		BIT(4)
#define PCS_PROT_10G_SXGMII	BIT(6)

#define NETC_LINK_CFG(a)	(0x4cU + (a) * 4)
#define  LINK_IO_VAR		GENMASK(19, 16)
#define  LINK_MII_PORT		GENMASK(3, 0)

#define CFG_LINK_PCS_PROT(a)	(0x14 + (a) * 4)
#define  PCS_PROT_1G_SGMII	BIT(0)
#define  PCS_PROT_2500M_SGMII	BIT(1)
#define  PCS_PROT_XFI		BIT(3)
#define  PCS_PROT_SFI		BIT(4)
#define  PCS_PROT_10G_SXGMII	BIT(6)

#define IERB_CAPR(a)		(0x0 + 0x4 * (a))
#define IERB_ITTMCAPR		0x30
#define IERB_HBTMAR		0x100
#define IERB_EMDIOMCR		0x314
#define IERB_T0MCR		0x414
#define IERB_TGSM0CAPR		0x808
#define IERB_LCAPR(a)		(0x1000 + 0x40 * (a))
#define IERB_LMCAPR(a)		(0x1004 + 0x40 * (a))
#define IERB_LIOCAPR(a)		(0x1008 + 0x40 * (a))
#define IERB_LBCR(a)		(0x1010 + 0x40 * (a))
#define IERB_EBCR1(a)		(0x3004 + 0x100 * (a))
#define IERB_EBCR2(a)		(0x3008 + 0x100 * (a))
#define IERB_EVFRIDAR(a)	(0x3010 + 0x100 * (a))
#define IERB_EMCR(a)		(0x3014 + 0x100 * (a))
#define IERB_EIPFTMAR(a)	(0x3088 + 0x100 * (a))
#define IERB_EMDIOFAUXR		0x344
#define IERB_T0FAUXR		0x444
#define IERB_EFAUXR(a)		(0x3044 + 0x100 * (a))
#define IERB_VFAUXR(a)		(0x4004 + 0x40 * (a))
#define IERB_FAUXR_LDID		GENMASK(3, 0)

#define PRB_NETCRR		0x100
#define NETCRR_SR		BIT(0)
#define NETCRR_LOCK		BIT(1)
#define PRB_NETCSR		0x104
#define NETCSR_ERROR		BIT(0)
#define NETCSR_STATE		BIT(1)

void enetc_blk_ctrl_cfg_imx95(void* base)
{
	/* configure Link I/O variant */
	writel(IO_VAR(2, IO_VAR_16FF_16G_SERDES), base + CFG_LINK_IO_VAR);
	/* configure Link0/1/2 MII port */
	writel(MII_PROT(0, MII_PROT_RGMII) | MII_PROT(1, MII_PROT_RGMII) |
	       MII_PROT(2, MII_PROT_SERIAL), base + CFG_LINK_MII_PROT);
	/* configure Link0/1/2 PCS protocol */
	writel(0, base + CFG_LINK_PCS_PROT_0);
	writel(0, base + CFG_LINK_PCS_PROT_1);
	writel(PCS_PROT_10G_SXGMII, base + CFG_LINK_PCS_PROT_2);
}

static const struct enetc_bl_data enetc_bl_data_imx95 = {
	.blk_ctrl_cfg = enetc_blk_ctrl_cfg_imx95,
};

static int enetc_blk_ctrl_of_to_plat(struct udevice *dev)
{
	fdt_addr_t addr;
	fdt_addr_t size;
	struct enetc_bl_plat *plat = dev_get_plat(dev);

	addr = devfdt_get_addr_size_name(dev, "ierb", &size);
	if (addr == FDT_ADDR_T_NONE) {
		dev_err(dev, "ierb regs missing\n");
		return -ENODEV;
	}
	plat->netc_ierb_base = (void*)addr;

	addr = devfdt_get_addr_size_name(dev, "prb", &size);
	if (addr == FDT_ADDR_T_NONE) {
		dev_err(dev, "ierb regs missing\n");
		return -ENODEV;
	}
        plat->netc_priv_base = (void*)addr;

	addr = devfdt_get_addr_size_name(dev, "netcmix", &size);
	if (addr == FDT_ADDR_T_NONE) {
		dev_err(dev, "ierb regs missing\n");
		return -ENODEV;
	}
        plat->netc_blk_ctrl_base = (void*)addr;

	debug("netcmix=%p netc_ierb_base=%p netc_priv_base=%p\r\n",
	      plat->netc_blk_ctrl_base, plat->netc_ierb_base, plat->netc_priv_base);
        return 0;
}

static const struct udevice_id netc_blk_ctrl_ids[] = {
	{ .compatible = "nxp,imx95-netc-blk-ctrl", .data = (ulong)&enetc_bl_data_imx95 },
	{ }
};

U_BOOT_DRIVER(enetc_blk_ctrl) = {
	.name	= "enetc_blk_ctrl",
	.id	= UCLASS_SYSCON,
	.bind           = dm_scan_fdt_dev,
	.of_match = netc_blk_ctrl_ids,
	.of_to_plat	= enetc_blk_ctrl_of_to_plat,
	.plat_auto	= sizeof(struct enetc_bl_plat),
};
