/*
 * (C) Copyright 2009-2010 Freescale Semiconductor, Inc.
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

#ifndef _IMX_FUSE_H_
#define _IMX_FUSE_H_

#define IIM_ERR_SHIFT       8
#define POLL_FUSE_PRGD      (IIM_STAT_PRGD | (IIM_ERR_PRGE << IIM_ERR_SHIFT))
#define POLL_FUSE_SNSD      (IIM_STAT_SNSD | (IIM_ERR_SNSE << IIM_ERR_SHIFT))

int fuse_read(int bank, char row);
int fuse_blow(int bank, int row, int val);
int fuse_blow_func(char *func_name, char *func_val);

#endif
