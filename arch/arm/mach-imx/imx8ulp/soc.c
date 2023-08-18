// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2021-2022 NXP
 */

#include <asm/io.h>
#include <asm/arch/clock.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/sys_proto.h>
#include <asm/armv8/mmu.h>
#include <asm/mach-imx/boot_mode.h>
#include <asm/global_data.h>
#include <efi_loader.h>
#include <event.h>
#include <spl.h>
#include <asm/arch/rdc.h>
#include <asm/mach-imx/ele_api.h>
#include <asm/mach-imx/mu_hal.h>
#include <cpu_func.h>
#include <asm/setup.h>
#include <dm.h>
#include <dm/device-internal.h>
#include <dm/lists.h>
#include <dm/uclass.h>
#include <dm/device.h>
#include <dm/uclass-internal.h>
#include <fuse.h>
#include <thermal.h>
#include <linux/iopoll.h>
#include <env.h>
#include <env_internal.h>
#include <asm/mach-imx/optee.h>
#include <kaslr.h>

DECLARE_GLOBAL_DATA_PTR;

struct rom_api *g_rom_api = (struct rom_api *)0x1980;

bool is_usb_boot(void)
{
	enum boot_device bt_dev = get_boot_device();
	return (bt_dev == USB_BOOT || bt_dev == USB2_BOOT);
}

void disconnect_from_pc(void)
{
	enum boot_device bt_dev = get_boot_device();

	if (bt_dev == USB_BOOT)
		writel(0x0, USBOTG0_RBASE + 0x140);
	else if (bt_dev == USB2_BOOT)
		writel(0x0, USBOTG1_RBASE + 0x140);

	return;
}

#ifdef CONFIG_ENV_IS_IN_MMC
__weak int board_mmc_get_env_dev(int devno)
{
	return devno;
}

int mmc_get_env_dev(void)
{
	int ret;
	u32 boot;
	u16 boot_type;
	u8 boot_instance;

	ret = rom_api_query_boot_infor(QUERY_BT_DEV, &boot);

	if (ret != ROM_API_OKAY) {
		puts("ROMAPI: failure at query_boot_info\n");
		return CONFIG_SYS_MMC_ENV_DEV;
	}

	boot_type = boot >> 16;
	boot_instance = (boot >> 8) & 0xff;

	debug("boot_type %d, instance %d\n", boot_type, boot_instance);

	/* If not boot from sd/mmc, use default value */
	if ((boot_type != BOOT_TYPE_SD) && (boot_type != BOOT_TYPE_MMC))
		return env_get_ulong("mmcdev", 10, CONFIG_SYS_MMC_ENV_DEV);

	return board_mmc_get_env_dev(boot_instance);

}
#endif

#ifdef CONFIG_USB_PORT_AUTO
int board_usb_gadget_port_auto(void)
{
    enum boot_device bt_dev = get_boot_device();
	int usb_boot_index = 0;

	if (bt_dev == USB2_BOOT)
		usb_boot_index = 1;

	printf("auto usb %d\n", usb_boot_index);

	return usb_boot_index;
}
#endif

static void set_cpu_info(struct ele_get_info_data *info)
{
	gd->arch.soc_rev = info->soc;
	gd->arch.lifecycle = info->lc;
	memcpy((void *)&gd->arch.uid, &info->uid, 4 * sizeof(u32));
}

u32 get_cpu_speed_grade_hz(void)
{
	int ret;
	u32 val;
	u32 speed = MHZ(800);

	ret = fuse_read(3, 1, &val);
	if (!ret) {
		val >>= 14;
		val &= 0x3;

		switch (val) {
		case 0x1:
			speed = MHZ(900); /* 900Mhz*/
			break;
		default:
			speed = MHZ(800); /* 800Mhz*/
		}
	}
	return speed;
}

static u32 get_cpu_variant_type(u32 type)
{
	u32 val;
	int ret;
	ret = fuse_read(3, 2, &val);
	if (!ret) {
		bool epdc_disable = !!(val & BIT(23));
		bool core1_disable = !!(val & BIT(15));
		bool gpu_disable = false;
		bool a35_900mhz = (get_cpu_speed_grade_hz() == MHZ(900));

		if ((val & (BIT(18) | BIT(19))) == (BIT(18) | BIT(19)))
			gpu_disable = true;

		if (epdc_disable && gpu_disable)
			return core1_disable? (type + 4): (type + 3);
		else if (epdc_disable && a35_900mhz)
			return MXC_CPU_IMX8ULPSC;
		else if (epdc_disable)
			return core1_disable? (type + 2): (type + 1);
	}

	return type;
}

static bool is_psw_active_disabled(u32 psw_mask)
{
	u32 psw_active = 0xfffff, powersys_otp_valid;
	int ret;

	ret = fuse_read(3, 5, &powersys_otp_valid);
	if (!ret && (powersys_otp_valid & 0x8000)) {
		ret = fuse_read(5, 5, &psw_active);
		if (ret)
			psw_active = 0xfffff;
	}

	if ((psw_active & psw_mask) == psw_mask)
		return false;

	return true;
}

