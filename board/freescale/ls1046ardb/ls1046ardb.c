// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 * Copyright 2021 NXP
 */

#include <common.h>
#include <i2c.h>
#include <fdt_support.h>
#include <init.h>
#include <semihosting.h>
#include <serial.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <asm/arch/clock.h>
#include <asm/arch/fsl_serdes.h>
#include <asm/arch/ppa.h>
#include <asm/arch/soc.h>
#include <asm/arch-fsl-layerscape/fsl_icid.h>
#include <hwconfig.h>
#include <ahci.h>
#include <mmc.h>
#include <scsi.h>
#include <fm_eth.h>
#include <fsl_csu.h>
#include <fsl_esdhc.h>
#include <power/mc34vr500_pmic.h>
#include "cpld.h"

DECLARE_GLOBAL_DATA_PTR;

struct serial_device *default_serial_console(void)
{
#if IS_ENABLED(CONFIG_SEMIHOSTING_SERIAL)
	if (semihosting_enabled())
		return &serial_smh_device;
#endif
	return &eserial1_device;
}

int board_early_init_f(void)
{
	fsl_lsch2_early_init_f();

	return 0;
}

#ifndef CONFIG_SPL_BUILD
int checkboard(void)
{
	static const char *freq[2] = {"100.00MHZ", "156.25MHZ"};
	u8 cfg_rcw_src1, cfg_rcw_src2;
	u16 cfg_rcw_src;
	u8 sd1refclk_sel;

	puts("Board: LS1046ARDB, boot from ");

	cfg_rcw_src1 = CPLD_READ(cfg_rcw_src1);
	cfg_rcw_src2 = CPLD_READ(cfg_rcw_src2);
	cpld_rev_bit(&cfg_rcw_src1);
	cfg_rcw_src = cfg_rcw_src1;
	cfg_rcw_src = (cfg_rcw_src << 1) | cfg_rcw_src2;

	if (cfg_rcw_src == 0x44)
		printf("QSPI vBank %d\n", CPLD_READ(vbank));
	else if (cfg_rcw_src == 0x40)
		puts("SD\n");
	else
		puts("Invalid setting of SW5\n");

	printf("CPLD:  V%x.%x\nPCBA:  V%x.0\n", CPLD_READ(cpld_ver),
	       CPLD_READ(cpld_ver_sub), CPLD_READ(pcba_ver));

	puts("SERDES Reference Clocks:\n");
	sd1refclk_sel = CPLD_READ(sd1refclk_sel);
	printf("SD1_CLK1 = %s, SD1_CLK2 = %s\n", freq[sd1refclk_sel], freq[0]);

	return 0;
}

int board_init(void)
{
	struct ccsr_scfg *scfg = (struct ccsr_scfg *)CFG_SYS_FSL_SCFG_ADDR;

#ifdef CONFIG_NXP_ESBC
	/*
	 * In case of Secure Boot, the IBR configures the SMMU
	 * to allow only Secure transactions.
	 * SMMU must be reset in bypass mode.
	 * Set the ClientPD bit and Clear the USFCFG Bit
	 */
	u32 val;
	val = (in_le32(SMMU_SCR0) | SCR0_CLIENTPD_MASK) & ~(SCR0_USFCFG_MASK);
	out_le32(SMMU_SCR0, val);
	val = (in_le32(SMMU_NSCR0) | SCR0_CLIENTPD_MASK) & ~(SCR0_USFCFG_MASK);
	out_le32(SMMU_NSCR0, val);
#endif

#ifdef CONFIG_FSL_LS_PPA
	ppa_init();
#endif

#if !defined(CONFIG_SYS_EARLY_PCI_INIT) && defined(CONFIG_DM_ETH)
	pci_init();
#endif

	/* invert AQR105 IRQ pins polarity */
	out_be32(&scfg->intpcr, AQR105_IRQ_MASK);

	return 0;
}

int board_setup_core_volt(u32 vdd)
{
	bool en_0v9;

	en_0v9 = (vdd == 900) ? true : false;
	cpld_select_core_volt(en_0v9);

	return 0;
}

int get_serdes_volt(void)
{
	return mc34vr500_get_sw_volt(SW4);
}

int set_serdes_volt(int svdd)
{
	return mc34vr500_set_sw_volt(SW4, svdd);
}

int power_init_board(void)
{
	int ret;

	ret = power_mc34vr500_init(0);
	if (ret)
		return ret;

	setup_chip_volt();

	return 0;
}

