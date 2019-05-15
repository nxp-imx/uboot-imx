# SPDX-License-Identifier: GPL-2.0+

obj-y += fb_common.o
ifndef CONFIG_FSL_FASTBOOT
obj-y += fb_command.o
obj-y += fb_getvar.o
obj-$(CONFIG_FASTBOOT_FLASH_MMC) += fb_mmc.o
obj-$(CONFIG_FASTBOOT_FLASH_NAND) += fb_nand.o
else
obj-y += fb_fsl/
endif
