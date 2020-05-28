// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2018-2020 NXP
 * Peng Fan <peng.fan@nxp.com>
 */

#include <common.h>
#include <clk-uclass.h>
#include <dm.h>
#include <asm/arch/sci/sci.h>
#include <asm/arch/clock.h>
#include <dt-bindings/clock/imx8qxp-clock.h>
#include <dt-bindings/clock/imx8qm-clock.h>
#include <dt-bindings/soc/imx_rsrc.h>
#include <misc.h>
#include <asm/arch/lpcg.h>

#define CLK_IMX8_MAX_MUX_SEL 5

#define FLAG_CLK_IMX8_IMX8QM	BIT(0)
#define FLAG_CLK_IMX8_IMX8QXP	BIT(1)

struct imx8_clks {
	ulong id;
	const char *name;
	u16 rsrc;
	sc_pm_clk_t pm_clk;
};

struct imx8_fixed_clks {
	ulong id;
	const char *name;
	ulong rate;
};

struct imx8_gpr_clks {
	ulong id;
	const char *name;
	u16 rsrc;
	sc_ctrl_t gpr_id;
	ulong parent_id;
};

struct imx8_lpcg_clks {
	ulong id;
	const char *name;
	u8 bit_idx;
	u32 lpcg;
	ulong parent_id;
};

struct imx8_mux_clks {
	ulong id;
	const char *name;
	ulong slice_clk_id;
	ulong parent_clks[CLK_IMX8_MAX_MUX_SEL];
};

#if defined(CONFIG_IMX8QXP) || defined(CONFIG_IMX8DXL)
static struct imx8_clks imx8qxp_clks[] = {
	{ IMX8QXP_A35_DIV, "A35_DIV", SC_R_A35, SC_PM_CLK_CPU },
	{ IMX8QXP_I2C0_DIV, "I2C0_DIV", SC_R_I2C_0, SC_PM_CLK_PER },
	{ IMX8QXP_I2C1_DIV, "I2C1_DIV", SC_R_I2C_1, SC_PM_CLK_PER },
	{ IMX8QXP_I2C2_DIV, "I2C2_DIV", SC_R_I2C_2, SC_PM_CLK_PER },
	{ IMX8QXP_I2C3_DIV, "I2C3_DIV", SC_R_I2C_3, SC_PM_CLK_PER },
	{ IMX8QXP_MIPI0_I2C0_DIV, "MIPI0 I2C0_DIV", SC_R_MIPI_0_I2C_0, SC_PM_CLK_MISC2 },
	{ IMX8QXP_MIPI0_I2C1_DIV, "MIPI0 I2C1_DIV", SC_R_MIPI_0_I2C_1, SC_PM_CLK_MISC2 },
	{ IMX8QXP_MIPI1_I2C0_DIV, "MIPI1 I2C0_DIV", SC_R_MIPI_1_I2C_0, SC_PM_CLK_MISC2 },
	{ IMX8QXP_MIPI1_I2C1_DIV, "MIPI1 I2C1_DIV", SC_R_MIPI_1_I2C_1, SC_PM_CLK_MISC2 },
	{ IMX8QXP_CSI0_I2C0_DIV, "CSI0 I2C0_DIV", SC_R_CSI_0_I2C_0, SC_PM_CLK_PER },
	{ IMX8QXP_UART0_DIV, "UART0_DIV", SC_R_UART_0, SC_PM_CLK_PER },
	{ IMX8QXP_UART1_DIV, "UART1_DIV", SC_R_UART_1, SC_PM_CLK_PER },
	{ IMX8QXP_UART2_DIV, "UART2_DIV", SC_R_UART_2, SC_PM_CLK_PER },
	{ IMX8QXP_UART3_DIV, "UART3_DIV", SC_R_UART_3, SC_PM_CLK_PER },
	{ IMX8QXP_SDHC0_DIV, "SDHC0_DIV", SC_R_SDHC_0, SC_PM_CLK_PER },
	{ IMX8QXP_SDHC1_DIV, "SDHC1_DIV", SC_R_SDHC_1, SC_PM_CLK_PER },
	{ IMX8QXP_SDHC2_DIV, "SDHC2_DIV", SC_R_SDHC_2, SC_PM_CLK_PER },
#if !defined(CONFIG_IMX8DXL)
	{ IMX8QXP_ENET0_ROOT_DIV, "ENET0_ROOT_DIV", SC_R_ENET_0, SC_PM_CLK_PER },
#endif
	{ IMX8QXP_ENET0_RGMII_DIV, "ENET0_RGMII_DIV", SC_R_ENET_0, SC_PM_CLK_MISC0 },
	{ IMX8QXP_ENET1_ROOT_DIV, "ENET1_ROOT_DIV", SC_R_ENET_1, SC_PM_CLK_PER },
	{ IMX8QXP_ENET1_RGMII_DIV, "ENET1_RGMII_DIV", SC_R_ENET_1, SC_PM_CLK_MISC0 },

	{ IMX8QXP_USB3_ACLK_DIV, "USB3_ACLK_DIV", SC_R_USB_2, SC_PM_CLK_PER },
	{ IMX8QXP_USB3_BUS_DIV, "USB3_BUS_DIV", SC_R_USB_2, SC_PM_CLK_MST_BUS },
	{ IMX8QXP_USB3_LPM_DIV, "USB3_LPM_DIV", SC_R_USB_2, SC_PM_CLK_MISC },

	{ IMX8QXP_LSIO_FSPI0_DIV, "FSPI0_DIV", SC_R_FSPI_0, SC_PM_CLK_PER },

	{ IMX8QXP_GPMI_BCH_IO_DIV, "GPMI_IO_DIV", SC_R_NAND, SC_PM_CLK_MST_BUS },
	{ IMX8QXP_GPMI_BCH_DIV, "GPMI_BCH_DIV", SC_R_NAND, SC_PM_CLK_PER },
};

static struct imx8_fixed_clks imx8qxp_fixed_clks[] = {
	{ IMX8QXP_IPG_CONN_CLK_ROOT, "IPG_CONN_CLK", SC_83MHZ },
	{ IMX8QXP_AHB_CONN_CLK_ROOT, "AHB_CONN_CLK", SC_166MHZ },
	{ IMX8QXP_AXI_CONN_CLK_ROOT, "AXI_CONN_CLK", SC_333MHZ },
	{ IMX8QXP_IPG_DMA_CLK_ROOT, "IPG_DMA_CLK", SC_120MHZ },
	{ IMX8QXP_MIPI_IPG_CLK, "IPG_MIPI_CLK", SC_120MHZ },
	{ IMX8QXP_LSIO_BUS_CLK, "LSIO_BUS_CLK", SC_100MHZ },
	{ IMX8QXP_LSIO_MEM_CLK, "LSIO_MEM_CLK", SC_200MHZ },
	{ IMX8QXP_HSIO_PER_CLK, "HSIO_CLK", SC_133MHZ },
	{ IMX8QXP_HSIO_AXI_CLK, "HSIO_AXI", SC_400MHZ },
#if defined(CONFIG_IMX8DXL)
	{ IMX8QXP_ENET0_ROOT_DIV, "ENET0_ROOT_DIV", SC_250MHZ },
#endif
};

static struct imx8_gpr_clks imx8qxp_gpr_clks[] = {
	{ IMX8QXP_ENET0_REF_DIV, "ENET0_REF_DIV", SC_R_ENET_0, SC_C_CLKDIV, IMX8QXP_ENET0_ROOT_DIV },
	{ IMX8QXP_ENET0_REF_25MHZ_125MHZ_SEL, "ENET0_REF_25_125", SC_R_ENET_0, SC_C_SEL_125 },
	{ IMX8QXP_ENET0_RMII_TX_SEL, "ENET0_RMII_TX", SC_R_ENET_0, SC_C_TXCLK },
	{ IMX8QXP_ENET0_REF_25MHZ_125MHZ_CLK, "ENET0_REF_25_125_CLK", SC_R_ENET_0, SC_C_DISABLE_125 },
	{ IMX8QXP_ENET0_REF_50MHZ_CLK, "ENET0_REF_50", SC_R_ENET_0, SC_C_DISABLE_50 },

	{ IMX8QXP_ENET1_REF_DIV, "ENET1_REF_DIV", SC_R_ENET_1, SC_C_CLKDIV, IMX8QXP_ENET1_ROOT_DIV },
	{ IMX8QXP_ENET1_REF_25MHZ_125MHZ_SEL, "ENET1_REF_25_125", SC_R_ENET_1, SC_C_SEL_125 },
	{ IMX8QXP_ENET1_RMII_TX_SEL, "ENET1_RMII_TX", SC_R_ENET_1, SC_C_TXCLK },
	{ IMX8QXP_ENET1_REF_25MHZ_125MHZ_CLK, "ENET1_REF_25_125_CLK", SC_R_ENET_1, SC_C_DISABLE_125 },
	{ IMX8QXP_ENET1_REF_50MHZ_CLK, "ENET1_REF_50", SC_R_ENET_1, SC_C_DISABLE_50 },
};

