/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2024 NXP.
 */

#ifndef AVB_SHA256_NEON_H
#define AVB_SHA256_NEON_H

void SHA256_transform_NEON(AvbSHA256Ctx* ctx, const uint8_t* message, size_t block_nb);

#endif /* AVB_SHA256_NEON_H */
