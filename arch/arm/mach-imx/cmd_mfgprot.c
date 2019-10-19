/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier: GPL-2.0+
 *
 * These commands enable the use of the CAAM MPPubK-generation and MPSign
 * functions in supported i.MX devices.
 */

#include <command.h>
#include <common.h>
#include <environment.h>
#include <mapmem.h>
#include <memalign.h>
#ifdef CONFIG_IMX_CAAM_MFG_PROT
#include <asm/arch/clock.h>
#include <fsl_sec.h>
#endif
#ifdef CONFIG_IMX_SECO_MFG_PROT
#include <asm/io.h>
#include <asm/arch/sci/sci.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

/**
 * do_mfgprot() - Handle the "mfgprot" command-line command
 * @cmdtp:  Command data struct pointer
 * @flag:   Command flag
 * @argc:   Command-line argument count
 * @argv:   Array of command-line arguments
 *
 * Returns zero on success, CMD_RET_USAGE in case of misuse and negative
 * on error.
 */
#ifdef CONFIG_IMX_CAAM_MFG_PROT

static int do_mfgprot(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	u8 *m_ptr, *dgst_ptr, *c_ptr, *d_ptr, *dst_ptr;
	char *pubk, *sign, *sel;
	int m_size, i, ret;
	u32 m_addr;

	pubk = "pubk";
	sign = "sign";
	sel = argv[1];

	/* Enable HAB clock */
	hab_caam_clock_enable(1);

	u32 out_jr_size = sec_in32(CONFIG_SYS_FSL_JR0_ADDR +
				   FSL_CAAM_ORSR_JRa_OFFSET);

	if (out_jr_size != FSL_CAAM_MAX_JR_SIZE)
		sec_init();

	if (strcmp(sel, pubk) == 0) {
		dst_ptr = malloc_cache_aligned(FSL_CAAM_MP_PUBK_BYTES);
		if (!dst_ptr)
			return -ENOMEM;

		ret = gen_mppubk(dst_ptr);
		if (ret) {
			free(dst_ptr);
			return ret;
		}

		/* Output results */
		puts("Public key:\n");
		for (i = 0; i < FSL_CAAM_MP_PUBK_BYTES; i++)
			printf("%02X", (dst_ptr)[i]);
		puts("\n");
		free(dst_ptr);

	} else if (strcmp(sel, sign) == 0) {
		if (argc != 4)
			return CMD_RET_USAGE;

		m_addr = simple_strtoul(argv[2], NULL, 16);
		m_size = simple_strtoul(argv[3], NULL, 10);
		m_ptr = map_physmem(m_addr, m_size, MAP_NOCACHE);
		if (!m_ptr)
			return -ENOMEM;

		dgst_ptr = malloc_cache_aligned(FSL_CAAM_MP_MES_DGST_BYTES);
		if (!dgst_ptr) {
			ret = -ENOMEM;
			goto free_m;
		}

		c_ptr = malloc_cache_aligned(FSL_CAAM_MP_PRVK_BYTES);
		if (!c_ptr) {
			ret = -ENOMEM;
			goto free_dgst;
		}

		d_ptr = malloc_cache_aligned(FSL_CAAM_MP_PRVK_BYTES);
		if (!d_ptr) {
			ret = -ENOMEM;
			goto free_c;
		}

		ret = sign_mppubk(m_ptr, m_size, dgst_ptr, c_ptr, d_ptr);
		if (ret)
			goto free_d;

		/* Output results */
		puts("Message: ");
		for (i = 0; i < m_size; i++)
			printf("%02X ", (m_ptr)[i]);
		puts("\n");

		puts("Message Representative Digest(SHA-256):\n");
		for (i = 0; i < FSL_CAAM_MP_MES_DGST_BYTES; i++)
			printf("%02X", (dgst_ptr)[i]);
		puts("\n");

		puts("Signature:\n");
		puts("C:\n");
		for (i = 0; i < FSL_CAAM_MP_PRVK_BYTES; i++)
			printf("%02X", (c_ptr)[i]);
		puts("\n");

		puts("d:\n");
		for (i = 0; i < FSL_CAAM_MP_PRVK_BYTES; i++)
			printf("%02X", (d_ptr)[i]);
		puts("\n");
free_d:
	free(d_ptr);
free_c:
	free(c_ptr);
free_dgst:
	free(dgst_ptr);
free_m:
	unmap_sysmem(m_ptr);

	} else {
		return CMD_RET_USAGE;
	}
	return ret;
}
#endif /* CONFIG_IMX_CAAM_MFG_PROT */

