// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2022 NXP
 */
#include <dm.h>
#include <malloc.h>
#include <rng.h>
#include <fdt_support.h>
#include <kaslr.h>

int do_generate_kaslr(void *blob) {

	struct udevice *dev;
	size_t n = 0x8;
	u64 *buf;
	int rc, nodeoff, ret = 0;
	nodeoff = fdt_find_or_add_subnode(blob, 0, "chosen");
	if (nodeoff < 0) {
		printf("Reading chosen node failed.\n");
		return nodeoff;
	}
	rc =  uclass_get_device(UCLASS_RNG, 0, &dev);
	if (rc || !dev) {
		printf("No RNG device\n");
		return rc;
	}
	buf = malloc(n);
	if (!buf) {
		printf("Out of memory\n");
		return -ENOMEM;
	}
	ret = dm_rng_read(dev, buf, n);
	if (ret) {
		printf("Reading RNG failed\n");
		goto err;
	}
	ret = fdt_setprop(blob, nodeoff, "kaslr-seed", buf, sizeof(buf));
	if (ret < 0) {
		printf("Unable to set kaslr-seed on chosen node: %s\n", fdt_strerror(ret));
		goto err;
	}

err:
	free(buf);
	return ret;

	return 0;

}


