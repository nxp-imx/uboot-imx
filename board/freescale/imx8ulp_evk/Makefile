# SPDX-License-Identifier: GPL-2.0+

obj-y   += imx8ulp_evk.o

ifdef CONFIG_SPL_BUILD
obj-y += spl.o
obj-$(CONFIG_TARGET_IMX8ULP_9X9_EVK) += lpddr4_timing_9x9.o
ifdef CONFIG_IMX8ULP_ND_MODE
obj-$(CONFIG_TARGET_IMX8ULP_EVK) += lpddr4_timing_266.o
else
obj-$(CONFIG_TARGET_IMX8ULP_EVK) += lpddr4_timing.o
endif
endif
