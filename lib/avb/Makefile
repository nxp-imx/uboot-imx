subdir-ccflags-y += -D_FILE_OFFSET_BITS=64 \
			-D_POSIX_C_SOURCE=199309L \
			-Wa,--noexecstack \
			-Werror \
			-Wall \
			-Wextra \
			-Wformat=2 \
			-Wno-type-limits \
			-Wno-psabi \
			-Wno-unused-parameter \
			-ffunction-sections \
			-std=gnu99
obj-y += libavb/
obj-y += libavb_ab/
obj-$(CONFIG_AVB_ATX) += libavb_atx/
obj-y += fsl/
