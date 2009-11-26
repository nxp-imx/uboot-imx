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

#ifndef _IMX_SPI_CPLD_H_
#define _IMX_SPI_CPLD_H_

#include <linux/types.h>

#define PBC_LED_CTRL		0x20000
#define PBC_SB_STAT		0x20008
#define PBC_ID_AAAA		0x20040
#define PBC_ID_5555		0x20048
#define PBC_VERSION		0x20050
#define PBC_ID_CAFE		0x20058
#define PBC_INT_STAT		0x20010
#define PBC_INT_MASK		0x20038
#define PBC_INT_REST		0x20020
#define PBC_SW_RESET		0x20060

void cpld_reg_write(u32 offset, u32 val);
u32 cpld_reg_read(u32 offset);
struct spi_slave *spi_cpld_probe();
void spi_cpld_free(struct spi_slave *slave);
unsigned int cpld_reg_xfer(unsigned int reg, unsigned int val,
				unsigned int read);

#endif				/* _IMX_SPI_CPLD_H_ */
