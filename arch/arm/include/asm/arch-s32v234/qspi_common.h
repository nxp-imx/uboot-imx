/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2016,2020 NXP
 *
 */

#ifndef QSPI_COMMON_H_
#define QSPI_COMMON_H_

void qspi_iomux(void);

#ifdef CONFIG_S32V234_FLASH
int do_qspinor_setup(cmd_tbl_t *cmdtp, int flag, int argc,
		     char * const argv[]);
#else
int do_qspinor_setup(cmd_tbl_t *cmdtp, int flag, int argc,
		     char * const argv[])
{
	printf("SD/eMMC is disabled. SPI flash is active and can be used!\n");
	qspi_iomux();
	return 0;
}
#endif

#endif /* QSPI_COMMON_H_ */
