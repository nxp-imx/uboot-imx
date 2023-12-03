/*
 * Copyright 2017-2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#include <asm/io.h>
#include <common.h>
#include <dm.h>
#include <asm/arch/imx-regs.h>
#include <linux/iopoll.h>
#include <mailbox-uclass.h>

#define MAX_CHANNELS	8

/* This driver only exposes the status bits to keep with the
 * polling methodology of u-boot.
 */
DECLARE_GLOBAL_DATA_PTR;

struct imx_mu_mbox {
	struct mu_type *base;
	/* use pointers to channel as a way to reserve channels */
	struct mbox_chan * channels[MAX_CHANNELS];
};

/* check that the channel is open or owned by caller */
static int imx_mu_mbox_check_channel(struct mbox_chan *chan)
{
	struct imx_mu_mbox *priv = dev_get_plat(chan->dev);

	if (priv->channels[chan->id] != NULL) {
		/* if reserved check that caller owns */
		if (priv->channels[chan->id] == chan)
			return 1; /* caller owns the channel */

		return -EACCES;
	}

	return 0; /* channel empty */
}

static int imx_mu_mbox_chan_request(struct mbox_chan *chan)
{
	struct imx_mu_mbox *priv = dev_get_plat(chan->dev);
	struct mu_type *base = priv->base;

	/* use id as number of channel within mbox only */
	if (chan->id >= (base->par & 0xFF)) {
		debug("nxp mu id out of range: %lu\n", chan->id);
		return -EINVAL;
	}

	if (imx_mu_mbox_check_channel(chan) < 0) /* check if channel already in use */
		return -EPERM;

	priv->channels[chan->id] = chan;
	/* set GIER[GIEn] */
	setbits_le32(&base->gier, BIT(chan->id));

	return 0;
}

static int imx_mu_mbox_chan_free(struct mbox_chan *chan)
{
	struct imx_mu_mbox *priv = dev_get_plat(chan->dev);
	struct mu_type *base = priv->base;

	if (imx_mu_mbox_check_channel(chan) <= 0) /* check that the channel is also not empty */
		return -EINVAL;

	/* if you own channel and  channel is NOT empty */
	priv->channels[chan->id] = NULL;
	/* clear GIER[GIEn] */
	clrbits_le32(&base->gier, BIT(chan->id));

	return 0;
}

static int imx_mu_mbox_send(struct mbox_chan *chan, const void *data)
{
	struct imx_mu_mbox *priv = dev_get_plat(chan->dev);
	struct mu_type *base = priv->base;
	uint32_t gcr, val = *((uint32_t *)data);

	if (imx_mu_mbox_check_channel(chan) < 1) /* return if channel isnt owned */
		return -EPERM;

	/* check if GCR[GIRn] bit is cleared, ie no pending interrupt */
	if (readl_poll_timeout(&base->gcr, gcr, !(gcr & BIT(chan->id)), 100) < 0)
		return -EBUSY;

	/* send out on transmit register */
	writel(val, &base->tr[chan->id]);
	/* sets GCR[GIRn] bit */
	setbits_le32(&base->gcr, BIT(chan->id));

	return 0;
}

static int imx_mu_mbox_recv(struct mbox_chan *chan, void *data)
{
	struct imx_mu_mbox *priv = dev_get_plat(chan->dev);
	struct mu_type *base = priv->base;
	uint32_t *buffer = data, gsr;

	if (imx_mu_mbox_check_channel(chan) < 1) /* return if channel isnt owned */
		return -EPERM;

	/* check if GSR[GIRn] bit is set */
	if (readl_poll_timeout(&base->gsr, gsr, (gsr & BIT(chan->id)), 1000000) < 0)
		return -EBUSY;

	/* read from receive register */
	*buffer = readl(&base->rr[chan->id]);
	/* Clear GSR[GIRn] bit, W1C */
	setbits_le32(&base->gsr, BIT(chan->id));

	return 0;
}

static int imx_mu_mbox_of_to_plat(struct udevice *dev)
{
	struct imx_mu_mbox *priv = dev_get_plat(dev);
	fdt_addr_t addr;

	addr = dev_read_addr(dev);
	if (addr == FDT_ADDR_T_NONE)
		return -ENODEV;

	priv->base = (struct mu_type *)addr;

	return 0;
}

static const struct udevice_id ids[] = {
	{ .compatible = "nxp,imx-mu-mbox-imx93", },
	{ .compatible = "nxp,imx-mu-mbox-imx95", },
	{ }
};

struct mbox_ops imx_mu_mbox_ops = {
	.request  = imx_mu_mbox_chan_request,
	.rfree    = imx_mu_mbox_chan_free,
	.send     = imx_mu_mbox_send,
	.recv     = imx_mu_mbox_recv,
};

U_BOOT_DRIVER(imx_mu_mbox) = {
	.name = "imx-mu-mbox",
	.id = UCLASS_MAILBOX,
	.of_match = ids,
	.of_to_plat = imx_mu_mbox_of_to_plat,
	.plat_auto = sizeof(struct imx_mu_mbox),
	.ops = &imx_mu_mbox_ops,
	.flags = DM_FLAG_PRE_RELOC,
};
