/*
 * Copyright (c) 2012-2016, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/crm_regs.h>
#include "fsl_caam_internal.h"
#include <fsl_caam.h>

/*---------- Global variables ----------*/
/* Input job ring - single entry input ring */
uint32_t g_input_ring[JOB_RING_ENTRIES] = {0};

/* Output job ring - single entry output ring (consists of two words) */    
uint32_t g_output_ring[2*JOB_RING_ENTRIES] = {0, 0};

uint32_t decap_dsc[] = 
{
	DECAP_BLOB_DESC1,
	DECAP_BLOB_DESC2,
	DECAP_BLOB_DESC3,
	DECAP_BLOB_DESC4,
	DECAP_BLOB_DESC5,
	DECAP_BLOB_DESC6,
	DECAP_BLOB_DESC7,
	DECAP_BLOB_DESC8,
	DECAP_BLOB_DESC9
};

uint32_t encap_dsc[] = 
{
	ENCAP_BLOB_DESC1,
	ENCAP_BLOB_DESC2,
	ENCAP_BLOB_DESC3,
	ENCAP_BLOB_DESC4,
	ENCAP_BLOB_DESC5,
	ENCAP_BLOB_DESC6,
	ENCAP_BLOB_DESC7,
	ENCAP_BLOB_DESC8,
	ENCAP_BLOB_DESC9
};

uint32_t rng_inst_dsc[] = 
{
	RNG_INST_DESC1,
	RNG_INST_DESC2,
	RNG_INST_DESC3,
	RNG_INST_DESC4,
	RNG_INST_DESC5,
	RNG_INST_DESC6,
	RNG_INST_DESC7,
	RNG_INST_DESC8,
	RNG_INST_DESC9
};

static uint8_t skeymod[] = {
	0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08,
	0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00
};

/*!
 * Secure memory run command.
 *
 * @param   sec_mem_cmd  Secure memory command register
 * @return  cmd_status  Secure memory command status register
 */
uint32_t secmem_set_cmd_1(uint32_t sec_mem_cmd)
{
	uint32_t temp_reg;
	__raw_writel(sec_mem_cmd, CAAM_SMCJR0);
	do {
	temp_reg = __raw_readl(CAAM_SMCSJR0);
	} while(temp_reg & CMD_COMPLETE);

	return temp_reg;
}

/*!
 * CAAM page allocation.
 *
 * @param   page  Number of the page to allocate.
 * @param   partition  Number of the partition to allocate.
 */
static uint32_t caam_page_alloc(uint8_t page_num, uint8_t partition_num)
{
	uint32_t temp_reg;

	/* 
	 * De-Allocate partition_num if already allocated to ARM core
	 */
	if(__raw_readl(CAAM_SMPO_0) & PARTITION_OWNER(partition_num))
	{
		temp_reg = secmem_set_cmd_1(PARTITION(partition_num) | CMD_PART_DEALLOC);
		if(temp_reg & SMCSJR_AERR)
		{
		printf("Error: De-allocation status 0x%X\n",temp_reg);
		return ERROR_IN_PAGE_ALLOC;
		}
	}

	/* set the access rights to allow full access */ 
	__raw_writel(0xF, CAAM_SMAG1JR0(partition_num));
	__raw_writel(0xF, CAAM_SMAG2JR0(partition_num));
	__raw_writel(0xFF, CAAM_SMAPJR0(partition_num));

	/* Now need to allocate partition_num of secure RAM. */    
	/* De-Allocate page_num by starting with a page inquiry command */
	temp_reg = secmem_set_cmd_1(PAGE(page_num) | CMD_INQUIRY);
	/* if the page is owned, de-allocate it */
	if((temp_reg & SMCSJR_PO) == PAGE_OWNED)
	{
		temp_reg = secmem_set_cmd_1(PAGE(page_num) | CMD_PAGE_DEALLOC);
		if(temp_reg & SMCSJR_AERR)
	{
	  printf("Error: Allocation status 0x%X\n",temp_reg);
	  return ERROR_IN_PAGE_ALLOC;
		}
	}

	/* Allocate page_num to partition_num */
	temp_reg = secmem_set_cmd_1(PAGE(page_num) | PARTITION(partition_num)
		| CMD_PAGE_ALLOC);
	if(temp_reg & SMCSJR_AERR)
	{
		printf("Error: Allocation status 0x%X\n",temp_reg);
		return ERROR_IN_PAGE_ALLOC;
	}
	/* page inquiry command to ensure that the page was allocated */
	temp_reg = secmem_set_cmd_1(PAGE(page_num) | CMD_INQUIRY);
	/* if the page is not owned => problem */
	if((temp_reg & SMCSJR_PO) != PAGE_OWNED)
	{
		printf("Error: Allocation of page %d in partition %d failed 0x%X\n"
		,temp_reg, page_num, partition_num);

		return ERROR_IN_PAGE_ALLOC;
	}

	return SUCCESS;
}

