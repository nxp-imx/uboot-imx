#
# Copyright 2017 Freescale Semiconductor
#
# SPDX-License-Identifier:      GPL-2.0+
#

obj-y += imx8m_arm2.o

ifdef CONFIG_SPL_BUILD
obj-y += spl.o
obj-y += ddr/helper.o
ifdef CONFIG_TARGET_IMX8MQ_DDR3L_ARM2
obj-y += ddr/ddr3l/ddr_init.o ddr/ddr3l/ddrphy_train.o
else
obj-y += ddr/ddr4/ddr_init.o ddr/ddr4/ddrphy_train.o
endif
endif
