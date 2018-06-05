/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef __FSL_ATX_ATTRIBUTES_H__
#define __FSL_ATX_ATTRIBUTES_H__

#define fsl_version 1
/* This product_id is generated from
 * extern/avb/test/data/atx_product_id.bin */
extern unsigned char fsl_atx_product_id[17];
/* This product_root_public_key is generated form
 * extern/avb/test/data/testkey_atx_prk.pem */
extern unsigned char fsl_product_root_public_key[1032];

#endif /* __FSL_ATX_ATTRIBUTES_H__ */