/*!
 * Use CAAM to decapsulate a blob to secure memory.
 * Such blob of secret key cannot be read once decrypted,
 * but can still be used for enc/dec operation of user's data.
 *
 * @param   blob_addr  Location address of the blob.
 *
 * @return  SUCCESS or ERROR_XXX
 */
uint32_t caam_decap_blob(uint32_t plain_text, uint32_t blob_addr, uint32_t size)
{
	uint32_t ret = SUCCESS;

	/* Buffer that holds blob */
	uint8_t *blob = (uint8_t *)blob_addr;

	/**** Prepare partition and page, and start the job to create the blob ***/
#if 0
    ret = caam_page_alloc(PAGE_1, PARTITION_1);
    if(ret != SUCCESS)
        return ret;

    /* Now configure the access rights of the partition */
    __raw_writel(KS_G1, CAAM_SMAG1JR0(PARTITION_1)); // set group 1
    __raw_writel(0, CAAM_SMAG2JR0(PARTITION_1));     // clear group 2
	__raw_writel(PERM, CAAM_SMAPJR0(PARTITION_1));   // set permissions & locks
#endif

	/* TODO: Fix Hardcoded Descriptor */
	decap_dsc[0] = (uint32_t)0xB0800008;
	decap_dsc[1] = (uint32_t)0x14400010;
	decap_dsc[2] = (uint32_t)skeymod;
	decap_dsc[3] = (uint32_t)0xF0000000 | (0x0000ffff & (size+48) );
	decap_dsc[4] = blob_addr;
	decap_dsc[5] = (uint32_t)0xF8000000 | (0x0000ffff & (size));
	decap_dsc[6] = (uint32_t)(uint8_t*)plain_text;
	decap_dsc[7] = (uint32_t)0x860D0000;

// uncomment when using descriptor from "fsl_caam_internal.h"
// does not use key modifier.
#if 0  
    /* Fill in input blob addr in decap_dsc */
    decap_dsc[5] = (uint32_t)blob;
    /* Fill in the address where to decrypt the blob */
    decap_dsc[7] = (uint32_t)SEC_MEM_PAGE1;
#endif

    /* Run descriptor with result written to blob buffer */
    /* Add job to input ring */
	g_input_ring[0] = (uint32_t)decap_dsc;

	flush_dcache_range((uint32_t)blob_addr & 0xffffffe0, ((uint32_t)blob_addr & 0xffffffe0) + 2*size);
	flush_dcache_range((uint32_t)plain_text& 0xffffffe0, ((uint32_t)plain_text& 0xffffffe0) + 2*size);
	flush_dcache_range((uint32_t)decap_dsc & 0xffffffe0, ((uint32_t)decap_dsc & 0xffffffe0) + 128); 
	flush_dcache_range((uint32_t)g_input_ring & 0xffffffe0, ((uint32_t)g_input_ring & 0xffffffe0) + 128);
    /* Increment jobs added */
	__raw_writel(1, CAAM_IRJAR0);

    /* Wait for job ring to complete the job: 1 completed job expected */
	while(__raw_readl(CAAM_ORSFR0) != 1);

	// TODO: check if Secure memory is cacheable.
	invalidate_dcache_range((uint32_t)g_output_ring & 0xffffffe0, ((uint32_t)g_output_ring & 0xffffffe0) + 128);
	/* check that descriptor address is the one expected in the output ring */
	if(g_output_ring[0] == (uint32_t)decap_dsc)
	{
		/* check if any error is reported in the output ring */
		if ((g_output_ring[1] & JOB_RING_STS) != 0)
		{
			printf("Error: blob decap job completed with errors 0x%X\n",
						g_output_ring[1]);
		}
	}
	else
	{
		printf("Error: blob decap job output ring descriptor address does" \
	                " not match\n");
	}
	flush_dcache_range((uint32_t)plain_text& 0xffffffe0, ((uint32_t)plain_text& 0xffffffe0) + 2*size);


	/* Remove job from Job Ring Output Queue */
	__raw_writel(1, CAAM_ORJRR0);

	return ret;
}

/*!
 * Use CAAM to generate a blob.
 *
 * @param   plain_data_addr  Location address of the plain data.
 * @param   blob_addr  Location address of the blob.
 *
 * @return  SUCCESS or ERROR_XXX
 */
