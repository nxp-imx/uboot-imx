#
# Copyright 2022 NXP
#
# SPDX-License-Identifier:      GPL-2.0+
#

obj-y += imx93_qsb.o

ifdef CONFIG_SPL_BUILD
obj-y += spl.o
ifdef CONFIG_IMX9_DRAM_INLINE_ECC
obj-$(CONFIG_TARGET_IMX93_9X9_QSB) += lpddr4_timing_ecc.o
obj-$(CONFIG_TARGET_IMX91P_9X9_QSB) += lpddr4_timing_1600mts_ecc.o lpddr4_timing_2400mts_ecc.o
else
obj-$(CONFIG_TARGET_IMX93_9X9_QSB) += lpddr4_timing.o
obj-$(CONFIG_TARGET_IMX91P_9X9_QSB) += lpddr4_timing_1600mts.o lpddr4_timing_2400mts.o
endif
endif
