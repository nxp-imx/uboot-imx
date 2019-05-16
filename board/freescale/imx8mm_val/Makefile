#
# Copyright 2018 NXP
#
# SPDX-License-Identifier:      GPL-2.0+
#

obj-y += imx8mm_val.o

ifdef CONFIG_SPL_BUILD
obj-y += spl.o
obj-$(CONFIG_IMX8M_DDR4) += ddr4_timing.o
obj-$(CONFIG_IMX8M_DDR3L) += ddr3l_timing.o
endif
