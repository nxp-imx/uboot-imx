// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2022 NXP
 *
 * Peng Fan <peng.fan@nxp.com>
 */

#include <common.h>
#include <cpu_func.h>
#include <init.h>
#include <log.h>
#include <asm/arch/imx-regs.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <asm/arch/clock.h>
#include <asm/arch/ccm_regs.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/trdc.h>
#include <asm/mach-imx/boot_mode.h>
#include <asm/mach-imx/syscounter.h>
#include <asm/armv8/mmu.h>
#include <dm/uclass.h>
#include <env.h>
#include <env_internal.h>
#include <errno.h>
#include <fdt_support.h>
#include <linux/bitops.h>
#include <asm/setup.h>
#include <asm/bootm.h>
#include <asm/arch-imx/cpu.h>
#include <asm/mach-imx/s400_api.h>
#include <asm/mach-imx/optee.h>
#include <linux/delay.h>
#include <fuse.h>
#include <imx_thermal.h>
#include <thermal.h>
#include <imx_sip.h>
#include <linux/arm-smccc.h>

DECLARE_GLOBAL_DATA_PTR;

struct rom_api *g_rom_api = (struct rom_api *)0x1980;

enum boot_device get_boot_device(void)
{
	volatile gd_t *pgd = gd;
	int ret;
	u32 boot;
	u16 boot_type;
	u8 boot_instance;
	enum boot_device boot_dev = SD1_BOOT;

	ret = g_rom_api->query_boot_infor(QUERY_BT_DEV, &boot,
					  ((uintptr_t)&boot) ^ QUERY_BT_DEV);
	set_gd(pgd);

	if (ret != ROM_API_OKAY) {
		puts("ROMAPI: failure at query_boot_info\n");
		return -1;
	}

	boot_type = boot >> 16;
	boot_instance = (boot >> 8) & 0xff;

	switch (boot_type) {
	case BT_DEV_TYPE_SD:
		boot_dev = boot_instance + SD1_BOOT;
		break;
	case BT_DEV_TYPE_MMC:
		boot_dev = boot_instance + MMC1_BOOT;
		break;
	case BT_DEV_TYPE_NAND:
		boot_dev = NAND_BOOT;
		break;
	case BT_DEV_TYPE_FLEXSPINOR:
		boot_dev = QSPI_BOOT;
		break;
	case BT_DEV_TYPE_USB:
		boot_dev = boot_instance + USB_BOOT;
		break;
	default:
		break;
	}

	debug("boot dev %d\n", boot_dev);

	return boot_dev;
}

bool is_usb_boot(void)
{
	enum boot_device bt_dev = get_boot_device();
	return (bt_dev == USB_BOOT || bt_dev == USB2_BOOT);
}

void disconnect_from_pc(void)
{
	enum boot_device bt_dev = get_boot_device();

	if (bt_dev == USB_BOOT)
		writel(0x0, USB1_BASE_ADDR + 0x140);
	else if (bt_dev == USB2_BOOT)
		writel(0x0, USB2_BASE_ADDR + 0x140);

	return;
}

#ifdef CONFIG_ENV_IS_IN_MMC
__weak int board_mmc_get_env_dev(int devno)
{
	return devno;
}

