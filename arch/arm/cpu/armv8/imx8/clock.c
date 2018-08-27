/*
 * Copyright 2017-2018 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <linux/errno.h>
#include <asm/arch/clock.h>
#include <asm/imx-common/sci/sci.h>
#include <asm/arch/imx8-pins.h>
#include <asm/arch/i2c.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/cpu.h>

DECLARE_GLOBAL_DATA_PTR;

u32 get_lpuart_clk(void)
{
	return mxc_get_clock(MXC_UART_CLK);
}

static u32 get_arm_main_clk(void)
{
	sc_err_t err;
	sc_pm_clock_rate_t clkrate;

	if (is_cortex_a53())
		err = sc_pm_get_clock_rate((sc_ipc_t)gd->arch.ipc_channel_handle,
				SC_R_A53, SC_PM_CLK_CPU, &clkrate);
	else if (is_cortex_a72())
		err = sc_pm_get_clock_rate((sc_ipc_t)gd->arch.ipc_channel_handle,
				SC_R_A72, SC_PM_CLK_CPU, &clkrate);
	else if (is_cortex_a35())
		err = sc_pm_get_clock_rate((sc_ipc_t)gd->arch.ipc_channel_handle,
				SC_R_A35, SC_PM_CLK_CPU, &clkrate);
	else
		err = SC_ERR_UNAVAILABLE;

	if (err != SC_ERR_NONE) {
		printf("sc get ARM clk failed! err=%d\n", err);
		return 0;
	}
	return clkrate;
}

unsigned int mxc_get_clock(enum mxc_clock clk)
{
	sc_err_t err;
	sc_pm_clock_rate_t clkrate;

	switch (clk) {
	case MXC_UART_CLK:
		err = sc_pm_get_clock_rate((sc_ipc_t)gd->arch.ipc_channel_handle,
				SC_R_UART_0, 2, &clkrate);
		if (err != SC_ERR_NONE) {
			printf("sc get UART clk failed! err=%d\n", err);
			return 0;
		}
		return clkrate;
	case MXC_ESDHC_CLK:
		err = sc_pm_get_clock_rate((sc_ipc_t)gd->arch.ipc_channel_handle,
				SC_R_SDHC_0, 2, &clkrate);
		if (err != SC_ERR_NONE) {
			printf("sc get uSDHC1 clk failed! err=%d\n", err);
			return 0;
		}
		return clkrate;
	case MXC_ESDHC2_CLK:
		err = sc_pm_get_clock_rate((sc_ipc_t)gd->arch.ipc_channel_handle,
				SC_R_SDHC_1, 2, &clkrate);
		if (err != SC_ERR_NONE) {
			printf("sc get uSDHC2 clk failed! err=%d\n", err);
			return 0;
		}
		return clkrate;
	case MXC_ESDHC3_CLK:
		err = sc_pm_get_clock_rate((sc_ipc_t)gd->arch.ipc_channel_handle,
				SC_R_SDHC_2, 2, &clkrate);
		if (err != SC_ERR_NONE) {
			printf("sc get uSDHC3 clk failed! err=%d\n", err);
			return 0;
		}
		return clkrate;
	case MXC_FEC_CLK:
		err = sc_pm_get_clock_rate((sc_ipc_t)gd->arch.ipc_channel_handle,
				SC_R_ENET_0, 2, &clkrate);
		if (err != SC_ERR_NONE) {
			printf("sc get ENET clk failed! err=%d\n", err);
			return 0;
		}
		return clkrate;
	case MXC_FSPI_CLK:
		err = sc_pm_get_clock_rate((sc_ipc_t)gd->arch.ipc_channel_handle,
				SC_R_FSPI_0, 2, &clkrate);
		if (err != SC_ERR_NONE) {
			printf("sc get FSPI clk failed! err=%d\n", err);
			return 0;
		}
		return clkrate;
	case MXC_DDR_CLK:
		err = sc_pm_get_clock_rate((sc_ipc_t)gd->arch.ipc_channel_handle,
				SC_R_DRC_0, 0, &clkrate);
		if (err != SC_ERR_NONE) {
			printf("sc get DRC0 clk failed! err=%d\n", err);
			return 0;
		}
		return clkrate;
	case MXC_ARM_CLK:
		return get_arm_main_clk();
	default:
		printf("Unsupported mxc_clock %d\n", clk);
		break;
	}

	return 0;
}

u32 imx_get_fecclk(void)
{
	return mxc_get_clock(MXC_FEC_CLK);
}

int enable_i2c_clk(unsigned char enable, unsigned i2c_num)
{
	sc_ipc_t ipc;
	sc_err_t err;

	if (i2c_num >= ARRAY_SIZE(imx_i2c_desc))
		return -EINVAL;

	ipc = gd->arch.ipc_channel_handle;

	if (enable)
		err = sc_pm_clock_enable(ipc,
			imx_i2c_desc[i2c_num].rsrc, 2, true, false);
	else
		err = sc_pm_clock_enable(ipc,
			imx_i2c_desc[i2c_num].rsrc, 2, false, false);

	if (err != SC_ERR_NONE) {
		printf("i2c clock error %d\n", err);
		return -EPERM;
	}

	return 0;
}

u32 imx_get_i2cclk(unsigned i2c_num)
{
	sc_err_t err;
	sc_ipc_t ipc;
	u32 clock_rate;

	if (i2c_num >= ARRAY_SIZE(imx_i2c_desc))
		return 0;

	ipc = gd->arch.ipc_channel_handle;
	err = sc_pm_get_clock_rate(ipc, imx_i2c_desc[i2c_num].rsrc, 2,
		&clock_rate);
	if (err != SC_ERR_NONE)
		return 0;

	return clock_rate;
}

void init_clk_fspi(int index)
{
	sc_err_t sciErr = 0;
	sc_pm_clock_rate_t rate;
	sc_ipc_t ipcHndl = gd->arch.ipc_channel_handle;

	/* Set FSPI0 clock root to 29 MHz */
	rate = 29000000;
	sciErr = sc_pm_set_clock_rate(ipcHndl, SC_R_FSPI_0, SC_PM_CLK_PER, &rate);
	if (sciErr != SC_ERR_NONE) {
		puts("FSPI0 setrate failed\n");
		return;
	}

	/* Enable FSPI0 clock root */
	sciErr = sc_pm_clock_enable(ipcHndl, SC_R_FSPI_0, SC_PM_CLK_PER, true, false);
	if (sciErr != SC_ERR_NONE) {
		puts("FSPI0 enable clock failed\n");
		return;
	}

	return;
}

