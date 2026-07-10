// SPDX-License-Identifier: GPL-2.0+
/*
 * MaxLinear GPY215 2.5G Ethernet PHY driver for U-Boot
 *
 * Based on Ethernet Network Connection GPY215 (GPY215B1VI, GPY215C0VI) Data
 * Sheet Revision 1.4
 * Reference ID 617800
 *
 * Copyright 2026 NXP
 */

#include <phy.h>
#include <linux/bitops.h>
#include <linux/delay.h>

/* -----------------------------------------------------------------------
 * Standard MDIO registers (Clause 22, Device 0)
 * -----------------------------------------------------------------------
 */
#define GPY215_STD_CTRL			0x00
#define   STD_CTRL_RST			BIT(15)	/* Self-clearing reset        */
#define   STD_CTRL_LB			BIT(14)	/* Loopback                   */
#define   STD_CTRL_SSL			BIT(13)	/* Speed select LSB           */
#define   STD_CTRL_ANEN			BIT(12)	/* Auto-negotiation enable    */
#define   STD_CTRL_PD			BIT(11)	/* Power down (SLEEP)         */
#define   STD_CTRL_ANRS			BIT(9)	/* Restart auto-negotiation   */
#define   STD_CTRL_DPLX			BIT(8)	/* Duplex (1=full)            */
#define   STD_CTRL_SSM			BIT(6)	/* Speed select MSB           */

#define GPY215_STD_STAT			0x01
#define   STD_STAT_LS			BIT(2)	/* Link status (latching low) */
#define   STD_STAT_ANEG_DONE		BIT(5)	/* Auto-neg complete          */

#define GPY215_STD_PHYID1		0x02	/* OUI bits [3:18]  = 0x67C9  */
#define GPY215_STD_PHYID2		0x03	/* OUI bits [19:24] + model + rev */

/* PHY ID: OUI = 0x67C9 (bits 3:18) | 0xDC (bits 19:24)
 * PHYID1 = 0x67C9, PHYID2 = 0xDCxx (model/rev in lower 10 bits)
 */
#define GPY215_PHY_ID1			0x67C9
#define GPY215_PHY_ID2_MASK		0xFC00	/* OUI[19:24] only */
#define GPY215_PHY_ID2_VAL		0xDC00

/* -----------------------------------------------------------------------
 * GPY-specific registers (Clause 22, Device 0)
 * -----------------------------------------------------------------------
 */
#define GPY215_PHY_MIISTAT		0x18	/* Register 0.24              */
#define   MIISTAT_LS			BIT(10)	/* TPI link status            */
#define   MIISTAT_DPX			BIT(3)	/* Duplex (1=full)            */
#define   MIISTAT_SPEED_MASK		GENMASK(2, 0)
#define   MIISTAT_SPEED_10		0x0
#define   MIISTAT_SPEED_100		0x1
#define   MIISTAT_SPEED_1000		0x2
#define   MIISTAT_SPEED_ANEG		0x3
#define   MIISTAT_SPEED_2500		0x4

#define GPY215_PHY_IMASK		0x19	/* Register 0.25              */
#define   IMASK_LSTC			BIT(0)	/* Link state change          */
#define   IMASK_LSPC			BIT(1)	/* Link speed change          */

#define GPY215_PHY_ISTAT		0x1A	/* Register 0.26 (clr-on-rd) */
#define   ISTAT_LSTC			BIT(0)
#define   ISTAT_LSPC			BIT(1)

/* -----------------------------------------------------------------------
 * Vendor Specific 1 registers (MMD device 30)
 * Accessed via Clause 45 or Clause 22 Extended indirect
 * -----------------------------------------------------------------------
 */
#define GPY215_MMD_VSPEC1		30	/* MMD device address         */

