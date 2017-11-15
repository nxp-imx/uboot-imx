/*
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <spl.h>
#include <asm/io.h>
#include <errno.h>
#include <asm/io.h>
#include <asm/arch/ddr.h>
#include <asm/sections.h>

#include "ddr.h"

DECLARE_GLOBAL_DATA_PTR;

#define IMEM_LEN 32768
#define DMEM_LEN 16384
#define IMEM_2D_OFFSET 	49152

#define IMEM_OFFSET_ADDR 0x00050000
#define DMEM_OFFSET_ADDR 0x00054000
#define DDR_TRAIN_CODE_BASE_ADDR IP2APB_DDRPHY_IPS_BASE_ADDR(0)

/* We need PHY iMEM PHY is 32KB padded */
void ddr_load_train_code(enum fw_type type)
{
	u32 tmp32, i;
	u32 error = 0;
	unsigned long pr_to32, pr_from32;
	unsigned long fw_offset = type ? IMEM_2D_OFFSET : 0;
	unsigned long imem_start = (unsigned long)&_end + fw_offset;
	unsigned long dmem_start = imem_start + IMEM_LEN;

	pr_from32 = imem_start;
	pr_to32 = DDR_TRAIN_CODE_BASE_ADDR + 4 * IMEM_OFFSET_ADDR;
	for(i = 0x0; i < IMEM_LEN; ){
		tmp32 = readl(pr_from32);
		writew(tmp32 & 0x0000ffff, pr_to32);
		pr_to32 += 4;
		writew((tmp32 >> 16) & 0x0000ffff, pr_to32);
		pr_to32 += 4;
		pr_from32 += 4;
		i += 4;
	}

	pr_from32 = dmem_start;
	pr_to32 = DDR_TRAIN_CODE_BASE_ADDR + 4 * DMEM_OFFSET_ADDR;
	for(i = 0x0; i < DMEM_LEN;){
		tmp32 = readl(pr_from32);
		writew(tmp32 & 0x0000ffff, pr_to32);
		pr_to32 += 4;
		writew((tmp32 >> 16) & 0x0000ffff, pr_to32);
		pr_to32 += 4;
		pr_from32 += 4;
		i += 4;
	}

	printf("check ddr_imem code\n");
	pr_from32 = imem_start;
	pr_to32 = DDR_TRAIN_CODE_BASE_ADDR + 4 * IMEM_OFFSET_ADDR;
	for(i = 0x0; i < IMEM_LEN;){
		tmp32 = (readw(pr_to32) & 0x0000ffff);
		pr_to32 += 4;
		tmp32 += ((readw(pr_to32) & 0x0000ffff) << 16);

		if(tmp32 != readl(pr_from32)){
			printf("%lx %lx\n", pr_from32, pr_to32);
			error++;
		}
		pr_from32 += 4;
		pr_to32 += 4;
		i += 4;
	}
	if(error){
		printf("check ddr_imem code fail=%d\n",error);
	}else{
		printf("check ddr_imem code pass\n");
	}

	printf("check ddr_dmem code\n");
	pr_from32 = dmem_start;
	pr_to32 = DDR_TRAIN_CODE_BASE_ADDR + 4 * DMEM_OFFSET_ADDR;
	for(i = 0x0; i < DMEM_LEN;){
		tmp32 = (readw(pr_to32) & 0x0000ffff);
		pr_to32 += 4;
		tmp32 += ((readw(pr_to32) & 0x0000ffff) << 16);
		if(tmp32 != readl(pr_from32)){
			printf("%lx %lx\n", pr_from32, pr_to32);
			error++;
		}
		pr_from32 += 4;
		pr_to32 += 4;
		i += 4;
	}

	if(error){
		printf("check ddr_dmem code fail=%d",error);
	}else{
		printf("check ddr_dmem code pass\n");
	}
}
