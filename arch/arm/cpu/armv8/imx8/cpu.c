/*
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
 */

#include <common.h>
#include <errno.h>
#include <asm/io.h>
#include <asm/imx-common/sci/sci.h>
#include <asm/arch/i2c.h>
#include <asm/arch/clock.h>
#include <asm/armv8/mmu.h>
#include <elf.h>
#include <asm/arch/sid.h>
#include <asm/arch-imx/cpu.h>

DECLARE_GLOBAL_DATA_PTR;

static struct mm_region imx8_mem_map[] = {
	{
		.virt = 0x0UL,
		.phys = 0x0UL,
		.size = 0x2000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
			 PTE_BLOCK_OUTER_SHARE
	}, {
		.virt = 0x2000000UL,
		.phys = 0x2000000UL,
		.size = 0x7E000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN
	}, {
		.virt = 0x80000000UL,
		.phys = 0x80000000UL,
		.size = 0x80000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
			 PTE_BLOCK_OUTER_SHARE
	}, {
		.virt = 0x100000000UL,
		.phys = 0x100000000UL,
		.size = 0x700000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN
	}, {
		.virt = 0x880000000UL,
		.phys = 0x880000000UL,
		.size = 0x780000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
			 PTE_BLOCK_OUTER_SHARE
	}, {
		/* List terminator */
		0,
	}
};
struct mm_region *mem_map = imx8_mem_map;

u32 get_cpu_rev(void)
{
	sc_ipc_t ipcHndl;
	uint32_t id = 0, rev = 0;
	sc_err_t err;

	ipcHndl = gd->arch.ipc_channel_handle;

	err = sc_misc_get_control(ipcHndl, SC_R_SC_PID0, SC_C_ID, &id);
	if (err != SC_ERR_NONE)
		return 0;

	rev = (id >> 5)  & 0xf;
	id = (id & 0x1f) + MXC_SOC_IMX8;  /* Dummy ID for chip */

	return (id << 12) | rev;
}

#ifdef CONFIG_DISPLAY_CPUINFO
const char *get_imx8_type(u32 imxtype)
{
	switch (imxtype) {
	case MXC_CPU_IMX8QM:
		return "8QM";	/* i.MX8 Quad MAX */
	case MXC_CPU_IMX8QXP:
		return "8QXP";	/* i.MX8 Quad XP */
	case MXC_CPU_IMX8DX:
		return "8DX";	/* i.MX8 Dual X */
	default:
		return "??";
	}
}

const char *get_imx8_rev(u32 rev)
{
	switch (rev) {
	case CHIP_REV_A:
		return "A";
	case CHIP_REV_B:
		return "B";
	default:
		return "?";
	}
}

int print_cpuinfo(void)
{
	u32 cpurev;
	cpurev = get_cpu_rev();

	printf("CPU:   Freescale i.MX%s rev%s at %d MHz\n",
			get_imx8_type((cpurev & 0xFF000) >> 12),
			get_imx8_rev((cpurev & 0xFFF)),
		mxc_get_clock(MXC_ARM_CLK) / 1000000);

	return 0;
}
#endif

int arch_cpu_init(void)
{
	sc_ipc_t ipcHndl = 0;
	sc_err_t sciErr = 0;
	gd->arch.ipc_channel_handle = 0;

	/* Open IPC channel */
	sciErr = sc_ipc_open(&ipcHndl, SC_IPC_CH);
	if (sciErr != SC_ERR_NONE)
		return -EPERM;

	gd->arch.ipc_channel_handle = ipcHndl;

#ifdef CONFIG_IMX_SMMU
	sciErr = sc_pm_set_resource_power_mode(ipcHndl, SC_R_SMMU,
				SC_PM_PW_MODE_ON);
	if (sciErr != SC_ERR_NONE)
		return 0;
#endif

	return 0;
}

u32 cpu_mask(void)
{
#ifdef CONFIG_IMX8QM
	return 0x3f;
#else
	return 0xf;	/*For IMX8QXP*/
#endif
}

