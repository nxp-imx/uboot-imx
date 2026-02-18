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
