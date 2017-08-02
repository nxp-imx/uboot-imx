# (C) Copyright 2017 Murata Electronics
# (C) Copyright 2015 Freescale Semiconductor, Inc.
# (C) Copyright 2017 NXP
#
# SPDX-License-Identifier:	GPL-2.0+
#

obj-y  := mx6ul_spriot.o

extra-$(CONFIG_USE_PLUGIN) :=  plugin.bin
$(obj)/plugin.bin: $(obj)/plugin.o
	$(OBJCOPY) -O binary --gap-fill 0xff $< $@
