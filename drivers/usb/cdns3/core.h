/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright 2019 NXP
 */

#ifndef __DRIVERS_USB_CDNS3_CORE_H
#define __DRIVERS_USB_CDNS3_CORE_H

struct cdns3;
enum cdns3_roles {
	CDNS3_ROLE_HOST = 0,
	CDNS3_ROLE_GADGET,
	CDNS3_ROLE_END,
};

/**
 * struct cdns3_role_driver - host/gadget role driver
 * @start: start this role
 * @stop: stop this role
 * @suspend: suspend callback for this role
 * @resume: resume callback for this role
 * @irq: irq handler for this role
 * @name: role name string (host/gadget)
 */
struct cdns3_role_driver {
	int (*start)(struct cdns3 *cdns);
	void (*stop)(struct cdns3 *cdns);
	int (*suspend)(struct cdns3 *cdns, bool do_wakeup);
	int (*resume)(struct cdns3 *cdns, bool hibernated);
	int (*irq)(struct cdns3 *cdns);
	const char *name;
};

#define CDNS3_NUM_OF_CLKS	5
/**
 * struct cdns3 - Representation of Cadence USB3 DRD controller.
 * @dev: pointer to Cadence device struct
 * @xhci_regs: pointer to base of xhci registers
 * @xhci_res: the resource for xhci
 * @dev_regs: pointer to base of dev registers
 * @none_core_regs: pointer to base of nxp wrapper registers
 * @phy_regs: pointer to base of phy registers
 * @otg_regs: pointer to base of otg registers
 * @irq: irq number for controller
 * @roles: array of supported roles for this controller
 * @role: current role
 * @host_dev: the child host device pointer for cdns3 core
 * @gadget_dev: the child gadget device pointer for cdns3 core
 * @usbphy: usbphy for this controller
 * @cdns3_clks: Clock pointer array for cdns3 core
 * @extcon: Type-C extern connector
 * @extcon_nb: notifier block for Type-C extern connector
 * @role_switch_wq: work queue item for role switch
 * @in_lpm: the controller in low power mode
 * @wakeup_int: the wakeup interrupt
 */
struct cdns3 {
	struct udevice *dev;
	void __iomem *xhci_regs;
	struct resource *xhci_res;
	struct usbss_dev_register_block_type __iomem *dev_regs;
	void __iomem *none_core_regs;
	void __iomem *phy_regs;
	void __iomem *otg_regs;
	int irq;
	struct cdns3_role_driver *roles[CDNS3_ROLE_END];
	enum cdns3_roles role;
	struct udevice *host_dev;
	struct udevice *gadget_dev;
	struct clk *cdns3_clks[CDNS3_NUM_OF_CLKS];

	int  index;
	struct list_head list;
};

static inline struct cdns3_role_driver *cdns3_role(struct cdns3 *cdns)
{
	WARN_ON(cdns->role >= CDNS3_ROLE_END || !cdns->roles[cdns->role]);
	return cdns->roles[cdns->role];
}

static inline int cdns3_role_start(struct cdns3 *cdns, enum cdns3_roles role)
{
	if (role >= CDNS3_ROLE_END)
		return 0;

	if (!cdns->roles[role])
		return -ENXIO;

	cdns->role = role;
	return cdns->roles[role]->start(cdns);
}

static inline void cdns3_role_stop(struct cdns3 *cdns)
{
	enum cdns3_roles role = cdns->role;

	if (role == CDNS3_ROLE_END)
		return;

	cdns->roles[role]->stop(cdns);
	cdns->role = CDNS3_ROLE_END;
}

static inline void cdns3_role_irq_handler(struct cdns3 *cdns)
{
	enum cdns3_roles role = cdns->role;

	if (role == CDNS3_ROLE_END)
		return;

	cdns->roles[role]->irq(cdns);
}

int cdns3_init(struct cdns3 *cdns);
void cdns3_exit(struct cdns3 *cdns);

#endif /* __DRIVERS_USB_CDNS3_CORE_H */