static struct imx8_lpcg_clks imx8qxp_lpcg_clks[] = {
	{ IMX8QXP_I2C0_CLK, "I2C0_CLK", 0, LPI2C_0_LPCG, IMX8QXP_I2C0_DIV },
	{ IMX8QXP_I2C0_IPG_CLK, "I2C0_IPG", 16, LPI2C_0_LPCG, IMX8QXP_IPG_DMA_CLK_ROOT },
	{ IMX8QXP_I2C1_CLK, "I2C1_CLK", 0, LPI2C_1_LPCG, IMX8QXP_I2C1_DIV },
	{ IMX8QXP_I2C1_IPG_CLK, "I2C1_IPG", 16, LPI2C_1_LPCG, IMX8QXP_IPG_DMA_CLK_ROOT },
	{ IMX8QXP_I2C2_CLK, "I2C2_CLK", 0, LPI2C_2_LPCG, IMX8QXP_I2C2_DIV },
	{ IMX8QXP_I2C2_IPG_CLK, "I2C2_IPG", 16, LPI2C_2_LPCG, IMX8QXP_IPG_DMA_CLK_ROOT },
	{ IMX8QXP_I2C3_CLK, "I2C3_CLK", 0, LPI2C_3_LPCG, IMX8QXP_I2C3_DIV },
	{ IMX8QXP_I2C3_IPG_CLK, "I2C3_IPG", 16, LPI2C_3_LPCG, IMX8QXP_IPG_DMA_CLK_ROOT },
	{ IMX8QXP_MIPI0_I2C0_CLK, "MIPI0_I2C0_CLK", 0, DI_MIPI0_LPCG + 0x10, IMX8QXP_MIPI0_I2C0_DIV },
	{ IMX8QXP_MIPI0_I2C0_IPG_CLK, "MIPI0_I2C0_IPG", 16, DI_MIPI0_LPCG + 0x10, IMX8QXP_MIPI_IPG_CLK },
	{ IMX8QXP_MIPI0_I2C1_CLK, "MIPI0_I2C1_CLK", 0, DI_MIPI0_LPCG + 0x14, IMX8QXP_MIPI0_I2C1_DIV },
	{ IMX8QXP_MIPI0_I2C1_IPG_CLK, "MIPI0_I2C1_IPG", 16, DI_MIPI0_LPCG + 0x14, IMX8QXP_MIPI_IPG_CLK },
	{ IMX8QXP_MIPI1_I2C0_CLK, "MIPI1_I2C0_CLK", 0, DI_MIPI1_LPCG + 0x10, IMX8QXP_MIPI1_I2C0_DIV },
	{ IMX8QXP_MIPI1_I2C0_IPG_CLK, "MIPI1_I2C0_IPG", 16, DI_MIPI1_LPCG + 0x10, IMX8QXP_MIPI_IPG_CLK },
	{ IMX8QXP_MIPI1_I2C1_CLK, "MIPI1_I2C1_CLK", 0, DI_MIPI1_LPCG + 0x14, IMX8QXP_MIPI1_I2C1_DIV },
	{ IMX8QXP_MIPI1_I2C1_IPG_CLK, "MIPI1_I2C1_IPG", 16, DI_MIPI1_LPCG + 0x14, IMX8QXP_MIPI_IPG_CLK },
	{ IMX8QXP_CSI0_I2C0_CLK, "CSI0_I2C0_CLK", 0, MIPI_CSI_0_LPCG + 0x14, IMX8QXP_CSI0_I2C0_DIV },
	{ IMX8QXP_CSI0_I2C0_IPG_CLK, "CSI0_I2C0_IPG", 16, MIPI_CSI_0_LPCG + 0x14, IMX8QXP_MIPI_IPG_CLK },

	{ IMX8QXP_UART0_CLK, "UART0_CLK", 0, LPUART_0_LPCG, IMX8QXP_UART0_DIV },
	{ IMX8QXP_UART0_IPG_CLK, "UART0_IPG", 16, LPUART_0_LPCG, IMX8QXP_IPG_DMA_CLK_ROOT },
	{ IMX8QXP_UART1_CLK, "UART1_CLK", 0, LPUART_1_LPCG, IMX8QXP_UART1_DIV },
	{ IMX8QXP_UART1_IPG_CLK, "UART1_IPG", 16, LPUART_1_LPCG, IMX8QXP_IPG_DMA_CLK_ROOT },
	{ IMX8QXP_UART2_CLK, "UART2_CLK", 0, LPUART_2_LPCG, IMX8QXP_UART2_DIV },
	{ IMX8QXP_UART2_IPG_CLK, "UART2_IPG", 16, LPUART_2_LPCG, IMX8QXP_IPG_DMA_CLK_ROOT },
	{ IMX8QXP_UART3_CLK, "UART3_CLK", 0, LPUART_3_LPCG, IMX8QXP_UART3_DIV },
	{ IMX8QXP_UART3_IPG_CLK, "UART3_IPG", 16, LPUART_3_LPCG, IMX8QXP_IPG_DMA_CLK_ROOT },

	{ IMX8QXP_SDHC0_CLK, "SDHC0_CLK", 0, USDHC_0_LPCG, IMX8QXP_SDHC0_DIV },
	{ IMX8QXP_SDHC0_IPG_CLK, "SDHC0_IPG", 16, USDHC_0_LPCG, IMX8QXP_IPG_CONN_CLK_ROOT },
	{ IMX8QXP_SDHC0_AHB_CLK, "SDHC0_AHB", 20, USDHC_0_LPCG, IMX8QXP_AHB_CONN_CLK_ROOT },
	{ IMX8QXP_SDHC1_CLK, "SDHC1_CLK", 0, USDHC_1_LPCG, IMX8QXP_SDHC1_DIV },
	{ IMX8QXP_SDHC1_IPG_CLK, "SDHC1_IPG", 16, USDHC_1_LPCG, IMX8QXP_IPG_CONN_CLK_ROOT },
	{ IMX8QXP_SDHC1_AHB_CLK, "SDHC1_AHB", 20, USDHC_1_LPCG, IMX8QXP_AHB_CONN_CLK_ROOT },
	{ IMX8QXP_SDHC2_CLK, "SDHC2_CLK", 0, USDHC_2_LPCG, IMX8QXP_SDHC2_DIV },
	{ IMX8QXP_SDHC2_IPG_CLK, "SDHC2_IPG", 16, USDHC_2_LPCG, IMX8QXP_IPG_CONN_CLK_ROOT },
	{ IMX8QXP_SDHC2_AHB_CLK, "SDHC2_AHB", 20, USDHC_2_LPCG, IMX8QXP_AHB_CONN_CLK_ROOT },

	{ IMX8QXP_ENET0_IPG_S_CLK, "ENET0_IPG_S", 20, ENET_0_LPCG, IMX8QXP_IPG_CONN_CLK_ROOT },
	{ IMX8QXP_ENET0_IPG_CLK, "ENET0_IPG", 16, ENET_0_LPCG, IMX8QXP_ENET0_IPG_S_CLK },
	{ IMX8QXP_ENET0_AHB_CLK, "ENET0_AHB", 8, ENET_0_LPCG, IMX8QXP_AXI_CONN_CLK_ROOT },
	{ IMX8QXP_ENET0_TX_CLK, "ENET0_TX", 4, ENET_0_LPCG, IMX8QXP_ENET0_ROOT_DIV },
	{ IMX8QXP_ENET0_PTP_CLK, "ENET0_PTP", 0, ENET_0_LPCG, IMX8QXP_ENET0_ROOT_DIV  },
	{ IMX8QXP_ENET0_RGMII_TX_CLK, "ENET0_RGMII_TX", 12, ENET_0_LPCG, IMX8QXP_ENET0_RMII_TX_SEL  },
	{ IMX8QXP_ENET0_RMII_RX_CLK, "ENET0_RMII_RX", 0, ENET_0_LPCG + 0x4, IMX8QXP_ENET0_RGMII_DIV  },

	{ IMX8QXP_ENET1_IPG_S_CLK, "ENET1_IPG_S", 20, ENET_1_LPCG, IMX8QXP_IPG_CONN_CLK_ROOT },
	{ IMX8QXP_ENET1_IPG_CLK, "ENET1_IPG", 16, ENET_1_LPCG, IMX8QXP_ENET1_IPG_S_CLK },
	{ IMX8QXP_ENET1_AHB_CLK, "ENET1_AHB", 8, ENET_1_LPCG, IMX8QXP_AXI_CONN_CLK_ROOT },
	{ IMX8QXP_ENET1_TX_CLK, "ENET1_TX", 4, ENET_1_LPCG, IMX8QXP_ENET1_ROOT_DIV },
	{ IMX8QXP_ENET1_PTP_CLK, "ENET1_PTP", 0, ENET_1_LPCG, IMX8QXP_ENET1_ROOT_DIV  },
	{ IMX8QXP_ENET1_RGMII_TX_CLK, "ENET1_RGMII_TX", 12, ENET_1_LPCG, IMX8QXP_ENET1_RMII_TX_SEL  },
	{ IMX8QXP_ENET1_RMII_RX_CLK, "ENET1_RMII_RX", 0, ENET_1_LPCG + 0x4, IMX8QXP_ENET1_RGMII_DIV  },

#if defined(CONFIG_IMX8DXL)
	{ IMX8DXL_EQOS_MEM_CLK, "EQOS_MEM_CLK", 8, ENET_1_LPCG, IMX8QXP_AXI_CONN_CLK_ROOT },
	{ IMX8DXL_EQOS_ACLK, "EQOS_ACLK", 16, ENET_1_LPCG, IMX8DXL_EQOS_MEM_CLK },
	{ IMX8DXL_EQOS_CSR_CLK, "EQOS_CSR_CLK", 24, ENET_1_LPCG, IMX8QXP_IPG_CONN_CLK_ROOT },
	{ IMX8DXL_EQOS_CLK, "EQOS_CLK", 20, ENET_1_LPCG, IMX8QXP_ENET1_ROOT_DIV },
	{ IMX8DXL_EQOS_PTP_CLK_S, "EQOS_PTP_S", 8, ENET_1_LPCG, IMX8QXP_ENET0_ROOT_DIV  },
	{ IMX8DXL_EQOS_PTP_CLK, "EQOS_PTP", 0, ENET_1_LPCG, IMX8DXL_EQOS_PTP_CLK_S  },
#endif