#ifdef CONFIG_IMX_SECO_MFG_PROT

#define FSL_CAAM_MP_PUBK_BYTES			96
#define FSL_CAAM_MP_SIGN_BYTES			96
#define SCU_SEC_SECURE_RAM_BASE			(0x20800000UL)
#define SEC_SECURE_RAM_BASE			(0x31800000UL)

static int do_mfgprot(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	u8 *m_ptr, *sign_ptr, *dst_ptr;
	char *pubk, *sign, *sel;
	int m_size, i, ret;
	u32 m_addr;

	pubk = "pubk";
	sign = "sign";
	sel = argv[1];

	if (!sel)
		return CMD_RET_USAGE;

	if (strcmp(sel, pubk) == 0) {
		dst_ptr = malloc_cache_aligned(FSL_CAAM_MP_PUBK_BYTES);
		if (!dst_ptr)
			return -ENOMEM;

		puts("\nGenerating Manufacturing Protection Public Key\n");

		ret = sc_seco_get_mp_key(-1, SCU_SEC_SECURE_RAM_BASE,
					 FSL_CAAM_MP_PUBK_BYTES);
		if (ret) {
			printf("SECO get MP key failed, return %d\n", ret);
			ret = -EIO;
			free(dst_ptr);
			return ret;
		}

		memcpy((void *)dst_ptr, (const void *)SEC_SECURE_RAM_BASE,
			ALIGN(FSL_CAAM_MP_PUBK_BYTES,
			CONFIG_SYS_CACHELINE_SIZE));

		/* Output results */
		puts("\nPublic key:\n");
		for (i = 0; i < FSL_CAAM_MP_PUBK_BYTES; i++)
			printf("%02X", (dst_ptr)[i]);
		puts("\n");
		free(dst_ptr);

	} else if (strcmp(sel, sign) == 0) {
		if (argc != 4)
			return CMD_RET_USAGE;

		m_addr = simple_strtoul(argv[2], NULL, 16);
		m_size = simple_strtoul(argv[3], NULL, 10);
		m_ptr = map_physmem(m_addr, m_size, MAP_NOCACHE);
		if (!m_ptr)
			return -ENOMEM;

		sign_ptr = malloc_cache_aligned(FSL_CAAM_MP_SIGN_BYTES);
		if (!sign_ptr) {
			ret = -ENOMEM;
			goto free_m;
		}

		memcpy((void *)SEC_SECURE_RAM_BASE, (const void *)m_ptr,
			ALIGN(m_size, CONFIG_SYS_CACHELINE_SIZE));

		puts("\nSigning message with SECO MP signature function\n");

		ret = sc_seco_get_mp_sign(-1, SCU_SEC_SECURE_RAM_BASE, m_size,
					  SCU_SEC_SECURE_RAM_BASE + 0x1000,
					  FSL_CAAM_MP_SIGN_BYTES);

		if (ret) {
			printf("SECO get MP signature failed, return %d\n",
			       ret);
			ret = -EIO;
			goto free_sign;
		}

		memcpy((void *)sign_ptr, (const void *)SEC_SECURE_RAM_BASE
			+ 0x1000, ALIGN(FSL_CAAM_MP_SIGN_BYTES,
			CONFIG_SYS_CACHELINE_SIZE));

		/* Output results */
		puts("\nMessage: ");
		for (i = 0; i < m_size; i++)
			printf("%02X", (m_ptr)[i]);
		puts("\n");

		puts("\nSignature:\n");
		puts("c:\n");
		for (i = 0; i < FSL_CAAM_MP_SIGN_BYTES / 2; i++)
			printf("%02X", (sign_ptr)[i]);
		puts("\n");

		puts("d:\n");
		for (i = FSL_CAAM_MP_SIGN_BYTES / 2; i < FSL_CAAM_MP_SIGN_BYTES;
		     i++)
			printf("%02X", (sign_ptr)[i]);
		puts("\n");

free_sign:
	free(sign_ptr);
free_m:
	unmap_sysmem(m_ptr);

	} else {
		return CMD_RET_USAGE;
	}
	return ret;
}
#endif /* CONFIG_IMX_SECO_MFG_PROT */

/***************************************************/
static char mfgprot_help_text[] =
	"Usage:\n"
	 "Print the public key for Manufacturing Protection\n"
	 "\tmfgprot pubk\n"
	 "Generates a Manufacturing Protection signature\n"
	 "\tmfgprot sign <data_addr> <size>";

U_BOOT_CMD(
	mfgprot, 4, 1, do_mfgprot,
	"Manufacturing Protection\n",
	mfgprot_help_text
);