u32 get_cpu_rev(void)
{
	u32 rev = (gd->arch.soc_rev >> 24) - 0xa0;

	return (get_cpu_variant_type(MXC_CPU_IMX8ULP) << 12) |
		(CHIP_REV_1_0 + rev);
}

enum bt_mode get_boot_mode(void)
{
	u32 bt0_cfg = 0;

	bt0_cfg = readl(SIM_SEC_BASE_ADDR + 0x24);
	bt0_cfg &= (BT0CFG_LPBOOT_MASK | BT0CFG_DUALBOOT_MASK);

	if (!(bt0_cfg & BT0CFG_LPBOOT_MASK)) {
		/* No low power boot */
		if (bt0_cfg & BT0CFG_DUALBOOT_MASK)
			return DUAL_BOOT;
		else
			return SINGLE_BOOT;
	}

	return LOW_POWER_BOOT;
}

bool m33_image_booted(void)
{
	if (IS_ENABLED(CONFIG_SPL_BUILD)) {
		u32 gp6 = 0;

		/* DGO_GP6 */
		gp6 = readl(SIM_SEC_BASE_ADDR + 0x28);
		if (gp6 & BIT(5))
			return true;

		return false;
	} else {
		u32 gpr0 = readl(SIM1_BASE_ADDR);
		if (gpr0 & BIT(0))
			return true;

		return false;
	}
}

bool rdc_enabled_in_boot(void)
{
	if (IS_ENABLED(CONFIG_SPL_BUILD)) {
		u32 val = 0;
		int ret;
		bool rdc_en = true; /* Default assume DBD_EN is set */

		/* Read DBD_EN fuse */
		ret = fuse_read(8, 1, &val);
		if (!ret)
			rdc_en = !!(val & 0x200); /* only A1 part uses DBD_EN, so check DBD_EN new place*/

		return rdc_en;
	} else {
		u32 gpr0 = readl(SIM1_BASE_ADDR);
		if (gpr0 & 0x2)
			return true;

		return false;
	}
}

static void spl_pass_boot_info(void)
{
	if (IS_ENABLED(CONFIG_SPL_BUILD)) {
		bool m33_booted = m33_image_booted();
		bool rdc_en = rdc_enabled_in_boot();
		u32 val = 0;

		if (m33_booted)
			val |= 0x1;

		if (rdc_en)
			val |= 0x2;

		writel(val, SIM1_BASE_ADDR);
	}
}

bool is_m33_handshake_necessary(void)
{
	/* Only need handshake in u-boot */
	if (!IS_ENABLED(CONFIG_SPL_BUILD))
		return (m33_image_booted() || rdc_enabled_in_boot());
	else
		return false;
}

int m33_image_handshake(ulong timeout_ms)
{
	u32 fsr;
	int ret;
	ulong timeout_us = timeout_ms * 1000;

	/* Notify m33 that it's ready to do init srtm(enable mu receive interrupt and so on) */
	setbits_le32(MU0_B_BASE_ADDR + 0x100, BIT(0)); /* set FCR F0 flag of MU0_MUB */

	/*
	 * Wait m33 to set FCR F0 flag of MU0_MUA
	 * Clear FCR F0 flag of MU0_MUB after m33 has set FCR F0 flag of MU0_MUA
	 */
	ret = readl_poll_sleep_timeout(MU0_B_BASE_ADDR + 0x104, fsr, fsr & BIT(0), 10, timeout_us);
	if (!ret)
		clrbits_le32(MU0_B_BASE_ADDR + 0x100, BIT(0));

	return ret;
}

#define CMC_SRS_TAMPER                    BIT(31)
#define CMC_SRS_SECURITY                  BIT(30)
#define CMC_SRS_TZWDG                     BIT(29)
#define CMC_SRS_JTAG_RST                  BIT(28)
#define CMC_SRS_CORE1                     BIT(16)
#define CMC_SRS_LOCKUP                    BIT(15)
#define CMC_SRS_SW                        BIT(14)
#define CMC_SRS_WDG                       BIT(13)
#define CMC_SRS_PIN_RESET                 BIT(8)
#define CMC_SRS_WARM                      BIT(4)
#define CMC_SRS_HVD                       BIT(3)
#define CMC_SRS_LVD                       BIT(2)
#define CMC_SRS_POR                       BIT(1)
#define CMC_SRS_WUP                       BIT(0)

