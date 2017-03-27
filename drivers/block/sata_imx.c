/*
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <linux/errno.h>
#include <asm/io.h>
#include <asm/imx-common/sci/sci.h>
#include <ahci.h>
#include <scsi.h>
#include <imx8_hsio.h>

int sata_init(void)
{
	int ret;
	u32 val, i = 0;

	printf("start sata init\n");
	writel(0x22222222, GPR_LPCG_PHYX2APB_0_APB);
	writel(0x22222222, GPR_LPCG_PHYX1_APB);

	setbits_le32(0x5F130008, BIT(21));
	setbits_le32(0x5F130008, BIT(23));

	/* PHY_MODE to SATA100Mhz ref clk */
	setbits_le32(HW_PHYX1_CTRL0_ADDR, BIT(19));

	/*
	 * bit 0 rx ena, bit 1 tx ena, bit 11 fast_init,
	 * bit12 PHY_X1_EPCS_SEL 1.
	 */
	setbits_le32(HW_MISC_CTRL0_ADDR, HW_MISC_CTRL0_IOB_RXENA
		     | HW_MISC_CTRL0_PHY_X1_EPCS_SEL);

	clrbits_le32(HW_SATA_CTRL0_ADDR, HW_SATA_CTRL0_PHY_RESET);
	setbits_le32(HW_SATA_CTRL0_ADDR, HW_SATA_CTRL0_PHY_RESET);
	setbits_le32(HW_SATA_CTRL0_ADDR, HW_SATA_CTRL0_RESET);
	udelay(1);
	clrbits_le32(HW_SATA_CTRL0_ADDR, HW_SATA_CTRL0_RESET);
	setbits_le32(HW_SATA_CTRL0_ADDR, HW_SATA_CTRL0_RESET);

	setbits_le32(HW_PHYX1_CTRL0_ADDR, HW_PHYX1_CTRL0_APB_RSTN);

	for (i = 0; i < 100; i++) {
		val = readl(HW_PHYX1_STTS0_ADDR);
		val &= HW_PHYX1_STTS0_LANE0_TX_PLL_LOCK;
		if (val == HW_PHYX1_STTS0_LANE0_TX_PLL_LOCK)
			break;
		udelay(1);
	}

	if (val != HW_PHYX1_STTS0_LANE0_TX_PLL_LOCK) {
		printf("TX PLL is not locked.\n");
		return -ENODEV;
	}

	ret = ahci_init((void __iomem *)AHCI_BASE_ADDR);
	if (ret)
		return ret;
	scsi_scan(1);

	return 0;
}
