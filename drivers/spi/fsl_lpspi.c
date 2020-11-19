// SPDX-License-Identifier: GPL-2.0+
/*
 * LPSPI controller driver.
 *
 * Copyright 2020 NXP Semiconductor, Inc.
 * Author: Clark Wang (xiaoning.wang@nxp.com)
 */

#include <common.h>
#include <clk.h>
#include <dm.h>
#include <malloc.h>
#include <spi.h>
#include <dm/device_compat.h>
#include <linux/errno.h>
#include <asm/io.h>
#include <asm/gpio.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/clock.h>
#include <asm/mach-imx/spi.h>
#include <asm/arch/sys_proto.h>
#include "fsl_lpspi.h"

DECLARE_GLOBAL_DATA_PTR;

__weak int enable_lpspi_clk(unsigned char enable, unsigned int spi_num)
{
	return 0;
}

__weak u32 imx_get_spiclk(u32 spi_num)
{
	return 0;
}

#define reg_read readl
#define reg_write(a, v) writel(v, a)

#define MAX_CS_COUNT	4
#define MAX_SPI_BYTES	16

struct fsl_lpspi_slave {
	struct spi_slave slave;
	unsigned long	base;
	unsigned int	seq;
	unsigned int	prescale;
	unsigned int	max_hz;
	unsigned int	speed;
	unsigned int	mode;
	unsigned int	wordlen;
	unsigned int	fifolen;
	struct gpio_desc ss;
	struct gpio_desc cs_gpios[MAX_CS_COUNT];
	struct udevice *dev;
	struct clk per_clk;
	struct clk ipg_clk;

	/* SPI FiFo accessor */
	void *rx_buf;
	const void *tx_buf;
	void (*rx_fifo)(struct fsl_lpspi_slave *);
	void (*tx_fifo)(struct fsl_lpspi_slave *);
};

static inline struct fsl_lpspi_slave *to_fsl_lpspi_slave(struct spi_slave *slave)
{
	return container_of(slave, struct fsl_lpspi_slave, slave);
}

#define BUILD_SPI_FIFO_RW(__name, __type)				\
static void fsl_lpspi_rx_##__name(struct fsl_lpspi_slave *lpspi)	\
{									\
	struct LPSPI_Type *regs = (struct LPSPI_Type *)lpspi->base;	\
	unsigned int val = reg_read(&regs->RDR);			\
									\
	if (lpspi->rx_buf) {						\
		*(__type *)lpspi->rx_buf = val;				\
		lpspi->rx_buf += sizeof(__type);			\
	}								\
}									\
									\
static void fsl_lpspi_tx_##__name(struct fsl_lpspi_slave *lpspi)	\
{									\
	__type val = 0;							\
	struct LPSPI_Type *regs = (struct LPSPI_Type *)lpspi->base;	\
									\
	if (lpspi->tx_buf) {						\
		val = *(__type *)lpspi->tx_buf;				\
		lpspi->tx_buf += sizeof(__type);			\
	}								\
									\
	reg_write(&regs->TDR, val);					\
}
BUILD_SPI_FIFO_RW(byte, u8);
BUILD_SPI_FIFO_RW(word, u16);
BUILD_SPI_FIFO_RW(dword, u32);

static int fsl_lpspi_set_word_size(struct fsl_lpspi_slave *lpspi,
				   unsigned int wordlen)
{
	lpspi->wordlen = wordlen;

	switch (wordlen) {
	case 8:
		lpspi->rx_fifo = fsl_lpspi_rx_byte;
		lpspi->tx_fifo = fsl_lpspi_tx_byte;
		break;
	case 16:
		lpspi->rx_fifo = fsl_lpspi_rx_word;
		lpspi->tx_fifo = fsl_lpspi_tx_word;
		break;
	case 32:
		lpspi->rx_fifo = fsl_lpspi_rx_dword;
		lpspi->tx_fifo = fsl_lpspi_tx_dword;
		break;
	default:
		dev_err(lpspi->dev, "fsl_lpspi: unsupported wordlen: %d\n",
				wordlen);
		return -EINVAL;
	}

	return 0;
}

static void fsl_lpspi_cs_activate(struct fsl_lpspi_slave *lpspi)
{
	struct udevice *dev = lpspi->dev;
	struct dm_spi_slave_platdata *slave_plat = dev_get_parent_platdata(dev);

	u32 cs = slave_plat->cs;

	if (!dm_gpio_is_valid(&lpspi->cs_gpios[cs]))
		return;

	dm_gpio_set_value(&lpspi->cs_gpios[cs], 1);
}

