// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2013 Stefan Roese <sr@denx.de>
 * Copyright 2018 NXP
 */

#include <common.h>
#include <asm/arch/sys_proto.h>
#include <linux/errno.h>
#include <asm/io.h>
#include <asm/mach-imx/regs-common.h>
#include <fsl_caam.h>
#include <fdt_support.h>

DECLARE_GLOBAL_DATA_PTR;

/* 1 second delay should be plenty of time for block reset. */
#define	RESET_MAX_TIMEOUT	1000000

#define	MXS_BLOCK_SFTRST	(1 << 31)
#define	MXS_BLOCK_CLKGATE	(1 << 30)

int mxs_wait_mask_set(struct mxs_register_32 *reg, uint32_t mask, unsigned
								int timeout)
{
	while (--timeout) {
		if ((readl(&reg->reg) & mask) == mask)
			break;
		udelay(1);
	}

	return !timeout;
}

int mxs_wait_mask_clr(struct mxs_register_32 *reg, uint32_t mask, unsigned
								int timeout)
{
	while (--timeout) {
		if ((readl(&reg->reg) & mask) == 0)
			break;
		udelay(1);
	}

	return !timeout;
}

int mxs_reset_block(struct mxs_register_32 *reg)
{
	/* Clear SFTRST */
	writel(MXS_BLOCK_SFTRST, &reg->reg_clr);

	if (mxs_wait_mask_clr(reg, MXS_BLOCK_SFTRST, RESET_MAX_TIMEOUT))
		return 1;

	/* Clear CLKGATE */
	writel(MXS_BLOCK_CLKGATE, &reg->reg_clr);

	/* Set SFTRST */
	writel(MXS_BLOCK_SFTRST, &reg->reg_set);

	/* Wait for CLKGATE being set */
	if (mxs_wait_mask_set(reg, MXS_BLOCK_CLKGATE, RESET_MAX_TIMEOUT))
		return 1;

	/* Clear SFTRST */
	writel(MXS_BLOCK_SFTRST, &reg->reg_clr);

	if (mxs_wait_mask_clr(reg, MXS_BLOCK_SFTRST, RESET_MAX_TIMEOUT))
		return 1;

	/* Clear CLKGATE */
	writel(MXS_BLOCK_CLKGATE, &reg->reg_clr);

	if (mxs_wait_mask_clr(reg, MXS_BLOCK_CLKGATE, RESET_MAX_TIMEOUT))
		return 1;

	return 0;
}

static ulong get_sp(void)
{
	ulong ret;

	asm("mov %0, sp" : "=r"(ret) : );
	return ret;
}

void board_lmb_reserve(struct lmb *lmb)
{
	ulong sp, bank_end;
	int bank;

	sp = get_sp();
	debug("## Current stack ends at 0x%08lx ", sp);

	/* adjust sp by 16K to be safe */
	sp -= 4096 << 2;
	for (bank = 0; bank < CONFIG_NR_DRAM_BANKS; bank++) {
		if (sp < gd->bd->bi_dram[bank].start)
			continue;
		bank_end = gd->bd->bi_dram[bank].start +
			gd->bd->bi_dram[bank].size;
		if (sp >= bank_end)
			continue;
		lmb_reserve(lmb, sp, bank_end - sp);
		break;
	}
}

void configure_tzc380(void)
{
#if defined (IP2APB_TZASC1_BASE_ADDR)
	struct iomuxc *iomux = (struct iomuxc *)IOMUXC_BASE_ADDR;
	if (iomux->gpr[9] & 0x1)
		writel(0xf0000000, IP2APB_TZASC1_BASE_ADDR + 0x108);
#endif
#if defined (IP2APB_TZASC2_BASE_ADDR)
	if (iomux->gpr[9] & 0x2)
		writel(0xf0000000, IP2APB_TZASC2_BASE_ADDR + 0x108);
#endif
}

void imx_sec_init(void)
{
#if defined(CONFIG_SPL_BUILD) || !defined(CONFIG_SPL)
	caam_open();
#endif
}

static void set_dt_val(void *data, uint32_t cell_size, uint64_t val)
{
	if (cell_size == 1) {
		fdt32_t v = cpu_to_fdt32((uint32_t)val);

		memcpy(data, &v, sizeof(v));
	} else {
		fdt64_t v = cpu_to_fdt64(val);

		memcpy(data, &v, sizeof(v));
	}
}

int add_dt_path_subnode(void *fdt, const char *path, const char *subnode)
{
	int offs;

	offs = fdt_path_offset(fdt, path);
	if (offs < 0)
		return -1;

	offs = fdt_add_subnode(fdt, offs, subnode);
	if (offs < 0)
		return -1;
	return offs;
}

int add_res_mem_dt_node(void *fdt, const char *name, phys_addr_t pa,
			size_t size)
{
	int offs = 0;
	int ret = 0;
	int addr_size = -1;
	int len_size = -1;
	bool found = true;
	char subnode_name[80] = { 0 };

	offs = fdt_path_offset(fdt, "/reserved-memory");

	if (offs < 0) {
		found = false;
		offs = 0;
	}

	len_size = fdt_size_cells(fdt, offs);
	if (len_size < 0)
		return -1;
	addr_size = fdt_address_cells(fdt, offs);
	if (addr_size < 0)
		return -1;

	if (!found) {
		offs = add_dt_path_subnode(fdt, "/", "reserved-memory");
		if (offs < 0)
			return -1;

		ret = fdt_setprop_cell(fdt, offs, "#address-cells", addr_size);
		if (ret < 0)
			return -1;
		ret = fdt_setprop_cell(fdt, offs, "#size-cells", len_size);
		if (ret < 0)
			return -1;
		ret = fdt_setprop(fdt, offs, "ranges", NULL, 0);
		if (ret < 0)
			return -1;
	}

#ifdef CONFIG_PHYS_64BIT
	snprintf(subnode_name, sizeof(subnode_name), "%s@0x%llx", name, pa);
#else
	snprintf(subnode_name, sizeof(subnode_name), "%s@0x%lx", name, pa);
#endif
	offs = fdt_add_subnode(fdt, offs, subnode_name);
	if (offs >= 0) {
		u32 data[FDT_MAX_NCELLS * 2];

		set_dt_val(data, addr_size, pa);
		set_dt_val(data + addr_size, len_size, size);
		ret = fdt_setprop(fdt, offs, "reg", data,
				  sizeof(uint32_t) * (addr_size + len_size));
		if (ret < 0)
			return -1;
		ret = fdt_setprop(fdt, offs, "no-map", NULL, 0);
		if (ret < 0)
			return -1;
	} else {
		return -1;
	}
	return 0;
}