int mmc_get_env_dev(void)
{
	volatile gd_t *pgd = gd;
	int ret;
	u32 boot;
	u16 boot_type;
	u8 boot_instance;

	ret = g_rom_api->query_boot_infor(QUERY_BT_DEV, &boot,
					  ((uintptr_t)&boot) ^ QUERY_BT_DEV);
	set_gd(pgd);

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

u32 get_cpu_speed_grade_hz(void)
{
	u32 speed, max_speed;
	u32 val = readl((ulong)FSB_BASE_ADDR + 0x8000 + (19 << 2));
	val >>= 6;
	val &= 0xf;

	max_speed = 2300000000;
	speed = max_speed - val * 100000000;

	if (is_imx93())
		max_speed = 1700000000;

	/* In case the fuse of speed grade not programmed */
	if (speed > max_speed)
		speed = max_speed;

	return speed;
}

u32 get_cpu_temp_grade(int *minc, int *maxc)
{
	u32 val = readl((ulong)FSB_BASE_ADDR + 0x8000 + (19 << 2));

	val >>= 4;
	val &= 0x3;

	if (minc && maxc) {
		if (val == TEMP_AUTOMOTIVE) {
			*minc = -40;
			*maxc = 125;
		} else if (val == TEMP_INDUSTRIAL) {
			*minc = -40;
			*maxc = 105;
		} else if (val == TEMP_EXTCOMMERCIAL) {
			if (is_imx93()){
				/* imx93 only has extended industrial*/
				*minc = -40;
				*maxc = 125;
			} else {
				*minc = -20;
				*maxc = 105;
			}
		} else {
			*minc = 0;
			*maxc = 95;
		}
	}
	return val;
}

static void set_cpu_info(struct sentinel_get_info_data *info)
{
	gd->arch.soc_rev = info->soc;
	gd->arch.lifecycle = info->lc;
	memcpy((void *)&gd->arch.uid, &info->uid, 4 * sizeof(u32));
}

static u32 get_cpu_variant_type(u32 type)
{
	/* word 19 */
	u32 val = readl((ulong)FSB_BASE_ADDR + 0x8000 + (19 << 2));
	u32 val2 = readl((ulong)FSB_BASE_ADDR + 0x8000 + (20 << 2));
	bool npu_disable = !!(val & BIT(13));
	bool core1_disable = !!(val & BIT(15));
	u32 pack_9x9_fused = BIT(4) | BIT(5) | BIT(17) | BIT(19) | BIT(24);

	if ((val2 & pack_9x9_fused) == pack_9x9_fused)
		type = MXC_CPU_IMX9322;

	if (npu_disable && core1_disable)
		return type + 3;
	else if (npu_disable)
		return type + 2;
	else if (core1_disable)
		return type + 1;

	return type;
}

u32 get_cpu_rev(void)
{
	u32 rev = (gd->arch.soc_rev >> 24) - 0xa0;
	return (get_cpu_variant_type(MXC_CPU_IMX93) << 12) |
		(CHIP_REV_1_0 + rev);
}

#define UNLOCK_WORD 0xD928C520 /* unlock word */
#define REFRESH_WORD 0xB480A602 /* refresh word */

static void disable_wdog(void __iomem *wdog_base)
{
	u32 val_cs = readl(wdog_base + 0x00);

	if (!(val_cs & 0x80))
		return;

	/* default is 32bits cmd */
	writel(REFRESH_WORD, (wdog_base + 0x04)); /* Refresh the CNT */

	if (!(val_cs & 0x800)) {
		writel(UNLOCK_WORD, (wdog_base + 0x04));
		while (!(readl(wdog_base + 0x00) & 0x800))
			;
	}
	writel(0x0, (wdog_base + 0x0C)); /* Set WIN to 0 */
	writel(0x400, (wdog_base + 0x08)); /* Set timeout to default 0x400 */
	writel(0x2120, (wdog_base + 0x00)); /* Disable it and set update */

	while (!(readl(wdog_base + 0x00) & 0x400))
		;
}

void init_wdog(void)
{
	u32 src_val;

	disable_wdog((void __iomem *)WDG3_BASE_ADDR);
	disable_wdog((void __iomem *)WDG4_BASE_ADDR);
	disable_wdog((void __iomem *)WDG5_BASE_ADDR);

	src_val = readl(0x54460018); /* reset mask */
	src_val &= ~0x1c;
	writel(src_val, 0x54460018);
}

static struct mm_region imx93_mem_map[] = {
	{
		/* ROM */
		.virt = 0x0UL,
		.phys = 0x0UL,
		.size = 0x100000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
			 PTE_BLOCK_OUTER_SHARE
	}, {
		/* TCM */
		.virt = 0x201c0000UL,
		.phys = 0x201c0000UL,
		.size = 0x80000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN
	}, {
		/* OCRAM */
		.virt = 0x20480000UL,
		.phys = 0x20480000UL,
		.size = 0xA0000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
			 PTE_BLOCK_OUTER_SHARE
	}, {
		/* AIPS */
		.virt = 0x40000000UL,
		.phys = 0x40000000UL,
		.size = 0x40000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN
	}, {
		/* Flexible Serial Peripheral Interface */
		.virt = 0x28000000UL,
		.phys = 0x28000000UL,
		.size = 0x30000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN
	}, {
		/* DRAM1 */
		.virt = 0x80000000UL,
		.phys = 0x80000000UL,
		.size = PHYS_SDRAM_SIZE,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
			 PTE_BLOCK_OUTER_SHARE
	}, {
		/* empty entrie to split table entry 5 if needed when TEEs are used */
		0,
	}, {
		/* List terminator */
		0,
	}
};

struct mm_region *mem_map = imx93_mem_map;

static unsigned int imx9_find_dram_entry_in_mem_map(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(imx93_mem_map); i++)
		if (imx93_mem_map[i].phys == CONFIG_SYS_SDRAM_BASE)
			return i;

	hang();	/* Entry not found, this must never happen. */
}

