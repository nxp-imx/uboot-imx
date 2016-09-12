/*
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#include <common.h>
#include <asm/io.h>
#include <dm.h>
#include <mailbox-uclass.h>

#define NUM_MU_CHANNELS     4
#define NUM_MU_FLAGS        4
#define NUM_MU_GIP          4

#define mu_rr(x)            (0x10 + (x * 0x4))
#define mu_tr(x)            (x * 0x4)
#define MU_SR_OFFSET        0x20
#define MU_CR_OFFSET        0x24
#define CHAN_TE_MASK(x)     (0x00100000 << (x))
#define CHAN_RF_MASK(x)     (0x01000000 << (x))
#define MU_CR_INT_MSK       0xFFF00000
#define MU_FLGS_MSK         0x00000007
#define MU_GIP_MSK          0xF0000000



/* This driver only exposes the status bits to keep with the
 * polling methodology of u-boot.
 */

DECLARE_GLOBAL_DATA_PTR;

struct imx_mu_mbox {
	fdt_addr_t base;

	/* use pointers to channel as a way to reserve channels */
	void *channels[NUM_MU_CHANNELS];
	bool flags[NUM_MU_FLAGS];

	/* TODO add support for the reading/setting of flags to
	 * B side of MU
	 */
};


/* check that the channel is open or owned by caller */
static int mu_check_channel(struct mbox_chan *chan)
{
	struct imx_mu_mbox *mailbox = dev_get_priv(chan->dev);

	/* use id as number of channel within mbox only */
	if ((chan->id < 0) || (chan->id >= NUM_MU_CHANNELS)) {
		debug("nxp mu id out of range: %lu\n", chan->id);
		return -EINVAL;
	}
	if (mailbox->channels[chan->id] != NULL) {
		/* if reserved check that caller owns */
		if (mailbox->channels[chan->id] == chan)
			return 1; /* caller owns the channel */

		return -EACCES;
	}
	return 0;/* channel empty */
}

static int mu_chan_request(struct mbox_chan *chan)
{
	struct imx_mu_mbox *mailbox = dev_get_priv(chan->dev);

	debug("%s(chan=%p)\n", __func__, chan);

	int status = mu_check_channel(chan);
	if (status < 0) {
		debug("channel not available :%d\n", status);
		return -EPERM;
	}
	mailbox->channels[chan->id] = chan;

	return 0;
}
/* currently not dynamically allocated
 * only change pointer back to NULL */
static int mu_chan_free(struct mbox_chan *chan)
{
	struct imx_mu_mbox *mailbox = dev_get_priv(chan->dev);
	int status = mu_check_channel(chan);

	debug("%s(chan=%p)\n", __func__, chan);
	if (status <= 0) { /* check that the channel is also not empty */
		debug("mu_chan_free() failed exit code: %d\n", status);
		return status;
	}
	/*if you own channel and  channel is NOT empty */
	mailbox->channels[chan->id] = NULL;

	return 0;
}

static int mu_send(struct mbox_chan *chan, const void *data)
{
	struct imx_mu_mbox *mbox = dev_get_priv(chan->dev);
	int status = mu_check_channel(chan);
	uint32_t val = *((uint32_t *)data);

	debug("%s(chan=%p, data=%p)\n", __func__, chan, data);
	if (status < 1) {
		debug("mu_send() failed. mu_chan_status is :%d\n", status);
		return -EPERM;
	}

	/*check if transmit register is empty */
	if (!(readl(mbox->base+MU_SR_OFFSET) & CHAN_TE_MASK(chan->id)))
		return -EBUSY;

	/* send out on transmit register*/
	writel(val, mbox->base + mu_tr(chan->id));
	return 0;
}

static int mu_recv(struct mbox_chan *chan, void *data)
{
	struct imx_mu_mbox *mbox = dev_get_priv(chan->dev);
	int status = mu_check_channel(chan);
	uint32_t *buffer =  data;

	debug("%s(chan=%p, data=%p)\n", __func__, chan, data);

	if (status < 1)
		return -EPERM; /* return if channel isnt owned */

	if (readl(mbox->base + MU_SR_OFFSET) & CHAN_RF_MASK(chan->id))
		return -ENODATA;

	*buffer = readl(mu_rr(chan->id));

	return 0;
}

static int imx_mu_bind(struct udevice *dev)
{
	debug("%s(dev=%p)\n", __func__, dev);

	return 0;
}

static int imx_mu_probe(struct udevice *dev)
{
	struct imx_mu_mbox *mbox = dev_get_priv(dev);
	uint32_t val;
	debug("%s(dev=%p)\n", __func__, dev);

	/* get address from device tree */
	mbox->base = dev_get_addr(dev);
	if (mbox->base == FDT_ADDR_T_NONE)
		return -ENODEV;

	val = readl(mbox->base + MU_CR_OFFSET);
	val = val & ~MU_CR_INT_MSK;/* disable all interrupts */
	val = val & ~MU_FLGS_MSK; /* clear all flags */

	writel(val, mbox->base + MU_CR_OFFSET);

	val = readl(mbox->base + MU_SR_OFFSET);
	val = val | MU_GIP_MSK;   /* clear any pending GIP */
	writel(val, mbox->base + MU_SR_OFFSET);

	return 0;
}

static const struct udevice_id imx_mu_ids[] = {
	{ .compatible = "nxp,imx-mu" },
	{ }
};

struct mbox_ops imx_mu_mbox_ops = {
	.request = mu_chan_request,
	.free = mu_chan_free,
	.send = mu_send,
	.recv = mu_recv,
};

U_BOOT_DRIVER(imx_mu) = {
	.name = "imx-mu",
	.id = UCLASS_MAILBOX,
	.of_match = imx_mu_ids,
	.bind = imx_mu_bind,
	.probe = imx_mu_probe,
	.priv_auto_alloc_size = sizeof(struct imx_mu_mbox),
	.ops = &imx_mu_mbox_ops,
};
