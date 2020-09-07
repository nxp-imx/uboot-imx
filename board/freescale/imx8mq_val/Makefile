#
# Copyright 2017 NXP
#
# SPDX-License-Identifier:      GPL-2.0+
#

obj-y += imx8mq_val.o

ifdef CONFIG_SPL_BUILD
obj-y += spl.o
obj-y += ddr/helper.o
ifdef CONFIG_TARGET_IMX8MQ_DDR3L_VAL
obj-y += ddr/ddr3l/ddr_init.o ddr/ddr3l/ddrphy_train.o
else
obj-$(CONFIG_IMX8M_DDR4) += ddr4_timing.o
endif
endif
