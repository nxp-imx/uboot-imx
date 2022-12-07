/* SPDX-License-Identifier: GPL-2.0+ */
/*
* Copyright (c) 2022 NXP
*/

#ifndef _KASLR_H
#define _KASLR_H
/*
 * do_generate_kaslr(): Fix up kaslr-seed in device-tree with random 64
 * bit value.
 *
 * @blob: Working device tree pointer
 */

int do_generate_kaslr(void *blob);

#endif
