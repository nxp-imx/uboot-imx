// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2017-2020 NXP
 */

#include <common.h>
#include <clk.h>
#include <cpu.h>
#include <cpu_func.h>
#include <dm.h>
#include <init.h>
#include <dm/device-internal.h>
#include <dm/lists.h>
#include <dm/uclass.h>
#include <errno.h>
#include <asm/arch/clock.h>
#include <power-domain.h>
#include <dm/device.h>
#include <dm/uclass-internal.h>
#include <thermal.h>
#include <asm/arch/sci/sci.h>
#include <power-domain.h>
#include <elf.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch-imx/cpu.h>
#include <asm/armv8/cpu.h>
#include <asm/armv8/mmu.h>
#include <asm/setup.h>
#include <asm/mach-imx/boot_mode.h>
#include <asm/mach-imx/imx_vservice.h>
#include <spl.h>
#include <usb/ci_udc.h>

DECLARE_GLOBAL_DATA_PTR;

#define BT_PASSOVER_TAG	0x504F
struct pass_over_info_t *get_pass_over_info(void)
{
	struct pass_over_info_t *p =
		(struct pass_over_info_t *)PASS_OVER_INFO_ADDR;

	if (p->barker != BT_PASSOVER_TAG ||
	    p->len != sizeof(struct pass_over_info_t))
		return NULL;

	return p;
}

#if defined(CONFIG_SPL_BUILD) && defined(CONFIG_RECOVER_SPL_DATA_SECTION)
char __data_save_start[0] __attribute__((section(".__data_save_start")));
char __data_save_end[0] __attribute__((section(".__data_save_end")));

u32 cold_reboot_flag = 1;

static void save_restore_data(void)
{
	u32 data_size = __data_save_end - __data_save_start;

	if (cold_reboot_flag == 1) {
		/* Save data section to data_save section */
		memcpy(__data_save_start, __data_save_start - data_size, data_size);
	} else {
		/* Restore the data_save section to data section */
		memcpy(__data_save_start - data_size, __data_save_start, data_size);
	}
	cold_reboot_flag++;
}
#endif

int arch_cpu_init(void)
{
#if defined(CONFIG_SPL_BUILD) && defined(CONFIG_RECOVER_SPL_DATA_SECTION)
	save_restore_data();
#endif

	return 0;
}

static void power_off_all_usb(void);

int arch_cpu_init_dm(void)
{
	struct udevice *devp;
	int node, ret;

	node = fdt_node_offset_by_compatible(gd->fdt_blob, -1, "fsl,imx8-mu");

	ret = uclass_get_device_by_of_offset(UCLASS_MISC, node, &devp);
	if (ret) {
		printf("could not get scu %d\n", ret);
		return ret;
	}

	if (IS_ENABLED(CONFIG_XEN))
		return 0;

	struct pass_over_info_t *pass_over;

	if ((is_imx8qm() || is_imx8qxp()) && is_soc_rev(CHIP_REV_A)) {
		pass_over = get_pass_over_info();
		if (pass_over && pass_over->g_ap_mu == 0) {
			/*
			 * When ap_mu is 0, means the U-Boot booted
			 * from first container
			 */
			sc_misc_boot_status(-1, SC_MISC_BOOT_STATUS_SUCCESS);
		}
	}

	if (is_imx8qm()) {
		ret = sc_pm_set_resource_power_mode(-1, SC_R_SMMU,
						    SC_PM_PW_MODE_ON);
		if (ret)
			return ret;
	}

	power_off_all_usb();

	return 0;
}

#ifdef CONFIG_IMX_BOOTAUX

#ifdef CONFIG_IMX8QM
int arch_auxiliary_core_up(u32 core_id, ulong boot_private_data)
{
	sc_rsrc_t core_rsrc, mu_rsrc;
	sc_faddr_t tcml_addr;
	u32 tcm_size = SZ_256K; /* TCML + TCMU */
	ulong addr;


	switch (core_id) {
	case 0:
		core_rsrc = SC_R_M4_0_PID0;
		tcml_addr = 0x34FE0000;
		mu_rsrc = SC_R_M4_0_MU_1A;
		break;
	case 1:
		core_rsrc = SC_R_M4_1_PID0;
		tcml_addr = 0x38FE0000;
		mu_rsrc = SC_R_M4_1_MU_1A;
		break;
	default:
		printf("Not support this core boot up, ID:%u\n", core_id);
		return -EINVAL;
	}

	addr = (sc_faddr_t)boot_private_data;

	if (addr >= tcml_addr && addr <= tcml_addr + tcm_size) {
		printf("Wrong image address 0x%lx, should not in TCML\n",
			addr);
		return -EINVAL;
	}

	printf("Power on M4 and MU\n");

	if (sc_pm_set_resource_power_mode(-1, core_rsrc, SC_PM_PW_MODE_ON) != SC_ERR_NONE)
		return -EIO;

	if (sc_pm_set_resource_power_mode(-1, mu_rsrc, SC_PM_PW_MODE_ON) != SC_ERR_NONE)
		return -EIO;

	printf("Copy M4 image from 0x%lx to TCML 0x%lx\n", addr, (ulong)tcml_addr);

	if (addr != tcml_addr)
		memcpy((void *)tcml_addr, (void *)addr, tcm_size);

	printf("Start M4 %u\n", core_id);
	if (sc_pm_cpu_start(-1, core_rsrc, true, tcml_addr) != SC_ERR_NONE)
		return -EIO;

	printf("bootaux complete\n");
	return 0;
}
#endif

