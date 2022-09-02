// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2022 NXP
 */

#include <common.h>
#include <clk.h>
#include <cpu_func.h>
#include <dm.h>
#include <errno.h>
#include <eth_phy.h>
#include <log.h>
#include <malloc.h>
#include <memalign.h>
#include <miiphy.h>
#include <net.h>
#include <netdev.h>
#include <phy.h>
#include <reset.h>
#include <wait_bit.h>
#include <asm/arch/clock.h>
#include <asm/cache.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <asm/mach-imx/sys_proto.h>
#include <linux/delay.h>

#include "dwc_eth_qos.h"

static int eqos_start_clks_imx(struct udevice *dev)
{
#if CONFIG_IS_ENABLED(CLK) && IS_ENABLED(CONFIG_IMX8)
	struct eqos_priv *eqos = dev_get_priv(dev);
	int ret;

	debug("%s(dev=%p):\n", __func__, dev);

	ret = clk_enable(&eqos->clk_slave_bus);
	if (ret < 0) {
		pr_err("clk_enable(clk_slave_bus) failed: %d", ret);
		goto err;
	}

	ret = clk_enable(&eqos->clk_master_bus);
	if (ret < 0) {
		pr_err("clk_enable(clk_master_bus) failed: %d", ret);
		goto err_disable_clk_slave_bus;
	}

	ret = clk_enable(&eqos->clk_tx);
	if (ret < 0) {
		pr_err("clk_enable(clk_tx) failed: %d", ret);
		goto err_disable_clk_master_bus;
	}
#endif

	debug("%s: OK\n", __func__);
	return 0;

#if CONFIG_IS_ENABLED(CLK) && IS_ENABLED(CONFIG_IMX8)
err_disable_clk_master_bus:
	clk_disable(&eqos->clk_master_bus);
err_disable_clk_slave_bus:
	clk_disable(&eqos->clk_slave_bus);
err:
	debug("%s: FAILED: %d\n", __func__, ret);
	return ret;
#endif
}

static int eqos_stop_clks_imx(struct udevice *dev)
{
#if CONFIG_IS_ENABLED(CLK) && IS_ENABLED(CONFIG_IMX8)
	struct eqos_priv *eqos = dev_get_priv(dev);

	debug("%s(dev=%p):\n", __func__, dev);

	clk_disable(&eqos->clk_tx);
	clk_disable(&eqos->clk_slave_bus);
	clk_disable(&eqos->clk_master_bus);
#endif

	debug("%s: OK\n", __func__);
	return 0;
}

__weak u32 imx_get_eqos_csr_clk(void)
{
	return 100 * 1000000;
}

__weak int imx_eqos_txclk_set_rate(unsigned long rate)
{
	return 0;
}

static ulong eqos_get_tick_clk_rate_imx(struct udevice *dev)
{
#if CONFIG_IS_ENABLED(CLK) && IS_ENABLED(CONFIG_IMX8)
	struct eqos_priv *eqos = dev_get_priv(dev);
	return clk_get_rate(&eqos->clk_slave_bus);
#else
	return imx_get_eqos_csr_clk();
#endif
}

static int eqos_probe_resources_imx(struct udevice *dev)
{
	struct eqos_priv *eqos = dev_get_priv(dev);
	phy_interface_t interface;
	__maybe_unused int ret;

	debug("%s(dev=%p):\n", __func__, dev);

	interface = eqos->config->interface(dev);

	if (interface == PHY_INTERFACE_MODE_NA) {
		pr_err("Invalid PHY interface\n");
		return -EINVAL;
	}

#if CONFIG_IS_ENABLED(CLK) && IS_ENABLED(CONFIG_IMX8)
	ret = clk_get_by_name(dev, "aclk", &eqos->clk_master_bus);
	if (ret) {
		pr_err("clk_get_by_name(csr) failed: %d", ret);
		goto err_probe;
	}

	ret = clk_get_by_name(dev, "csr", &eqos->clk_slave_bus);
	if (ret) {
		pr_err("clk_get_by_name(aclk) failed: %d", ret);
		goto err_free_clk_master_bus;
	}

	ret = clk_get_by_name(dev, "tx_clk", &eqos->clk_tx);
	if (ret) {
		pr_err("clk_get_by_name(tx) failed: %d", ret);
		goto err_free_clk_slave_bus;
	}
#endif

	debug("%s: OK\n", __func__);
	return 0;

#if CONFIG_IS_ENABLED(CLK) && IS_ENABLED(CONFIG_IMX8)
err_free_clk_slave_bus:
	clk_free(&eqos->clk_slave_bus);
err_free_clk_master_bus:
	clk_free(&eqos->clk_master_bus);
err_probe:

	debug("%s: returns %d\n", __func__, ret);
	return ret;
#endif
}

