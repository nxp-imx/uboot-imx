// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019-2020 NXP
 */

#include <net/dsa.h>

#define DSA_SANDBOX_MAGIC	0x00415344
#define DSA_SANDBOX_TAG_LEN	sizeof(struct dsa_sandbox_tag)
/*
 * This global flag is used to enable DSA just for DSA test so it doesn't affect
 * the existing eth unit test.
 */
int dsa_sandbox_port_mask;

struct dsa_sandbox_priv {
	int enabled;
	int port_enabled;
};

struct dsa_sandbox_tag {
	u32 magic;
	u32 port;
};

static int dsa_sandbox_port_enable(struct udevice *dev, int port,
				   struct phy_device *phy)
{
	struct dsa_sandbox_priv *priv = dev->priv;

	if (!priv->enabled)
		return -EFAULT;

	priv->port_enabled |= BIT(port);

	return 0;
}

static void dsa_sandbox_port_disable(struct udevice *dev, int port,
				     struct phy_device *phy)
{
	struct dsa_sandbox_priv *priv = dev->priv;

	if (!priv->enabled)
		return;

	priv->port_enabled &= ~BIT(port);
}

static int dsa_sandbox_xmit(struct udevice *dev, int port, void *packet,
			    int length)
{
	struct dsa_sandbox_priv *priv = dev->priv;
	struct dsa_sandbox_tag *tag = packet;

	if (!priv->enabled)
		return -EFAULT;

	if (!(priv->port_enabled & BIT(port)))
		return -EFAULT;

	tag->magic = DSA_SANDBOX_MAGIC;
	tag->port = port;

	return 0;
}

static int dsa_sandbox_rcv(struct udevice *dev, int *port, void *packet,
			   int length)
{
	struct dsa_sandbox_priv *priv = dev->priv;
	struct dsa_sandbox_tag *tag = packet;

	if (!priv->enabled)
		return -EFAULT;

	if (tag->magic != DSA_SANDBOX_MAGIC)
		return -EFAULT;

	*port = tag->port;
	if (!(priv->port_enabled & BIT(*port)))
		return -EFAULT;

	return 0;
}

static const struct dsa_ops dsa_sandbox_ops = {
	.port_enable = dsa_sandbox_port_enable,
	.port_disable = dsa_sandbox_port_disable,
	.xmit = dsa_sandbox_xmit,
	.rcv = dsa_sandbox_rcv,
};

static int dsa_sandbox_bind(struct udevice *dev)
{
	struct dsa_perdev_platdata *pdata = dev->platdata;

	/* must be at least 4 to match sandbox test DT */
	pdata->num_ports = 4;
	pdata->headroom = DSA_SANDBOX_TAG_LEN;

	return 0;
}

static int dsa_sandbox_probe(struct udevice *dev)
{
	struct dsa_sandbox_priv *priv = dev_get_priv(dev);

	/*
	 * return error if DSA is not being tested so we don't break existing
	 * eth test.
	 */
	if (!dsa_sandbox_port_mask)
		return -EINVAL;

	priv->enabled = 1;

	return 0;
}

static int dsa_sandbox_remove(struct udevice *dev)
{
	struct dsa_sandbox_priv *priv = dev_get_priv(dev);

	priv->enabled = 0;

	return 0;
}

static const struct udevice_id dsa_sandbox_ids[] = {
	{ .compatible = "sandbox,dsa" },
	{ }
};

U_BOOT_DRIVER(dsa_sandbox) = {
	.name		= "dsa_sandbox",
	.id		= UCLASS_DSA,
	.of_match	= dsa_sandbox_ids,
	.bind		= dsa_sandbox_bind,
	.probe		= dsa_sandbox_probe,
	.remove		= dsa_sandbox_remove,
	.ops		= &dsa_sandbox_ops,
	.priv_auto_alloc_size = sizeof(struct dsa_sandbox_priv),
	.platdata_auto_alloc_size = sizeof(struct dsa_perdev_platdata),
};

struct dsa_sandbox_eth_priv {
	int enabled;
	int started;
	int packet_length;
	uchar packet[PKTSIZE_ALIGN];
};

static int dsa_eth_sandbox_start(struct udevice *dev)
{
	struct dsa_sandbox_eth_priv *priv = dev->priv;

	if (!priv->enabled)
		return -EFAULT;

	priv->started = 1;

	return 0;
}

static void dsa_eth_sandbox_stop(struct udevice *dev)
{
	struct dsa_sandbox_eth_priv *priv = dev->priv;

	if (!priv->enabled)
		return;

	priv->started = 0;
}

static int dsa_eth_sandbox_send(struct udevice *dev, void *packet, int length)
{
	struct dsa_sandbox_eth_priv *priv = dev->priv;
	struct dsa_sandbox_tag *tag = packet;

	if (!priv->enabled || !priv->started)
		return -EFAULT;

	memcpy(priv->packet, packet, length);
	priv->packet_length = length;

	/*
	 * for DSA test frames we only respond if the associated port is enabled
	 * in the dsa test port mask
	 */

	if (tag->magic == DSA_SANDBOX_MAGIC) {
		int port = tag->port;

		if (!(dsa_sandbox_port_mask & BIT(port)))
			/* drop the frame, port is not enabled */
			priv->packet_length = 0;
	}

	return 0;
}

static int dsa_eth_sandbox_recv(struct udevice *dev, int flags, uchar **packetp)
{
	struct dsa_sandbox_eth_priv *priv = dev->priv;
	int length = priv->packet_length;

	if (!priv->enabled || !priv->started)
		return -EFAULT;

	if (!length) {
		/* no frames pending, force a time-out */
		timer_test_add_offset(100);
		return -EAGAIN;
	}

	*packetp = priv->packet;
	priv->packet_length = 0;

	return length;
}

static const struct eth_ops dsa_eth_sandbox_ops = {
	.start	= dsa_eth_sandbox_start,
	.send	= dsa_eth_sandbox_send,
	.recv	= dsa_eth_sandbox_recv,
	.stop	= dsa_eth_sandbox_stop,
};

static int dsa_eth_sandbox_bind(struct udevice *dev)
{
	return 0;
}

static int dsa_eth_sandbox_probe(struct udevice *dev)
{
	struct dsa_sandbox_eth_priv *priv = dev->priv;

	priv->enabled = 1;

	/*
	 * return error if DSA is not being tested do we don't break existing
	 * eth test.
	 */
	return dsa_sandbox_port_mask ? 0 : -EINVAL;
}

static int dsa_eth_sandbox_remove(struct udevice *dev)
{
	struct dsa_sandbox_eth_priv *priv = dev->priv;

	priv->enabled = 0;

	return 0;
}

static const struct udevice_id dsa_eth_sandbox_ids[] = {
	{ .compatible = "sandbox,dsa-eth" },
	{ }
};

U_BOOT_DRIVER(dsa_eth_sandbox) = {
	.name		= "dsa_eth_sandbox",
	.id		= UCLASS_ETH,
	.of_match	= dsa_eth_sandbox_ids,
	.bind		= dsa_eth_sandbox_bind,
	.probe		= dsa_eth_sandbox_probe,
	.remove		= dsa_eth_sandbox_remove,
	.ops		= &dsa_eth_sandbox_ops,
	.platdata_auto_alloc_size = sizeof(struct eth_pdata),
	.priv_auto_alloc_size = sizeof(struct dsa_sandbox_eth_priv),
};
