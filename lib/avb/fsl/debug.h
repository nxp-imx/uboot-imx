/*
 * Copyright (C) 2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef __AVB_DEBUG_H__
#define __AVB_DEBUG_H__

#ifdef AVB_VVDEBUG
#define AVB_VDEBUG
#define VVDEBUG(format, ...) printf(" %s: "format, __func__, ##__VA_ARGS__)
#else
#define VVDEBUG(format, ...)
#endif

#ifdef AVB_VDEBUG
#define AVB_DEBUG
#define VDEBUG(format, ...) printf(" %s: "format, __func__, ##__VA_ARGS__)
#else
#define VDEBUG(format, ...)
#endif

#ifdef AVB_DEBUG
#define DEBUGAVB(format, ...) printf(" %s: "format, __func__, ##__VA_ARGS__)
#else
#define DEBUGAVB(format, ...)
#endif

#define ERR(format, ...) printf("%s: "format, __func__, ##__VA_ARGS__)

#define HEXDUMP_COLS 16
#define HEXDUMP_WIDTH 1

#endif
