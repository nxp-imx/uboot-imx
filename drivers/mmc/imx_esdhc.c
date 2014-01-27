/*
 * Copyright 2007, Freescale Semiconductor, Inc
 * Andy Fleming
 *
 * Copyright (C) 2008-2014 Freescale Semiconductor, Inc.
 *
 * Based vaguely on the pxa mmc code:
 * (C) Copyright 2003
 * Kyle Harris, Nexus Technologies, Inc. kharris@nexus-tech.net
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <config.h>
#include <common.h>
#include <command.h>
#include <hwconfig.h>
#include <mmc.h>
#include <part.h>
#include <malloc.h>
#include <mmc.h>
#include <fsl_esdhc.h>
#include <fdt_support.h>
#include <asm/io.h>


DECLARE_GLOBAL_DATA_PTR;

#define SDHCI_IRQ_EN_BITS		(IRQSTATEN_CC | IRQSTATEN_TC | \
				IRQSTATEN_BWR | IRQSTATEN_BRR | IRQSTATEN_CINT | \
				IRQSTATEN_CTOE | IRQSTATEN_CCE | IRQSTATEN_CEBE | \
				IRQSTATEN_CIE | IRQSTATEN_DTOE | IRQSTATEN_DCE | IRQSTATEN_DEBE)

struct fsl_esdhc {
	uint	dsaddr;
	uint	blkattr;
	uint	cmdarg;
	uint	xfertyp;
	uint	cmdrsp0;
	uint	cmdrsp1;
	uint	cmdrsp2;
	uint	cmdrsp3;
	uint	datport;
	uint	prsstat;
	uint	proctl;
	uint	sysctl;
	uint	irqstat;
	uint	irqstaten;
	uint	irqsigen;
	uint	autoc12err;
	uint	hostcapblt;
	uint	wml;
	uint	mixctrl;
	char	reserved1[4];
	uint	fevt;
	char	reserved2[12];
	uint dllctrl;
	uint dllstatus;
	uint clktunectrlstatus;
	char	reserved3[84];
	uint vendorspec;
	uint	mmcboot;
	char	reserved4[52];
	uint	hostver;
};

/* Return the XFERTYP flags for a given command and data packet */
uint esdhc_xfertyp(struct mmc_cmd *cmd, struct mmc_data *data)
{
	uint xfertyp = 0;

	if (data) {
		xfertyp |= XFERTYP_DPSEL;

		if (data->blocks > 1) {
			xfertyp |= XFERTYP_MSBSEL;
			xfertyp |= XFERTYP_BCEN;
		}

		if (data->flags & MMC_DATA_READ)
			xfertyp |= XFERTYP_DTDSEL;
	}

	if (cmd->resp_type & MMC_RSP_CRC)
		xfertyp |= XFERTYP_CCCEN;
	if (cmd->resp_type & MMC_RSP_OPCODE)
		xfertyp |= XFERTYP_CICEN;
	if (cmd->resp_type & MMC_RSP_136)
		xfertyp |= XFERTYP_RSPTYP_136;
	else if (cmd->resp_type & MMC_RSP_BUSY)
		xfertyp |= XFERTYP_RSPTYP_48_BUSY;
	else if (cmd->resp_type & MMC_RSP_PRESENT)
		xfertyp |= XFERTYP_RSPTYP_48;

	if (cmd->cmdidx == MMC_CMD_STOP_TRANSMISSION)
		xfertyp |= XFERTYP_CMDTYP_ABORT;

	return XFERTYP_CMD(cmd->cmdidx) | xfertyp;
}

