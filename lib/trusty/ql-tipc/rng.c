// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright 2023 NXP
 */

#include <dm.h>
#include <rng.h>
#include <linux/kernel.h>
#include <trusty/hwcrypto.h>

U_BOOT_DRVINFO(trusty_rng) = {
	.name = "trusty-rng",
};

static int trusty_rng_read(struct udevice *dev, void *data, size_t len)
{
	return hwcrypto_gen_rng(data, len);
}

static int trusty_rng_probe(struct udevice *dev)
{
	/* return directly */
	return 0;
}

static const struct dm_rng_ops trusty_rng_ops = {
	.read = trusty_rng_read,
};

U_BOOT_DRIVER(trusty_rng) = {
	.name = "trusty-rng",
	.id = UCLASS_RNG,
	.ops = &trusty_rng_ops,
	.probe = trusty_rng_probe,
	.flags = DM_FLAG_ALLOC_PRIV_DMA,
};