	{ IMX8QXP_LSIO_FSPI0_IPG_S_CLK, "FSPI0_IPG_S", 0x18, FSPI_0_LPCG, IMX8QXP_LSIO_BUS_CLK },
	{ IMX8QXP_LSIO_FSPI0_IPG_CLK, "FSPI0_IPG", 0x14, FSPI_0_LPCG, IMX8QXP_LSIO_FSPI0_IPG_S_CLK },
	{ IMX8QXP_LSIO_FSPI0_HCLK, "FSPI0_HCLK", 0x10, FSPI_0_LPCG, IMX8QXP_LSIO_MEM_CLK },
	{ IMX8QXP_LSIO_FSPI0_CLK, "FSPI0_CLK", 0, FSPI_0_LPCG, IMX8QXP_LSIO_FSPI0_DIV },

#if !defined(CONFIG_IMX8DXL)
	{ IMX8QXP_USB2_OH_AHB_CLK, "USB2_OH_AHB", 24, USB_2_LPCG, IMX8QXP_AHB_CONN_CLK_ROOT },
	{ IMX8QXP_USB2_OH_IPG_S_CLK, "USB2_OH_IPG_S", 16, USB_2_LPCG, IMX8QXP_IPG_CONN_CLK_ROOT },
	{ IMX8QXP_USB2_OH_IPG_S_PL301_CLK, "USB2_OH_IPG_S_PL301", 20, USB_2_LPCG, IMX8QXP_IPG_CONN_CLK_ROOT },
#endif

	{ IMX8QXP_USB2_PHY_IPG_CLK, "USB2_PHY_IPG", 28, USB_2_LPCG, IMX8QXP_IPG_CONN_CLK_ROOT },

	{ IMX8QXP_USB3_IPG_CLK, "USB3_IPG", 16, USB_3_LPCG, IMX8QXP_IPG_CONN_CLK_ROOT },
	{ IMX8QXP_USB3_CORE_PCLK, "USB3_CORE", 20, USB_3_LPCG, IMX8QXP_IPG_CONN_CLK_ROOT },
	{ IMX8QXP_USB3_PHY_CLK, "USB3_PHY", 24, USB_3_LPCG, IMX8QXP_USB3_IPG_CLK },

#if defined(CONFIG_IMX8DXL)
	{ IMX8DXL_USB2_PHY2_IPG_CLK, "USB3_ACLK", 28, USB_3_LPCG, IMX8QXP_IPG_CONN_CLK_ROOT },
#endif
	{ IMX8QXP_USB3_ACLK, "USB3_ACLK", 28, USB_3_LPCG, IMX8QXP_USB3_ACLK_DIV },
	{ IMX8QXP_USB3_BUS_CLK, "USB3_BUS", 0, USB_3_LPCG, IMX8QXP_USB3_BUS_DIV },
	{ IMX8QXP_USB3_LPM_CLK, "USB3_LPM", 4, USB_3_LPCG, IMX8QXP_USB3_LPM_DIV },

	{ IMX8QXP_GPMI_APB_CLK, "GPMI_APB", 16, NAND_LPCG, IMX8QXP_AXI_CONN_CLK_ROOT },
	{ IMX8QXP_GPMI_APB_BCH_CLK, "GPMI_APB_BCH", 20, NAND_LPCG, IMX8QXP_AXI_CONN_CLK_ROOT },
	{ IMX8QXP_GPMI_BCH_IO_CLK, "GPMI_IO_CLK", 4, NAND_LPCG, IMX8QXP_GPMI_BCH_IO_DIV },
	{ IMX8QXP_GPMI_BCH_CLK, "GPMI_BCH_CLK", 0, NAND_LPCG, IMX8QXP_GPMI_BCH_DIV },
	{ IMX8QXP_APBHDMA_CLK, "GPMI_CLK", 16, NAND_LPCG + 0x4, IMX8QXP_AXI_CONN_CLK_ROOT },

	{ IMX8QXP_HSIO_PCIE_MSTR_AXI_CLK, "HSIO_PCIE_A_MSTR_AXI_CLK", 16, HSIO_PCIE_X1_LPCG, IMX8QXP_HSIO_AXI_CLK },
	{ IMX8QXP_HSIO_PCIE_SLV_AXI_CLK, "HSIO_PCIE_A_SLV_AXI_CLK", 20, HSIO_PCIE_X1_LPCG, IMX8QXP_HSIO_AXI_CLK },
	{ IMX8QXP_HSIO_PCIE_DBI_AXI_CLK, "HSIO_PCIE_A_DBI_AXI_CLK", 24, HSIO_PCIE_X1_LPCG, IMX8QXP_HSIO_AXI_CLK },
	{ IMX8QXP_HSIO_PCIE_X1_PER_CLK, "HSIO_PCIE_X1_PER_CLK", 16, HSIO_PCIE_X1_CRR3_LPCG, IMX8QXP_HSIO_PER_CLK },
	{ IMX8QXP_HSIO_PHY_X1_PER_CLK, "HSIO_PHY_X1_PER_CLK", 16, HSIO_PHY_X1_CRR1_LPCG, IMX8QXP_HSIO_PER_CLK },
	{ IMX8QXP_HSIO_MISC_PER_CLK, "HSIO_MISC_PER_CLK", 16, HSIO_MISC_LPCG, IMX8QXP_HSIO_PER_CLK },
	{ IMX8QXP_HSIO_PHY_X1_APB_CLK, "HSIO_PHY_X1_APB_CLK", 16, HSIO_PHY_X1_LPCG, IMX8QXP_HSIO_PER_CLK },
	{ IMX8QXP_HSIO_GPIO_CLK, "HSIO_GPIO_CLK", 16, HSIO_GPIO_LPCG, IMX8QXP_HSIO_PER_CLK },
	{ IMX8QXP_HSIO_PHY_X1_PCLK, "HSIO_PHY_X1_PCLK", 0, HSIO_PHY_X1_LPCG, 0 },
};

struct imx8_mux_clks imx8qxp_mux_clks[] = {
	{ IMX8QXP_SDHC0_SEL, "SDHC0_SEL", IMX8QXP_SDHC0_DIV, {IMX8QXP_CLK_DUMMY, IMX8QXP_CONN_PLL0_CLK,
		IMX8QXP_CONN_PLL1_CLK, IMX8QXP_CLK_DUMMY, IMX8QXP_CLK_DUMMY} },
	{ IMX8QXP_SDHC1_SEL, "SDHC1_SEL", IMX8QXP_SDHC1_DIV, {IMX8QXP_CLK_DUMMY, IMX8QXP_CONN_PLL0_CLK,
		IMX8QXP_CONN_PLL1_CLK, IMX8QXP_CLK_DUMMY, IMX8QXP_CLK_DUMMY} },
	{ IMX8QXP_SDHC2_SEL, "SDHC2_SEL", IMX8QXP_SDHC2_DIV, {IMX8QXP_CLK_DUMMY, IMX8QXP_CONN_PLL0_CLK,
		IMX8QXP_CONN_PLL1_CLK, IMX8QXP_CLK_DUMMY, IMX8QXP_CLK_DUMMY} },
};
#endif

