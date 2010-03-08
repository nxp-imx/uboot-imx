/*
 * (c) 2007 Sascha Hauer <s.hauer@pengutronix.de>
 *
 * (C) Copyright 2009-2010 Freescale Semiconductor, Inc.
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
#include <asm/arch/regs-uartdbg.h>

DECLARE_GLOBAL_DATA_PTR;

/*
 * Set baud rate. The settings are always 8n1:
 * 8 data bits, no parity, 1 stop bit
 */
void serial_setbrg(void)
{
	u32 cr;
	u32 quot;

	/* Disable everything */
	cr = REG_RD(REGS_UARTDBG_BASE, HW_UARTDBGCR);
	REG_WR(REGS_UARTDBG_BASE, HW_UARTDBGCR, 0);

	/* Calculate and set baudrate */
	quot = (CONFIG_UARTDBG_CLK * 4)	/ gd->baudrate;
	REG_WR(REGS_UARTDBG_BASE, HW_UARTDBGFBRD, quot & 0x3f);
	REG_WR(REGS_UARTDBG_BASE, HW_UARTDBGIBRD, quot >> 6);

	/* Set 8n1 mode, enable FIFOs */
	REG_WR(REGS_UARTDBG_BASE, HW_UARTDBGLCR_H,
		BM_UARTDBGLCR_H_WLEN | BM_UARTDBGLCR_H_FEN);

	/* Enable Debug UART */
	REG_WR(REGS_UARTDBG_BASE, HW_UARTDBGCR, cr);
}

int serial_init(void)
{
	/* Disable UART */
	REG_WR(REGS_UARTDBG_BASE, HW_UARTDBGCR, 0);

	/* Mask interrupts */
	REG_WR(REGS_UARTDBG_BASE, HW_UARTDBGIMSC, 0);

	/* Set default baudrate */
	serial_setbrg();

	/* Enable UART */
	REG_WR(REGS_UARTDBG_BASE, HW_UARTDBGCR,
		BM_UARTDBGCR_TXE | BM_UARTDBGCR_RXE | BM_UARTDBGCR_UARTEN);

	return 0;
}

/* Send a character */
void serial_putc(const char c)
{
	/* Wait for room in TX FIFO */
	while (REG_RD(REGS_UARTDBG_BASE, HW_UARTDBGFR) & BM_UARTDBGFR_TXFF)
		;

	/* Write the data byte */
	REG_WR(REGS_UARTDBG_BASE, HW_UARTDBGDR, c);

	if (c == '\n')
		serial_putc('\r');
}

void serial_puts(const char *s)
{
	while (*s)
		serial_putc(*s++);
}

/* Test whether a character is in TX buffer */
int serial_tstc(void)
{
	/* Check if RX FIFO is not empty */
	return !(REG_RD(REGS_UARTDBG_BASE, HW_UARTDBGFR) & BM_UARTDBGFR_RXFE);
}

/* Receive character */
int serial_getc(void)
{
	/* Wait while TX FIFO is empty */
	while (REG_RD(REGS_UARTDBG_BASE, HW_UARTDBGFR) & BM_UARTDBGFR_RXFE)
		;

	/* Read data byte */
	return REG_RD(REGS_UARTDBG_BASE, HW_UARTDBGDR) & 0xff;
}