static int esdhc_setup_data(struct mmc *mmc, struct mmc_data *data)
{
	uint wml_value;
	int timeout;
	u32 tmp;
	struct fsl_esdhc_cfg *cfg = (struct fsl_esdhc_cfg *)mmc->priv;
	struct fsl_esdhc *regs = (struct fsl_esdhc *)cfg->esdhc_base;

	wml_value = data->blocksize / 4;

	if (wml_value > 0x80)
		wml_value = 0x80;

	if (!(data->flags & MMC_DATA_READ)) {
		if ((readl(&regs->prsstat) & PRSSTAT_WPSPL) == 0) {
			printf("\nThe SD card is locked. Can not write to a locked card.\n\n");
			return TIMEOUT;
		}
		wml_value = wml_value << 16;
	}

	writel(wml_value, &regs->wml);

	writel(data->blocks << 16 | data->blocksize, &regs->blkattr);

	/* Calculate the timeout period for data transactions */
	/*
	timeout = fls(mmc->tran_speed / 10) - 1;
	timeout -= 13;

	if (timeout > 14)
		timeout = 14;

	if (timeout < 0)
		timeout = 0;
	*/
	timeout = 14;

	tmp = (readl(&regs->sysctl) & (~SYSCTL_TIMEOUT_MASK)) | (timeout << 16);
	writel(tmp, &regs->sysctl);

	return 0;
}


/*
 * Sends a command out on the bus.  Takes the mmc pointer,
 * a command pointer, and an optional data pointer.
 */
