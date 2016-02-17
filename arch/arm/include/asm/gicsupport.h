/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2016 NXP
 *
 */
 /* Functions declared in arch/arm/cpu/armv8/s32/gicsupport.c */
int gic_irq_status(unsigned int irq);
int gic_register_handler(int irq,
			 void (*handler)(struct pt_regs *pt_regs,
					 unsigned int esr),
			 int type, const char *name);