#define GPY215_VSPEC1_SGMII_CTRL	0x08	/* Register 30.8              */
#define   SGMII_CTRL_RST		BIT(15)	/* SGMII reset (self-clear)   */
#define   SGMII_CTRL_LB			BIT(14)	/* SGMII loopback             */
#define   SGMII_CTRL_ANEN		BIT(12)	/* SGMII ANEG enable          */
#define   SGMII_CTRL_PD			BIT(11)	/* SGMII power down           */
#define   SGMII_CTRL_RXINV		BIT(10)	/* Invert RX0_M/RX0_P         */
#define   SGMII_CTRL_EEE_CAP		BIT(7)	/* Advertise EEE in ANEG      */
#define   SGMII_CTRL_FIXED2G5		BIT(5)	/* Force SGMII to 2.5G        */
#define   SGMII_CTRL_ANMODE_MASK	GENMASK(1, 0)
#define   SGMII_CTRL_ANMODE_1000BX	0x1	/* IEEE 1000Bx Clause 37      */
#define   SGMII_CTRL_ANMODE_CIS_PHY	0x2	/* Cisco SGMII, PHY side      */
#define   SGMII_CTRL_ANMODE_CIS_MAC	0x3	/* Cisco SGMII, MAC side      */
/* Default reset value = 0x34DA:
 *   ANEN=1, ANMODE=AN_CIS_PHY(2), FIXED2G5=0
 */

#define GPY215_VSPEC1_SGMII_STAT	0x09	/* Register 30.9 (read-only)  */
#define   SGMII_STAT_ANOK		BIT(5)	/* ANEG completed             */
#define   SGMII_STAT_RF			BIT(4)	/* Remote fault (latch high)  */
#define   SGMII_STAT_ANAB		BIT(3)	/* ANEG ability (static)      */
#define   SGMII_STAT_LS			BIT(2)	/* SGMII link up (latch low)  */
#define   SGMII_STAT_DR_MASK		GENMASK(1, 0)
#define   SGMII_STAT_DR_10		0x0
#define   SGMII_STAT_DR_100		0x1
#define   SGMII_STAT_DR_1G		0x2
#define   SGMII_STAT_DR_2G5		0x3

/* -----------------------------------------------------------------------
 * Timing / retry constants
 * -----------------------------------------------------------------------
 */
#define GPY215_RESET_TIMEOUT_MS		500
#define GPY215_ANEG_TIMEOUT_MS		5000
#define GPY215_POLL_INTERVAL_MS		10

/**
 * gpy215_probe - Verify PHY identity
 */
static int gpy215_probe(struct phy_device *phydev)
{
	int id1, id2;

	id1 = phy_read(phydev, MDIO_DEVAD_NONE, GPY215_STD_PHYID1);
	id2 = phy_read(phydev, MDIO_DEVAD_NONE, GPY215_STD_PHYID2);

	if (id1 < 0 || id2 < 0) {
		printf("GPY215: MDIO read failed (id1=%d id2=%d)\n", id1, id2);
		return -EIO;
	}

	if (id1 != GPY215_PHY_ID1 ||
	    (id2 & GPY215_PHY_ID2_MASK) != GPY215_PHY_ID2_VAL) {
		printf("GPY215: unexpected PHY ID 0x%04X 0x%04X\n", id1, id2);
		return -ENODEV;
	}

	debug("GPY215: detected (ID1=0x%04X ID2=0x%04X)\n", id1, id2);
	return 0;
}

/**
 * gpy215_config - Configure PHY for SGMII operation
 *
 * The GPY215 SGMII interface comes up automatically after reset with:
 *   - SGMII ANEG enabled (Cisco SGMII, PHY side)
 *   - PHY side SGMII speed tracks TPI link speed
 */
