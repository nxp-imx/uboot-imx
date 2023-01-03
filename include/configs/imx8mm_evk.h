/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2019 NXP
 */

#ifndef __IMX8MM_EVK_H
#define __IMX8MM_EVK_H

#include <linux/sizes.h>
#include <linux/stringify.h>
#include <asm/arch/imx-regs.h>
#include "imx_env.h"

#define CONFIG_SYS_BOOTM_LEN		(64 * SZ_1M)
#define CONFIG_SPL_MAX_SIZE		(148 * 1024)
#define CONFIG_SYS_MONITOR_LEN		SZ_512K
#define CONFIG_SYS_UBOOT_BASE	\
	(QSPI0_AMBA_BASE + CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR * 512)

#ifdef CONFIG_SPL_BUILD
#define CONFIG_SPL_STACK		0x920000
#define CONFIG_SPL_BSS_START_ADDR	0x910000
#define CONFIG_SPL_BSS_MAX_SIZE		SZ_8K	/* 8 KB */
#define CONFIG_SYS_SPL_MALLOC_START	0x42200000
#define CONFIG_SYS_SPL_MALLOC_SIZE	SZ_512K	/* 512 KB */

/* malloc f used before GD_FLG_FULL_MALLOC_INIT set */
#define CONFIG_MALLOC_F_ADDR		0x930000
/* For RAW image gives a error info not panic */
#define CONFIG_SPL_ABORT_ON_RAW_IMAGE

#if defined(CONFIG_IMX8M_LPDDR4) && defined(CONFIG_TARGET_IMX8MM_EVK)
#define CONFIG_POWER_PCA9450
#else
#define CONFIG_POWER_BD71837
#endif

#define CONFIG_SPL_NAND_BASE
#define CONFIG_SPL_NAND_IDENT
#define CONFIG_SYS_NAND_U_BOOT_OFFS 	0x4000000 /* Put the FIT out of first 64MB boot area */

/* Set a redundant offset in nand FIT mtdpart. The new uuu will burn full boot image (not only FIT part) to the mtdpart, so we check both two offsets */
#define CONFIG_SYS_NAND_U_BOOT_OFFS_REDUND \
	(CONFIG_SYS_NAND_U_BOOT_OFFS + CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR * 512 - 0x8400)

#endif

#define CONFIG_CMD_READ
#define CONFIG_SERIAL_TAG
#define CONFIG_FASTBOOT_USB_DEV 0

#define CONFIG_REMAKE_ELF
/* ENET Config */
/* ENET1 */
#if defined(CONFIG_FEC_MXC)
#define CONFIG_ETHPRIME                 "FEC"
#define PHY_ANEG_TIMEOUT 20000

#define CONFIG_FEC_XCV_TYPE             RGMII
#define CONFIG_FEC_MXC_PHYADDR          0

#define IMX_FEC_BASE			0x30BE0000
#endif

#ifdef CONFIG_NAND_BOOT
#define MFG_NAND_PARTITION "mtdparts=gpmi-nand:64m(nandboot),16m(nandfit),32m(nandkernel),16m(nanddtb),8m(nandtee),-(nandrootfs)"
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

/*
 * Another approach is add the clocks for inmates into clks_init_on
 * in clk-imx8mm.c, then clk_ingore_unused could be removed.
 */
#define JH_ROOT_DTB    "imx8mm-evk-root.dtb"

#define JAILHOUSE_ENV \
	"jh_clk= \0 " \
	"jh_root_dtb=" JH_ROOT_DTB "\0" \
	"jh_mmcboot=mw 0x303d0518 0xff; setenv fdtfile ${jh_root_dtb};" \
		"setenv jh_clk clk_ignore_unused mem=1212MB; " \
			   "if run loadimage; then " \
				   "run mmcboot; " \
			   "else run jh_netboot; fi; \0" \
	"jh_netboot=mw 0x303d0518 0xff; setenv fdtfile ${jh_root_dtb}; setenv jh_clk clk_ignore_unused mem=1212MB; run netboot; \0 "

#define SR_IR_V2_COMMAND \
	"nodes=/usbg1 /usbg2 /wdt-reboot /lcdif-resets /mipi-dsi-resets  /dispmix-reset /power-domains /soc@0/caam-sm@100000 /soc@0/bus@30000000/caam_secvio /soc@0/bus@30000000/caam-snvs@30370000 /soc@0/bus@32c00000/lcdif@32e00000 /soc@0/bus@32c00000/csi1_bridge@32e20000 /soc@0/bus@32c00000/mipi_dsi@32e10000 /soc@0/bus@32c00000/mipi_csi@32e30000 /soc@0/bus@32c00000/display-subsystem /audio-codec /sound-wm8524 /dsi-host /rm67199_panel /soc@0/bus@30800000/i2c@30a20000/pca9450@25 /soc@0/bus@30800000/i2c@30a30000/adv7535@3d /soc@0/bus@30800000/i2c@30a30000/tcpc@50 /soc@0/memory-controller@3d400000 /soc@0/bus@30800000/spi@30830000/spi@0 /binman \0" \
	"sr_ir_v2_cmd=cp.b ${fdtcontroladdr} ${fdt_addr_r} 0x10000;"\
	"fdt addr ${fdt_addr_r};"\
	"for i in ${nodes}; do fdt rm ${i}; done \0"