#ifdef CONFIG_IMX8QM
static struct imx8_clks imx8qm_clks[] = {
	{ IMX8QM_A53_DIV, "A53_DIV", SC_R_A53, SC_PM_CLK_CPU, },
	{ IMX8QM_A72_DIV, "A72_DIV", SC_R_A72, SC_PM_CLK_CPU, },
	{ IMX8QM_I2C0_DIV, "I2C0_DIV", SC_R_I2C_0, SC_PM_CLK_PER },
	{ IMX8QM_I2C1_DIV, "I2C1_DIV", SC_R_I2C_1, SC_PM_CLK_PER },
	{ IMX8QM_I2C2_DIV, "I2C2_DIV", SC_R_I2C_2, SC_PM_CLK_PER },
	{ IMX8QM_I2C3_DIV, "I2C3_DIV", SC_R_I2C_3, SC_PM_CLK_PER },
	{ IMX8QM_LVDS0_I2C0_DIV, "LVDS0 I2C0 DIV", SC_R_LVDS_0_I2C_0, SC_PM_CLK_PER },
	{ IMX8QM_LVDS0_I2C1_DIV, "LVDS0 I2C1 DIV", SC_R_LVDS_0_I2C_1, SC_PM_CLK_PER },
	{ IMX8QM_LVDS1_I2C0_DIV, "LVDS1 I2C0 DIV", SC_R_LVDS_1_I2C_0, SC_PM_CLK_PER },
	{ IMX8QM_LVDS1_I2C1_DIV, "LVDS1 I2C1 DIV", SC_R_LVDS_1_I2C_1, SC_PM_CLK_PER },
	{ IMX8QM_MIPI0_I2C0_DIV, "MIPI0 I2C0_DIV", SC_R_MIPI_0_I2C_0, SC_PM_CLK_MISC2 },
	{ IMX8QM_MIPI0_I2C1_DIV, "MIPI0 I2C1_DIV", SC_R_MIPI_0_I2C_1, SC_PM_CLK_MISC2 },
	{ IMX8QM_MIPI1_I2C0_DIV, "MIPI1 I2C0_DIV", SC_R_MIPI_1_I2C_0, SC_PM_CLK_MISC2 },
	{ IMX8QM_MIPI1_I2C1_DIV, "MIPI1 I2C1_DIV", SC_R_MIPI_1_I2C_1, SC_PM_CLK_MISC2 },
	{ IMX8QM_CSI0_I2C0_DIV, "CSI0 I2C0_DIV", SC_R_CSI_0_I2C_0, SC_PM_CLK_PER },
	{ IMX8QM_CSI1_I2C0_DIV, "CSI1 I2C0_DIV", SC_R_CSI_1_I2C_0, SC_PM_CLK_PER },
	{ IMX8QM_HDMI_I2C0_DIV, "HDMI I2C0_DIV", SC_R_HDMI_I2C_0, SC_PM_CLK_MISC2 },
	{ IMX8QM_HDMI_IPG_CLK, "HDMI IPG_CLK", SC_R_HDMI, SC_PM_CLK_MISC },
	{ IMX8QM_HDMI_RX_I2C0_DIV, "HDMI RX I2C_DIV", SC_R_HDMI_RX_I2C_0, SC_PM_CLK_MISC2 },
	{ IMX8QM_UART0_DIV, "UART0_DIV", SC_R_UART_0, SC_PM_CLK_PER },
	{ IMX8QM_UART1_DIV, "UART1_DIV", SC_R_UART_1, SC_PM_CLK_PER },
	{ IMX8QM_UART2_DIV, "UART2_DIV", SC_R_UART_2, SC_PM_CLK_PER },
	{ IMX8QM_UART3_DIV, "UART3_DIV", SC_R_UART_3, SC_PM_CLK_PER },
	{ IMX8QM_SDHC0_DIV, "SDHC0_DIV", SC_R_SDHC_0, SC_PM_CLK_PER },
	{ IMX8QM_SDHC1_DIV, "SDHC1_DIV", SC_R_SDHC_1, SC_PM_CLK_PER },
	{ IMX8QM_SDHC2_DIV, "SDHC2_DIV", SC_R_SDHC_2, SC_PM_CLK_PER },
	{ IMX8QM_ENET0_ROOT_DIV, "ENET0_ROOT_DIV", SC_R_ENET_0, SC_PM_CLK_PER },
	{ IMX8QM_ENET0_RGMII_DIV, "ENET0_RGMII_DIV", SC_R_ENET_0, SC_PM_CLK_MISC0 },
	{ IMX8QM_ENET1_ROOT_DIV, "ENET1_ROOT_DIV", SC_R_ENET_1, SC_PM_CLK_PER },
	{ IMX8QM_ENET1_RGMII_DIV, "ENET1_RGMII_DIV", SC_R_ENET_1, SC_PM_CLK_MISC0 },

	{ IMX8QM_USB3_ACLK_DIV, "USB3_ACLK_DIV", SC_R_USB_2, SC_PM_CLK_PER },
	{ IMX8QM_USB3_BUS_DIV, "USB3_BUS_DIV", SC_R_USB_2, SC_PM_CLK_MST_BUS },
	{ IMX8QM_USB3_LPM_DIV, "USB3_LPM_DIV", SC_R_USB_2, SC_PM_CLK_MISC },

	{ IMX8QM_FSPI0_DIV, "FSPI0_DIV", SC_R_FSPI_0, SC_PM_CLK_PER },

	{ IMX8QM_GPMI_BCH_IO_DIV, "GPMI_IO_DIV", SC_R_NAND, SC_PM_CLK_MST_BUS },
	{ IMX8QM_GPMI_BCH_DIV, "GPMI_BCH_DIV", SC_R_NAND, SC_PM_CLK_PER },
};

static struct imx8_fixed_clks imx8qm_fixed_clks[] = {
	{ IMX8QM_IPG_CONN_CLK_ROOT, "IPG_CONN_CLK", SC_83MHZ },
	{ IMX8QM_AHB_CONN_CLK_ROOT, "AHB_CONN_CLK", SC_166MHZ },
	{ IMX8QM_AXI_CONN_CLK_ROOT, "AXI_CONN_CLK", SC_333MHZ },
	{ IMX8QM_IPG_DMA_CLK_ROOT, "IPG_DMA_CLK", SC_120MHZ },
	{ IMX8QM_IPG_MIPI_CSI_CLK_ROOT, "IPG_MIPI_CLK", SC_120MHZ },
	{ IMX8QM_LVDS_IPG_CLK, "IPG_LVDS_CLK", SC_24MHZ },
	{ IMX8QM_LSIO_BUS_CLK, "LSIO_BUS_CLK", SC_100MHZ },
	{ IMX8QM_LSIO_MEM_CLK, "LSIO_MEM_CLK", SC_200MHZ },
	{ IMX8QM_MIPI0_CLK_ROOT, "MIPI0_CLK", SC_120MHZ },
	{ IMX8QM_MIPI1_CLK_ROOT, "MIPI1_CLK", SC_120MHZ },
	{ IMX8QM_HDMI_RX_IPG_CLK, "HDMI_RX_IPG_CLK", SC_200MHZ },
	{ IMX8QM_HSIO_PER_CLK, "HSIO_CLK", SC_133MHZ },
	{ IMX8QM_HSIO_AXI_CLK, "HSIO_AXI", SC_400MHZ },
};

static struct imx8_gpr_clks imx8qm_gpr_clks[] = {
	{ IMX8QM_ENET0_REF_DIV, "ENET0_REF_DIV", SC_R_ENET_0, SC_C_CLKDIV, IMX8QM_ENET0_ROOT_DIV },
	{ IMX8QM_ENET0_REF_25MHZ_125MHZ_SEL, "ENET0_REF_25_125", SC_R_ENET_0, SC_C_SEL_125 },
	{ IMX8QM_ENET0_RMII_TX_SEL, "ENET0_RMII_TX", SC_R_ENET_0, SC_C_TXCLK },
	{ IMX8QM_ENET0_REF_25MHZ_125MHZ_CLK, "ENET0_REF_25_125_CLK", SC_R_ENET_0, SC_C_DISABLE_125 },
	{ IMX8QM_ENET0_REF_50MHZ_CLK, "ENET0_REF_50", SC_R_ENET_0, SC_C_DISABLE_50 },

	{ IMX8QM_ENET1_REF_DIV, "ENET1_REF_DIV", SC_R_ENET_1, SC_C_CLKDIV, IMX8QM_ENET1_ROOT_DIV },
	{ IMX8QM_ENET1_REF_25MHZ_125MHZ_SEL, "ENET1_REF_25_125", SC_R_ENET_1, SC_C_SEL_125 },
	{ IMX8QM_ENET1_RMII_TX_SEL, "ENET1_RMII_TX", SC_R_ENET_1, SC_C_TXCLK },
	{ IMX8QM_ENET1_REF_25MHZ_125MHZ_CLK, "ENET1_REF_25_125_CLK", SC_R_ENET_1, SC_C_DISABLE_125 },
	{ IMX8QM_ENET1_REF_50MHZ_CLK, "ENET1_REF_50", SC_R_ENET_1, SC_C_DISABLE_50 },
};

static struct imx8_lpcg_clks imx8qm_lpcg_clks[] = {
	{ IMX8QM_I2C0_CLK, "I2C0_CLK", 0, LPI2C_0_LPCG, IMX8QM_I2C0_DIV },
	{ IMX8QM_I2C0_IPG_CLK, "I2C0_IPG", 16, LPI2C_0_LPCG, IMX8QM_IPG_DMA_CLK_ROOT },
	{ IMX8QM_I2C1_CLK, "I2C1_CLK", 0, LPI2C_1_LPCG, IMX8QM_I2C1_DIV },
	{ IMX8QM_I2C1_IPG_CLK, "I2C1_IPG", 16, LPI2C_1_LPCG, IMX8QM_IPG_DMA_CLK_ROOT },
	{ IMX8QM_I2C2_CLK, "I2C2_CLK", 0, LPI2C_2_LPCG, IMX8QM_I2C2_DIV },
	{ IMX8QM_I2C2_IPG_CLK, "I2C2_IPG", 16, LPI2C_2_LPCG, IMX8QM_IPG_DMA_CLK_ROOT },
	{ IMX8QM_I2C3_CLK, "I2C3_CLK", 0, LPI2C_3_LPCG, IMX8QM_I2C3_DIV },
	{ IMX8QM_I2C3_IPG_CLK, "I2C3_IPG", 16, LPI2C_3_LPCG, IMX8QM_IPG_DMA_CLK_ROOT },

