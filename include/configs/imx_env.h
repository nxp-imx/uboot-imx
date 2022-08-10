/* Copyright 2018 NXP
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef __IMX_COMMON_CONFIG_H
#define __IMX_COMMON_CONFIG_H

#ifdef CONFIG_ARM64
    #define MFG_BOOT_CMD "booti "
#else
    #define MFG_BOOT_CMD "bootz "
#endif

#ifdef CONFIG_USB_PORT_AUTO
    #define FASTBOOT_CMD "echo \"Run fastboot ...\"; fastboot auto; "
#else
    #define FASTBOOT_CMD "echo \"Run fastboot ...\"; fastboot 0; "
#endif

/* define the nandfit partiton environment for uuu */
#if defined(CONFIG_IMX8MM) || defined(CONFIG_IMX8MQ) || \
	defined(CONFIG_IMX8QM) || defined(CONFIG_IMX8QXP) || \
	defined(CONFIG_IMX8DXL) || defined(CONFIG_IMX8MN) || \
	defined(CONFIG_IMX8MP)
#define MFG_NAND_FIT_PARTITION "nandfit_part=yes\0"
#else
#define MFG_NAND_FIT_PARTITION ""
#endif

#if defined(CONFIG_IMX8MP)
#define IMX_BOOT_IMAGE_GUID \
	EFI_GUID(0x928b33bc, 0xe58b, 0x4247, 0x9f, 0x1d, \
		 0x3b, 0xf1, 0xee, 0x1c, 0xda, 0xff)
#endif
#if defined(CONFIG_IMX8MM)
#define IMX_BOOT_IMAGE_GUID \
	EFI_GUID(0xead2005e, 0x7780, 0x400b, 0x93, 0x48, \
		 0xa2, 0x82, 0xeb, 0x85, 0x8b, 0x6b)

#endif
#if defined(CONFIG_IMX8MN)
#define IMX_BOOT_IMAGE_GUID \
	EFI_GUID(0xcbabf44d, 0x12cc, 0x45dd, 0xb0, 0xc5, \
		 0x29, 0xc5, 0xb7, 0x42, 0x2d, 0x34)

#endif
#if defined(CONFIG_IMX8MQ)
#define IMX_BOOT_IMAGE_GUID \
	EFI_GUID(0x296119cf, 0xdd70, 0x43de, 0x8a, 0xc8, \
		 0xa7, 0x05, 0x1f, 0x31, 0x25, 0x77)

#endif

#define CFG_MFG_ENV_SETTINGS_DEFAULT \
	"mfgtool_args=setenv bootargs console=${console},${baudrate} " \
		"rdinit=/linuxrc " \
		"clk_ignore_unused "\
		"\0" \
	"kboot="MFG_BOOT_CMD"\0"\
	"bootcmd_mfg=run mfgtool_args;" \
        "if iminfo ${initrd_addr}; then " \
            "if test ${tee} = yes; then " \
                "bootm ${tee_addr} ${initrd_addr} ${fdt_addr}; " \
            "else " \
                MFG_BOOT_CMD "${loadaddr} ${initrd_addr} ${fdt_addr}; " \
            "fi; " \
        "else " \
		FASTBOOT_CMD  \
        "fi;\0" \
	MFG_NAND_FIT_PARTITION \

#endif
