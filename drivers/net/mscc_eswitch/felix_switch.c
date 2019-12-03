// SPDX-License-Identifier: GPL-2.0+ OR BSD-3-Clause
/*
 * Felix Ethernet switch driver
 * Copyright 2018-2020 NXP
 */

/*
 * This driver is used for the Ethernet switch integrated into LS1028A NXP.
 * Felix switch is derived from Microsemi Ocelot but there are several NXP
 * adaptations that makes the two U-Boot drivers largely incompatible.
 *
 * Felix on LS1028A has 4 front panel ports and two internal ports, connected
 * to ENETC interfaces.  We're using one of the ENETC interfaces to push traffic
 * into the switch.  Injection/extraction headers are used to identify
 * egress/ingress ports in the switch for Tx/Rx.
 */

#include <common.h>
#include <dm/device_compat.h>
#include <net/dsa.h>
#include <asm/io.h>
#include <pci.h>
#include <miiphy.h>

/* defines especially around PCS are reused from enetc */
#include "../fsl_enetc.h"

#define PCI_DEVICE_ID_FELIX_ETHSW	0xEEF0

/* Felix has in fact 6 ports, but we don't use the last internal one */
#define FELIX_PORT_COUNT		5
/* Front panel port mask */
#define FELIX_FP_PORT_MASK		0xf

/* Register map for BAR4 */
#define FELIX_SYS			0x010000
#define FELIX_ES0			0x040000
#define FELIX_IS1			0x050000
#define FELIX_IS2			0x060000
#define FELIX_GMII(port)		(0x100000 + (port) * 0x10000)
#define FELIX_QSYS			0x200000

#define FELIX_SYS_SYSTEM		(FELIX_SYS + 0x00000E00)
#define  FELIX_SYS_SYSTEM_EN		BIT(0)
#define FELIX_SYS_RAM_CTRL		(FELIX_SYS + 0x00000F24)
#define  FELIX_SYS_RAM_CTRL_INIT	BIT(1)
#define FELIX_SYS_SYSTEM_PORT_MODE(a)	(FELIX_SYS_SYSTEM + 0xC + (a) * 4)
#define  FELIX_SYS_SYSTEM_PORT_MODE_CPU	0x0000001e

#define FELIX_ES0_TCAM_CTRL		(FELIX_ES0 + 0x000003C0)
#define  FELIX_ES0_TCAM_CTRL_EN		BIT(0)
#define FELIX_IS1_TCAM_CTRL		(FELIX_IS1 + 0x000003C0)
#define  FELIX_IS1_TCAM_CTRL_EN		BIT(0)
#define FELIX_IS2_TCAM_CTRL		(FELIX_IS2 + 0x000003C0)
#define  FELIX_IS2_TCAM_CTRL_EN		BIT(0)

#define FELIX_GMII_CLOCK_CFG(port)	(FELIX_GMII(port) + 0x00000000)
#define  FELIX_GMII_CLOCK_CFG_LINK_1G	1
#define  FELIX_GMII_CLOCK_CFG_LINK_100M	2
#define  FELIX_GMII_CLOCK_CFG_LINK_10M	3
#define FELIX_GMII_MAC_ENA_CFG(port)	(FELIX_GMII(port) + 0x0000001C)
#define  FELIX_GMII_MAX_ENA_CFG_TX	BIT(0)
#define  FELIX_GMII_MAX_ENA_CFG_RX	BIT(4)
#define FELIX_GMII_MAC_IFG_CFG(port)	(FELIX_GMII(port) + 0x0000001C + 0x14)
#define  FELIX_GMII_MAC_IFG_CFG_DEF	0x515

#define FELIX_QSYS_SYSTEM		(FELIX_QSYS + 0x0000F460)
#define FELIX_QSYS_SYSTEM_SW_PORT_MODE(a)	\
					(FELIX_QSYS_SYSTEM + 0x20 + (a) * 4)
