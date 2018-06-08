ccflags-y += -DAVB_COMPILATION

ifndef CONFIG_SPL_BUILD
obj-y += avb_descriptor.o \
	avb_kernel_cmdline_descriptor.o \
	avb_sha512.o \
	avb_vbmeta_image.o \
	avb_chain_partition_descriptor.o \
	avb_footer.o \
	avb_property_descriptor.o \
	avb_slot_verify.o \
	avb_hash_descriptor.o \
	avb_rsa.o \
	avb_crypto.o \
	avb_hashtree_descriptor.o \
	avb_sha256.o \
	avb_util.o \
	avb_cmdline.o
endif

obj-y += avb_crc32.o
