/* Copyright 2017 NXP
 *
 * Peng Fan <peng.fan@nxp.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */
#include <common.h>
#include <asm/arch/imx-regs.h>
#include <asm/io.h>
#include <asm/arch/clock.h>
#include <asm/arch/sys_proto.h>
#include <asm/imx-common/boot_mode.h>
#include <asm/armv8/mmu.h>
#include <errno.h>

DECLARE_GLOBAL_DATA_PTR;

int timer_init(void)
{
#ifdef CONFIG_SPL_BUILD
	void __iomem *sctr_base = (void __iomem *)SCTR_BASE_ADDR;
	unsigned long freq;
	u32 val;

	freq = readl(sctr_base + CNTFID0_OFF);

	/* Update with accurate clock frequency */
	asm volatile("msr cntfrq_el0, %0" : : "r" (freq) : "memory");

	val = readl(sctr_base + CNTCR_OFF);
	val &= ~(SC_CNTCR_FREQ0 | SC_CNTCR_FREQ1);
	val |= SC_CNTCR_FREQ0 | SC_CNTCR_ENABLE | SC_CNTCR_HDBG;
	writel(val, sctr_base + CNTCR_OFF);
#endif

	gd->arch.tbl = 0;
	gd->arch.tbu = 0;

	return 0;
}

void set_wdog_reset(struct wdog_regs *wdog)
{
	u32 reg = readw(&wdog->wcr);
	/*
	 * Output WDOG_B signal to reset external pmic or POR_B decided by
	 * the board desgin. Without external reset, the peripherals/DDR/
	 * PMIC are not reset, that may cause system working abnormal.
	 */
	reg = readw(&wdog->wcr);
	reg |= 1 << 3;
	/*
	 * WDZST bit is write-once only bit. Align this bit in kernel,
	 * otherwise kernel code will have no chance to set this bit.
	 */
	reg |= 1 << 0;
	writew(reg, &wdog->wcr);
}

#ifdef CONFIG_SPL_BUILD
void reset_cpu(ulong addr)
{
	/* TODO */
	printf("%s\n", __func__);
	while (1);
}
#endif

static struct mm_region imx8m_mem_map[] = {
	{
		.virt = 0x0UL,
		.phys = 0x0UL,
		.size = 0xB00000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
			 PTE_BLOCK_OUTER_SHARE
	}, {
		.virt = 0xB00000UL,
		.phys = 0xB00000UL,
		.size = 0x3f500000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN
	}, {
		.virt = 0x40000000UL,
		.phys = 0x40000000UL,
		.size = 0xC0000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
			 PTE_BLOCK_OUTER_SHARE
	}, {
		.virt = 0x100000000UL,
		.phys = 0x100000000UL,
		.size = 0x040000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
			 PTE_BLOCK_OUTER_SHARE
	}, {
		/* List terminator */
		0,
	}
};
struct mm_region *mem_map = imx8m_mem_map;

u32 get_cpu_rev(void)
{
	/* TODO: */
	return (MXC_CPU_IMX8MQ << 12) | (1 << 4);
}

void imx_set_wdog_powerdown(bool enable)
{
	struct wdog_regs *wdog1 = (struct wdog_regs *)WDOG1_BASE_ADDR;
	struct wdog_regs *wdog2 = (struct wdog_regs *)WDOG2_BASE_ADDR;
	struct wdog_regs *wdog3 = (struct wdog_regs *)WDOG3_BASE_ADDR;

	/* Write to the PDE (Power Down Enable) bit */
	writew(enable, &wdog1->wmcr);
	writew(enable, &wdog2->wmcr);
	writew(enable, &wdog3->wmcr);
}

int arch_cpu_init(void)
{
	/*
	 * Init timer at very early state, because sscg pll setting
	 * will use it
	 */
	timer_init();
	clock_init();

	imx_set_wdog_powerdown(false);

	return 0;
}

#if defined(CONFIG_FEC_MXC)
void imx_get_mac_from_fuse(int dev_id, unsigned char *mac)
{
	struct ocotp_regs *ocotp = (struct ocotp_regs *)OCOTP_BASE_ADDR;
	struct fuse_bank *bank = &ocotp->bank[9];
	struct fuse_bank9_regs *fuse =
		(struct fuse_bank9_regs *)bank->fuse_regs;

	u32 value = readl(&fuse->mac_addr1);
	mac[0] = (value >> 8);
	mac[1] = value;

	value = readl(&fuse->mac_addr0);
	mac[2] = value >> 24;
	mac[3] = value >> 16;
	mac[4] = value >> 8;
	mac[5] = value;
}
#endif

#ifdef CONFIG_IMX_BOOTAUX
#define M4RCR (0xC)
int arch_auxiliary_core_up(u32 core_id, ulong boot_private_data)
{
	u32 stack, pc;
	u32 val;

	if (!boot_private_data)
		return -EINVAL;

	stack = *(u32 *)boot_private_data;
	pc = *(u32 *)(boot_private_data + 4);

	/* Set the stack and pc to M4 bootROM */
	writel(stack, M4_BOOTROM_BASE_ADDR);
	writel(pc, M4_BOOTROM_BASE_ADDR + 4);

	/* Enable M4 */
	val = readl(SRC_BASE_ADDR + M4RCR);
	val &= ~SRC_SCR_M4C_NON_SCLR_RST_MASK;
	val |= SRC_SCR_M4_ENABLE_MASK;
	writel(val, SRC_BASE_ADDR + M4RCR);

	return 0;
}

int arch_auxiliary_core_check_up(u32 core_id)
{
	unsigned val;

	val = readl(SRC_BASE_ADDR + M4RCR);

	if (val & 0x00000001)
		return 0;  /* assert in reset */

	return 1;
}
#endif

enum boot_device get_boot_device(void)
{
	struct bootrom_sw_info **p =
		(struct bootrom_sw_info **)ROM_SW_INFO_ADDR;

	enum boot_device boot_dev = SD1_BOOT;
	u8 boot_type = (*p)->boot_dev_type;
	u8 boot_instance = (*p)->boot_dev_instance;

	switch (boot_type) {
	case BOOT_TYPE_SD:
		boot_dev = boot_instance + SD1_BOOT;
		break;
	case BOOT_TYPE_MMC:
		boot_dev = boot_instance + MMC1_BOOT;
		break;
	case BOOT_TYPE_NAND:
		boot_dev = NAND_BOOT;
		break;
	case BOOT_TYPE_QSPI:
		boot_dev = QSPI_BOOT;
		break;
	case BOOT_TYPE_WEIM:
		boot_dev = WEIM_NOR_BOOT;
		break;
	case BOOT_TYPE_SPINOR:
		boot_dev = SPI_NOR_BOOT;
		break;
	default:
		break;
	}

	return boot_dev;
}

#ifdef CONFIG_SERIAL_TAG
void get_board_serial(struct tag_serialnr *serialnr)
{
	/* TODO: */
	serialnr->low = 0;
	serialnr->high = 0;
}
#endif