void init_clk_gpmi_nand(void)
{
	sc_err_t sciErr = 0;
	sc_pm_clock_rate_t rate;
	sc_ipc_t ipcHndl = gd->arch.ipc_channel_handle;

	/* Set NAND BCH clock root to 50 MHz */
	rate = 50000000;
	sciErr = sc_pm_set_clock_rate(ipcHndl, SC_R_NAND, SC_PM_CLK_PER, &rate);
	if (sciErr != SC_ERR_NONE) {
		puts("NAND BCH set rate failed\n");
		return;
	}

	/* Enable NAND BCH clock root */
	sciErr = sc_pm_clock_enable(ipcHndl, SC_R_NAND, SC_PM_CLK_PER, true, false);
	if (sciErr != SC_ERR_NONE) {
		puts("NAND BCH enable clock failed\n");
		return;
	}

	/* Set NAND GPMI clock root to 50 MHz */
	rate = 50000000;
	sciErr = sc_pm_set_clock_rate(ipcHndl, SC_R_NAND, SC_PM_CLK_MST_BUS, &rate);
	if (sciErr != SC_ERR_NONE) {
		puts("NAND GPMI set rate failed\n");
		return;
	 }

	/* Enable NAND GPMI clock root */
	sciErr = sc_pm_clock_enable(ipcHndl, SC_R_NAND, SC_PM_CLK_MST_BUS, true, false);
	if (sciErr != SC_ERR_NONE) {
		puts("NAND GPMI enable clock failed\n");
		return;
	}

	return;
}

void enable_usboh3_clk(unsigned char enable)
{
	return;
}

void init_clk_usb3(int index)
{
	sc_err_t err;
	sc_ipc_t ipc;

	ipc = gd->arch.ipc_channel_handle;

	err = sc_pm_clock_enable(ipc, SC_R_USB_2, SC_PM_CLK_MISC, true, false);
	if (err != SC_ERR_NONE)
		printf("USB3 set clock failed!, line=%d (error = %d)\n",
			__LINE__, err);

	err = sc_pm_clock_enable(ipc, SC_R_USB_2, SC_PM_CLK_MST_BUS, true, false);
	if (err != SC_ERR_NONE)
		printf("USB3 set clock failed!, line=%d (error = %d)\n",
			__LINE__, err);

	err = sc_pm_clock_enable(ipc, SC_R_USB_2, SC_PM_CLK_PER, true, false);
	if (err != SC_ERR_NONE)
		printf("USB3 set clock failed!, line=%d (error = %d)\n",
			__LINE__, err);

	return;
}

int cdns3_enable_clks(int index)
{
	init_clk_usb3(index);
	return 0;
}

int cdns3_disable_clks(int index)
{
	sc_err_t err;
	sc_ipc_t ipc;

	ipc = gd->arch.ipc_channel_handle;

	err = sc_pm_clock_enable(ipc, SC_R_USB_2, SC_PM_CLK_MISC, false, false);
	if (err != SC_ERR_NONE)
		printf("USB3 disable clock failed!, line=%d (error = %d)\n",
			__LINE__, err);

	err = sc_pm_clock_enable(ipc, SC_R_USB_2, SC_PM_CLK_MST_BUS, false, false);
	if (err != SC_ERR_NONE)
		printf("USB3 disable clock failed!, line=%d (error = %d)\n",
			__LINE__, err);

	err = sc_pm_clock_enable(ipc, SC_R_USB_2, SC_PM_CLK_PER, false, false);
	if (err != SC_ERR_NONE)
		printf("USB3 disable clock failed!, line=%d (error = %d)\n",
			__LINE__, err);

	return 0;
}

