/*
 * Copyright (C) 2015-2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <asm/io.h>
#include <asm/arch/mx6_bee.h>
#include <asm-generic/errno.h>
#include <asm/system.h>
#include <common.h>
#include <command.h>
#include <fuse.h>
#include <asm/arch/sys_proto.h>

DECLARE_GLOBAL_DATA_PTR;

#if (defined(CONFIG_SYS_DCACHE_OFF) || defined(CONFIG_SYS_ICACHE_OFF))
#error "Bee needs Cache Open"
#endif

struct bee_parameters {
	int key_method;
	int mode;
	u32 start1;
	u32 size1;
	u32 start2;
	u32 size2;
};

#define SOFT_KEY 0
#define SNVS_KEY 1

#define ECB_MODE 0
#define CTR_MODE 1

#define AES_REGION0_ADDR	0x10000000
#define AES_REGION1_ADDR	0x30000000

static struct bee_parameters para;
static int bee_inited;

union key_soft {
	u8 s_key[16];
	u32 b_key[4];
};

union key_soft key_bad;

/* software version */
u8 hw_get_random_byte(void)
{
	static u32 lcg_state;
	static u32 nb_soft = 9876543;
#define MAX_SOFT_RNG 1024
	static const u32 a = 1664525;
	static const u32 c = 1013904223;
	nb_soft = (nb_soft + 1) % MAX_SOFT_RNG;
	lcg_state = (a * lcg_state + c);
	return (u8) (lcg_state >> 24);
}

/*
 * Lock bee GPR0 bits
 * Only reset can release these bits.
 */
static int bee_lock(void)
{
	int val;

	val = readl(BEE_BASE_ADDR + GPR0);
	val |= (GPR0_CTRL_CLK_EN_LOCK | GPR0_CTRL_SFTRST_N_LOCK |
		GPR0_CTRL_AES_MODE_LOCK | GPR0_SEC_LEVEL_LOCK |
		GPR0_AES_KEY_SEL_LOCK | GPR0_BEE_ENABLE_LOCK);
	writel(val, BEE_BASE_ADDR + GPR0);

	return 0;
}

/* Only check bee enable lock is enough */
static int bee_locked(void)
{
	int val;

	val = readl(BEE_BASE_ADDR + GPR0);

	return val & GPR0_BEE_ENABLE_LOCK ? 1 : 0;
}

int bee_init(struct bee_parameters *p)
{
	int i;
	union key_soft *key = &key_bad;
	u32 value;

	if (bee_locked()) {
		printf("BEE already enabled and locked.\n");
		return CMD_RET_FAILURE;
	}

	/* CLKGATE, SFTRST */
	writel(GPR0_CTRL_CLK_EN | GPR0_CTRL_SFTRST_N, BEE_BASE_ADDR + GPR0);
	/* OFFSET_ADDR0 */
	writel(p->start1 >> 16, BEE_BASE_ADDR + GPR1);
	/*
	 * OFFSET_ADDR1
	 * Default protect IRAM region, if what you want to protect
	 * bigger that 512M which is the max size that one AES region
	 * can protect, we need AES region 1 to cover.
	 */
	writel(p->start2 >> 16, BEE_BASE_ADDR + GPR2);

	if (p->key_method == SOFT_KEY) {
		for (i = 0; i < 16; i++)
			key->s_key[i] = hw_get_random_byte();
		/* AES 128 key from software */
		/* aes0_key0_w0 */
		writel(key->b_key[0], BEE_BASE_ADDR + GPR3);
		/* aes0_key0_w1 */
		writel(key->b_key[1], BEE_BASE_ADDR + GPR4);
		/* aes0_key0_w2 */
		writel(key->b_key[2], BEE_BASE_ADDR + GPR5);
		/* aes0_key0_w3 */
		writel(key->b_key[3], BEE_BASE_ADDR + GPR6);
	}

	if (p->mode == ECB_MODE) {
		value = GPR0_CTRL_CLK_EN | GPR0_CTRL_SFTRST_N |
			GPR0_SEC_LEVEL_3 | GPR0_AES_KEY_SEL_SNVS |
			GPR0_BEE_ENABLE | GPR0_CTRL_AES_MODE_ECB;
		if (p->key_method == SOFT_KEY)
			value = GPR0_CTRL_CLK_EN | GPR0_CTRL_SFTRST_N |
				GPR0_SEC_LEVEL_3 | GPR0_AES_KEY_SEL_SOFT |
				GPR0_BEE_ENABLE | GPR0_CTRL_AES_MODE_ECB;
		writel(value, BEE_BASE_ADDR + GPR0);
	} else {
		for (i = 0; i < 16; i++)
			key->s_key[i] = hw_get_random_byte();
		/* aes_key1_w0 */
		writel(key->b_key[0], BEE_BASE_ADDR + GPR8);
		/* aes_key1_w1 */
		writel(key->b_key[1], BEE_BASE_ADDR + GPR9);
		/* aes_key1_w2 */
		writel(key->b_key[2], BEE_BASE_ADDR + GPR10);
		/* aes_key1_w3 */
		writel(key->b_key[3], BEE_BASE_ADDR + GPR11);

		value = GPR0_CTRL_CLK_EN | GPR0_CTRL_SFTRST_N |
			GPR0_SEC_LEVEL_3 | GPR0_AES_KEY_SEL_SNVS |
			GPR0_BEE_ENABLE | GPR0_CTRL_AES_MODE_CTR;
		if (p->key_method == SOFT_KEY)
			value = GPR0_CTRL_CLK_EN | GPR0_CTRL_SFTRST_N |
				GPR0_SEC_LEVEL_3 | GPR0_AES_KEY_SEL_SOFT |
				GPR0_BEE_ENABLE | GPR0_CTRL_AES_MODE_CTR;
		writel(value, BEE_BASE_ADDR + GPR0);
	}

	bee_lock();

	printf("BEE is settings as: %s mode, %s %d key\n",
	       (p->mode == ECB_MODE) ? "ECB" : "CTR",
	       (p->key_method == SOFT_KEY) ? "SOFT" : "SNVS HW",
	       (p->mode == ECB_MODE) ? 128 : 256);

	return CMD_RET_SUCCESS;
}