static char *get_reset_cause(char *ret)
{
	u32 cause1, cause = 0, srs = 0;
	void __iomem *reg_ssrs = (void __iomem *)(CMC1_BASE_ADDR + 0x88);
	void __iomem *reg_srs = (void __iomem *)(CMC1_BASE_ADDR + 0x80);

	if (!ret)
		return "null";

	srs = readl(reg_srs);
	cause1 = readl(reg_ssrs);

	cause = srs & (CMC_SRS_POR | CMC_SRS_WUP | CMC_SRS_WARM);

	switch (cause) {
	case CMC_SRS_POR:
		sprintf(ret, "%s", "POR");
		break;
	case CMC_SRS_WUP:
		sprintf(ret, "%s", "WUP");
		break;
	case CMC_SRS_WARM:
		cause = srs & (CMC_SRS_WDG | CMC_SRS_SW |
			CMC_SRS_JTAG_RST);
		switch (cause) {
		case CMC_SRS_WDG:
			sprintf(ret, "%s", "WARM-WDG");
			break;
		case CMC_SRS_SW:
			sprintf(ret, "%s", "WARM-SW");
			break;
		case CMC_SRS_JTAG_RST:
			sprintf(ret, "%s", "WARM-JTAG");
			break;
		default:
			sprintf(ret, "%s", "WARM-UNKN");
			break;
		}
		break;
	default:
		sprintf(ret, "%s-%X", "UNKN", srs);
		break;
	}

	debug("[%X] SRS[%X] %X - ", cause1, srs, srs ^ cause1);
	return ret;
}

#if defined(CONFIG_DISPLAY_CPUINFO)
const char *get_imx_type(u32 imxtype)
{
	switch (imxtype) {
	case MXC_CPU_IMX8ULP:
		return "8ULP(Dual 7)";/* iMX8ULP Dual core 7D/7C */
	case MXC_CPU_IMX8ULPD5:
		return "8ULP(Dual 5)";/* iMX8ULP Dual core 5D/5C, EPDC disabled */
	case MXC_CPU_IMX8ULPS5:
		return "8ULP(Solo 5)";/* iMX8ULP Single core 5D/5C, EPDC disabled */
	case MXC_CPU_IMX8ULPD3:
		return "8ULP(Dual 3)";/* iMX8ULP Dual core 3D/3C, EPDC + GPU disabled */
	case MXC_CPU_IMX8ULPS3:
		return "8ULP(Solo 3)";/* iMX8ULP Single core 3D/3C, EPDC + GPU disabled */
	case MXC_CPU_IMX8ULPSC:
		return "8ULP(SC)";/* iMX8ULP SC part, 900Mhz + EPDC disabled */
	default:
		return "??";
	}
}

int print_cpuinfo(void)
{
	u32 cpurev, max_freq;
	char cause[18];

	cpurev = get_cpu_rev();

	printf("CPU:   i.MX%s rev%d.%d",
		get_imx_type((cpurev & 0x1FF000) >> 12),
		(cpurev & 0x000F0) >> 4, (cpurev & 0x0000F) >> 0);

	max_freq = get_cpu_speed_grade_hz();
	if (!max_freq || max_freq == mxc_get_clock(MXC_ARM_CLK)) {
		printf(" at %dMHz\n", mxc_get_clock(MXC_ARM_CLK) / 1000000);
	} else {
		printf(" %d MHz (running at %d MHz)\n", max_freq / 1000000,
			   mxc_get_clock(MXC_ARM_CLK) / 1000000);
	}

#if defined(CONFIG_SCMI_THERMAL)
	struct udevice *udev;
	int ret, temp;

	ret = uclass_get_device(UCLASS_THERMAL, 0, &udev);
	if (!ret) {
		ret = thermal_get_temp(udev, &temp);
		if (!ret)
			printf("CPU current temperature: %d\n", temp);
		else
			debug(" - failed to get CPU current temperature\n");
	} else {
		debug(" - failed to get CPU current temperature\n");
	}
#endif

	printf("Reset cause: %s\n", get_reset_cause(cause));

	printf("Boot mode: ");
	switch (get_boot_mode()) {
	case LOW_POWER_BOOT:
		printf("Low power boot\n");
		break;
	case DUAL_BOOT:
		printf("Dual boot\n");
		break;
	case SINGLE_BOOT:
	default:
		printf("Single boot\n");
		break;
	}

	return 0;
}
#endif

#define UNLOCK_WORD0 0xC520 /* 1st unlock word */
#define UNLOCK_WORD1 0xD928 /* 2nd unlock word */
#define REFRESH_WORD0 0xA602 /* 1st refresh word */
#define REFRESH_WORD1 0xB480 /* 2nd refresh word */

