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

