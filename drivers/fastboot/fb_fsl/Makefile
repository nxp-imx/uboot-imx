#
# Copyright 2019 NXP
#
# SPDX-License-Identifier:	GPL-2.0+
#

ccflags-y += -I./lib

ifndef CONFIG_SPL_BUILD
obj-y += fb_fsl_dev.o fb_fsl_boot.o fb_fsl_command.o fb_fsl_common.o fb_fsl_getvar.o fb_fsl_partitions.o
obj-$(CONFIG_FASTBOOT_LOCK) += fastboot_lock_unlock.o
obj-$(CONFIG_BCB_SUPPORT) += command.o bcb.o
obj-$(CONFIG_VIRTUAL_AB_SUPPORT) += fb_fsl_virtual_ab.o
endif
