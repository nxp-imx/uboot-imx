# SPDX-License-Identifier: GPL-2.0+
#
# (C) Copyright 2000-2006
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# (C) Copyright 2011 Freescale Semiconductor, Inc.
# Copyright 2018 NXP
#

obj-y	:= soc.o clock.o
obj-$(CONFIG_SPL_BUILD)	     += ddr.o
obj-$(CONFIG_MP)             += mp.o
obj-$(CONFIG_MX6UL_LITESOM)  += litesom.o
obj-$(CONFIG_MX6UL_OPOS6UL)  += opos6ul.o
ifdef CONFIG_MX6UL
obj-$(CONFIG_CMD_BEE)        += bee.o
endif
obj-$(CONFIG_MODULE_FUSE)        += module_fuse.o
