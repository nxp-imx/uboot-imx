// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2013-2017 Freescale Semiconductor, Inc.
 * Copyright 2016-2018,2020 NXP
 */

#include <common.h>
#include <hang.h>
#include <clock_legacy.h>
#include <cpu_func.h>
#include <asm/io.h>
#include <asm/arch/soc.h>
#include <asm/arch/siul.h>
#include <asm/arch/src.h>
#include <fsl_esdhc_imx.h>
#include <mmc.h>
#include <netdev.h>
#include <div64.h>
#include <errno.h>

#define DFS_NONE	0
#define DFS0		1
#define DFS1		2
#define DFS2		3
#define DFS3		4

#define PERIPH_PLL_PHI0_DIV3	3
#define PERIPH_PLL_PHI0_DIV5	5
#define VIDEO_PLL_PHI0_DIV2	2

#define MHZ			1000000
#define MAC_ADDR_STR_LEN	17

#ifdef CONFIG_DCU_QOS_FIX
#define S32V234_NIC_FASTDMA1_IB_READ_QOS	(0x40012380)
#define S32V234_NIC_FASTDMA1_IB_WRITE_QOS	(0x40012384)
#define S32V234_NIC_GPU0_READ_QOS		(0x40012480)
#define S32V234_NIC_GPU0_WRITE_QOS		(0x40012484)
#define S32V234_NIC_H264DEC_READ_QOS		(0x40012580)
#define S32V234_NIC_H264DEC_WRITE_QOS		(0x40012584)
#define S32V234_NIC_GPU1_READ_QOS		(0x40012680)
#define S32V234_NIC_GPU1_WRITE_QOS		(0x40012684)
#define S32V234_NIC_CORES_CCI1_IB_READ_QOS	(0x40012780)
#define S32V234_NIC_CORES_CCI1_IB_WRITE_QOS	(0x40012784)
#define S32V234_NIC_APEX0_DMA_READ_QOS		(0x40012880)
#define S32V234_NIC_APEX0_DMA_WRITE_QOS		(0x40012884)
#define S32V234_NIC_APEX0_BLKDM_A_READ_QOS	(0x40012980)
#define S32V234_NIC_APEX0_BLKDM_A_WRITE_QOS	(0x40012984)
#define S32V234_NIC_APEX1_DMA_READ_QOS		(0x40012A80)
#define S32V234_NIC_APEX1_DMA_WRITE_QOS		(0x40012A84)
#define S32V234_NIC_APEX1_BLKDMA_READ_QOS	(0x40012B80)
#define S32V234_NIC_APEX1_BLKDMA_WRITE_QOS	(0x40012B84)
#define S32V234_NIC_PCIE_READ_QOS		(0x40012C80)
#define S32V234_NIC_PCIE_WRITE_QOS		(0x40012C84)
#define S32V234_NIC_ENET0_READ_QOS		(0x40012D80)
#define S32V234_NIC_ENET0_WRITE_QOS		(0x40012D84)
#define S32V234_NIC_ENET1_READ_QOS		(0x40012E80)
#define S32V234_NIC_ENET1_WRITE_QOS		(0x40012E84)
#define S32V234_NIC_CORES_CCI0_READ_QOS		(0x40012F80)
#define S32V234_NIC_CORES_CCI0_WRITE_QOS	(0x40012F84)
#define S32V234_NIC_AXBS_READ_QOS		(0x40013080)
#define S32V234_NIC_AXBS_WRITE_QOS		(0x40013084)
#define S32V234_NIC_PDI0_READ_QOS		(0x40013180)
#define S32V234_NIC_PDI0_WRITE_QOS		(0x40013184)
#define S32V234_NIC_PDI1_READ_QOS		(0x40013280)
#define S32V234_NIC_PDI1_WRITE_QOS		(0x40013284)
#define S32V234_NIC_AXBS_RAM_READ_QOS		(0x40013380)
#define S32V234_NIC_AXBS_RAM_WRITE_QOS		(0x40013384)
#define S32V234_NIC_FASTDMA0_READ_QOS		(0x40013480)
#define S32V234_NIC_FASTDMA0_WRITE_QOS		(0x40013484)

#define S32V234_NIC_RESET			(0x0)

/* SRC_GPR8 bit fields */
#define SRC_GPR8_2D_ACE_QOS_OFFSET				0
#define MIN_DCU_QOS_PRIORITY					0xD

#endif /* CONFIG_DCU_QOS_FIX */

u32 get_cpu_rev(void)
{
	struct mscm_ir *mscmir = (struct mscm_ir *)MSCM_BASE_ADDR;
	u32 cpu = readl(&mscmir->cpxtype);

	return cpu;
}

DECLARE_GLOBAL_DATA_PTR;

