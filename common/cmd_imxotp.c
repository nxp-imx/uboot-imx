/*
 * Copyright (C) 2011 Freescale Semiconductor, Inc.
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

#include <config.h>
#include <common.h>
#include <command.h>
#include <imx_otp.h>

/*
 * meant to reject most of the zeros return by simple_strtoul call.
 *
 */
int validate_zero_from_simple_strtoul(char *str)
{
	int i = 0;

	/* reject negatives */
	if (str[0] == '-')
		return -1;

	/* reject no zero initialed */
	if (str[0] != '0')
		return -1;

	/* accept zero */
	if (strlen(str) == 1)
		return 0;

	/* only accept started with "0x" or "0X" or "00" */
	if (str[1] != 'x' && str[1] != 'X' && str[1] != '0')
		return -1;

	/* reject "0x" or "0X" or "00" only */
	if (strlen(str) == 3)
		return -1;

	/* only accept all zeros */
	for (i = 1; i < strlen(str) - 1; i++) {
		if (str[1] != '0')
			return -1;
	}
	return 0;
}

int do_imxotp(cmd_tbl_t *cmd_tbl, int flag, int argc, char* argv[])
{
	u32 addr, value_read, fused_value, value_to_fuse;
	int force_to_fuse = 0;

	if (argc < 3) {
usage:
		cmd_usage(cmd_tbl);
		return 1;
	}

	if (!strcmp(argv[1], "read")) {
		switch (argc) {
		case 3:
			addr = simple_strtoul(argv[2], NULL, 16);
			if ((addr == 0) &&
				validate_zero_from_simple_strtoul(argv[2]))
				goto usage;
			break;
		default:
			goto usage;
		}
		printf("Reading fuse at index: 0x%X\n", addr);
		if (imx_otp_read_one_u32(addr, &value_read))
			return -1;
		printf("Fuse at (index: 0x%X) value: 0x%X\n", addr, value_read);
	} else if (!strcmp(argv[1], "blow")) {
		if (argc < 4 || argc > 5)
			goto usage;

		if (!strcmp(argv[2], "--force")) {
			force_to_fuse = 1;
			addr = simple_strtoul(argv[3], NULL, 16);
			if ((addr == 0)
				  && validate_zero_from_simple_strtoul(argv[3]))
				goto usage;
			value_to_fuse = simple_strtoul(argv[4], NULL, 16);
			if ((value_to_fuse == 0)
				  && validate_zero_from_simple_strtoul(argv[4]))
				goto usage;
		} else {
			addr = simple_strtoul(argv[2], NULL, 16);
			if ((addr == 0)
				  && validate_zero_from_simple_strtoul(argv[2]))
				goto usage;
			value_to_fuse = simple_strtoul(argv[3], NULL, 16);
			if ((value_to_fuse == 0)
				  && validate_zero_from_simple_strtoul(argv[3]))
				goto usage;
		}

		/* show the current value of specified address (fuse index) */
		if (imx_otp_read_one_u32(addr, &value_read))
			return -1;
		printf("Current fuse at (index: 0x%X) value: 0x%X\n",
				addr, value_read);

		if ((value_to_fuse & value_read) == value_to_fuse)
			printf("!! Fuse blow skipped:"
					" the bits have been already set.\n");
		else if (force_to_fuse) {
			printf("Blowing fuse at index: 0x%X, value: 0x%X\n",
					addr, value_to_fuse);
			if (imx_otp_blow_one_u32(addr,
					value_to_fuse, &fused_value)) {
				printf("ERROR: failed to blow fuse"
					  " at 0x%X with value of 0x%X\n",
						addr, value_to_fuse);
			} else {
				printf("Operation %s fuse"
					  " at (index: 0x%X) value: 0x%X\n",
					((fused_value & value_to_fuse) ==
					     value_to_fuse) ? "succeeded"
										: "failed",
					addr, fused_value);
			}
		} else {
			printf("!! Fuse blow aborted."
				" if you do want to blow it."
				" Please use the command:\n"
				"%s blow --force %X %X\n",
				argv[0], addr, value_to_fuse);
		}
	} else
		goto usage;

	return 0;
}

U_BOOT_CMD(imxotp, 5, 0, do_imxotp,
	"One-Time Programable sub-system",
	"imxotp read <addr>\n"
	" - read fuse at 'addr'\n"
	"imxotp blow [--force] <addr> <value>\n"
	" - blow fuse at 'addr' with hex value 'value'\n"
);