#if defined(CONFIG_IMX8QXP) || defined(CONFIG_IMX8DXL)
static unsigned long load_elf_image_shdr(unsigned long addr)
{
	Elf32_Ehdr *ehdr; /* Elf header structure pointer */
	Elf32_Shdr *shdr; /* Section header structure pointer */
	unsigned char *strtab = 0; /* String table pointer */
	unsigned char *image; /* Binary image pointer */
	int i; /* Loop counter */

	ehdr = (Elf32_Ehdr *)addr;

	/* Find the section header string table for output info */
	shdr = (Elf32_Shdr *)(addr + ehdr->e_shoff +
			     (ehdr->e_shstrndx * sizeof(Elf32_Shdr)));

	if (shdr->sh_type == SHT_STRTAB)
		strtab = (unsigned char *)(addr + shdr->sh_offset);

	/* Load each appropriate section */
	for (i = 0; i < ehdr->e_shnum; ++i) {
		shdr = (Elf32_Shdr *)(addr + ehdr->e_shoff +
				     (i * sizeof(Elf32_Shdr)));

		if (!(shdr->sh_flags & SHF_ALLOC) ||
		    shdr->sh_addr == 0 || shdr->sh_size == 0) {
			continue;
		}

		if (strtab) {
			debug("%sing %s @ 0x%08lx (%ld bytes)\n",
			      (shdr->sh_type == SHT_NOBITS) ? "Clear" : "Load",
			       &strtab[shdr->sh_name],
			       (unsigned long)shdr->sh_addr,
			       (long)shdr->sh_size);
		}

		if (shdr->sh_type == SHT_NOBITS) {
			memset((void *)(uintptr_t)shdr->sh_addr, 0,
			       shdr->sh_size);
		} else {
			image = (unsigned char *)addr + shdr->sh_offset;
			memcpy((void *)(uintptr_t)shdr->sh_addr,
			       (const void *)image, shdr->sh_size);
		}
		flush_cache(shdr->sh_addr, shdr->sh_size);
	}

	return ehdr->e_entry;
}

int arch_auxiliary_core_up(u32 core_id, ulong boot_private_data)
{
	sc_rsrc_t core_rsrc, mu_rsrc = SC_R_NONE;
	sc_faddr_t aux_core_ram;
	u32 size;
	ulong addr;

	switch (core_id) {
	case 0:
		core_rsrc = SC_R_M4_0_PID0;
		aux_core_ram = 0x34FE0000;
		mu_rsrc = SC_R_M4_0_MU_1A;
		size = SZ_256K;
		break;
	case 1:
		core_rsrc = SC_R_DSP;
		aux_core_ram = 0x596f8000;
		size = SZ_2K;
		break;
	default:
		printf("Not support this core boot up, ID:%u\n", core_id);
		return -EINVAL;
	}

	addr = (sc_faddr_t)boot_private_data;

	if (addr >= aux_core_ram && addr <= aux_core_ram + size) {
		printf("Wrong image address 0x%lx, should not in aux core ram\n",
			addr);
		return -EINVAL;
	}

	printf("Power on aux core %d\n", core_id);

	if (sc_pm_set_resource_power_mode(-1, core_rsrc, SC_PM_PW_MODE_ON) != SC_ERR_NONE)
		return -EIO;

	if (mu_rsrc != SC_R_NONE) {
		if (sc_pm_set_resource_power_mode(-1, mu_rsrc, SC_PM_PW_MODE_ON) != SC_ERR_NONE)
			return -EIO;
	}

	if (core_id == 1) {
		struct power_domain pd;

		if (sc_pm_clock_enable(-1, core_rsrc, SC_PM_CLK_PER, true, false) != SC_ERR_NONE) {
			printf("Error enable clock\n");
			return -EIO;
		}

		if (!power_domain_lookup_name("audio_sai0", &pd)) {
			if (power_domain_on(&pd)) {
				printf("Error power on SAI0\n");
				return -EIO;
			}
		}

		if (!power_domain_lookup_name("audio_ocram", &pd)) {
			if (power_domain_on(&pd)) {
				printf("Error power on HIFI RAM\n");
				return -EIO;
			}
		}
	}

	printf("Copy image from 0x%lx to 0x%lx\n", addr, (ulong)aux_core_ram);
	if (core_id == 0) {
		/* M4 use bin file */
		memcpy((void *)aux_core_ram, (void *)addr, size);
	} else {
		/* HIFI use elf file */
		if (!valid_elf_image(addr))
			return -1;
		addr = load_elf_image_shdr(addr);
	}

	printf("Start %s\n", core_id == 0 ? "M4" : "HIFI");

	if (sc_pm_cpu_start(-1, core_rsrc, true, aux_core_ram) != SC_ERR_NONE)
		return -EIO;

	printf("bootaux complete\n");
	return 0;
}
#endif

