ccflags-y += -DAVB_COMPILATION
ccflags-y += -D_FILE_OFFSET_BITS=64 \
			-D_POSIX_C_SOURCE=199309L \
			-Wa,--noexecstack \
			-Werror \
			-Wall \
			-Wextra \
			-Wformat=2 \
			-Wno-psabi \
			-Wno-unused-parameter \
			-ffunction-sections \
			-std=gnu99
obj-y += avb_ab_flow.o \
		avb_descriptor.o \
		avb_kernel_cmdline_descriptor.o \
		avb_sha512.o \
		avb_vbmeta_image.o \
		avb_chain_partition_descriptor.o \
		avb_footer.o \
		avb_property_descriptor.o \
		avb_slot_verify.o \
		avb_crc32.o \
		avb_hash_descriptor.o \
		avb_rsa.o \
		avb_crypto.o \
		avb_hashtree_descriptor.o \
		avb_sha256.o \
		avb_util.o \
		avb_sysdeps_uboot.o
