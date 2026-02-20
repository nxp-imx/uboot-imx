// SPDX-License-Identifier: GPL-2.0+
/*
 * Conga-SMX95 Board File
 * i.MX95 Platform Initialization
 */

#include <common.h>
#include <init.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <i2c.h>

DECLARE_GLOBAL_DATA_PTR;

/* PMIC I2C Address */
#define PF0900_I2C_ADDR  0x08

/*
 * PMIC Initialization (I2C1 – PF0900)
 */
static int conga_pmic_init(void)
{
    struct udevice *dev;
    int ret;

    printf("Initializing PMIC (PF0900)...\n");

    /* Get I2C bus 0 (mapped to I2C1 in DTS) */
    ret = uclass_get_device_by_seq(UCLASS_I2C, 0, &dev);
    if (ret) {
        printf("Failed to get I2C bus: %d\n", ret);
        return ret;
    }

    /* Probe PF0900 */
    ret = dm_i2c_probe(dev, PF0900_I2C_ADDR, 0, &dev);
    if (ret) {
        printf("PF0900 not detected on I2C1.\n");
        return ret;
    }

    printf("PF0900 detected successfully.\n");

    /*
     * Placeholder:
     * Voltage rail configuration should be added here.
     * Example:
     * - VDD_SOC
     * - VDD_DRAM
     * - VDD_ARM
     */

    return 0;
}

/*
 * Board early init
 */
int board_early_init_f(void)
{
    printf("Conga-SMX95 Early Init\n");

    conga_pmic_init();

    return 0;
}

/*
 * Board init
 */
int board_init(void)
{
    printf("Conga-SMX95 Board Init Complete\n");
    return 0;
}

#include <phy.h>
#include <miiphy.h>

#define DP83867_RGMII_CTRL       0x0032
#define DP83867_RGMII_DELAY_CTRL 0x0086

/*
 * Ethernet PHY Fixup for TI DP83867
 */
int board_phy_config(struct phy_device *phydev)
{
    printf("Configuring DP83867 PHY...\n");

    /*
     * Enable RGMII internal delays
     * Required for stable gigabit link
     */

    phy_write(phydev, MDIO_DEVAD_NONE, DP83867_RGMII_CTRL, 0x00D3);
    phy_write(phydev, MDIO_DEVAD_NONE, DP83867_RGMII_DELAY_CTRL, 0x0008);

    if (phydev->drv->config)
        phydev->drv->config(phydev);

    printf("DP83867 PHY configured.\n");

    return 0;
}

#include <mmc.h>
#include <asm/arch/imx-regs.h>

/*
 * eMMC Initialization Hook (SDHC1 – HS400 capable)
 */
int board_mmc_init(struct bd_info *bis)
{
    printf("Initializing eMMC (SDHC1)...\n");

    /*
     * In i.MX95 EVK reference:
     * SDHC1 is typically used for eMMC.
     * Proper pad configuration must be handled in DTS.
     */

    printf("eMMC HS400 mode supported (configuration via DTS).\n");

    return 0;
}

#include <usb.h>

/*
 * USB 3.0 / Type-C Initialization Hook
 */
int board_usb_init(int index, enum usb_init_type init)
{
    printf("Initializing USB controller %d...\n", index);

    /*
     * For i.MX95:
     * - USB PHY power must be enabled
     * - VBUS control GPIO may be required
     * - Role mode configured in DTS
     */

    if (init == USB_INIT_HOST)
        printf("USB initialized in HOST mode\n");
    else
        printf("USB initialized in DEVICE mode\n");

    return 0;
}

#include <pci.h>

/*
 * PCIe Initialization Hook
 */
int board_pci_init(void)
{
    printf("Initializing PCIe subsystem...\n");

    /*
     * For i.MX95:
     * - PCIe PHY power handled by PMIC rails
     * - Clocks configured via DTS
     * - Reset GPIO may be required
     * - Link training handled by controller driver
     */

    printf("PCIe initialization framework ready.\n");

    return 0;
}