void init_clk_usdhc(u32 index)
{
#ifdef CONFIG_IMX8QM
	sc_rsrc_t usdhcs[] = {SC_R_SDHC_0, SC_R_SDHC_1, SC_R_SDHC_2};
	u32 instances = 3;
#else
	sc_rsrc_t usdhcs[] = {SC_R_SDHC_0, SC_R_SDHC_1};
	u32 instances = 2;
#endif

	sc_err_t err;
	sc_ipc_t ipc;
	sc_pm_clock_rate_t actual = 400000000;

	ipc = gd->arch.ipc_channel_handle;

	if (index >= instances)
		return;

	/*
	 * IMX8QXP USDHC_CLK_ROOT default source from DPLL, but this DPLL
	 * do not stable, will cause usdhc data transfer crc error. So here
	 * is a workaround, let USDHC_CLK_ROOT source from AVPLL. Due to
	 * AVPLL is fixed to 1000MHz, so here config USDHC1_CLK_ROOT to 333MHz,
	 * USDHC2_CLK_ROOT to 200MHz, make eMMC HS400ES work at 166MHz, and SD
	 * SDR104 work at 200MHz.
	 */
	if (is_imx8qxp() && is_soc_rev(CHIP_REV_A)) {
		err = sc_pm_set_clock_parent(ipc, usdhcs[index], 2, SC_PM_PARENT_PLL1);
		if (err != SC_ERR_NONE)
			printf("SDHC_%d set clock parent failed!(error = %d)\n", index, err);

		if (index == 1)
			actual = 200000000;
	}

	err = sc_pm_set_clock_rate(ipc, usdhcs[index], 2, &actual);
	if (err != SC_ERR_NONE) {
		printf("SDHC_%d set clock failed! (error = %d)\n", index, err);
		return;
	}

	if (actual != 400000000)
		debug("Actual rate for SDHC_%d is %d\n", index, actual);

	err = sc_pm_clock_enable(ipc, usdhcs[index], SC_PM_CLK_PER, true, false);
	if (err != SC_ERR_NONE) {
		printf("SDHC_%d per clk enable failed!\n", index);
		return;
	}
}

void init_clk_fec(int index)
{
	sc_err_t err;
	sc_ipc_t ipc;
	sc_pm_clock_rate_t rate = 24000000;
	sc_rsrc_t enet[2] = {SC_R_ENET_0, SC_R_ENET_1};

	if (index > 1)
		return;

	if (index == -1)
		index = 0;

	ipc = gd->arch.ipc_channel_handle;

	/* Disable SC_R_ENET_0 clock root */
	err = sc_pm_clock_enable(ipc, enet[index], 0, false, false);
	err |= sc_pm_clock_enable(ipc, enet[index], 2, false, false);
	err |= sc_pm_clock_enable(ipc, enet[index], 4, false, false);
	if (err != SC_ERR_NONE) {
		printf("\nSC_R_ENET_0 set clock disable failed! (error = %d)\n", err);
		return;
	}

	/* Set SC_R_ENET_0 clock root to 125 MHz */
	rate = 125000000;

	/* div = 8 clk_source = PLL_1 ss_slice #7 in verfication codes */
	err = sc_pm_set_clock_rate(ipc, enet[index], 2, &rate);
	if (err != SC_ERR_NONE) {
		printf("\nSC_R_ENET_0 set clock ref clock 125M failed! (error = %d)\n", err);
		return;
	}

	/* Enable SC_R_ENET_0 clock root */
	err = sc_pm_clock_enable(ipc, enet[index], 0, true, true);
	err |= sc_pm_clock_enable(ipc, enet[index], 2, true, true);
	err |= sc_pm_clock_enable(ipc, enet[index], 4, true, true);
	if (err != SC_ERR_NONE) {
		printf("\nSC_R_ENET_0 set clock enable failed! (error = %d)\n", err);
		return;
	}
}

/*
 * Dump some core clockes.
 */
int do_mx8_showclocks(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	printf("ARM        %8d kHz\n", mxc_get_clock(MXC_ARM_CLK) / 1000);
	printf("DRC        %8d kHz\n", mxc_get_clock(MXC_DDR_CLK) / 1000);
	printf("USDHC1     %8d kHz\n", mxc_get_clock(MXC_ESDHC_CLK) / 1000);
	printf("USDHC2     %8d kHz\n", mxc_get_clock(MXC_ESDHC2_CLK) / 1000);
	printf("USDHC3     %8d kHz\n", mxc_get_clock(MXC_ESDHC3_CLK) / 1000);
	printf("UART0     %8d kHz\n", mxc_get_clock(MXC_UART_CLK) / 1000);
	printf("FEC0     %8d kHz\n", mxc_get_clock(MXC_FEC_CLK) / 1000);
	printf("FLEXSPI0     %8d kHz\n", mxc_get_clock(MXC_FSPI_CLK) / 1000);

	return 0;
}

U_BOOT_CMD(
	clocks,	CONFIG_SYS_MAXARGS, 1, do_mx8_showclocks,
	"display clocks",
	""
);