	{ IMX8QM_LVDS0_I2C0_CLK, "LVDS0_I2C0_CLK", 0, DI_LVDS_0_LPCG + 0x10, IMX8QM_LVDS0_I2C0_DIV },
	{ IMX8QM_LVDS0_I2C0_IPG_CLK, "LVDS0_I2C0_IPG", 16, DI_LVDS_0_LPCG + 0x10, IMX8QM_LVDS_IPG_CLK },
	{ IMX8QM_LVDS0_I2C1_CLK, "LVDS0_I2C1_CLK", 0, DI_LVDS_0_LPCG + 0x14, IMX8QM_LVDS0_I2C1_DIV },
	{ IMX8QM_LVDS0_I2C1_IPG_CLK, "LVDS0_I2C1_IPG", 16, DI_LVDS_0_LPCG + 0x14, IMX8QM_LVDS_IPG_CLK },
	{ IMX8QM_LVDS1_I2C0_CLK, "LVDS1_I2C0_CLK", 0, DI_LVDS_1_LPCG + 0x10, IMX8QM_LVDS1_I2C0_DIV },
	{ IMX8QM_LVDS1_I2C0_IPG_CLK, "LVDS1_I2C0_IPG", 16, DI_LVDS_1_LPCG + 0x10, IMX8QM_LVDS_IPG_CLK },
	{ IMX8QM_LVDS1_I2C1_CLK, "LVDS1_I2C1_CLK", 0, DI_LVDS_1_LPCG + 0x14, IMX8QM_LVDS1_I2C1_DIV },
	{ IMX8QM_LVDS1_I2C1_IPG_CLK, "LVDS1_I2C1_IPG", 16, DI_LVDS_1_LPCG + 0x14, IMX8QM_LVDS_IPG_CLK },

	{ IMX8QM_MIPI0_I2C0_CLK, "MIPI0_I2C0_CLK", 0, MIPI_DSI_0_LPCG + 0x1c, IMX8QM_MIPI0_I2C0_DIV },
	{ IMX8QM_MIPI0_I2C0_IPG_CLK, "MIPI0_I2C0_IPG", 0, MIPI_DSI_0_LPCG + 0x14,  IMX8QM_MIPI0_I2C0_IPG_S_CLK},
	{ IMX8QM_MIPI0_I2C0_IPG_S_CLK, "MIPI0_I2C0_IPG_S", 0, MIPI_DSI_0_LPCG + 0x18, IMX8QM_MIPI0_CLK_ROOT },
	{ IMX8QM_MIPI0_I2C1_CLK, "MIPI0_I2C1_CLK", 0, MIPI_DSI_0_LPCG + 0x2c, IMX8QM_MIPI0_I2C1_DIV },
	{ IMX8QM_MIPI0_I2C1_IPG_CLK, "MIPI0_I2C1_IPG", 0, MIPI_DSI_0_LPCG + 0x24,  IMX8QM_MIPI0_I2C1_IPG_S_CLK},
	{ IMX8QM_MIPI0_I2C1_IPG_S_CLK, "MIPI0_I2C1_IPG_S", 0, MIPI_DSI_0_LPCG + 0x28, IMX8QM_MIPI0_CLK_ROOT },
	{ IMX8QM_MIPI1_I2C0_CLK, "MIPI1_I2C0_CLK", 0, MIPI_DSI_1_LPCG + 0x1c, IMX8QM_MIPI1_I2C0_DIV },
	{ IMX8QM_MIPI1_I2C0_IPG_CLK, "MIPI1_I2C0_IPG", 0, MIPI_DSI_1_LPCG + 0x14, IMX8QM_MIPI1_I2C0_IPG_S_CLK},
	{ IMX8QM_MIPI1_I2C0_IPG_S_CLK, "MIPI1_I2C0_IPG_S", 0, MIPI_DSI_1_LPCG + 0x18, IMX8QM_MIPI1_CLK_ROOT },
	{ IMX8QM_MIPI1_I2C1_CLK, "MIPI1_I2C1_CLK", 0, MIPI_DSI_1_LPCG + 0x2c, IMX8QM_MIPI1_I2C1_DIV },
	{ IMX8QM_MIPI1_I2C1_IPG_CLK, "MIPI1_I2C1_IPG", 0, MIPI_DSI_1_LPCG + 0x24, IMX8QM_MIPI1_I2C1_IPG_S_CLK},
	{ IMX8QM_MIPI1_I2C1_IPG_S_CLK, "MIPI1_I2C1_IPG_S", 0, MIPI_DSI_1_LPCG + 0x28, IMX8QM_MIPI1_CLK_ROOT },

	{ IMX8QM_CSI0_I2C0_CLK, "CSI0_I2C0_CLK", 0, MIPI_CSI_0_LPCG + 0x14, IMX8QM_CSI0_I2C0_DIV },
	{ IMX8QM_CSI0_I2C0_IPG_CLK, "CSI0_I2C0_IPG", 16, MIPI_CSI_0_LPCG + 0x14, IMX8QM_IPG_MIPI_CSI_CLK_ROOT },
	{ IMX8QM_CSI1_I2C0_CLK, "CSI1_I2C0_CLK", 0, MIPI_CSI_1_LPCG + 0x14, IMX8QM_CSI1_I2C0_DIV },
	{ IMX8QM_CSI1_I2C0_IPG_CLK, "CSI1_I2C0_IPG", 16, MIPI_CSI_1_LPCG + 0x14, IMX8QM_IPG_MIPI_CSI_CLK_ROOT },
	{ IMX8QM_HDMI_I2C0_CLK, "HDMI_I2C0_CLK", 0, DI_HDMI_LPCG, IMX8QM_HDMI_I2C0_DIV },
	{ IMX8QM_HDMI_I2C_IPG_CLK, "HDMI_I2C0_IPG", 16, DI_HDMI_LPCG, IMX8QM_HDMI_IPG_CLK },
	{ IMX8QM_HDMI_RX_I2C_DIV_CLK, "HDMI RX_I2C_DIV_CLK", 0, MIPI_DSI_0_LPCG + 0x14, IMX8QM_MIPI0_I2C0_DIV },
	{ IMX8QM_HDMI_RX_I2C0_CLK, "HDMI RX_I2C_CLK", 0, MIPI_DSI_0_LPCG + 0x10, IMX8QM_HDMI_RX_I2C_DIV_CLK },
	{ IMX8QM_HDMI_RX_I2C_IPG_CLK, "HDMI_RX_I2C_IPG", 0, RX_HDMI_LPCG + 0x18,  IMX8QM_HDMI_RX_I2C_IPG_S_CLK},
	{ IMX8QM_HDMI_RX_I2C_IPG_S_CLK, "HDMI_I2C_IPG_S", 0, RX_HDMI_LPCG + 0x1c, IMX8QM_HDMI_RX_IPG_CLK },

	{ IMX8QM_UART0_CLK, "UART0_CLK", 0, LPUART_0_LPCG, IMX8QM_UART0_DIV },
	{ IMX8QM_UART0_IPG_CLK, "UART0_IPG", 16, LPUART_0_LPCG, IMX8QM_IPG_DMA_CLK_ROOT },
	{ IMX8QM_UART1_CLK, "UART1_CLK", 0, LPUART_1_LPCG, IMX8QM_UART1_DIV },
	{ IMX8QM_UART1_IPG_CLK, "UART1_IPG", 16, LPUART_1_LPCG, IMX8QM_IPG_DMA_CLK_ROOT },
	{ IMX8QM_UART2_CLK, "UART2_CLK", 0, LPUART_2_LPCG, IMX8QM_UART2_DIV },
	{ IMX8QM_UART2_IPG_CLK, "UART2_IPG", 16, LPUART_2_LPCG, IMX8QM_IPG_DMA_CLK_ROOT },
	{ IMX8QM_UART3_CLK, "UART3_CLK", 0, LPUART_3_LPCG, IMX8QM_UART3_DIV },
	{ IMX8QM_UART3_IPG_CLK, "UART3_IPG", 16, LPUART_3_LPCG, IMX8QM_IPG_DMA_CLK_ROOT },

	{ IMX8QM_SDHC0_CLK, "SDHC0_CLK", 0, USDHC_0_LPCG, IMX8QM_SDHC0_DIV },
	{ IMX8QM_SDHC0_IPG_CLK, "SDHC0_IPG", 16, USDHC_0_LPCG, IMX8QM_IPG_CONN_CLK_ROOT },
	{ IMX8QM_SDHC0_AHB_CLK, "SDHC0_AHB", 20, USDHC_0_LPCG, IMX8QM_AHB_CONN_CLK_ROOT },
	{ IMX8QM_SDHC1_CLK, "SDHC1_CLK", 0, USDHC_1_LPCG, IMX8QM_SDHC1_DIV },
	{ IMX8QM_SDHC1_IPG_CLK, "SDHC1_IPG", 16, USDHC_1_LPCG, IMX8QM_IPG_CONN_CLK_ROOT },
	{ IMX8QM_SDHC1_AHB_CLK, "SDHC1_AHB", 20, USDHC_1_LPCG, IMX8QM_AHB_CONN_CLK_ROOT },
	{ IMX8QM_SDHC2_CLK, "SDHC2_CLK", 0, USDHC_2_LPCG, IMX8QM_SDHC2_DIV },
	{ IMX8QM_SDHC2_IPG_CLK, "SDHC2_IPG", 16, USDHC_2_LPCG, IMX8QM_IPG_CONN_CLK_ROOT },
	{ IMX8QM_SDHC2_AHB_CLK, "SDHC2_AHB", 20, USDHC_2_LPCG, IMX8QM_AHB_CONN_CLK_ROOT },

