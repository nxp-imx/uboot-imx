/*
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
 */

#include <common.h>
#include <errno.h>
#include <asm/io.h>
#include <asm/mach-imx/sci/sci.h>
#include <asm/mach-imx/boot_mode.h>
#include <asm/arch/clock.h>
#include <asm/armv8/mmu.h>
#include <elf.h>
#include <asm/arch-imx/cpu.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/cpu.h>

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

	err = sc_misc_get_control(ipcHndl, SC_R_SYSTEM, SC_C_ID, &id);
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

const char *get_core_name(void)
{
	if (is_cortex_a53())
		return "A53";
	else if (is_cortex_a35())
		return "A35";
	else if (is_cortex_a72())
		return "A72";
	else
		return "?";
}


int print_cpuinfo(void)
{
	u32 cpurev;

	cpurev = get_cpu_rev();

	printf("CPU:   Freescale i.MX%s rev%s %s at %d MHz",
			get_imx8_type((cpurev & 0xFF000) >> 12),
			get_imx8_rev((cpurev & 0xFFF)),
			get_core_name(),
		mxc_get_clock(MXC_ARM_CLK) / 1000000);

	printf("\n");
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

#if defined(CONFIG_IMX8QM)
#define FUSE_MAC0_WORD0 452
#define FUSE_MAC0_WORD1 453
#define FUSE_MAC1_WORD0 454
#define FUSE_MAC1_WORD1 455
#elif defined(CONFIG_IMX8QXP)
#define FUSE_MAC0_WORD0 708
#define FUSE_MAC0_WORD1 709
#define FUSE_MAC1_WORD0 710
#define FUSE_MAC1_WORD1 711
#endif

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

enum boot_device get_boot_device(void)
{
	enum boot_device boot_dev = SD1_BOOT;

	sc_ipc_t ipcHndl = 0;
	sc_rsrc_t dev_rsrc;

	ipcHndl = gd->arch.ipc_channel_handle;
	sc_misc_get_boot_dev(ipcHndl, &dev_rsrc);

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

int print_bootinfo(void)
{
	enum boot_device bt_dev;
	bt_dev = get_boot_device();

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

#ifdef CONFIG_SERIAL_TAG
#define FUSE_UNIQUE_ID_WORD0 16
#define FUSE_UNIQUE_ID_WORD1 17
void get_board_serial(struct tag_serialnr *serialnr)
{
	sc_err_t err;
	sc_ipc_t ipc;
	uint32_t val1 = 0, val2 = 0;
	uint32_t word1, word2;

	ipc = gd->arch.ipc_channel_handle;

	word1 = FUSE_UNIQUE_ID_WORD0;
	word2 = FUSE_UNIQUE_ID_WORD1;

	err = sc_misc_otp_fuse_read(ipc, word1, &val1);
	if (err != SC_ERR_NONE) {
		printf("%s fuse %d read error: %d\n", __func__,word1, err);
		return;
	}

	err = sc_misc_otp_fuse_read(ipc, word2, &val2);
	if (err != SC_ERR_NONE) {
		printf("%s fuse %d read error: %d\n", __func__, word2, err);
		return;
	}
	serialnr->low = val1;
	serialnr->high = val2;
}
#endif /*CONFIG_SERIAL_TAG*/

#ifdef CONFIG_ENV_IS_IN_MMC
__weak int board_mmc_get_env_dev(int devno)
{
	return CONFIG_SYS_MMC_ENV_DEV;
}

int mmc_get_env_dev(void)
{
	sc_ipc_t ipcHndl = 0;
	sc_rsrc_t dev_rsrc;
	int devno;

	ipcHndl = gd->arch.ipc_channel_handle;
	sc_misc_get_boot_dev(ipcHndl, &dev_rsrc);

	switch(dev_rsrc) {
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
		return CONFIG_SYS_MMC_ENV_DEV;
	}

	return board_mmc_get_env_dev(devno);
}
#endif
