# SPDX-License-Identifier: GPL-2.0+
#
# (C) Copyright 2013-2016, Freescale Semiconductor, Inc.
ccflags-y += -I./board/freescale/common/

obj-$(CONFIG_ARCH_S32V234)		+= soc.o
obj-$(CONFIG_ARCH_S32V234)		+= cpu.o
obj-$(CONFIG_ARCH_S32V234)		+= clock.o
obj-$(CONFIG_ARCH_S32V234)		+= lowlevel.o
obj-$(CONFIG_S32V234_LPDDR2)		+= lpddr2.o
obj-$(CONFIG_S32V234_DDR3)		+= ddr3.o
obj-$(CONFIG_MP)			+= mp.o
obj-$(CONFIG_OF_LIBFDT)			+= fdt.o
obj-$(CONFIG_S32V234_FLASH)		+= qspi_driver.o
obj-$(CONFIG_S32V234_USES_FLASH)	+= qspi_common.o
obj-$(CONFIG_FSL_DCU_FB)		+= dcu.o
