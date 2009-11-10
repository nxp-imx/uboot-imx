/*
 * (C) Copyright 2009 Freescale Semiconductor, Inc.
 *
 * Configuration settings for the MX51-3Stack Freescale board.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef _MXC_KEYPAD_H_
#define _MXC_KEYPAD_H_

#include <config.h>

#define KEY_1                   2
#define KEY_2                   3
#define KEY_3                   4
#define KEY_F1                  59
#define KEY_UP                  103
#define KEY_F2                  60

#define KEY_4                   5
#define KEY_5                   6
#define KEY_6                   7
#define KEY_LEFT                105
#define KEY_SELECT              0x161
#define KEY_RIGHT               106

#define KEY_7                   8
#define KEY_8                   9
#define KEY_9                   10
#define KEY_F3                  61
#define KEY_DOWN                108
#define KEY_F4                  62

#define KEY_0                   11
#define KEY_OK                  0x160
#define KEY_ESC                 1
#define KEY_ENTER               28
#define KEY_MENU                139     /* Menu (show menu) */
#define KEY_BACK                158     /* AC Back */

#if defined(CONFIG_MX51_BBG)
#define TEST_HOME_KEY_DEPRESS(k, e)  (((k) == (KEY_F1)) && (((e) == (KDepress))))
#define TEST_POWER_KEY_DEPRESS(k, e) (((k) == (KEY_ENTER)) && (((e) == (KDepress))))
#elif defined(CONFIG_MX51_3DS)
#define TEST_HOME_KEY_DEPRESS(k, e)  (((k) == (KEY_MENU)) && (((e) == (KDepress))))
#define TEST_POWER_KEY_DEPRESS(k, e) (((k) == (KEY_F2)) && (((e) == (KDepress))))
#else
# error Undefined board type!
#endif

#endif