uint32_t caam_gen_blob(uint32_t plain_data_addr, uint32_t blob_addr, uint32_t size)
{
	uint32_t ret = SUCCESS;
	uint32_t addr;

	/* Buffer to hold the resulting blob */
	uint8_t *blob = (uint8_t *)blob_addr;

	/* initialize the blob array */
	memset(blob,0,size);

    /**** Prepare partition and page, and start the job to create the blob ***/
#if 0
    ret = caam_page_alloc(PAGE_1, PARTITION_1);
    if(ret != SUCCESS)
	return ret;

    /* Write the DEK to the partition. */
    memcpy((uint32_t*)SEC_MEM_PAGE1, (uint32_t*)plain_data_addr, size);

    /* Now configure the access rights of the partition */
    //  __raw_writel(KS_G1, CAAM_SMAG1JR0(PARTITION_1)); // set group 1
    //  __raw_writel(0, CAAM_SMAG2JR0(PARTITION_1));     // clear group 2
    //   __raw_writel(PERM, CAAM_SMAPJR0(PARTITION_1));   // set permissions & locks
#endif

    /* TODO: Fix Hardcoded Descriptor */
	encap_dsc[0] = (uint32_t)0xB0800008;
	encap_dsc[1] = (uint32_t)0x14400010;
	encap_dsc[2] = (uint32_t)skeymod;
	encap_dsc[3] = (uint32_t)0xF0000000 | (0x0000ffff & (size));
	encap_dsc[4] = (uint32_t)plain_data_addr;
	encap_dsc[5] = (uint32_t)0xF8000000 | (0x0000ffff & (size+48));
	encap_dsc[6] = (uint32_t)blob;	
	encap_dsc[7] = (uint32_t)0x870D0000;

    // uncomment when using descriptor from "fsl_caam_internal.h"
    // does not use key modifier.	
#if 0
    //
    /* Fill in the address where the DEK resides */
    encap_dsc[5] = (uint32_t)SEC_MEM_PAGE1;    
    /* Fill in output blob addr in encap_dsc */
    encap_dsc[7] = (uint32_t)blob;
#endif

    /* Run descriptor with result written to blob buffer */
    /* Add job to input ring */
	g_input_ring[0] = (uint32_t)encap_dsc;

	flush_dcache_range((uint32_t)plain_data_addr& 0xffffffe0, ((uint32_t)plain_data_addr& 0xffffffe0) + size);
	flush_dcache_range((uint32_t)encap_dsc & 0xffffffe0, ((uint32_t)encap_dsc & 0xffffffe0) + 128);
	flush_dcache_range((uint32_t)blob & 0xffffffe0, ((uint32_t)g_input_ring & 0xffffffe0) + 2 * size);
	/* Increment jobs added */
	__raw_writel(1, CAAM_IRJAR0);

    /* Wait for job ring to complete the job: 1 completed job expected */
	while(__raw_readl(CAAM_ORSFR0) != 1);

    // flush cache
	invalidate_dcache_range((uint32_t)g_output_ring & 0xffffffe0, ((uint32_t)g_output_ring & 0xffffffe0) + 128);
	invalidate_dcache_range((uint32_t)g_output_ring & 0xffffffe0, ((uint32_t)g_output_ring & 0xffffffe0) + 128);
	/* check that descriptor address is the one expected in the output ring */
	if(g_output_ring[0] == (uint32_t)encap_dsc)
	{
	/* check if any error is reported in the output ring */
		if ((g_output_ring[1] & JOB_RING_STS) != 0)
		{
			printf("Error: blob encap job completed with errors 0x%X\n",
			      g_output_ring[1]);
		}
	}
	else
	{
	printf("Error: blob encap job output ring descriptor address does" \
		" not match\n");
	}

	/* Remove job from Job Ring Output Queue */
	__raw_writel(1, CAAM_ORJRR0);

	return ret;
}

/*!
 * Initialize the CAAM.
 *
 */