int arch_auxiliary_core_check_up(u32 core_id)
{
	sc_rsrc_t core_rsrc;
	sc_pm_power_mode_t power_mode;

	switch (core_id) {
	case 0:
		core_rsrc = SC_R_M4_0_PID0;
		break;
#ifdef CONFIG_IMX8QM
	case 1:
		core_rsrc = SC_R_M4_1_PID0;
		break;
#endif
	default:
		printf("Not support this core, ID:%u\n", core_id);
		return 0;
	}

	if (sc_pm_get_resource_power_mode(-1, core_rsrc, &power_mode) != SC_ERR_NONE)
		return 0;

	if (power_mode != SC_PM_PW_MODE_OFF)
		return 1;

	return 0;
}
#endif

int print_bootinfo(void)
{
	enum boot_device bt_dev = get_boot_device();

	puts("Boot:  ");
	switch (bt_dev) {
	case SD1_BOOT:
		puts("SD0\n");
		break;
	case SD2_BOOT:
		puts("SD1\n");
		break;
	case SD3_BOOT:
		puts("SD2\n");
		break;
	case MMC1_BOOT:
		puts("MMC0\n");
		break;
	case MMC2_BOOT:
		puts("MMC1\n");
		break;
	case MMC3_BOOT:
		puts("MMC2\n");
		break;
	case FLEXSPI_BOOT:
		puts("FLEXSPI\n");
		break;
	case SATA_BOOT:
		puts("SATA\n");
		break;
	case NAND_BOOT:
		puts("NAND\n");
		break;
	case USB_BOOT:
		puts("USB\n");
		break;
	default:
		printf("Unknown device %u\n", bt_dev);
		break;
	}

	return 0;
}

enum boot_device get_boot_device(void)
{
	enum boot_device boot_dev = SD1_BOOT;

	sc_rsrc_t dev_rsrc;

	/* Note we only support android in EMMC SDHC0 */
	if (IS_ENABLED(CONFIG_XEN))
		return MMC1_BOOT;

	sc_misc_get_boot_dev(-1, &dev_rsrc);

	switch (dev_rsrc) {
	case SC_R_SDHC_0:
		boot_dev = MMC1_BOOT;
		break;
	case SC_R_SDHC_1:
		boot_dev = SD2_BOOT;
		break;
	case SC_R_SDHC_2:
		boot_dev = SD3_BOOT;
		break;
	case SC_R_NAND:
		boot_dev = NAND_BOOT;
		break;
	case SC_R_FSPI_0:
		boot_dev = FLEXSPI_BOOT;
		break;
	case SC_R_SATA_0:
		boot_dev = SATA_BOOT;
		break;
	case SC_R_USB_0:
	case SC_R_USB_1:
	case SC_R_USB_2:
		boot_dev = USB_BOOT;
		break;
	default:
		break;
	}

	return boot_dev;
}

bool is_usb_boot(void)
{
	return get_boot_device() == USB_BOOT;
}

#ifdef CONFIG_SERIAL_TAG
#define FUSE_UNIQUE_ID_WORD0 16
#define FUSE_UNIQUE_ID_WORD1 17
void get_board_serial(struct tag_serialnr *serialnr)
{
	sc_err_t err;
	uint32_t val1 = 0, val2 = 0;
	uint32_t word1, word2;

	word1 = FUSE_UNIQUE_ID_WORD0;
	word2 = FUSE_UNIQUE_ID_WORD1;

	err = sc_misc_otp_fuse_read(-1, word1, &val1);
	if (err != SC_ERR_NONE) {
		printf("%s fuse %d read error: %d\n", __func__,word1, err);
		return;
	}

	err = sc_misc_otp_fuse_read(-1, word2, &val2);
	if (err != SC_ERR_NONE) {
		printf("%s fuse %d read error: %d\n", __func__, word2, err);
		return;
	}
	serialnr->low = val1;
	serialnr->high = val2;
}
#endif /*CONFIG_SERIAL_TAG*/

__weak int board_mmc_get_env_dev(int devno)
{
	return devno;
}

int mmc_get_env_dev(void)
{
	sc_rsrc_t dev_rsrc;
	int devno;

	sc_misc_get_boot_dev(-1, &dev_rsrc);

	switch (dev_rsrc) {
	case SC_R_SDHC_0:
		devno = 0;
		break;
	case SC_R_SDHC_1:
		devno = 1;
		break;
	case SC_R_SDHC_2:
		devno = 2;
		break;
	default:
		/* If not boot from sd/mmc, use default value */
		return env_get_ulong("mmcdev", 10, CONFIG_SYS_MMC_ENV_DEV);
	}

	return board_mmc_get_env_dev(devno);
}

#define MEMSTART_ALIGNMENT  SZ_2M /* Align the memory start with 2MB */