#define  FELIX_QSYS_SYSTEM_SW_PORT_ENA		BIT(14)
#define  FELIX_QSYS_SYSTEM_SW_PORT_LOSSY	BIT(9)
#define  FELIX_QSYS_SYSTEM_SW_PORT_SCH(a)	(((a) & 0x3800) << 11)
#define FELIX_QSYS_SYSTEM_EXT_CPU_CFG	(FELIX_QSYS_SYSTEM + 0x80)
#define  FELIX_QSYS_SYSTEM_EXT_CPU_PORT(a)	(((a) & 0xf) << 8 | 0xff)

/* internal MDIO in BAR0 */
#define FELIX_PM_IMDIO_BASE		0x8030

/* Serdes block on LS1028A */
#define FELIX_SERDES_BASE		0x1ea0000L
#define FELIX_SERDES_LNATECR0(lane)	(FELIX_SERDES_BASE + 0x818 + \
					 (lane) * 0x40)
#define  FELIX_SERDES_LNATECR0_ADPT_EQ	0x00003000
#define FELIX_SERDES_SGMIICR1(lane)	(FELIX_SERDES_BASE + 0x1804 + \
					 (lane) * 0x10)
#define  FELIX_SERDES_SGMIICR1_SGPCS	BIT(11)
#define  FELIX_SERDES_SGMIICR1_MDEV(a)	(((a) & 0x1f) << 27)

#define FELIX_PCS_CTRL			0
#define  FELIX_PCS_CTRL_RST		BIT(15)

/*
 * The long prefix format used here contains two dummy MAC addresses, a magic
 * value in place of a VLAN tag followed by the extraction/injection header and
 * the original L2 frame.  Out of all this we only use the port ID.
 */

#define FELIX_DSA_TAG_LEN		sizeof(struct felix_dsa_tag)
#define FELIX_DSA_TAG_MAGIC		0x0a008088
#define FELIX_DSA_TAG_INJ_PORT		7
#define  FELIX_DSA_TAG_INJ_PORT_SET(a)	(0x1 << ((a) & FELIX_FP_PORT_MASK))
#define FELIX_DSA_TAG_EXT_PORT		10
#define  FELIX_DSA_TAG_EXT_PORT_GET(a)	((a) >> 3)

struct felix_dsa_tag {
	uchar d_mac[6];
	uchar s_mac[6];
	u32   magic;
	uchar meta[16];
};

struct felix_priv {
	void *regs_base;
	void *imdio_base;
	struct mii_dev imdio;
	struct udevice *port[FELIX_PORT_COUNT];
};

/* MDIO wrappers, we're using these to drive internal MDIO to get to serdes */
static int felix_mdio_read(struct mii_dev *bus, int addr, int devad, int reg)
{
	struct enetc_mdio_priv priv;

	priv.regs_base = bus->priv;
	return enetc_mdio_read_priv(&priv, addr, devad, reg);
}

static int felix_mdio_write(struct mii_dev *bus, int addr, int devad, int reg,
			    u16 val)
{
	struct enetc_mdio_priv priv;

	priv.regs_base = bus->priv;
	return enetc_mdio_write_priv(&priv, addr, devad, reg, val);
}

/* set up serdes for SGMII */
static int felix_init_sgmii(struct udevice *dev, int port, bool an)
{
	struct felix_priv *priv = dev_get_priv(dev);
	u16 reg;

	/* set up PCS lane address */
	out_le32(FELIX_SERDES_SGMIICR1(port), FELIX_SERDES_SGMIICR1_SGPCS |
		 FELIX_SERDES_SGMIICR1_MDEV(port));

	if (!priv->imdio.priv)
		return 0;
	/*
	 * Set to SGMII mode, for 1Gbps enable AN, for 2.5Gbps set fixed speed.
	 * Although fixed speed is 1Gbps, we could be running at 2.5Gbps based
	 * on PLL configuration.  Setting 1G for 2.5G here is counter intuitive
	 * but intentional.
	 */
	reg = ENETC_PCS_IF_MODE_SGMII;
	reg |= an ? ENETC_PCS_IF_MODE_SGMII_AN : ENETC_PCS_IF_MODE_SPEED_1G;
	felix_mdio_write(&priv->imdio, port, MDIO_DEVAD_NONE,
			 ENETC_PCS_IF_MODE, reg);

	/* Dev ability - SGMII */
	felix_mdio_write(&priv->imdio, port, MDIO_DEVAD_NONE,
			 ENETC_PCS_DEV_ABILITY, ENETC_PCS_DEV_ABILITY_SGMII);

	/* Adjust link timer for SGMII */
	felix_mdio_write(&priv->imdio, port, MDIO_DEVAD_NONE,
			 ENETC_PCS_LINK_TIMER1, ENETC_PCS_LINK_TIMER1_VAL);
	felix_mdio_write(&priv->imdio, port, MDIO_DEVAD_NONE,
			 ENETC_PCS_LINK_TIMER2, ENETC_PCS_LINK_TIMER2_VAL);

	reg = ENETC_PCS_CR_DEF_VAL;
	reg |= an ? ENETC_PCS_CR_RESET_AN : ENETC_PCS_CR_RST;
	/* restart PCS AN */
	felix_mdio_write(&priv->imdio, port, MDIO_DEVAD_NONE,
			 ENETC_PCS_CR, reg);

	return 0;
}