static u32 get_pllfreq(u32 pll, u32 refclk_freq, u32 plldv,
		       u32 pllfd, u32 selected_output)
{
	u32 plldv_prediv = 0, plldv_mfd = 0,	pllfd_mfn = 0;
	u32 plldv_rfdphi_div = 0, fout = 0;
	u32 dfs_portn = 0, dfs_mfn = 0, dfs_mfi = 0;
	double vco = 0;

	if (selected_output > DFS_MAXNUMBER) {
		return -1;
	}

	plldv_prediv =
	    (plldv & PLLDIG_PLLDV_PREDIV_MASK) >> PLLDIG_PLLDV_PREDIV_OFFSET;
	plldv_mfd = (plldv & PLLDIG_PLLDV_MFD_MASK);

	pllfd_mfn = (pllfd & PLLDIG_PLLFD_MFN_MASK);

	plldv_prediv = plldv_prediv == 0 ? 1 : plldv_prediv;

	/* The formula for VCO is from TR manual, rev. 1 */
	vco = (refclk_freq / (double)plldv_prediv) *
	       (plldv_mfd + pllfd_mfn / (double)20480);

	if (selected_output != DFS_NONE) {
		/* Determine the RFDPHI for PHI1 */
		plldv_rfdphi_div =
		    (plldv & PLLDIG_PLLDV_RFDPHI1_MASK) >>
		    PLLDIG_PLLDV_RFDPHI1_OFFSET;
		plldv_rfdphi_div = plldv_rfdphi_div == 0 ? 1 : plldv_rfdphi_div;
		if (pll == ARM_PLL || pll == ENET_PLL || pll == DDR_PLL) {
			dfs_portn =
			    readl(DFS_DVPORTn(pll, selected_output - 1));
			dfs_mfi =
			    (dfs_portn & DFS_DVPORTn_MFI_MASK) >>
			    DFS_DVPORTn_MFI_OFFSET;
			dfs_mfn =
			    (dfs_portn & DFS_DVPORTn_MFN_MASK) >>
			    DFS_DVPORTn_MFN_OFFSET;

			dfs_mfi <<= 8;
			vco /= plldv_rfdphi_div;
			fout = vco / (dfs_mfi + dfs_mfn);
			fout <<= 8;
		} else {
			fout = vco / plldv_rfdphi_div;
		}

	} else {
		/* Determine the RFDPHI for PHI0 */
		plldv_rfdphi_div =
		    (plldv & PLLDIG_PLLDV_RFDPHI_MASK) >>
		    PLLDIG_PLLDV_RFDPHI_OFFSET;
		fout = vco / plldv_rfdphi_div;
	}

	return fout;

}

/* Implemented for ARMPLL, PERIPH_PLL, ENET_PLL, DDR_PLL, VIDEO_LL */
static u32 decode_pll(enum pll_type pll, u32 refclk_freq,
		      u32 selected_output)
{
	u32 plldv, pllfd, freq;

	plldv = readl(PLLDIG_PLLDV(pll));
	pllfd = readl(PLLDIG_PLLFD(pll));

	freq = get_pllfreq(pll, refclk_freq, plldv, pllfd, selected_output);
	return freq  < 0 ? 0 : freq;
}

static u32 sys_source_clk_get(uintptr_t cgm_addr)
{
	return MC_CGM_SC_SEL_GET(readl(CGM_SC_SS(cgm_addr)));
}

static u32 sys_div_clk_get(uintptr_t cgm_addr, u8 dc)
{
	return MC_CGM_SC_DIV_GET(readl(CGM_SC_DCn(cgm_addr, dc)));
}

static u32 aux_source_clk_get(uintptr_t cgm_addr, u8 ac)
{
	return MC_CGM_ACn_SEL_GET(readl(CGM_ACn_SS(cgm_addr, ac)));
}

static u32 aux_div_clk_get(uintptr_t cgm_addr, u8 ac, u8 dc)
{
	return MC_CGM_ACn_DIV_GET(readl(CGM_ACn_DCm(cgm_addr, ac, dc)));
}

static u32 get_mcu_main_clk(void)
{
	u32 coreclk_div;
	u32 sysclk_sel;
	u32 freq = 0;

	sysclk_sel = sys_source_clk_get(MC_CGM1_BASE_ADDR);
	coreclk_div = sys_div_clk_get(MC_CGM1_BASE_ADDR, CGM_SCn_DC0);

	switch (sysclk_sel) {
	case MC_CGM_SC_SEL_FIRC:
		freq = FIRC_CLK_FREQ;
		break;
	case MC_CGM_SC_SEL_XOSC:
		freq = XOSC_CLK_FREQ;
		break;
	case MC_CGM_SC_SEL_ARMPLL:
		/* ARMPLL has as source XOSC and CORE_CLK has as input PHI0 */
		freq = decode_pll(ARM_PLL, XOSC_CLK_FREQ, DFS_NONE);
		break;
	case MC_CGM_SC_SEL_CLKDISABLE:
		printf("Sysclk is disabled\n");
		break;
	default:
		printf("unsupported system clock select\n");
		freq = 0;
	}

	return freq / coreclk_div;
}

static u32 get_sys_clk(u32 number)
{
	u32 sysclk_div, sysclk_div_number;
	u32 sysclk_sel;
	u32 freq = 0;

	switch (number) {
	case MXC_SYS3_CLK:
		sysclk_div_number = CGM_SCn_DC0;
		break;
	case MXC_SYS6_CLK:
		sysclk_div_number = CGM_SCn_DC1;
		break;
	default:
		printf("unsupported system clock \n");
		sysclk_div_number = 0;
	}
	sysclk_sel = sys_source_clk_get(MC_CGM0_BASE_ADDR);
	sysclk_div = sys_div_clk_get(MC_CGM0_BASE_ADDR, sysclk_div_number);

	switch (sysclk_sel) {
	case MC_CGM_SC_SEL_FIRC:
		freq = FIRC_CLK_FREQ;
		break;
	case MC_CGM_SC_SEL_XOSC:
		freq = XOSC_CLK_FREQ;
		break;
	case MC_CGM_SC_SEL_ARMPLL:
		/* ARMPLL has as source XOSC and SYSn_CLK has as input DFS1 */
		freq = decode_pll(ARM_PLL, XOSC_CLK_FREQ, DFS0);
		break;
	case MC_CGM_SC_SEL_CLKDISABLE:
		printf("Sysclk is disabled\n");
		freq = 0;
		break;
	default:
		printf("unsupported system clock select\n");
		freq = 0;
	}
	return freq / sysclk_div;
}

