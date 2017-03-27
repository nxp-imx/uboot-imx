/*
 *
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0
 */

#include <common.h>
#include <pci.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <asm/arch/sci/sci.h>
#include <linux/sizes.h>
#include <errno.h>
#include <imx8_hsio.h>

void mx8x_pcie_controller_reset(sc_ipc_t ipc, u32 scr)
{
	sc_err_t err;
	int i;

	err = sc_misc_set_control(ipc, scr, SC_C_PCIE_G_RST, 1);
	if (err != SC_ERR_NONE)
		printf("SC_R_PCIE G_RST failed! (error = %d)\n", err);
	for (i = 0; i < 200; i = i + 1)
		asm("nop");

	err = sc_misc_set_control(ipc, scr, SC_C_PCIE_G_RST, 0);
	if (err != SC_ERR_NONE)
		printf("SC_R_PCIE G_RST failed! (error = %d)\n", err);

	err = sc_misc_set_control(ipc, scr, SC_C_PCIE_PERST, 1);
	if (err != SC_ERR_NONE)
		printf("SC_R_PCIE PCIE_RST failed! (error = %d)\n", err);

	err = sc_misc_set_control(ipc, scr, SC_C_PCIE_BUTTON_RST, 1);
	if (err != SC_ERR_NONE)
		printf("SC_R_PCIE BUTTON_RST failed! (error = %d)\n", err);
}

static void pcie_mapping_region(u32 index, u32 direction, u32 type,
				u32 addr, u32 size, u32 target_l, u32 target_h)
{
	/* Select a iATU and configure its direction */
	pcie_writel(index | direction, PCIE0_ATU_VIEWPORT);
	setbits_le32(PCIE0_ATU_CR1, type);

	/* Set memory address and size */
	pcie_writel(addr, PCIE0_ATU_LOWER_BASE);
	pcie_writel(0, PCIE0_ATU_UPPER_BASE);
	pcie_writel((addr + size - 1), PCIE0_ATU_LIMIT);

	pcie_writel(target_l, PCIE0_ATU_LOWER_TARGET);
	pcie_writel(target_h, PCIE0_ATU_UPPER_TARGET);

	/* Enable this iATU */
	setbits_le32(PCIE0_ATU_CR2, PCIE_ATU_ENABLE);
}

static void pcie_ctrlb_mapping_region(u32 index, u32 direction, u32 type,
				      u32 addr, u32 size, u32 target_l, u32 target_h)
{
	/* Select a iATU and configure its direction */
	pcie_writel(index | direction, PCIE1_ATU_VIEWPORT);
	setbits_le32(PCIE1_ATU_CR1, type);

	/* Set memory address and size */
	pcie_writel(addr, PCIE1_ATU_LOWER_BASE);
	pcie_writel(0, PCIE1_ATU_UPPER_BASE);
	pcie_writel((addr + size - 1), PCIE1_ATU_LIMIT);

	pcie_writel(target_l, PCIE1_ATU_LOWER_TARGET);
	pcie_writel(target_h, PCIE1_ATU_UPPER_TARGET);

	/* Enable this iATU */
	setbits_le32(PCIE1_ATU_CR2, PCIE_ATU_ENABLE);
}

/* CFG Space  -->   0x40000000
 * 1st Region -->   0x41000000
 * 2nd Region -->   0x42000000
 * ...
 */