static void fsl_lpspi_cs_deactivate(struct fsl_lpspi_slave *lpspi)
{
	struct udevice *dev = lpspi->dev;
	struct dm_spi_slave_platdata *slave_plat = dev_get_parent_platdata(dev);

	u32 cs = slave_plat->cs;

	if (!dm_gpio_is_valid(&lpspi->cs_gpios[cs]))
		return;

	dm_gpio_set_value(&lpspi->cs_gpios[cs], 0);
}

static int clkdivs[] = {1, 2, 4, 8, 16, 32, 64, 128};

static s32 spi_cfg_lpspi(struct fsl_lpspi_slave *lpspi, unsigned int cs)
{
	s32 reg_config;
	unsigned int perclk_rate, scldiv;
	u8 prescale;
	struct LPSPI_Type *regs = (struct LPSPI_Type *)lpspi->base;
	unsigned int speed = lpspi->speed;

	/* Disable all interrupt */
	reg_write(&regs->IER, 0);
	/* W1C for all flags in SR */
	reg_write(&regs->SR, (0x3F << 8));
	/* Clear FIFO and disable module */
	reg_write(&regs->CR, (LPSPI_CR_RTF_MASK | LPSPI_CR_RRF_MASK));

	lpspi->fifolen = 1 << (reg_read(&regs->PARAM) & LPSPI_PARAM_TXFIFO_MASK);

	reg_config = reg_read(&regs->CFGR1);
	reg_config = (reg_config & (~LPSPI_CFGR1_MASTER_MASK)) |
		     LPSPI_CFGR1_MASTER(1);
	if (lpspi->mode & SPI_CS_HIGH)
		reg_config = reg_config |
			     (1UL << (LPSPI_CFGR1_PCSPOL_SHIFT + (uint32_t)cs));
	else
		reg_config = reg_config &
			    ~(1UL << (LPSPI_CFGR1_PCSPOL_SHIFT + (uint32_t)cs));
	reg_config = (reg_config & ~(LPSPI_CFGR1_OUTCFG_MASK |
			LPSPI_CFGR1_PINCFG_MASK | LPSPI_CFGR1_NOSTALL_MASK)) |
			LPSPI_CFGR1_OUTCFG(0) | LPSPI_CFGR1_PINCFG(0) |
			LPSPI_CFGR1_NOSTALL(0);
	reg_write(&regs->CFGR1, reg_config);

	if (IS_ENABLED(CONFIG_CLK)) {
		perclk_rate = clk_get_rate(&lpspi->per_clk);
		if (perclk_rate <= 0) {
			dev_err(lpspi->dev, "Failed to get spi clk: %d\n",
				perclk_rate);
			return perclk_rate;
		}
	} else {
		perclk_rate = imx_get_spiclk(lpspi->seq);
		if (!perclk_rate)
			return -EPERM;
	}

	if (speed > perclk_rate / 2) {
		dev_err(lpspi->dev,
		      "per-clk should be at least two times of transfer speed, speed=%d", speed);
		return -EINVAL;
	}

	for (prescale = 0; prescale < 8; prescale++) {
		scldiv = perclk_rate /
			 (clkdivs[prescale] * speed) - 2;
		if (scldiv < 256) {
			lpspi->prescale = prescale;
			break;
		}
	}

	if (prescale == 8 && scldiv >= 256)
		return -EINVAL;

	reg_write(&regs->CCR, (scldiv | (scldiv << 8) | ((scldiv >> 1) << 16)));

	dev_dbg(lpspi->dev, "perclk=%d, speed=%d, prescale=%d, scldiv=%d\n",
		perclk_rate, speed, prescale, scldiv);

	return 0;
}

int spi_xfer_single(struct fsl_lpspi_slave *lpspi, unsigned int bitlen,
	unsigned long flags)
{
	int nbytes = DIV_ROUND_UP(bitlen, 8);
	u32 ts;
	struct LPSPI_Type *regs = (struct LPSPI_Type *)lpspi->base;
	int status;

	dev_dbg(lpspi->dev, "%s: bitlen %d tx_buf 0x%lx rx_buf 0x%lx\n",
		__func__, bitlen, (ulong)lpspi->tx_buf, (ulong)lpspi->rx_buf);

	while (nbytes > 0) {
		lpspi->tx_fifo(lpspi);
		nbytes -= (lpspi->wordlen / 8);
	}

	reg_write(&regs->TCR, ((lpspi->mode & 0x3) << LPSPI_TCR_CPHA_SHIFT |
		  LPSPI_TCR_FRAMESZ(lpspi->wordlen - 1) |
		  LPSPI_TCR_PRESCALE(lpspi->prescale) |
		  LPSPI_TCR_PCS(1) | LPSPI_TCR_CONT(1) | LPSPI_TCR_CONTC(0)));

	ts = get_timer(0);
	status = reg_read(&regs->SR);
	/* Wait until the TC (Transfer completed) bit is set */
	while ((status & LPSPI_SR_TCF_MASK) != 0) {
		if (get_timer(ts) > (CONFIG_SYS_HZ / 2)) {
			dev_err(lpspi->dev, "lpspi_xfer_single: TX Timeout!\n");
			return -ETIMEDOUT;
		}
		status = reg_read(&regs->RSR);
	}
	nbytes = DIV_ROUND_UP(bitlen, 8);
	ts = get_timer(0);
	while (nbytes > 0) {
		if (get_timer(ts) > (CONFIG_SYS_HZ / 2)) {
			dev_err(lpspi->dev, "lpspi_xfer_single: RX Timeout!\n");
			return -ETIMEDOUT;
		}
		if ((reg_read(&regs->FSR) & LPSPI_FSR_RXCOUNT_MASK) > 0) {
			lpspi->rx_fifo(lpspi);
			nbytes -= (lpspi->wordlen / 8);
		}
	}

	return nbytes;
}