/* set up MAC and serdes for (Q)SXGMII */
static int felix_init_sxgmii(struct udevice *dev, int port)
{
	struct felix_priv *priv = dev_get_priv(dev);
	int to = 1000;

	/* set up transit equalization control on serdes lane */
	out_le32(FELIX_SERDES_LNATECR0(1), FELIX_SERDES_LNATECR0_ADPT_EQ);

	if (!priv->imdio.priv)
		return 0;

	/*reset lane */
	felix_mdio_write(&priv->imdio, port, MDIO_MMD_PCS, FELIX_PCS_CTRL,
			 FELIX_PCS_CTRL_RST);
	while (felix_mdio_read(&priv->imdio, port, MDIO_MMD_PCS,
			       FELIX_PCS_CTRL) & FELIX_PCS_CTRL_RST &&
			--to) {
		mdelay(10);
	}
	if (felix_mdio_read(&priv->imdio, port, MDIO_MMD_PCS,
			    FELIX_PCS_CTRL) & FELIX_PCS_CTRL_RST)
		dev_dbg(port, "PCS reset time-out\n");

	/* Dev ability - SXGMII */
	felix_mdio_write(&priv->imdio, port, ENETC_PCS_DEVAD_REPL,
			 ENETC_PCS_DEV_ABILITY, ENETC_PCS_DEV_ABILITY_SXGMII);

	/* Restart PCS AN */
	felix_mdio_write(&priv->imdio, port, ENETC_PCS_DEVAD_REPL,
			 ENETC_PCS_CR,
			 ENETC_PCS_CR_RST | ENETC_PCS_CR_RESET_AN);
	felix_mdio_write(&priv->imdio, port, ENETC_PCS_DEVAD_REPL,
			 ENETC_PCS_REPL_LINK_TIMER_1,
			 ENETC_PCS_REPL_LINK_TIMER_1_DEF);
	felix_mdio_write(&priv->imdio, port, ENETC_PCS_DEVAD_REPL,
			 ENETC_PCS_REPL_LINK_TIMER_2,
			 ENETC_PCS_REPL_LINK_TIMER_2_DEF);

	return 0;
}

/* Apply protocol specific configuration to MAC, serdes as needed */
static void felix_start_pcs(struct udevice *dev, int port,
			    struct phy_device *phy)
{
	struct dsa_perdev_platdata *platdata = dev->platdata;
	struct felix_priv *priv = dev_get_priv(dev);
	bool autoneg = true;
	const char *if_str;
	int if_type;

	if_type = PHY_INTERFACE_MODE_NONE;

	priv->imdio.read = felix_mdio_read;
	priv->imdio.write = felix_mdio_write;
	priv->imdio.priv = priv->imdio_base + FELIX_PM_IMDIO_BASE;
	strncpy(priv->imdio.name, dev->name, MDIO_NAME_LEN);

	if_str = ofnode_read_string(platdata->port[port].node, "phy-mode");
	if (if_str)
		if_type = phy_get_interface_by_name(if_str);
	else
		dev_dbg(port,
			"phy-mode property not found, defaulting to NONE\n");
	if (if_type < 0)
		if_type = PHY_INTERFACE_MODE_NONE;

