# SPDX-License-Identifier: GPL-2.0+
#
# (C) Copyright 2017 Linaro Limited

ifndef CONFIG_SPL_BUILD
obj-$(CONFIG_LIBAVB) += avb_chain_partition_descriptor.o avb_cmdline.o
obj-$(CONFIG_LIBAVB) += avb_crypto.o avb_footer.o avb_hashtree_descriptor.o
obj-$(CONFIG_LIBAVB) += avb_property_descriptor.o avb_sha256.o
obj-$(CONFIG_LIBAVB) += avb_slot_verify.o avb_util.o avb_version.o
obj-$(CONFIG_LIBAVB) += avb_descriptor.o avb_hash_descriptor.o
obj-$(CONFIG_LIBAVB) += avb_kernel_cmdline_descriptor.o avb_rsa.o avb_sha512.o
obj-$(CONFIG_LIBAVB) += avb_sysdeps_posix.o avb_vbmeta_image.o
ifdef CONFIG_ARM64_NEON
CFLAGS_REMOVE_avb_sha256_neon.o := -mgeneral-regs-only
obj-$(CONFIG_LIBAVB) += avb_sha256_neon.o
endif
endif
obj-$(CONFIG_LIBAVB) += avb_crc32.o

ccflags-y = -DAVB_COMPILATION
