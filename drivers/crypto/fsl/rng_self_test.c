/*
 * Copyright 2018, 2021 NXP
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <common.h>
#include <cpu_func.h>
#include <malloc.h>
#include <asm/byteorder.h>
#include <linux/compiler.h>
#include <asm/arch/clock.h>
#include <asm/global_data.h>
#include <asm/arch/sys_proto.h>
#include <memalign.h>
#include <fsl_sec.h>
#include "jobdesc.h"
#include "desc.h"
#include "jr.h"

DECLARE_GLOBAL_DATA_PTR;

uint32_t rng_dsc[] = {
	0xb0800036, 0x04800010, 0x3c85a15b, 0x50a9d0b1,
	0x71a09fee, 0x2eecf20b, 0x02800020, 0xb267292e,
	0x85bf712d, 0xe85ff43a, 0xa716b7fb, 0xc40bb528,
	0x27b6f564, 0x8821cb5d, 0x9b5f6c26, 0x12a00020,
	0x0a20de17, 0x6529357e, 0x316277ab, 0x2846254e,
	0x34d23ba5, 0x6f5e9c32, 0x7abdc1bb, 0x0197a385,
	0x82500405, 0xa2000001, 0x10880004, 0x00000005,
	0x12820004, 0x00000020, 0x82500001, 0xa2000001,
	0x10880004, 0x40000045, 0x02800020, 0x8f389cc7,
	0xe7f7cbb0, 0x6bf2073d, 0xfc380b6d, 0xb22e9d1a,
	0xee64fcb7, 0xa2b48d49, 0xdf9bc3a4, 0x82500009,
	0xa2000001, 0x10880004, 0x00000005, 0x82500001,
	0x60340020, 0xFFFFFFFF, 0xa2000001, 0x10880004,
	0x00000005, 0x8250000d
};

uint8_t rng_result[] = {
	0x3a, 0xfe, 0x2c, 0x87, 0xcc, 0xb6, 0x44, 0x49,
	0x19, 0x16, 0x9a, 0x74, 0xa1, 0x31, 0x8b, 0xef,
	0xf4, 0x86, 0x0b, 0xb9, 0x5e, 0xee, 0xae, 0x91,
	0x92, 0xf4, 0xa9, 0x8f, 0xb0, 0x37, 0x18, 0xa4
};

#define DSCSIZE 0x36
#define KAT_SIZE 32
#define INTEGRAL(x) ((x & 0x000F0) >> 4)
#define FRACTIONAL(x) (x & 0x0000F)
/*
 * construct_rng_self_test_jobdesc() - Implement destination address in RNG self test descriptors
 * @desc:	RNG descriptors address
 *
 * Returns zero on success,and negative on error.
 */
int construct_rng_self_test_jobdesc(u32 *desc, int desc_size, u8 *res_addr)
{
	u32 *rng_st_dsc = (uint32_t *)rng_dsc;
	int result_addr_idx = desc_size - 5;
	int i = 0;

	/* Replace destination address in the descriptor */
	rng_st_dsc[result_addr_idx] = (u32)res_addr;

	for (i = 0; i < desc_size; i++)
		desc[i] = rng_st_dsc[i];

	debug("RNG SELF TEST DESCRIPTOR:\n");
	for (i = 0; i < desc_size; i++)
		debug("0x%08X\n", desc[i]);
	debug("\n");

	if (!desc) {
		puts("RNG self test descriptor construction failed\n");
		return -1;
	}

	return 0;
}

/*
 * rng_test() - Perform RNG self test
 *
 */
void rng_test(void)
{
	int ret, size, i;
	int desc_size = DSCSIZE;
	u8 *result;
	u32 *desc;

	result = memalign(ARCH_DMA_MINALIGN, KAT_SIZE);
	if(!result) {
		puts("Not enough memory for RNG result\n");
		return;
	}

	desc = memalign(ARCH_DMA_MINALIGN, sizeof(uint32_t) * desc_size);
	if (!desc) {
		puts("Not enough memory for descriptor allocation\n");
		free(result);
		return;
	}

	ret = construct_rng_self_test_jobdesc(desc, desc_size, result);
	if (ret) {
		puts("Error in Job Descriptor Construction\n");
		goto err;
	} else {
		size = roundup(sizeof(uint32_t) * desc_size, ARCH_DMA_MINALIGN);
		flush_dcache_range((unsigned long)desc, (unsigned long)desc + size);
		size = roundup(sizeof(uint8_t) * KAT_SIZE, ARCH_DMA_MINALIGN);
		flush_dcache_range((unsigned long)result, (unsigned long)result + size);

		ret = run_descriptor_jr(desc);
	}

	if (ret) {
		printf("Error while running RNG self-test descriptor: %d\n", ret);
		goto err;
	}

	size = roundup(KAT_SIZE, ARCH_DMA_MINALIGN);
	invalidate_dcache_range((unsigned long)result, (unsigned long)result + size);

	debug("Result\n");
	for (i = 0; i < KAT_SIZE; i++)
		debug("%02X", result[i]);
	debug("\n");

	debug("Expected Result\n");
	for (i = 0; i < KAT_SIZE; i++)
		debug("%02X", rng_result[i]);
	debug("\n");

	for (i = 0; i < KAT_SIZE; i++) {
		if (result[i] != rng_result[i]) {
			printf("!!!WARNING!!!\nRNG self test failed.");
			printf("If it keeps failing, do not perform any further crypto operations with RNG.\n");
			printf("!!!!!!!!!!!!!\n");
			goto err;
		}
	}
	puts("RNG self test passed\n");

err:
	free(desc);
	free(result);
	return;
}

/*
 * rng_self_test(void) - Check affected board and call RNG self test
 */
void rng_self_test(void)
{
	u32 cpurev;
	cpurev = get_cpu_rev();
#if defined(CONFIG_MX6QP)
	if ((INTEGRAL(cpurev) == 1) && (FRACTIONAL(cpurev) == 1)) {
		rng_test();
	}
#elif defined(CONFIG_MX6Q)
	if ((INTEGRAL(cpurev) == 1) && (FRACTIONAL(cpurev) == 6)) {
		rng_test();
	}
#elif defined(CONFIG_MX6S) || defined(CONFIG_MX6DL) || defined(CONFIG_MX6SX)
	if ((INTEGRAL(cpurev) == 1) && (FRACTIONAL(cpurev) == 4)) {
		rng_test();
	}
#endif
	return;
}