int bee_test(struct bee_parameters *p, int region)
{
	u32 result = 0, range, address;
	int i, val;
	/*
	 * Test instruction running in AES Region:
	 * int test(void)
	 * {
	 *	return 0x55aa55aa;
	 * }
	 * Assemble:
	 * 0xe59f0000: ldr r0, [pc]
	 * 0xe12fff1e: bx lr
	 * 0x55aa55aa: 0x55aa55aa
	 */
	u32 inst[3] = {0xe59f0000, 0xe12fff1e, 0x55aa55aa};

	/* Cache enabled? */
	if ((get_cr() & (CR_I | CR_C)) != (CR_I | CR_C)) {
		printf("Enable dcache and icache first!\n");
		return CMD_RET_FAILURE;
	}

	printf("Test Region %d\nBegin Data test: Writing... ", region);

	range = (region == 0) ? p->size1 : p->size2;
	address = (region == 0) ? AES_REGION0_ADDR : AES_REGION1_ADDR;
	for (i = 0; i < range; i = i + 4)
		writel(i, address + i);

	printf("Finshed Write!\n");

	flush_dcache_range(address, address + range);

	printf("Reading... ");
	for (i = 0; i < range; i = i + 4) {
		val = readl(address + i);
		if (val != i)
			result++;
	}
	printf("Finshed Read!\n");

	if (result > 0)
		printf("BEE Data Test check Failed!\n");
	else
		printf("BEE Data Test Check Passed!\n");

	for (i = 0; i < ARRAY_SIZE(inst); i++)
		writel(inst[i], address + (i * 4));

	flush_dcache_range(address, address + sizeof(inst));

	val = ((int (*)(void))address)();

	printf("\nBee Instruction test, Program:\n"
	       "int test(void)\n"
	       "{\n"
	       "      return 0x55aa55aa;\n"
	       "}\n"
	       "Assemble:\n"
	       "0xe59f0000: ldr r0, [pc]\n"
	       "0xe12fff1e: bx lr\n"
	       "0x55aa55aa: 0x55aa55aa\n"
	       "Runnint at 0x%x\n", address);
	if (val == 0x55aa55aa)
		printf("Bee Instruction Test Passed!\n");
	else
		printf("Bee Instruction Test Failed!\n");

	return CMD_RET_SUCCESS;
}

