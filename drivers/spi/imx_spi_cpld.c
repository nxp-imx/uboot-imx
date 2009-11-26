/*
 * (C) Copyright 2009 Freescale Semiconductor, Inc.
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

#include <imx_spi.h>
#include <asm/arch/imx_spi_cpld.h>

static struct spi_slave *cpld_slave;

void cpld_reg_write(u32 offset, u32 val)
{
	cpld_reg_xfer(offset, val, 0);
	cpld_reg_xfer(offset + 0x2, (val >> 16), 0);
}

u32 cpld_reg_read(u32 offset)
{
	return cpld_reg_xfer(offset, 0x0, 1) | \
		(cpld_reg_xfer(offset + 0x2, 0x0, 1) << 16);
}

/*!
 * To read/write to a CPLD register.
 *
 * @param   reg         register number inside the CPLD
 * @param   val         data to be written to the register; don't care for read
 * @param   read        0 for write; 1 for read
 *
 * @return              the actual data in the CPLD register
 */
unsigned int cpld_reg_xfer(unsigned int reg, unsigned int val,
			   unsigned int read)
{
	unsigned int local_val1, local_val2;
	unsigned int g_tx_buf[2], g_rx_buf[2];

	reg >>= 1;

	local_val1 = (read << 13) | ((reg & 0x0001FFFF) >> 5) | 0x00001000;
	if (read)
		local_val2 = (((reg & 0x0000001F) << 27) | 0x0200001f);
	else
		local_val2 =
		    (((reg & 0x0000001F) << 27) | ((val & 0x0000FFFF) << 6) |
		     0x03C00027);

	*g_tx_buf = local_val1;
	*(g_tx_buf + 1) = local_val2;

	if (read) {
		if (spi_xfer(cpld_slave, 46, (u8 *) g_tx_buf, (u8 *) g_rx_buf,
			     SPI_XFER_BEGIN | SPI_XFER_END)) {
			return -1;
		}
	} else {
		if (spi_xfer(cpld_slave, 46, (u8 *) g_tx_buf, (u8 *) g_rx_buf,
			     SPI_XFER_BEGIN)) {
			return -1;
		}
	}
	return ((*(g_rx_buf + 1)) >> 6) & 0xffff;
}

struct spi_slave *spi_cpld_probe()
{
	u32 reg;
	cpld_slave = spi_setup_slave(0, 0, 25000000, 0);

	udelay(1000);

	/* Reset interrupt status reg */
	cpld_reg_write(PBC_INT_REST, 0x1F);
	cpld_reg_write(PBC_INT_REST, 0);
	cpld_reg_write(PBC_INT_MASK, 0xFFFF);
	/* Reset the XUART and Ethernet controllers */
	reg = cpld_reg_read(PBC_SW_RESET);
	reg |= 0x9;
	cpld_reg_write(PBC_SW_RESET, reg);
	reg &= ~0x9;
	cpld_reg_write(PBC_SW_RESET, reg);

	return cpld_slave;
}

void mxc_cpld_spi_init(void)
{
	spi_cpld_probe();
}

void spi_cpld_free(struct spi_slave *slave)
{
	if (slave)
		spi_free_slave(slave);
}
