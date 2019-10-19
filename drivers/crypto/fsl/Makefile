# SPDX-License-Identifier: GPL-2.0+
#
# Copyright 2014 Freescale Semiconductor, Inc.

obj-y += sec.o
obj-$(CONFIG_FSL_CAAM) += jr.o fsl_hash.o jobdesc.o error.o
obj-$(CONFIG_CMD_BLOB)$(CONFIG_IMX_CAAM_DEK_ENCAP) += fsl_blob.o
obj-$(CONFIG_RSA_FREESCALE_EXP) += fsl_rsa.o
obj-$(CONFIG_IMX_CAAM_MFG_PROT) += fsl_mfgprot.o
