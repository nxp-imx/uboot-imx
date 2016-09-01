// SPDX-License-Identifier:     GPL-2.0+
/*
 * Copyright 2018 NXP
 */

#include <common.h>
#include <linux/kernel.h>

int dma_mem_clr(int addr, int size);

static int do_init_sram(cmd_tbl_t *cmdtp, int flag, int argc,
			char * const argv[])
{
	unsigned long addr;
	unsigned int size, max_size, ret_size;
	char *ep;

	if (argc < 3)
		return CMD_RET_USAGE;

	addr = simple_strtoul(argv[1], &ep, 16);
	if (ep == argv[1] || *ep != '\0')
		return CMD_RET_USAGE;

	size = simple_strtoul(argv[2], &ep, 16);
	if (ep == argv[2] || *ep != '\0')
		return CMD_RET_USAGE;

	if (!IS_ALIGNED(addr, 32)) {
		printf("ERROR: Address 0x%08lX is not 32 byte aligned ...\n",
		       addr);
		return CMD_RET_USAGE;
	}

	if (!IS_ALIGNED(size, 32)) {
		printf("ERROR: size 0x%08X is not a 32 byte multiple ...\n",
		       size);
		return CMD_RET_USAGE;
	}

	if (!IS_ADDR_IN_IRAM(addr)) {
		printf("ERROR: Address 0x%08lX not in internal SRAM ...\n",
		       addr);
		return CMD_RET_USAGE;
	}

	max_size = IRAM_SIZE - (addr - IRAM_BASE_ADDR);
	if (size > max_size) {
		printf("WARNING: given size exceeds SRAM boundaries.\n");
		size = max_size;
	}

	ret_size = dma_mem_clr(addr, size);
	__asm_invalidate_dcache_range(addr, addr + size);

	if (!ret_size) {
		printf("Init SRAM failed\n");
		return CMD_RET_FAILURE;
	}

	printf("Init SRAM region at address 0x%08lX, size 0x%X bytes ...\n",
	       addr, ret_size);

	return CMD_RET_SUCCESS;
}

U_BOOT_CMD(initsram,	3,	1,	do_init_sram,
	   "DMA init SRAM from address",
	   "startAddress[hex] size[hex]"
	   );

