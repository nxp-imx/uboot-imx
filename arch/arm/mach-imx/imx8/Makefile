#
# Copyright 2018 NXP
#
# SPDX-License-Identifier:	GPL-2.0+
#

obj-y += cpu.o iomux.o fuse.o lpcg.o clock.o
obj-y += lowlevel_init.o
obj-$(CONFIG_SPL_BUILD) += image.o
obj-$(CONFIG_SPL_BUILD) += parser.o
ifneq ($(CONFIG_SPL_BUILD),y)
obj-y += partition.o
endif
obj-$(CONFIG_AHAB_BOOT) += ahab.o
obj-$(CONFIG_VIDEO_IMXDPUV1) += video_common.o
obj-$(CONFIG_SNVS_SEC_SC) += snvs_security_sc.o
