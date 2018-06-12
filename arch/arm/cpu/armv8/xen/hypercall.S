/*
 * Copyright 2018 NXP

 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <xen.h>

.globl HYPERVISOR_console_io;
.align 4;
HYPERVISOR_console_io:
	mov x16, __HYPERVISOR_console_io;
	hvc 0xEA1;
	ret;