#define CCI400_DVM_MESSAGE_REQ_EN	0x00000002
#define CCI400_SNOOP_REQ_EN		0x00000001
#define CHANGE_PENDING_BIT		(1 << 0)
int imx8qm_wake_seconday_cores(void)
{
#ifdef CONFIG_ARMV8_MULTIENTRY
	sc_ipc_t ipcHndl;
	u64 *table = get_spin_tbl_addr();

	/* Clear spin table so that secondary processors
	 * observe the correct value after waking up from wfe.
	 */
	memset(table, 0, CONFIG_MAX_CPUS*SPIN_TABLE_ELEM_SIZE);
	flush_dcache_range((unsigned long)table,
			   (unsigned long)table +
			   (CONFIG_MAX_CPUS*SPIN_TABLE_ELEM_SIZE));

	/* Open IPC channel */
	if (sc_ipc_open(&ipcHndl, SC_IPC_CH)  != SC_ERR_NONE)
		return -EIO;

	__raw_writel(0xc, 0x52090000);
	__raw_writel(1, 0x52090008);

	/* IPC to pwr up and boot other cores */
	if (sc_pm_set_resource_power_mode(ipcHndl, SC_R_A53_1, SC_PM_PW_MODE_ON) != SC_ERR_NONE)
		return -EIO;
	if (sc_pm_cpu_start(ipcHndl, SC_R_A53_1, true, 0x80000000) != SC_ERR_NONE)
		return -EIO;

	if (sc_pm_set_resource_power_mode(ipcHndl, SC_R_A53_2, SC_PM_PW_MODE_ON) != SC_ERR_NONE)
		return -EIO;
	if (sc_pm_cpu_start(ipcHndl, SC_R_A53_2, true, 0x80000000) != SC_ERR_NONE)
		return -EIO;

	if (sc_pm_set_resource_power_mode(ipcHndl, SC_R_A53_3, SC_PM_PW_MODE_ON) != SC_ERR_NONE)
		return -EIO;
	if (sc_pm_cpu_start(ipcHndl, SC_R_A53_3, true, 0x80000000) != SC_ERR_NONE)
		return -EIO;

	/* Enable snoop and dvm msg requests for a53 port on CCI slave interface 3 */
	__raw_writel(CCI400_DVM_MESSAGE_REQ_EN | CCI400_SNOOP_REQ_EN, 0x52094000);

	while (__raw_readl(0x5209000c) & CHANGE_PENDING_BIT)
		;

	/* Pwr up cluster 1 and boot core 0*/
	if (sc_pm_set_resource_power_mode(ipcHndl, SC_R_A72, SC_PM_PW_MODE_ON) != SC_ERR_NONE)
		return -EIO;

	if (sc_pm_set_resource_power_mode(ipcHndl, SC_R_A72_0, SC_PM_PW_MODE_ON) != SC_ERR_NONE)
		return -EIO;
	if (sc_pm_cpu_start(ipcHndl, SC_R_A72_0, true, 0x80000000) != SC_ERR_NONE)
		return -EIO;

	/* IPC to pwr up and boot core 1 */
	if (sc_pm_set_resource_power_mode(ipcHndl, SC_R_A72_1, SC_PM_PW_MODE_ON) != SC_ERR_NONE)
		return -EIO;
	if (sc_pm_cpu_start(ipcHndl, SC_R_A72_1, true, 0x80000000) != SC_ERR_NONE)
		return -EIO;

	/* Enable snoop and dvm msg requests for a72 port on CCI slave interface 4 */
	__raw_writel(CCI400_DVM_MESSAGE_REQ_EN | CCI400_SNOOP_REQ_EN, 0x52095000);

	while (__raw_readl(0x5209000c) & CHANGE_PENDING_BIT)
		;
#endif
	return 0;
}