static sc_faddr_t reserve_optee_shm(sc_faddr_t addr_start)
{
	/* OPTEE has a share memory at its top address,
	 * ATF assigns the share memory to non-secure os partition for share with kernel
	 * We should not add this share memory to DDR bank, as this memory is dedicated for
	 * optee, optee driver will memremap it and can't be used by system malloc.
	 */

	sc_faddr_t optee_start = rom_pointer[0];
	sc_faddr_t optee_size = rom_pointer[1];

	if (optee_size && optee_start <= addr_start &&
		addr_start < optee_start + optee_size) {
		debug("optee 0x%llx 0x%llx, addr_start 0x%llx\n",
			optee_start, optee_size, addr_start);
		return optee_start + optee_size;
	}

	return addr_start;
}

static int get_owned_memreg(sc_rm_mr_t mr, sc_faddr_t *addr_start,
			    sc_faddr_t *addr_end)
{
	sc_faddr_t start, end;
	int ret;
	bool owned;

	owned = sc_rm_is_memreg_owned(-1, mr);
	if (owned) {
		ret = sc_rm_get_memreg_info(-1, mr, &start, &end);
		if (ret) {
			printf("Memreg get info failed, %d\n", ret);
			return -EINVAL;
		}
		debug("0x%llx -- 0x%llx\n", start, end);
		*addr_start = reserve_optee_shm(start);
		*addr_end = end;

		return 0;
	}

	return -EINVAL;
}

phys_size_t get_effective_memsize(void)
{
	sc_rm_mr_t mr;
	sc_faddr_t start, end, end1, start_aligned;
	int err;

	if (IS_ENABLED(CONFIG_XEN))
		return PHYS_SDRAM_1_SIZE;

	end1 = (sc_faddr_t)PHYS_SDRAM_1 + PHYS_SDRAM_1_SIZE;
	for (mr = 0; mr < 64; mr++) {
		err = get_owned_memreg(mr, &start, &end);
		if (!err) {
			start_aligned = roundup(start, MEMSTART_ALIGNMENT);
			/* Too small memory region, not use it */
			if (start_aligned > end)
				continue;

			/* Find the memory region runs the U-Boot */
			if (start >= PHYS_SDRAM_1 && start <= end1 &&
			    (start <= CONFIG_SYS_TEXT_BASE &&
			    end >= CONFIG_SYS_TEXT_BASE)) {
				if ((end + 1) <= ((sc_faddr_t)PHYS_SDRAM_1 +
				    PHYS_SDRAM_1_SIZE))
					return (end - PHYS_SDRAM_1 + 1);
				else
					return PHYS_SDRAM_1_SIZE;
			}
		}
	}

	return PHYS_SDRAM_1_SIZE;
}

int dram_init(void)
{
	sc_rm_mr_t mr;
	sc_faddr_t start, end, end1, end2;
	int err;

	if (IS_ENABLED(CONFIG_XEN)) {
		gd->ram_size = PHYS_SDRAM_1_SIZE;
		gd->ram_size += PHYS_SDRAM_2_SIZE;

		return 0;
	}

	end1 = (sc_faddr_t)PHYS_SDRAM_1 + PHYS_SDRAM_1_SIZE;
	end2 = (sc_faddr_t)PHYS_SDRAM_2 + PHYS_SDRAM_2_SIZE;
	for (mr = 0; mr < 64; mr++) {
		err = get_owned_memreg(mr, &start, &end);
		if (!err) {
			start = roundup(start, MEMSTART_ALIGNMENT);
			/* Too small memory region, not use it */
			if (start > end)
				continue;

			if (start >= PHYS_SDRAM_1 && start <= end1) {
				if ((end + 1) <= end1)
					gd->ram_size += end - start + 1;
				else
					gd->ram_size += end1 - start;
			} else if (start >= PHYS_SDRAM_2 && start <= end2) {
				if ((end + 1) <= end2)
					gd->ram_size += end - start + 1;
				else
					gd->ram_size += end2 - start;
			}
		}
	}

	/* If error, set to the default value */
	if (!gd->ram_size) {
		gd->ram_size = PHYS_SDRAM_1_SIZE;
		gd->ram_size += PHYS_SDRAM_2_SIZE;
	}
	return 0;
}

static void dram_bank_sort(int current_bank)
{
	phys_addr_t start;
	phys_size_t size;

	while (current_bank > 0) {
		if (gd->bd->bi_dram[current_bank - 1].start >
		    gd->bd->bi_dram[current_bank].start) {
			start = gd->bd->bi_dram[current_bank - 1].start;
			size = gd->bd->bi_dram[current_bank - 1].size;

			gd->bd->bi_dram[current_bank - 1].start =
				gd->bd->bi_dram[current_bank].start;
			gd->bd->bi_dram[current_bank - 1].size =
				gd->bd->bi_dram[current_bank].size;

			gd->bd->bi_dram[current_bank].start = start;
			gd->bd->bi_dram[current_bank].size = size;
		}
		current_bank--;
	}
}

