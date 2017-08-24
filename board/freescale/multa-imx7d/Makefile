# Copyright 2017 NXP
#
# SPDX-License-Identifier:	GPL-2.0+
#

obj-y  := multa-imx7d.o

extra-$(CONFIG_USE_PLUGIN) :=  plugin.bin
$(obj)/plugin.bin: $(obj)/plugin.o
	$(OBJCOPY) -O binary --gap-fill 0xff $< $@
