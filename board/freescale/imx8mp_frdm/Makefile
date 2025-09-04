#
# Copyright 2025 NXP
#
# SPDX-License-Identifier:      GPL-2.0+
#

obj-y += imx8mp_frdm.o

ifdef CONFIG_SPL_BUILD
obj-y += spl.o
ifdef CONFIG_IMX8M_LPDDR4_FREQ0_3200MTS
obj-y += lpddr4_timing_ndm.o
else
obj-$(CONFIG_IMX8M_LPDDR4) += lpddr4_timing.o
endif
endif