int dram_init_banksize(void)
{
	sc_rm_mr_t mr;
	sc_faddr_t start, end, end1, end2;
	int i = 0;
	int err;

	if (IS_ENABLED(CONFIG_XEN)) {
		gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
		gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;
		gd->bd->bi_dram[1].start = PHYS_SDRAM_2;
		gd->bd->bi_dram[1].size = PHYS_SDRAM_2_SIZE;

		return 0;
	}

	end1 = (sc_faddr_t)PHYS_SDRAM_1 + PHYS_SDRAM_1_SIZE;
	end2 = (sc_faddr_t)PHYS_SDRAM_2 + PHYS_SDRAM_2_SIZE;
	for (mr = 0; mr < 64 && i < CONFIG_NR_DRAM_BANKS; mr++) {
		err = get_owned_memreg(mr, &start, &end);
		if (!err) {
			start = roundup(start, MEMSTART_ALIGNMENT);
			if (start > end) /* Small memory region, no use it */
				continue;

			if (start >= PHYS_SDRAM_1 && start <= end1) {
				gd->bd->bi_dram[i].start = start;

				if ((end + 1) <= end1)
					gd->bd->bi_dram[i].size =
						end - start + 1;
				else
					gd->bd->bi_dram[i].size = end1 - start;

				dram_bank_sort(i);
				i++;
			} else if (start >= PHYS_SDRAM_2 && start <= end2) {
				gd->bd->bi_dram[i].start = start;

				if ((end + 1) <= end2)
					gd->bd->bi_dram[i].size =
						end - start + 1;
				else
					gd->bd->bi_dram[i].size = end2 - start;

				dram_bank_sort(i);
				i++;
			}
		}
	}

	/* If error, set to the default value */
	if (!i) {
		gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
		gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;
		gd->bd->bi_dram[1].start = PHYS_SDRAM_2;
		gd->bd->bi_dram[1].size = PHYS_SDRAM_2_SIZE;
	}

	return 0;
}

static u64 get_block_attrs(sc_faddr_t addr_start)
{
	u64 attr = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) | PTE_BLOCK_NON_SHARE |
		PTE_BLOCK_PXN | PTE_BLOCK_UXN;

	if ((addr_start >= PHYS_SDRAM_1 &&
	     addr_start <= ((sc_faddr_t)PHYS_SDRAM_1 + PHYS_SDRAM_1_SIZE)) ||
	    (addr_start >= PHYS_SDRAM_2 &&
	     addr_start <= ((sc_faddr_t)PHYS_SDRAM_2 + PHYS_SDRAM_2_SIZE)))
#ifdef CONFIG_IMX_TRUSTY_OS
		return (PTE_BLOCK_MEMTYPE(MT_NORMAL) | PTE_BLOCK_INNER_SHARE);
#else
		return (PTE_BLOCK_MEMTYPE(MT_NORMAL) | PTE_BLOCK_OUTER_SHARE);
#endif

	return attr;
}

static u64 get_block_size(sc_faddr_t addr_start, sc_faddr_t addr_end)
{
	sc_faddr_t end1, end2;

	end1 = (sc_faddr_t)PHYS_SDRAM_1 + PHYS_SDRAM_1_SIZE;
	end2 = (sc_faddr_t)PHYS_SDRAM_2 + PHYS_SDRAM_2_SIZE;

	if (addr_start >= PHYS_SDRAM_1 && addr_start <= end1) {
		if ((addr_end + 1) > end1)
			return end1 - addr_start;
	} else if (addr_start >= PHYS_SDRAM_2 && addr_start <= end2) {
		if ((addr_end + 1) > end2)
			return end2 - addr_start;
	}

	return (addr_end - addr_start + 1);
}

#define MAX_PTE_ENTRIES 512
#define MAX_MEM_MAP_REGIONS 16

static struct mm_region imx8_mem_map[MAX_MEM_MAP_REGIONS];
struct mm_region *mem_map = imx8_mem_map;