static void disable_wdog(void __iomem *wdog_base)
{
	u32 val_cs = readl(wdog_base + 0x00);

	dmb();
	__raw_writel(REFRESH_WORD0, (wdog_base + 0x04)); /* Refresh the CNT */
	__raw_writel(REFRESH_WORD1, (wdog_base + 0x04));
	dmb();

	if (!(val_cs & 0x800)) {
		dmb();
		__raw_writel(UNLOCK_WORD0, (wdog_base + 0x04));
		__raw_writel(UNLOCK_WORD1, (wdog_base + 0x04));
		dmb();

		while (!(readl(wdog_base + 0x00) & 0x800))
			;
	}
	writel(0x0, (wdog_base + 0x0C)); /* Set WIN to 0 */
	writel(0x400, (wdog_base + 0x08)); /* Set timeout to default 0x400 */
	writel(0x2120, (wdog_base + 0x00)); /* Change to 32bit cmd, disable it and set update */

	while (!(readl(wdog_base + 0x00) & 0x400))
		;
}

void init_wdog(void)
{
	disable_wdog((void __iomem *)WDG3_RBASE);
}

static struct mm_region imx8ulp_arm64_mem_map[] = {
	{
		/* ROM */
		.virt = 0x0,
		.phys = 0x0,
		.size = 0x40000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
			 PTE_BLOCK_OUTER_SHARE
	},
	{
		/* FLEXSPI0 */
		.virt = 0x04000000,
		.phys = 0x04000000,
		.size = 0x08000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN
	},
	{
		/* SSRAM (align with 2M) */
		.virt = 0x1FE00000UL,
		.phys = 0x1FE00000UL,
		.size = 0x400000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
			 PTE_BLOCK_OUTER_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN
	}, {
		/* SRAM1 (align with 2M) */
		.virt = 0x21000000UL,
		.phys = 0x21000000UL,
		.size = 0x200000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
			 PTE_BLOCK_OUTER_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN
	}, {
		/* SRAM0 (align with 2M) */
		.virt = 0x22000000UL,
		.phys = 0x22000000UL,
		.size = 0x200000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
			 PTE_BLOCK_OUTER_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN
	}, {
		/* Peripherals */
		.virt = 0x27000000UL,
		.phys = 0x27000000UL,
		.size = 0x3000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN
	}, {
		/* Peripherals */
		.virt = 0x2D000000UL,
		.phys = 0x2D000000UL,
		.size = 0x1600000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN
	}, {
		/* FLEXSPI1-2 */
		.virt = 0x40000000UL,
		.phys = 0x40000000UL,
		.size = 0x40000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN
	}, {
		/* DRAM1 */
		.virt = 0x80000000UL,
		.phys = 0x80000000UL,
		.size = PHYS_SDRAM_SIZE,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
#ifdef CONFIG_IMX_TRUSTY_OS
			 PTE_BLOCK_INNER_SHARE
#else
			 PTE_BLOCK_OUTER_SHARE
#endif
	}, {
		/*
		 * empty entrie to split table entry 5
		 * if needed when TEEs are used
		 */
		0,
	}, {
		/* List terminator */
		0,
	}
};

struct mm_region *mem_map = imx8ulp_arm64_mem_map;

static unsigned int imx8ulp_find_dram_entry_in_mem_map(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(imx8ulp_arm64_mem_map); i++)
		if (imx8ulp_arm64_mem_map[i].phys == CFG_SYS_SDRAM_BASE)
			return i;

	hang();	/* Entry not found, this must never happen. */
}

/* simplify the page table size to enhance boot speed */
#define MAX_PTE_ENTRIES		512
#define MAX_MEM_MAP_REGIONS	16
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

void enable_caches(void)
{
	/* If OPTEE runs, remove OPTEE memory from MMU table to avoid speculative prefetch */
	if (rom_pointer[1]) {
		/*
		 * TEE are loaded, So the ddr bank structures
		 * have been modified update mmu table accordingly
		 */
		int i = 0;
		int entry = imx8ulp_find_dram_entry_in_mem_map();
		u64 attrs = imx8ulp_arm64_mem_map[entry].attrs;

		while (i < CONFIG_NR_DRAM_BANKS &&
		       entry < ARRAY_SIZE(imx8ulp_arm64_mem_map)) {
			if (gd->bd->bi_dram[i].start == 0)
				break;
			imx8ulp_arm64_mem_map[entry].phys = gd->bd->bi_dram[i].start;
			imx8ulp_arm64_mem_map[entry].virt = gd->bd->bi_dram[i].start;
			imx8ulp_arm64_mem_map[entry].size = gd->bd->bi_dram[i].size;
			imx8ulp_arm64_mem_map[entry].attrs = attrs;
			debug("Added memory mapping (%d): %llx %llx\n", entry,
			      imx8ulp_arm64_mem_map[entry].phys, imx8ulp_arm64_mem_map[entry].size);
			i++; entry++;
		}
	}

	icache_enable();
	dcache_enable();
}

__weak int board_phys_sdram_size(phys_size_t *size)
{
	if (!size)
		return -EINVAL;

	*size = PHYS_SDRAM_SIZE;
	return 0;
}