void config_board_mux(void)
{
#ifdef CONFIG_HAS_FSL_XHCI_USB
	struct ccsr_scfg *scfg = (struct ccsr_scfg *)CFG_SYS_FSL_SCFG_ADDR;
	u32 usb_pwrfault;

	/* USB3 is not used, configure mux to IIC4_SCL/IIC4_SDA */
	out_be32(&scfg->rcwpmuxcr0, 0x3300);
	out_be32(&scfg->usbdrvvbus_selcr, SCFG_USBDRVVBUS_SELCR_USB1);
	usb_pwrfault = (SCFG_USBPWRFAULT_DEDICATED <<
			SCFG_USBPWRFAULT_USB3_SHIFT) |
			(SCFG_USBPWRFAULT_DEDICATED <<
			SCFG_USBPWRFAULT_USB2_SHIFT) |
			(SCFG_USBPWRFAULT_SHARED <<
			SCFG_USBPWRFAULT_USB1_SHIFT);
	out_be32(&scfg->usbpwrfault_selcr, usb_pwrfault);
#endif
}

#ifdef CONFIG_MISC_INIT_R
int misc_init_r(void)
{
	config_board_mux();
	return 0;
}
#endif

/* Update the PHY descriptions for boards revisions v4.0 and up. Main changes:
 * - The RTL8211FS PHY on SerDes1 lane B is replaced with an AQR115 PHY at
 *   address 0x3 on the second MDIO bus. The PHY is connected to MAC5 running
 *   in SGMII mode.
 * - The DS110DF111 retimer on SerDes1 lane C is replaced with an AQR113C PHY
 *   at address 0x8 on the second MDIO bus. The PHY is connected to MAC10
 *   running in XFI mode.
 * - There are now two AQR113C PHYs on the second MDIO bus and they share an
 *   interrupt.
 */
