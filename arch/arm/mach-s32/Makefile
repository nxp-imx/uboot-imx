# SPDX-License-Identifier: GPL-2.0+
#
# (C) Copyright 2000-2006
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
# (C) Copyright 2011 Freescale Semiconductor, Inc.
# Copyright 2019 NXP

quiet_cmd_cpp_cfg = CFGS    $@
      cmd_cpp_cfg = $(CPP) $(cpp_flags) -x c -o $@ $<

# mkimage source config file
IMX_CONFIG = $(CONFIG_IMX_CONFIG:"%"=%)

# How to create a cpp processed config file, they all use the same source
%.cfgout: $(IMX_CONFIG) FORCE
	$(Q)mkdir -p $(dir $@)
	$(call if_changed_dep,cpp_cfg)

MKIMAGEFLAGS_u-boot.s32 = -n $(filter-out $< $(PHONY),$^) -T imximage \
	-e $(CONFIG_SYS_TEXT_BASE)
u-boot.s32: MKIMAGEOUTPUT = u-boot.s32.log

u-boot.s32: u-boot.bin u-boot.cfgout FORCE
	$(call if_changed,mkimage)
