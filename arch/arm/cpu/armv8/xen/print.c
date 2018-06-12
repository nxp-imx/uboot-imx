/*
 * Copyright 2018 NXP
 *
 * Peng Fan <peng.fan@nxp.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <hypercall.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <xen.h>

/*
 * To non privileged domain, need CONFIG_VERBOSE_DEBUG in XEN to 
 * get output.
 */
void xenprintf(const char *buf)
{
	(void)HYPERVISOR_console_io(CONSOLEIO_write, strlen(buf), buf);
	return;
}

void xenprintc(const char c)
{
	(void)HYPERVISOR_console_io(CONSOLEIO_write, 1, &c);
	return;
}