	if (!phy || phy->phy_id == PHY_FIXED_ID ||
	    if_type == PHY_INTERFACE_MODE_SGMII_2500)
		autoneg = false;

	switch (if_type) {
	case PHY_INTERFACE_MODE_SGMII:
	case PHY_INTERFACE_MODE_SGMII_2500:
	case PHY_INTERFACE_MODE_QSGMII:
		felix_init_sgmii(dev, port, autoneg);
		break;
	case PHY_INTERFACE_MODE_XGMII:
	case PHY_INTERFACE_MODE_XFI:
	case PHY_INTERFACE_MODE_USXGMII:
		felix_init_sxgmii(dev, port);
		break;
	}
}

void felix_init(struct udevice *dev)
{
	struct dsa_perdev_platdata *platdata = dev->platdata;
	struct felix_priv *priv = dev_get_priv(dev);
	int supported, to = 100, port;
	void *base = priv->regs_base;
	struct phy_device *phy;

	dev_dbg(dev, "trying to set up L2 switch\n");

	/* Init core memories */
	out_le32(base + FELIX_SYS_RAM_CTRL, FELIX_SYS_RAM_CTRL_INIT);
	while (in_le32(base + FELIX_SYS_RAM_CTRL) & FELIX_SYS_RAM_CTRL_INIT &&
	       --to)
		udelay(10);
	if (in_le32(base + FELIX_SYS_RAM_CTRL) & FELIX_SYS_RAM_CTRL_INIT)
		dev_dbg(dev, "Time-out waiting for switch memories\n");

	/* Start switch core, set up ES0, IS1, IS2 */
	out_le32(base + FELIX_SYS_SYSTEM, FELIX_SYS_SYSTEM_EN);
	out_le32(base + FELIX_ES0_TCAM_CTRL, FELIX_ES0_TCAM_CTRL_EN);
	out_le32(base + FELIX_IS1_TCAM_CTRL, FELIX_IS1_TCAM_CTRL_EN);
	out_le32(base + FELIX_IS2_TCAM_CTRL, FELIX_IS2_TCAM_CTRL_EN);
	udelay(20);

	supported = PHY_GBIT_FEATURES | SUPPORTED_2500baseX_Full;

	for (port = 0; port < FELIX_PORT_COUNT; port++) {
		/* Set up MAC registers */
		out_le32(base + FELIX_GMII_CLOCK_CFG(port),
			 FELIX_GMII_CLOCK_CFG_LINK_1G);

		out_le32(base + FELIX_GMII_MAC_IFG_CFG(port),
			 FELIX_GMII_MAC_IFG_CFG_DEF);

		phy = platdata->port[port].phy;
		felix_start_pcs(dev, port, phy);

		if (phy) {
			phy->supported &= supported;
			phy->advertising &= supported;
			phy_config(phy);
		}
	}

	/* set up CPU port */
	out_le32(base + FELIX_QSYS_SYSTEM_EXT_CPU_CFG,
		 FELIX_QSYS_SYSTEM_EXT_CPU_PORT(platdata->cpu_port));
	out_le32(base + FELIX_SYS_SYSTEM_PORT_MODE(platdata->cpu_port),
		 FELIX_SYS_SYSTEM_PORT_MODE_CPU);
}

static int felix_bind(struct udevice *dev)
{
	struct dsa_perdev_platdata *pdata = dev->platdata;

	pdata->num_ports = FELIX_PORT_COUNT;
	pdata->headroom = FELIX_DSA_TAG_LEN;

	return 0;
}

/*
 * Probe Felix:
 * - enable the PCI function
 * - map BAR 4
 * - init switch core and port registers
 */