static int
esdhc_send_cmd(struct mmc *mmc, struct mmc_cmd *cmd, struct mmc_data *data)
{
	uint	xfertyp, mixctrl;
	uint	irqstat;
	u32	tmp, sysctl_restore = 0;
	struct fsl_esdhc_cfg *cfg = (struct fsl_esdhc_cfg *)mmc->priv;
	volatile struct fsl_esdhc *regs = (struct fsl_esdhc *)cfg->esdhc_base;

	writel(-1, &regs->irqstat);

	sync();

	tmp = readl(&regs->irqstaten) | SDHCI_IRQ_EN_BITS;
	writel(tmp, &regs->irqstaten);

	/* Wait for the bus to be idle */
	while ((readl(&regs->prsstat) & PRSSTAT_CICHB) ||
			(readl(&regs->prsstat) & PRSSTAT_CIDHB))
			;

	while (readl(&regs->prsstat) & PRSSTAT_DLA);

	/* Set up for a data transfer if we have one */
	if (data) {
		int err;

		err = esdhc_setup_data(mmc, data);
		if(err)
			return err;
	}

	/* Figure out the transfer arguments */
	xfertyp = esdhc_xfertyp(cmd, data);

	if (mmc->card_caps & EMMC_MODE_4BIT_DDR ||
		mmc->card_caps & EMMC_MODE_8BIT_DDR)
		xfertyp |= XFERTYP_DDR_EN;

	/* ESDHC errata ENGcm03648: Turn off auto-clock gate for commands
	 * with busy signaling and no data
	 */
	if (!cfg->is_usdhc && !data && (cmd->resp_type & MMC_RSP_BUSY)) {
		sysctl_restore = readl(&regs->sysctl);
		writel(sysctl_restore | 0xF, &regs->sysctl);
	}

	/* Send the command */
	writel(cmd->cmdarg, &regs->cmdarg);

	/* write lower-half of xfertyp to mixctrl */
	mixctrl = xfertyp & 0xFFFF;
	/* Keep the bits 22-25 of the register as is */
	mixctrl |= (readl(&regs->mixctrl) & (0xF << 22));
	writel(mixctrl, &regs->mixctrl);

	writel(xfertyp, &regs->xfertyp);

	/* Mask all irqs */
	writel(0, &regs->irqsigen);

	/* Wait for the command to complete */
	while (!(readl(&regs->irqstat) & (IRQSTAT_CC | IRQSTAT_CTOE)))
		;

	irqstat = readl(&regs->irqstat);
	writel(irqstat, &regs->irqstat);

	/* Reset CMD and DATA portions on error */
	if (irqstat & (CMD_ERR | IRQSTAT_CTOE)) {

		if (SD_CMD_TUNING == cmd->cmdidx) {
			/*add delay to output tunning block from card*/
			udelay(50);
		}

		writel(readl(&regs->sysctl) | SYSCTL_RSTC, &regs->sysctl);
		while (readl(&regs->sysctl) & SYSCTL_RSTC)
			;

		if (data) {
			writel(readl(&regs->sysctl) | SYSCTL_RSTD,
				&regs->sysctl);
			while (readl(&regs->sysctl) & SYSCTL_RSTD)
				;
		}

		/* Restore auto-clock gate if error */
		if (!cfg->is_usdhc && !data && (cmd->resp_type & MMC_RSP_BUSY))
			writel(sysctl_restore, &regs->sysctl);

		/* If this was CMD11, then notify that power cycle is needed */
		if (cmd->cmdidx == SD_CMD_SWITCH_UHS18V)
			printf("CMD11 to switch to 1.8V mode failed."
				"Card requires power cycle\n");

		/* Clear the tune execute bit*/
		mixctrl = readl(&regs->mixctrl);
		if (mixctrl & USDHC_MIXCTRL_EXE_TUNE) {
			mixctrl &= ~USDHC_MIXCTRL_EXE_TUNE;
			writel(mixctrl, &regs->mixctrl);
		}

	}

	if (irqstat & CMD_ERR)
		return COMM_ERR;

	if (irqstat & IRQSTAT_CTOE)
		return TIMEOUT;

	/* Switch voltage to 1.8V if CMD11 succeeded */
	if (cmd->cmdidx == SD_CMD_SWITCH_UHS18V) {
		/* Set SD_VSELECT to switch to 1.8V */
		u32 reg;
		reg = readl(&regs->vendorspec);
		reg |= VENDORSPEC_VSELECT;
		writel(reg, &regs->vendorspec);

		/* Sleep for 5 ms - max time for card to switch to 1.8V */
		udelay(5000);

		/* Turn on SD clock */
		writel(reg | VENDORSPEC_FRC_SDCLK_ON, &regs->vendorspec);

		while (!(readl(&regs->prsstat) & PRSSTAT_DAT0))
			;

		/* restore SD clock status */
		writel(reg, &regs->vendorspec);
	}

	/* Workaround for ESDHC errata ENGcm03648 */
	if (!cfg->is_usdhc && !data && (cmd->resp_type & MMC_RSP_BUSY)) {
		int timeout = 2500;

		/* Poll on DATA0 line for cmd with busy signal for 250 ms */
		while (timeout > 0 && !(readl(&regs->prsstat) & PRSSTAT_DAT0)) {
			udelay(100);
			timeout--;
		}

		writel(sysctl_restore, &regs->sysctl);

		if (timeout <= 0) {
			printf("Timeout waiting for DAT0 to go high!\n");
			return TIMEOUT;
		}
	}

	/* Copy the response to the response buffer */
	if (cmd->resp_type & MMC_RSP_136) {
		u32 cmdrsp3, cmdrsp2, cmdrsp1, cmdrsp0;

		cmdrsp3 = readl(&regs->cmdrsp3);
		cmdrsp2 = readl(&regs->cmdrsp2);
		cmdrsp1 = readl(&regs->cmdrsp1);
		cmdrsp0 = readl(&regs->cmdrsp0);
		cmd->response[0] = (cmdrsp3 << 8) | (cmdrsp2 >> 24);
		cmd->response[1] = (cmdrsp2 << 8) | (cmdrsp1 >> 24);
		cmd->response[2] = (cmdrsp1 << 8) | (cmdrsp0 >> 24);
		cmd->response[3] = (cmdrsp0 << 8);
	} else
		cmd->response[0] = readl(&regs->cmdrsp0);

	/* Wait until all of the blocks are transferred */
	if (data) {
		int i = 0, j = 0;
		u32 *tmp_ptr = NULL;
		uint block_size = data->blocksize;
		uint block_cnt = data->blocks;

		tmp = readl(&regs->irqstaten) | SDHCI_IRQ_EN_BITS;
		writel(tmp, &regs->irqstaten);

		if (data->flags & MMC_DATA_READ) {
			tmp_ptr = (u32 *)data->dest;

			for (i = 0; i < (block_cnt); ++i) {
				while (!(readl(&regs->irqstat) & IRQSTAT_BRR)) {
					/*Check data error to avoid dead loop*/
					if (readl(&regs->irqstat) & DATA_ERR) {
						goto send_end;
					}
				}

				for (j = 0; j < (block_size >> 2); ++j, ++tmp_ptr) {
					*tmp_ptr = readl(&regs->datport);
				}

				tmp = readl(&regs->irqstat) & (IRQSTAT_BRR);
				writel(tmp, &regs->irqstat);
			}
		} else {
			tmp_ptr = (u32 *)data->src;

			for (i = 0; i < (block_cnt); ++i) {
				while (!(readl(&regs->irqstat) & IRQSTAT_BWR)) {
					/*Check timeout error to avoid dead loop*/
					if (readl(&regs->irqstat) & IRQSTAT_DTOE) {
						goto send_end;
					}
				}

				for (j = 0; j < (block_size >> 2); ++j, ++tmp_ptr) {
					writel(*tmp_ptr, &regs->datport);
				}

				tmp = readl(&regs->irqstat) & (IRQSTAT_BWR);
				writel(tmp, &regs->irqstat);
			}
		}

		while (!(readl(&regs->irqstat) & IRQSTAT_TC)) ;
	}

send_end:

	/* Clear the tune execute bit*/
	mixctrl = readl(&regs->mixctrl);
	if (mixctrl & USDHC_MIXCTRL_EXE_TUNE) {
		mixctrl &= ~USDHC_MIXCTRL_EXE_TUNE;
		writel(mixctrl, &regs->mixctrl);
	}

	/* Reset CMD and DATA portions of the controller on error */
	if (readl(&regs->irqstat) & 0xFFFF0000) {
		writel(readl(&regs->sysctl) | SYSCTL_RSTC | SYSCTL_RSTD,
			&regs->sysctl);
		while (readl(&regs->sysctl) & (SYSCTL_RSTC | SYSCTL_RSTD))
			;

		return COMM_ERR;
	}
	writel(-1, &regs->irqstat);

	return 0;
}

