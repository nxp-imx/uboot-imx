/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2023 Voxelbotics
 * Copyright 2019,2024 NXP
 */

#ifndef __NAVQPLUS_H
#define __NAVQPLUS_H

#include <linux/sizes.h>
#include <linux/stringify.h>
#include <asm/arch/imx-regs.h>
/* Enable USB boot port auto-detection */
#define CONFIG_USB_PORT_AUTO
#include "imx_env.h"

#define CFG_SYS_UBOOT_BASE	(QSPI0_AMBA_BASE + CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR * 512)

#if defined(CONFIG_CMD_NET)

#define CFG_FEC_MXC_PHYADDR          1

#define DWC_NET_PHYADDR			1

#define PHY_ANEG_TIMEOUT 20000

#endif

#ifdef CONFIG_DISTRO_DEFAULTS
#define BOOT_TARGET_DEVICES(func) \
	func(USB, usb, 0) \
	func(MMC, mmc, 1) \
	func(MMC, mmc, 2)

#include <config_distro_bootcmd.h>
#else
#define BOOTENV
#endif

#define JH_ROOT_DTB    "imx8mp-evk-root.dtb"

#define JAILHOUSE_ENV \
	"jh_clk= \0 " \
	"jh_root_dtb=" JH_ROOT_DTB "\0" \
	"jh_mmcboot=setenv fdtfile ${jh_root_dtb};" \
		"setenv jh_clk kvm.enable_virt_at_load=false clk_ignore_unused mem=1920MB; " \
			   "if run loadimage; then " \
				   "run mmcboot; " \
			   "else run jh_netboot; fi; \0" \
	"jh_netboot=setenv fdtfile ${jh_root_dtb}; setenv jh_clk kvm.enable_virt_at_load=false clk_ignore_unused mem=1920MB; run netboot; \0 "

#define CFG_MFG_ENV_SETTINGS \
	CFG_MFG_ENV_SETTINGS_DEFAULT \
	"initrd_addr=0x43800000\0" \
	"initrd_high=0xffffffffffffffff\0" \
	"emmc_dev=2\0"\
	"sd_dev=1\0"


#ifdef CONFIG_NAND_BOOT
#define MFG_NAND_PARTITION "mtdparts=gpmi-nand:64m(nandboot),16m(nandfit),32m(nandkernel),16m(nanddtb),8m(nandtee),-(nandrootfs)"
#endif

/* Initial environment variables */
#if defined(CONFIG_NAND_BOOT)
#define CFG_EXTRA_ENV_SETTINGS \
	CFG_MFG_ENV_SETTINGS \
	"splashimage=0x50000000\0" \
	"fdt_addr_r=0x43000000\0"			\
	"fdt_addr=0x43000000\0"			\
	"fdt_high=0xffffffffffffffff\0" \
	"mtdparts=" MFG_NAND_PARTITION "\0" \
	"console=ttymxc1,115200 earlycon=ec_imx6q,0x30890000,115200\0" \
	"bootargs=console=ttymxc1,115200 earlycon=ec_imx6q,0x30890000,115200 ubi.mtd=nandrootfs "  \
		"root=ubi0:nandrootfs rootfstype=ubifs "		     \
		MFG_NAND_PARTITION \
		"\0" \
	"bootcmd=nand read ${loadaddr} 0x5000000 0x2000000;"\
		"nand read ${fdt_addr_r} 0x7000000 0x100000;"\
		"booti ${loadaddr} - ${fdt_addr_r}"