static int felix_probe(struct udevice *dev)
{
	struct felix_priv *priv = dev_get_priv(dev);

	if (ofnode_valid(dev->node) && !ofnode_is_available(dev->node)) {
		dev_dbg(dev, "switch disabled\n");
		return -ENODEV;
	}

	priv->imdio_base = dm_pci_map_bar(dev, PCI_BASE_ADDRESS_0, 0);
	if (!priv->imdio_base) {
		dev_dbg(dev, "failed to map BAR0\n");
		return -EINVAL;
	}

	priv->regs_base = dm_pci_map_bar(dev, PCI_BASE_ADDRESS_4, 0);
	if (!priv->regs_base) {
		dev_dbg(dev, "failed to map BAR4\n");
		return -EINVAL;
	}

	/* register internal MDIO for debug */
	if (!miiphy_get_dev_by_name(dev->name)) {
		struct mii_dev *mii_bus;

		mii_bus = mdio_alloc();
		mii_bus->read = felix_mdio_read;
		mii_bus->write = felix_mdio_write;
		mii_bus->priv = priv->imdio_base + FELIX_PM_IMDIO_BASE;
		strncpy(mii_bus->name, dev->name, MDIO_NAME_LEN);
		mdio_register(mii_bus);
	}

	dm_pci_clrset_config16(dev, PCI_COMMAND, 0, PCI_COMMAND_MEMORY);

	/* set up registers */
	felix_init(dev);

	return 0;
}

static int felix_port_enable(struct udevice *dev, int port,
			     struct phy_device *phy)
{
	struct felix_priv *priv = dev_get_priv(dev);
	void *base = priv->regs_base;

	out_le32(base + FELIX_GMII_MAC_ENA_CFG(port),
		 FELIX_GMII_MAX_ENA_CFG_TX | FELIX_GMII_MAX_ENA_CFG_RX);

	out_le32(base + FELIX_QSYS_SYSTEM_SW_PORT_MODE(port),
		 FELIX_QSYS_SYSTEM_SW_PORT_ENA |
		 FELIX_QSYS_SYSTEM_SW_PORT_LOSSY |
		 FELIX_QSYS_SYSTEM_SW_PORT_SCH(1));

	if (phy)
		phy_startup(phy);
	return 0;
}

static void felix_port_disable(struct udevice *dev, int port,
			       struct phy_device *phy)
{
	struct felix_priv *priv = dev_get_priv(dev);
	void *base = priv->regs_base;

	out_le32(base + FELIX_GMII_MAC_ENA_CFG(port), 0);

	out_le32(base + FELIX_QSYS_SYSTEM_SW_PORT_MODE(port),
		 FELIX_QSYS_SYSTEM_SW_PORT_LOSSY |
		 FELIX_QSYS_SYSTEM_SW_PORT_SCH(1));

	/*
	 * we don't call phy_shutdown here to avoind waiting next time we use
	 * the port, but the downside is that remote side will think we're
	 * actively processing traffic although we are not.
	 */
}

static int felix_xmit(struct udevice *dev, int port, void *packet, int length)
{
	struct felix_dsa_tag *tag = packet;

	tag->magic = FELIX_DSA_TAG_MAGIC;
	tag->meta[FELIX_DSA_TAG_INJ_PORT] = FELIX_DSA_TAG_INJ_PORT_SET(port);

	return 0;
}

static int felix_rcv(struct udevice *dev, int *port, void *packet, int length)
{
	struct felix_dsa_tag *tag = packet;

	if (tag->magic != FELIX_DSA_TAG_MAGIC)
		return -EINVAL;

	*port = FELIX_DSA_TAG_EXT_PORT_GET(tag->meta[FELIX_DSA_TAG_EXT_PORT]);

	return 0;
}

static const struct dsa_ops felix_dsa_ops = {
	.port_enable  = felix_port_enable,
	.port_disable = felix_port_disable,
	.xmit         = felix_xmit,
	.rcv          = felix_rcv,
};

U_BOOT_DRIVER(felix_ethsw) = {
	.name	= "felix-switch",
	.id	= UCLASS_DSA,
	.bind	= felix_bind,
	.probe	= felix_probe,
	.ops    = &felix_dsa_ops,
	.priv_auto_alloc_size = sizeof(struct felix_priv),
	.platdata_auto_alloc_size = sizeof(struct dsa_perdev_platdata),
};

static struct pci_device_id felix_ethsw_ids[] = {
	{ PCI_DEVICE(PCI_VENDOR_ID_FREESCALE, PCI_DEVICE_ID_FELIX_ETHSW) },
	{}
};

U_BOOT_PCI_DEVICE(felix_ethsw, felix_ethsw_ids);
