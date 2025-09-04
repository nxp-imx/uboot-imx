#
# Copyright 2025 NXP
#
# SPDX-License-Identifier:      GPL-2.0+
#

obj-y += imx91_frdm_imx91s.o

ifdef CONFIG_SPL_BUILD
obj-y += spl.o
ifdef CONFIG_IMX9_DRAM_INLINE_ECC
obj-$(CONFIG_IMX91_FRDM_IMX91S_LPDDR4) += lpddr4_timing_2400mts_ecc.o lpddr4_timing_1600mts_ecc.o
else
obj-$(CONFIG_IMX91_FRDM_IMX91S_LPDDR4) += lpddr4_timing_2400mts.o lpddr4_timing_1600mts.o
endif
endif