	{ IMX8QM_ENET0_IPG_S_CLK, "ENET0_IPG_S", 20, ENET_0_LPCG, IMX8QM_IPG_CONN_CLK_ROOT },
	{ IMX8QM_ENET0_IPG_CLK, "ENET0_IPG", 16, ENET_0_LPCG, IMX8QM_ENET0_IPG_S_CLK },
	{ IMX8QM_ENET0_AHB_CLK, "ENET0_AHB", 8, ENET_0_LPCG, IMX8QM_AXI_CONN_CLK_ROOT },
	{ IMX8QM_ENET0_TX_CLK, "ENET0_TX", 4, ENET_0_LPCG, IMX8QM_ENET0_ROOT_DIV },
	{ IMX8QM_ENET0_PTP_CLK, "ENET0_PTP", 0, ENET_0_LPCG, IMX8QM_ENET0_ROOT_DIV  },
	{ IMX8QM_ENET0_RGMII_TX_CLK, "ENET0_RGMII_TX", 12, ENET_0_LPCG, IMX8QM_ENET0_RMII_TX_SEL  },
	{ IMX8QM_ENET0_RMII_RX_CLK, "ENET0_RMII_RX", 0, ENET_0_LPCG + 0x4, IMX8QM_ENET0_RGMII_DIV  },

	{ IMX8QM_ENET1_IPG_S_CLK, "ENET1_IPG_S", 20, ENET_1_LPCG, IMX8QM_IPG_CONN_CLK_ROOT },
	{ IMX8QM_ENET1_IPG_CLK, "ENET1_IPG", 16, ENET_1_LPCG, IMX8QM_ENET1_IPG_S_CLK },
	{ IMX8QM_ENET1_AHB_CLK, "ENET1_AHB", 8, ENET_1_LPCG, IMX8QM_AXI_CONN_CLK_ROOT },
	{ IMX8QM_ENET1_TX_CLK, "ENET1_TX", 4, ENET_1_LPCG, IMX8QM_ENET1_ROOT_DIV },
	{ IMX8QM_ENET1_PTP_CLK, "ENET1_PTP", 0, ENET_1_LPCG, IMX8QM_ENET1_ROOT_DIV  },
	{ IMX8QM_ENET1_RGMII_TX_CLK, "ENET1_RGMII_TX", 12, ENET_1_LPCG, IMX8QM_ENET1_RMII_TX_SEL  },
	{ IMX8QM_ENET1_RMII_RX_CLK, "ENET1_RMII_RX", 0, ENET_1_LPCG + 0x4, IMX8QM_ENET1_RGMII_DIV  },

	{ IMX8QM_FSPI0_IPG_S_CLK, "FSPI0_IPG_S", 0x18, FSPI_0_LPCG, IMX8QM_LSIO_BUS_CLK },
	{ IMX8QM_FSPI0_IPG_CLK, "FSPI0_IPG", 0x14, FSPI_0_LPCG, IMX8QM_FSPI0_IPG_S_CLK },
	{ IMX8QM_FSPI0_HCLK, "FSPI0_HCLK", 0x10, FSPI_0_LPCG, IMX8QM_LSIO_MEM_CLK },
	{ IMX8QM_FSPI0_CLK, "FSPI0_CLK", 0, FSPI_0_LPCG, IMX8QM_FSPI0_DIV },

	{ IMX8QM_USB2_OH_AHB_CLK, "USB2_OH_AHB", 24, USB_2_LPCG, IMX8QM_AHB_CONN_CLK_ROOT },
	{ IMX8QM_USB2_OH_IPG_S_CLK, "USB2_OH_IPG_S", 16, USB_2_LPCG, IMX8QM_IPG_CONN_CLK_ROOT },
	{ IMX8QM_USB2_OH_IPG_S_PL301_CLK, "USB2_OH_IPG_S_PL301", 20, USB_2_LPCG, IMX8QM_IPG_CONN_CLK_ROOT },
	{ IMX8QM_USB2_PHY_IPG_CLK, "USB2_PHY_IPG", 28, USB_2_LPCG, IMX8QM_IPG_CONN_CLK_ROOT },

	{ IMX8QM_USB3_IPG_CLK, "USB3_IPG", 16, USB_3_LPCG, IMX8QM_IPG_CONN_CLK_ROOT },
	{ IMX8QM_USB3_CORE_PCLK, "USB3_CORE", 20, USB_3_LPCG, IMX8QM_IPG_CONN_CLK_ROOT },
	{ IMX8QM_USB3_PHY_CLK, "USB3_PHY", 24, USB_3_LPCG, IMX8QM_USB3_IPG_CLK },
	{ IMX8QM_USB3_ACLK, "USB3_ACLK", 28, USB_3_LPCG, IMX8QM_USB3_ACLK_DIV },
	{ IMX8QM_USB3_BUS_CLK, "USB3_BUS", 0, USB_3_LPCG, IMX8QM_USB3_BUS_DIV },
	{ IMX8QM_USB3_LPM_CLK, "USB3_LPM", 4, USB_3_LPCG, IMX8QM_USB3_LPM_DIV },

	{ IMX8QM_GPMI_APB_CLK, "GPMI_APB", 16, NAND_LPCG, IMX8QM_AXI_CONN_CLK_ROOT },
	{ IMX8QM_GPMI_APB_BCH_CLK, "GPMI_APB_BCH", 20, NAND_LPCG, IMX8QM_AXI_CONN_CLK_ROOT },
	{ IMX8QM_GPMI_BCH_IO_CLK, "GPMI_IO_CLK", 4, NAND_LPCG, IMX8QM_GPMI_BCH_IO_DIV },
	{ IMX8QM_GPMI_BCH_CLK, "GPMI_BCH_CLK", 0, NAND_LPCG, IMX8QM_GPMI_BCH_DIV },
	{ IMX8QM_APBHDMA_CLK, "GPMI_CLK", 16, NAND_LPCG + 0x4, IMX8QM_AXI_CONN_CLK_ROOT },

	{ IMX8QM_HSIO_PCIE_A_MSTR_AXI_CLK, "HSIO_PCIE_A_MSTR_AXI_CLK", 16, HSIO_PCIE_X2_LPCG, IMX8QM_HSIO_AXI_CLK },
	{ IMX8QM_HSIO_PCIE_A_SLV_AXI_CLK, "HSIO_PCIE_A_SLV_AXI_CLK", 20, HSIO_PCIE_X2_LPCG, IMX8QM_HSIO_AXI_CLK },
	{ IMX8QM_HSIO_PCIE_A_DBI_AXI_CLK, "HSIO_PCIE_A_DBI_AXI_CLK", 24, HSIO_PCIE_X2_LPCG, IMX8QM_HSIO_AXI_CLK },
	{ IMX8QM_HSIO_PCIE_B_MSTR_AXI_CLK, "HSIO_PCIE_B_MSTR_AXI_CLK", 16, HSIO_PCIE_X1_LPCG, IMX8QM_HSIO_AXI_CLK },
	{ IMX8QM_HSIO_PCIE_B_SLV_AXI_CLK, "HSIO_PCIE_B_SLV_AXI_CLK", 20, HSIO_PCIE_X1_LPCG, IMX8QM_HSIO_AXI_CLK },
	{ IMX8QM_HSIO_PCIE_B_DBI_AXI_CLK, "HSIO_PCIE_B_DBI_AXI_CLK", 24, HSIO_PCIE_X1_LPCG, IMX8QM_HSIO_AXI_CLK },
	{ IMX8QM_HSIO_PCIE_X1_PER_CLK, "HSIO_PCIE_X1_PER_CLK", 16, HSIO_PCIE_X1_CRR3_LPCG, IMX8QM_HSIO_PER_CLK },
	{ IMX8QM_HSIO_PCIE_X2_PER_CLK, "HSIO_PCIE_X2_PER_CLK", 16, HSIO_PCIE_X2_CRR2_LPCG, IMX8QM_HSIO_PER_CLK },
	{ IMX8QM_HSIO_SATA_PER_CLK, "HSIO_SATA_PER_CLK", 16, HSIO_SATA_CRR4_LPCG, IMX8QM_HSIO_PER_CLK },
	{ IMX8QM_HSIO_PHY_X1_PER_CLK, "HSIO_PHY_X1_PER_CLK", 16, HSIO_PHY_X1_CRR1_LPCG, IMX8QM_HSIO_PER_CLK },
	{ IMX8QM_HSIO_PHY_X2_PER_CLK, "HSIO_PHY_X2_PER_CLK", 16, HSIO_PHY_X2_CRR0_LPCG, IMX8QM_HSIO_PER_CLK },
	{ IMX8QM_HSIO_MISC_PER_CLK, "HSIO_MISC_PER_CLK", 16, HSIO_MISC_LPCG, IMX8QM_HSIO_PER_CLK },
	{ IMX8QM_HSIO_PHY_X1_APB_CLK, "HSIO_PHY_X1_APB_CLK", 16, HSIO_PHY_X1_LPCG, IMX8QM_HSIO_PER_CLK },
	{ IMX8QM_HSIO_PHY_X2_APB_0_CLK, "HSIO_PHY_X2_APB_0_CLK", 16, HSIO_PHY_X2_LPCG, IMX8QM_HSIO_PER_CLK },
	{ IMX8QM_HSIO_PHY_X2_APB_1_CLK, "HSIO_PHY_X2_APB_1_CLK", 20, HSIO_PHY_X2_LPCG, IMX8QM_HSIO_PER_CLK },
	{ IMX8QM_HSIO_SATA_CLK, "HSIO_SATA_CLK", 16, HSIO_SATA_LPCG, IMX8QM_HSIO_AXI_CLK },
	{ IMX8QM_HSIO_GPIO_CLK, "HSIO_GPIO_CLK", 16, HSIO_GPIO_LPCG, IMX8QM_HSIO_PER_CLK },
	{ IMX8QM_HSIO_PHY_X1_PCLK, "HSIO_PHY_X1_PCLK", 0, HSIO_PHY_X1_LPCG, 0 },
	{ IMX8QM_HSIO_PHY_X2_PCLK_0, "HSIO_PHY_X2_PCLK_0", 0, HSIO_PHY_X2_LPCG, 0 },
	{ IMX8QM_HSIO_PHY_X2_PCLK_1, "HSIO_PHY_X2_PCLK_1", 4, HSIO_PHY_X2_LPCG, 0 },
	{ IMX8QM_HSIO_SATA_EPCS_RX_CLK, "HSIO_SATA_EPCS_RX_CLK", 8, HSIO_PHY_X1_LPCG, 0 },
	{ IMX8QM_HSIO_SATA_EPCS_TX_CLK, "HSIO_SATA_EPCS_TX_CLK", 4, HSIO_PHY_X1_LPCG, 0 },
};