void set_sysctl(struct mmc *mmc, uint clock)
{
	int sdhc_clk = mxc_get_clock(MXC_ESDHC_CLK);
	int div, pre_div;
	struct fsl_esdhc_cfg *cfg = (struct fsl_esdhc_cfg *)mmc->priv;
	volatile struct fsl_esdhc *regs = (struct fsl_esdhc *)cfg->esdhc_base;
	uint clk;
	u32 tmp;

	if (sdhc_clk / 16 > clock) {
		for (pre_div = 2; pre_div < 256; pre_div *= 2)
			if ((sdhc_clk / pre_div) <= (clock * 16))
				break;
	} else
		pre_div = 2;

	/* For the case where clock requested is equal to SDHC clock,
	 * the pre_div should be 1.
	 */
	if (clock == sdhc_clk)
		pre_div = 1;

	for (div = 1; div <= 16; div++)
		if ((sdhc_clk / (div * pre_div)) <= clock)
			break;

	pre_div >>= 1;
	div -= 1;

	/* for USDHC, pre_div requires another shift in DDR mode */
	if (cfg->is_usdhc && (mmc->card_caps & EMMC_MODE_4BIT_DDR ||
		mmc->card_caps & EMMC_MODE_8BIT_DDR))
		pre_div >>= 1;

	clk = (pre_div << 8) | (div << 4);

#ifndef CONFIG_IMX_ESDHC_V1
	tmp = readl(&regs->sysctl) & (~SYSCTL_SDCLKEN);
	writel(tmp, &regs->sysctl);
#endif

	tmp = (readl(&regs->sysctl) & (~SYSCTL_CLOCK_MASK)) | clk;
	writel(tmp, &regs->sysctl);

	udelay(10000);

#ifdef CONFIG_IMX_ESDHC_V1
	tmp = readl(&regs->sysctl) | SYSCTL_PEREN;
	writel(tmp, &regs->sysctl);
#else
	while (!(readl(&regs->prsstat) & PRSSTAT_SDSTB)) ;

	tmp = readl(&regs->sysctl) | (SYSCTL_SDCLKEN);
	writel(tmp, &regs->sysctl);
#endif
}