static u32 get_peripherals_clk(void)
{
	u32 auxclk5_div, auxclk5_sel, freq = 0;

	auxclk5_sel = aux_source_clk_get(MC_CGM0_BASE_ADDR, CGM_AC5_SC);
	auxclk5_div = aux_div_clk_get(MC_CGM0_BASE_ADDR, CGM_AC5_SC,
				      CGM_ACn_DC0);

	switch (auxclk5_sel) {
	case MC_CGM_ACn_SEL_FIRC:
		freq = FIRC_CLK_FREQ;
		break;
	case MC_CGM_ACn_SEL_XOSC:
		freq = XOSC_CLK_FREQ;
		break;
	case MC_CGM_ACn_SEL_PERPLLDIVX:
		freq = decode_pll(PERIPH_PLL, XOSC_CLK_FREQ, DFS_NONE) /
				  PERIPH_PLL_PHI0_DIV5;
		break;
	default:
		printf("unsupported source clock\n");
		freq = 0;
	}

	return freq / auxclk5_div;
}

static u32 get_uart_clk(void)
{
	u32 auxclk3_div, auxclk3_sel, freq = 0;

	auxclk3_sel = aux_source_clk_get(MC_CGM0_BASE_ADDR, CGM_AC3_SC);
	auxclk3_div = aux_div_clk_get(MC_CGM0_BASE_ADDR, CGM_AC3_SC,
				      CGM_ACn_DC0);

	switch (auxclk3_sel) {
	case MC_CGM_ACn_SEL_FIRC:
		freq = FIRC_CLK_FREQ;
		break;
	case MC_CGM_ACn_SEL_XOSC:
		freq = XOSC_CLK_FREQ;
		break;
	case MC_CGM_ACn_SEL_PERPLLDIVX:
		freq = decode_pll(PERIPH_PLL, XOSC_CLK_FREQ, DFS_NONE) /
			PERIPH_PLL_PHI0_DIV3;
		break;
	case MC_CGM_ACn_SEL_SYSCLK:
		freq = get_sys_clk(MXC_SYS6_CLK);
		break;
	default:
		printf("unsupported system clock select\n");
		freq = 0;
	}

	return freq / auxclk3_div;
}

static u32 get_fec_clk(void)
{
	u32 auxclk2_div;
	u32 freq = 0;

	auxclk2_div = aux_div_clk_get(MC_CGM2_BASE_ADDR, CGM_AC2_SC,
				      CGM_ACn_DC0);

	freq = decode_pll(ENET_PLL, XOSC_CLK_FREQ, DFS_NONE);

	return freq / auxclk2_div;
}

static u32 get_usdhc_clk(void)
{
	u32 auxclk15_div;
	u32 freq = 0;

	auxclk15_div = aux_div_clk_get(MC_CGM0_BASE_ADDR, CGM_AC15_SC,
				       CGM_ACn_DC0);

	freq = decode_pll(ENET_PLL, XOSC_CLK_FREQ, DFS3);

	return freq / auxclk15_div;
}

static u32 get_i2c_clk(void)
{
	return get_peripherals_clk();
}

static u32 get_qspi_clk(void)
{
	u32 auxclk14_div, auxclk14_sel, freq = 0;

	auxclk14_sel = aux_source_clk_get(MC_CGM0_BASE_ADDR, CGM_AC14_SC);
	auxclk14_div = aux_div_clk_get(MC_CGM0_BASE_ADDR, CGM_AC14_SC,
				       CGM_ACn_DC0);

	switch (auxclk14_sel) {
	case MC_CGM_ACn_SEL_FIRC:
		freq = FIRC_CLK_FREQ;
		break;
	case MC_CGM_ACn_SEL_XOSC:
		freq = XOSC_CLK_FREQ;
		break;
	case MC_CGM_ACn_SEL_ENETPLL:
		freq = decode_pll(ENET_PLL, XOSC_CLK_FREQ, DFS2);
		break;
	default:
		printf("unsupported system clock select\n");
		freq = 0;
	}

	return freq / auxclk14_div;
}

static u32 get_dcu_pix_clk(void)
{
	u32 auxclk9_div, auxclk9_sel;
	u32 freq = 0;

	auxclk9_sel = aux_source_clk_get(MC_CGM0_BASE_ADDR, CGM_AC9_SC);
	auxclk9_div = aux_div_clk_get(MC_CGM0_BASE_ADDR, CGM_AC9_SC,
					 CGM_ACn_DC1);

	switch (auxclk9_sel) {
	case MC_CGM_ACn_SEL_FIRC:
		freq = FIRC_CLK_FREQ;
		break;
	case MC_CGM_ACn_SEL_XOSC:
		freq = XOSC_CLK_FREQ;
		break;
	case MC_CGM_ACn_SEL_VIDEOPLLDIV2:
		freq = decode_pll(VIDEO_PLL, XOSC_CLK_FREQ, DFS_NONE) /
				VIDEO_PLL_PHI0_DIV2;
		break;
	default:
		printf("unsupported source clock select\n");
		freq = 0;
	}

	return freq / auxclk9_div;
}

static u32 get_dspi_clk(void)
{
	return get_sys_clk(MXC_SYS6_CLK);
}

/* return clocks in Hz */
unsigned int mxc_get_clock(enum mxc_clock clk)
{
	switch (clk) {
	case MXC_ARM_CLK:
		return get_mcu_main_clk();
	case MXC_PERIPHERALS_CLK:
		return get_peripherals_clk();
	case MXC_UART_CLK:
		return get_uart_clk();
	case MXC_FEC_CLK:
		return get_fec_clk();
	case MXC_I2C_CLK:
		return get_i2c_clk();
	case MXC_USDHC_CLK:
		return get_usdhc_clk();
	case MXC_QSPI_CLK:
		return get_qspi_clk();
	case MXC_DCU_PIX_CLK:
		return get_dcu_pix_clk();
	case MXC_DSPI_CLK:
		return get_dspi_clk();
	default:
		break;
	}
	printf("Error: Unsupported function to read the frequency! \
			Please define it correctly!");
	return 0;
}

