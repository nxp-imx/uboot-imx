// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2019 NXP
 *
 */

#include <common.h>
#include <asm/io.h>
#include <dm.h>
#include <dm/lists.h>
#include <dm/root.h>
#include <dm/device-internal.h>
#include <asm/arch/sci/sci.h>
#include <linux/iopoll.h>
#include <misc.h>
#include <imx_m4_mu.h>

DECLARE_GLOBAL_DATA_PTR;

struct mu_type {
	u32 tr[4];
	u32 rr[4];
	u32 sr;
	u32 cr;
};

struct imx_m4_mu {
	struct mu_type *base;
};

#define MU_CR_GIE_MASK		0xF0000000u
#define MU_CR_RIE_MASK		0xF000000u
#define MU_CR_GIR_MASK		0xF0000u
#define MU_CR_TIE_MASK		0xF00000u
#define MU_CR_F_MASK		0x7u
#define MU_SR_TE0_MASK		BIT(23)
#define MU_SR_RF0_MASK		BIT(27)
#define MU_TR_COUNT		4
#define MU_RR_COUNT		4

static inline void mu_hal_init(struct mu_type *base)
{
	/* Clear GIEn, RIEn, TIEn, GIRn and ABFn. */
	clrbits_le32(&base->cr, MU_CR_GIE_MASK | MU_CR_RIE_MASK |
		     MU_CR_TIE_MASK | MU_CR_GIR_MASK | MU_CR_F_MASK);
}

static int mu_hal_sendmsg(struct mu_type *base, u32 reg_index, u32 msg)
{
	u32 mask = MU_SR_TE0_MASK >> reg_index;
	u32 val;
	int ret;

	assert(reg_index < MU_TR_COUNT);

	debug("sendmsg sr 0x%x\n", readl(&base->sr));

	/* Wait TX register to be empty. */
	ret = readl_poll_timeout(&base->sr, val, val & mask, 10000);
	if (ret < 0) {
		debug("%s timeout\n", __func__);
		return -ETIMEDOUT;
	}

	debug("tr[%d] 0x%x\n",reg_index,  msg);

	writel(msg, &base->tr[reg_index]);

	return 0;
}

static int mu_hal_receivemsg(struct mu_type *base, u32 reg_index, u32 *msg)
{
	u32 mask = MU_SR_RF0_MASK >> reg_index;
	u32 val;
	int ret;

	assert(reg_index < MU_TR_COUNT);

	debug("receivemsg sr 0x%x\n", readl(&base->sr));

	/* Wait RX register to be full. */
	ret = readl_poll_timeout(&base->sr, val, val & mask, 10000);
	if (ret < 0) {
		debug("%s timeout\n", __func__);
		return -ETIMEDOUT;
	}

	*msg = readl(&base->rr[reg_index]);

	debug("rr[%d] 0x%x\n",reg_index,  *msg);

	return 0;
}

static int mu_hal_poll_receive(struct mu_type *base, ulong rx_timeout)
{
	u32 mask = MU_SR_RF0_MASK;
	u32 val;
	int ret;

	debug("receivemsg sr 0x%x\n", readl(&base->sr));

	/* Wait RX register to be full. */
	ret = readl_poll_timeout(&base->sr, val, val & mask, rx_timeout);
	if (ret < 0) {
		debug("%s timeout\n", __func__);
		return -ETIMEDOUT;
	}

	return 0;
}

static int imx_m4_mu_read(struct mu_type *base, void *data)
{
	union imx_m4_msg *msg = (union imx_m4_msg *)data;
	int ret;
	u8 count = 0;

	if (!msg)
		return -EINVAL;

	/* Read 4  words */
	while (count < 4) {
		ret = mu_hal_receivemsg(base, count % MU_RR_COUNT,
					&msg->data[count]);
		if (ret)
			return ret;
		count++;
	}

	return 0;
}

static int imx_m4_mu_write(struct mu_type *base, void *data)
{
	union imx_m4_msg *msg = (union imx_m4_msg *)data;
	int ret;
	u8 count = 0;

	if (!msg)
		return -EINVAL;

	/* Write 4 words */
	while (count < 4) {
		ret = mu_hal_sendmsg(base, count % MU_TR_COUNT,
				     msg->data[count]);
		if (ret)
			return ret;
		count++;
	}

	return 0;
}

/*
 * Note the function prototype use msgid as the 2nd parameter, here
 * we take it as no_resp.
 */
static int imx_m4_mu_call(struct udevice *dev, int resp_timeout, void *tx_msg,
			 int tx_size, void *rx_msg, int rx_size)
{
	struct imx_m4_mu *priv = dev_get_priv(dev);
	int ret;

	if (resp_timeout < 0)
		return -EINVAL;

	if (tx_msg) {
		ret = imx_m4_mu_write(priv->base, tx_msg);
		if (ret)
			return ret;
	}

	if (rx_msg) {
		if (resp_timeout) {
			ret = mu_hal_poll_receive(priv->base, resp_timeout);
			if (ret)
				return ret;
		}

		ret = imx_m4_mu_read(priv->base, rx_msg);
		if (ret)
			return ret;
	}

	return 0;
}

static int imx_m4_mu_probe(struct udevice *dev)
{
	struct imx_m4_mu *priv = dev_get_priv(dev);
	fdt_addr_t addr;

	debug("%s(dev=%p) (priv=%p)\n", __func__, dev, priv);

	addr = devfdt_get_addr(dev);
	if (addr == FDT_ADDR_T_NONE)
		return -EINVAL;

	priv->base = (struct mu_type *)addr;

	debug("mu base 0x%lx\n", (ulong)priv->base);

	/* U-Boot not enable interrupts, so need to enable RX interrupts */
	mu_hal_init(priv->base);

	return 0;
}

static int imx_m4_mu_remove(struct udevice *dev)
{
	return 0;
}

static int imx_m4_mu_bind(struct udevice *dev)
{
	debug("%s(dev=%p)\n", __func__, dev);

	return 0;
}

static struct misc_ops imx_m4_mu_ops = {
	.call = imx_m4_mu_call,
};

static const struct udevice_id imx_m4_mu_ids[] = {
	{ .compatible = "fsl,imx-m4-mu" },
	{ }
};

U_BOOT_DRIVER(imx_m4_mu) = {
	.name		= "imx_m4_mu",
	.id		= UCLASS_MISC,
	.of_match	= imx_m4_mu_ids,
	.probe		= imx_m4_mu_probe,
	.bind		= imx_m4_mu_bind,
	.remove		= imx_m4_mu_remove,
	.ops		= &imx_m4_mu_ops,
	.priv_auto_alloc_size = sizeof(struct imx_m4_mu),
};