void enable_caches(void)
{
	sc_rm_mr_t mr;
	sc_faddr_t start, end;
	int err, i;

	if (IS_ENABLED(CONFIG_XEN)) {
		imx8_mem_map[0].virt = 0x00000000UL;
		imx8_mem_map[0].phys = 0x00000000UL;
		imx8_mem_map[0].size = 0x39000000UL;
		imx8_mem_map[0].attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
				 PTE_BLOCK_NON_SHARE | PTE_BLOCK_PXN | PTE_BLOCK_UXN;
		imx8_mem_map[1].virt = 0x39000000UL;
		imx8_mem_map[1].phys = 0x39000000UL;
		imx8_mem_map[1].size = 0x01000000UL;
		imx8_mem_map[1].attrs = (PTE_BLOCK_MEMTYPE(MT_NORMAL) | PTE_BLOCK_INNER_SHARE);

		imx8_mem_map[2].virt = 0x40000000UL;
		imx8_mem_map[2].phys = 0x40000000UL;
		imx8_mem_map[2].size = 0x40000000UL;
		imx8_mem_map[2].attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
				 PTE_BLOCK_NON_SHARE | PTE_BLOCK_PXN | PTE_BLOCK_UXN;

		imx8_mem_map[3].virt = 0x80000000UL;
		imx8_mem_map[3].phys = 0x80000000UL;
		imx8_mem_map[3].size = 0x80000000UL;
		imx8_mem_map[3].attrs = (PTE_BLOCK_MEMTYPE(MT_NORMAL) | PTE_BLOCK_INNER_SHARE);

		imx8_mem_map[4].virt = 0x100000000UL;
		imx8_mem_map[4].phys = 0x100000000UL;
		imx8_mem_map[4].size = 0x100000000UL;
		imx8_mem_map[4].attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
				 PTE_BLOCK_NON_SHARE | PTE_BLOCK_PXN | PTE_BLOCK_UXN;

		icache_enable();
		dcache_enable();

		return;
	}

	/* Create map for registers access from 0x1c000000 to 0x80000000*/
	imx8_mem_map[0].virt = 0x1c000000UL;
	imx8_mem_map[0].phys = 0x1c000000UL;
	imx8_mem_map[0].size = 0x64000000UL;
	imx8_mem_map[0].attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE | PTE_BLOCK_PXN | PTE_BLOCK_UXN;

	i = 1;

#ifdef CONFIG_IMX_VSERVICE_SHARED_BUFFER
	imx8_mem_map[i].virt = CONFIG_IMX_VSERVICE_SHARED_BUFFER;
	imx8_mem_map[i].phys = CONFIG_IMX_VSERVICE_SHARED_BUFFER;
	imx8_mem_map[i].size = CONFIG_IMX_VSERVICE_SHARED_BUFFER_SIZE;
	imx8_mem_map[i].attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE | PTE_BLOCK_PXN | PTE_BLOCK_UXN;
	i++;
#endif

	for (mr = 0; mr < 64 && i < MAX_MEM_MAP_REGIONS; mr++) {
		err = get_owned_memreg(mr, &start, &end);
		if (!err) {
			imx8_mem_map[i].virt = start;
			imx8_mem_map[i].phys = start;
			imx8_mem_map[i].size = get_block_size(start, end);
			imx8_mem_map[i].attrs = get_block_attrs(start);
			i++;
		}
	}

	if (i < MAX_MEM_MAP_REGIONS) {
		imx8_mem_map[i].size = 0;
		imx8_mem_map[i].attrs = 0;
	} else {
		puts("Error, need more MEM MAP REGIONS reserved\n");
		icache_enable();
		return;
	}

	for (i = 0; i < MAX_MEM_MAP_REGIONS; i++) {
		debug("[%d] vir = 0x%llx phys = 0x%llx size = 0x%llx attrs = 0x%llx\n",
		      i, imx8_mem_map[i].virt, imx8_mem_map[i].phys,
		      imx8_mem_map[i].size, imx8_mem_map[i].attrs);
	}

	icache_enable();
	dcache_enable();
}

#if !CONFIG_IS_ENABLED(SYS_DCACHE_OFF)
u64 get_page_table_size(void)
{
	u64 one_pt = MAX_PTE_ENTRIES * sizeof(u64);
	u64 size = 0;

	/*
	 * For each memory region, the max table size:
	 * 2 level 3 tables + 2 level 2 tables + 1 level 1 table
	 */
	size = (2 + 2 + 1) * one_pt * MAX_MEM_MAP_REGIONS + one_pt;

	/*
	 * We need to duplicate our page table once to have an emergency pt to
	 * resort to when splitting page tables later on
	 */
	size *= 2;

	/*
	 * We may need to split page tables later on if dcache settings change,
	 * so reserve up to 4 (random pick) page tables for that.
	 */
	size += one_pt * 4;

	return size;
}
#endif

#if defined(CONFIG_IMX8QM)
#define FUSE_MAC0_WORD0 452
#define FUSE_MAC0_WORD1 453
#define FUSE_MAC1_WORD0 454
#define FUSE_MAC1_WORD1 455
#elif defined(CONFIG_IMX8QXP) || defined (CONFIG_IMX8DXL)
#define FUSE_MAC0_WORD0 708
#define FUSE_MAC0_WORD1 709
#define FUSE_MAC1_WORD0 710
#define FUSE_MAC1_WORD1 711
#endif

