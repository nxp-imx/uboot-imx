// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019-2020 NXP
 */

#include <net/dsa.h>
#include <dm/lists.h>
#include <dm/device_compat.h>
#include <dm/device-internal.h>
#include <dm/uclass-internal.h>
#include <miiphy.h>

#define DSA_PORT_CHILD_DRV_NAME "dsa-port"

/* helper that returns the DSA master Ethernet device */
static struct udevice *dsa_port_get_master(struct udevice *pdev)
{
	struct udevice *dev = dev_get_parent(pdev);
	struct dsa_perdev_platdata *platdata = dev_get_platdata(dev);

	return platdata->master_dev;
}

/*
 * Start the desired port, the CPU port and the master Eth interface.
 * TODO: if cascaded we may need to _start ports in other switches too
 */
static int dsa_port_start(struct udevice *pdev)
{
	struct udevice *dev = dev_get_parent(pdev);
	struct dsa_perdev_platdata *platdata = dev_get_platdata(dev);
	struct udevice *master = dsa_port_get_master(pdev);
	struct dsa_port_platdata *ppriv = dev_get_priv(pdev);
	struct dsa_ops *ops = dsa_get_ops(dev);
	int err;

	if (!ppriv || !platdata)
		return -EINVAL;

	if (!master) {
		dev_err(pdev, "DSA master Ethernet device not found!\n");
		return -EINVAL;
	}

	if (ops->port_enable) {
		err = ops->port_enable(dev, ppriv->index, ppriv->phy);
		if (err)
			return err;
		err = ops->port_enable(dev, platdata->cpu_port,
				       platdata->port[platdata->cpu_port].phy);
		if (err)
			return err;
	}

	return eth_get_ops(master)->start(master);
}

/* Stop the desired port, the CPU port and the master Eth interface */
static void dsa_port_stop(struct udevice *pdev)
{
	struct udevice *dev = dev_get_parent(pdev);
	struct dsa_perdev_platdata *platdata = dev_get_platdata(dev);
	struct udevice *master = dsa_port_get_master(pdev);
	struct dsa_port_platdata *ppriv = dev_get_priv(pdev);
	struct dsa_ops *ops = dsa_get_ops(dev);

	if (!ppriv || !platdata)
		return;

	if (ops->port_disable) {
		ops->port_disable(dev, ppriv->index, ppriv->phy);
		ops->port_disable(dev, platdata->cpu_port,
				  platdata->port[platdata->cpu_port].phy);
	}

	/*
	 * stop master only if it's active, don't probe it otherwise.
	 * Under normal usage it would be active because we're using it, but
	 * during tear-down it may have been removed ahead of us.
	 */
	if (master && device_active(master))
		eth_get_ops(master)->stop(master);
}

/*
 * Insert a DSA tag and call master Ethernet send on the resulting packet
 * We copy the frame to a stack buffer where we have reserved headroom and
 * tailroom space.  Headroom and tailroom are set to 0.
 */
static int dsa_port_send(struct udevice *pdev, void *packet, int length)
{
	struct udevice *dev = dev_get_parent(pdev);
	struct dsa_perdev_platdata *platdata = dev_get_platdata(dev);
	int head = platdata->headroom, tail = platdata->tailroom;
	struct udevice *master = dsa_port_get_master(pdev);
	struct dsa_port_platdata *ppriv = dev_get_priv(pdev);
	struct dsa_ops *ops = dsa_get_ops(dev);
	uchar dsa_packet_tmp[PKTSIZE_ALIGN];
	int err;

	if (!master)
		return -EINVAL;

	if (length + head + tail > PKTSIZE_ALIGN)
		return -EINVAL;

	memset(dsa_packet_tmp, 0, head);
	memset(dsa_packet_tmp + head + length, 0, tail);
	memcpy(dsa_packet_tmp + head, packet, length);
	length += head + tail;
	/* copy back to preserve original buffer alignment */
	memcpy(packet, dsa_packet_tmp, length);

	err = ops->xmit(dev, ppriv->index, packet, length);
	if (err)
		return err;

	return eth_get_ops(master)->send(master, packet, length);
}

