/*
 * Copyright (C) 2012 Freescale Semiconductor, Inc.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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

#include <linux/types.h>
#include <command.h>
#include <common.h>
#include <asm/regulator.h>

int do_regulops(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int rc = 0;
	int uv = 0;

	switch (argc) {
	case 2:
		if (strcmp(argv[1], "list") == 0)
			regul_list(0);
		else
			printf("Unsupported command: %s!\n", argv[1]);

		break;
	case 3:
		if (strcmp(argv[1], "show") == 0) {
			if (strcmp(argv[2], "all") == 0) {
				regul_list(1);
				break;
			} else if (strcmp(argv[2], "core") == 0)
				uv = regul_get_core();
			else if (strcmp(argv[2], "periph") == 0)
				uv = regul_get_periph();
			else
				uv = regul_get(argv[2]);

			if (uv >= 0) {
				printf("Name\t\tVoltage\n");
				printf("%s:\t\t%d\n", argv[2], uv);
			} else
				printf("Can't get regulator's voltage!\n");
		} else
			printf("Unsupported command: %s!\n", argv[1]);
		break;
	case 4:
		if (strcmp(argv[1], "set") == 0) {
			uv = simple_strtoul(argv[3], NULL, 10);
			if (strcmp(argv[2], "core") == 0)
				rc = regul_set_core(uv);
			else if (strcmp(argv[2], "periph") == 0)
				rc = regul_set_periph(uv);
			else
				rc = regul_set(argv[2], uv);

			if (!rc) {
				printf("Set voltage succeed!\n");
				printf("Name\t\tVoltage\n");
				printf("%s:\t\t%d\n", argv[2], uv);
			}
		} else
			printf("Unsupported command: %s!\n", argv[1]);
		break;
	default:
		rc = 1;
		printf("Too many or less parameters!\n");
		break;
	}

	return rc;
}

U_BOOT_CMD(
	regul, 4, 1, do_regulops,
	"Regulator sub system",
	"list - List all regulators' name\n"
	"regul show all "
	"- Display all regulators' voltage\n"
	"regul show core "
	"- Show core voltage in mV\n"
	"regul show periph "
	"- Show peripheral voltage in mV\n"
	"regul show <regulator name> "
	"- Show regulator's voltage in mV\n"
	"regul set core <voltage value> "
	"- Set core voltage in mV\n"
	"regul set periph <voltage value> "
	"- Set periph voltage in mV\n"
	"regul set <regulator name> <voltage value> "
	"- Set regulator's voltage in mV");

