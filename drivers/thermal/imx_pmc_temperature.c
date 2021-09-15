// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2021 NXP
 */

#include <asm/arch/imx-regs.h>
#include <asm/arch-imx8ulp/clock.h>
#include <asm/io.h>
#include <dm.h>
#include <linux/delay.h>
#include <thermal.h>

#define ADC_CTRL_ADCEN_MASK	0x1
#define ADC_CTRL_RST_MASK	0x2
#define ADC_CTRL_DOZEN_MASK	0x4
#define ADC_CTRL_RSTFIFO0_MASK	0x100

#define ADC_CFG_PUDLY(x)	(((u32)(((u32)(x)) << 16)) & 0xff0000)
#define ADC_CFG_PWREN_MASK	0x10000000

#define ADC_CMDL_ADCH(x)	(((u32)(((u32)(x)) << 0)) & 0x1f)
#define ADC_CMDL_DIFF(x)	(((u32)(((u32)(x)) << 6)) & 0x40)
#define ADC_CMDL_CSCALE(x)	(((u32)(((u32)(x)) << 13)) & 0x2000)

#define ADC_CMDH_STS(x)		(((u32)(((u32)(x)) << 8)) & 0x700)
#define ADC_CMDH_AVGS(x)	(((u32)(((u32)(x)) << 12)) & 0x7000)

#define ADC_TCTRL_TCMD(x)	(((u32)(((u32)(x)) << 24)) & 0xf000000)

#define ANACORE_CTRL_OFFSET     0x30
#define TSENSM_MASK	0xf00000
#define TSENSM(x)	(((u32)(((u32)(x)) << 20)) & 0xf00000)
#define TSENSEN_MASK	0x10000
#define TSENSEN(x)	(((u32)(((u32)(x)) << 16)) & 0x10000)

static int imx_read_pmc_temperature(struct udevice *dev, int *temperature)
{
	void __iomem *pmc_reg;
	void __iomem *pmc_anacore_ctrl_addr;
	struct adc_regs *regs;
	struct ofnode_phandle_args args;
	u8 tsensm[7] = {0, 2, 6, 4, 6, 2, 0};
	u16 tsensorvalue[7] = {0}, conv_value;
	u32 val, tmp32, i, cm_000, cm_010, cm_110, c1_temp, c2_temp, cm_temp, vm_temp;
	int ret;
	fdt_addr_t addr = devfdt_get_addr_index(dev, 0);

	if (addr == FDT_ADDR_T_NONE)
		return -EINVAL;

	pmc_reg = (void __iomem *)addr;
	pmc_anacore_ctrl_addr = (void __iomem *)(addr + ANACORE_CTRL_OFFSET);

	/* enable the ADC1's clock */
	enable_adc1_clk(true);

	ret = dev_read_phandle_with_args(dev, "adc", NULL, 0, 0, &args);
	if (ret)
		return ret;
	addr = ofnode_get_addr(args.node);
	if (addr == FDT_ADDR_T_NONE)
		return -EINVAL;

	regs = (struct adc_regs *)addr;
	/* reset the ADC1's configuration */
	val = readl(&regs->ctrl);
	val |= ADC_CTRL_RST_MASK;
	writel(val, &regs->ctrl);
	val = readl(&regs->ctrl);
	val &= ~ADC_CTRL_RST_MASK;
	writel(val, &regs->ctrl);

	/* reset the conversion FIFO0 */
	val = readl(&regs->ctrl);
	val |= ADC_CTRL_RSTFIFO0_MASK;
	writel(val, &regs->ctrl);

	/* disable the ADC1 */
	val = readl(&regs->ctrl);
	val &= ~ADC_CTRL_ADCEN_MASK;
	writel(val, &regs->ctrl);

	/* ADC1 is enabled in Doze mode */
	val = readl(&regs->ctrl);
	val &= ~ADC_CTRL_DOZEN_MASK;
	writel(val, &regs->ctrl);

	/* ADC1's Configuration Register */
	val = 0;
	val |= ADC_CFG_PWREN_MASK;
	val |= ADC_CFG_PUDLY(0x80);
	writel(val, &regs->cfg);

	/* enable the ADC1 */
	val = readl(&regs->ctrl);
	val |= ADC_CTRL_ADCEN_MASK;
	writel(val, &regs->ctrl);

	/* reset the conversion FIFO0 */
	val = readl(&regs->ctrl);
	val |= ADC_CTRL_RSTFIFO0_MASK;
	writel(val, &regs->ctrl);

	/* set conversion CMD configuration */
	val = readl(&regs->cmdl1);
	val |= ADC_CMDL_ADCH(8);
	val |= ADC_CMDL_DIFF(1);
	val |= ADC_CMDL_CSCALE(1);
	writel(val, &regs->cmdl1);

	val = readl(&regs->cmdh1);
	val |= ADC_CMDH_STS(7);
	val |= ADC_CMDH_AVGS(7);
	writel(val, &regs->cmdh1);

	/* set trigger configuration */
	val = readl(&regs->tctrl0);
	val |= ADC_TCTRL_TCMD(1);
	writel(val, &regs->tctrl0);

	/* enable PMC temperature sensor and wait 40us */
	val = readl(pmc_anacore_ctrl_addr);
	val &= ~TSENSEN_MASK;
	val |= TSENSEN(1);
	writel(val, pmc_anacore_ctrl_addr);
	udelay(40);

	for (i = 0; i < 7; i++) {
		val = readl(pmc_reg);
		val &= ~TSENSM_MASK;
		val |= TSENSM(tsensm[i]);
		writel(val, pmc_reg);

		writel(1, &regs->swtrig);
		while (((tmp32 = readl(&regs->resfifo0)) & 0x80000000) == 0) {}
		conv_value = (u16)((tmp32 & 0xffff) >> 3);
		tsensorvalue[i] = conv_value;
	}

	cm_000 = (tsensorvalue[0] + tsensorvalue[6]) / 2;
	cm_010 = (tsensorvalue[1] + tsensorvalue[5]) / 2;
	cm_110 = (tsensorvalue[2] + tsensorvalue[4]) / 2;
	c1_temp = (2 * cm_000 - cm_010);
	c2_temp = (2 * tsensorvalue[3] - cm_110);
	cm_temp = (c1_temp + c2_temp) / 2;
	vm_temp = (100 * cm_temp + 80 * cm_temp) / 4096;
	*temperature = (303 * vm_temp + 105 * vm_temp / 1000 - 27315) / 100;

	return 0;
}

static const struct dm_thermal_ops temperature_ops = {
	.get_temp = imx_read_pmc_temperature,
};

static int imx_pmc_temperature_probe(struct udevice *dev)
{
	return 0;
}

static const struct udevice_id imx_ids[] = {
	{ .compatible = "fsl,imx8ulp-pmc-temperature", },
	{ }
};

U_BOOT_DRIVER(imx_pmc_temperature) = {
	.name	= "imx_pmc_temperature",
	.id	= UCLASS_THERMAL,
	.ops	= &temperature_ops,
	.of_match = imx_ids,
	.probe	= imx_pmc_temperature_probe,
	.flags  = DM_FLAG_PRE_RELOC,
};
