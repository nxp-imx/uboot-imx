// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2024 NXP
 *
 */

#include <config.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <asm/arch/clock.h>
#include <asm/arch/sys_proto.h>
#include <dm.h>
#include <dm/device_compat.h>
#include <dm/device-internal.h>
#include <dm/device.h>
#include <errno.h>
#include <fuse.h>
#include <linux/delay.h>
#include <malloc.h>
#include <thermal.h>
#include <clk.h>
#include <nvmem.h>
#include <linux/bitfield.h>
#include <linux/iopoll.h>

DECLARE_GLOBAL_DATA_PTR;

#define CTRL0			0x0

#define STAT0			0x10
#define STAT0_DRDY0_IF_MASK	BIT(16)

#define DATA0			0x20
#define DATA_NEGATIVE_MASK	BIT(15)
#define DATA_INT_MASK		GENMASK(14, 7)
#define DATA_FRAC_MASK		GENMASK(6, 0)

#define THR_CTRL01		0x30
#define THR_CTRL23		0x40

#define CTRL1			0x200
#define CTRL1_SET		0x204
#define CTRL1_CLR		0x208
#define CTRL1_EN		BIT(31)
#define CTRL1_START		BIT(30)
#define CTRL1_STOP		BIT(29)
#define CTRL1_RES_MASK		GENMASK(19, 18)
#define CTRL1_MEAS_MODE_MASK	GENMASK(25, 24)

#define PERIOD_CTRL		0x270
#define MEAS_FREQ_MASK		GENMASK(23, 0)

#define REF_DIV			0x280
#define DIV_EN			BIT(31)
#define DIV_MASK		GENMASK(23, 16)

#define PUD_ST_CTRL		0x2B0
#define PUDL_MASK		GENMASK(23, 16)

#define TRIM1			0x2E0
#define TRIM2			0x2F0

#define TMU_TEMP_PASSIVE_COOL_DELTA	10000

#define TMU_TEMP_LOW_LIMIT	-40
#define TMU_TEMP_HIGH_LIMIT	125

#define DEFAULT_TRIM1_CONFIG 0xB561BC2DU
#define DEFAULT_TRIM2_CONFIG 0x65D4U

#define IMX_TMU_POLLING_DELAY_MS      5000

struct imx91_tmu {
	struct udevice *dev;
	void __iomem *iobase;
	struct clk clk;
	int polling_delay;
	int critical;
	int passive;
};

int imx91_tmu_get_temp(struct udevice *dev, int *temp)
{
	struct imx91_tmu *tmu = dev_get_priv(dev);
	u32 val;
	int ret;

	ret = readl_poll_sleep_timeout(tmu->iobase + STAT0, val,
				       (val & STAT0_DRDY0_IF_MASK), 1000, 40000);
	if (ret)
		return -EAGAIN;

	val = readl_relaxed(tmu->iobase + DATA0) & 0xffffU;
	*temp = (int)val * 1000LL / 64LL / 1000LL;
	if (*temp < TMU_TEMP_LOW_LIMIT || *temp > TMU_TEMP_HIGH_LIMIT)
		return -EAGAIN;

	return 0;
}

static const struct dm_thermal_ops imx91_tmu_ops = {
	.get_temp	= imx91_tmu_get_temp,
};

static void imx91_tmu_start(struct imx91_tmu *tmu, bool start)
{
	if (start)
		writel_relaxed(CTRL1_START, tmu->iobase + CTRL1_SET);
	else
		writel_relaxed(CTRL1_STOP, tmu->iobase + CTRL1_SET);
}

static void imx91_tmu_enable(struct imx91_tmu *tmu, bool enable)
{
	if (enable)
		writel_relaxed(CTRL1_EN, tmu->iobase + CTRL1_SET);
	else
		writel_relaxed(CTRL1_EN, tmu->iobase + CTRL1_CLR);
}

static int imx_init_from_fuse(struct imx91_tmu *tmu)
{
	int ret;
	u32 trim_val[2] = {};

	ret = fuse_read(5, 0, &trim_val[0]);
	if (ret)
		return ret;

	ret = fuse_read(5, 1, &trim_val[1]);
	if (ret)
		return ret;

	if (trim_val[0] == 0 || trim_val[1] == 0)
		return -EINVAL;

	writel_relaxed(trim_val[0], tmu->iobase + TRIM1);
	writel_relaxed(trim_val[1], tmu->iobase + TRIM2);

	return 0;
}