#define CONFIG_MFG_ENV_SETTINGS \
	CONFIG_MFG_ENV_SETTINGS_DEFAULT \
	"initrd_addr=0x43800000\0" \
	"initrd_high=0xffffffffffffffff\0" \
	"emmc_dev=2\0"\
	"sd_dev=1\0" \

/* Initial environment variables */
#if defined(CONFIG_NAND_BOOT)
#define CONFIG_EXTRA_ENV_SETTINGS \
	CONFIG_MFG_ENV_SETTINGS \
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
#define CONFIG_EXTRA_ENV_SETTINGS		\
	CONFIG_MFG_ENV_SETTINGS \
	BOOTENV \
	JAILHOUSE_ENV \
	SR_IR_V2_COMMAND \
	"prepare_mcore=setenv mcore_clk clk-imx8mm.mcore_booted;\0" \
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
	"mmcroot=" CONFIG_MMCROOT " rootwait rw\0" \
	"mmcautodetect=yes\0" \
	"mmcargs=setenv bootargs ${jh_clk} ${mcore_clk} console=${console} root=${mmcroot}\0 " \
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
	"netargs=setenv bootargs ${jh_clk} ${mcore_clk} console=${console} " \
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

#define CONFIG_SYS_INIT_RAM_ADDR        0x40000000
#define CONFIG_SYS_INIT_RAM_SIZE        0x200000
#define CONFIG_SYS_INIT_SP_OFFSET \
	(CONFIG_SYS_INIT_RAM_SIZE - GENERATED_GBL_DATA_SIZE)
#define CONFIG_SYS_INIT_SP_ADDR \
	(CONFIG_SYS_INIT_RAM_ADDR + CONFIG_SYS_INIT_SP_OFFSET)

#define CONFIG_SYS_MMC_ENV_DEV		1   /* USDHC2 */
#define CONFIG_MMCROOT			"/dev/mmcblk1p2"  /* USDHC2 */

#define CONFIG_SYS_SDRAM_BASE           0x40000000
#define PHYS_SDRAM                      0x40000000
#define PHYS_SDRAM_SIZE			0x80000000 /* 2GB DDR */

#define CONFIG_MXC_UART_BASE		UART2_BASE_ADDR

/* Monitor Command Prompt */
#define CONFIG_SYS_CBSIZE		2048
#define CONFIG_SYS_MAXARGS		64
#define CONFIG_SYS_BARGSIZE		CONFIG_SYS_CBSIZE
#define CONFIG_SYS_PBSIZE		(CONFIG_SYS_CBSIZE + \
					sizeof(CONFIG_SYS_PROMPT) + 16)

#define CONFIG_IMX_BOOTAUX

/* USDHC */

#ifdef CONFIG_TARGET_IMX8MM_DDR4_EVK
#define CONFIG_SYS_FSL_USDHC_NUM	1
#else
#define CONFIG_SYS_FSL_USDHC_NUM	2
#endif
#define CONFIG_SYS_FSL_ESDHC_ADDR       0

#ifdef CONFIG_FSL_FSPI
#define FSL_FSPI_FLASH_SIZE		SZ_32M
#define FSL_FSPI_FLASH_NUM		1
#define FSPI0_BASE_ADDR			0x30bb0000
#define FSPI0_AMBA_BASE			0x0
#define CONFIG_FSPI_QUAD_SUPPORT

#define CONFIG_SYS_FSL_FSPI_AHB
#endif

#ifdef CONFIG_NAND_MXS
#define CONFIG_CMD_NAND_TRIMFFS

/* NAND stuff */
#define CONFIG_SYS_MAX_NAND_DEVICE     1
#define CONFIG_SYS_NAND_BASE           0x20000000
#define CONFIG_SYS_NAND_USE_FLASH_BBT
#endif /* CONFIG_NAND_MXS */

/* USB configs */
#ifndef CONFIG_SPL_BUILD
#define CONFIG_USBD_HS

#endif

#define CONFIG_USB_GADGET_VBUS_DRAW 2

#define CONFIG_MXC_USB_PORTSC  (PORT_PTS_UTMI | PORT_PTS_PTW)
#define CONFIG_USB_MAX_CONTROLLER_COUNT         2

#ifdef CONFIG_ANDROID_SUPPORT
#include "imx8mm_evk_android.h"
#endif

#endif