struct imx8_mux_clks imx8qm_mux_clks[] = {
};
#endif

static ulong __imx8_clk_get_rate(struct udevice *dev, ulong id);
static int __imx8_clk_enable(struct udevice *dev, ulong id, bool enable);
static ulong __imx8_clk_set_rate(struct udevice *dev, ulong id, unsigned long rate);

static struct imx8_fixed_clks * check_imx8_fixed_clk(struct udevice *dev, ulong id)
{
	u32 i, size;
	ulong data = (ulong)dev_get_driver_data(dev);
	struct imx8_fixed_clks *clks;

	switch (data) {
#if defined(CONFIG_IMX8QXP) || defined(CONFIG_IMX8DXL)
	case FLAG_CLK_IMX8_IMX8QXP:
		size = ARRAY_SIZE(imx8qxp_fixed_clks);
		clks = imx8qxp_fixed_clks;
		break;
#endif
#ifdef CONFIG_IMX8QM
	case FLAG_CLK_IMX8_IMX8QM:
		size = ARRAY_SIZE(imx8qm_fixed_clks);
		clks = imx8qm_fixed_clks;
		break;
#endif
	default:
		printf("%s(Wrong driver data 0x%lx)\n",
		       __func__, (ulong)data);
		return NULL;
	}

	for (i = 0; i < size; i++) {
		if (id == clks[i].id)
			return &clks[i];
	}

	return NULL;
}

static struct imx8_lpcg_clks * check_imx8_lpcg_clk(struct udevice *dev, ulong id)
{
	u32 i, size;
	ulong data = (ulong)dev_get_driver_data(dev);
	struct imx8_lpcg_clks *clks;

	switch (data) {
#if defined(CONFIG_IMX8QXP) || defined(CONFIG_IMX8DXL)
	case FLAG_CLK_IMX8_IMX8QXP:
		size = ARRAY_SIZE(imx8qxp_lpcg_clks);
		clks = imx8qxp_lpcg_clks;
		break;
#endif
#ifdef CONFIG_IMX8QM
	case FLAG_CLK_IMX8_IMX8QM:
		size = ARRAY_SIZE(imx8qm_lpcg_clks);
		clks = imx8qm_lpcg_clks;
		break;
#endif
	default:
		printf("%s(Wrong driver data 0x%lx)\n",
		       __func__, (ulong)data);
		return NULL;
	}

	for (i = 0; i < size; i++) {
		if (id == clks[i].id)
			return &clks[i];
	}

	return NULL;
}

static struct imx8_clks * check_imx8_slice_clk(struct udevice *dev, ulong id)
{
	u32 i, size;
	ulong data = (ulong)dev_get_driver_data(dev);
	struct imx8_clks *clks;

	switch (data) {
#if defined(CONFIG_IMX8QXP) || defined(CONFIG_IMX8DXL)
	case FLAG_CLK_IMX8_IMX8QXP:
		size = ARRAY_SIZE(imx8qxp_clks);
		clks = imx8qxp_clks;
		break;
#endif
#ifdef CONFIG_IMX8QM
	case FLAG_CLK_IMX8_IMX8QM:
		size = ARRAY_SIZE(imx8qm_clks);
		clks = imx8qm_clks;
		break;
#endif
	default:
		printf("%s(Wrong driver data 0x%lx)\n",
		       __func__, (ulong)data);
		return NULL;
	}

	for (i = 0; i < size; i++) {
		if (id == clks[i].id)
			return &clks[i];
	}

	return NULL;
}

static struct imx8_gpr_clks * check_imx8_gpr_clk(struct udevice *dev, ulong id)
{
	u32 i, size;
	ulong data = (ulong)dev_get_driver_data(dev);
	struct imx8_gpr_clks *clks;

	switch (data) {
#if defined(CONFIG_IMX8QXP) || defined(CONFIG_IMX8DXL)
	case FLAG_CLK_IMX8_IMX8QXP:
		size = ARRAY_SIZE(imx8qxp_gpr_clks);
		clks = imx8qxp_gpr_clks;
		break;
#endif
#ifdef CONFIG_IMX8QM
	case FLAG_CLK_IMX8_IMX8QM:
		size = ARRAY_SIZE(imx8qm_gpr_clks);
		clks = imx8qm_gpr_clks;
		break;
#endif
	default:
		printf("%s(Wrong driver data 0x%lx)\n",
		       __func__, (ulong)data);
		return NULL;
	}

	for (i = 0; i < size; i++) {
		if (id == clks[i].id)
			return &clks[i];
	}

	return NULL;

}

static struct imx8_mux_clks * check_imx8_mux_clk(struct udevice *dev, ulong id)
{
	u32 i, size;
	ulong data = (ulong)dev_get_driver_data(dev);
	struct imx8_mux_clks *clks;

	switch (data) {
#if defined(CONFIG_IMX8QXP) || defined(CONFIG_IMX8DXL)
	case FLAG_CLK_IMX8_IMX8QXP:
		size = ARRAY_SIZE(imx8qxp_mux_clks);
		clks = imx8qxp_mux_clks;
		break;
#endif
#ifdef CONFIG_IMX8QM
	case FLAG_CLK_IMX8_IMX8QM:
		size = ARRAY_SIZE(imx8qm_mux_clks);
		clks = imx8qm_mux_clks;
		break;
#endif
	default:
		printf("%s(Wrong driver data 0x%lx)\n",
		       __func__, (ulong)data);
		return NULL;
	}

	for (i = 0; i < size; i++) {
		if (id == clks[i].id)
			return &clks[i];
	}

	return NULL;

}

static ulong imx8_get_rate_lpcg(struct udevice *dev, struct imx8_lpcg_clks *lpcg_clk)
{
	if (lpcg_clk->parent_id != 0) {
		if (lpcg_is_clock_on(lpcg_clk->lpcg, lpcg_clk->bit_idx >> 2)) {
			return __imx8_clk_get_rate(dev, lpcg_clk->parent_id);
		} else {
			return 0;
		}
	} else {
		return -ENOSYS;
	}
}

static ulong imx8_get_rate_slice(struct udevice *dev, struct imx8_clks *slice_clk)
{
	int ret;
	u32 rate;

	ret = sc_pm_get_clock_rate(-1, slice_clk->rsrc, slice_clk->pm_clk,
				   (sc_pm_clock_rate_t *)&rate);
	if (ret) {
		printf("%s err %d\n", __func__, ret);
		return ret;
	}

	return rate;
}

static ulong imx8_get_rate_fixed(struct udevice *dev, struct imx8_fixed_clks *fixed_clk)
{
	return fixed_clk->rate;
}