static int gpy215_config(struct phy_device *phydev)
{
	int val, timeout;

	val = phy_read(phydev, MDIO_DEVAD_NONE, GPY215_STD_CTRL);
	if (val < 0)
		return val;

	phy_write(phydev, MDIO_DEVAD_NONE, GPY215_STD_CTRL,
		  val | STD_CTRL_RST);

	/* Wait for self-clearing RST bit */
	timeout = GPY215_RESET_TIMEOUT_MS / GPY215_POLL_INTERVAL_MS;
	do {
		mdelay(GPY215_POLL_INTERVAL_MS);
		val = phy_read(phydev, MDIO_DEVAD_NONE, GPY215_STD_CTRL);
		if (val < 0)
			return val;
	} while ((val & STD_CTRL_RST) && --timeout);

	if (val & STD_CTRL_RST) {
		printf("GPY215: reset timeout\n");
		return -ETIMEDOUT;
	}

	/* Clear stale interrupt status (read-to-clear) */
	phy_read(phydev, MDIO_DEVAD_NONE, GPY215_PHY_ISTAT);

	val = phy_read(phydev, MDIO_DEVAD_NONE, GPY215_STD_CTRL);
	if (val < 0)
		return val;

	/* Enable ANEG, clear forced speed/duplex bits */
	val |= STD_CTRL_ANEN;
	val &= ~(STD_CTRL_SSL | STD_CTRL_SSM | STD_CTRL_PD);
	phy_write(phydev, MDIO_DEVAD_NONE, GPY215_STD_CTRL, val);

	phy_write(phydev, MDIO_DEVAD_NONE, GPY215_STD_CTRL,
		  val | STD_CTRL_ANRS);

	/* Configure SGMII (Cisco ANEG, PHY side, ANEG enabled) */
	val = phy_read_mmd(phydev, GPY215_MMD_VSPEC1,
			   GPY215_VSPEC1_SGMII_CTRL);
	if (val < 0)
		return val;

	val &= ~SGMII_CTRL_PD;
	val |= SGMII_CTRL_ANEN;
	val &= ~SGMII_CTRL_ANMODE_MASK;
	val |= SGMII_CTRL_ANMODE_CIS_PHY;
	val &= ~SGMII_CTRL_FIXED2G5;
	phy_write_mmd(phydev, GPY215_MMD_VSPEC1,
		      GPY215_VSPEC1_SGMII_CTRL, val);

	debug("GPY215: configured for SGMII (Cisco ANEG, PHY side)\n");
	return 0;
}

/**
 * gpy215_startup - Wait for link and report status
 *
 * Polls PHY_MIISTAT (TPI link) and VSPEC1_SGMII_STAT (SGMII link).
 * Updates phydev->speed, phydev->duplex, phydev->link.
 */