int imx8qxp_wake_secondary_cores(void)
{
#ifdef CONFIG_ARMV8_MULTIENTRY
	sc_ipc_t ipcHndl;
	u64 *table = get_spin_tbl_addr();

	/* Clear spin table so that secondary processors
	 * observe the correct value after waking up from wfe.
	 */
	memset(table, 0, CONFIG_MAX_CPUS*SPIN_TABLE_ELEM_SIZE);
	flush_dcache_range((unsigned long)table,
			   (unsigned long)table +
			   (CONFIG_MAX_CPUS*SPIN_TABLE_ELEM_SIZE));

	/* Open IPC channel */
	if (sc_ipc_open(&ipcHndl, SC_IPC_CH)  != SC_ERR_NONE)
		return -EIO;

	/* IPC to pwr up and boot other cores */
	if (sc_pm_set_resource_power_mode(ipcHndl, SC_R_A35_1, SC_PM_PW_MODE_ON) != SC_ERR_NONE)
		return -EIO;
	if (sc_pm_cpu_start(ipcHndl, SC_R_A35_1, true, 0x80000000) != SC_ERR_NONE)
		return -EIO;

	if (sc_pm_set_resource_power_mode(ipcHndl, SC_R_A35_2, SC_PM_PW_MODE_ON) != SC_ERR_NONE)
		return -EIO;
	if (sc_pm_cpu_start(ipcHndl, SC_R_A35_2, true, 0x80000000) != SC_ERR_NONE)
		return -EIO;

	if (sc_pm_set_resource_power_mode(ipcHndl, SC_R_A35_3, SC_PM_PW_MODE_ON) != SC_ERR_NONE)
		return -EIO;
	if (sc_pm_cpu_start(ipcHndl, SC_R_A35_3, true, 0x80000000) != SC_ERR_NONE)
		return -EIO;

#endif
	return 0;
}

int init_i2c_power(unsigned i2c_num)
{
	sc_ipc_t ipc;
	sc_err_t err;
	u32 i;

	if (i2c_num >= ARRAY_SIZE(imx_i2c_desc))
		return -EINVAL;

	ipc = gd->arch.ipc_channel_handle;

	for (i = 0; i < ARRAY_SIZE(i2c_parent_power_desc); i++) {
		if (i2c_parent_power_desc[i].index == i2c_num) {
			err = sc_pm_set_resource_power_mode(ipc,
				i2c_parent_power_desc[i].rsrc, SC_PM_PW_MODE_ON);
			if (err != SC_ERR_NONE)
				return -EPERM;
		}
	}

	/* power up i2c resource */
	err = sc_pm_set_resource_power_mode(ipc,
			imx_i2c_desc[i2c_num].rsrc, SC_PM_PW_MODE_ON);
	if (err != SC_ERR_NONE)
		return -EPERM;

	return 0;
}

#define FUSE_MAC0_WORD0 452
#define FUSE_MAC0_WORD1 453
#define FUSE_MAC1_WORD0 454
#define FUSE_MAC1_WORD1 455 
void imx_get_mac_from_fuse(int dev_id, unsigned char *mac)
{
	sc_err_t err;
	sc_ipc_t ipc;
	uint32_t val1 = 0, val2 = 0;
	uint32_t word1, word2;

	ipc = gd->arch.ipc_channel_handle;

	if (dev_id == 0) {
		word1 = FUSE_MAC0_WORD0;
		word2 = FUSE_MAC0_WORD1;
	} else {
		word1 = FUSE_MAC1_WORD0;
		word2 = FUSE_MAC1_WORD1;
	}

	err = sc_misc_otp_fuse_read(ipc, word1, &val1);
	if (err != SC_ERR_NONE) {
		printf("%s fuse %d read error: %d\n", __func__, word1, err);
		return;
	}

	err = sc_misc_otp_fuse_read(ipc, word2, &val2);
	if (err != SC_ERR_NONE) {
		printf("%s fuse %d read error: %d\n", __func__, word2, err);
		return;
	}

	mac[0] = val1;
	mac[1] = val1 >> 8;
	mac[2] = val1 >> 16;
	mac[3] = val1 >> 24;
	mac[4] = val2;
	mac[5] = val2 >> 8;
}

#ifdef CONFIG_IMX_BOOTAUX

#ifdef CONFIG_IMX8QM
int arch_auxiliary_core_up(u32 core_id, ulong boot_private_data)
{
	sc_ipc_t ipcHndl;
	sc_rsrc_t core_rsrc, mu_rsrc;
	sc_faddr_t tcml_addr;
	u32 tcml_size = SZ_128K;
	ulong addr;

	ipcHndl = gd->arch.ipc_channel_handle;

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

	if (addr >= tcml_addr && addr <= tcml_addr + tcml_size) {
		printf("Wrong image address 0x%lx, should not in TCML\n",
			addr);
		return -EINVAL;
	}

	printf("Power on M4 and MU\n");

	if (sc_pm_set_resource_power_mode(ipcHndl, core_rsrc, SC_PM_PW_MODE_ON) != SC_ERR_NONE)
		return -EIO;

	if (sc_pm_set_resource_power_mode(ipcHndl, mu_rsrc, SC_PM_PW_MODE_ON) != SC_ERR_NONE)
		return -EIO;

	printf("Copy M4 image from 0x%lx to TCML 0x%lx\n", addr, (ulong)tcml_addr);

	if (addr != tcml_addr)
		memcpy((void *)tcml_addr, (void *)addr, tcml_size);

	printf("Start M4 %u\n", core_id);
	if (sc_pm_cpu_start(ipcHndl, core_rsrc, true, tcml_addr) != SC_ERR_NONE)
		return -EIO;

	printf("bootaux complete\n");
	return 0;
}
#endif

