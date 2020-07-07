#
# Copyright 2018 NXP
#
# SPDX-License-Identifier:	GPL-2.0+
#

obj-y += cpu.o iomux.o misc.o lowlevel_init.o lpcg.o clock.o
obj-$(CONFIG_OF_SYSTEM_SETUP) += fdt.o
obj-$(CONFIG_AHAB_BOOT) += ahab.o

ifdef CONFIG_SPL_BUILD
obj-$(CONFIG_SPL_LOAD_IMX_CONTAINER) += image.o parse-container.o
else
obj-y += partition.o
obj-$(CONFIG_SNVS_SEC_SC) += snvs_security_sc.o
endif
