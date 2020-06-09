subdir-ccflags-y += -I./lib \
			-D_FILE_OFFSET_BITS=64 \
			-D_POSIX_C_SOURCE=199309L \
			-Wa,--noexecstack \
			-Wall \
			-Wextra \
			-Wformat=2 \
			-Wno-type-limits \
			-Wno-psabi \
			-Wno-unused-parameter \
			-Wno-sign-compare \
			-ffunction-sections \
			-std=gnu99

ifndef CONFIG_SPL_BUILD
obj-$(CONFIG_AVB_ATX) += libavb_atx/
endif
obj-y += fsl/
