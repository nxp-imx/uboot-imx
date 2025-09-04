#
# Copyright 2025 NXP
#
# SPDX-License-Identifier:      GPL-2.0+
#

obj-y += imx93_frdm.o

ifdef CONFIG_SPL_BUILD
obj-y += spl.o
ifdef CONFIG_IMX9_DRAM_INLINE_ECC
obj-y += lpddr4x_ecc_1gb_timing.o lpddr4x_ecc_2gb_timing.o
else
obj-y += lpddr4x_1gb_timing.o  lpddr4x_2gb_timing.o
endif
endif
