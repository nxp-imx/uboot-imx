/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2022 NXP
 */

#ifndef __ARCH_IMX9_SYS_PROTO_H
#define __ARCH_NMX9_SYS_PROTO_H

#include <asm/mach-imx/sys_proto.h>

ulong spl_romapi_raw_seekable_read(u32 offset, u32 size, void *buf);
ulong spl_romapi_get_uboot_base(u32 image_offset, u32 rom_bt_dev);
extern unsigned long rom_pointer[];
enum boot_device get_boot_device(void);
bool is_usb_boot(void);
int mix_power_init(enum mix_power_domain pd);
void soc_power_init(void);
bool m33_is_rom_kicked(void);
int m33_prepare(void);
#endif