/* Receive a frame from master Ethernet, process it and pass it on */
static int dsa_port_recv(struct udevice *pdev, int flags, uchar **packetp)
{
	struct udevice *dev = dev_get_parent(pdev);
	struct dsa_perdev_platdata *platdata = dev_get_platdata(dev);
	struct udevice *master = dsa_port_get_master(pdev);
	struct dsa_port_platdata *ppriv = dev_get_priv(pdev);
	struct dsa_ops *ops = dsa_get_ops(dev);
	int head = platdata->headroom, tail = platdata->tailroom;
	int length, port_index, err;

	if (!master)
		return -EINVAL;

	length = eth_get_ops(master)->recv(master, flags, packetp);
	if (length <= 0)
		return length;

	/*
	 * if we receive frames from a different port or frames that DSA driver
	 * doesn't like we discard them here.
	 * In case of discard we return with no frame and expect to be called
	 * again instead of looping here, so upper layer can deal with timeouts
	 * and ctrl-c
	 */
	err = ops->rcv(dev, &port_index, *packetp, length);
	if (err || port_index != ppriv->index || (length <= head + tail)) {
		if (eth_get_ops(master)->free_pkt)
			eth_get_ops(master)->free_pkt(master, *packetp, length);
		return -EAGAIN;
	}

	/*
	 * We move the pointer over headroom here to avoid a copy.  If free_pkt
	 * gets called we move the pointer back before calling master free_pkt.
	 */
	*packetp += head;

	return length - head - tail;
}

static int dsa_port_free_pkt(struct udevice *pdev, uchar *packet, int length)
{
	struct udevice *dev = dev_get_parent(pdev);
	struct dsa_perdev_platdata *platdata = dev_get_platdata(dev);
	struct udevice *master = dsa_port_get_master(pdev);

	if (!master)
		return -EINVAL;

	if (eth_get_ops(master)->free_pkt) {
		/* return the original pointer and length to master Eth */
		packet -= platdata->headroom;
		length += platdata->headroom - platdata->tailroom;

		return eth_get_ops(master)->free_pkt(master, packet, length);
	}

	return 0;
}

static int dsa_port_probe(struct udevice *pdev)
{
	struct udevice *master = dsa_port_get_master(pdev);
	unsigned char env_enetaddr[ARP_HLEN];

	/* If there is no MAC address in the environment, inherit it
	 * from the DSA master.
	 */
	eth_env_get_enetaddr_by_index("eth", pdev->seq, env_enetaddr);
	if (!is_zero_ethaddr(env_enetaddr))
		return 0;

	if (master) {
		struct eth_pdata *meth = dev_get_platdata(master);
		struct eth_pdata *peth = dev_get_platdata(pdev);

		memcpy(peth->enetaddr, meth->enetaddr, ARP_HLEN);
		eth_env_set_enetaddr_by_index("eth", pdev->seq,
					      meth->enetaddr);
	}

	return 0;
}

static const struct eth_ops dsa_port_ops = {
	.start		= dsa_port_start,
	.send		= dsa_port_send,
	.recv		= dsa_port_recv,
	.stop		= dsa_port_stop,
	.free_pkt	= dsa_port_free_pkt,
};

U_BOOT_DRIVER(dsa_port) = {
	.name	= DSA_PORT_CHILD_DRV_NAME,
	.id	= UCLASS_ETH,
	.ops	= &dsa_port_ops,
	.probe  = dsa_port_probe,
	.platdata_auto_alloc_size = sizeof(struct eth_pdata),
};

/*
 * reads the DT properties of the given DSA port.
 * If the return value is != 0 then the port is skipped
 */
static int dsa_port_parse_dt(struct udevice *dev, int port_index,
			     ofnode ports_node, bool *is_cpu)
{
	struct dsa_perdev_platdata *platdata = dev_get_platdata(dev);
	struct dsa_port_platdata *port = &platdata->port[port_index];
	ofnode temp_node;
	u32 ethernet;

	/*
	 * if we don't have a DT we don't do anything here but the port is
	 * registered normally
	 */
	if (!ofnode_valid(ports_node))
		return 0;

	ofnode_for_each_subnode(temp_node, ports_node) {
		const char *port_label;
		u32 reg;

		if (ofnode_read_u32(temp_node, "reg", &reg) ||
		    reg != port_index)
			continue;

		port->node = temp_node;
		/* if the port is explicitly disabled in DT skip it */
		if (!ofnode_is_available(temp_node))
			return -ENODEV;

		dev_dbg(dev, "port %d node %s\n", port->index,
			ofnode_get_name(port->node));

		/* Use 'label' if present in DT */
		port_label = ofnode_read_string(port->node, "label");
		if (port_label)
			strncpy(port->name, port_label, DSA_PORT_NAME_LENGTH);

		*is_cpu = !ofnode_read_u32(port->node, "ethernet",
					   &ethernet);

		if (*is_cpu) {
			platdata->master_node =
				ofnode_get_by_phandle(ethernet);
			platdata->cpu_port = port_index;

			dev_dbg(dev, "master node %s on port %d\n",
				ofnode_get_name(platdata->master_node),
				port_index);
		}
		break;
	}

	return 0;
}

