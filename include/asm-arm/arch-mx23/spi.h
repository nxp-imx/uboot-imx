/*
 * Copyright (C) 2008 Embedded Alley Solutions Inc.
 *
 * (C) Copyright 2009-2010 Freescale Semiconductor, Inc.
 *
 * SSP/SPI driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef SPI_H
#define SPI_H

#include <config.h>
#include <common.h>
#include <asm/arch/ssp.h>

/*
 * Flags to set SPI mode
 */
#define SPI_PHASE	0x1 /* Set phase to 1 */
#define SPI_POLARITY	0x2 /* Set polarity to 1 */

/* Various flags to control SPI transfers */
#define SPI_START	0x1	/* Lock CS signal */
#define SPI_STOP	0x2	/* Unlock CS signal */

/*
 * Init SSPx interface, must be called first
 */
void spi_init(void);

/*
 * Set phase, polarity and CS number (SS0, SS1, SS2)
 */
void spi_set_cfg(unsigned int bus, unsigned int cs, unsigned long mode);


/*
 * Send @rx_len bytes from @dout, then receive @rx_len bytes
 * saving them to @din
 */
void spi_txrx(const char *dout, unsigned int tx_len, char *din,
	       unsigned int rx_len, unsigned long flags);


/* Lock/unlock SPI bus */
static inline void spi_lock(void)
{
	disable_interrupts();
}

static inline void spi_unlock(void)
{
	enable_interrupts();
}

#endif /* SPI_H */
