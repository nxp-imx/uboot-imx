# SPDX-License-Identifier: GPL-2.0+
#
# Copyright 2014 Freescale Semiconductor, Inc.
# Copyright 2022 NXP

obj-y += sec.o
obj-$(CONFIG_FSL_CAAM) += jr.o fsl_hash.o jobdesc.o error.o
obj-$(CONFIG_FSL_BLOB) += fsl_blob.o
obj-$(CONFIG_RSA_FREESCALE_EXP) += fsl_rsa.o
ifdef CONFIG_SPL_BUILD
ifdef CONFIG_SPL_DM
obj-$(CONFIG_FSL_CAAM_RNG) += rng.o
obj-$(CONFIG_FSL_DCP_RNG) += dcp_rng.o
endif
else
obj-$(CONFIG_FSL_CAAM_RNG) += rng.o
obj-$(CONFIG_FSL_DCP_RNG) += dcp_rng.o
endif
obj-$(CONFIG_IMX_CAAM_MFG_PROT) += fsl_mfgprot.o
obj-$(CONFIG_RNG_SELF_TEST) += rng_self_test.o
obj-$(CONFIG_CMD_PROVISION_KEY) += fsl_aes.o tag_object.o