/**
 * This function mostly deals with pulling information out of the device tree
 * into the platdata structure.
 * It goes through the list of switch ports, registers an Eth device for each
 * front panel port and identifies the cpu port connected to master Eth device.
 * TODO: support cascaded switches
 */
static int dm_dsa_post_bind(struct udevice *dev)
{
	struct dsa_perdev_platdata *platdata = dev_get_platdata(dev);
	ofnode ports_node = ofnode_null();
	int first_err = 0, err = 0, i;

	if (!platdata) {
		dev_err(dev, "missing plaform data\n");
		return -EINVAL;
	}

	if (platdata->num_ports <= 0 || platdata->num_ports > DSA_MAX_PORTS) {
		dev_err(dev, "unexpected num_ports value (%d)\n",
			platdata->num_ports);
		return -EINVAL;
	}

	platdata->master_node = ofnode_null();

	if (!ofnode_valid(dev->node)) {
		dev_dbg(dev, "Device doesn't have a valid DT node!\n");
	} else {
		ports_node = ofnode_find_subnode(dev->node, "ports");
		if (!ofnode_valid(ports_node))
			dev_dbg(dev,
				"ports node is missing under DSA device!\n");
	}

	for (i = 0; i < platdata->num_ports; i++) {
		struct dsa_port_platdata *port = &platdata->port[i];
		bool skip_port, is_cpu = false;

		port->index = i;

		/*
		 * If the driver set up port names in _bind use those, otherwise
		 * use default ones.
		 * If present, DT label is used as name and overrides anything
		 * we may have here.
		 */
		if (!strlen(port->name))
			snprintf(port->name, DSA_PORT_NAME_LENGTH, "%s@%d",
				 dev->name, i);

		skip_port = !!dsa_port_parse_dt(dev, i, ports_node, &is_cpu);

		/*
		 * if this is the CPU port don't register it as an ETH device,
		 * we skip it on purpose since I/O to/from it from the CPU
		 * isn't useful
		 * TODO: cpu port may have a PHY and we don't handle that yet.
		 */
		if (is_cpu || skip_port)
			continue;

		err = device_bind_driver_to_node(dev, DSA_PORT_CHILD_DRV_NAME,
						 port->name, port->node,
						 &port->dev);

		/* try to bind all ports but keep 1st error */
		if (err && !first_err)
			first_err = err;
	}

	if (!ofnode_valid(platdata->master_node))
		dev_dbg(dev, "DSA master Eth device is missing!\n");

	return first_err;
}

/**
 * This function deals with additional devices around the switch as these should
 * have been bound to drivers by now.
 * TODO: pick up references to other switch devices here, if we're cascaded.
 */
static int dm_dsa_pre_probe(struct udevice *dev)
{
	struct dsa_perdev_platdata *platdata = dev_get_platdata(dev);
	int i;

	if (!platdata)
		return -EINVAL;

	if (ofnode_valid(platdata->master_node))
		uclass_find_device_by_ofnode(UCLASS_ETH, platdata->master_node,
					     &platdata->master_dev);

	for (i = 0; i < platdata->num_ports; i++) {
		struct dsa_port_platdata *port = &platdata->port[i];

		/* non-cpu ports only */
		if (!port->dev)
			continue;

		port->dev->priv = port;
		port->phy = dm_eth_phy_connect(port->dev);
	}

	return 0;
}

UCLASS_DRIVER(dsa) = {
	.id = UCLASS_DSA,
	.name = "dsa",
	.post_bind  = dm_dsa_post_bind,
	.pre_probe = dm_dsa_pre_probe,
	.per_device_platdata_auto_alloc_size =
			sizeof(struct dsa_perdev_platdata),
};
