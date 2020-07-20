/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2019-2020 NXP
 */

#ifndef __DSA_H__
#define __DSA_H__

#include <common.h>
#include <dm.h>
#include <phy.h>

/**
 * DSA stands for Distributed Switch Architecture and it is infrastructure
 * intended to support drivers for Switches that rely on an intermediary
 * Ethernet device for I/O.  These switches may support cascading allowing
 * them to be arranged as a tree.
 * DSA is documented in detail in the Linux kernel documentation under
 * Documentation/networking/dsa/dsa.txt
 * The network layout of such a switch is shown below:
 *
 *	    |---------------------------
 *	    | CPU network device (eth0)|
 *	    ----------------------------
 *	    | <tag added by switch     |
 *	    |                          |
 *	    |                          |
 *	    |        tag added by CPU> |
 *	|--------------------------------------------|
 *	| Switch driver				     |
 *	|--------------------------------------------|
 *	    ||        ||         ||
 *	|-------|  |-------|  |-------|
 *	| sw0p0 |  | sw0p1 |  | sw0p2 |
 *	|-------|  |-------|  |-------|
 *
 * In U-Boot the intent is to allow access to front panel ports (shown at the
 * bottom of the picture) though the master Ethernet port (eth0 in the picture).
 * Front panel ports are presented as regular Ethernet devices in U-Boot and
 * they are expected to support the typical networking commands.
 * In general DSA switches require the use of tags, extra headers added both by
 * software on Tx and by the switch on Rx.  These tags carry at a minimum port
 * information and switch information for cascaded set-ups.
 * In U-Boot these tags are inserted and parsed by the DSA switch driver, the
 * class code helps with headroom/tailroom for the extra headers.
 *
 * TODO:
 * - handle switch cascading, for now U-Boot only supports stand-alone switches.
 * - Add support to probe DSA switches connected to a MDIO bus, this is needed
 * to convert switch drivers that are now under drivers/net/phy.
 */

#define DSA_PORT_NAME_LENGTH	16

/* Maximum number of ports each DSA device can have */
#define DSA_MAX_PORTS		12

/**
 * struct dsa_ops - DSA operations
 *
 * @port_enable:  Initialize a switch port for I/O
 * @port_disable: Disable a port
 * @xmit:         Insert the DSA tag for transmission
 *                DSA drivers receive a copy of the packet with headroom and
 *                tailroom reserved and set to 0.
 *                Packet points to headroom and length is updated to include
 *                both headroom and tailroom
 * @rcv:          Process the DSA tag on reception
 *                Packet and length describe the frame as received from master
 *                including any additional headers
 */
struct dsa_ops {
	int (*port_enable)(struct udevice *dev, int port,
			   struct phy_device *phy);
	void (*port_disable)(struct udevice *dev, int port,
			     struct phy_device *phy);
	int (*xmit)(struct udevice *dev, int port, void *packet, int length);
	int (*rcv)(struct udevice *dev, int *port, void *packet, int length);
};

#define dsa_get_ops(dev) ((struct dsa_ops *)(dev)->driver->ops)

/**
 * struct dsa_port_platdata - DSA port platform data
 *
 * @dev :  Port u-device
 *         Uclass code sets this field for all ports
 * @phy:   PHY device associated with this port
 *         Uclass code sets this field for all ports except CPU port, based on
 *         DT information.  It may be NULL.
 * @node:  Port DT node, if any.  Uclass code sets this field.
 * @index: Port index in the DSA switch, set by class code.
 * @name:  Name of the port Eth device.  If a label property is present in the
 *         port DT node, it is used as name.  Drivers can use custom names by
 *         populating this field, otherwise class code generates a default.
 */
struct dsa_port_platdata {
	struct udevice *dev;
	struct phy_device *phy;
	ofnode node;
	int index;
	char name[DSA_PORT_NAME_LENGTH];
};

/**
 * struct dsa_perdev_platdata - Per-device platform data for DSA DM
 *
 * @num_ports:   Number of ports the device has, must be <= DSA_MAX_PORTS
 *               All DSA drivers must set this at _bind
 * @headroom:    Size, in bytes, of headroom needed for the DSA tag
 *               All DSA drivers must set this at _bind or _probe
 * @tailroom:    Size, in bytes, of tailroom needed for the DSA tag
 *               DSA class code allocates headroom and tailroom on Tx before
 *               calling DSA driver xmit function
 *               All DSA drivers must set this at _bind or _probe
 * @master_node: DT node of the master Ethernet.  DT is optional so this may be
 *               null.
 * @master_dev:  Ethernet device to be used as master.  Uclass code sets this
 *               based on DT information if present, otherwise drivers must set
 *               this field in _probe.
 * @cpu_port:    Index of switch port linked to master Ethernet.
 *               Uclass code sets this based on DT information if present,
 *               otherwise drivers must set this field in _bind.
 * @port:        per-port data
 */
struct dsa_perdev_platdata {
	int num_ports;
	int headroom;
	int tailroom;

	ofnode master_node;
	struct udevice *master_dev;
	int cpu_port;
	struct dsa_port_platdata port[DSA_MAX_PORTS];
};

#endif /* __DSA_H__ */