#ifdef CONFIG_IMX8QXP
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
	sc_ipc_t ipcHndl;
	sc_rsrc_t core_rsrc, mu_rsrc = -1;
	sc_faddr_t aux_core_ram;
	u32 size;
	ulong addr;

	ipcHndl = gd->arch.ipc_channel_handle;

	switch (core_id) {
	case 0:
		core_rsrc = SC_R_M4_0_PID0;
		aux_core_ram = 0x34FE0000;
		mu_rsrc = SC_R_M4_0_MU_1A;
		size = SZ_128K;
		break;
	case 1:
		core_rsrc = SC_R_HIFI;
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

	if (sc_pm_set_resource_power_mode(ipcHndl, core_rsrc, SC_PM_PW_MODE_ON) != SC_ERR_NONE)
		return -EIO;

	if (mu_rsrc != -1) {
		if (sc_pm_set_resource_power_mode(ipcHndl, mu_rsrc, SC_PM_PW_MODE_ON) != SC_ERR_NONE)
			return -EIO;
	}

	if (core_id == 1) {
		if (sc_pm_clock_enable(ipcHndl, core_rsrc, SC_PM_CLK_PER, true, false) != SC_ERR_NONE) {
			printf("Error enable clock\n");
			return -EIO;
		}
		if (sc_pm_set_resource_power_mode(ipcHndl, SC_R_SAI_0, SC_PM_PW_MODE_ON) != SC_ERR_NONE) {
			printf("Error power on SAI0\n");
			return -EIO;
		}
		if (sc_pm_set_resource_power_mode(ipcHndl, SC_R_HIFI_RAM, SC_PM_PW_MODE_ON) != SC_ERR_NONE) {
			printf("Error power on HIFI RAM\n");
			return -EIO;
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

	if (sc_pm_cpu_start(ipcHndl, core_rsrc, true, aux_core_ram) != SC_ERR_NONE)
		return -EIO;

	printf("bootaux complete\n");
	return 0;
}
#endif

int arch_auxiliary_core_check_up(u32 core_id)
{
	sc_rsrc_t core_rsrc;
	sc_pm_power_mode_t power_mode;
	sc_ipc_t ipcHndl;

	ipcHndl = gd->arch.ipc_channel_handle;

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

	if (sc_pm_get_resource_power_mode(ipcHndl, core_rsrc, &power_mode) != SC_ERR_NONE)
		return 0;

	if (power_mode != SC_PM_PW_MODE_OFF)
		return 1;

	return 0;
}
#endif

#ifdef CONFIG_IMX_SMMU
struct smmu_sid dev_sids[] = {
	{ SC_R_SDHC_0, 0x11, "SDHC0" },
	{ SC_R_SDHC_1, 0x11, "SDHC1" },
	{ SC_R_SDHC_2, 0x11, "SDHC2" },
	{ SC_R_ENET_0, 0x12, "FEC0" },
	{ SC_R_ENET_1, 0x12, "FEC1" },
};

sc_err_t imx8_config_smmu_sid(struct smmu_sid *dev_sids, int size)
{
	int i;
	sc_err_t sciErr = SC_ERR_NONE;

	if ((dev_sids == NULL) || (size <= 0))
		return SC_ERR_NONE;

	for (i = 0; i < size; i++) {
		sciErr = sc_rm_set_master_sid(gd->arch.ipc_channel_handle,
					      dev_sids[i].rsrc,
					      dev_sids[i].sid);
		if (sciErr != SC_ERR_NONE) {
			printf("set master sid error\n");
			return sciErr;
		}
	}

	return SC_ERR_NONE;
}

void arch_preboot_os(void)
{
	imx8_config_smmu_sid(dev_sids, ARRAY_SIZE(dev_sids));
}
#endif