void imx_get_mac_from_fuse(int dev_id, unsigned char *mac)
{
	u32 word[2], val[2] = {};
	int i, ret;

	if (dev_id == 0) {
		word[0] = FUSE_MAC0_WORD0;
		word[1] = FUSE_MAC0_WORD1;
	} else {
		word[0] = FUSE_MAC1_WORD0;
		word[1] = FUSE_MAC1_WORD1;
	}

	for (i = 0; i < 2; i++) {
		ret = sc_misc_otp_fuse_read(-1, word[i], &val[i]);
		if (ret < 0)
			goto err;
	}

	mac[0] = val[0];
	mac[1] = val[0] >> 8;
	mac[2] = val[0] >> 16;
	mac[3] = val[0] >> 24;
	mac[4] = val[1];
	mac[5] = val[1] >> 8;

	debug("%s: MAC%d: %02x.%02x.%02x.%02x.%02x.%02x\n",
	      __func__, dev_id, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	return;
err:
	printf("%s: fuse %d, err: %d\n", __func__, word[i], ret);
}

u32 get_cpu_rev(void)
{
	u32 id = 0, rev = 0;
	int ret;

	ret = sc_misc_get_control(-1, SC_R_SYSTEM, SC_C_ID, &id);
	if (ret)
		return 0;

	rev = (id >> 5)  & 0xf;
	id = (id & 0x1f) + MXC_SOC_IMX8;  /* Dummy ID for chip */

	/* 8DXL uses A1/A2, so generate dummy rev to differentiate with B/C */
	if (id == MXC_CPU_IMX8DXL && rev != 0)
		rev = 0x10 + rev;

	return (id << 12) | rev;
}

static bool check_device_power_off(struct udevice *dev,
	const char* permanent_on_devices[], int size)
{
	int i;

	for (i = 0; i < size; i++) {
		if (!strcmp(dev->name, permanent_on_devices[i]))
			return false;
	}

	return true;
}

void power_off_pd_devices(const char* permanent_on_devices[], int size)
{
	struct udevice *dev;
	struct power_domain pd;

	for (uclass_find_first_device(UCLASS_POWER_DOMAIN, &dev); dev;
		uclass_find_next_device(&dev)) {

		if (device_active(dev)) {
			/* Power off active pd devices except the permanent power on devices */
			if (check_device_power_off(dev, permanent_on_devices, size)) {
				pd.dev = dev;
				power_domain_off(&pd);
			}
		}
	}
}

void disconnect_from_pc(void)
{
	int ret;
	struct power_domain pd;

	if (!power_domain_lookup_name("conn_usb0", &pd)) {
		ret = power_domain_on(&pd);
		if (ret) {
			printf("conn_usb0 Power up failed! (error = %d)\n", ret);
			return;
		}

		writel(0x0, USB_BASE_ADDR + 0x140);

		ret = power_domain_off(&pd);
		if (ret) {
			printf("conn_usb0 Power off failed! (error = %d)\n", ret);
			return;
		}
	} else {
		printf("conn_usb0 finding failed!\n");
		return;
	}
}

bool check_owned_udevice(struct udevice *dev)
{
	int ret;
	sc_rsrc_t resource_id;
	struct ofnode_phandle_args args;

	/* Get the resource id from its power-domain */
	ret = dev_read_phandle_with_args(dev, "power-domains",
					 "#power-domain-cells", 0, 0, &args);
	if (ret) {
		printf("no power-domains found\n");
		return false;
	}

	/* Get the owner partition for resource*/
	resource_id = (sc_rsrc_t)ofnode_read_u32_default(args.node, "reg", SC_R_NONE);
	if (resource_id == SC_R_NONE) {
		printf("Can't find the resource id for udev %s\n", dev->name);
		return false;
	}

	debug("udev %s, resource id %d\n", dev->name, resource_id);

	return sc_rm_is_resource_owned(-1, resource_id);
}

bool check_m4_parts_boot(void)
{
	sc_rm_pt_t m4_parts[2];
	int err;

	err = sc_rm_get_resource_owner(-1, SC_R_M4_0_PID0, &m4_parts[0]);
	if (err != SC_ERR_NONE) {
		printf("%s get resource [%d] owner error: %d\n", __func__, SC_R_M4_0_PID0, err);
		return false;
	}

	if (sc_pm_is_partition_started(-1, m4_parts[0]))
		return true;

	if (is_imx8qm()) {
		err = sc_rm_get_resource_owner(-1, SC_R_M4_1_PID0, &m4_parts[1]);
		if (err != SC_ERR_NONE) {
			printf("%s get resource [%d] owner error: %d\n", __func__, SC_R_M4_1_PID0, err);
			return false;
		}

		if (sc_pm_is_partition_started(-1, m4_parts[1]))
			return true;
	}

	return false;
}

#ifdef CONFIG_IMX_VSERVICE
struct udevice * board_imx_vservice_find_mu(struct udevice *dev)
{
	int ret;
	const char *m4_mu_name[2] = {
		"mu@5d230000",
		"mu@5d240000"
	};
	struct udevice *m4_mu[2];
	sc_rm_pt_t m4_parts[2];
	int err;
	struct ofnode_phandle_args args;
	sc_rsrc_t resource_id;
	sc_rm_pt_t resource_part;

	/* Get the resource id from its power-domain */
	ret = dev_read_phandle_with_args(dev, "power-domains",
					 "#power-domain-cells", 0, 0, &args);
	if (ret) {
		printf("Can't find the power-domains property for udev %s\n", dev->name);
		return NULL;
	}

	/* Get the owner partition for resource*/
	resource_id = (sc_rsrc_t)ofnode_read_u32_default(args.node, "reg", SC_R_NONE);
	if (resource_id == SC_R_NONE) {
		printf("Can't find the resource id for udev %s\n", dev->name);
		return NULL;
	}

	err = sc_rm_get_resource_owner(-1, resource_id, &resource_part);
	if (err != SC_ERR_NONE) {
		printf("%s get resource [%d] owner error: %d\n", __func__, resource_id, err);
		return NULL;
	}

	debug("udev %s, resource id %d, resource part %d\n", dev->name, resource_id, resource_part);

	/* MU8 for communication between M4_0 and u-boot, MU9 for M4_1 and u-boot */
	err = sc_rm_get_resource_owner(-1, SC_R_M4_0_PID0, &m4_parts[0]);
	if (err != SC_ERR_NONE) {
		printf("%s get resource [%d] owner error: %d\n", __func__, SC_R_M4_0_PID0, err);
		return NULL;
	}

	ret = uclass_find_device_by_name(UCLASS_MISC,  m4_mu_name[0], &m4_mu[0]);
	if (!ret) {
		/* If the i2c is in m4_0 partition, return the mu8 */
		if (resource_part == m4_parts[0])
			return m4_mu[0];
	}

	if (is_imx8qm()) {
		err = sc_rm_get_resource_owner(-1, SC_R_M4_1_PID0, &m4_parts[1]);
		if (err != SC_ERR_NONE) {
			printf("%s get resource [%d] owner error: %d\n", __func__, SC_R_M4_1_PID0, err);
			return NULL;
		}

		ret = uclass_find_device_by_name(UCLASS_MISC,  m4_mu_name[1], &m4_mu[1]);
		if (!ret) {
			/* If the i2c is in m4_1 partition, return the mu9 */
			if (resource_part == m4_parts[1])
				return m4_mu[1];
		}
	}

	return NULL;
}

void * board_imx_vservice_get_buffer(struct imx_vservice_channel *node, u32 size)
{
	const char *m4_mu_name[2] = {
		"mu@5d230000",
		"mu@5d240000"
	};

	/* Each MU ownes 1M buffer */
	if (size <= 0x100000) {
		if (!strcmp(node->mu_dev->name, m4_mu_name[0]))
			return (void * )CONFIG_IMX_VSERVICE_SHARED_BUFFER;
		else if (!strcmp(node->mu_dev->name, m4_mu_name[1]))
			return (void * )(CONFIG_IMX_VSERVICE_SHARED_BUFFER + 0x100000);
		else
			return NULL;
	}

	return NULL;
}
#endif

/* imx8qxp i2c1 has lots of devices may used by both M4 and A core
*   If A core partition does not own the resource, we will start
*   virtual i2c driver. Otherwise use local i2c driver.
*/
int board_imx_virt_i2c_bind(struct udevice *dev)
{
	if (check_owned_udevice(dev))
		return -ENODEV;

	return 0;
}

int board_imx_lpi2c_bind(struct udevice *dev)
{
	if (check_owned_udevice(dev))
		return 0;

	return -ENODEV;
}

void board_boot_order(u32 *spl_boot_list)
{
	spl_boot_list[0] = spl_boot_device();

	if (spl_boot_list[0] == BOOT_DEVICE_SPI) {
		/* Check whether we own the flexspi0, if not, use NOR boot */
		if (!sc_rm_is_resource_owned(-1, SC_R_FSPI_0))
			spl_boot_list[0] = BOOT_DEVICE_NOR;
	}
}

#ifdef CONFIG_USB_PORT_AUTO
static int usb_port_auto_check(void)
{
	int ret;
	u32 usb2_data;
	struct power_domain pd;
	struct power_domain phy_pd;

	if (!power_domain_lookup_name("conn_usb0", &pd)) {
		ret = power_domain_on(&pd);
		if (ret) {
			printf("conn_usb0 Power up failed!\n");
			return ret;
		}

		if (!power_domain_lookup_name("conn_usb0_phy", &phy_pd)) {
			ret = power_domain_on(&phy_pd);
			if (ret) {
				printf("conn_usb0_phy Power up failed!\n");
				return ret;
			}
		} else {
			return -1;
		}

		enable_usboh3_clk(1);
		usb2_data = ci_udc_check_bus_active(USB_BASE_ADDR, USB_PHY0_BASE_ADDR, 0);

		ret = power_domain_off(&phy_pd);
		if (ret) {
			printf("conn_usb0_phy Power off failed!\n");
			return ret;
		}
		ret = power_domain_off(&pd);
		if (ret) {
			printf("conn_usb0 Power off failed!\n");
			return ret;
		}

		if (!usb2_data)
			return 1;
		else
			return 0;
	}
	return -1;
}

int board_usb_gadget_port_auto(void)
{
    int usb_boot_index;
	usb_boot_index = usb_port_auto_check();

	if (usb_boot_index < 0)
		usb_boot_index = 0;

	printf("auto usb %d\n", usb_boot_index);

	return usb_boot_index;
}
#endif

static void power_off_all_usb(void)
{
	if (is_usb_boot()) {
		/* Turn off all usb resource to let conn SS power down */
		sc_pm_set_resource_power_mode(-1, SC_R_USB_0_PHY, SC_PM_PW_MODE_OFF);
		sc_pm_set_resource_power_mode(-1, SC_R_USB_1_PHY, SC_PM_PW_MODE_OFF);
		sc_pm_set_resource_power_mode(-1, SC_R_USB_2_PHY, SC_PM_PW_MODE_OFF);

		sc_pm_set_resource_power_mode(-1, SC_R_USB_0, SC_PM_PW_MODE_OFF);
		sc_pm_set_resource_power_mode(-1, SC_R_USB_1, SC_PM_PW_MODE_OFF);
		sc_pm_set_resource_power_mode(-1, SC_R_USB_2, SC_PM_PW_MODE_OFF);
	}
}