int do_s32_showclocks(cmd_tbl_t *cmdtp, int flag, int argc,
		      char * const argv[])
{
	printf("Root clocks:\n");
	printf("CPU clock:%5d MHz\n",
	       mxc_get_clock(MXC_ARM_CLK) / MHZ);
	printf("PERIPHERALS clock: %5d MHz\n",
	       mxc_get_clock(MXC_PERIPHERALS_CLK) / MHZ);
	printf("uSDHC clock:	%5d MHz\n",
	       mxc_get_clock(MXC_USDHC_CLK) / MHZ);
	printf("FEC clock:	%5d MHz\n",
	       mxc_get_clock(MXC_FEC_CLK) / MHZ);
	printf("UART clock:	%5d MHz\n",
	       mxc_get_clock(MXC_UART_CLK) / MHZ);
	printf("QSPI clock:	%5d MHz\n",
	       mxc_get_clock(MXC_QSPI_CLK) / MHZ);
	printf("DSPI clock:	%5d MHz\n",
	       mxc_get_clock(MXC_DSPI_CLK) / MHZ);

	return 0;
}

U_BOOT_CMD(clocks, CONFIG_SYS_MAXARGS, 1, do_s32_showclocks,
	   "display clocks",
	   ""
	   );

#ifdef CONFIG_FEC_MXC
void imx_get_mac_from_fuse(int dev_id, unsigned char *mac)
{
	const char *mac_str = S32V234_FEC_DEFAULT_ADDR;

	if ((!env_get("ethaddr")) ||
	    (strncasecmp(mac_str, env_get("ethaddr"), MAC_ADDR_STR_LEN) == 0)) {
		printf("\nWarning: System is using default MAC address. ");
		printf("Please set a new value\n");
		string_to_enetaddr(mac_str, mac);
	} else {
		string_to_enetaddr(env_get("ethdaddr"), mac);
	}
}
#endif

#if defined(CONFIG_DISPLAY_CPUINFO)
static char *get_reset_cause(void)
{
	u32 cause = readl(MC_RGM_FES);

	switch (cause) {
	case F_SWT4:
		return "WDOG";
	case F_JTAG:
		return "JTAG";
	case F_FCCU_SOFT:
		return "FCCU soft reaction";
	case F_FCCU_HARD:
		return "FCCU hard reaction";
	case F_SOFT_FUNC:
		return "Software Functional reset";
	case F_ST_DONE:
		return "Self Test done reset";
	case F_EXT_RST:
		return "External reset";
	default:
		return "unknown reset";
	}

}

void reset_cpu(ulong addr)
{
	entry_to_target_mode(MC_ME_MCTL_RESET);

	/* If we get there, we are not in good shape */
	mdelay(1000);
	printf("FATAL: Reset Failed!\n");
	hang();
};

int print_cpuinfo(void)
{
	int speed;
	u32 osc_freq;
	struct src *src = (struct src *)SRC_SOC_BASE_ADDR;

	speed = get_siul2_midr2_speed();
	if (speed != SIUL2_MIDR2_SPEED_1GHZ &&
	    speed != SIUL2_MIDR2_SPEED_800MHZ) {
		printf("Warning: ");
		if (speed == SIUL2_MIDR2_SPEED_600MHZ)
			printf("Unsupported speed grading: 600 MHz. ");
		else
			printf("Unknown speed grading: %#x. ", speed);
		if (readl(&src->gpr1) &
		    (SRC_GPR1_PLL_SOURCE_MASK << SRC_GPR1_PLL_OFFSET))
			osc_freq = XOSC_CLK_FREQ;
		else
			osc_freq = FIRC_CLK_FREQ;
		printf("ARM-PLL frequency was configured to %u MHz\n",
		       decode_pll(ARM_PLL, osc_freq, 0) / MHZ);
	}

	printf("CPU:   NXP S32V234 V%d.%d at %d MHz\n",
	       get_siul2_midr1_major() + 1, get_siul2_midr1_minor(),
	       mxc_get_clock(MXC_ARM_CLK) / MHZ);
	printf("Reset cause: %s\n", get_reset_cause());

	return 0;
}
#endif

int cpu_eth_init(bd_t *bis)
{
	int rc = -ENODEV;

#if defined(CONFIG_FEC_MXC)
	volatile struct src *src_regs = (struct src *)SRC_SOC_BASE_ADDR;

	/* enable RGMII mode */
#if (CONFIG_FEC_XCV_TYPE == RGMII)
	clrsetbits_le32(&src_regs->gpr3, SRC_GPR3_ENET_MODE,
			SRC_GPR3_ENET_MODE);
#else
	clrsetbits_le32(&src_regs->gpr3, SRC_GPR3_ENET_MODE,
			0);
#endif

#ifdef CONFIG_FEC_MXC_PHYADDR
	rc = fecmxc_initialize(bis);
#endif /* CONFIG_FEC_MXC_PHYADDR */

#endif /* CONFIG_FEC_MXC */

	return rc;
}

void cpu_pci_clock_init(const int clockexternal)
{
	volatile struct src *src_regs = (struct src *)SRC_SOC_BASE_ADDR;

	clrsetbits_le32(&src_regs->gpr3,
			SRC_GPR3_PCIE_RFCC_CLK,
			(clockexternal) ? SRC_GPR3_PCIE_RFCC_CLK : 0);
	/* The RM does not state that a delay is required.
	 * I want to be on the safe side however to ensure clock
	 * stability before checking the link. With respect to boot
	 * time delays, this can be revisited later on.
	 */
	udelay(100);
}

static int detect_boot_interface(void)
{
	volatile struct src *src = (struct src *)SRC_SOC_BASE_ADDR;

	u32 reg_val;
	int value;

	reg_val = readl(&src->bmr1);
	value = reg_val & SRC_BMR1_CFG1_MASK;
	value = value >> SRC_BMR1_CFG1_BOOT_SHIFT;

	if (value != SRC_BMR1_CFG1_QuadSPI &&
	    value != SRC_BMR1_CFG1_SD &&
	    value != SRC_BMR1_CFG1_eMMC) {
		printf("Unknown booting environment\n");
		value = -1;
	}

	return value;
}

