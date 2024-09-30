// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright 2024 NXP
 */

#include <dm.h>
#include <rng.h>
#include <linux/kernel.h>
#include <asm/mach-imx/ele_api.h>

U_BOOT_DRVINFO(ele_rng) = {
	.name = "ele-rng",
};

static int ele_rng_read(struct udevice *dev, void *data, size_t len)
{
	int ret = 0;
	uint8_t *rand_buf = NULL;

	if (!data || len == 0)
		return -EINVAL;

	/* use intermediate buffer in case the output buffer
	 * is not cacheline aligned.
	 */
#if CONFIG_ELE_SHARED_BUFFER
	rand_buf = (uint8_t *)CONFIG_ELE_SHARED_BUFFER;
#else
	rand_buf = (uint8_t *)memalign(ARCH_DMA_MINALIGN, len);
#endif

	ret = ele_get_random((u32)(ulong)rand_buf, len);

	if (!ret) {
		memcpy(data, rand_buf, len);
		memset(rand_buf, 0, len);
	}

#if !CONFIG_ELE_SHARED_BUFFER
	if (rand_buf)
		free(rand_buf);
#endif

	return ret;
}

static int ele_rng_probe(struct udevice *dev)
{
	/* return directly */
	return 0;
}

static const struct dm_rng_ops ele_rng_ops = {
	.read = ele_rng_read,
};

U_BOOT_DRIVER(ele_rng) = {
	.name = "ele-rng",
	.id = UCLASS_RNG,
	.ops = &ele_rng_ops,
	.probe = ele_rng_probe,
	.flags = DM_FLAG_ALLOC_PRIV_DMA,
};
