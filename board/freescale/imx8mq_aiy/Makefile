#
# Copyright 2016 Freescale Semiconductor
#
# SPDX-License-Identifier:      GPL-2.0+
#

obj-y += imx8m_aiy.o

ifdef CONFIG_SPL_BUILD
obj-y += spl.o
ifdef CONFIG_AIY_LPDDR4_3G
obj-$(CONFIG_IMX8M_LPDDR4) += ddr/lpddr4_timing_micron_3g.o
else
obj-$(CONFIG_IMX8M_LPDDR4) += ddr/lpddr4_timing_kingston_2g.o
endif
obj-$(CONFIG_IMX8M_LPDDR4) += ddr/lpddr4_timing_1g.o
endif