#else
#define CFG_EXTRA_ENV_SETTINGS		\
	CFG_MFG_ENV_SETTINGS \
	JAILHOUSE_ENV \
	BOOTENV \
	"enable_3v3p=mw.l 0x303301D0 5; mw.l 0x30330430 0x6; mw.l 0x30240000 0x4; mw.l 0x30240004 0x4\0" \
	"enable_5v0p=mw.l 0x30330118 5; mw.l 0x30330378 0x6; mw.l 0x30220000 0x0000; mw.l 0x30220004 0x4000\0" \
	"scriptaddr=0x43500000\0" \
	"kernel_addr_r=" __stringify(CONFIG_SYS_LOAD_ADDR) "\0" \
	"bsp_script=boot.scr\0" \
	"image=Image\0" \
	"splashimage=0x50000000\0" \
	"console=ttymxc1,115200\0" \
	"fdt_addr_r=0x43000000\0"			\
	"fdt_addr=0x43000000\0"			\
	"boot_fdt=try\0" \
	"fdt_high=0xffffffffffffffff\0"		\
	"boot_fit=no\0" \
	"fdtfile=" CONFIG_DEFAULT_FDT_FILE "\0" \
	"bootm_size=0x10000000\0" \
	"mmcdev="__stringify(CONFIG_SYS_MMC_ENV_DEV)"\0" \
	"mmcpart=1\0" \
	"mmcroot=" CFG_MMCROOT " rootwait rw\0" \
	"mmcautodetect=yes\0" \
	"mmcargs=setenv bootargs ${jh_clk} console=${console} root=${mmcroot}\0 " \
	"loadbootscript=fatload mmc ${mmcdev}:${mmcpart} ${loadaddr} ${bsp_script};\0" \
	"bootscript=echo Running bootscript from mmc ...; " \
		"source\0" \
	"loadimage=fatload mmc ${mmcdev}:${mmcpart} ${loadaddr} ${image}\0" \
	"loadfdt=fatload mmc ${mmcdev}:${mmcpart} ${fdt_addr_r} ${fdtfile}\0" \
	"mmcboot=echo Booting from mmc ...; " \
		"run mmcargs; " \
		"if test ${boot_fit} = yes || test ${boot_fit} = try; then " \
			"bootm ${loadaddr}; " \
		"else " \
			"if run loadfdt; then " \
				"booti ${loadaddr} - ${fdt_addr_r}; " \
			"else " \
				"echo WARN: Cannot load the DT; " \
			"fi; " \
		"fi;\0" \
	"netargs=setenv bootargs ${jh_clk} console=${console} " \
		"root=/dev/nfs " \
		"ip=dhcp nfsroot=${serverip}:${nfsroot},v3,tcp\0" \
	"netboot=echo Booting from net ...; " \
		"run netargs;  " \
		"if test ${ip_dyn} = yes; then " \
			"setenv get_cmd dhcp; " \
		"else " \
			"setenv get_cmd tftp; " \
		"fi; " \
		"${get_cmd} ${loadaddr} ${image}; " \
		"if test ${boot_fit} = yes || test ${boot_fit} = try; then " \
			"bootm ${loadaddr}; " \
		"else " \
			"if ${get_cmd} ${fdt_addr_r} ${fdtfile}; then " \
				"booti ${loadaddr} - ${fdt_addr_r}; " \
			"else " \
				"echo WARN: Cannot load the DT; " \
			"fi; " \
		"fi;\0" \
	"bsp_bootcmd=echo Running BSP bootcmd ...; " \
        "echo Enabling 3V3P/5VP; run enable_3v3p; run enable_5v0p; " \
		"mmc dev ${mmcdev}; if mmc rescan; then " \
		   "if run loadbootscript; then " \
			   "run bootscript; " \
		   "else " \
			   "if run loadimage; then " \
				   "run mmcboot; " \
			   "else run netboot; " \
			   "fi; " \
		   "fi; " \
	   "fi;"
#endif

/* Link Definitions */

#define CFG_SYS_INIT_RAM_ADDR	0x40000000
#define CFG_SYS_INIT_RAM_SIZE	0x80000

#define CFG_MMCROOT			"/dev/mmcblk1p2"  /* USDHC2 */

/* Totally 4GB DDR --- KINGSTON*/
#define CFG_SYS_SDRAM_BASE		0x40000000
#define PHYS_SDRAM			0x40000000
#define PHYS_SDRAM_SIZE			0xC0000000	/* 3 GB */
#define PHYS_SDRAM_2			0x100000000
#define PHYS_SDRAM_2_SIZE		0x140000000	/* 5 GB */

#define CFG_MXC_UART_BASE		UART2_BASE_ADDR

#define CFG_SYS_NAND_BASE           0x20000000

#define CFG_SYS_FSL_USDHC_NUM	1
#define CFG_SYS_FSL_ESDHC_ADDR	0

/* USB configs */

#define CONFIG_USB_MAX_CONTROLLER_COUNT         2
#define CONFIG_USBD_HS
#define CONFIG_USB_GADGET_VBUS_DRAW 2

#define NAVQP_BOOT_IMAGE_GUID \
        EFI_GUID(0x1ea7e557, 0xbb97, 0x496e, 0x98, 0xa6, \
                 0x00, 0x74, 0xcf, 0x7b, 0x55, 0xde)

#endif