void caam_open(void)
{
	struct mxc_ccm_reg *mxc_ccm = (struct mxc_ccm_reg *)CCM_BASE_ADDR;
	uint32_t temp_reg;
	//uint32_t addr;

    /* switch on the clock */
	temp_reg = __raw_readl(&mxc_ccm->CCGR0);
	temp_reg |= MXC_CCM_CCGR0_CAAM_SECURE_MEM_MASK | 
		MXC_CCM_CCGR0_CAAM_WRAPPER_ACLK_MASK | 
		MXC_CCM_CCGR0_CAAM_WRAPPER_IPG_MASK;
	__raw_writel(temp_reg, &mxc_ccm->CCGR0);

    /* MID for CAAM - already done by HAB in ROM during preconfigure,
     * That is JROWN for JR0/1 = 1 (TZ, Secure World, ARM)
     * JRNSMID and JRSMID for JR0/1 = 2 (TZ, Secure World, CAAM)
     *
     * However, still need to initialize Job Rings as these are torn
     * down by HAB for each command
     */    

    /* Initialize job ring addresses */
	__raw_writel((uint32_t)g_input_ring, CAAM_IRBAR0);   // input ring address
	__raw_writel((uint32_t)g_output_ring, CAAM_ORBAR0);  // output ring address

	/* Initialize job ring sizes to 1 */
	__raw_writel(JOB_RING_ENTRIES, CAAM_IRSR0);
	__raw_writel(JOB_RING_ENTRIES, CAAM_ORSR0);

    /* HAB disables interrupts for JR0 so do the same here */
	temp_reg = __raw_readl(CAAM_JRCFGR0_LS) | JRCFG_LS_IMSK;
	__raw_writel(temp_reg, CAAM_JRCFGR0_LS);    

    /********* Initialize and instantiate the RNG *******************/
    /* if RNG already instantiated then skip it */
	if ((__raw_readl(CAAM_RDSTA) & RDSTA_IF0) != RDSTA_IF0)
	{
	/* Enter TRNG Program mode */
	__raw_writel(RTMCTL_PGM, CAAM_RTMCTL);

	/* Set OSC_DIV field to TRNG */
	temp_reg = __raw_readl(CAAM_RTMCTL) | (RNG_TRIM_OSC_DIV << 2);
	__raw_writel(temp_reg, CAAM_RTMCTL);

	/* Set delay */
	__raw_writel(((RNG_TRIM_ENT_DLY << 16) | 0x09C4), CAAM_RTSDCTL);   
	__raw_writel((RNG_TRIM_ENT_DLY >> 1), CAAM_RTFRQMIN);
	__raw_writel((RNG_TRIM_ENT_DLY << 4), CAAM_RTFRQMAX);

	/* Resume TRNG Run mode */
	temp_reg = __raw_readl(CAAM_RTMCTL) ^ RTMCTL_PGM;
	__raw_writel(temp_reg, CAAM_RTMCTL);   

	/* Clear the ERR bit in RTMCTL if set. The TRNG error can occur when the
	 * RNG clock is not within 1/2x to 8x the system clock.
	 * This error is possible if ROM code does not initialize the system PLLs
	 * immediately after PoR.
	 */
	temp_reg = __raw_readl(CAAM_RTMCTL) | RTMCTL_ERR;
	__raw_writel(temp_reg, CAAM_RTMCTL);

	/* Run descriptor to instantiate the RNG */
	/* Add job to input ring */
	g_input_ring[0] = (uint32_t)rng_inst_dsc;

	flush_dcache_range((uint32_t)g_input_ring & 0xffffffe0, ((uint32_t)g_input_ring & 0xffffffe0) + 128);
	/* Increment jobs added */
	__raw_writel(1, CAAM_IRJAR0); 

	/* Wait for job ring to complete the job: 1 completed job expected */
	while(__raw_readl(CAAM_ORSFR0) != 1);


	invalidate_dcache_range((uint32_t)g_output_ring & 0xffffffe0, ((uint32_t)g_output_ring & 0xffffffe0) + 128);

	/* check that descriptor address is the one expected in the out ring */
	if(g_output_ring[0] == (uint32_t)rng_inst_dsc)
	{
		/* check if any error is reported in the output ring */
		if ((g_output_ring[1] & JOB_RING_STS) != 0)
		{
		printf("Error: RNG instantiation errors g_output_ring[1]: 0x%X\n"
			, g_output_ring[1]);
		printf("RTMCTL 0x%X\n", __raw_readl(CAAM_RTMCTL));
		printf("RTSTATUS 0x%X\n", __raw_readl(CAAM_RTSTATUS));
		printf("RTSTA 0x%X\n", __raw_readl(CAAM_RDSTA));
            }
	}
	else
	{
		printf("Error: RNG job output ring descriptor address does " \
				"not match: 0x%X != 0x%X \n", g_output_ring[0], rng_inst_dsc[0]);
	}

		/* ensure that the RNG was correctly instantiated */
		temp_reg = __raw_readl(CAAM_RDSTA);
		if (temp_reg != (RDSTA_IF0 | RDSTA_SKVN))
		{
		
			printf("Error: RNG instantiation failed 0x%X\n", temp_reg);
		}
		
		/* Remove job from Job Ring Output Queue */
		__raw_writel(1, CAAM_ORJRR0);
	}
		
	return;
}
