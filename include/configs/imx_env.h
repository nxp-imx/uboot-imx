/* Copyright 2018 NXP
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef __IMX_COMMON_CONFIG_H
#define __IMX_COMMON_CONFIG_H

#define CONFIG_MFG_ENV_SETTINGS_DEFAULT \
	"mfgtool_args=setenv bootargs console=${console},${baudrate} " \
		"rdinit=/linuxrc " \
		"clk_ignore_unused "\
		"\0" \
	"bootcmd_mfg=run mfgtool_args;  if iminfo ${initrd_addr}; then "\
                                             "booti ${loadaddr} ${initrd_addr} ${fdt_addr};"\
                                        "else echo \"Run fastboot ...\"; fastboot 0; fi\0" \

#endif