static int region_valid(u32 start, u32 size)
{
	if ((start < PHYS_SDRAM) || (start >= (start + size - 1)) ||
	    (start >= (PHYS_SDRAM + PHYS_SDRAM_SIZE - 1))) {
		printf("Invalid start 0x%x, size 0x%x\n", start, size);
		return -EINVAL;
	}

	if (size > SZ_512M) {
		printf("The region size exceeds SZ_512M\n");
		return -EINVAL;
	}

	if ((start & 0xFFFF) && (size & 0xFFFF)) {
		printf("start or size not 64KB aligned!\n");
		return -EINVAL;
	}

	if ((start + size - 1) >= (gd->start_addr_sp - CONFIG_STACKSIZE)) {
		printf("Overlap with uboot execution environment!\n"
		       "Decrease size or start\n");
		return -EINVAL;
	}

	return 0;
}

static int do_bee_init(cmd_tbl_t *cmdtp, int flag, int argc,
		       char * const argv[])
{
	u32 start, size;
	int ret;
	struct bee_parameters *p = &para;

#if defined(CONFIG_SYS_ARM_CACHE_WRITETHROUGH)
	enum dcache_option option = DCACHE_WRITETHROUGH;
#else
	enum dcache_option option = DCACHE_WRITEBACK;
#endif

	if (argc > 5)
		return CMD_RET_USAGE;

#ifdef CONFIG_MX6
	if (check_module_fused(MX6_MODULE_BEE)) {
		printf("BEE is fused, disable it!\n");
		return CMD_RET_FAILURE;
	}
#endif

	/* Cache enabled? */
	if ((get_cr() & (CR_I | CR_C)) != (CR_I | CR_C)) {
		/*
		 * Here we need icache and dcache both enabled, because
		 * we may take the protected region for instruction and
		 * data usage. And icache and dcache both enabled are
		 * better for performance.
		 */
		printf("Please enable dcache and icache first!\n");
		return CMD_RET_FAILURE;
	}

	p->key_method = SOFT_KEY;
	p->mode = ECB_MODE;
	p->start1 = PHYS_SDRAM;
	p->size1 = SZ_512M;
	p->start2 = IRAM_BASE_ADDR;
	p->size2 = IRAM_SIZE;

	if (argc == 2) {
		p->key_method = (int)simple_strtoul(argv[1], NULL, 16);
		p->mode = ECB_MODE;
		p->start1 = PHYS_SDRAM;
		p->size1 = SZ_512M;
	} else if (argc == 3) {
		p->key_method = (int)simple_strtoul(argv[1], NULL, 16);
		p->mode = (int)simple_strtoul(argv[2], NULL, 10);
		p->start1 = PHYS_SDRAM;
		p->size1 = SZ_512M;
	} else if ((argc == 4) || (argc == 5)) {
		p->key_method = (int)simple_strtoul(argv[1], NULL, 16);
		p->mode = (int)simple_strtoul(argv[2], NULL, 10);
		start = (u32)simple_strtoul(argv[3], NULL, 16);
		/* Default size that AES Region0 can protected */
		size = SZ_512M;
		if (argc == 5)
			size = (u32)simple_strtoul(argv[4], NULL, 16);
		p->start1 = start;
		p->size1 = size;
	}

	if ((p->key_method != SOFT_KEY) && (p->key_method != SNVS_KEY))
		return CMD_RET_USAGE;

	if ((p->mode != ECB_MODE) && (p->mode != CTR_MODE))
		return CMD_RET_USAGE;

	/*
	 * No need to check region valid for IRAM, since it is fixed.
	 * Only check DRAM region here.
	 */
	if (region_valid(p->start1, p->size1))
		return CMD_RET_FAILURE;

	ret = bee_init(p);
	if (ret)
		return CMD_RET_FAILURE;

	/*
	 * Set DCACHE OFF to AES REGION0 and AES REGION1 first
	 * to avoid possible unexcepted cache settings.
	 */
	mmu_set_region_dcache_behaviour(AES_REGION0_ADDR, SZ_1G, DCACHE_OFF);

	mmu_set_region_dcache_behaviour(AES_REGION0_ADDR, p->size1, option);

	mmu_set_region_dcache_behaviour(AES_REGION1_ADDR, p->size2, option);

	printf("Access Region 0x%x - 0x%x to protect 0x%x - 0x%x\n"
	       "Do not directly access 0x%x - 0x%x\n"
	       "Access Region 0x%x - 0x%x to protect 0x%x - 0x%x\n"
	       "Do not directly access 0x%x - 0x%x\n",
	       AES_REGION0_ADDR, AES_REGION0_ADDR + p->size1 - 1,
	       p->start1, p->start1 + p->size1 - 1,
	       p->start1, p->start1 + p->size1 - 1,
	       AES_REGION1_ADDR, AES_REGION1_ADDR + p->size2 - 1,
	       p->start2, p->start2 + p->size2 - 1,
	       p->start2, p->start2 + p->size2 - 1);

	bee_inited = 1;

	return CMD_RET_SUCCESS;
}

