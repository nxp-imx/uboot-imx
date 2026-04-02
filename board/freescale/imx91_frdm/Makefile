#
# Copyright 2025 NXP
#
# SPDX-License-Identifier:      GPL-2.0+
#

obj-y += imx91_frdm.o

ifdef CONFIG_SPL_BUILD
obj-y += spl.o
ifdef CONFIG_IMX9_DRAM_INLINE_ECC
obj-$(CONFIG_IMX91_FRDM_LPDDR4) += lpddr4_2400mts_ecc_timing.o
else
obj-$(CONFIG_IMX91_FRDM_LPDDR4) += lpddr4_2400mts_timing.o
endif
endif