static int eqos_remove_resources_imx(struct udevice *dev)
{
#if CONFIG_IS_ENABLED(CLK) && IS_ENABLED(CONFIG_IMX8)
	struct eqos_priv *eqos = dev_get_priv(dev);
	debug("%s(dev=%p):\n", __func__, dev);
	clk_free(&eqos->clk_tx);
	clk_free(&eqos->clk_slave_bus);
	clk_free(&eqos->clk_master_bus);
#endif

	debug("%s: OK\n", __func__);
	return 0;
}

static int eqos_set_tx_clk_speed_imx(struct udevice *dev)
{
	struct eqos_priv *eqos = dev_get_priv(dev);
	ulong rate;
	int ret = 0;

	debug("%s(dev=%p):\n", __func__, dev);

	switch (eqos->phy->speed) {
	case SPEED_1000:
		rate = 125 * 1000 * 1000;
		break;
	case SPEED_100:
		rate = 25 * 1000 * 1000;
		break;
	case SPEED_10:
		rate = 2.5 * 1000 * 1000;
		break;
	default:
		pr_err("invalid speed %d", eqos->phy->speed);
		return -EINVAL;
	}

#if CONFIG_IS_ENABLED(CLK) && IS_ENABLED(CONFIG_IMX8)
	if (!is_imx8dxl())
		ret = clk_set_rate(&eqos->clk_tx, rate);
#else
	ret = imx_eqos_txclk_set_rate(rate);
#endif
	if (ret < 0) {
		pr_err("imx (tx_clk, %lu) failed: %d", rate, ret);
		return ret;
	}

	return 0;
}

static int eqos_get_enetaddr_imx(struct udevice *dev)
{
	struct eth_pdata *pdata = dev_get_plat(dev);

	imx_get_mac_from_fuse(dev_seq(dev), pdata->enetaddr);

	return 0;
}

static int eqos_start_resets_imx(struct udevice *dev)
{
	struct eqos_priv *eqos = dev_get_priv(dev);

	writel(EQOS_DMA_MODE_SWR, &eqos->dma_regs->mode);
	return 0;
}

static struct eqos_ops eqos_imx_ops = {
	.eqos_inval_desc = eqos_inval_desc_generic,
	.eqos_flush_desc = eqos_flush_desc_generic,
	.eqos_inval_buffer = eqos_inval_buffer_generic,
	.eqos_flush_buffer = eqos_flush_buffer_generic,
	.eqos_probe_resources = eqos_probe_resources_imx,
	.eqos_remove_resources = eqos_remove_resources_imx,
	.eqos_stop_resets = eqos_null_ops,
	.eqos_start_resets = eqos_start_resets_imx,
	.eqos_stop_clks = eqos_stop_clks_imx,
	.eqos_start_clks = eqos_start_clks_imx,
	.eqos_calibrate_pads = eqos_null_ops,
	.eqos_disable_calibration = eqos_null_ops,
	.eqos_set_tx_clk_speed = eqos_set_tx_clk_speed_imx,
	.eqos_get_enetaddr = eqos_get_enetaddr_imx,
	.eqos_get_tick_clk_rate = eqos_get_tick_clk_rate_imx,
};

struct eqos_config __maybe_unused eqos_imx_config = {
	.reg_access_always_ok = false,
	.mdio_wait = 10,
	.swr_wait = 50,
	.config_mac = EQOS_MAC_RXQ_CTRL0_RXQ0EN_ENABLED_DCB,
	.config_mac_mdio = EQOS_MAC_MDIO_ADDRESS_CR_250_300,
	.axi_bus_width = EQOS_AXI_WIDTH_64,
	.interface = dev_read_phy_mode,
	.ops = &eqos_imx_ops
};