int get_clocks(void)
{
#ifdef CONFIG_FSL_ESDHC_IMX
	gd->arch.sdhc_clk = mxc_get_clock(MXC_USDHC_CLK);
#endif
	return 0;
}

__weak void setup_iomux_enet(void)
{
#ifndef CONFIG_PHY_RGMII_DIRECT_CONNECTED
	/* set PC13 - MSCR[45] - for MDC */
	writel(SIUL2_MSCR_ENET_MDC, SIUL2_MSCRn(SIUL2_MSCR_PC13));

	/* set PC14 - MSCR[46] - for MDIO */
	writel(SIUL2_MSCR_ENET_MDIO, SIUL2_MSCRn(SIUL2_MSCR_PC14));
	writel(SIUL2_MSCR_ENET_MDIO_IN, SIUL2_MSCRn(SIUL2_MSCR_PC14_IN));
#endif

#ifdef CONFIG_PHY_RGMII_DIRECT_CONNECTED
	/* set PC15 - MSCR[47] - for TX CLK SWITCH */
	writel(SIUL2_MSCR_ENET_TX_CLK_SWITCH,
	       SIUL2_MSCRn(SIUL2_MSCR_PC15_SWITCH));
#else
	/* set PC15 - MSCR[47] - for TX CLK */
	writel(SIUL2_MSCR_ENET_TX_CLK, SIUL2_MSCRn(SIUL2_MSCR_PC15));
#endif
	writel(SIUL2_MSCR_ENET_TX_CLK_IN, SIUL2_MSCRn(SIUL2_MSCR_PC15_IN));

	/* set PD0 - MSCR[48] - for RX_CLK */
	writel(SIUL2_MSCR_ENET_RX_CLK, SIUL2_MSCRn(SIUL2_MSCR_PD0));
	writel(SIUL2_MSCR_ENET_RX_CLK_IN, SIUL2_MSCRn(SIUL2_MSCR_PD0_IN));

	/* set PD1 - MSCR[49] - for RX_D0 */
	writel(SIUL2_MSCR_ENET_RX_D0, SIUL2_MSCRn(SIUL2_MSCR_PD1));
	writel(SIUL2_MSCR_ENET_RX_D0_IN, SIUL2_MSCRn(SIUL2_MSCR_PD1_IN));

	/* set PD2 - MSCR[50] - for RX_D1 */
	writel(SIUL2_MSCR_ENET_RX_D1, SIUL2_MSCRn(SIUL2_MSCR_PD2));
	writel(SIUL2_MSCR_ENET_RX_D1_IN, SIUL2_MSCRn(SIUL2_MSCR_PD2_IN));

	/* set PD3 - MSCR[51] - for RX_D2 */
	writel(SIUL2_MSCR_ENET_RX_D2, SIUL2_MSCRn(SIUL2_MSCR_PD3));
	writel(SIUL2_MSCR_ENET_RX_D2_IN, SIUL2_MSCRn(SIUL2_MSCR_PD3_IN));

	/* set PD4 - MSCR[52] - for RX_D3 */
	writel(SIUL2_MSCR_ENET_RX_D3, SIUL2_MSCRn(SIUL2_MSCR_PD4));
	writel(SIUL2_MSCR_ENET_RX_D3_IN, SIUL2_MSCRn(SIUL2_MSCR_PD4_IN));

	/* set PD5 - MSCR[53] - for RX_DV */
	writel(SIUL2_MSCR_ENET_RX_DV, SIUL2_MSCRn(SIUL2_MSCR_PD5));
	writel(SIUL2_MSCR_ENET_RX_DV_IN, SIUL2_MSCRn(SIUL2_MSCR_PD5_IN));

	/* set PD7 - MSCR[55] - for TX_D0 */
	writel(SIUL2_MSCR_ENET_TX_D0, SIUL2_MSCRn(SIUL2_MSCR_PD7));
	/* set PD8 - MSCR[56] - for TX_D1 */
	writel(SIUL2_MSCR_ENET_TX_D1, SIUL2_MSCRn(SIUL2_MSCR_PD8));
	/* set PD9 - MSCR[57] - for TX_D2 */
	writel(SIUL2_MSCR_ENET_TX_D2, SIUL2_MSCRn(SIUL2_MSCR_PD9));
	/* set PD10 - MSCR[58] - for TX_D3 */
	writel(SIUL2_MSCR_ENET_TX_D3, SIUL2_MSCRn(SIUL2_MSCR_PD10));
	/* set PD11 - MSCR[59] - for TX_EN */
	writel(SIUL2_MSCR_ENET_TX_EN, SIUL2_MSCRn(SIUL2_MSCR_PD11));
}

