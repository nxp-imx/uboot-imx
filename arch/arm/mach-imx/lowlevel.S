/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2019 NXP
 */

#include <linux/linkage.h>

ENTRY(lowlevel_init)
#ifdef CONFIG_SPL_BUILD
	mrs	x0, CurrentEL
	cmp	x0, #12
	b.eq	1f
	ret
1:
	msr daifclr, #4
	isb
	ret
#else
	mrs	x0, CurrentEL
	cmp	x0, #8
	b.eq	1f
	ret
1:
	msr daifclr, #4

	/* set HCR_EL2.AMO to catch SERROR */
	mrs	x0, hcr_el2
	orr	x0, x0, #0x20
	msr	hcr_el2, x0
	isb
	ret
#endif
ENDPROC(lowlevel_init)
