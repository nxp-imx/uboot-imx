/*
 * (C) Copyright 2008-2009 Freescale Semiconductor, Inc.
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

static u32 pmic_tx, pmic_rx;

/*!
 * To read/write to a PMIC register. For write, it does another read for the
 * actual register value.
 *
 * @param	reg			register number inside the PMIC
 * @param	val			data to be written to the register; don't care for read
 * @param	write		0 for read; 1 for write
 *
 * @return				the actual data in the PMIC register
 */
u32 pmic_reg(struct spi_slave *slave, u32 reg, u32 val, u32 write)
{
	if (!slave)
		return 0;

	if (reg > 63 || write > 1) {
		printf("<reg num> = %d is invalide. Should be less then 63\n",
			reg);
		return 0;
	}
	pmic_tx = (write << 31) | (reg << 25) | (val & 0x00FFFFFF);
	debug("reg=0x%x, val=0x%08x\n", reg, pmic_tx);

	if (spi_xfer(slave, 4 << 3, (u8 *)&pmic_tx, (u8 *)&pmic_rx,
			SPI_XFER_BEGIN | SPI_XFER_END)) {
		return -1;
	}

	if (write) {
		pmic_tx &= ~(1 << 31);
		if (spi_xfer(slave, 4 << 3, (u8 *)&pmic_tx, (u8 *)&pmic_rx,
			SPI_XFER_BEGIN | SPI_XFER_END)) {
			return -1;
		}
	}

	return pmic_rx;
}

void show_pmic_info(struct spi_slave *slave)
{
	volatile u32 rev_id;

	if (!slave)
		return;

	rev_id = pmic_reg(slave, 7, 0, 0);
	debug("PMIC ID: 0x%08x [Rev: ", rev_id);
	switch (rev_id & 0x1F) {
	case 0x1:
		printf("1.0");
		break;
	case 0x9:
		printf("1.1");
		break;
	case 0xA:
		printf("1.2");
		break;
	case 0x10:
		printf("2.0");
		break;
	case 0x11:
		printf("2.1");
		break;
	case 0x18:
		printf("3.0");
		break;
	case 0x19:
		printf("3.1");
		break;
	case 0x1A:
		printf("3.2");
		break;
	case 0x2:
		printf("3.2A");
		break;
	case 0x1B:
		printf("3.3");
		break;
	case 0x1D:
		printf("3.5");
		break;
	default:
		printf("unknown");
		break;
	}
	printf("]\n");
}

struct spi_slave *spi_pmic_probe(void)
{
	return spi_setup_slave(0, CONFIG_IMX_SPI_PMIC_CS, 2500000, 0);
}

void spi_pmic_free(struct spi_slave *slave)
{
	if (slave)
		spi_free_slave(slave);
}