int dram_init(void)
{
	unsigned int entry = imx8ulp_find_dram_entry_in_mem_map();
	phys_size_t sdram_size;
	int ret;

	ret = board_phys_sdram_size(&sdram_size);
	if (ret)
		return ret;

	/* rom_pointer[1] contains the size of TEE occupies */
	if (rom_pointer[1])
		gd->ram_size = sdram_size - rom_pointer[1];
	else
		gd->ram_size = sdram_size;

	/* also update the SDRAM size in the mem_map used externally */
	imx8ulp_arm64_mem_map[entry].size = sdram_size;
	return 0;
}

int dram_init_banksize(void)
{
	int bank = 0;
	int ret;
	phys_size_t sdram_size;

	ret = board_phys_sdram_size(&sdram_size);
	if (ret)
		return ret;

	gd->bd->bi_dram[bank].start = PHYS_SDRAM;
	if (rom_pointer[1]) {
		phys_addr_t optee_start = (phys_addr_t)rom_pointer[0];
		phys_size_t optee_size = (size_t)rom_pointer[1];

		gd->bd->bi_dram[bank].size = optee_start - gd->bd->bi_dram[bank].start;
		if ((optee_start + optee_size) < (PHYS_SDRAM + sdram_size)) {
			if (++bank >= CONFIG_NR_DRAM_BANKS) {
				puts("CONFIG_NR_DRAM_BANKS is not enough\n");
				return -1;
			}

			gd->bd->bi_dram[bank].start = optee_start + optee_size;
			gd->bd->bi_dram[bank].size = PHYS_SDRAM +
				sdram_size - gd->bd->bi_dram[bank].start;
		}
	} else {
		gd->bd->bi_dram[bank].size = sdram_size;
	}

	return 0;
}

phys_size_t get_effective_memsize(void)
{
	/* return the first bank as effective memory */
	if (rom_pointer[1])
		return ((phys_addr_t)rom_pointer[0] - PHYS_SDRAM);

	return gd->ram_size;
}

#ifdef CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG
void get_board_serial(struct tag_serialnr *serialnr)
{
	u32 uid[4];
	u32 res;
	int ret;

	ret = ahab_read_common_fuse(1, uid, 4, &res);
	if (ret)
		printf("ahab read fuse failed %d, 0x%x\n", ret, res);
	else
		printf("UID 0x%x,0x%x,0x%x,0x%x\n", uid[0], uid[1], uid[2], uid[3]);

	serialnr->low = uid[0];
	serialnr->high = uid[3];
}
#endif

static void set_core0_reset_vector(u32 entry)
{
	/* Update SIM1 DGO8 for reset vector base */
	writel(entry, SIM1_BASE_ADDR + 0x5c);

	/* set update bit */
	setbits_le32(SIM1_BASE_ADDR + 0x8, 0x1 << 24);

	/* polling the ack */
	while ((readl(SIM1_BASE_ADDR + 0x8) & (0x1 << 26)) == 0)
		;

	/* clear the update */
	clrbits_le32(SIM1_BASE_ADDR + 0x8, (0x1 << 24));

	/* clear the ack by set 1 */
	setbits_le32(SIM1_BASE_ADDR + 0x8, (0x1 << 26));
}

/* Not used now */
int trdc_set_access(void)
{
	/*
	 * TRDC mgr + 4 MBC + 2 MRC.
	 */
	trdc_mbc_set_access(2, 7, 0, 49, true);
	trdc_mbc_set_access(2, 7, 0, 50, true);
	trdc_mbc_set_access(2, 7, 0, 51, true);
	trdc_mbc_set_access(2, 7, 0, 52, true);
	trdc_mbc_set_access(2, 7, 0, 53, true);
	trdc_mbc_set_access(2, 7, 0, 54, true);

	/* 0x1fff8000 used for resource table by remoteproc */
	trdc_mbc_set_access(0, 7, 2, 31, false);

	/* CGC0: PBridge0 slot 47 and PCC0 slot 48 */
	trdc_mbc_set_access(2, 7, 0, 47, false);
	trdc_mbc_set_access(2, 7, 0, 48, false);

	/* PCC1 */
	trdc_mbc_set_access(2, 7, 1, 17, false);
	trdc_mbc_set_access(2, 7, 1, 34, false);

	/* Iomuxc0: : PBridge1 slot 33 */
	trdc_mbc_set_access(2, 7, 1, 33, false);

	/* flexspi0 */
	trdc_mbc_set_access(2, 7, 0, 57, false);
	trdc_mrc_region_set_access(0, 7, 0x04000000, 0x0c000000, false);

	/* tpm0: PBridge1 slot 21 */
	trdc_mbc_set_access(2, 7, 1, 21, false);
	/* lpi2c0: PBridge1 slot 24 */
	trdc_mbc_set_access(2, 7, 1, 24, false);

	/* Allow M33 to access TRDC MGR */
	trdc_mbc_set_access(2, 6, 0, 49, true);
	trdc_mbc_set_access(2, 6, 0, 50, true);
	trdc_mbc_set_access(2, 6, 0, 51, true);
	trdc_mbc_set_access(2, 6, 0, 52, true);
	trdc_mbc_set_access(2, 6, 0, 53, true);
	trdc_mbc_set_access(2, 6, 0, 54, true);

	/* Set SAI0 for eDMA 0, NS */
	trdc_mbc_set_access(2, 0, 1, 28, false);

	/* Set SSRAM for eDMA0 access */
	trdc_mbc_set_access(0, 0, 2, 0, false);
	trdc_mbc_set_access(0, 0, 2, 1, false);
	trdc_mbc_set_access(0, 0, 2, 2, false);
	trdc_mbc_set_access(0, 0, 2, 3, false);
	trdc_mbc_set_access(0, 0, 2, 4, false);
	trdc_mbc_set_access(0, 0, 2, 5, false);
	trdc_mbc_set_access(0, 0, 2, 6, false);
	trdc_mbc_set_access(0, 0, 2, 7, false);

	writel(0x800000a0, 0x28031840);

	return 0;
}