static ulong __imx8_clk_get_rate(struct udevice *dev, ulong id)
{
	void* clkdata;

	clkdata = check_imx8_lpcg_clk(dev, id);
	if (clkdata) {
		return imx8_get_rate_lpcg(dev, (struct imx8_lpcg_clks *)clkdata);
	}

	clkdata = check_imx8_slice_clk(dev, id);
	if (clkdata) {
		return imx8_get_rate_slice(dev, (struct imx8_clks *)clkdata);
	}

	clkdata = check_imx8_fixed_clk(dev, id);
	if (clkdata) {
		return imx8_get_rate_fixed(dev, (struct imx8_fixed_clks *)clkdata);
	}

	return -ENOSYS;
}

static ulong imx8_clk_get_rate(struct clk *clk)
{
	return __imx8_clk_get_rate(clk->dev, clk->id);
}

static ulong imx8_set_rate_lpcg(struct udevice *dev, struct imx8_lpcg_clks *lpcg_clk, unsigned long rate)
{
	if (lpcg_clk->parent_id != 0) {
		return __imx8_clk_set_rate(dev, lpcg_clk->parent_id, rate);
	} else {
		return -ENOSYS;
	}
}

static ulong imx8_set_rate_slice(struct udevice *dev, struct imx8_clks *slice_clk, unsigned long rate)
{
	int ret;
	u32 new_rate = rate;

	ret = sc_pm_set_clock_rate(-1, slice_clk->rsrc, slice_clk->pm_clk, &new_rate);
	if (ret) {
		printf("%s err %d\n", __func__, ret);
		return ret;
	}

	return new_rate;
}

static ulong imx8_set_rate_gpr(struct udevice *dev, struct imx8_gpr_clks *gpr_clk, unsigned long rate)
{
	ulong parent_rate;
	u32 val;
	int ret;

	if (gpr_clk->parent_id == 0)
		return -ENOSYS;

	parent_rate = __imx8_clk_get_rate(dev, gpr_clk->parent_id);
	if (parent_rate > 0) {
		val = (rate < parent_rate) ? 1 : 0;

		ret = sc_misc_set_control(-1, gpr_clk->rsrc,
			gpr_clk->gpr_id, val);
		if (ret) {
			printf("%s err %d\n", __func__, ret);
			return ret;
		}

		return rate;
	}

	return -ENOSYS;
}

static ulong __imx8_clk_set_rate(struct udevice *dev, ulong id, unsigned long rate)
{
	void* clkdata;

	clkdata = check_imx8_slice_clk(dev, id);
	if (clkdata) {
		return imx8_set_rate_slice(dev, (struct imx8_clks *)clkdata, rate);
	}

	clkdata = check_imx8_lpcg_clk(dev, id);
	if (clkdata) {
		return imx8_set_rate_lpcg(dev, (struct imx8_lpcg_clks *)clkdata, rate);
	}

	clkdata = check_imx8_gpr_clk(dev, id);
	if (clkdata) {
		return imx8_set_rate_gpr(dev, (struct imx8_gpr_clks *)clkdata, rate);
	}

	return -ENOSYS;
}

static ulong imx8_clk_set_rate(struct clk *clk, unsigned long rate)
{
	return __imx8_clk_set_rate(clk->dev, clk->id, rate);

}

static int imx8_enable_slice(struct udevice *dev, struct imx8_clks *slice_clk, bool enable)
{
	int ret;

	ret = sc_pm_clock_enable(-1, slice_clk->rsrc, slice_clk->pm_clk, enable, 0);
	if (ret) {
		printf("%s err %d\n", __func__, ret);
		return ret;
	}

	return 0;
}

static int imx8_enable_lpcg(struct udevice *dev, struct imx8_lpcg_clks *lpcg_clk, bool enable)
{
	if (enable) {
		if (lpcg_clk->parent_id != 0) {
			__imx8_clk_enable(dev, lpcg_clk->parent_id, enable);
		}

		lpcg_clock_on(lpcg_clk->lpcg, lpcg_clk->bit_idx >> 2);
	} else {
		lpcg_clock_off(lpcg_clk->lpcg, lpcg_clk->bit_idx >> 2);

		if (lpcg_clk->parent_id != 0) {
			__imx8_clk_enable(dev, lpcg_clk->parent_id, enable);
		}
	}

	return 0;
}

static int __imx8_clk_enable(struct udevice *dev, ulong id, bool enable)
{
	void* clkdata;

	clkdata = check_imx8_lpcg_clk(dev, id);
	if (clkdata) {
		return imx8_enable_lpcg(dev, (struct imx8_lpcg_clks *)clkdata, enable);
	}

	clkdata = check_imx8_slice_clk(dev, id);
	if (clkdata) {
		return imx8_enable_slice(dev, (struct imx8_clks *)clkdata, enable);
	}

	return -ENOSYS;
}

static int imx8_clk_disable(struct clk *clk)
{
	return __imx8_clk_enable(clk->dev, clk->id, 0);
}

static int imx8_clk_enable(struct clk *clk)
{
	return __imx8_clk_enable(clk->dev, clk->id, 1);
}

static int imx8_set_parent_mux(struct udevice *dev, struct imx8_mux_clks *mux_clk, ulong pid)
{
	u32 i;
	int ret;
	struct imx8_clks *slice_clkdata;

	slice_clkdata = check_imx8_slice_clk(dev, mux_clk->slice_clk_id);
	if (!slice_clkdata) {
		printf("Error: fail to find slice clk %lu for this mux %lu\n", mux_clk->slice_clk_id, mux_clk->id);
		return -EINVAL;
	}

	for (i = 0; i< CLK_IMX8_MAX_MUX_SEL; i++) {
		if (pid == mux_clk->parent_clks[i]) {
			ret = sc_pm_set_clock_parent(-1, slice_clkdata->rsrc,  slice_clkdata->pm_clk, i);
			if (ret)
				printf("Error: fail to set clock parent rsrc %d, pm_clk %d, parent clk %d\n",
					slice_clkdata->rsrc,  slice_clkdata->pm_clk, i);
			return ret;
		}
	}

	return -ENOSYS;
}

static int imx8_clk_set_parent(struct clk *clk, struct clk *parent)
{
	void* clkdata;

	clkdata = check_imx8_mux_clk(clk->dev, clk->id);
	if (clkdata) {
		return imx8_set_parent_mux(clk->dev, (struct imx8_mux_clks *)clkdata, parent->id);
	}

	return -ENOSYS;
}

#if CONFIG_IS_ENABLED(CMD_CLK)
int soc_clk_dump(void)
{
	struct udevice *dev;
	struct clk clk;
	unsigned long rate;
	int i, ret;
	u32 size;
	struct imx8_clks *clks;
	ulong data;

	ret = uclass_get_device_by_driver(UCLASS_CLK,
					  DM_GET_DRIVER(imx8_clk), &dev);
	if (ret)
		return ret;

	printf("Clk\t\tHz\n");

	data = (ulong)dev_get_driver_data(dev);

	switch (data) {
#if defined(CONFIG_IMX8QXP) || defined(CONFIG_IMX8DXL)
	case FLAG_CLK_IMX8_IMX8QXP:
		size = ARRAY_SIZE(imx8qxp_clks);
		clks = imx8qxp_clks;
		break;
#endif
#ifdef CONFIG_IMX8QM
	case FLAG_CLK_IMX8_IMX8QM:
		size = ARRAY_SIZE(imx8qm_clks);
		clks = imx8qm_clks;
		break;
#endif
	default:
		printf("%s(Wrong driver data 0x%lx)\n",
		       __func__, (ulong)data);
		return -EINVAL;
	}

	for (i = 0; i < size; i++) {
		clk.id = clks[i].id;
		ret = clk_request(dev, &clk);
		if (ret < 0) {
			debug("%s clk_request() failed: %d\n", __func__, ret);
			continue;
		}

		ret = clk_get_rate(&clk);
		rate = ret;

		clk_free(&clk);

		if (ret == -ENOTSUPP) {
			printf("clk ID %lu not supported yet\n",
			       clks[i].id);
			continue;
		}
		if (ret < 0) {
			printf("%s %lu: get_rate err: %d\n",
			       __func__, clks[i].id, ret);
			continue;
		}

		printf("%s(%3lu):\t%lu\n",
		       clks[i].name, clks[i].id, rate);
	}

	return 0;
}

#endif

static struct clk_ops imx8_clk_ops = {
	.set_rate = imx8_clk_set_rate,
	.get_rate = imx8_clk_get_rate,
	.enable = imx8_clk_enable,
	.disable = imx8_clk_disable,
	.set_parent = imx8_clk_set_parent,
};

static int imx8_clk_probe(struct udevice *dev)
{
	return 0;
}

static const struct udevice_id imx8_clk_ids[] = {
#if defined(CONFIG_IMX8QXP) || defined(CONFIG_IMX8DXL)
	{ .compatible = "fsl,imx8qxp-clk", .data = FLAG_CLK_IMX8_IMX8QXP, },
#endif
#ifdef CONFIG_IMX8QM
	{ .compatible = "fsl,imx8qm-clk", .data = FLAG_CLK_IMX8_IMX8QM, },
#endif
	{ },
};

U_BOOT_DRIVER(imx8_clk) = {
	.name = "clk_imx8",
	.id = UCLASS_CLK,
	.of_match = imx8_clk_ids,
	.ops = &imx8_clk_ops,
	.probe = imx8_clk_probe,
	.flags = DM_FLAG_PRE_RELOC,
};
