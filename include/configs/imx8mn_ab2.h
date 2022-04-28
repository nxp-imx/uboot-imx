/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2020 NXP
 */

#ifndef __IMX8MN_AB2_H
#define __IMX8MN_AB2_H

#include <linux/sizes.h>
#include <linux/stringify.h>
#include <asm/arch/imx-regs.h>
#include "imx_env.h"

#define CFG_SYS_UBOOT_BASE	\
	(QSPI0_AMBA_BASE + CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR * 512)

#define PHY_ANEG_TIMEOUT 20000

#ifdef CONFIG_DISTRO_DEFAULTS
#define BOOT_TARGET_DEVICES(func) \
	func(USB, usb, 0) \
	func(MMC, mmc, 1) \
	func(MMC, mmc, 2)

#include <config_distro_bootcmd.h>
#else
#define BOOTENV
#endif

/*
 * Another approach is add the clocks for inmates into clks_init_on
 * in clk-imx8mm.c, then clk_ingore_unused could be removed.
 */
#ifdef CONFIG_TARGET_IMX8MN_DDR4_AB2
#define JH_ROOT_DTB	"imx8mn-ddr4-ab2-root.dtb"
#else
#define JH_ROOT_DTB	"imx8mn-ab2-root.dtb"
#endif

#define JAILHOUSE_ENV \
	"jh_clk= \0 " \
	"jh_root_dtb=" JH_ROOT_DTB "\0" \
	"jh_mmcboot=mw 0x303d0518 0xff; setenv fdtfile ${jh_root_dtb};" \
		"setenv jh_clk clk_ignore_unused mem=1212MB; " \
			   "if run loadimage; then " \
				   "run mmcboot; " \
			   "else run jh_netboot; fi; \0" \
	"jh_netboot=mw 0x303d0518 0xff; setenv fdtfile ${jh_root_dtb}; setenv jh_clk clk_ignore_unused mem=1212MB; run netboot; \0 "

#define M7_BOOT_ENV \
	"m7_boot=no\0" \
	"m7_image=nxh3670.itb\0" \
	"m7_loadaddr=0x80000000\0" \
	"m7_nxh_app_loadaddr=0x81000000\0" \
	"m7_nxh_rfmac_loadaddr=0x81012000\0" \
	"m7_nxh_cf_loadaddr=0x81016000\0" \
	"m7_nxh_data_loadaddr=0x8101E000\0" \
	"m7_fdt_file=imx8mn-ab2-m7.dtb\0" \
	"m7_nxh_bin=main@1\0" \
	"m7_nxh_app=app@1\0" \
	"m7_nxh_rfmac=rfmac@1\0" \
	"m7_nxh_cf=cf@1\0" \
	"m7_nxh_data=data@1\0" \
	"loadm7nxhfw=imxtract ${loadaddr} ${m7_nxh_bin} ${m7_loadaddr}; " \
		"imxtract ${loadaddr} ${m7_nxh_app} ${m7_nxh_app_loadaddr}; " \
		"imxtract ${loadaddr} ${m7_nxh_rfmac} ${m7_nxh_rfmac_loadaddr}; " \
		"imxtract ${loadaddr} ${m7_nxh_cf} ${m7_nxh_cf_loadaddr}; " \
		"imxtract ${loadaddr} ${m7_nxh_data} ${m7_nxh_data_loadaddr}\0" \
	"loadm7image=fatload mmc ${mmcdev}:${mmcpart} ${loadaddr} ${m7_image}\0" \
	"update_m7_from_sd=" \
		"if sf probe 0:0; then " \
			"if run loadm7image; then " \
				"setexpr fw_sz ${filesize} + 0xffff; " \
				"setexpr fw_sz ${fw_sz} / 0x10000; " \
				"setexpr fw_sz ${fw_sz} * 0x10000; " \
				"sf erase 0x100000 ${fw_sz}; " \
				"sf write ${m7_loadaddr} 0x100000 ${filesize}; " \
			"fi; " \
		"fi\0" \
	"m7boot=run loadm7image; run loadm7nxhfw; dcache flush; bootaux ${m7_loadaddr}\0" \
	"m7netboot=${get_cmd} ${loaddadr} ${m7_image}; " \
		"run loadm7nxhfw; dcache flush; bootaux ${m7_loadaddr}; \0" \
	"m7boot_sf=sf probe 0:0; dcache flush; bootaux ${m7_sf_loadaddr}\0"

#define CFG_MFG_ENV_SETTINGS \
	CFG_MFG_ENV_SETTINGS_DEFAULT \
	"initrd_addr=0x43800000\0" \
	"initrd_high=0xffffffffffffffff\0" \
	"emmc_dev=2\0"\
	"sd_dev=1\0" \

/* Initial environment variables */
#define CFG_EXTRA_ENV_SETTINGS		\
	CFG_MFG_ENV_SETTINGS \
	JAILHOUSE_ENV \
	BOOTENV \
	M7_BOOT_ENV \
	"scriptaddr=0x43500000\0" \
	"kernel_addr_r=" __stringify(CONFIG_SYS_LOAD_ADDR) "\0" \
	"bsp_script=boot.scr\0" \
	"image=Image\0" \
	"splashimage=0x50000000\0" \
	"console=ttymxc1,115200\0" \
	"fdt_addr_r=0x43000000\0"			\
	"fdt_addr=0x43000000\0"			\
	"fdt_high=0xffffffffffffffff\0"		\
	"boot_fit=no\0" \
	"fdtfile=" CONFIG_DEFAULT_FDT_FILE "\0" \
	"bootm_size=0x10000000\0" \
	"mmcdev="__stringify(CONFIG_SYS_MMC_ENV_DEV)"\0" \
	"mmcpart=1\0" \
	"mmcroot=/dev/mmcblk1p2 rootwait rw\0" \
	"mmcautodetect=yes\0" \
	"mmcargs=setenv bootargs ${jh_clk} console=${console} root=${mmcroot}\0 " \
	"loadbootscript=fatload mmc ${mmcdev}:${mmcpart} ${loadaddr} ${bsp_script};\0" \
	"bootscript=echo Running bootscript from mmc ...; " \
		"source\0" \
	"loadimage=fatload mmc ${mmcdev}:${mmcpart} ${loadaddr} ${image}\0" \
	"loadfdt=fatload mmc ${mmcdev}:${mmcpart} ${fdt_addr_r} ${fdtfile}\0" \
	"mmcboot=echo Booting from mmc ...; " \
		"run mmcargs; " \
		"if test ${m7_boot} = yes || test ${m7_boot} = try; then "\
			"echo Booting M7 aux core...; " \
			"run m7boot; " \
		"fi; " \
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
		"if test ${m7_boot} = yes || test ${m7_boot} = try; then " \
			"echo Booting M7 aux core...;" \
			"run m7netboot;" \
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


/* Link Definitions */

#define CFG_SYS_INIT_RAM_ADDR        0x40000000
#define CFG_SYS_INIT_RAM_SIZE        0x200000

#define CFG_SYS_SDRAM_BASE           0x40000000
#define PHYS_SDRAM                      0x40000000

#ifdef CONFIG_TARGET_IMX8MN_DDR3L_AB2
#define PHYS_SDRAM_SIZE			0x40000000 /* 1GB DDR */
#else
#define PHYS_SDRAM_SIZE			0x80000000 /* 2GB DDR */
#endif

#define CFG_SYS_NAND_BASE           0x20000000
#endif
