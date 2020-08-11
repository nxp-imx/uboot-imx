// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019 NXP
 */

#include <common.h>
#include <cpu.h>
#include <dm.h>
#include <thermal.h>
#include <asm/arch/sci/sci.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch-imx/cpu.h>
#include <asm/armv8/cpu.h>
#include <clk.h>

DECLARE_GLOBAL_DATA_PTR;

struct cpu_imx_platdata {
	const char *name;
	const char *rev;
	const char *type;
	u32 cpurev;
	u32 freq_mhz;
	u32 mpidr;
};

const char *get_imx8_type(u32 imxtype)
{
	switch (imxtype) {
	case MXC_CPU_IMX8QXP:
	case MXC_CPU_IMX8QXP_A0:
		return "QXP";
	case MXC_CPU_IMX8QM:
		return "QM";
	case MXC_CPU_IMX8DXL:
		return "DXL";
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
	case CHIP_REV_C:
		return "C";
	case CHIP_REV_A1:
		return "A1";
	case CHIP_REV_A2:
		return "A2";
	default:
		return "?";
	}
}

const char *get_core_name(struct udevice *dev)
{
	const void *blob = gd->fdt_blob;
	int node = dev_of_offset(dev);

	if (!fdt_node_check_compatible(blob, node, "arm,cortex-a35"))
		return "A35";
	else if (!fdt_node_check_compatible(blob, node, "arm,cortex-a53"))
		return "A53";
	else if (!fdt_node_check_compatible(blob, node, "arm,cortex-a72"))
		return "A72";
	else
		return "?";
}

#if IS_ENABLED(CONFIG_IMX_SCU_THERMAL)
static int cpu_imx_get_temp(struct cpu_imx_platdata *plat)
{
	struct udevice *thermal_dev;
	int cpu_tmp, ret;

	if (!strcmp(plat->name, "A72"))
		ret = uclass_get_device_by_name(UCLASS_THERMAL, "cpu-thermal1",
			&thermal_dev);
	else
		ret = uclass_get_device_by_name(UCLASS_THERMAL, "cpu-thermal0",
			&thermal_dev);

	if (!ret) {
		ret = thermal_get_temp(thermal_dev, &cpu_tmp);
		if (ret)
			return 0xdeadbeef;
	} else {
		return 0xdeadbeef;
	}

	return cpu_tmp;
}
#else
static int cpu_imx_get_temp(struct cpu_imx_platdata *plat)
{
	return 0;
}
#endif

int cpu_imx_get_desc(struct udevice *dev, char *buf, int size)
{
	struct cpu_imx_platdata *plat = dev_get_platdata(dev);
	int ret, temp;

	if (size < 100)
		return -ENOSPC;

	ret = snprintf(buf, size, "NXP i.MX8%s Rev%s %s at %u MHz",
		       plat->type, plat->rev, plat->name, plat->freq_mhz);

	if (IS_ENABLED(CONFIG_IMX_SCU_THERMAL)) {
		temp = cpu_imx_get_temp(plat);
		buf = buf + ret;
		size = size - ret;
		if (temp != 0xdeadbeef)
			ret = snprintf(buf, size, " at %dC", temp);
		else
			ret = snprintf(buf, size, " - invalid sensor data");
	}

	snprintf(buf + ret, size - ret, "\n");

	return 0;
}

static int cpu_imx_get_info(struct udevice *dev, struct cpu_info *info)
{
	struct cpu_imx_platdata *plat = dev_get_platdata(dev);

	info->cpu_freq = plat->freq_mhz * 1000;
	info->features = BIT(CPU_FEAT_L1_CACHE) | BIT(CPU_FEAT_MMU);
	return 0;
}

static int cpu_imx_get_count(struct udevice *dev)
{
	if (is_imx8qxp())
		return 4;
	else if (is_imx8dxl())
		return 2;
	else
		return 6;
}

static int cpu_imx_get_vendor(struct udevice *dev,  char *buf, int size)
{
	snprintf(buf, size, "NXP");
	return 0;
}

static bool cpu_imx_is_current(struct udevice *dev)
{
	struct cpu_imx_platdata *plat = dev_get_platdata(dev);

	if (plat->mpidr == (read_mpidr() & 0xffff))
		return true;

	return false;
}

static const struct cpu_ops cpu_imx8_ops = {
	.get_desc	= cpu_imx_get_desc,
	.get_info	= cpu_imx_get_info,
	.get_count	= cpu_imx_get_count,
	.get_vendor	= cpu_imx_get_vendor,
	.is_current_cpu = cpu_imx_is_current,
};

static const struct udevice_id cpu_imx8_ids[] = {
	{ .compatible = "arm,cortex-a35" },
	{ .compatible = "arm,cortex-a53" },
	{ .compatible = "arm,cortex-a72" },
	{ }
};

static int imx8_cpu_probe(struct udevice *dev)
{
	struct cpu_imx_platdata *plat = dev_get_platdata(dev);
	struct clk cpu_clk;
	u32 cpurev;
	int ret;

	cpurev = get_cpu_rev();
	plat->cpurev = cpurev;
	plat->name = get_core_name(dev);
	plat->rev = get_imx8_rev(cpurev & 0xFFF);
	plat->type = get_imx8_type((cpurev & 0xFF000) >> 12);

	plat->mpidr = dev_read_addr(dev);
	if (plat->mpidr == FDT_ADDR_T_NONE) {
		printf("%s: Failed to get CPU reg property\n", __func__);
		return -EINVAL;
	}

	ret = clk_get_by_index(dev, 0, &cpu_clk);
	if (ret) {
		debug("%s: Failed to get CPU clk: %d\n", __func__, ret);
		return 0;
	}

	plat->freq_mhz = clk_get_rate(&cpu_clk) / 1000000;
	return 0;
}

U_BOOT_DRIVER(cpu_imx8_drv) = {
	.name		= "imx8x_cpu",
	.id		= UCLASS_CPU,
	.of_match	= cpu_imx8_ids,
	.ops		= &cpu_imx8_ops,
	.probe		= imx8_cpu_probe,
	.platdata_auto_alloc_size = sizeof(struct cpu_imx_platdata),
	.flags		= DM_FLAG_PRE_RELOC,
};