#ifdef CONFIG_FSL_DCU_FB
__weak void setup_iomux_dcu(void)
{
	/* set PH8 - MSCR[120] - for HSYNC_C1 */
	writel(SIUL2_MSCR_DCU_CFG, SIUL2_MSCRn(SIUL2_MSCR_PH8));
	/* set PH9 - MSCR[121] - for VSYNC_C2 */
	writel(SIUL2_MSCR_DCU_CFG, SIUL2_MSCRn(SIUL2_MSCR_PH9));
	/* set PH10 - MSCR[122] - for DE_C3 */
	writel(SIUL2_MSCR_DCU_CFG, SIUL2_MSCRn(SIUL2_MSCR_PH10));
	/* set PH12 - MSCR[124] - for PCLK_D1 */
	writel(SIUL2_MSCR_DCU_CFG, SIUL2_MSCRn(SIUL2_MSCR_PH12));
	/* set PH13 - MSCR[125] - for R0_D2 */
	writel(SIUL2_MSCR_DCU_CFG, SIUL2_MSCRn(SIUL2_MSCR_PH13));
	/* set PH14 - MSCR[126] - for R1_D3 */
	writel(SIUL2_MSCR_DCU_CFG, SIUL2_MSCRn(SIUL2_MSCR_PH14));
	/* set PH15 - MSCR[127] - for R2_D4 */
	writel(SIUL2_MSCR_DCU_CFG, SIUL2_MSCRn(SIUL2_MSCR_PH15));
	/* set PJ0 - MSCR[128] - for R3_D5 */
	writel(SIUL2_MSCR_DCU_CFG, SIUL2_MSCRn(SIUL2_MSCR_PJ0));
	/* set PJ1 - MSCR[129] - for R4_D6 */
	writel(SIUL2_MSCR_DCU_CFG, SIUL2_MSCRn(SIUL2_MSCR_PJ1));
	/* set PJ2 - MSCR[130] - for R5_D7 */
	writel(SIUL2_MSCR_DCU_CFG, SIUL2_MSCRn(SIUL2_MSCR_PJ2));
	/* set PJ3 - MSCR[131] - for R6_D8 */
	writel(SIUL2_MSCR_DCU_CFG, SIUL2_MSCRn(SIUL2_MSCR_PJ3));
	/* set PJ4 - MSCR[132] - for R7_D9 */
	writel(SIUL2_MSCR_DCU_CFG, SIUL2_MSCRn(SIUL2_MSCR_PJ4));
	/* set PJ5 - MSCR[133] - for G0_D10 */
	writel(SIUL2_MSCR_DCU_CFG, SIUL2_MSCRn(SIUL2_MSCR_PJ5));
	/* set PJ6 - MSCR[134] - for G1_D11 */
	writel(SIUL2_MSCR_DCU_CFG, SIUL2_MSCRn(SIUL2_MSCR_PJ6));
	/* set PJ7 - MSCR[135] - for G2_D12 */
	writel(SIUL2_MSCR_DCU_CFG, SIUL2_MSCRn(SIUL2_MSCR_PJ7));
	/* set PJ8 - MSCR[136] - for G3_D13 */
	writel(SIUL2_MSCR_DCU_CFG, SIUL2_MSCRn(SIUL2_MSCR_PJ8));
	/* set PJ9 - MSCR[137] - for G4_D14 */
	writel(SIUL2_MSCR_DCU_CFG, SIUL2_MSCRn(SIUL2_MSCR_PJ9));
	/* set PJ10 - MSCR[138] - for G5_D15 */
	writel(SIUL2_MSCR_DCU_CFG, SIUL2_MSCRn(SIUL2_MSCR_PJ10));
	/* set PJ11 - MSCR[139] - for G6_D16 */
	writel(SIUL2_MSCR_DCU_CFG, SIUL2_MSCRn(SIUL2_MSCR_PJ11));
	/* set PJ12 - MSCR[140] - for G7_D17 */
	writel(SIUL2_MSCR_DCU_CFG, SIUL2_MSCRn(SIUL2_MSCR_PJ12));
	/* set PJ13 - MSCR[141] - for B0_D18 */
	writel(SIUL2_MSCR_DCU_CFG, SIUL2_MSCRn(SIUL2_MSCR_PJ13));
	/* set PJ14 - MSCR[142] - for B1_D19 */
	writel(SIUL2_MSCR_DCU_CFG, SIUL2_MSCRn(SIUL2_MSCR_PJ14));
	/* set PJ15 - MSCR[143] - for B2_D20 */
	writel(SIUL2_MSCR_DCU_CFG, SIUL2_MSCRn(SIUL2_MSCR_PJ15));
	/* set PK0 - MSCR[144] - for B3_D21 */
	writel(SIUL2_MSCR_DCU_CFG, SIUL2_MSCRn(SIUL2_MSCR_PK0));
	/* set PK1 - MSCR[145] - for B4_D22 */
	writel(SIUL2_MSCR_DCU_CFG, SIUL2_MSCRn(SIUL2_MSCR_PK1));
	/* set PK2 - MSCR[146] - for B5_D23 */
	writel(SIUL2_MSCR_DCU_CFG, SIUL2_MSCRn(SIUL2_MSCR_PK2));
	/* set PK3 - MSCR[147] - for B6_D24 */
	writel(SIUL2_MSCR_DCU_CFG, SIUL2_MSCRn(SIUL2_MSCR_PK3));
	/* set PK4 - MSCR[148] - for B7_D25 */
	writel(SIUL2_MSCR_DCU_CFG, SIUL2_MSCRn(SIUL2_MSCR_PK4));
}
#endif