void enable_caches(void)
{
	/* If OPTEE runs, remove OPTEE memory from MMU table to avoid speculative prefetch
	 * If OPTEE does not run, still update the MMU table according to dram banks structure
	 * to set correct dram size from board_phys_sdram_size
	 */
	int i = 0;
	/*
	 * please make sure that entry initial value matches
	 * imx93_mem_map for DRAM1
	 */
	int entry = imx9_find_dram_entry_in_mem_map();
	u64 attrs = imx93_mem_map[entry].attrs;

	while (i < CONFIG_NR_DRAM_BANKS &&
	       entry < ARRAY_SIZE(imx93_mem_map)) {
		if (gd->bd->bi_dram[i].start == 0)
			break;
		imx93_mem_map[entry].phys = gd->bd->bi_dram[i].start;
		imx93_mem_map[entry].virt = gd->bd->bi_dram[i].start;
		imx93_mem_map[entry].size = gd->bd->bi_dram[i].size;
		imx93_mem_map[entry].attrs = attrs;
		debug("Added memory mapping (%d): %llx %llx\n", entry,
		      imx93_mem_map[entry].phys, imx93_mem_map[entry].size);
		i++; entry++;
	}

	icache_enable();
	dcache_enable();
}

__weak int board_phys_sdram_size(phys_size_t *size)
{
	if (!size)
		return -EINVAL;

	*size = PHYS_SDRAM_SIZE;

#ifdef PHYS_SDRAM_2_SIZE
	*size += PHYS_SDRAM_2_SIZE;
#endif
	return 0;
}

int dram_init(void)
{
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

	return 0;
}

int dram_init_banksize(void)
{
	int bank = 0;
	int ret;
	phys_size_t sdram_size;
	phys_size_t sdram_b1_size, sdram_b2_size;

	ret = board_phys_sdram_size(&sdram_size);
	if (ret)
		return ret;

	/* Bank 1 can't cross over 4GB space */
	if (sdram_size > 0x80000000) {
		sdram_b1_size = 0x80000000;
		sdram_b2_size = sdram_size - 0x80000000;
	} else {
		sdram_b1_size = sdram_size;
		sdram_b2_size = 0;
	}

	gd->bd->bi_dram[bank].start = PHYS_SDRAM;
	if (rom_pointer[1]) {
		phys_addr_t optee_start = (phys_addr_t)rom_pointer[0];
		phys_size_t optee_size = (size_t)rom_pointer[1];

		gd->bd->bi_dram[bank].size = optee_start - gd->bd->bi_dram[bank].start;
		if ((optee_start + optee_size) < (PHYS_SDRAM + sdram_b1_size)) {
			if (++bank >= CONFIG_NR_DRAM_BANKS) {
				puts("CONFIG_NR_DRAM_BANKS is not enough\n");
				return -1;
			}

			gd->bd->bi_dram[bank].start = optee_start + optee_size;
			gd->bd->bi_dram[bank].size = PHYS_SDRAM +
				sdram_b1_size - gd->bd->bi_dram[bank].start;
		}
	} else {
		gd->bd->bi_dram[bank].size = sdram_b1_size;
	}

	if (sdram_b2_size) {
		if (++bank >= CONFIG_NR_DRAM_BANKS) {
			puts("CONFIG_NR_DRAM_BANKS is not enough for SDRAM_2\n");
			return -1;
		}
		gd->bd->bi_dram[bank].start = 0x100000000UL;
		gd->bd->bi_dram[bank].size = sdram_b2_size;
	}

	return 0;
}