void lpav_configure(bool lpav_to_m33)
{
	if (!lpav_to_m33)
		setbits_le32(SIM_SEC_BASE_ADDR + 0x44, BIT(7)); /* LPAV to APD */

	/* PXP/GPU 2D/3D/DCNANO/MIPI_DSI/EPDC/HIFI4 to APD */
	setbits_le32(SIM_SEC_BASE_ADDR + 0x4c, 0x7F);

	/* LPAV slave/dma2 ch allocation and request allocation to APD */
	writel(0x1f, SIM_SEC_BASE_ADDR + 0x50);
	writel(0xffffffff, SIM_SEC_BASE_ADDR + 0x54);
	writel(0x003fffff, SIM_SEC_BASE_ADDR + 0x58);
}

void load_lposc_fuse(void)
{
	int ret;
	u32 val = 0, val2 = 0, reg;

	ret = fuse_read(25, 0, &val);
	if (ret)
		return; /* failed */

	ret = fuse_read(25, 1, &val2);
	if (ret)
		return; /* failed */

	/* LPOSCCTRL */
	reg = readl(0x2802f304);
	reg &= ~0xff;
	reg |= (val & 0xff);
	writel(reg, 0x2802f304);
}

void set_lpav_qos(void)
{
	/* Set read QoS of dcnano on LPAV NIC */
	writel(0xf, 0x2e447100);
}

int arch_cpu_init(void)
{
	if (IS_ENABLED(CONFIG_SPL_BUILD)) {
		/* Enable System Reset Interrupt using WDOG_AD */
		setbits_le32(CMC1_BASE_ADDR + 0x8C, BIT(13));
		/* Clear AD_PERIPH Power switch domain out of reset interrupt flag */
		setbits_le32(CMC1_BASE_ADDR + 0x70, BIT(4));

		if (readl(CMC1_BASE_ADDR + 0x90) & BIT(13)) {
			/* Clear System Reset Interrupt Flag Register of WDOG_AD */
			setbits_le32(CMC1_BASE_ADDR + 0x90, BIT(13));
			/* Reset WDOG to clear reset request */
			pcc_reset_peripheral(3, WDOG3_PCC3_SLOT, true);
			pcc_reset_peripheral(3, WDOG3_PCC3_SLOT, false);
		}

		/* Disable wdog */
		init_wdog();

		if (get_boot_mode() == SINGLE_BOOT)
			lpav_configure(false);
		else
			lpav_configure(true);

		/* Release xrdc, then allow A35 to write SRAM2 */
		if (rdc_enabled_in_boot())
			release_rdc(RDC_XRDC);

		xrdc_mrc_region_set_access(2, CONFIG_SPL_TEXT_BASE, 0xE00);

		clock_init_early();

		spl_pass_boot_info();
	} else {
		int ret;
		/* reconfigure core0 reset vector to ROM */
		set_core0_reset_vector(0x1000);

		if (is_m33_handshake_necessary()) {
			/* Start handshake with M33 to ensure TRDC configuration completed */
			ret = m33_image_handshake(3000);
			if (!ret)
				gd->arch.m33_handshake_done = true;
			else /* Skip and go through to panic in checkcpu as console is ready then */
				gd->arch.m33_handshake_done = false;
		}
	}

	return 0;
}

int checkcpu(void)
{
	if (is_m33_handshake_necessary()) {
		if (!gd->arch.m33_handshake_done) {
			puts("M33 Sync: Timeout, Boot Stop!\n");
			hang();
		} else {
			puts("M33 Sync: OK\n");
		}
	}
	return 0;
}