#ifdef CONFIG_DCU_QOS_FIX
int board_dcu_qos(void)
{
	/*
	 * SRC_GPR8.2D_ACE_QOS was introduced in TR2.0 and is used to set
	 * the minimum QOS value for DCU DMA master. Depending on the pixel
	 * output rate of the display, the real time requirements of the 2D-ACE
	 * have to be ensured to avoid underruns of its local buffers. This
	 * requires that other masters of the device interconnect should receive
	 * a lower QoS than programmed by SRC_GPR8.
	 */
	if (get_siul2_midr1_major() == TREERUNNER_GENERATION_2_MAJOR) {
		struct src *src_regs = (struct src *)SRC_SOC_BASE_ADDR;
		u32 val = readl(&src_regs->gpr8);

		writel(val |
		       (MIN_DCU_QOS_PRIORITY << SRC_GPR8_2D_ACE_QOS_OFFSET),
		       &src_regs->gpr8);
	}

	/* m_fastdma1_ib */
	writel(S32V234_NIC_RESET, S32V234_NIC_FASTDMA1_IB_READ_QOS);
	writel(S32V234_NIC_RESET, S32V234_NIC_FASTDMA1_IB_WRITE_QOS);

	/* m_gpu0 */
	writel(S32V234_NIC_RESET, S32V234_NIC_GPU0_READ_QOS);
	writel(S32V234_NIC_RESET, S32V234_NIC_GPU0_WRITE_QOS);

	/* m_h264dec */
	writel(S32V234_NIC_RESET, S32V234_NIC_H264DEC_READ_QOS);
	writel(S32V234_NIC_RESET, S32V234_NIC_H264DEC_WRITE_QOS);

	/* m_gpu1 */
	writel(S32V234_NIC_RESET, S32V234_NIC_GPU1_READ_QOS);
	writel(S32V234_NIC_RESET, S32V234_NIC_GPU1_WRITE_QOS);

	/* m_cores_cci1_ib */
	writel(S32V234_NIC_RESET, S32V234_NIC_CORES_CCI1_IB_READ_QOS);
	writel(S32V234_NIC_RESET, S32V234_NIC_CORES_CCI1_IB_WRITE_QOS);

	/* m_apex0_dma */
	writel(S32V234_NIC_RESET, S32V234_NIC_APEX0_DMA_READ_QOS);
	writel(S32V234_NIC_RESET, S32V234_NIC_APEX0_DMA_WRITE_QOS);

	/* m_apex0_blkdm a */
	writel(S32V234_NIC_RESET, S32V234_NIC_APEX0_BLKDM_A_READ_QOS);
	writel(S32V234_NIC_RESET, S32V234_NIC_APEX0_BLKDM_A_WRITE_QOS);

	/* m_apex1_dma */
	writel(S32V234_NIC_RESET, S32V234_NIC_APEX1_DMA_READ_QOS);
	writel(S32V234_NIC_RESET, S32V234_NIC_APEX1_DMA_WRITE_QOS);

	/* m_apex1_blkdma */
	writel(S32V234_NIC_RESET, S32V234_NIC_APEX1_BLKDMA_READ_QOS);
	writel(S32V234_NIC_RESET, S32V234_NIC_APEX1_BLKDMA_WRITE_QOS);

	/* m_pcie */
	writel(S32V234_NIC_RESET, S32V234_NIC_PCIE_READ_QOS);
	writel(S32V234_NIC_RESET, S32V234_NIC_PCIE_WRITE_QOS);

	/* m_enet0 */
	writel(S32V234_NIC_RESET, S32V234_NIC_ENET0_READ_QOS);
	writel(S32V234_NIC_RESET, S32V234_NIC_ENET0_WRITE_QOS);

	/* m_enet1 */
	writel(S32V234_NIC_RESET, S32V234_NIC_ENET1_READ_QOS);
	writel(S32V234_NIC_RESET, S32V234_NIC_ENET1_WRITE_QOS);

	/* m_cores_cci0 */
	writel(S32V234_NIC_RESET, S32V234_NIC_CORES_CCI0_READ_QOS);
	writel(S32V234_NIC_RESET, S32V234_NIC_CORES_CCI0_WRITE_QOS);

	/* m_axbs */
	writel(S32V234_NIC_RESET, S32V234_NIC_AXBS_READ_QOS);
	writel(S32V234_NIC_RESET, S32V234_NIC_AXBS_WRITE_QOS);

	/* m_pdi0 */
	writel(S32V234_NIC_RESET, S32V234_NIC_PDI0_READ_QOS);
	writel(S32V234_NIC_RESET, S32V234_NIC_PDI0_WRITE_QOS);

	/* m_pdi1 */
	writel(S32V234_NIC_RESET, S32V234_NIC_PDI1_READ_QOS);
	writel(S32V234_NIC_RESET, S32V234_NIC_PDI1_WRITE_QOS);

	/* m_axbs_ram */
	writel(S32V234_NIC_RESET, S32V234_NIC_AXBS_RAM_READ_QOS);
	writel(S32V234_NIC_RESET, S32V234_NIC_AXBS_RAM_WRITE_QOS);

	/* m_fastdma0 */
	writel(S32V234_NIC_RESET, S32V234_NIC_FASTDMA0_READ_QOS);
	writel(S32V234_NIC_RESET, S32V234_NIC_FASTDMA0_WRITE_QOS);

	return 0;
}
#endif

__weak void setup_iomux_sdhc(void)
{
	/* Set iomux PADS for USDHC */

	/* PK6 pad: uSDHC clk */
	writel(SIUL2_USDHC_PAD_CTRL_CLK, SIUL2_MSCRn(150));
	writel(0x3, SIUL2_MSCRn(902));

	/* PK7 pad: uSDHC CMD */
	writel(SIUL2_USDHC_PAD_CTRL_CMD, SIUL2_MSCRn(151));
	writel(0x3, SIUL2_MSCRn(901));

	/* PK8 pad: uSDHC DAT0 */
	writel(SIUL2_USDHC_PAD_CTRL_DAT0_3, SIUL2_MSCRn(152));
	writel(0x3, SIUL2_MSCRn(903));

	/* PK9 pad: uSDHC DAT1 */
	writel(SIUL2_USDHC_PAD_CTRL_DAT0_3, SIUL2_MSCRn(153));
	writel(0x3, SIUL2_MSCRn(904));

	/* PK10 pad: uSDHC DAT2 */
	writel(SIUL2_USDHC_PAD_CTRL_DAT0_3, SIUL2_MSCRn(154));
	writel(0x3, SIUL2_MSCRn(905));

	/* PK11 pad: uSDHC DAT3 */
	writel(SIUL2_USDHC_PAD_CTRL_DAT0_3, SIUL2_MSCRn(155));
	writel(0x3, SIUL2_MSCRn(906));

	/* PK15 pad: uSDHC DAT4 */
	writel(SIUL2_USDHC_PAD_CTRL_DAT4_7, SIUL2_MSCRn(159));
	writel(0x3, SIUL2_MSCRn(907));

	/* PL0 pad: uSDHC DAT5 */
	writel(SIUL2_USDHC_PAD_CTRL_DAT4_7, SIUL2_MSCRn(160));
	writel(0x3, SIUL2_MSCRn(908));

	/* PL1 pad: uSDHC DAT6 */
	writel(SIUL2_USDHC_PAD_CTRL_DAT4_7, SIUL2_MSCRn(161));
	writel(0x3, SIUL2_MSCRn(909));

	/* PL2 pad: uSDHC DAT7 */
	writel(SIUL2_USDHC_PAD_CTRL_DAT4_7, SIUL2_MSCRn(162));
	writel(0x3, SIUL2_MSCRn(910));
}