phys_size_t get_effective_memsize(void)
{
	int ret;
	phys_size_t sdram_size;
	phys_size_t sdram_b1_size;
	ret = board_phys_sdram_size(&sdram_size);
	if (!ret) {
		/* Bank 1 can't cross over 4GB space */
		if (sdram_size > 0x80000000) {
			sdram_b1_size = 0x80000000;
		} else {
			sdram_b1_size = sdram_size;
		}

		if (rom_pointer[1]) {
			/* We will relocate u-boot to Top of dram1. Tee position has two cases:
			 * 1. At the top of dram1,  Then return the size removed optee size.
			 * 2. In the middle of dram1, return the size of dram1.
			 */
			if ((rom_pointer[0] + rom_pointer[1]) == (PHYS_SDRAM + sdram_b1_size))
				return ((phys_addr_t)rom_pointer[0] - PHYS_SDRAM);
		}

		return sdram_b1_size;
	} else {
		return PHYS_SDRAM_SIZE;
	}
}


void imx_get_mac_from_fuse(int dev_id, unsigned char *mac)
{
	u32 val[2] = {};
	int ret;

	if (dev_id == 0) {
		ret = fuse_read(39, 3, &val[0]);
		if (ret)
			goto err;

		ret = fuse_read(39, 4, &val[1]);
		if (ret)
			goto err;

		mac[0] = val[1] >> 8;
		mac[1] = val[1];
		mac[2] = val[0] >> 24;
		mac[3] = val[0] >> 16;
		mac[4] = val[0] >> 8;
		mac[5] = val[0];

	} else {
		ret = fuse_read(39, 5, &val[0]);
		if (ret)
			goto err;

		ret = fuse_read(39, 4, &val[1]);
		if (ret)
			goto err;

		mac[0] = val[1] >> 24;
		mac[1] = val[1] >> 16;
		mac[2] = val[0] >> 24;
		mac[3] = val[0] >> 16;
		mac[4] = val[0] >> 8;
		mac[5] = val[0];
	}

	debug("%s: MAC%d: %02x.%02x.%02x.%02x.%02x.%02x\n",
	      __func__, dev_id, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	return;
err:
	memset(mac, 0, 6);
	printf("%s: fuse read err: %d\n", __func__, ret);
}

const char *get_imx_type(u32 imxtype)
{
	switch (imxtype) {
	case MXC_CPU_IMX93:
		return "93(52)";/* iMX93 Dual core with NPU */
	case MXC_CPU_IMX9351:
		return "93(51)";/* iMX93 Single core with NPU */
	case MXC_CPU_IMX9332:
		return "93(32)";/* iMX93 Dual core without NPU */
	case MXC_CPU_IMX9331:
		return "93(31)";/* iMX93 Single core without NPU */
	case MXC_CPU_IMX9322:
		return "93(22)";/* iMX93 9x9 Dual core  */
	case MXC_CPU_IMX9321:
		return "93(21)";/* iMX93 9x9 Single core  */
	case MXC_CPU_IMX9312:
		return "93(12)";/* iMX93 9x9 Dual core without NPU */
	case MXC_CPU_IMX9311:
		return "93(11)";/* iMX93 9x9 Single core without NPU */
	default:
		return "??";
	}
}

#define SRC_SRSR_RESET_CAUSE_NUM 16
const char *reset_cause[SRC_SRSR_RESET_CAUSE_NUM] = {
	"POR ",
	"JTAG ",
	"IPP USER ",
	"WDOG1 ",
	"WDOG2 ",
	"WDOG3 ",
	"WDOG4 ",
	"WDOG5 ",
	"TEMPSENSE ",
	"CSU ",
	"JTAG_SW ",
	"M33_REQ ",
	"M33_LOCKUP "
	"UNK ",
	"UNK ",
	"UNK ",
};

static void save_reset_cause(void)
{
	struct src_general_regs *src = (struct src_general_regs *)SRC_GLOBAL_RBASE;
	u32 srsr = readl(&src->srsr);
	writel(srsr, &src->srsr); /* clear srsr in sec mode */

	/* Save value to GPR1 to pass to nonsecure */
	writel(srsr, &src->gpr[0]);
}

static const char *get_reset_cause(u32 *srsr_ret)
{
	struct src_general_regs *src = (struct src_general_regs *)SRC_GLOBAL_RBASE;
	u32 srsr;
	u32 i;

	srsr = readl(&src->gpr[0]);
	if (srsr_ret)
		*srsr_ret = srsr;

	for (i = SRC_SRSR_RESET_CAUSE_NUM; i > 0; i--) {
		if (srsr & (1 << (i - 1)))
			return reset_cause[i - 1];
	}

	return "unknown reset";
}

int print_cpuinfo(void)
{
	u32 cpurev, max_freq;
	int minc, maxc;
	u32 ssrs_ret;

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

	puts("CPU:   ");
	switch (get_cpu_temp_grade(&minc, &maxc)) {
	case TEMP_AUTOMOTIVE:
		puts("Automotive temperature grade ");
		break;
	case TEMP_INDUSTRIAL:
		puts("Industrial temperature grade ");
		break;
	case TEMP_EXTCOMMERCIAL:
		if (is_imx93())
			puts("Extended Industrial temperature grade ");
		else
			puts("Extended Consumer temperature grade ");
		break;
	default:
		puts("Consumer temperature grade ");
		break;
	}
	printf("(%dC to %dC)", minc, maxc);

#if defined(CONFIG_IMX_TMU)
	struct udevice *udev;
	int ret, temp;

	ret = uclass_get_device_by_name(UCLASS_THERMAL, "cpu-thermal", &udev);
	if (!ret) {
		ret = thermal_get_temp(udev, &temp);

		if (!ret)
			printf(" at %dC", temp);
		else
			debug(" - invalid sensor data\n");
	} else {
		debug(" - invalid sensor device\n");
	}
#endif
	puts("\n");

	printf("Reset cause: %s", get_reset_cause(&ssrs_ret));
	printf("(0x%x)\n", ssrs_ret);

	return 0;
}

void build_info(void)
{
	u32 fw_version, sha1, res, status;
	int ret;

	printf("\nBuildInfo:\n");

	ret = ahab_get_fw_status(&status, &res);
	if (ret) {
		printf("  - ELE firmware status failed %d, 0x%x\n", ret, res);
	} else if ((status & 0xff) == 1) {
		ret = ahab_get_fw_version(&fw_version, &sha1, &res);
		if (ret) {
			printf("  - ELE firmware version failed %d, 0x%x\n", ret, res);
		} else {
			printf("  - ELE firmware version %u.%u.%u-%x",
			       (fw_version & (0x00ff0000)) >> 16,
			       (fw_version & (0x0000ff00)) >> 8,
			       (fw_version & (0x000000ff)), sha1);
			((fw_version & (0x80000000)) >> 31) == 1 ? puts("-dirty\n") : puts("\n");
		}
	} else {
		printf("  - ELE firmware not included\n");
	}
	puts("\n");
}

int arch_misc_init(void)
{
	build_info();
	return 0;
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

static int disable_npu_nodes(void *blob)
{
	static const char * const nodes_path_npu[] = {
		"/ethosu",
		"/reserved-memory/ethosu_region@C0000000"
	};

	return delete_fdt_nodes(blob, nodes_path_npu, ARRAY_SIZE(nodes_path_npu));
}

static void disable_thermal_cpu_nodes(void *blob, u32 disabled_cores)
{
	static const char * const thermal_path[] = {
		"/thermal-zones/cpu-thermal/cooling-maps/map0"
	};

	int nodeoff, cnt, i, ret, j;
	u32 cooling_dev[6];

	for (i = 0; i < ARRAY_SIZE(thermal_path); i++) {
		nodeoff = fdt_path_offset(blob, thermal_path[i]);
		if (nodeoff < 0)
			continue; /* Not found, skip it */

		cnt = fdtdec_get_int_array_count(blob, nodeoff, "cooling-device", cooling_dev, 6);
		if (cnt < 0)
			continue;

		if (cnt != 6)
			printf("Warning: %s, cooling-device count %d\n", thermal_path[i], cnt);

		for (j = 0; j < cnt; j++)
			cooling_dev[j] = cpu_to_fdt32(cooling_dev[j]);

		ret = fdt_setprop(blob, nodeoff, "cooling-device", &cooling_dev,
				  sizeof(u32) * (6 - disabled_cores * 3));
		if (ret < 0) {
			printf("Warning: %s, cooling-device setprop failed %d\n",
			       thermal_path[i], ret);
			continue;
		}

		printf("Update node %s, cooling-device prop\n", thermal_path[i]);
	}
}

static int disable_cpu_nodes(void *blob, u32 disabled_cores)
{
	u32 i = 0;
	int rc;
	int nodeoff;
	char nodes_path[32];

	for (i = 1; i <= disabled_cores; i++) {

		sprintf(nodes_path, "/cpus/cpu@%u00", i);

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

	disable_thermal_cpu_nodes(blob, disabled_cores);

	return 0;
}

struct low_drive_freq_entry {
	const char *node_path;
	u32 clk;
	u32 new_rate;
};

static int low_drive_fdt_fix_clock(void *fdt, int node_off, u32 clk_index, u32 new_rate)
{
#define MAX_ASSIGNED_CLKS 8
	int cnt, j;
	u32 assignedclks[MAX_ASSIGNED_CLKS]; /* max 8 clocks*/

	cnt = fdtdec_get_int_array_count(fdt, node_off, "assigned-clock-rates",
		assignedclks, MAX_ASSIGNED_CLKS);
	if (cnt > 0) {
		if (cnt <= clk_index)
			return -ENOENT;

		if (assignedclks[clk_index] <= new_rate)
			return 0;

		assignedclks[clk_index] = new_rate;
		for (j = 0; j < cnt; j++)
			assignedclks[j] = cpu_to_fdt32(assignedclks[j]);

		return fdt_setprop(fdt, node_off, "assigned-clock-rates", &assignedclks, cnt * sizeof(u32));
	}

	return -ENOENT;
}

static int low_drive_freq_update(void *blob)
{
	int nodeoff, ret;
	int i;

	/* Update kernel dtb clocks for low drive mode */
	struct low_drive_freq_entry table[] = {
		{"/soc@0/lcd-controller@4ae30000", 2, 200000000},
		{"/soc@0/bus@42800000/camera/isi@4ae40000", 0, 200000000},
		{"/soc@0/bus@42800000/mmc@42850000", 0, 266666667},
		{"/soc@0/bus@42800000/mmc@42860000", 0, 266666667},
		{"/soc@0/bus@42800000/mmc@428b0000", 0, 266666667},
	};

	for (i = 0; i < ARRAY_SIZE(table); i++) {
		nodeoff = fdt_path_offset(blob, table[i].node_path);
		if (nodeoff >= 0) {
			ret = low_drive_fdt_fix_clock(blob, nodeoff, table[i].clk, table[i].new_rate);
			if (!ret)
				printf("%s freq updated\n", table[i].node_path);
		}
	}

	return 0;
}

#ifdef CONFIG_OF_BOARD_FIXUP
#ifndef CONFIG_SPL_BUILD
int board_fix_fdt(void *fdt)
{
	/* Update u-boot dtb clocks for low drive mode */
	if (IS_ENABLED(CONFIG_IMX9_LOW_DRIVE_MODE)){
		int nodeoff;
		int i;

		struct low_drive_freq_entry table[] = {
			{"/soc@0/lcd-controller@4ae30000", 0, 200000000},
			{"/soc@0/bus@42800000/mmc@42850000", 0, 266666667},
			{"/soc@0/bus@42800000/mmc@42860000", 0, 266666667},
			{"/soc@0/bus@42800000/mmc@428b0000", 0, 266666667},
		};

		for (i = 0; i < ARRAY_SIZE(table); i++) {
			nodeoff = fdt_path_offset(fdt, table[i].node_path);
			if (nodeoff >= 0)
				low_drive_fdt_fix_clock(fdt, nodeoff, table[i].clk, table[i].new_rate);
		}
	}

	return 0;
}
#endif
#endif

int ft_system_setup(void *blob, struct bd_info *bd)
{
	if (is_imx9351() || is_imx9331() || is_imx9321() || is_imx9311())
		disable_cpu_nodes(blob, 1);

	if (is_imx9332() || is_imx9331() || is_imx9312() || is_imx9311())
		disable_npu_nodes(blob);

	if (IS_ENABLED(CONFIG_IMX9_LOW_DRIVE_MODE))
		low_drive_freq_update(blob);

	return ft_add_optee_node(blob, bd);
}

#if defined(CONFIG_SERIAL_TAG) || defined(CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG)
void get_board_serial(struct tag_serialnr *serialnr)
{
	printf("UID: 0x%x 0x%x 0x%x 0x%x\n",
		gd->arch.uid[0], gd->arch.uid[1], gd->arch.uid[2], gd->arch.uid[3]);

	serialnr->low = gd->arch.uid[0];
	serialnr->high = gd->arch.uid[3];
}
#endif

int arch_cpu_init(void)
{
	if (IS_ENABLED(CONFIG_SPL_BUILD)) {
		/* Disable wdog */
		init_wdog();

		clock_init();

		trdc_early_init();

		/* Save SRC SRSR to GPR1 and clear it */
		save_reset_cause();
	}

	return 0;
}

int arch_cpu_init_dm(void)
{
	struct udevice *devp;
	int node, ret;
	u32 res;
	struct sentinel_get_info_data info;

	node = fdt_node_offset_by_compatible(gd->fdt_blob, -1, "fsl,imx93-mu-s4");

	ret = uclass_get_device_by_of_offset(UCLASS_MISC, node, &devp);
	if (ret)
		return ret;

	ret = ahab_get_info(&info, &res);
	if (ret)
		return ret;

	set_cpu_info(&info);

	return 0;
}

#ifdef CONFIG_ARCH_EARLY_INIT_R
int arch_early_init_r(void)
{
	struct udevice *devp;
	int node, ret;

	node = fdt_node_offset_by_compatible(gd->fdt_blob, -1, "fsl,imx93-mu-s4");

	ret = uclass_get_device_by_of_offset(UCLASS_MISC, node, &devp);
	if (ret) {
		printf("could not get S400 mu %d\n", ret);
		return ret;
	}

	return 0;
}
#endif

int timer_init(void)
{
#ifdef CONFIG_SPL_BUILD
	struct sctr_regs *sctr = (struct sctr_regs *)SYSCNT_CTRL_BASE_ADDR;
	unsigned long freq = readl(&sctr->cntfid0);

	/* Update with accurate clock frequency */
	asm volatile("msr cntfrq_el0, %0" : : "r" (freq) : "memory");

	clrsetbits_le32(&sctr->cntcr, SC_CNTCR_FREQ0 | SC_CNTCR_FREQ1,
			SC_CNTCR_FREQ0 | SC_CNTCR_ENABLE | SC_CNTCR_HDBG);
#endif

	gd->arch.tbl = 0;
	gd->arch.tbu = 0;

	return 0;
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

int mix_power_init(enum mix_power_domain pd)
{
	enum src_mix_slice_id mix_id;
	enum src_mem_slice_id mem_id;
	struct src_mix_slice_regs *mix_regs;
	struct src_mem_slice_regs *mem_regs;
	struct src_general_regs *global_regs;
	u32 scr, val;

	switch (pd) {
	case MIX_PD_MEDIAMIX:
		mix_id = SRC_MIX_MEDIA;
		mem_id = SRC_MEM_MEDIA;
		scr = BIT(5);

		/* Enable S400 handshake */
		struct blk_ctrl_s_aonmix_regs *s_regs =
			(struct blk_ctrl_s_aonmix_regs *)BLK_CTRL_S_ANOMIX_BASE_ADDR;

		setbits_le32(&s_regs->lp_handshake[0], BIT(13));
		break;
	case MIX_PD_MLMIX:
		mix_id = SRC_MIX_ML;
		mem_id = SRC_MEM_ML;
		scr = BIT(4);
		break;
	case MIX_PD_DDRMIX:
		mix_id = SRC_MIX_DDRMIX;
		mem_id = SRC_MEM_DDRMIX;
		scr = BIT(6);
		break;
	default:
		return -EINVAL;
	}

	mix_regs = (struct src_mix_slice_regs *)(ulong)(SRC_IPS_BASE_ADDR + 0x400 * (mix_id + 1));
	mem_regs = (struct src_mem_slice_regs *)(ulong)(SRC_IPS_BASE_ADDR + 0x3800 + 0x400 * mem_id);
	global_regs = (struct src_general_regs *)(ulong)SRC_GLOBAL_RBASE;

	/* Allow NS to set it */
	setbits_le32(&mix_regs->authen_ctrl, BIT(9));

	clrsetbits_le32(&mix_regs->psw_ack_ctrl[0], BIT(28), BIT(29));

	/* mix reset will be held until boot core write this bit to 1 */
	setbits_le32(&global_regs->scr, scr);

	/* Enable mem in Low power auto sequence */
	setbits_le32(&mem_regs->mem_ctrl, BIT(2));

	/* Set the power down state */
	val = readl(&mix_regs->func_stat);
	if (val & SRC_MIX_SLICE_FUNC_STAT_PSW_STAT) {
		/* The mix is default power off, power down it to make PDN_SFT bit
		 *  aligned with FUNC STAT
		 */
		setbits_le32(&mix_regs->slice_sw_ctrl, BIT(31));
		val = readl(&mix_regs->func_stat);

		/* Since PSW_STAT is 1, can't be used for power off status (SW_CTRL BIT31 set)) */
		/* Check the MEM STAT change to ensure SSAR is completed */
		while (!(val & SRC_MIX_SLICE_FUNC_STAT_MEM_STAT)) {
			val = readl(&mix_regs->func_stat);
		}

		/* wait few ipg clock cycles to ensure FSM done and power off status is correct */
		/* About 5 cycles at 24Mhz, 1us is enough  */
		udelay(1);
	} else {
		/*  The mix is default power on, Do mix power cycle */
		setbits_le32(&mix_regs->slice_sw_ctrl, BIT(31));
		val = readl(&mix_regs->func_stat);
		while (!(val & SRC_MIX_SLICE_FUNC_STAT_PSW_STAT)) {
			val = readl(&mix_regs->func_stat);
		}
	}

	/* power on */
	clrbits_le32(&mix_regs->slice_sw_ctrl, BIT(31));
	val = readl(&mix_regs->func_stat);
	while (val & SRC_MIX_SLICE_FUNC_STAT_ISO_STAT) {
		val = readl(&mix_regs->func_stat);
	}

	return 0;
}

void disable_isolation(void)
{
	struct src_general_regs *global_regs = (struct src_general_regs *)(ulong)SRC_GLOBAL_RBASE;
	/* clear isolation for usbphy, dsi, csi*/
	writel(0x0, &global_regs->sp_iso_ctrl);
}

void soc_power_init(void)
{
	mix_power_init(MIX_PD_MEDIAMIX);
	mix_power_init(MIX_PD_MLMIX);

	disable_isolation();
}

bool m33_is_rom_kicked(void)
{
	struct blk_ctrl_s_aonmix_regs *s_regs =
			(struct blk_ctrl_s_aonmix_regs *)BLK_CTRL_S_ANOMIX_BASE_ADDR;

	if (!(readl(&s_regs->m33_cfg) & BCTRL_S_ANOMIX_M33_CPU_WAIT_MASK))
		return true;

	return false;
}

int m33_prepare(void)
{
	struct src_mix_slice_regs *mix_regs =
		(struct src_mix_slice_regs *)(ulong)(SRC_IPS_BASE_ADDR + 0x400 * (SRC_MIX_CM33 + 1));
	struct src_general_regs *global_regs =
		(struct src_general_regs *)(ulong)SRC_GLOBAL_RBASE;
	struct blk_ctrl_s_aonmix_regs *s_regs =
			(struct blk_ctrl_s_aonmix_regs *)BLK_CTRL_S_ANOMIX_BASE_ADDR;
	u32 val;

	/* Allow NS to set it */
	setbits_le32(&mix_regs->authen_ctrl, BIT(9));

	if (m33_is_rom_kicked())
		return -EPERM;

	/* Release reset of M33 */
	setbits_le32(&global_regs->scr, BIT(0));

	/* Check the reset released in M33 MIX func stat */
	val = readl(&mix_regs->func_stat);
	while (!(val & SRC_MIX_SLICE_FUNC_STAT_RST_STAT)) {
		val = readl(&mix_regs->func_stat);
	}

	/* Because CPUWAIT is default set, so M33 won't run, Clear it when kick M33 */
	/* Release Sentinel TROUT */
	ahab_release_m33_trout();

	/* Mask WDOG1 IRQ from A55, we use it for M33 reset */
	setbits_le32(&s_regs->ca55_irq_mask[1], BIT(6));

	/* Turn on WDOG1 clock */
	ccm_lpcg_on(CCGR_WDG1, 1);

	/* Set sentinel LP handshake for M33 reset */
	setbits_le32(&s_regs->lp_handshake[0], BIT(6));

	/* Clear M33 TCM for ECC */
	memset((void *)(ulong)0x201e0000, 0, 0x40000);

	return 0;
}
