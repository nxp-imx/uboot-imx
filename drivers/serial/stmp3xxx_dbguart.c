/*
 * (c) 2007 Sascha Hauer <s.hauer@pengutronix.de>
 *
 * (C) Copyright 2009 Freescale Semiconductor, Inc.
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
 *
 */
#include <common.h>

#ifdef CONFIG_STMP3XXX_DBGUART

#include "stmp3xxx_dbguart.h"

DECLARE_GLOBAL_DATA_PTR;

/*
 * Set baud rate. The settings are always 8n1:
 * 8 data bits, no parity, 1 stop bit
 */
void serial_setbrg(void)
{
	u32 cr, lcr_h;
	u32 quot;

	/* Disable everything */
	cr = REG_RD(DBGUART_BASE + UARTDBGCR);
	REG_WR(DBGUART_BASE + UARTDBGCR, 0);

	/* Calculate and set baudrate */
	quot = (CONFIG_DBGUART_CLK * 4)	/ gd->baudrate;
	REG_WR(DBGUART_BASE + UARTDBGFBRD, quot & 0x3f);
	REG_WR(DBGUART_BASE + UARTDBGIBRD, quot >> 6);

	/* Set 8n1 mode, enable FIFOs */
	lcr_h = WLEN8 | FEN;
	REG_WR(DBGUART_BASE + UARTDBGLCR_H, lcr_h);

	/* Enable Debug UART */
	REG_WR(DBGUART_BASE + UARTDBGCR, cr);
}

int serial_init(void)
{
	u32 cr;

	/* Disable UART */
	REG_WR(DBGUART_BASE + UARTDBGCR, 0);

	/* Mask interrupts */
	REG_WR(DBGUART_BASE + UARTDBGIMSC, 0);

	/* Set default baudrate */
	serial_setbrg();

	/* Enable UART */
	cr = TXE | RXE | UARTEN;
	REG_WR(DBGUART_BASE + UARTDBGCR, cr);

	return 0;
}

/* Send a character */
void serial_putc(const char c)
{
	/* Wait for room in TX FIFO */
	while (REG_RD(DBGUART_BASE + UARTDBGFR) & TXFF)
		;

	/* Write the data byte */
	REG_WR(DBGUART_BASE + UARTDBGDR, c);

	if (c == '\n')
		serial_putc('\r');
}

void serial_puts(const char *s)
{
	while (*s) {
		serial_putc(*s++);
	}
}

/* Test whether a character is in TX buffer */
int serial_tstc(void)
{
	/* Check if RX FIFO is not empty */
	return !(REG_RD(DBGUART_BASE + UARTDBGFR) & RXFE);
}

/* Receive character */
int serial_getc(void)
{
	/* Wait while TX FIFO is empty */
	while (REG_RD(DBGUART_BASE + UARTDBGFR) & RXFE)
		;

	/* Read data byte */
	return REG_RD(DBGUART_BASE + UARTDBGDR) & 0xff;
}

#endif /* CONFIG_STMP378X_DBGUART */
