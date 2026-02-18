/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * conga-smx95 U-Boot Configuration
 *
 * Copyright (C) 2026 Daksh Patel
 */

#ifndef __CONGA_SMX95_H
#define __CONGA_SMX95_H

/* Memory Layout */
#define CFG_SYS_SDRAM_BASE        0x80000000
#define CFG_SYS_INIT_RAM_ADDR     0x00100000
#define CFG_SYS_INIT_RAM_SIZE     0x00020000

/* Console */
#define CFG_SYS_UART_BASE         0x44380000  /* Adjust if needed */
#define CFG_BAUDRATE_TABLE        { 115200 }

/* Environment */
#define CFG_EXTRA_ENV_SETTINGS \
    "console=ttyLP0,115200\0" \
    "fdtfile=imx95-conga-smx95.dtb\0" \
    "bootcmd=run distro_bootcmd\0"

/* Boot Targets Priority */
#define BOOT_TARGET_DEVICES(func) \
    func(SF, sf, 0) \
    func(MMC, mmc, 1) \
    func(MMC, mmc, 0)

#endif /* __CONGA_SMX95_H */