int imx8ulp_dm_post_init(void)
{
	struct udevice *devp;
	int ret;
	u32 res;
	struct ele_get_info_data *info = (struct ele_get_info_data *)SRAM0_BASE;

	ret = uclass_get_device_by_driver(UCLASS_MISC, DM_DRIVER_GET(imx8ulp_mu), &devp);
	if (ret) {
		printf("could not get S400 mu %d\n", ret);
		return ret;
	}

	ret = ahab_get_info(info, &res);
	if (ret) {
		printf("ahab_get_info failed %d\n", ret);
		/* fallback to A0.1 revision */
		memset((void *)info, 0, sizeof(struct ele_get_info_data));
		info->soc = 0xa000084d;
	}

	set_cpu_info(info);

	return 0;
}

static int imx8ulp_evt_dm_post_init(void *ctx, struct event *event)
{
	return imx8ulp_dm_post_init();
}
EVENT_SPY(EVT_DM_POST_INIT, imx8ulp_evt_dm_post_init);

#if defined(CONFIG_ARCH_MISC_INIT)
int arch_misc_init(void)
{
	if (IS_ENABLED(CONFIG_FSL_CAAM)) {
		struct udevice *dev;
		int ret;

		ret = uclass_get_device_by_driver(UCLASS_MISC, DM_DRIVER_GET(caam_jr), &dev);
		if (ret)
			printf("Failed to initialize %s: %d\n", dev->name, ret);
	}


	return 0;
}
#endif

#if defined(CONFIG_SPL_BUILD)
__weak void __noreturn jump_to_image_no_args(struct spl_image_info *spl_image)
{
	debug("image entry point: 0x%lx\n", spl_image->entry_point);

	set_core0_reset_vector((u32)spl_image->entry_point);

	/* Enable the 512KB cache */
	setbits_le32(SIM1_BASE_ADDR + 0x30, (0x1 << 4));

	/* reset core */
	setbits_le32(SIM1_BASE_ADDR + 0x30, (0x1 << 16));

	while (1)
		;
}
#endif

void imx_get_mac_from_fuse(int dev_id, unsigned char *mac)
{
	u32 val[2] = {};
	int ret;

	ret = fuse_read(5, 3, &val[0]);
	if (ret)
		goto err;

	ret = fuse_read(5, 4, &val[1]);
	if (ret)
		goto err;

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
	memset(mac, 0, 6);
	printf("%s: fuse read err: %d\n", __func__, ret);
}

int (*card_emmc_is_boot_part_en)(void) = (void *)0x67cc;
u32 spl_arch_boot_image_offset(u32 image_offset, u32 rom_bt_dev)
{
	/* Hard code for eMMC image_offset on 8ULP ROM, need fix by ROM, temp workaround */
	if (is_soc_rev(CHIP_REV_1_0) && ((rom_bt_dev >> 16) & 0xff) == BT_DEV_TYPE_MMC &&
		card_emmc_is_boot_part_en())
		image_offset = 0;

	return image_offset;
}

static int delete_fdt_nodes(void *blob, const char *const nodes_path[], int size_array)
{
	int i = 0;
	int rc;
	int nodeoff;

	for (i = 0; i < size_array; i++) {
		nodeoff = fdt_path_offset(blob, nodes_path[i]);
		if (nodeoff < 0)
			continue; /* Not found, skip it */

		debug("Found %s node\n", nodes_path[i]);

		rc = fdt_del_node(blob, nodeoff);
		if (rc < 0) {
			printf("Unable to delete node %s, err=%s\n",
			       nodes_path[i], fdt_strerror(rc));
		} else {
			printf("Delete node %s\n", nodes_path[i]);
		}
	}

	return 0;
}

static int disable_gpu_nodes(void *blob)
{
	static const char * const nodes_path_npu[] = {
		"/soc@0/gpu3d@2e000000",
		"/soc@0/gpu2d@2e010000",
		"/gpu"
	};

	return delete_fdt_nodes(blob, nodes_path_npu, ARRAY_SIZE(nodes_path_npu));
}

static int disable_pxp_epdc_nodes(void *blob)
{
	static const char * const nodes_path_npu[] = {
		"/soc@0/bus@2d800000/epdc@2db30000",
		"/soc@0/bus@2d800000/epxp@2db40000"
	};

	return delete_fdt_nodes(blob, nodes_path_npu, ARRAY_SIZE(nodes_path_npu));
}

static int disable_hifi_nodes(void *blob)
{
	static const char * const nodes_path_hifi[] = {
		"/sof-sound-btsco",
		"/soc@0/dsp@21170000"
	};

	return delete_fdt_nodes(blob, nodes_path_hifi, ARRAY_SIZE(nodes_path_hifi));
}

