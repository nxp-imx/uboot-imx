#
# Copyright 2018 NXP
#
# SPDX-License-Identifier:	GPL-2.0+
#

obj-y += cpu.o iomux.o fuse.o lpcg.o clock.o
ifneq ($(CONFIG_SPL_BUILD),y)
obj-y += partition.o
endif
obj-$(CONFIG_AHAB_BOOT) += ahab.o
obj-$(CONFIG_VIDEO_IMXDPUV1) += video_common.o