void mx8x_pcie_ctrla_setup_regions(void)
{
	u32 i, cmd;
	u32 val, index;
	u32 is_32bit;
	u32 type, size;
	u64 size64;
	const u32 region_types[] = {
		PCIE_ATU_TYPE_MEM,
		PCIE_ATU_TYPE_IO,
	};

	cmd = PCI_COMMAND_MASTER;

	pcie_mapping_region(0, PCIE_ATU_REGION_OUTBOUND, PCIE_ATU_TYPE_CFG0,
			    PCIEA_CFG_PCI_BASE, PCIE_CFG_MEM_SIZE, 0, 0);

	index = 1;
	udelay(1000);

	for (i = 0; i < 6; i++) {
		val = pcie_readl(PCIEA_CFG_CPU_BASE + 0x10 + i * 4);
		printf("#### [%d] val=%X addr=%X\r\n ", i, val,
		       PCIEA_CFG_CPU_BASE + 0x10 + i * 4);
		if (!val)
			continue;
		type = region_types[val & 0x1];
		is_32bit = ((val & 0x4) == 0);
		pcie_writel(0xFFFFFFFF, PCIEA_CFG_CPU_BASE + 0x10 + i * 4);
		size = pcie_readl(PCIEA_CFG_CPU_BASE + 0x10 + i * 4);
		size = 0xFFFFFFFF - (size & ~0xF) + 1;
		if (is_32bit) {
			pcie_mapping_region(index, PCIE_ATU_REGION_OUTBOUND,
					    type, PCIEA_CFG_PCI_BASE
					    + index * 0x1000000, size,
					    index * 0x1000000, 0);
			val = (val & 0xF) + index * 0x1000000;
			pcie_writel(val, (PCIEA_CFG_CPU_BASE + 0x10 + i * 4));
		} else {
			pcie_writel(0xFFFFFFFF, (PCIEA_CFG_CPU_BASE + 0x10
						+ i * 4 + 4));
			size64 = pcie_readl(PCIEA_CFG_CPU_BASE
					+ 0x10 + i * 4 + 4);
			size64 = 0xFFFFFFFF - size64;
			size64 <<= 32;
			size64 |= size;
			size64++;
			pcie_mapping_region(index, PCIE_ATU_REGION_OUTBOUND,
					    type, PCIEA_CFG_PCI_BASE
					    + index * 0x1000000, size64,
					    index * 0x1000000, 0);
			val = (val & 0xF) + index * 0x1000000;
			pcie_writel(val, (PCIEA_CFG_CPU_BASE + 0x10 + i * 4));
			pcie_writel(0, (PCIEA_CFG_CPU_BASE + 0x10 + i * 4 + 4));
			i++;
		}

		index++;

		if (type == PCIE_ATU_TYPE_MEM)
			cmd |= PCI_COMMAND_MEMORY;
		else
			cmd |= PCI_COMMAND_IO;
	}

	pcie_writel(cmd, PCIEA_CFG_CPU_BASE + 4);
}

/* CFG Space  -->   0x80000000
 * 1st Region -->   0x81000000
 * 2nd Region -->   0x82000000
 * ...
 */
void mx8x_pcie_ctrlb_setup_regions(void)
{
	u32 i, cmd;
	u32 val, index;
	u32 is_32bit;
	u32 type, size;
	u64 size64;
	const u32 region_types[] = {
		PCIE_ATU_TYPE_MEM,
		PCIE_ATU_TYPE_IO,
	};

	cmd = PCI_COMMAND_MASTER;

	pcie_ctrlb_mapping_region(0, PCIE_ATU_REGION_OUTBOUND, PCIE_ATU_TYPE_CFG0,
				  PCIEB_CFG_PCI_BASE, PCIE_CFG_MEM_SIZE, 0, 0);

	index = 1;
	udelay(1000);

	for (i = 0; i < 6; i++) {
		val = pcie_readl(PCIEB_CFG_CPU_BASE + 0x10 + i * 4);
		printf("#### [%d] val=%X addr=%X\r\n ", i, val,
		       PCIEB_CFG_CPU_BASE + 0x10 + i * 4);
		if (!val)
			continue;
		type = region_types[val & 0x1];
		is_32bit = ((val & 0x4) == 0);
		pcie_writel(0xFFFFFFFF, PCIEB_CFG_CPU_BASE + 0x10 + i * 4);
		size = pcie_readl(PCIEB_CFG_CPU_BASE + 0x10 + i * 4);
		size = 0xFFFFFFFF - (size & ~0xF) + 1;
		if (is_32bit) {
			pcie_ctrlb_mapping_region(index, PCIE_ATU_REGION_OUTBOUND,
						  type, PCIEB_CFG_PCI_BASE
						  + index * 0x1000000, size,
						  index * 0x1000000, 0);
			val = (val & 0xF) + index * 0x1000000;
			pcie_writel(val, (PCIEB_CFG_CPU_BASE + 0x10 + i * 4));
		} else {
			pcie_writel(0xFFFFFFFF, (PCIEB_CFG_CPU_BASE + 0x10
						+ i * 4 + 4));
			size64 = pcie_readl(PCIEB_CFG_CPU_BASE
					+ 0x10 + i * 4 + 4);
			size64 = 0xFFFFFFFF - size64;
			size64 <<= 32;
			size64 |= size;
			size64++;
			pcie_ctrlb_mapping_region(index, PCIE_ATU_REGION_OUTBOUND,
						  type, PCIEB_CFG_PCI_BASE
						  + index * 0x1000000, size64,
						  index * 0x1000000, 0);
			val = (val & 0xF) + index * 0x1000000;
			pcie_writel(val, (PCIEB_CFG_CPU_BASE + 0x10 + i * 4));
			pcie_writel(0, (PCIEB_CFG_CPU_BASE + 0x10 + i * 4 + 4));
			i++;
		}

		index++;

		if (type == PCIE_ATU_TYPE_MEM)
			cmd |= PCI_COMMAND_MEMORY;
		else
			cmd |= PCI_COMMAND_IO;
	}

	pcie_writel(cmd, PCIEB_CFG_CPU_BASE + 4);
}

