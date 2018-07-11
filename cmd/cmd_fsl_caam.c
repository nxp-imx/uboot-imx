/*
 * Copyright (C) 2012-2016 Freescale Semiconductor, Inc.
 * Copyright 2018 NXP
 *
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */


#include <common.h>
#include <command.h>
#include <fsl_caam.h>

static int do_caam(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{

	int ret, i;

	if (argc < 2)
	return CMD_RET_USAGE;

	if (strcmp(argv[1], "genblob") == 0) {

	if (argc != 5)
	    return CMD_RET_USAGE;

	void *data_addr;
	void *blob_addr;
	int size;

	data_addr = (void *)simple_strtoul(argv[2], NULL, 16);
	blob_addr = (void *)simple_strtoul(argv[3], NULL, 16);
	size      = simple_strtoul(argv[4], NULL, 10);
	if (size <= 48)
		return CMD_RET_USAGE;

	caam_open();
	ret = caam_gen_blob((uint32_t)data_addr, (uint32_t)blob_addr, (uint32_t)size);

	if(ret != SUCCESS){
		printf("Error during blob encap operation: 0x%x\n", ret);
		return 0;
	}

	/* Print the generated DEK blob */
	printf("DEK blob is available at 0x%08X and equals:\n",(unsigned int)blob_addr);
	for(i=0;i<size;i++)
		printf("%02X ",((uint8_t *)blob_addr)[i]);
	printf("\n\n");


	return 1;

	}

	else if (strcmp(argv[1], "decap") == 0){

	if (argc != 5)
		return CMD_RET_USAGE;

	void *blob_addr;
	void *data_addr;
	int size;

	blob_addr = (void *)simple_strtoul(argv[2], NULL, 16);
	data_addr = (void *)simple_strtoul(argv[3], NULL, 16);
	size      = simple_strtoul(argv[4], NULL, 10);
	if (size <= 48)
		return CMD_RET_USAGE;

	caam_open();
	ret = caam_decap_blob((uint32_t)(data_addr), (uint32_t)(blob_addr), (uint32_t)size);
	if(ret != SUCCESS)
		printf("Error during blob decap operation: 0x%x\n", ret);
	else {
		printf("Success, blob decap at SM PAGE1 original data is:\n");
		int i = 0;
		for (i = 0; i < size; i++) {
		printf("0x%x  ",*(unsigned char*)(data_addr+i));
		if (i % 16 == 0)
			printf("\n");
		}
		printf("\n");
	}

	return 1;
	}

	return CMD_RET_USAGE;
}

U_BOOT_CMD(
	caam, 5, 1, do_caam,
	"Freescale i.MX CAAM command",
	"caam genblob data_addr blob_addr data_size\n \
	caam decap blobaddr data_addr data_size\n \
	\n "
	);