static int fsl_lpspi_check_trans_len(unsigned int len, unsigned int wordlen)
{
	int ret = 0;

	switch (wordlen) {
	case 32:
		if (len % 4)
			ret = -EINVAL;
		break;
	case 16:
		if (len % 2)
			ret = -EINVAL;
		break;
	case 8:
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

static int fsl_lpspi_xfer_internal(struct fsl_lpspi_slave *lpspi,
				 unsigned int bitlen, const void *dout,
				 void *din, unsigned long flags)
{
	int n_bytes = DIV_ROUND_UP(bitlen, 8);
	int n_bits;
	int ret = 0;
	u32 blk_size;
	struct LPSPI_Type *regs;
	u8 watermark = 0;
	struct udevice *dev = lpspi->dev;
	struct dm_spi_slave_platdata *slave_plat = dev_get_parent_platdata(dev);

	if (!lpspi)
		return -EINVAL;

	regs = (struct LPSPI_Type *)lpspi->base;

	ret = fsl_lpspi_check_trans_len(n_bytes, lpspi->wordlen);
	if (ret)
	{
		dev_err(lpspi->dev, "fsl_lpspi: wordlen(%d) and transfer len(%d) mismatch!\n",
			DIV_ROUND_UP(lpspi->wordlen, 8), n_bytes);
		return ret;
	}

	if (dout)
		lpspi->tx_buf = dout;
	if (din)
		lpspi->rx_buf = din;

	if (n_bytes <= lpspi->fifolen)
		watermark = n_bytes;
	else
		watermark = lpspi->fifolen;
	reg_write(&regs->FCR, watermark >> 1 | (watermark >> 1) << 16);

	reg_write(&regs->TCR, ((lpspi->mode & 0x3) << LPSPI_TCR_CPHA_SHIFT |
		  LPSPI_TCR_FRAMESZ(lpspi->wordlen - 1) |
		  LPSPI_TCR_PRESCALE(lpspi->prescale) |
		  LPSPI_TCR_PCS(slave_plat->cs) | LPSPI_TCR_CONT(1) |
		  LPSPI_TCR_CONTC(0)));

	reg_write(&regs->CR, LPSPI_CR_MEN_MASK);

	if (flags & SPI_XFER_BEGIN)
		fsl_lpspi_cs_activate(lpspi);

	while (n_bytes > 0) {
		if (n_bytes < lpspi->fifolen * lpspi->wordlen / 8)
			blk_size = n_bytes;
		else
			blk_size = lpspi->fifolen * lpspi->wordlen / 8;

		n_bits = blk_size * 8;

		ret = spi_xfer_single(lpspi, n_bits, 0);
		if (ret)
			break;

		n_bytes -= blk_size;
	}

	/* Disable all interrupt */
	reg_write(&regs->IER, 0);
	/* W1C for all flags in SR */
	reg_write(&regs->SR, (0x3F << 8));
	/* Clear FIFO and disable module */
	reg_write(&regs->CR, (LPSPI_CR_RTF_MASK | LPSPI_CR_RRF_MASK));

	if (flags & SPI_XFER_END || ret)
		fsl_lpspi_cs_deactivate(lpspi);

	lpspi->tx_buf = NULL;
	lpspi->rx_buf = NULL;

	return ret;
}

static int fsl_lpspi_claim_bus_internal(struct fsl_lpspi_slave *lpspi, int cs)
{
	int ret;

	ret = spi_cfg_lpspi(lpspi, cs);
	if (ret) {
		dev_err(lpspi->dev, "fsl_lpspi: cannot setup SPI controller\n");
		return ret;
	}

	return 0;
}

static int fsl_lpspi_probe(struct udevice *bus)
{
	struct fsl_lpspi_slave *lpspi = dev_get_platdata(bus);
	int node = dev_of_offset(bus);
	const void *blob = gd->fdt_blob;
	int ret;
	int i;

	ret = gpio_request_list_by_name(bus, "cs-gpios", lpspi->cs_gpios,
					ARRAY_SIZE(lpspi->cs_gpios), 0);
	if (ret < 0) {
		dev_err(bus, "Can't get %s gpios! Error: %d", bus->name, ret);
		return ret;
	}

	for (i = 0; i < ARRAY_SIZE(lpspi->cs_gpios); i++) {
		if (!dm_gpio_is_valid(&lpspi->cs_gpios[i]))
			continue;

		ret = dm_gpio_set_dir_flags(&lpspi->cs_gpios[i],
					    GPIOD_IS_OUT | GPIOD_ACTIVE_LOW);
		if (ret) {
			dev_err(bus, "Setting cs %d error\n", i);
			return ret;
		}
	}

	lpspi->base = devfdt_get_addr(bus);
	if (lpspi->base == FDT_ADDR_T_NONE)
		return -ENODEV;

	lpspi->max_hz = fdtdec_get_int(blob, node, "spi-max-frequency",
				      500000);

	if (IS_ENABLED(CONFIG_CLK)) {
		//Enable clks
		ret = clk_get_by_name(bus, "per", &lpspi->per_clk);
		if (ret) {
			dev_err(bus, "Failed to get per clk\n");
			return ret;
		}
		ret = clk_enable(&lpspi->per_clk);
		if (ret) {
			dev_err(bus, "Failed to enable per clk, ret=%d\n", ret);
			return ret;
		}

		ret = clk_get_by_name(bus, "ipg", &lpspi->ipg_clk);
		if (ret) {
			dev_err(bus, "Failed to get ipg clk\n");
			return ret;
		}
		ret = clk_enable(&lpspi->ipg_clk);
		if (ret) {
			dev_err(bus, "Failed to enable ipg clk\n");
			return ret;
		}
	} else {
		lpspi->seq = bus->seq;
		/* To i.MX7ULP, only spi2/3 can be handled by A7 core */
		ret = enable_lpspi_clk(1, lpspi->seq);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static int fsl_lpspi_xfer(struct udevice *dev, unsigned int bitlen,
		const void *dout, void *din, unsigned long flags)
{
	struct fsl_lpspi_slave *lpspi = dev_get_platdata(dev->parent);

	return fsl_lpspi_xfer_internal(lpspi, bitlen, dout, din, flags);
}

static int fsl_lpspi_claim_bus(struct udevice *dev)
{
	struct fsl_lpspi_slave *lpspi = dev_get_platdata(dev->parent);
	struct dm_spi_slave_platdata *slave_plat = dev_get_parent_platdata(dev);

	lpspi->dev = dev;

	/* set default configurations */
	lpspi->mode = 0;
	lpspi->speed = lpspi->max_hz;
	lpspi->wordlen = 8;
	fsl_lpspi_set_word_size(lpspi, lpspi->wordlen);

	return fsl_lpspi_claim_bus_internal(lpspi, slave_plat->cs);
}

static int fsl_lpspi_release_bus(struct udevice *dev)
{
	return 0;
}

static int fsl_lpspi_set_speed(struct udevice *bus, uint speed)
{
	struct fsl_lpspi_slave *lpspi = dev_get_platdata(bus);

	lpspi->speed = speed;

	return 0;
}

static int fsl_lpspi_set_mode(struct udevice *bus, uint mode)
{
	struct fsl_lpspi_slave *lpspi = dev_get_platdata(bus);

	lpspi->mode = mode;

	return 0;
}

static int fsl_lpspi_set_wordlen(struct udevice *bus, unsigned int wordlen)
{
	struct fsl_lpspi_slave *lpspi = dev_get_platdata(bus);

	return fsl_lpspi_set_word_size(lpspi, wordlen);
}

static const struct dm_spi_ops fsl_lpspi_ops = {
	.claim_bus	= fsl_lpspi_claim_bus,
	.release_bus	= fsl_lpspi_release_bus,
	.xfer		= fsl_lpspi_xfer,
	.set_speed	= fsl_lpspi_set_speed,
	.set_mode	= fsl_lpspi_set_mode,
	.set_wordlen	= fsl_lpspi_set_wordlen,
};

static const struct udevice_id fsl_lpspi_ids[] = {
	{ .compatible = "fsl,imx7ulp-spi" },
	{ }
};

U_BOOT_DRIVER(fsl_lpspi) = {
	.name	= "fsl_lpspi",
	.id	= UCLASS_SPI,
	.of_match = fsl_lpspi_ids,
	.ops	= &fsl_lpspi_ops,
	.platdata_auto_alloc_size = sizeof(struct fsl_lpspi_slave),
	.probe	= fsl_lpspi_probe,
};
