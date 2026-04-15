// SPDX-License-Identifier: GPL-2.0+
/*
 * bootm_ren - Authenticate AHAB OS container then boot FIT via bootm
 */

#include <command.h>
#include <env.h>
#include <linux/kernel.h>
#include <linux/types.h>

extern int do_bootm(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[]);
extern int authenticate_os_container(ulong addr);

static int do_bootm_ren(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	ulong cntr_addr, fit_addr;
	const char *fit_conf = "conf-imx91-9x9-flux.dtb";
	const char *env_fit_conf;
	char fit_spec[64];
	char *bootm_argv[3];

	if (argc < 2)
		return CMD_RET_USAGE;

	/* Keep container address in low 32-bit physical range on AArch64 */
	cntr_addr = hextoul(argv[1], NULL) & 0xffffffffUL;
	printf("Authenticate OS container at 0x%08lx\n", cntr_addr);

	if (authenticate_os_container(cntr_addr)) {
		printf("Authenticate OS container failed\n");
		return CMD_RET_FAILURE;
	}

	if (argc > 2 && argv[2] && argv[2][0]) {
		fit_conf = argv[2];
	} else {
		env_fit_conf = env_get("fit_conf");
		if (env_fit_conf && env_fit_conf[0])
			fit_conf = env_fit_conf;
	}

	fit_addr = cntr_addr + 0x2000;
	snprintf(fit_spec, sizeof(fit_spec), "0x%08lx#%s", fit_addr, fit_conf);

	bootm_argv[0] = "bootm";
	bootm_argv[1] = fit_spec;
	bootm_argv[2] = NULL;

	return do_bootm(cmdtp, flag, 2, bootm_argv);
}

U_BOOT_LONGHELP(bootm_ren,
	"<container_addr> [fit_conf]\n"
	"    - authenticate AHAB OS container and boot FIT config with bootm\n"
	"      fit_conf priority: CLI argument, env fit_conf, then conf-1\n"
);

U_BOOT_CMD(
	bootm_ren, 3, 0, do_bootm_ren,
	"authenticate AHAB container and boot FIT config", bootm_ren_help_text
);