#ifdef CONFIG_FSL_ESDHC_IMX
struct fsl_esdhc_cfg esdhc_cfg[1] = {
	{USDHC_BASE_ADDR},
};

__weak int board_mmc_getcd(struct mmc *mmc)
{
	/* eSDHC1 is always present */
	return 1;
}

__weak int sdhc_setup(bd_t *bis)
{
	esdhc_cfg[0].sdhc_clk = mxc_get_clock(MXC_USDHC_CLK);

	setup_iomux_sdhc();

	return fsl_esdhc_initialize(bis, &esdhc_cfg[0]);
}

int board_mmc_init(bd_t *bis)
{
	int ret = detect_boot_interface();

	if (ret < 0)
		return -1;

	if (ret != SRC_BMR1_CFG1_QuadSPI)
		return sdhc_setup(bis);
	else
		return 0;
}

static int do_sdhc_setup(cmd_tbl_t *cmdtp, int flag, int argc,
			 char * const argv[])
{
	int ret;
	struct mmc *mmc = find_mmc_device(0);

	printf("Hyperflash is disabled. SD/eMMC is active and can be used\n");

	if (!mmc)
		return sdhc_setup(gd->bd);

	/* set the sdhc pinmuxing */
	setup_iomux_sdhc();

	/* reforce the mmc's initialization */
	ret = mmc_init(mmc);
	if (ret) {
		printf("Impossible to configure the SDHC controller. Please check the SDHC jumpers\n");
		return 1;
	}
	return 0;
}

/* sdhc setup */
U_BOOT_CMD(sdhcsetup, 1, 1, do_sdhc_setup,
	   "setup sdhc pinmuxing and sdhc registers for access to SD",
	   "\n"
	   "Set up the sdhc pinmuxing and sdhc registers to access the SD\n"
	   "and disconnect from the Hyperflash.\n"
);
#endif

void setup_iomux_ddr(void)
{
	ddr_config_iomux(DDR0);
	ddr_config_iomux(DDR1);
}

void ddr_ctrl_init(void)
{
	config_mmdc(0);
	config_mmdc(1);
}

#ifdef CONFIG_DDR_HANDSHAKE_AT_RESET
void ddr_check_post_func_reset(uint8_t module)
{
	u32 ddr_self_ref_clr, mmdc_mapsr;
	unsigned long mmdc_addr;
	volatile struct src *src = (struct src *)SRC_SOC_BASE_ADDR;

	mmdc_addr = (module) ? MMDC1_BASE_ADDR : MMDC0_BASE_ADDR;
	ddr_self_ref_clr = (module) ? SRC_DDR_EN_SELF_REF_CTRL_DDR1_SLF_REF_CLR
				    : SRC_DDR_EN_SELF_REF_CTRL_DDR0_SLF_REF_CLR;

	/* Check if DDR is still in refresh mode */
	if (src->ddr_self_ref_ctrl & ddr_self_ref_clr) {
		mmdc_mapsr = readl(mmdc_addr + MMDC_MAPSR);
		writel(mmdc_mapsr | MMDC_MAPSR_EN_SLF_REF,
		       mmdc_addr + MMDC_MAPSR);

		src->ddr_self_ref_ctrl =
			src->ddr_self_ref_ctrl | ddr_self_ref_clr;

		mmdc_mapsr = readl(mmdc_addr + MMDC_MAPSR);
		writel(mmdc_mapsr & ~MMDC_MAPSR_EN_SLF_REF,
		       mmdc_addr + MMDC_MAPSR);
	}
}
#endif

__weak int dram_init(void)
{
#ifdef CONFIG_DDR_HANDSHAKE_AT_RESET
	u32 enabled_hs_events, func_event;

	if (readl(MC_RGM_DDR_HE) & MC_RGM_DDR_HE_EN) {
		/* Enable DDR handshake for all functional events */
		volatile struct src *src = (struct src *)SRC_SOC_BASE_ADDR;

		src->ddr_self_ref_ctrl = src->ddr_self_ref_ctrl |
					 SRC_DDR_EN_SLF_REF_VALUE;

		/* If reset event was received, check DDR state */
		func_event = readl(MC_RGM_FES);
		enabled_hs_events = readl(MC_RGM_FRHE);
		if (func_event & enabled_hs_events) {
			if (func_event & MC_RGM_FES_ANY_FUNC_EVENT) {
				/* Check if DDR handshake was done */
				while (!(readl(MC_RGM_DDR_HS) &
				       MC_RGM_DDR_HS_HNDSHK_DONE))
					;

				ddr_check_post_func_reset(DDR0);
				ddr_check_post_func_reset(DDR1);
			}
		}
	} else {
		/*
		 * First boot so the handshake isn't necessary.
		 * We should only enable it for future functional resets.
		 */
		writel(MC_RGM_DDR_HE_VALUE, MC_RGM_DDR_HE);
		writel(MC_RGM_FRHE_ALL_VALUE, MC_RGM_FRHE);
	}
#endif
	setup_iomux_ddr();

	ddr_ctrl_init();

	gd->ram_size = get_ram_size((void *)PHYS_SDRAM, PHYS_SDRAM_SIZE);

	return 0;
}
