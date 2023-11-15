# SPDX-License-Identifier: GPL-2.0+
#
# Copyright 2022 NXP

obj-y += lowlevel_init.o
obj-y += soc.o clock.o clock_root.o trdc.o

ifneq ($(CONFIG_SPL_BUILD),y)
obj-y += imx_bootaux.o
endif
