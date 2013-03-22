/*
 * Freescale system chip & board version define
 * Copyright (C) 2012-2013 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include <config.h>
#include <common.h>
#include <asm/io.h>
#if defined(CONFIG_MX6Q) || defined(CONFIG_MX6DL) || defined(CONFIG_MX6SL)
#include <asm/arch/mx6.h>
#endif

#ifdef CONFIG_CMD_IMXOTP
#include <imx_otp.h>
#endif

unsigned int fsl_system_rev;

#if defined(CONFIG_MX6Q) || defined(CONFIG_MX6DL) || defined(CONFIG_MX6SL)
/*
 * Set fsl_system_rev:
 * bit 0-7: Chip Revision ID
 * bit 8-11: Board Revision ID
 *     0: Unknown or latest revision
 *     1: RevA Board
 *     2: RevB board
 *     3: RevC board
 * bit 12-19: Chip Silicon ID
 *     0x63: i.MX6 Dual/Quad
 *     0x61: i.MX6 Solo/DualLite
 *     0x60: i.MX6 SoloLite
 */
void fsl_set_system_rev(void)
{
	/* Read Silicon information from Anatop register */
	/* The register layout:
	 * bit 16-23: Chip Silicon ID
	 * 0x60: i.MX6 SoloLite
	 * 0x61: i.MX6 Solo/DualLite
	 * 0x63: i.MX6 Dual/Quad
	 *
	 * bit 0-7: Chip Revision ID
	 * 0x00: TO1.0
	 * 0x01: TO1.1
	 * 0x02: TO1.2
	 *
	 * exp:
	 * Chip             Major    Minor
	 * i.MX6Q1.0:       6300     00
	 * i.MX6Q1.1:       6300     01
	 * i.MX6Solo1.0:    6100     00

	 * Thus the system_rev will be the following layout:
	 * | 31 - 20 | 19 - 12 | 11 - 8 | 7 - 0 |
	 * | resverd | CHIP ID | BD REV | SI REV |
	 */
	u32 board_type = 0;
	u32 cpu_type = readl(ANATOP_BASE_ADDR + 0x280);

	if ((cpu_type >> 16) == 0x60)
		goto found;

	cpu_type = readl(ANATOP_BASE_ADDR + 0x260);
found:
	/* Chip Silicon ID */
	fsl_system_rev = ((cpu_type >> 16) & 0xFF) << 12;
	/* Chip silicon major revision */
	fsl_system_rev |= ((cpu_type >> 8) & 0xFF) << 4;
	fsl_system_rev += 0x10;
	/* Chip silicon minor revision */
	fsl_system_rev |= cpu_type & 0xFF;

	/* Get Board ID information from OCOTP_GP1[15:8]
	 * bit 12-15: Board type
	 * 0x0 : Unknown
	 * 0x1 : Sabre-AI (ARD)
	 * 0x2 : Smart Device (SD)
	 * 0x3 : Quick-Start Board (QSB)
	 * 0x4 : SoloLite EVK (SL-EVK)
     * 0x6 : HDMI Dongle
	 *
	 * bit 8-11: Board Revision ID
	 * 0x0 : Unknown or latest revision
	 * 0x1 : RevA board
	 * 0x2 : RevB
	 * 0x3 : RevC
	 *
	 * exp:
	 * i.MX6Q ARD RevA:     0x11
	 * i.MX6Q ARD RevB:     0x12
	 * i.MX6Solo ARD RevA:  0x11
	 * i.MX6Solo ARD RevB:  0x12
	 */
#ifdef CONFIG_CMD_IMXOTP
	imx_otp_read_one_u32(0x26, &board_type);
	switch ((board_type >> 8) & 0xF) {
	case 0x1: /* RevA */
		fsl_system_rev |= BOARD_REV_2;
		break;
	case 0x2: /* RevB */
		fsl_system_rev |= BOARD_REV_3;
		break;
	case 0x3: /* RevC */
		fsl_system_rev |= BOARD_REV_4;
		break;
	case 0x0: /* Unknown */
	default:
		fsl_system_rev |= BOARD_REV_1;
		break;
	}
#endif
}

int cpu_is_mx6q()
{
	if (fsl_system_rev != NULL)
		fsl_set_system_rev();
	return (((fsl_system_rev & 0xff000)>>12) == 0x63);
}
#else
void fsl_set_system_rev(void)
{
}

int cpu_is_mx6q()
{
	return 0;
}
#endif