#define MAX_CORE_NUM 2
static void disable_pmu_cpu_nodes(void *blob, u32 disabled_cores)
{
	static const char * const pmu_path[] = {
		"/pmu"
	};

	int nodeoff, cnt, i, ret, j;
	u32 irq_affinity[MAX_CORE_NUM];

	for (i = 0; i < ARRAY_SIZE(pmu_path); i++) {
		nodeoff = fdt_path_offset(blob, pmu_path[i]);
		if (nodeoff < 0)
			continue; /* Not found, skip it */

		cnt = fdtdec_get_int_array_count(blob, nodeoff, "interrupt-affinity",
						 irq_affinity, MAX_CORE_NUM);
		if (cnt < 0)
			continue;

		if (cnt != MAX_CORE_NUM)
			printf("Warning: %s, interrupt-affinity count %d\n", pmu_path[i], cnt);

		for (j = 0; j < cnt; j++)
			irq_affinity[j] = cpu_to_fdt32(irq_affinity[j]);

		ret = fdt_setprop(blob, nodeoff, "interrupt-affinity", &irq_affinity,
				 sizeof(u32) * (MAX_CORE_NUM - disabled_cores));
		if (ret < 0) {
			printf("Warning: %s, interrupt-affinity setprop failed %d\n",
			       pmu_path[i], ret);
			continue;
		}

		printf("Update node %s, interrupt-affinity prop\n", pmu_path[i]);
	}
}

static int disable_cpu_nodes(void *blob, u32 disabled_cores)
{
	u32 i = 0;
	int rc;
	int nodeoff;
	char nodes_path[32];

	for (i = 1; i <= disabled_cores; i++) {

		sprintf(nodes_path, "/cpus/cpu@%u", i);

		nodeoff = fdt_path_offset(blob, nodes_path);
		if (nodeoff < 0)
			continue; /* Not found, skip it */

		debug("Found %s node\n", nodes_path);

		rc = fdt_del_node(blob, nodeoff);
		if (rc < 0) {
			printf("Unable to delete node %s, err=%s\n",
			       nodes_path, fdt_strerror(rc));
		} else {
			printf("Delete node %s\n", nodes_path);
		}
	}

	disable_pmu_cpu_nodes(blob, disabled_cores);
	return 0;
}


int ft_system_setup(void *blob, struct bd_info *bd)
{
	u32 uid[4];
	u32 res;
	int ret;
	int nodeoff = fdt_path_offset(blob, "/soc");
	/* Nibble 1st for major version
	 * Nibble 0th for minor version.
	 */
	const u32 rev = 0x10;

	if (nodeoff < 0) {
		printf("Node to update the SoC serial number is not found.\n");
		goto skip_upt;
	}

	ret = ahab_read_common_fuse(1, uid, 4, &res);
	if (ret) {
		printf("ahab read fuse failed %d, 0x%x\n", ret, res);
		memset(uid, 0x0, 4 * sizeof(u32));
	}

	ret = fdt_setprop_u32(blob, nodeoff, "soc-rev", rev);
	if (ret)
		printf("Error[0x%x] fdt_setprop revision-number.\n", ret);

	ret = fdt_setprop_u64(blob, nodeoff, "soc-serial",
				(u64)uid[3] << 32 | uid[0]);
	if (ret)
		printf("Error[0x%x] fdt_setprop serial-number.\n", ret);

	if (IS_ENABLED(CONFIG_KASLR)) {
		ret = do_generate_kaslr(blob);
		if (ret)
			goto skip_upt;
	}

skip_upt:
	if (is_imx8ulps5() || is_imx8ulps3())
		disable_cpu_nodes(blob, 1);

	if (is_imx8ulpd5() || is_imx8ulps5() || is_imx8ulpd3() ||
	    is_imx8ulps3() || is_imx8ulpsc())
		disable_pxp_epdc_nodes(blob);

	if (is_imx8ulpd3() || is_imx8ulps3())
		disable_gpu_nodes(blob);

	/* Check if HIFI4 PS is not allowed to power on */
	if (is_imx8ulpsc() && is_psw_active_disabled(BIT(8)))
		disable_hifi_nodes(blob);

	return ft_add_optee_node(blob, bd);
}

enum env_location env_get_location(enum env_operation op, int prio)
{
	enum boot_device dev = get_boot_device();
	enum env_location env_loc = ENVL_UNKNOWN;

	if (prio)
		return env_loc;

	switch (dev) {
#ifdef CONFIG_ENV_IS_IN_SPI_FLASH
	case QSPI_BOOT:
		env_loc = ENVL_SPI_FLASH;
		break;
#endif
#ifdef CONFIG_ENV_IS_IN_MMC
	case SD1_BOOT:
	case SD2_BOOT:
	case SD3_BOOT:
	case MMC1_BOOT:
	case MMC2_BOOT:
	case MMC3_BOOT:
		env_loc =  ENVL_MMC;
		break;
#endif
	default:
#if defined(CONFIG_ENV_IS_NOWHERE)
		env_loc = ENVL_NOWHERE;
#endif
		break;
	}

	return env_loc;
}
