// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2023 NXP
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
#include <dm/device.h>
#include <env.h>
#include <env_internal.h>
#include <errno.h>
#include <fdt_support.h>
#include <linux/bitops.h>
#include <linux/bitfield.h>
#include <asm/setup.h>
#include <asm/bootm.h>
#include <asm/arch-imx/cpu.h>
#include <asm/mach-imx/ele_api.h>
#include <asm/mach-imx/optee.h>
#include <linux/delay.h>
#include <fuse.h>
#include <imx_thermal.h>
#include <thermal.h>
#include <imx_sip.h>
#include <linux/arm-smccc.h>
#include <asm/arch/ddr.h>
#ifdef CONFIG_SCMI_FIRMWARE
#include <scmi_agent.h>
#include <scmi_protocols.h>
#include <scmi_nxp_protocols.h>
#include <dt-bindings/power/fsl,imx95-power.h>
#endif
#include <spl.h>
#include <mmc.h>
#include <kaslr.h>


DECLARE_GLOBAL_DATA_PTR;

rom_passover_t rom_passover_data = {0};

uint32_t scmi_get_rom_data(rom_passover_t *rom_data)
{
	/* Read ROM passover data */
	struct scmi_rom_passover_get_out out;
	struct scmi_msg msg = SCMI_MSG(SCMI_PROTOCOL_ID_MISC, SCMI_MISC_ROM_PASSOVER_GET, out);
	int ret;

	ret = devm_scmi_process_msg(gd->arch.scmi_dev, &msg);
	if(ret == 0 && out.status == 0) {
		memcpy(rom_data, (struct rom_passover_t *)out.passover, sizeof(rom_passover_t));
	} else {
		printf("Failed to get ROM passover data, scmi_err = %d, size_of(out) = %ld\n",
		       out.status, sizeof(out));
		return -EINVAL;
	}

	return 0;
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
	int ret;
	u16 boot_type;
	u8 boot_instance;

	volatile gd_t *pgd = gd;
	rom_passover_t *rdata;
#ifdef CONFIG_SPL_BUILD
	rdata = &rom_passover_data;
#else
	rom_passover_t rom_data = {0};
	if (!pgd->reloc_off) {
		rdata = &rom_data;
	} else
		rdata = &rom_passover_data;
#endif
	if (rdata->tag == 0) {
		ret = scmi_get_rom_data(rdata);
		if (ret != 0) {
			puts("SCMI: failure at rom_boot_info\n");
			return CONFIG_SYS_MMC_ENV_DEV;
		}
	}
	boot_type = rdata->boot_dev_type;
	boot_instance = rdata->boot_dev_inst;
	set_gd(pgd);

	debug("boot_type %d, instance %d\n", boot_type, boot_instance);

	/* If not boot from sd/mmc, use default value */
	if (boot_type != BOOT_TYPE_SD && boot_type != BOOT_TYPE_MMC)
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
	int ret;
	u32 val, word, offset;

	word = 17;
	offset = 14;

	ret = fuse_read((word / 8), (word % 8), &val);
	if (ret)
		val = 0; /* If read fuse failed, return as blank fuse */

	val >>= offset;
	val &= 0xf;

	max_speed = 2300000000;
	speed = max_speed - val * 100000000;

	if (is_imx95())
		max_speed = 2000000000;

	/* In case the fuse of speed grade not programmed */
	if (speed > max_speed)
		speed = max_speed;

	return speed;
}

u32 get_cpu_temp_grade(int *minc, int *maxc)
{
	int ret;
	u32 val, word, offset;

	word = 17;
	offset = 12;

	ret = fuse_read((word / 8), (word % 8), &val);
	if (ret)
		val = 0; /* If read fuse failed, return as blank fuse */

	val >>= offset;
	val &= 0x3;

	if (minc && maxc) {
		if (val == TEMP_AUTOMOTIVE) {
			*minc = -40;
			*maxc = 125;
		} else if (val == TEMP_INDUSTRIAL) {
			*minc = -40;
			*maxc = 105;
		} else if (val == TEMP_EXTCOMMERCIAL) {
			*minc = -20;
			*maxc = 105;
		} else {
			*minc = 0;
			*maxc = 95;
		}
	}
	return val;
}

static void set_cpu_info(struct ele_get_info_data *info)
{
	gd->arch.soc_rev = info->soc;
	gd->arch.lifecycle = info->lc;
	memcpy((void *)&gd->arch.uid, &info->uid, 4 * sizeof(u32));
}