static int imx91_tmu_default_setup(struct imx91_tmu *tmu)
{
	int ret;
	unsigned long rate;
	u32 div;

#if (IS_ENABLED(CONFIG_CLK))
	ret = clk_enable(&tmu->clk);
	if (ret)
		return ret;
#endif

	/* disable the monitor during initialization */
	imx91_tmu_enable(tmu, false);
	imx91_tmu_start(tmu, false);

	ret = imx_init_from_fuse(tmu);
	if (ret) {
		dev_dbg(tmu->dev, "fail to get trim info %d, so use default configuration\n", ret);
		writel_relaxed(DEFAULT_TRIM1_CONFIG, tmu->iobase + TRIM1);
		writel_relaxed(DEFAULT_TRIM2_CONFIG, tmu->iobase + TRIM2);
	}

#if (IS_ENABLED(CONFIG_CLK))
	/* The typical conv clk is 4MHz, the output freq is 'rate / (div + 1)' */
	rate = clk_get_rate(&tmu->clk);
	div = (rate / 4000000) - 1;
#else
	/* default clk is 'rate / 6' */
	div = 5;
#endif

	/* Set divider value and enable divider */
	writel_relaxed(DIV_EN | FIELD_PREP(DIV_MASK, div), tmu->iobase + REF_DIV);

	/* Set max power up delay: 'Tpud(ms) = 0xFF * 1000 / 4000000' */
	writel_relaxed(FIELD_PREP(PUDL_MASK, 100U), tmu->iobase + PUD_ST_CTRL);

	/*
	 * Set resolution mode
	 * 00b - Conversion time = 0.59325 ms
	 * 01b - Conversion time = 1.10525 ms
	 * 10b - Conversion time = 2.12925 ms
	 * 11b - Conversion time = 4.17725 ms
	 */
	writel_relaxed(FIELD_PREP(CTRL1_RES_MASK, 0x3), tmu->iobase + CTRL1_CLR);
	writel_relaxed(FIELD_PREP(CTRL1_RES_MASK, 0x1), tmu->iobase + CTRL1_SET);

	/*
	 * Set measure mode
	 * 00b - Single oneshot measurement
	 * 01b - Continuous measurement
	 * 10b - Periodic oneshot measurement
	 */
	writel_relaxed(FIELD_PREP(CTRL1_MEAS_MODE_MASK, 0x3), tmu->iobase + CTRL1_CLR);
	writel_relaxed(FIELD_PREP(CTRL1_MEAS_MODE_MASK, 0x1), tmu->iobase + CTRL1_SET);

	/*
	 * Set Periodic Measurement Frequency to 25Hz:
	 * tMEAS_FREQ = tCONV_CLK * PERIOD_CTRL[MEAS_FREQ]. ->
	 * PERIOD_CTRL(MEAS_FREQ) = (1000 / 25) / (1000 / 4000000);
	 * Where tMEAS_FREQ = Measurement period and tCONV_CLK = 1/fCONV_CLK.
	 * This field should have value greater than count corresponds
	 * to time greater than summation of conversion time, power up
	 * delay, and six times of conversion clock time.
	 * tMEAS_FREQ > (tCONV + tPUD + 6 * tCONV_CLK).
	 * tCONV is conversion time determined by CTRL1[RESOLUTION].
	 */
	writel_relaxed(FIELD_PREP(MEAS_FREQ_MASK, 0x27100), tmu->iobase + PERIOD_CTRL);

	/* enable the monitor */
	imx91_tmu_enable(tmu, true);
	imx91_tmu_start(tmu, true);

	return 0;
}

static int imx91_tmu_probe(struct udevice *dev)
{
	struct imx91_tmu *tmu = dev_get_priv(dev);
	int ret;

	ret = imx91_tmu_default_setup(tmu);
	if (ret)
		dev_err(dev, "imx tmu default setup fail %d\n", ret);

	return 0;
}

static const struct udevice_id imx91_tmu_ids[] = {
	{ .compatible = "fsl,imx91-tmu" },
	{ }
};

static int imx91_tmu_of_to_plat(struct udevice *dev)
{
	struct imx91_tmu *tmu = dev_get_priv(dev);
	ofnode trips_np;
	void *iobase;
	int ret;

	tmu->dev = dev;

	iobase = dev_read_addr_ptr(dev);
	if (!iobase) {
		dev_err(dev, "tmu regs missing\n");
		return -EINVAL;
	}
	tmu->iobase = iobase;

	trips_np = ofnode_path("/thermal-zones/cpu-thermal/trips");
	ofnode_for_each_subnode(trips_np, trips_np) {
		const char *type;

		type = ofnode_get_property(trips_np, "type", NULL);
		if (!type)
			continue;
		if (!strcmp(type, "critical"))
			tmu->critical = ofnode_read_u32_default(trips_np, "temperature", 85);
		else if (strcmp(type, "passive") == 0)
			tmu->passive = ofnode_read_u32_default(trips_np, "temperature", 80);
		else
			continue;
	}

#if (IS_ENABLED(CONFIG_CLK))
	ret = clk_get_by_index(dev, 0, &tmu->clk);
	if (ret) {
		dev_err(dev, "failed to get tmu clock\n");
		return ret;
	}
#endif

	dev_dbg(dev, "polling_delay %d, critical %d, alert %d\n",
		tmu->polling_delay, tmu->critical, tmu->passive);
	return 0;
}

U_BOOT_DRIVER(imx91_tmu) = {
	.name	= "imx91_tmu",
	.id	= UCLASS_THERMAL,
	.ops	= &imx91_tmu_ops,
	.of_to_plat = imx91_tmu_of_to_plat,
	.of_match = imx91_tmu_ids,
	.probe	= imx91_tmu_probe,
	.priv_auto	= sizeof(struct imx91_tmu),
	.flags  = DM_FLAG_PRE_RELOC,
};
