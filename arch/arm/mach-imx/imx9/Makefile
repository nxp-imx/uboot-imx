# SPDX-License-Identifier: GPL-2.0+
#
# Copyright 2023 NXP

obj-y += lowlevel_init.o

ifeq ($(CONFIG_SCMI_FIRMWARE),y)
obj-y += scmi/
else
obj-y += native/
endif

ifneq ($(CONFIG_SPL_BUILD),y)
obj-y += imx_bootaux.o misc.o
endif