static void esdhc_dll_setup(struct mmc *mmc)
{
	struct fsl_esdhc_cfg *cfg = (struct fsl_esdhc_cfg *)mmc->priv;
	struct fsl_esdhc *regs = (struct fsl_esdhc *)cfg->esdhc_base;
	uint dll_control;
	u32 target_delay = ESDHC_DLL_TARGET_DEFAULT_VAL;

/* For DDR mode operation, provide target delay parameter for each SD port.
 * Use cfg->esdhc_base to distinguish the SD port #. The delay for each port
 * is dependent on signal layout for that particular port. If the following
 * CONFIG is not defined, then the default target delay value will be used.
 */
#ifdef CONFIG_GET_DDR_TARGET_DELAY
	target_delay = get_ddr_delay(cfg);
#endif

	/* For i.MX50 TO1, need to force slave override mode */
	if (get_board_rev() == (0x50000 | CHIP_REV_1_0) ||
			((get_board_rev() & 0xff000) == 0x53000)) {
		dll_control = readl(&regs->dllctrl);

		dll_control &= ~(ESDHC_DLLCTRL_SLV_OVERRIDE_VAL_MASK |
			ESDHC_DLLCTRL_SLV_OVERRIDE);
		dll_control |= ((ESDHC_DLLCTRL_SLV_OVERRIDE_VAL <<
			ESDHC_DLLCTRL_SLV_OVERRIDE_VAL_SHIFT) |
			ESDHC_DLLCTRL_SLV_OVERRIDE);

		writel(dll_control, &regs->dllctrl);
	} else {

		/* on USDHC, enable DLL only for clock > 25 MHz */
		if (cfg->is_usdhc && mmc->clock <= 25000000)
			return;

		/* Disable auto clock gating for PERCLK, HCLK, and IPGCLK */
		writel(readl(&regs->sysctl) | 0x7, &regs->sysctl);
		/* Stop SDCLK while delay line is calibrated */
		writel(readl(&regs->sysctl) &= ~SYSCTL_SDCLKEN, &regs->sysctl);

		/* Reset DLL */
		writel(readl(&regs->dllctrl) | 0x2, &regs->dllctrl);

		dll_control = 0;

		/* Enable DLL */
		if (cfg->is_usdhc)
			dll_control |= 0x01000001;
		else
			dll_control |= 0x00000001;

		writel(dll_control, &regs->dllctrl);

		/* Set target delay */
		if (cfg->is_usdhc) {
			dll_control &= ~USDHC_DLLCTRL_TARGET_MASK;
			dll_control |= (((target_delay & USDHC_DLL_LOW_MASK) <<
				USDHC_DLLCTRL_TARGET_LOW_SHIFT) |
				((target_delay >> USDHC_DLL_HIGH_SHIFT) <<
				USDHC_DLLCTRL_TARGET_HIGH_SHIFT));
			writel(dll_control, &regs->dllctrl);
		} else {
			dll_control &= ~ESDHC_DLLCTRL_TARGET_MASK;
			dll_control |= (target_delay << ESDHC_DLLCTRL_TARGET_SHIFT);
			writel(dll_control, &regs->dllctrl);
		}

		/* Wait for slave lock */
		while ((readl(&regs->dllstatus) & ESDHC_DLLSTS_SLV_LOCK_MASK) !=
			ESDHC_DLLSTS_SLV_LOCK_MASK)
			;

		/* Re-enable auto clock gating */
		writel(readl(&regs->sysctl) | SYSCTL_SDCLKEN, &regs->sysctl);
		/* Re-enable SDCLK */
		writel(readl(&regs->sysctl) &= ~0x7, &regs->sysctl);
	}
}

/*
 * CPU and board-specific Ethernet initializations.  Aliased function
 * signals caller to move on
 */
static int __def_mmc_io_switch(u32 index, u32 clock)
{
	return -1;
}

int board_mmc_io_switch(u32 index, u32 clock)
	__attribute__((weak, alias("__def_mmc_io_switch")));