void fdt_fixup_phy(void *blob)
{
	const char sgmii1_phy_path[] =
		"/soc/fman@1a00000/mdio@fc000/ethernet-phy@3";
	const char mdio_bus_path[] =
		"/soc/fman@1a00000/mdio@fd000";
	const char mac5_path[] =
		"/soc/fman@1a00000/ethernet@e8000";
	const char mac10_path[] =
		"/soc/fman@1a00000/ethernet@f2000";
	int ret, offset, new_offset;
	u32 phandle;

	if (CPLD_READ(pcba_ver) < 0x4)
		return;

	/* Increase the size of the fdt to add multiple new nodes */
	ret = fdt_increase_size(blob, 200);
	if (ret < 0) {
		printf("Could not increase the size of the fdt: %s\n",
		       fdt_strerror(ret));
		return;
	}

	/* Find the RTL8211FS PHY node connected to MAC5 */
	offset = fdt_path_offset(blob, sgmii1_phy_path);
	if (offset < 0 && offset != FDT_ERR_NOTFOUND) {
		printf("ethernet-phy@3 node not found in the dts: %s\n",
		       fdt_strerror(offset));
		return;
	}

	/* Delete the RTL8211FS PHY representation */
	if (offset != FDT_ERR_NOTFOUND) {
		ret = fdt_del_node(blob, offset);
		if (ret < 0) {
			printf("Deleting the ethernet-phy@3 node failed: %s\n",
			       fdt_strerror(ret));
			return;
		}
	}

	/* Find the second mdio bus node to add the new AQR PHYs */
	offset = fdt_path_offset(blob, mdio_bus_path);
	if (offset < 0) {
		printf("10G mdio bus node not found in the dts: %s\n",
		       fdt_strerror(offset));
		return;
	}

	/* Add the new AQR115 PHY node */
	new_offset = fdt_add_subnode(blob, offset, "ethernet-phy@3");
	if (new_offset < 0) {
		printf("Failed to add the AQR115 node: %s\n",
		       fdt_strerror(new_offset));
		return;
	}
	ret = fdt_setprop_u32(blob, new_offset, "reg", SGMII_PHY1_ADDR);
	if (ret < 0) {
		printf("Setting 'reg' for the AQR115 node failed: %s\n",
		       fdt_strerror(ret));
		return;
	}
	ret = fdt_setprop_string(blob, new_offset, "compatible",
				 "ethernet-phy-ieee802.3-c45");
	if (ret < 0) {
		printf("Setting 'compatible' for the AQR115 node failed: %s\n",
		       fdt_strerror(ret));
		return;
	}
	phandle = fdt_create_phandle(blob, new_offset);
	if (phandle == 0) {
		printf("Creating a phandle for the AQR115 node failed\n");
		return;
	}

	/* Set MAC5's phy-handle to point to the new AQR115 PHY node */
	offset = fdt_path_offset(blob, mac5_path);
	if (offset < 0) {
		printf("MAC5 node not found in the dts: %s\n",
		       fdt_strerror(offset));
		return;
	}
	ret = fdt_setprop_inplace_u32(blob, offset, "phy-handle", phandle);
	if (ret < 0) {
		printf("Setting 'phy-handle' for the MAC5 node failed: %s\n",
		       fdt_strerror(ret));
		return;
	}

	/* Add the new AQR113C PHY node */
	offset = fdt_path_offset(blob, mdio_bus_path);
	if (offset < 0) {
		printf("10G mdio bus node not found in the dts: %s\n",
		       fdt_strerror(offset));
		return;
	}
	new_offset = fdt_add_subnode(blob, offset, "ethernet-phy@8");
	if (new_offset < 0) {
		printf("Failed to add the AQR113C node: %s\n",
		       fdt_strerror(new_offset));
		return;
	}
	ret = fdt_setprop_u32(blob, new_offset, "reg", FM1_10GEC2_PHY_ADDR);
	if (ret < 0) {
		printf("Setting 'reg' for the AQR113C node failed: %s\n",
		       fdt_strerror(ret));
		return;
	}
	ret = fdt_setprop_string(blob, new_offset, "compatible",
				 "ethernet-phy-ieee802.3-c45");
	if (ret < 0) {
		printf("Setting 'compatible' for the AQR113C node failed: %s\n",
		       fdt_strerror(ret));
		return;
	}
	ret = fdt_appendprop_u32(blob, new_offset, "interrupts", 0);
	if (ret < 0) {
		printf("setting interrupts for the AQR113C PHY node failed: %s\n",
		       fdt_strerror(ret));
		return;
	}
	ret = fdt_appendprop_u32(blob, new_offset, "interrupts", 131);
	if (ret < 0) {
		printf("setting interrupts for the AQR113C PHY node failed: %s\n",
		       fdt_strerror(ret));
		return;
	}
	ret = fdt_appendprop_u32(blob, new_offset, "interrupts", 4);
	if (ret < 0) {
		printf("setting interrupts for the AQR113C PHY node failed: %s\n",
		       fdt_strerror(ret));
		return;
	}
	phandle = fdt_create_phandle(blob, new_offset);
	if (phandle == 0) {
		printf("Creating a phandle for the AQR113C node failed\n");
		return;
	}

	/* Set MAC10's phy-handle to point to the new PHY node */
	offset = fdt_path_offset(blob, mac10_path);
	if (offset < 0) {
		printf("MAC10 node not found in the dts: %s\n",
		       fdt_strerror(offset));
		return;
	}
	ret = fdt_setprop_u32(blob, offset, "phy-handle", phandle);
	if (ret < 0) {
		printf("Setting 'phy-handle' for the MAC10 node failed: %s\n",
		       fdt_strerror(ret));
		return;
	}
	ret = fdt_delprop(blob, offset, "fixed-link");
	if (ret < 0 && ret != -FDT_ERR_NOTFOUND) {
		printf("Deleting 'fixed-link' for MAC10 failed: %s\n",
		       fdt_strerror(ret));
		return;
	}
	ret = fdt_delprop(blob, offset, "managed");
	if (ret < 0 && ret != -FDT_ERR_NOTFOUND) {
		printf("Deleting 'managed' for MAC10 failed: %s\n",
		       fdt_strerror(ret));
		return;
	}
	ret = fdt_setprop_string(blob, offset, "phy-connection-type", "xgmii");
	if (ret < 0) {
		printf("Setting 'phy-connection-type' for MAC10 failed: %s\n",
		       fdt_strerror(ret));
		return;
	}
}

int ft_board_setup(void *blob, struct bd_info *bd)
{
	u64 base[CONFIG_NR_DRAM_BANKS];
	u64 size[CONFIG_NR_DRAM_BANKS];

	/* fixup DT for the two DDR banks */
	base[0] = gd->bd->bi_dram[0].start;
	size[0] = gd->bd->bi_dram[0].size;
	base[1] = gd->bd->bi_dram[1].start;
	size[1] = gd->bd->bi_dram[1].size;

	fdt_fixup_memory_banks(blob, base, size, 2);
	ft_cpu_setup(blob, bd);

#ifdef CONFIG_SYS_DPAA_FMAN
#ifndef CONFIG_DM_ETH
	fdt_fixup_fman_ethernet(blob);
#endif
	fdt_fixup_phy(blob);
#endif

	fdt_fixup_icid(blob);

	return 0;
}

#if IS_ENABLED(CONFIG_OF_BOARD_FIXUP)
int board_fix_fdt(void *blob)
{
	fdt_fixup_phy(blob);

	return 0;
}
#endif
#endif
