subdir-ccflags-y += -I./lib/avb \
			-D_FILE_OFFSET_BITS=64 \
			-D_POSIX_C_SOURCE=199309L \
			-Wa,--noexecstack \
			-Wall \
			-Wextra \
			-Wformat=2 \
			-Wno-type-limits \
			-Wno-psabi \
			-Wno-unused-parameter \
			-ffunction-sections \
			-std=gnu99

ifndef CONFIG_SPL_BUILD
obj-y += libavb_ab/
obj-$(CONFIG_AVB_ATX) += libavb_atx/
endif
obj-y += libavb/
obj-y += fsl/