static void esdhc_set_ios(struct mmc *mmc)
{
	struct fsl_esdhc_cfg *cfg = (struct fsl_esdhc_cfg *)mmc->priv;
	struct fsl_esdhc *regs = (struct fsl_esdhc *)cfg->esdhc_base;
	u32 tmp;

	/* Set the io pad*/
	board_mmc_io_switch(mmc->block_dev.dev, mmc->clock);

	/* Set the clock speed */
	set_sysctl(mmc, mmc->clock);

	/* Set the bus width */
	tmp = readl(&regs->proctl) & (~(PROCTL_DTW_4 | PROCTL_DTW_8));
	writel(tmp, &regs->proctl);

	if (mmc->bus_width == 4) {
		tmp = readl(&regs->proctl) | PROCTL_DTW_4;
		writel(tmp, &regs->proctl);
	} else if (mmc->bus_width == 8) {
		tmp = readl(&regs->proctl) | PROCTL_DTW_8;
		writel(tmp, &regs->proctl);
	}

	if (mmc->card_caps & EMMC_MODE_4BIT_DDR ||
		mmc->card_caps & EMMC_MODE_8BIT_DDR)
		esdhc_dll_setup(mmc);
}

static void esdhc_uhsi_tuning(struct mmc *mmc, uint val)
{
	struct fsl_esdhc_cfg *cfg = (struct fsl_esdhc_cfg *)mmc->priv;
	struct fsl_esdhc *regs = (struct fsl_esdhc *)cfg->esdhc_base;
	u32 mixctrl;

	/* No tuning needed for 50 MHz or lower */
	if (mmc->card_uhs_mode < SD_UHSI_FUNC_SDR50)
		return;

	mixctrl = readl(&regs->mixctrl);
	mixctrl |= USDHC_MIXCTRL_EXE_TUNE | \
		USDHC_MIXCTRL_SMPCLK_SEL | \
		USDHC_MIXCTRL_FBCLK_SEL;
	writel(mixctrl, &regs->mixctrl);
	writel((val << 8), &regs->clktunectrlstatus);
}

static int esdhc_init(struct mmc *mmc)
{
	struct fsl_esdhc_cfg *cfg = (struct fsl_esdhc_cfg *)mmc->priv;
	struct fsl_esdhc *regs = (struct fsl_esdhc *)cfg->esdhc_base;
	u32 tmp;

	/* Reset the eSDHC by writing 1 to RSTA bit of SYSCTRL Register */
	tmp = readl(&regs->sysctl) | SYSCTL_RSTA;
	writel(tmp, &regs->sysctl);

	while (readl(&regs->sysctl) & SYSCTL_RSTA)
		;

	/* RSTA doesn't reset MMC_BOOT register, so manually reset it */
	writel(0, &regs->mmcboot);
	/* Reset MIX_CTRL and CLK_TUNE_CTRL_STATUS regs to 0 */
	writel(0, &regs->mixctrl);
	writel(0, &regs->clktunectrlstatus);

	/* Put VEND_SPEC to default value */
	writel(VENDORSPEC_INIT, &regs->vendorspec);

#ifdef CONFIG_IMX_ESDHC_V1
	tmp = readl(&regs->sysctl) | (SYSCTL_HCKEN | SYSCTL_IPGEN);
	writel(tmp, &regs->sysctl);
#endif

	/* Set the initial clock speed */
	set_sysctl(mmc, 400000);

	/* Put the PROCTL reg back to the default */
	writel(PROCTL_INIT, &regs->proctl);

	/* FIXME: For our CINS bit doesn't work. So this section is disabled. */
	/*
	while (!(readl(&regs->prsstat) & PRSSTAT_CINS) && --timeout)
		;

	if (timeout <= 0) {
		printf("No MMC card detected!\n");
		return NO_CARD_ERR;
	}
	*/

#ifndef CONFIG_IMX_ESDHC_V1
	tmp = readl(&regs->sysctl) | SYSCTL_INITA;
	writel(tmp, &regs->sysctl);

	while (readl(&regs->sysctl) & SYSCTL_INITA)
		;
#endif

	return 0;
}

