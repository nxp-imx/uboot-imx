#
# Copyright 2020 NXP
#
# SPDX-License-Identifier:      GPL-2.0+
#

obj-y += imx8mm_ab2.o

ifdef CONFIG_SPL_BUILD
obj-y += spl.o
obj-$(CONFIG_TARGET_IMX8MM_AB2) += lpddr4_imx8mm_som.o
obj-$(CONFIG_TARGET_IMX8MM_DDR4_AB2) += ddr4_imx8mm_som.o
obj-$(CONFIG_TARGET_IMX8MN_AB2) += lpddr4_imx8mn_som.o
obj-$(CONFIG_TARGET_IMX8MN_DDR4_AB2) += ddr4_imx8mn_som.o
endif
