#
# Copyright 2017 NXP
#
# SPDX-License-Identifier:      GPL-2.0+
#

obj-y += imx8mq_evk.o

ifdef CONFIG_SPL_BUILD
obj-y += spl.o
ifdef CONFIG_IMX8MQ_4GB_DDR_TIMING
obj-$(CONFIG_IMX8M_LPDDR4) += lpddr4_timing_4g.o
else
obj-$(CONFIG_IMX8M_LPDDR4) += lpddr4_timing_b0.o lpddr4_timing.o
endif
endif
