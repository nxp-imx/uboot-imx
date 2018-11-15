/*
 * Copyright 2018 NXP

 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <xen.h>

.globl HYPERVISOR_console_io;
.globl HYPERVISOR_event_channel_op;
.globl HYPERVISOR_hvm_op;
.globl HYPERVISOR_sched_op;

.align 4;
HYPERVISOR_console_io:
	mov x16, __HYPERVISOR_console_io;
	hvc 0xEA1;
	ret;

HYPERVISOR_event_channel_op:
	mov x16, __HYPERVISOR_event_channel_op;
	hvc 0xEA1;
	ret;

HYPERVISOR_hvm_op:
	mov x16, __HYPERVISOR_hvm_op;
	hvc 0xEA1;
	ret;

HYPERVISOR_sched_op:
	mov x16, __HYPERVISOR_sched_op;
	hvc 0xEA1;
	ret;