static int gpy215_startup(struct phy_device *phydev)
{
	int miistat, sgmii_stat, val;
	int timeout = GPY215_ANEG_TIMEOUT_MS / GPY215_POLL_INTERVAL_MS;
	int speed_code;

	debug("GPY215: waiting for link");

	/* Poll TPI link status */
	do {
		mdelay(GPY215_POLL_INTERVAL_MS);
		miistat = phy_read(phydev, MDIO_DEVAD_NONE,
				   GPY215_PHY_MIISTAT);
		if (miistat < 0)
			return miistat;
	} while (!(miistat & MIISTAT_LS) && --timeout);

	if (!(miistat & MIISTAT_LS)) {
		printf("GPY215: link down (TPI timeout)\n");
		phydev->link = 0;
		return 0;
	}

	do {
		mdelay(GPY215_POLL_INTERVAL_MS);
		val = phy_read(phydev, MDIO_DEVAD_NONE, GPY215_STD_STAT);
		if (val < 0)
			return val;
	} while (!(val & STD_STAT_ANEG_DONE) && --timeout);

	if (!(val & STD_STAT_ANEG_DONE)) {
		printf("GPY215: auto-negotiation not complete\n");
		phydev->link = 0;
		return 0;
	}

	do {
		mdelay(GPY215_POLL_INTERVAL_MS);
		phy_read_mmd(phydev, GPY215_MMD_VSPEC1,
			     GPY215_VSPEC1_SGMII_STAT);
		sgmii_stat = phy_read_mmd(phydev, GPY215_MMD_VSPEC1,
					  GPY215_VSPEC1_SGMII_STAT);
		if (sgmii_stat < 0)
			return sgmii_stat;
	} while (((sgmii_stat & (SGMII_STAT_LS | SGMII_STAT_ANOK)) !=
		  (SGMII_STAT_LS | SGMII_STAT_ANOK)) && --timeout);

	if ((sgmii_stat & (SGMII_STAT_LS | SGMII_STAT_ANOK)) !=
	    (SGMII_STAT_LS | SGMII_STAT_ANOK)) {
		printf("GPY215: SGMII not ready (stat=0x%04X link=%d ANOK=%d RF=%d)\n",
		       sgmii_stat,
		       !!(sgmii_stat & SGMII_STAT_LS),
		       !!(sgmii_stat & SGMII_STAT_ANOK),
		       !!(sgmii_stat & SGMII_STAT_RF));
		phydev->link = 0;
		return 0;
	}

	/* Clear interrupt status */
	phy_read(phydev, MDIO_DEVAD_NONE, GPY215_PHY_ISTAT);

	miistat = phy_read(phydev, MDIO_DEVAD_NONE, GPY215_PHY_MIISTAT);
	if (miistat < 0)
		return miistat;

	speed_code = miistat & MIISTAT_SPEED_MASK;
	switch (speed_code) {
	case MIISTAT_SPEED_10:
		phydev->speed = SPEED_10;
		break;
	case MIISTAT_SPEED_100:
		phydev->speed = SPEED_100;
		break;
	case MIISTAT_SPEED_1000:
		phydev->speed = SPEED_1000;
		break;
	case MIISTAT_SPEED_2500:
		phydev->speed = SPEED_2500;
		break;
	default:
		printf("GPY215: speed not resolved (miistat=0x%04X)\n",
		       miistat);
		phydev->link = 0;
		return 0;
	}

	phydev->duplex = (miistat & MIISTAT_DPX) ? DUPLEX_FULL : DUPLEX_HALF;
	phydev->link   = 1;

	debug("GPY215: TPI link up  %d Mbit/s %s-duplex\n",
	      phydev->speed,
	      phydev->duplex == DUPLEX_FULL ? "full" : "half");

	debug("GPY215: SGMII status 0x%04X (link=%d ANOK=%d DR=%ld)\n",
	      sgmii_stat,
	      !!(sgmii_stat & SGMII_STAT_LS),
	      !!(sgmii_stat & SGMII_STAT_ANOK),
	      sgmii_stat & SGMII_STAT_DR_MASK);

	return 0;
}

/**
 * gpy215_shutdown - Power down SGMII and TPI
 */
static int gpy215_shutdown(struct phy_device *phydev)
{
	int val;

	val = phy_read_mmd(phydev, GPY215_MMD_VSPEC1,
			   GPY215_VSPEC1_SGMII_CTRL);
	if (val >= 0)
		phy_write_mmd(phydev, GPY215_MMD_VSPEC1,
			      GPY215_VSPEC1_SGMII_CTRL,
			      val | SGMII_CTRL_PD);

	val = phy_read(phydev, MDIO_DEVAD_NONE, GPY215_STD_CTRL);
	if (val >= 0)
		phy_write(phydev, MDIO_DEVAD_NONE, GPY215_STD_CTRL,
			  val | STD_CTRL_PD);

	return 0;
}

U_BOOT_PHY_DRIVER(mxl_gpy215c) = {
	.name		= "MaxLinear GPY215C",
	/*
	 * Match on OUI only. The 22-bit OUI spans PHYID1[15:0] and PHYID2[15:10].
	 * Combined 22-bit PHY ID = (0x67C9 << 6) | (0xDC00 >> 10) = 0x19F277
	 * U-Boot phy_id = (PHYID1 << 16) | PHYID2 with mask 0xFFFFFC00
	 */
	.uid		= (GPY215_PHY_ID1 << 16) | GPY215_PHY_ID2_VAL,
	.mask		= 0xFFFFFC00,
	.features	= PHY_GBIT_FEATURES,
	.probe		= &gpy215_probe,
	.config		= &gpy215_config,
	.startup	= &gpy215_startup,
	.shutdown	= &gpy215_shutdown,
};