static int do_bee_test(cmd_tbl_t *cmdtp, int flag, int argc,
		       char * const argv[])
{
	int ret;
	int region;

	if (bee_inited == 0) {
		printf("Bee not initialized, run bee init first!\n");
		return CMD_RET_FAILURE;
	}
	if (argc > 2)
		return CMD_RET_USAGE;

	region = 0;
	if (argc == 2)
		region = (int)simple_strtoul(argv[1], NULL, 16);
	/* Only two regions are supported, 0 and 1 */
	if (region >= 2)
		return CMD_RET_USAGE;

	ret = bee_test(&para, region);
	if (ret)
		return CMD_RET_FAILURE;

	return CMD_RET_SUCCESS;
}

static cmd_tbl_t cmd_bmp_sub[] = {
	U_BOOT_CMD_MKENT(init, 5, 0, do_bee_init, "", ""),
	U_BOOT_CMD_MKENT(test, 2, 0, do_bee_test, "", ""),
};

static int do_bee_ops(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	cmd_tbl_t *c;

	c = find_cmd_tbl(argv[1], &cmd_bmp_sub[0], ARRAY_SIZE(cmd_bmp_sub));

	/* Drop off the 'bee' command argument */
	argc--;
	argv++;

	if (c)
		return  c->cmd(cmdtp, flag, argc, argv);
	else
		return CMD_RET_USAGE;
}

U_BOOT_CMD(
	bee, CONFIG_SYS_MAXARGS, 1, do_bee_ops,
	"BEE function test",
	"init [key] [mode] [start] [size]	- BEE block initial\n"
	"  key: 0 | 1, 0 means software key, 1 means SNVS random key\n"
	"  mode: 0 | 1, 0 means ECB mode, 1 means CTR mode\n"
	"  start: start address that you want to protect\n"
	"  size: The size of the area that you want to protect\n"
	"  start and end(start + size) addr both should be 64KB aligned.\n"
	"\n"
	" After initialization, the mapping:\n"
	" 1. [0x10000000 - (0x10000000 + size - 1)] <--->\n"
	"    [start - (start + size - 1)]\n"
	"    Here [start - (start + size -1)] is fixed mapping to\n"
	"    [0x10000000 - (0x10000000 + size - 1)], whatever start is.\n"
	" 2. [0x30000000 - (0x30000000 + IRAM_SIZE - 1)] <--->\n"
	"    [IRAM_BASE_ADDR - (IRAM_BASE_ADDR + IRAM_SIZE - 1)]\n"
	"\n"
	" Note: Here we only use AES region 0 to protect the DRAM\n"
	"       area that you specified, max size SZ_512M.\n"
	"       AES region 1 is used to protect IRAM area.\n"
	" Example:\n"
	" 1. bee init 1 1 0xa0000000 0x10000\n"
	"    Access 0x10000000 - 0x10010000 to protect 0xa0000000 - 0xa0010000\n"
	" 2. bee init 1 1 0x80000000 0x20000\n"
	"    Access 0x10000000 - 0x10020000 to protect 0x80000000 - 0x80020000\n"
	"\n"
	" Default configuration if only `bee init` without any args:\n"
	" 1. software key\n"
	" 2. ECB mode\n"
	" 3. Address protected:\n"
	"   Remapped Region0: PHYS_SDRAM - PHYS_SDRAM + SZ_512M\n"
	"   Remapped Region1: IRAM_BASE_ADDR - IRAM_BASE_ADDR + IRAM_SIZE\n"
	" 4. Default Mapping for 6UL:\n"
	"   [0x10000000 - 0x2FFFFFFF] <-> [0x80000000 - 0x9FFFFFFF]\n"
	"   [0x30000000 - 0x3001FFFF] <-> [0x00900000 - 0x0091FFFF]\n"
	"\n"
	"bee test [region]			- BEE function test\n"
	"  region: 0 | 1, 0 means region0, 1 means regions1\n"
);

