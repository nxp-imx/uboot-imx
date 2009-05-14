/*
 *  include/linux/mmc/sd.h
 *
 *  Copyright (C) 2005-2007 Pierre Ossman, All Rights Reserved.
 *
 * Copyright 2008-2009 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 */

#ifndef MMC_SD_H
#define MMC_SD_H

/* SD commands                           type  argument     response */
  /* class 0 */
/* This is basically the same command as for MMC with some quirks. */
#define SD_SEND_RELATIVE_ADDR     3   /* bcr                     R6  */
#define SD_SEND_IF_COND           8   /* bcr  [11:0] See below   R7  */

  /* class 10 */
#define SD_SWITCH                 6   /* adtc [31:0] See below   R1  */

  /* Application commands */
#define SD_APP_SET_BUS_WIDTH      6   /* ac   [1:0] bus width    R1  */
#define SD_APP_SEND_NUM_WR_BLKS  22   /* adtc                    R1  */
#define SD_APP_OP_COND           41   /* bcr  [31:0] OCR         R3  */
#define SD_APP_SEND_SCR          51   /* adtc                    R1  */

#define SD_OCR_VALUE_HV_LC     (0x00ff8000)
#define SD_OCR_VALUE_HV_HC     (0x40ff8000)
#define SD_OCR_VALUE_LV_HC     (0x40000080)
#define SD_OCR_HC_RES          (0x40000000)
#define SD_IF_HV_COND_ARG      (0x000001AA)
#define SD_IF_LV_COND_ARG      (0x000002AA)

#define SD_OCR_VALUE_COUNT     (3)
#define SD_IF_CMD_ARG_COUNT    (2)

/*
 * SD_SWITCH argument format:
 *
 *      [31] Check (0) or switch (1)
 *      [30:24] Reserved (0)
 *      [23:20] Function group 6
 *      [19:16] Function group 5
 *      [15:12] Function group 4
 *      [11:8] Function group 3
 *      [7:4] Function group 2
 *      [3:0] Function group 1
 */

/*
 * SD_SEND_IF_COND argument format:
 *
 *	[31:12] Reserved (0)
 *	[11:8] Host Voltage Supply Flags
 *	[7:0] Check Pattern (0xAA)
 */

/*
 * SCR field definitions
 */

#define SCR_SPEC_VER_0  0   /* Implements system specification 1.0 - 1.01 */
#define SCR_SPEC_VER_1  1   /* Implements system specification 1.10 */
#define SCR_SPEC_VER_2  2   /* Implements system specification 2.00 */

/*
 * SD bus widths
 */
#define SD_BUS_WIDTH_1  0
#define SD_BUS_WIDTH_4  2

/*
 * SD_SWITCH mode
 */
#define SD_SWITCH_CHECK 0
#define SD_SWITCH_SET   1

/*
 * SD_SWITCH function groups
 */
#define SD_SWITCH_GRP_ACCESS 0

/*
 * SD_SWITCH access modes
 */
#define SD_SWITCH_ACCESS_DEF 0
#define SD_SWITCH_ACCESS_HS  1

#endif