u32 get_cpu_rev(void)
{
	u32 rev = (gd->arch.soc_rev >> 24) - 0xa0;

	return (MXC_CPU_IMX95 << 12) | (CHIP_REV_1_0 + rev);
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

static struct mm_region imx9_mem_map[] = {
	{
		/* M7 TCM */
		.virt = 0x203c0000UL,
		.phys = 0x203c0000UL,
		.size = 0x80000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN
	},
	{
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
		.size = 0x8000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN
	}, {
		/* DRAM1 */
		.virt = PHYS_SDRAM,
		.phys = PHYS_SDRAM,
		.size = PHYS_SDRAM_SIZE,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
#ifdef CONFIG_IMX_TRUSTY_OS
			 PTE_BLOCK_INNER_SHARE
#else
			 PTE_BLOCK_OUTER_SHARE
#endif
	}, {
#ifdef PHYS_SDRAM_2_SIZE
		/* DRAM2 */
		.virt = 0x100000000UL,
		.phys = 0x100000000UL,
		.size = PHYS_SDRAM_2_SIZE,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
			 PTE_BLOCK_OUTER_SHARE
	}, {
#endif
		/* empty entry to split table entry 5 if needed when TEEs are used */
		0,
	}, {
		/* List terminator */
		0,
	}
};

struct mm_region *mem_map = imx9_mem_map;

static unsigned int imx9_find_dram_entry_in_mem_map(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(imx9_mem_map); i++)
		if (imx9_mem_map[i].phys == CFG_SYS_SDRAM_BASE)
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
	 * imx9_mem_map for DRAM1
	 */
	int entry = imx9_find_dram_entry_in_mem_map();
	u64 attrs = imx9_mem_map[entry].attrs;

	while (i < CONFIG_NR_DRAM_BANKS &&
	       entry < ARRAY_SIZE(imx9_mem_map)) {
		if (gd->bd->bi_dram[i].start == 0)
			break;
		imx9_mem_map[entry].phys = gd->bd->bi_dram[i].start;
		imx9_mem_map[entry].virt = gd->bd->bi_dram[i].start;
		imx9_mem_map[entry].size = gd->bd->bi_dram[i].size;
		imx9_mem_map[entry].attrs = attrs;
		debug("Added memory mapping (%d): %llx %llx\n", entry,
		      imx9_mem_map[entry].phys, imx9_mem_map[entry].size);
		i++; entry++;
	}

	icache_enable();
	dcache_enable();
}

