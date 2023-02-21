// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2023 NXP
 */

#include <asm/io.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include "fsl_enetc4.h"

#define NETCMIX_BLK_CTRL_BASE	0x4c810000
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

#define NETC_IERB_BASE		0x4cde0000
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

#define NETC_PRIV_BASE		0x4cdf0000
#define PRB_NETCRR		0x100
#define NETCRR_SR		BIT(0)
#define NETCRR_LOCK		BIT(1)
#define PRB_NETCSR		0x104
#define NETCSR_ERROR		BIT(0)
#define NETCSR_STATE		BIT(1)

void enetc4_netcmix_blk_ctrl_cfg(void)
{
	/* configure Link I/O variant */
	writel(IO_VAR(2, IO_VAR_16FF_16G_SERDES), NETCMIX_BLK_CTRL_BASE + CFG_LINK_IO_VAR);
	/* configure Link0/1/2 MII port */
	writel(MII_PROT(0, MII_PROT_RGMII) | MII_PROT(1, MII_PROT_RGMII) |
	       MII_PROT(2, MII_PROT_SERIAL), NETCMIX_BLK_CTRL_BASE + CFG_LINK_MII_PROT);
	/* configure Link0/1/2 PCS protocol */
	writel(0, NETCMIX_BLK_CTRL_BASE + CFG_LINK_PCS_PROT_0);
	writel(0, NETCMIX_BLK_CTRL_BASE + CFG_LINK_PCS_PROT_1);
	writel(PCS_PROT_2500M_SGMII, NETCMIX_BLK_CTRL_BASE + CFG_LINK_PCS_PROT_2);
}

static bool enetc4_ierb_is_locked(void)
{
	u32 val;

	val = readl(NETC_PRIV_BASE + PRB_NETCRR);
	if (val & NETCRR_LOCK)
		return true;

	return false;
}

static int enetc4_lock_ierb(void)
{
	int timeout = 100;
	u32 val;

	writel(NETCRR_LOCK, NETC_PRIV_BASE + PRB_NETCRR);
	do {
		val = readl(NETC_PRIV_BASE + PRB_NETCSR);
		if (!(val & NETCSR_STATE))
			break;
		udelay(1100);
		timeout -= 1;
	} while (timeout);

	if (!timeout)
		return -ETIMEDOUT;

	return 0;
}

static int enetc4_unlock_ierb_with_warm_reset(void)
{
	int timeout = 100;
	u32 val;

	writel(0, NETC_PRIV_BASE + PRB_NETCRR);
	do {
		val = readl(NETC_PRIV_BASE + PRB_NETCRR);
		if (!(val & NETCRR_LOCK))
			break;
		udelay(1100);
		timeout -= 1;
	} while (timeout);

	if (!timeout)
		return -ETIMEDOUT;

	return 0;
}

/* configure Local Domain Identifier */
static void enetc4_ierb_init_ldid(void)
{
	writel(0, NETC_IERB_BASE + IERB_EMDIOFAUXR);
	writel(7, NETC_IERB_BASE + IERB_T0FAUXR);
	writel(0, NETC_IERB_BASE + IERB_EFAUXR(0));
	writel(3, NETC_IERB_BASE + IERB_EFAUXR(1));
	writel(6, NETC_IERB_BASE + IERB_EFAUXR(2));
	writel(1, NETC_IERB_BASE + IERB_VFAUXR(0));
	writel(2, NETC_IERB_BASE + IERB_VFAUXR(1));
	writel(4, NETC_IERB_BASE + IERB_VFAUXR(2));
	writel(5, NETC_IERB_BASE + IERB_VFAUXR(3));
	writel(0, NETC_IERB_BASE + IERB_VFAUXR(4));
	writel(0, NETC_IERB_BASE + IERB_VFAUXR(5));
}

int enetc4_ierb_cfg(void)
{
	int err;

	if (enetc4_ierb_is_locked()) {
		err = enetc4_unlock_ierb_with_warm_reset();
		if (err) {
			printf("Unlock IERB with warm reset failed.\n");
			return err;
		}
	}

	enetc4_ierb_init_ldid();

	err = enetc4_lock_ierb();
	if (err) {
		printf("Lock IERB failed.\n");
		return err;
	}

	return 0;
}

int enetc4_ierb_cfg_is_valid(void)
{
	u32 val;

	val = readl(NETC_PRIV_BASE + PRB_NETCSR);
	if (val & NETCSR_ERROR)
		return -1;

	return 0;
}
