// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * RNG driver for Freescale RNGC
 *
 * Copyright (C) 2008-2012 Freescale Semiconductor, Inc.
 * Copyright (C) 2017 Martin Kaiser <martin@kaiser.cx>
 *	Copyright 2022 NXP
 *
 * Based on RNGC driver in drivers/char/hw_random/imx-rngc.c in Linux
 */

#include <asm/cache.h>
#include <common.h>
#include <cpu_func.h>
#include <dm.h>
#include <rng.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <dm/root.h>

#define DCP_RNG_MAX_FIFO_STORE_SIZE	4
#define RNGC_VER_ID			0x0000
#define RNGC_COMMAND			0x0004
#define RNGC_CONTROL			0x0008
#define RNGC_STATUS			0x000C
#define RNGC_ERROR			0x0010
#define RNGC_FIFO			0x0014

/* the fields in the ver id register */
#define RNGC_TYPE_SHIFT			28

/* the rng_type field */
#define RNGC_TYPE_RNGB			0x1
#define RNGC_TYPE_RNGC			0x2

#define RNGC_CMD_CLR_ERR		0x00000020
#define RNGC_CMD_SEED			0x00000002

#define RNGC_CTRL_AUTO_SEED		0x00000010

#define RNGC_STATUS_ERROR		0x00010000
#define RNGC_STATUS_FIFO_LEVEL_MASK	0x00000f00
#define RNGC_STATUS_FIFO_LEVEL_SHIFT	8
#define RNGC_STATUS_SEED_DONE		0x00000020
#define RNGC_STATUS_ST_DONE		0x00000010

#define RNGC_ERROR_STATUS_STAT_ERR	0x00000008

#define RNGC_TIMEOUT			3000000U /* 3 sec */

struct imx_rngc {
	unsigned long base;
};

static int rngc_read(struct udevice *dev, void *data, size_t len)
{
	struct imx_rngc *rngc = dev_get_priv(dev);
	u8 buffer[DCP_RNG_MAX_FIFO_STORE_SIZE];
	u32 status, level;
	size_t size;

	while (len) {
		status = readl(rngc->base + RNGC_STATUS);

		/* is there some error while reading this random number? */
		if (status & RNGC_STATUS_ERROR)
			break;
		/* how many random numbers are in FIFO? [0-16] */
		level = (status & RNGC_STATUS_FIFO_LEVEL_MASK) >>
			RNGC_STATUS_FIFO_LEVEL_SHIFT;

		if (level) {
			/* retrieve a random number from FIFO */
			*(u32 *)buffer = readl(rngc->base + RNGC_FIFO);
			size = min(len, sizeof(u32));
			memcpy(data, buffer, size);
			data += size;
			len -= size;
		}
	}

	return len ? -EIO : 0;
}

static int rngc_init(struct imx_rngc *rngc)
{
	u32 cmd, ctrl, status, err_reg = 0;
	unsigned long long timeval = 0;
	unsigned long long timeout = RNGC_TIMEOUT;

	/* clear error */
	cmd = readl(rngc->base + RNGC_COMMAND);
	writel(cmd | RNGC_CMD_CLR_ERR, rngc->base + RNGC_COMMAND);

	/* create seed, repeat while there is some statistical error */
	do {
		/* seed creation */
		cmd = readl(rngc->base + RNGC_COMMAND);
		writel(cmd | RNGC_CMD_SEED, rngc->base + RNGC_COMMAND);

		udelay(1);
		timeval += 1;

		status = readl(rngc->base + RNGC_STATUS);
		err_reg = readl(rngc->base + RNGC_ERROR);

		if (status & (RNGC_STATUS_SEED_DONE | RNGC_STATUS_ST_DONE))
			break;

		if (timeval > timeout) {
			debug("rngc timed out\n");
			return -ETIMEDOUT;
		}
	} while (err_reg == RNGC_ERROR_STATUS_STAT_ERR);

	if (err_reg)
		return -EIO;

	/*
	 * enable automatic seeding, the rngc creates a new seed automatically
	 * after serving 2^20 random 160-bit words
	 */
	ctrl = readl(rngc->base + RNGC_CONTROL);
	ctrl |= RNGC_CTRL_AUTO_SEED;
	writel(ctrl, rngc->base + RNGC_CONTROL);
	return 0;
}

static int rngc_probe(struct udevice *dev)
{
	struct imx_rngc *rngc = dev_get_priv(dev);
	fdt_addr_t addr;
	u32 ver_id;
	u8  rng_type;
	int ret;

	addr = dev_read_addr(dev);
	if (addr == FDT_ADDR_T_NONE) {
		ret = -EINVAL;
		goto err;
	}

	rngc->base = addr;
	ver_id = readl(rngc->base + RNGC_VER_ID);
	rng_type = ver_id >> RNGC_TYPE_SHIFT;
	/*
	 * This driver supports only RNGC and RNGB. (There's a different
	 * driver for RNGA.)
	 */
	if (rng_type != RNGC_TYPE_RNGC && rng_type != RNGC_TYPE_RNGB) {
		ret = -ENODEV;
		goto err;
	}

	ret = rngc_init(rngc);
	if (ret)
		goto err;

	return 0;

err:
	printf("%s error = %d\n", __func__, ret);
	return ret;
}

static const struct dm_rng_ops rngc_ops = {
	.read = rngc_read,
};

static const struct udevice_id rngc_dt_ids[] = {
	{ .compatible = "fsl,imx25-rngb" },
	{ }
};

U_BOOT_DRIVER(dcp_rng) = {
	.name = "dcp_rng",
	.id = UCLASS_RNG,
	.of_match = rngc_dt_ids,
	.ops = &rngc_ops,
	.probe = rngc_probe,
	.priv_auto  = sizeof(struct imx_rngc),
	.flags = DM_FLAG_ALLOC_PRIV_DMA,
};