__weak int board_phys_sdram_size(phys_size_t *size){

	phys_size_t start, end;
	phys_size_t val;

	if (!size)
		return -EINVAL;

	val = readl(REG_DDR_CS0_BNDS);
	start = (val >> 16) << 24;
	end   = (val & 0xFFFF);
	end   = end ? end + 1 : 0;
	end   = end << 24;
	*size = end - start;

	val = readl(REG_DDR_CS1_BNDS);
	start = (val >> 16) << 24;
	end   = (val & 0xFFFF);
	end   = end ? end + 1 : 0;
	end   = end << 24;
	*size += end - start;

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
	if (rom_pointer[1] && (PHYS_SDRAM < (phys_addr_t)rom_pointer[0]))
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
		sdram_b1_size = 0x100000000UL - PHYS_SDRAM;
		sdram_b2_size = sdram_size - sdram_b1_size;
	} else {
		sdram_b1_size = sdram_size;
		sdram_b2_size = 0;
	}

	gd->bd->bi_dram[bank].start = PHYS_SDRAM;
	if (rom_pointer[1] && (PHYS_SDRAM < (phys_addr_t)rom_pointer[0])) {
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
			sdram_b1_size = 0x100000000UL - PHYS_SDRAM;
		} else {
			sdram_b1_size = sdram_size;
		}

		if (rom_pointer[1]) {
			/* We will relocate u-boot to Top of dram1. Tee position has three cases:
			 * 1. At the top of dram1,  Then return the size removed optee size.
			 * 2. In the middle of dram1, return the size of dram1.
			 * 3. Not in the scope of dram1, return the size of dram1.
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
	int ret, num_of_macs;

	ret = fuse_read(40, 5, &val[0]);
	if (ret)
		goto err;

	ret = fuse_read(40, 6, &val[1]);
	if (ret)
		goto err;

	num_of_macs = (val[1] >> 24) & 0xff;
	if (num_of_macs <= (dev_id * 3)) {
		printf("WARNING: no MAC address assigned for MAC%d\n", dev_id);
		goto err;
	}

	mac[0] = val[0] & 0xff;
	mac[1] = (val[0] >> 8) & 0xff;
	mac[2] = (val[0] >> 16) & 0xff;
	mac[3] = (val[0] >> 24) & 0xff;
	mac[4] = val[1] & 0xff;
	mac[5] = (val[1] >> 8) & 0xff;
	if (dev_id == 1)
		mac[5] = mac[5] + 3;
	if (dev_id == 2)
		mac[5] = mac[5] + 6;

	debug("%s: MAC%d: %02x.%02x.%02x.%02x.%02x.%02x\n",
	      __func__, dev_id, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	return;
err:
	memset(mac, 0, 6);
	printf("%s: fuse read err: %d\n", __func__, ret);
}

static char *rst_string[32] = {
	"cm33_lockup",
	"cm33_swreq",
	"cm7_lockup",
	"cm7_swreq",
	"fccu",
	"jtag_sw",
	"ele",
	"tempsense",
	"wdog1",
	"wdog2",
	"wdog3",
	"wdog4",
	"wdog5",
	"jtag",
	"cm33_exc",
	"bbm",
	"sw",
	"unused", "unused", "unused", "unused", "unused", "unused",
	"unused", "unused", "unused", "unused", "unused", "unused",
	"unused", "unused",
	"por"
};

int get_reset_reason(bool sys, bool lm)
{
	struct scmi_imx_misc_reset_reason_in in = {
		.flags = MISC_REASON_FLAG_SYSTEM,
	};

	struct scmi_imx_misc_reset_reason_out out = { 0 };
	struct scmi_msg msg = SCMI_MSG_IN(SCMI_IMX_PROTOCOL_ID_MISC,
					  SCMI_IMX_MISC_RESET_REASON,
					  in, out);
	int ret;

	if (sys) {
		ret = devm_scmi_process_msg(gd->arch.scmi_dev, &msg);
		if (out.status) {
			printf("%s:%d for SYS\n", __func__, out.status);
			return ret;
		}

		if (out.bootflags & MISC_BOOT_FLAG_VLD) {
			printf("SYS Boot reason: %s, origin: %ld, errid: %ld\n",
			       rst_string[out.bootflags & MISC_BOOT_FLAG_REASON],
			       out.bootflags & MISC_BOOT_FLAG_ORG_VLD ?
			       FIELD_GET(MISC_BOOT_FLAG_ORIGIN, out.bootflags) : -1,
			       out.bootflags & MISC_BOOT_FLAG_ERR_VLD ?
			       FIELD_GET(MISC_BOOT_FLAG_ERR_ID, out.bootflags) : -1
			       );
		}
		if (out.bootflags & MISC_SHUTDOWN_FLAG_VLD) {
			printf("SYS shutdown reason: %s, origin: %ld, errid: %ld\n",
			       rst_string[out.bootflags & MISC_SHUTDOWN_FLAG_REASON],
			       out.bootflags & MISC_SHUTDOWN_FLAG_ORG_VLD ?
			       FIELD_GET(MISC_SHUTDOWN_FLAG_ORIGIN, out.bootflags) : -1,
			       out.bootflags & MISC_SHUTDOWN_FLAG_ERR_VLD ?
			       FIELD_GET(MISC_SHUTDOWN_FLAG_ERR_ID, out.bootflags) : -1
			       );
		}
	}

	if (lm) {
		in.flags = 0;
		memset(&out, 0, sizeof(struct scmi_imx_misc_reset_reason_out));

		ret = devm_scmi_process_msg(gd->arch.scmi_dev, &msg);
		if (out.status) {
			printf("%s:%d for LM\n", __func__, out.status);
			return ret;
		}

		if (out.bootflags & MISC_BOOT_FLAG_VLD) {
			printf("LM Boot reason: %s, origin: %ld, errid: %ld\n",
			       rst_string[out.bootflags & MISC_BOOT_FLAG_REASON],
			       out.bootflags & MISC_BOOT_FLAG_ORG_VLD ?
			       FIELD_GET(MISC_BOOT_FLAG_ORIGIN, out.bootflags) : -1,
			       out.bootflags & MISC_BOOT_FLAG_ERR_VLD ?
			       FIELD_GET(MISC_BOOT_FLAG_ERR_ID, out.bootflags) : -1
			       );
		}

		if (out.bootflags & MISC_SHUTDOWN_FLAG_VLD) {
			printf("LM shutdown reason: %s, origin: %ld, errid: %ld\n",
			       rst_string[out.bootflags & MISC_SHUTDOWN_FLAG_REASON],
			       out.bootflags & MISC_SHUTDOWN_FLAG_ORG_VLD ?
			       FIELD_GET(MISC_SHUTDOWN_FLAG_ORIGIN, out.bootflags) : -1,
			       out.bootflags & MISC_SHUTDOWN_FLAG_ERR_VLD ?
			       FIELD_GET(MISC_SHUTDOWN_FLAG_ERR_ID, out.bootflags) : -1
			       );
		}
	}

	return 0;
}

/* name: SM CFG name */
int power_on_m7(char *name)
{
	struct scmi_imx_misc_cfg_info_out out = { 0 };
	struct scmi_msg msg = SCMI_MSG(SCMI_IMX_PROTOCOL_ID_MISC,
				       SCMI_IMX_MISC_CFG_INFO, out);
	int ret;

	ret = devm_scmi_process_msg(gd->arch.scmi_dev, &msg);
	if (out.status) {
		printf("%s:%d fail\n", __func__, out.status);
		return ret;
	}

	if (strncmp(out.cfgname, name, MISC_MAX_CFGNAME)) {
		printf("cfg name not match %s:%s, ignore\n", name, out.cfgname);
		return -EINVAL;
	}

	/* Power up M7MIX */
	ret = scmi_pwd_state_set(gd->arch.scmi_dev, 0, IMX95_PD_M7, 0);
	if (ret) {
		printf("Power M7 failed\n");
		return -EIO;
	}

	/* In case OEI not init ECC, do it here */
	memset_io((void *)0x203c0000, 0, 0x40000);
	memset_io((void *)0x20400000, 0, 0x40000);

	return 0;
}

const char *get_imx_type(u32 imxtype)
{
	switch (imxtype) {
	case MXC_CPU_IMX95:
		return "95";/* iMX95 FULL */
	default:
		return "??";
	}
}

int print_cpuinfo(void)
{
	u32 cpurev, max_freq;
	int minc, maxc;

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

#if defined(CONFIG_DM_THERMAL)
	struct udevice *udev;
	int ret, temp;

	if (IS_ENABLED(CONFIG_IMX_TMU))
		ret = uclass_get_device_by_name(UCLASS_THERMAL, "cpu-thermal", &udev);
	else
		ret = uclass_get_device(UCLASS_THERMAL, 0, &udev);
	if (!ret) {
		ret = thermal_get_temp(udev, &temp);

		if (!ret)
			printf(" at %dC", temp / 100);
		else
			debug(" - invalid sensor data\n");
	} else {
		debug(" - invalid sensor device\n");
	}
#endif
	puts("\n");

	get_reset_reason(false, true);

	return 0;
}

void build_info(void)
{
	u32 fw_version, sha1, res = 0, status;
	int ret;
	struct scmi_imx_misc_build_info_out out = { 0 };
	struct scmi_msg msg = SCMI_MSG(SCMI_IMX_PROTOCOL_ID_MISC,
				       SCMI_IMX_MISC_BUILD_INFO, out);

	printf("\nBuildInfo:\n");

	ret = devm_scmi_process_msg(gd->arch.scmi_dev, &msg);
	if (ret || out.status)
		printf("%s:%d:%d fail to get build info\n", __func__, ret, out.status);
	else
		printf("  - SM firmware Build %u, Commit %8x, %s %s\n", out.buildnum,
		       out.buildcommit, out.builddate, out.buildtime);

	ret = ele_get_fw_status(&status, &res);
	if (ret) {
		printf("  - ELE firmware status failed %d, 0x%x\n", ret, res);
	} else if ((status & 0xff) == 1) {
		ret = ele_get_fw_version(&fw_version, &sha1, &res);
		if (ret) {
			printf("  - ELE firmware version failed %d, 0x%x\n", ret, res);
		} else {
			printf("  - ELE firmware version %u.%u.%u-%x",
			       (fw_version & (0x00ff0000)) >> 16,
			       (fw_version & (0x0000fff0)) >> 4,
			       (fw_version & (0x0000000f)), sha1);
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

static int get_cooling_device_list(void * blob, u32 nodeoff, const char *const path, u32* cooling_dev, int max_cnt)
{
		int cnt, j;

		cnt = fdtdec_get_int_array_count(blob, nodeoff, "cooling-device", cooling_dev, max_cnt);
		if (cnt < 0) {
			printf("cnt incorrect, path %s, cnt = %d\n", path, cnt);
			return cnt;
		}
		if (cnt != max_cnt)
			printf("Warning: %s, cooling-device count %d\n", path, cnt);

		for (j = 0; j < cnt; j++)
			cooling_dev[j] = cpu_to_fdt32(cooling_dev[j]);

		return cnt;
}

static void disable_thermal_vpu_node(void *blob, u32 disabled_cores, u32 gpu_disabled)
{
	static const char * const thermal_path[] = {
		"/thermal-zones/ana/cooling-maps/map0"
	};
	u32 cooling_dev[24 - (disabled_cores * 3) - (gpu_disabled * 3)];
	u32 array_cnt = 24 - (disabled_cores * 3) - (gpu_disabled * 3);

	int nodeoff, ret, i;

	for (i = 0; i < ARRAY_SIZE(thermal_path); i++) {
		nodeoff = fdt_path_offset(blob, thermal_path[i]);
		if (nodeoff < 0) {
			printf("path not found %s\n", thermal_path[i]);
			continue; /* Not found, skip it */
		}
		get_cooling_device_list(blob, nodeoff, thermal_path[i], cooling_dev, array_cnt);

		/* Remove  VPU it the last two nodes in the fdt ana blob */
		ret = fdt_setprop(blob, nodeoff, "cooling-device", &cooling_dev,
				  sizeof(u32) * (array_cnt - 3));

		if (ret < 0) {
			printf("Warning: %s, cooling-device setprop failed %d\n",
			       thermal_path[i], ret);
			continue;
		}

		printf("Update node %s, cooling-device prop\n", thermal_path[i]);
	}
}

static void disable_thermal_gpu_node(void *blob, u32 disabled_cores)
{
	static const char * const thermal_path[] = {
		"/thermal-zones/ana/cooling-maps/map0",
	};
	u32 cooling_dev[24 - (disabled_cores * 3)];
	u32 array_cnt = 24 - (disabled_cores * 3);
	int nodeoff, ret, i;

	for (i = 0; i < ARRAY_SIZE(thermal_path); i++) {
		nodeoff = fdt_path_offset(blob, thermal_path[i]);
		if (nodeoff < 0) {
			printf("path not found %s\n", thermal_path[i]);
			continue; /* Not found, skip it */
		}
		get_cooling_device_list(blob, nodeoff, thermal_path[i], cooling_dev, array_cnt);

		/* Remove GPU and VPU as these are the last two nodes in the fdt ana blob */
		ret = fdt_setprop(blob, nodeoff, "cooling-device", &cooling_dev,
				  sizeof(u32) * (array_cnt - 6));

		if (ret < 0) {
			printf("Warning: %s, cooling-device setprop failed %d\n",
			       thermal_path[i], ret);
			continue;
		}

		/* Add VPU node back to ana thermal-zone. */
		ret = fdt_appendprop(blob, nodeoff, "cooling-device", &cooling_dev[array_cnt - 3],
				  sizeof(u32) * 3);

		if (ret < 0) {
			printf("Warning: %s, cooling-device appendprop failed %d\n",
			       thermal_path[i], ret);
			continue;
		}

		printf("Update node %s, cooling-device prop\n", thermal_path[i]);
	}
}

static void disable_thermal_cpu_nodes(void *blob, u32 disabled_cores)
{
	static const char * const thermal_path[] = {
		"/thermal-zones/pf53_arm/cooling-maps/map0",
		"/thermal-zones/ana/cooling-maps/map0",
		"/thermal-zones/a55/cooling-maps/map0",
	};
	u32 cooling_dev[24];

	int nodeoff, ret, i, cnt;

	for (i = 0; i < ARRAY_SIZE(thermal_path); i++) {
		nodeoff = fdt_path_offset(blob, thermal_path[i]);
		if (nodeoff < 0) {
			printf("path not found %s\n", thermal_path[i]);
			continue; /* Not found, skip it */
		}
		cnt = get_cooling_device_list(blob, nodeoff, thermal_path[i], cooling_dev, 24);

		ret = fdt_setprop(blob, nodeoff, "cooling-device", &cooling_dev,
				  sizeof(u32) * (18 - disabled_cores * 3));

		if (ret < 0) {
			printf("Warning: %s, cooling-device setprop failed %d\n",
			       thermal_path[i], ret);
			continue;
		}

		/* Add GPU and VPU nodes back to ana thermal-zone. */
		if (cnt > 18)
			ret = fdt_appendprop(blob, nodeoff, "cooling-device", &cooling_dev[18],
					  sizeof(u32) * 6);

		if (ret < 0) {
			printf("Warning: %s, cooling-device appendprop failed %d\n",
			       thermal_path[i], ret);
			continue;
		}

		printf("Update node %s, cooling-device prop\n", thermal_path[i]);
	}
}

static int disable_npu_node(void *blob)
{
	static const char * const nodes_path_npu[] = {
		"/soc/imx95-neutron-remoteproc@4ab00000",
		"/soc/imx95-neutron@4ab00004",
	};

	return delete_fdt_nodes(blob, nodes_path_npu, ARRAY_SIZE(nodes_path_npu));
}

static int disable_cpu_nodes(void *blob, u32 disabled_cores)
{
	u32 i = 0;
	int rc;
	int nodeoff;
	char nodes_path[32];

	printf("disable_cpu_nodes, num_disabled_cores = %d\n", disabled_cores);
	for (i = 6; i > (6 - disabled_cores); i--) {

		sprintf(nodes_path, "/cpus/cpu@%u00", i - 1);

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

static int disable_jpegdec_node(void *blob)
{
	static const char * const nodes_path_jpegdec[] = {
		"/soc/jpegdec@4c500000",
	};

	return delete_fdt_nodes(blob, nodes_path_jpegdec, ARRAY_SIZE(nodes_path_jpegdec));
}

static int disable_jpegenc_node(void *blob)
{
	static const char * const nodes_path_jpegenc[] = {
		"/soc/jpegenc@4c550000",
	};

	return delete_fdt_nodes(blob, nodes_path_jpegenc, ARRAY_SIZE(nodes_path_jpegenc));
}

static int disable_mipicsi0_node(void *blob)
{
	static const char * const nodes_path_mipicsi0[] = {
		"/soc/csi@4ad30000",
	};

	return delete_fdt_nodes(blob, nodes_path_mipicsi0, ARRAY_SIZE(nodes_path_mipicsi0));
}

static int disable_mipicsi1_node(void *blob)
{
	static const char * const nodes_path_mipicsi1[] = {
		"/soc/csi@4ad40000",
	};

	return delete_fdt_nodes(blob, nodes_path_mipicsi1, ARRAY_SIZE(nodes_path_mipicsi1));
}


static int disable_isp_node(void *blob)
{
	static const char * const nodes_path_isp[] = {
		"/soc@0/isp@4ae00000",
		"/soc/isp@4ae00000",
	};

	return delete_fdt_nodes(blob, nodes_path_isp, ARRAY_SIZE(nodes_path_isp));
}

static int disable_vpu_node(void *blob, u32 num_a55_cores_disabled, u32 gpu_disabled)
{
	uint32_t ret = 0;

	printf("Disable VPU nodes\n");
	static const char * const nodes_path_vpu[] = {
		"/soc/vpu-ctrl@4c4c0000",
		"/soc/vpu@4c480000",
		"/soc/vpu@4c490000",
		"/soc/vpu@4c4a0000",
		"/soc/vpu@4c4b0000",
		"/soc/jpegdec@4c500000",
		"/soc/jpegenc@4c550000",
		"/soc/syscon@4c410000"
	};

	ret = delete_fdt_nodes(blob, nodes_path_vpu, ARRAY_SIZE(nodes_path_vpu));
	disable_thermal_vpu_node(blob, num_a55_cores_disabled, gpu_disabled);
	return ret;
}

static int disable_gpu_node(void *blob, uint32_t num_a55_cores_disabled)
{
	uint32_t ret = 0;

	static const char * const nodes_path_gpu[] = {
		"/soc/gpu@4d900000",
		"/thermal-zones@1/ana/cooling-maps/map1/cooling-device/gpu@4d900000",
		"/thermal-zones/ana/cooling-maps/map1/cooling-device/gpu@4d900000",
		"/thermal-zones@1/ana/cooling-maps/map1/cooling-device",
		"/thermal-zones/ana/cooling-maps/map1/cooling-device",
		"/thermal-zones@1/ana/cooling-maps/map1",
		"/thermal-zones/ana/cooling-maps/map1",
	};

	ret = delete_fdt_nodes(blob, nodes_path_gpu, ARRAY_SIZE(nodes_path_gpu));
	disable_thermal_gpu_node(blob, num_a55_cores_disabled);
	return ret;
}

static int disable_pciea_node(void *blob)
{
	static const char * const nodes_path_pciea[] = {
		"/soc/pcie@4c300000"
	};

	return delete_fdt_nodes(blob, nodes_path_pciea, ARRAY_SIZE(nodes_path_pciea));
}

static int disable_pcieb_node(void *blob)
{
	static const char * const nodes_path_pcieb[] = {
		"/soc/pcie@4c380000"
	};

	return delete_fdt_nodes(blob, nodes_path_pcieb, ARRAY_SIZE(nodes_path_pcieb));
}

static int disable_m7_node(void *blob)
{
	static const char * const nodes_path_m7[] = {
		"/imx95-cm7"
	};

	return delete_fdt_nodes(blob, nodes_path_m7, ARRAY_SIZE(nodes_path_m7));
}

static bool is_m7_off(void)
{
	u32 state = 0;
	int ret;
	ret = scmi_pwd_state_get(gd->arch.scmi_dev, IMX95_PD_M7, &state);
	if (ret)
		printf("scmi_pwd_state_get Failed %d for M7\n", ret);

	if (state == BIT(30))
		return true;
	else
		return false;
}

int disable_enet10g_node(void *blob)
{
	static const char * const nodes_path_enet10g[] = {
		"/pcie@4ca00000/ethernet@10,0",
		"/soc/pcie@4ca00000/ethernet@10,0",
		"/soc/syscon@4ca00000/ethernet@10,0",
	};

	return delete_fdt_nodes(blob, nodes_path_enet10g, ARRAY_SIZE(nodes_path_enet10g));
}


int disable_mipidsi_node(void *blob)
{
	static const char * const nodes_path_mipidsi[] = {
		"/soc/dsi@4acf0000",
		"/soc/syscon@4acf0000",
	};

	return delete_fdt_nodes(blob, nodes_path_mipidsi, ARRAY_SIZE(nodes_path_mipidsi));
}


int disable_lvds_node(void *blob)
{
	static const char * const nodes_path_lvds[] = {
		"/soc/syscon@4b0c0000/ldb@4/channel@0",
		"/soc/syscon@4b0c0000/phy@8",
		"/soc/syscon@4b0c0000/ldb@4/channel@1",
		"/soc/syscon@4b0c0000/phy@c",
	};

	return delete_fdt_nodes(blob, nodes_path_lvds, ARRAY_SIZE(nodes_path_lvds));
}


int ft_system_setup(void *blob, struct bd_info *bd)
{
	u32 val = 0;
	int ret = 0;
	int num_a55_cores_disabled = 0;
	int gpu_disabled = 0;

	if (is_imx95()) {
		fuse_read(2, 2, &val);

		if (val & BIT(0)) /* NPU */
			disable_npu_node(blob);

		if (val & BIT(3)) /* A55C4 */
			num_a55_cores_disabled++;

		if (val & BIT(4)) /* A55C5 */
			num_a55_cores_disabled++;

		if (val & BIT(5)) /* A55C4 */
			num_a55_cores_disabled++;

		if (val & BIT(6)) /* A55C5 */
			num_a55_cores_disabled++;

		if (num_a55_cores_disabled > 0)
			disable_cpu_nodes(blob, num_a55_cores_disabled);

		if (val & BIT(27)) /* LVDS */
			disable_lvds_node(blob);

		if (val & BIT(29)) /* ISP */
			disable_isp_node(blob);

		/* Disable devices based on fuse*/
		val = 0x0;

		fuse_read(2, 3, &val);

		if (val & BIT(6)) /* PCIE A */
			disable_pciea_node(blob);
		if (val & BIT(7)) /* PCIE B */
			disable_pcieb_node(blob);

		if (val & BIT(17)) { /* GPU MIX */
			disable_gpu_node(blob, num_a55_cores_disabled);
			gpu_disabled = 1;
		}
		if (val & BIT(18)) /* VPU MIX */
			disable_vpu_node(blob, num_a55_cores_disabled, gpu_disabled);
		if (val & BIT(19)) /* JPEGDEC disable */
			disable_jpegenc_node(blob);
		if (val & BIT(20)) /* JPEGENC disable */
			disable_jpegdec_node(blob);

		if (val & BIT(22)) /* MIPI-CSI0 */
			disable_mipicsi0_node(blob);
		if (val & BIT(23)) /* MIPI-CSI1 */
			disable_mipicsi1_node(blob);
		if (val & BIT(24)) /* MIPI-DSI MIX */
			disable_mipidsi_node(blob);

		val = 0x0;
		fuse_read(2, 4, &val);

		if (val & BIT(12)) /* Disable 10G */
			disable_enet10g_node(blob);
	}

	if (is_imx95() && is_m7_off()) {
		disable_m7_node(blob);
	}

	if (IS_ENABLED(CONFIG_KASLR)) {
		ret = do_generate_kaslr(blob);
		if (ret)
			printf("Unable to set property %s, err=%s\n",
				"kaslr-seed", fdt_strerror(ret));
	}

	return ft_add_optee_node(blob, bd);
}

/* Fix uboot dtb based on fuses. */
int board_fix_fdt_fuse(void *fdt)
{
	u32 val = 0;;

	fuse_read(2, 2, &val);

	if (val & BIT(27)) /* LVDS */
		disable_lvds_node(fdt);

	val = 0x0;
	fuse_read(2, 3, &val);

	if (val & BIT(24)) /* MIPI-DSI MIX */
		disable_mipidsi_node(fdt);

	val = 0x0;
	fuse_read(2, 4, &val);

	if (val & BIT(12)) /* Disable 10G */
		disable_enet10g_node(fdt);
	return 0;
}

#if defined(CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG)
void get_board_serial(struct tag_serialnr *serialnr)
{
	printf("UID: %08x%08x%08x%08x\n", __be32_to_cpu(gd->arch.uid[0]),
	       __be32_to_cpu(gd->arch.uid[1]), __be32_to_cpu(gd->arch.uid[2]),
	       __be32_to_cpu(gd->arch.uid[3]));

	serialnr->low = __be32_to_cpu(gd->arch.uid[1]);
	serialnr->high = __be32_to_cpu(gd->arch.uid[0]);
}
#endif

static void gpio_reset(ulong gpio_base)
{
	writel(0, gpio_base + 0x10);
	writel(0, gpio_base + 0x14);
	writel(0, gpio_base + 0x18);
	writel(0, gpio_base + 0x1c);
}

int arch_cpu_init(void)
{
	if (IS_ENABLED(CONFIG_SPL_BUILD)) {
		disable_wdog((void __iomem *)WDG3_BASE_ADDR);
		disable_wdog((void __iomem *)WDG4_BASE_ADDR);

		clock_init_early();

		gpio_reset(GPIO2_BASE_ADDR);
		gpio_reset(GPIO3_BASE_ADDR);
		gpio_reset(GPIO4_BASE_ADDR);
		gpio_reset(GPIO5_BASE_ADDR);
	}

	return 0;
}

int imx9_probe_mu(void)
{
	struct udevice *dev;
	int ret;
	u32 res;
	struct ele_get_info_data info;

	ret = uclass_get_device_by_driver(UCLASS_SCMI_AGENT, DM_DRIVER_GET(scmi_mbox), &dev);
	if (ret)
		return ret;

	ret = uclass_get_device_by_name(UCLASS_CLK, "protocol@14", &dev);
	if (ret)
		return ret;

	ret = devm_scmi_of_get_channel(dev);
	if (ret)
		return ret;

	gd->arch.scmi_dev = dev;

	ret = uclass_get_device_by_name(UCLASS_PINCTRL, "protocol@19", &dev);
	if (ret)
		return ret;

#if defined(CONFIG_IMX_TRUSTY_OS) && defined(CONFIG_SPL_BUILD)
	ret = uclass_get_device_by_name(UCLASS_MISC, "mailbox@47530000", &dev);
#else
	ret = uclass_get_device_by_name(UCLASS_MISC, "mailbox@47550000", &dev);
#endif
	if (ret)
		return ret;

	if (gd->flags & GD_FLG_RELOC)
		return 0;

	ret = ele_get_info(&info, &res);
	if (ret)
		return ret;

	set_cpu_info(&info);

	return 0;
}
EVENT_SPY_SIMPLE(EVT_DM_POST_INIT_F, imx9_probe_mu);
EVENT_SPY_SIMPLE(EVT_DM_POST_INIT_R, imx9_probe_mu);

int timer_init(void)
{
	gd->arch.tbl = 0;
	gd->arch.tbu = 0;

#ifdef CONFIG_SPL_BUILD
	unsigned long freq = 24000000;
	asm volatile("msr cntfrq_el0, %0" : : "r" (freq) : "memory");

	/* Clear the compare frame interrupt */
	unsigned long sctr_cmpcr_addr = SYSCNT_CMP_BASE_ADDR + 0x2c;
	unsigned long sctr_cmpcr = readl(sctr_cmpcr_addr);

	sctr_cmpcr &= ~0x1;
	writel(sctr_cmpcr, sctr_cmpcr_addr);
#endif

	return 0;
}

enum env_location env_get_location(enum env_operation op, int prio)
{
	enum boot_device dev = get_boot_device();
	enum env_location env_loc = ENVL_UNKNOWN;

	if (prio)
		return env_loc;

	switch (dev) {
#if defined(CONFIG_ENV_IS_IN_SPI_FLASH)
	case QSPI_BOOT:
		env_loc = ENVL_SPI_FLASH;
		break;
#endif
#if defined(CONFIG_ENV_IS_IN_MMC)
	case SD1_BOOT:
	case SD2_BOOT:
	case SD3_BOOT:
	case MMC1_BOOT:
	case MMC2_BOOT:
	case MMC3_BOOT:
	case FLEXSPI_NAND_BOOT:
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

enum imx9_soc_voltage_mode soc_target_voltage_mode(void)
{
	u32 speed = get_cpu_speed_grade_hz();
	enum imx9_soc_voltage_mode voltage = VOLT_OVER_DRIVE;

	if (is_imx95()) {
		if (speed == 2000000000)
			voltage = VOLT_SUPER_OVER_DRIVE;
		else if (speed == 1800000000)
			voltage = VOLT_OVER_DRIVE;
		else if (speed == 1400000000)
			voltage = VOLT_NOMINAL_DRIVE;
		else /* boot not support low drive mode according to AS */
			printf("Unexpected A55 freq %u, default to OD\n", speed);
	}

	return voltage;
}

#if IS_ENABLED(CONFIG_SCMI_FIRMWARE)
enum boot_device get_boot_device(void)
{
	volatile gd_t *pgd = gd;
	int ret;
	u16 boot_type;
	u8 boot_instance;
	enum boot_device boot_dev = 0;
	rom_passover_t *rdata;

#ifdef CONFIG_SPL_BUILD
	rdata = &rom_passover_data;
#else
	rom_passover_t rom_data = {0};

	if (pgd->reloc_off == 0)
		rdata = &rom_data;
	else
		rdata = &rom_passover_data;
#endif
	if (rdata->tag == 0) {
		ret = scmi_get_rom_data(rdata);
		if (ret != 0) {
			puts("SCMI: failure at rom_boot_info\n");
			return -1;
		}
	}
	boot_type = rdata->boot_dev_type;
	boot_instance = rdata->boot_dev_inst;

	set_gd(pgd);

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
#ifdef CONFIG_IMX95
		boot_dev -= 3; //iMX95 usb instance start at 3
#endif
		break;
	default:
		break;
	}

	return boot_dev;
}
#endif

ulong h_spl_load_read(struct spl_load_info *load, ulong off,
		      ulong size, void *buf)
{
	struct blk_desc *bd = load->priv;
	lbaint_t sector = off >> bd->log2blksz;
	lbaint_t count = size >> bd->log2blksz;
	ulong trampoline_sz = SZ_16M;
	void *trampoline = (void *)((ulong)CFG_SYS_SDRAM_BASE + PHYS_SDRAM_SIZE - trampoline_sz);
	ulong ns_ddr_end = CFG_SYS_SDRAM_BASE + PHYS_SDRAM_SIZE;
	ulong read_count, trampoline_cnt = trampoline_sz >> bd->log2blksz, actual, total;

#ifdef PHYS_SDRAM_2_SIZE
	ns_ddr_end += PHYS_SDRAM_2_SIZE;
#endif

	/* Check if the buf is in non-secure world, otherwise copy from trampoline */
	if ((ulong)buf < CFG_SYS_SDRAM_BASE || (ulong)buf + (count * sector) > ns_ddr_end) {
		total = 0;
		while (count) {
			read_count = trampoline_cnt > count ? count : trampoline_cnt;
			actual = blk_dread(bd, sector, read_count, trampoline);
			if (actual != read_count) {
				printf("Error in blk_dread, %lu, %lu\n", read_count, actual);
				return 0;
			}
			memcpy(buf, trampoline, actual * 512);
			buf += actual * 512;
			sector += actual;
			total += actual;
			count -= actual;
		}

		return total << bd->log2blksz;
	}

	return blk_dread(bd, sector, count, buf) << bd->log2blksz;
}
