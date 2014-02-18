# (C) Copyright 2014 Freescale Semiconductor, Inc.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#

include $(TOPDIR)/config.mk

LIB    = $(obj)lib$(BOARD).o

COBJS  := mx6sx_17x17_arm2.o

SRCS   := $(COBJS:.o=.c)
OBJS   := $(addprefix $(obj),$(COBJS))

all:	$(LIB) plugin.bin

$(LIB):        $(obj).depend $(OBJS)
	$(call cmd_link_o_target, $(OBJS))

plugin.bin: plugin.o
	$(OBJCOPY) -O binary --gap-fill 0xff $< $@

#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################