int fsl_esdhc_initialize(bd_t *bis, struct fsl_esdhc_cfg *cfg)
{
	struct fsl_esdhc *regs;
	struct mmc *mmc;
	u32 caps;

	if (!cfg)
		return -1;

	mmc = malloc(sizeof(struct mmc));

	sprintf(mmc->name, "FSL_ESDHC");
	regs = (struct fsl_esdhc *)cfg->esdhc_base;
	mmc->priv = cfg;
	mmc->send_cmd = esdhc_send_cmd;
	mmc->set_ios = esdhc_set_ios;
	mmc->init = esdhc_init;
	mmc->set_tuning = esdhc_uhsi_tuning;

/* Enable uSDHC if the config is defined (only for i.MX50 in SDR mode) */
#ifdef CONFIG_MX50_ENABLE_USDHC_SDR
	enable_usdhc();
#endif

	if (cfg->is_usdhc)
		sprintf(mmc->name, "FSL_USDHC");

	caps = readl(&regs->hostcapblt);

	if (caps & ESDHC_HOSTCAPBLT_VS30)
		mmc->voltages |= MMC_VDD_29_30 | MMC_VDD_30_31;
	if (caps & ESDHC_HOSTCAPBLT_VS33)
		mmc->voltages |= MMC_VDD_32_33 | MMC_VDD_33_34;

	mmc->host_caps = MMC_MODE_4BIT;

	if (caps & ESDHC_HOSTCAPBLT_HSS)
		mmc->host_caps |= MMC_MODE_HS_52MHz | MMC_MODE_HS | MMC_MODE_HC;

	/* Do not advertise DDR capability for uSDHC on MX50 since
	*  it is to be used in SDR mode only. Use eSDHC for DDR mode.
	*/
#ifndef CONFIG_MX50_ENABLE_USDHC_SDR
	if (cfg->is_usdhc)
		mmc->host_caps |= EMMC_MODE_4BIT_DDR;

#ifdef CONFIG_EMMC_DDR_PORT_DETECT
	if (detect_mmc_emmc_ddr_port(cfg))
		mmc->host_caps |= EMMC_MODE_4BIT_DDR;
#endif
#endif /* #ifndef CONFIG_MX50_ENABLE_USDHC_SDR */

	mmc->f_min = 400000;
	mmc->f_max = MIN(mxc_get_clock(MXC_ESDHC_CLK), 52000000);

	if (cfg->is_usdhc) {
		mmc->f_max = MIN(mxc_get_clock(MXC_ESDHC_CLK), 208000000);
		mmc->tuning_max = USDHC_TUNE_CTRL_MAX;
		mmc->tuning_min = USDHC_TUNE_CTRL_MIN;
		mmc->tuning_step = USDHC_TUNE_CTRL_STEP;
	}

	if (cfg->port_supports_uhs18v)
		mmc->host_caps |= SD_UHSI_CAP_ALL_MODES;

	mmc_register(mmc);

#ifdef CONFIG_MMC_8BIT_PORTS
	if ((1 << mmc->block_dev.dev) & CONFIG_MMC_8BIT_PORTS) {
		mmc->host_caps |= MMC_MODE_8BIT;

		if (mmc->host_caps & EMMC_MODE_4BIT_DDR)
			mmc->host_caps |= EMMC_MODE_8BIT_DDR;
	}
#endif

	return 0;
}

int fsl_esdhc_mmc_init(bd_t *bis)
{
	struct fsl_esdhc_cfg *cfg;

	cfg = malloc(sizeof(struct fsl_esdhc_cfg));
	memset(cfg, 0, sizeof(struct fsl_esdhc_cfg));
	cfg->esdhc_base = CONFIG_SYS_FSL_ESDHC_ADDR;
	return fsl_esdhc_initialize(bis, cfg);
}

#ifdef CONFIG_OF_LIBFDT
void fdt_fixup_esdhc(void *blob, bd_t *bd)
{
	const char *compat = "fsl,esdhc";
	const char *status = "okay";

	if (!hwconfig("esdhc")) {
		status = "disabled";
		goto out;
	}

	do_fixup_by_compat_u32(blob, compat, "clock-frequency",
			       gd->sdhc_clk, 1);
out:
	do_fixup_by_compat(blob, compat, "status", status,
			   strlen(status) + 1, 1);
}
#endif
