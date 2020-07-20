// SPDX-License-Identifier:     GPL-2.0+
/*
 * (C) Copyright 2015-2016 Freescale Semiconductor, Inc.
 * (C) Copyright 2017 NXP
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/siul.h>
#include <asm/arch/ddr3.h>
#include <asm/arch/mmdc.h>

void ddr_config_iomux(uint8_t module)
{
	int i;

	switch (module) {
	case DDR0:
		writel(DDR3_RESET_PAD, SIUL2_MSCRn(_DDR0_RESET));

		writel(DDR3_CLK0_PAD, SIUL2_MSCRn(_DDR0_CLK0));

		writel(DDR3_CAS_PAD, SIUL2_MSCRn(_DDR0_CAS));
		writel(DDR3_RAS_PAD, SIUL2_MSCRn(_DDR0_RAS));

		writel(DDR3_WE_B_PAD, SIUL2_MSCRn(_DDR0_WE_B));

		writel(DDR3_CKEn_PAD, SIUL2_MSCRn(_DDR0_CKE0));
		writel(DDR3_CKEn_PAD, SIUL2_MSCRn(_DDR0_CKE1));

		writel(DDR3_CS_Bn_PAD, SIUL2_MSCRn(_DDR0_CS_B0));
		writel(DDR3_CS_Bn_PAD, SIUL2_MSCRn(_DDR0_CS_B1));

		for (i = _DDR0_BA0; i <= _DDR0_BA2; i++)
			writel(DDR3_BAn_PAD, SIUL2_MSCRn(i));

		for (i = _DDR0_DM0; i <= _DDR0_DM3; i++)
			writel(DDR3_DMn_PAD, SIUL2_MSCRn(i));

		for (i = _DDR0_DQS0; i <= _DDR0_DQS3; i++)
			writel(DDR3_DQSn_PAD, SIUL2_MSCRn(i));

		writel(DDR3_ODTn_PAD, SIUL2_MSCRn(_DDR0_ODT0));
		writel(DDR3_ODTn_PAD, SIUL2_MSCRn(_DDR0_ODT1));

		for (i = _DDR0_A0; i < _DDR0_A15; i++)
			writel(DDR3_An_PAD, SIUL2_MSCRn(i));

		for (i = _DDR0_D0; i <= _DDR0_D31; i++)
			writel(DDR3_Dn_PAD, SIUL2_MSCRn(i));
		break;
	case DDR1:
		writel(DDR3_RESET_PAD, SIUL2_MSCRn(_DDR1_RESET));

		writel(DDR3_CLK0_PAD, SIUL2_MSCRn(_DDR1_CLK0));

		writel(DDR3_CAS_PAD, SIUL2_MSCRn(_DDR1_CAS));
		writel(DDR3_RAS_PAD, SIUL2_MSCRn(_DDR1_RAS));

		writel(DDR3_WE_B_PAD, SIUL2_MSCRn(_DDR1_WE_B));

		writel(DDR3_CKEn_PAD, SIUL2_MSCRn(_DDR1_CKE0));
		writel(DDR3_CKEn_PAD, SIUL2_MSCRn(_DDR1_CKE1));

		writel(DDR3_CS_Bn_PAD, SIUL2_MSCRn(_DDR1_CS_B0));
		writel(DDR3_CS_Bn_PAD, SIUL2_MSCRn(_DDR1_CS_B1));

		for (i = _DDR1_BA0; i <= _DDR1_BA2; i++)
			writel(DDR3_BAn_PAD, SIUL2_MSCRn(i));

		for (i = _DDR1_DM0; i <= _DDR1_DM3; i++)
			writel(DDR3_DMn_PAD, SIUL2_MSCRn(i));

		for (i = _DDR1_DQS0; i <= _DDR1_DQS3; i++)
			writel(DDR3_DQSn_PAD, SIUL2_MSCRn(i));

		writel(DDR3_ODTn_PAD, SIUL2_MSCRn(_DDR1_ODT0));
		writel(DDR3_ODTn_PAD, SIUL2_MSCRn(_DDR1_ODT1));

		for (i = _DDR1_A0; i < _DDR1_A15; i++)
			writel(DDR3_An_PAD, SIUL2_MSCRn(i));

		for (i = _DDR1_D0; i <= _DDR1_D31; i++)
			writel(DDR3_Dn_PAD, SIUL2_MSCRn(i));
		break;
	}
}

void config_mmdc(uint8_t module)
{
	unsigned long mmdc_addr = (module) ? MMDC1_BASE_ADDR : MMDC0_BASE_ADDR;

	writel(MMDC_MDSCR_CFG_VALUE, mmdc_addr + MMDC_MDSCR);

	writel(MMDC_MDCFG0_VALUE, mmdc_addr + MMDC_MDCFG0);
	writel(MMDC_MDCFG1_VALUE, mmdc_addr + MMDC_MDCFG1);
	writel(MMDC_MDCFG2_VALUE, mmdc_addr + MMDC_MDCFG2);
	writel(MMDC_MDOTC_VALUE, mmdc_addr + MMDC_MDOTC);
	writel(MMDC_MDMISC_VALUE, mmdc_addr + MMDC_MDMISC);
	writel(MMDC_MDOR_VALUE, mmdc_addr + MMDC_MDOR);
	writel(_MDCTL, mmdc_addr + MMDC_MDCTL);

	/* Perform ZQ calibration */
	writel(MMDC_MPZQHWCTRL_VALUE, mmdc_addr + MMDC_MPZQHWCTRL);
	while (readl(mmdc_addr + MMDC_MPZQHWCTRL) & MMDC_MPZQHWCTRL_ZQ_HW_FOR)
		;

	/* Enable MMDC with CS0 */
	writel(_MDCTL + 0x80000000, mmdc_addr + MMDC_MDCTL);

	/* Complete the initialization sequence as defined by JEDEC */
	writel(MMDC_MDSCR_MR2_VALUE, mmdc_addr + MMDC_MDSCR);
	writel(MMDC_MDSCR_MR3_VALUE, mmdc_addr + MMDC_MDSCR);
	writel(MMDC_MDSCR_MR1_VALUE, mmdc_addr + MMDC_MDSCR);
	writel(MMDC_MDSCR_MR0_VALUE, mmdc_addr + MMDC_MDSCR);
	writel(MMDC_MDSCR_ZQ_VALUE, mmdc_addr + MMDC_MDSCR);

	/* Set the amount of DRAM */
	/* Set DQS settings based on board type */
	switch (module) {
	case MMDC0:
		writel(MMDC_MDASP_MODULE0_VALUE, mmdc_addr + MMDC_MDASP);
		writel(MMDC_MPRDDLCTL_MODULE0_VALUE,
		       mmdc_addr + MMDC_MPRDDLCTL);
		writel(MMDC_MPWRDLCTL_MODULE0_VALUE,
		       mmdc_addr + MMDC_MPWRDLCTL);
		writel(MMDC_MPDGCTRL0_MODULE0_VALUE,
		       mmdc_addr + MMDC_MPDGCTRL0);
		writel(MMDC_MPDGCTRL1_MODULE0_VALUE,
		       mmdc_addr + MMDC_MPDGCTRL1);
	break;
	case MMDC1:
		writel(MMDC_MDASP_MODULE1_VALUE, mmdc_addr + MMDC_MDASP);
		writel(MMDC_MPRDDLCTL_MODULE1_VALUE,
		       mmdc_addr + MMDC_MPRDDLCTL);
		writel(MMDC_MPWRDLCTL_MODULE1_VALUE,
		       mmdc_addr + MMDC_MPWRDLCTL);
		writel(MMDC_MPDGCTRL0_MODULE1_VALUE,
		       mmdc_addr + MMDC_MPDGCTRL0);
		writel(MMDC_MPDGCTRL1_MODULE1_VALUE,
		       mmdc_addr + MMDC_MPDGCTRL1);
		break;
	}

	writel(MMDC_MDRWD_VALUE, mmdc_addr + MMDC_MDRWD);
	writel(MMDC_MDPDC_VALUE, mmdc_addr + MMDC_MDPDC);
	writel(MMDC_MDREF_VALUE, mmdc_addr + MMDC_MDREF);
	writel(MMDC_MPODTCTRL_VALUE, mmdc_addr + MMDC_MPODTCTRL);
	writel(MMDC_MDSCR_RESET_VALUE, mmdc_addr + MMDC_MDSCR);

#if defined(CONFIG_DDR_INIT_DELAY)
	if (module == MMDC1)
		udelay(CONFIG_DDR_INIT_DELAY);
#endif
}
