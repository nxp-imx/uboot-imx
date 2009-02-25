/*
 * Copyright 2009 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#ifndef __BOARD_FREESCALE_BOARD_MX51_3STACK_H__
#define __BOARD_FREESCALE_BOARD_MX51_3STACK_H__

/*!
 * @defgroup BRDCFG_MX51 Board Configuration Options
 * @ingroup MSL_MX51
 */

/*!
 * @file mx51_3stack/board-mx51_3stack.h
 *
 * @brief This file contains all the board level configuration options.
 *
 * It currently hold the options defined for MX51 3Stack Platform.
 *
 * @ingroup BRDCFG_MX51
 */

/* CPLD offsets */
#define PBC_LED_CTRL		(0x20000)
#define PBC_SB_STAT		(0x20008)
#define PBC_ID_AAAA		(0x20040)
#define PBC_ID_5555		(0x20048)
#define PBC_VERSION		(0x20050)
#define PBC_ID_CAFE		(0x20058)
#define PBC_INT_STAT		(0x20010)
#define PBC_INT_MASK		(0x20038)
#define PBC_INT_REST		(0x20020)
#define PBC_SW_RESET		(0x20060)

/* LED switchs */
#define LED_SWITCH_REG		0x00
/* buttons */
#define SWITCH_BUTTONS_REG	0x08
/* status, interrupt */
#define INTR_STATUS_REG	0x10
#define INTR_MASK_REG		0x38
#define INTR_RESET_REG		0x20
/* magic word for debug CPLD */
#define MAGIC_NUMBER1_REG	0x40
#define MAGIC_NUMBER2_REG	0x48
/* CPLD code version */
#define CPLD_CODE_VER_REG	0x50
/* magic word for debug CPLD */
#define MAGIC_NUMBER3_REG	0x58
/* module reset register*/
#define MODULE_RESET_REG	0x60
/* CPU ID and Personality ID */
#define MCU_BOARD_ID_REG	0x68

#endif				/* __BOARD_FREESCALE_BOARD_MX51_3STACK_H__ */
