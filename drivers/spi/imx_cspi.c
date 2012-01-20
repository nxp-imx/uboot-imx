/*
 * (C) Copyright 2008-2012 Freescale Semiconductor, Inc.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <config.h>
#include <common.h>
#include <spi.h>
#include <asm/errno.h>
#include <linux/types.h>
#include <asm/io.h>
#include <malloc.h>

#include <imx_spi.h>

extern s32 spi_get_cfg(struct imx_spi_dev_t *dev);

static inline struct imx_spi_dev_t *to_imx_spi_slave(struct spi_slave *slave)
{
	return container_of(slave, struct imx_spi_dev_t, slave);
}

static s32 spi_reset(struct spi_slave *slave)
{
	u32 clk_src = mxc_get_clock(MXC_CSPI_CLK);
	s32 div = 0, i, reg_ctrl;
	struct imx_spi_dev_t *dev = to_imx_spi_slave(slave);
	struct spi_reg_t *reg = &(dev->reg);
	int lim = 0;
	unsigned int baud_rate_div[] = { 4, 8, 16, 32, 64, 128, 256, 512 };

	if (dev->freq == 0) {
		printf("Error: desired clock is 0\n");
		return 1;
	}

	reg_ctrl = readl(dev->base + SPI_CON_REG);
	/* Reset spi */
	writel(0, dev->base + SPI_CON_REG);
	writel((reg_ctrl | SPI_CTRL_EN), dev->base + SPI_CON_REG);

	lim = sizeof(baud_rate_div) / sizeof(unsigned int);
	if (clk_src > dev->freq) {
		div = clk_src / dev->freq;

		for (i = 0; i < lim; i++) {
			if (div <= baud_rate_div[i])
				break;
		}
	}
	debug("div = %d\n", baud_rate_div[i]);

	reg_ctrl =
	    (reg_ctrl & ~SPI_CTRL_SS_MASK) | (dev->ss << SPI_CTRL_SS_OFF);
	reg_ctrl = (reg_ctrl & ~SPI_CTRL_DATA_MASK) | (i << SPI_CTRL_DATA_OFF);
	reg_ctrl |= SPI_CTRL_MODE;	/* always set to master mode !!!! */
	reg_ctrl &= ~SPI_CTRL_EN;	/* disable spi */

	/* configuration register setup */
	reg_ctrl =
	    (reg_ctrl & ~SPI_CTRL_SSPOL) | (dev->ss_pol << SPI_CTRL_SSPOL_OFF);
	reg_ctrl =
	    (reg_ctrl & ~SPI_CTRL_SSCTL) | (dev->ssctl << SPI_CTRL_SSCTL_OFF);
	reg_ctrl =
	    (reg_ctrl & ~SPI_CTRL_SCLK_POL) | (dev->
					       sclkpol <<
					       SPI_CTRL_SCLK_POL_OFF);
	reg_ctrl =
	    (reg_ctrl & ~SPI_CTRL_SCLK_PHA) | (dev->
					       sclkpha <<
					       SPI_CTRL_SCLK_PHA_OFF);

	debug("reg_ctrl = 0x%x\n", reg_ctrl);
	writel(reg_ctrl, dev->base + SPI_CON_REG);
	/* save control register */
	reg->ctrl_reg = reg_ctrl;

	/* clear interrupt reg */
	writel(0, dev->base + SPI_INT_REG);
	writel(SPI_INT_STAT_TC, dev->base + SPI_STAT_REG);

	return 0;
}

void spi_init(void)
{
}

struct spi_slave *spi_setup_slave(unsigned int bus, unsigned int cs,
				  unsigned int max_hz, unsigned int mode)
{
	struct imx_spi_dev_t *imx_spi_slave = NULL;

	if (!spi_cs_is_valid(bus, cs))
		return NULL;

	imx_spi_slave =
	    (struct imx_spi_dev_t *)malloc(sizeof(struct imx_spi_dev_t));
	if (!imx_spi_slave)
		return NULL;

	memset(imx_spi_slave, 0, sizeof(struct imx_spi_dev_t));

	imx_spi_slave->slave.bus = bus;
	imx_spi_slave->slave.cs = cs;

	spi_get_cfg(imx_spi_slave);

	spi_io_init(imx_spi_slave);

	spi_reset(&(imx_spi_slave->slave));

	return &(imx_spi_slave->slave);
}

