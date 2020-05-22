# SPDX-License-Identifier: GPL-2.0+
#
# (C) Copyright 2000-2006
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
# (C) Copyright 2011 Freescale Semiconductor, Inc.
# Copyright 2020 NXP

MKIMAGEFLAGS_u-boot.s32 = -T $(CONFIG_MKIMAGE_T)
u-boot.s32: MKIMAGEOUTPUT = u-boot.s32.log
u-boot.s32: u-boot.bin FORCE
	$(call if_changed,mkimage)
