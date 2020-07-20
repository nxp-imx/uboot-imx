/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2014, Freescale Semiconductor
 * (C) Copyright 2017 NXP
 *
 */

#ifndef _S32V234_MP_H
#define _S32V234_MP_H

/*
 * spin table is defined as
 * struct {
 *      uint64_t entry_addr;
 *      uint64_t r3;
 *      uint64_t pir;
 * };
 * we pad this struct to 64 bytes so each entry is in its own cacheline
 */
#define ENTRY_SIZE	64
#define BOOT_ENTRY_ADDR	0
#define BOOT_ENTRY_R3	1
#define BOOT_ENTRY_PIR	2
#define NUM_BOOT_ENTRY	4	/* pad to 64 bytes */
#define SIZE_BOOT_ENTRY		(NUM_BOOT_ENTRY * sizeof(u64))

#define id_to_core(x)	(((x) & 3) | ((x) >> 8))
#ifndef __ASSEMBLY__
extern u64 __spin_table[];
extern u64 __real_cntfrq;
extern u64 *secondary_boot_page;
extern size_t __secondary_boot_page_size;
int fsl_s32v234_wake_seconday_cores(void);
void *get_spin_tbl_addr(void);
phys_addr_t determine_mp_bootpg(void);
void secondary_boot_func(void);
#endif
#endif /* _FSL_CH2_MP_H */