void spi_free_slave(struct spi_slave *slave)
{
	struct imx_spi_dev_t *imx_spi_slave;

	if (slave) {
		imx_spi_slave = to_imx_spi_slave(slave);
		free(imx_spi_slave);
	}
}

int spi_claim_bus(struct spi_slave *slave)
{
	return 0;
}

void spi_release_bus(struct spi_slave *slave)
{

}

/*
 * SPI transfer:
 *
 * See include/spi.h and http://www.altera.com/literature/ds/ds_nios_spi.pdf
 * for more informations.
 */
int spi_xfer(struct spi_slave *slave, unsigned int bitlen, const void *dout,
	     void *din, unsigned long flags)
{
	s32 val = SPI_RETRY_TIMES;
	u32 *p_buf;
	u32 reg;
	s32 len = 0, ret_val = 0;
	s32 burst_bytes = bitlen >> 3;
	struct imx_spi_dev_t *dev = to_imx_spi_slave(slave);
	struct spi_reg_t *spi_reg = &(dev->reg);

	if (!slave)
		return -1;

	if ((bitlen % 8) != 0)
		burst_bytes++;

	if (burst_bytes > (dev->fifo_sz)) {
		printf("Error: maximum burst size is 0x%x bytes, asking 0x%x\n",
		       dev->fifo_sz, burst_bytes);
		return -1;
	}

	if (flags & SPI_XFER_BEGIN) {
		spi_cs_activate(slave);

		if (spi_reg->ctrl_reg == 0) {
			printf
			    ("Error: spi(base=0x%x) has not been initialized\n",
			     dev->base);
			return -1;
		}

		spi_reg->ctrl_reg = (spi_reg->ctrl_reg & ~SPI_CTRL_BURST_MASK) |
		    ((bitlen - 1) << SPI_CTRL_BURST_OFF);
		writel(spi_reg->ctrl_reg | SPI_CTRL_EN,
		       dev->base + SPI_CON_REG);
		debug("ctrl_reg=0x%x\n", readl(dev->base + SPI_CON_REG));

		/* move data to the tx fifo */
		if (dout) {
			for (p_buf = (u32 *) dout, len = burst_bytes; len > 0;
			     p_buf++, len -= 4)
				writel(*p_buf, dev->base + SPI_TX_DATA);
		}

		reg = readl(dev->base + SPI_CON_REG);
		reg |= SPI_CTRL_REG_XCH_BIT;	/* set xch bit */
		debug("control reg = 0x%08x\n", reg);
		writel(reg, dev->base + SPI_CON_REG);

		/* poll on the TC bit (transfer complete) */
		while ((val-- > 0) &&
		       (((reg =
			  readl(dev->base + SPI_STAT_REG)) & SPI_INT_STAT_TC) ==
			0));

		/* clear the TC bit */
		writel(reg | SPI_INT_STAT_TC, dev->base + SPI_STAT_REG);
		if (val <= 0) {
			printf
			    ("Error: re-tried %d times without response. Give up\n",
			     SPI_RETRY_TIMES);
			ret_val = -1;
			goto error;
		}
	}

	/* move data in the rx buf */
	if (flags & SPI_XFER_END) {
		if (din) {
			for (p_buf = (u32 *) din, len = burst_bytes; len > 0;
			     p_buf++, len -= 4)
				*p_buf = readl(dev->base + SPI_RX_DATA);
		}
	}
error:
	spi_cs_deactivate(slave);
	return ret_val;
}

int spi_cs_is_valid(unsigned int bus, unsigned int cs)
{
	return 1;
}

void spi_cs_activate(struct spi_slave *slave)
{
	struct imx_spi_dev_t *dev = to_imx_spi_slave(slave);

	spi_io_init(dev);
}

void spi_cs_deactivate(struct spi_slave *slave)
{
	struct imx_spi_dev_t *dev = to_imx_spi_slave(slave);

	writel(0, dev->base + SPI_CON_REG);
}
