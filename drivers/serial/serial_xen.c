/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <dm.h>
#include <errno.h>
#include <serial.h>

extern void xenprintf(const char *buf);
extern void xenprintc(const char c);
#ifndef CONFIG_DM_SERIAL

static void xen_debug_serial_putc(const char c)
{
	/* If \n, also do \r */
	if (c == '\n')
		serial_putc('\r');

	xenprintc(c);
}

static void xen_debug_serial_puts(const char *buf)
{
	xenprintf(buf);
}

static int xen_debug_serial_start(void)
{
	return 0;
}

static void xen_debug_serial_setbrg(void)
{

}

static int xen_debug_serial_getc(void)
{
	return 0;
}

static int xen_debug_serial_tstc(void)
{
	return 0;
}

static struct serial_device xen_debug_serial_drv = {
	.name	= "xen_debug_serial",
	.start	= xen_debug_serial_start,
	.stop	= NULL,
	.setbrg	= xen_debug_serial_setbrg,
	.putc	= xen_debug_serial_putc,
	.puts	= xen_debug_serial_puts,
	.getc	= xen_debug_serial_getc,
	.tstc	= xen_debug_serial_tstc,
};

void xen_debug_serial_initialize(void)
{
	serial_register(&xen_debug_serial_drv);
}

__weak struct serial_device *default_serial_console(void)
{
	return &xen_debug_serial_drv;
}
#endif
