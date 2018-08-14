/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
 */

#include <common.h>
#include <errno.h>
#include <asm/io.h>
#include <asm/mach-imx/sci/sci.h>
#include <asm/mach-imx/sys_proto.h>
#include <asm/arch-imx/cpu.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/cpu.h>
#include <asm/arch/image.h>

DECLARE_GLOBAL_DATA_PTR;

#define SEC_SECURE_RAM_BASE             (0x31800000UL)
#define SEC_SECURE_RAM_END_BASE         (SEC_SECURE_RAM_BASE + 0xFFFFUL)
#define SECO_LOCAL_SEC_SEC_SECURE_RAM_BASE  (0x60000000UL)

#define SECO_PT                 2U

int authenticate_os_container(ulong addr)
{
	struct container_hdr *phdr;
	int i, ret = 0;
	sc_ipc_t ipcHndl = gd->arch.ipc_channel_handle;
	sc_err_t err;
	sc_rm_mr_t mr;
	sc_faddr_t start, end;
	uint16_t length;

	if (addr % 4)
		return -EINVAL;

	phdr = (struct container_hdr *)addr;
	if (phdr->tag != 0x87 && phdr->version != 0x0) {
		printf("Wrong container header\n");
		return -EFAULT;
	}

	if (!phdr->num_images) {
		printf("Wrong container, no image found\n");
		return -EFAULT;
	}

	length = phdr->length_lsb + (phdr->length_msb << 8);

	debug("container length %u\n", length);
	memcpy((void *)SEC_SECURE_RAM_BASE, (const void *)addr, ALIGN(length, CONFIG_SYS_CACHELINE_SIZE));

	err = sc_misc_seco_authenticate(ipcHndl, SC_MISC_AUTH_CONTAINER, SECO_LOCAL_SEC_SEC_SECURE_RAM_BASE);
	if (err) {
		printf("authenticate container hdr failed, return %d\n", err);
		ret = -EIO;
		goto exit;
	}

	/* Copy images to dest address */
	for (i=0; i < phdr->num_images; i++) {
		struct boot_img_t *img = (struct boot_img_t *)(addr + sizeof(struct container_hdr) + i * sizeof(struct boot_img_t));

		debug("img %d, dst 0x%llx, src 0x%lx, size 0x%x\n", i, img->dst, img->offset + addr, img->size);

		memcpy((void *)img->dst, (const void *)(img->offset + addr), img->size);
		flush_dcache_range(img->dst & ~(CONFIG_SYS_CACHELINE_SIZE - 1),
				ALIGN(img->dst + img->size, CONFIG_SYS_CACHELINE_SIZE));

		/* Find the memreg and set permission for seco pt */
		err = sc_rm_find_memreg(ipcHndl, &mr,
			img->dst & ~(CONFIG_SYS_CACHELINE_SIZE - 1), ALIGN(img->dst + img->size, CONFIG_SYS_CACHELINE_SIZE));

		if (err) {
			printf("can't find memreg for image load address %d, error %d\n", i, err);
			ret = -ENOMEM;
			goto exit;
		}

		err = sc_rm_get_memreg_info(ipcHndl, mr, &start, &end);
		if (!err)
			debug("memreg %u 0x%llx -- 0x%llx\n", mr, start, end);

		err = sc_rm_set_memreg_permissions(ipcHndl, mr, SECO_PT, SC_RM_PERM_FULL);
		if (err) {
			printf("set permission failed for img %d, error %d\n", i, err);
			ret = -EPERM;
			goto exit;
		}

		err = sc_misc_seco_authenticate(ipcHndl, SC_MISC_VERIFY_IMAGE, (1 << i));
		if (err) {
			printf("authenticate img %d failed, return %d\n", i, err);
			ret = -EIO;
		}

		err = sc_rm_set_memreg_permissions(ipcHndl, mr, SECO_PT, SC_RM_PERM_NONE);
		if (err) {
			printf("remove permission failed for img %d, error %d\n", i, err);
			ret = -EPERM;
		}

		if (ret)
			goto exit;
	}

exit:
	sc_misc_seco_authenticate(ipcHndl, SC_MISC_REL_CONTAINER, 0);

	return ret;
}


static int do_authenticate(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	ulong addr;
	if (argc < 2)
		return CMD_RET_USAGE;

	addr = simple_strtoul(argv[1], NULL, 16);

	printf("Authenticate OS container at 0x%lx \n", addr);

	if (authenticate_os_container(addr))
		return CMD_RET_FAILURE;

	return CMD_RET_SUCCESS;
}

U_BOOT_CMD(
	auth_cntr, CONFIG_SYS_MAXARGS, 1, do_authenticate,
	"autenticate OS container via AHAB",
	"addr\n"
	"addr - OS container hex address\n"
);
