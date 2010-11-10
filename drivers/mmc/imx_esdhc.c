/*
 * (C) Copyright 2008-2010 Freescale Semiconductor, Inc.
 * Terry Lv, Jason Liu
 *
 * Copyright 2007, Freescale Semiconductor, Inc
 * Andy Fleming
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
	char	reserved1[8];
	uint	fevt;
	char	reserved2[12];
	uint dllctrl;
	uint dllstatus;
	char	reserved3[148];
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
	uint	xfertyp;
	uint	irqstat;
	u32	tmp;
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

	/* Wait at least 8 SD clock cycles before the next command */
	/*
	 * Note: This is way more than 8 cycles, but 1ms seems to
	 * resolve timing issues with some cards
	 */
	udelay(10000);

	/* Set up for a data transfer if we have one */
	if (data) {
		int err;

		err = esdhc_setup_data(mmc, data);
		if(err)
			return err;
	}

	/* Figure out the transfer arguments */
	xfertyp = esdhc_xfertyp(cmd, data);

	if (mmc->bus_width == EMMC_MODE_4BIT_DDR ||
		mmc->bus_width == EMMC_MODE_8BIT_DDR)
		xfertyp |= XFERTYP_DDR_EN;

	/* Send the command */
	writel(cmd->cmdarg, &regs->cmdarg);
	writel(xfertyp, &regs->xfertyp);

	/* Mask all irqs */
	writel(0, &regs->irqsigen);

	/* Wait for the command to complete */
	while (!(readl(&regs->irqstat) & IRQSTAT_CC));

	irqstat = readl(&regs->irqstat);
	writel(irqstat, &regs->irqstat);

	if (irqstat & CMD_ERR)
		return COMM_ERR;

	if (irqstat & IRQSTAT_CTOE)
		return TIMEOUT;

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
				while (!(readl(&regs->irqstat) & IRQSTAT_BRR)) 
					;

				for (j = 0; j < (block_size >> 2); ++j, ++tmp_ptr) {
					*tmp_ptr = readl(&regs->datport);
				}

				tmp = readl(&regs->irqstat) & (IRQSTAT_BRR);
				writel(tmp, &regs->irqstat);
			}
		} else {
			tmp_ptr = (u32 *)data->src;

			for (i = 0; i < (block_cnt); ++i) {
				while (!(readl(&regs->irqstat) & IRQSTAT_BWR))
					;

				for (j = 0; j < (block_size >> 2); ++j, ++tmp_ptr) {
					writel(*tmp_ptr, &regs->datport);
				}

				tmp = readl(&regs->irqstat) & (IRQSTAT_BWR);
				writel(tmp, &regs->irqstat);
			}
		}

		while (!(readl(&regs->irqstat) & IRQSTAT_TC)) ;
	}

	if (readl(&regs->irqstat) & 0xFFFF0000)
		return COMM_ERR;

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

	for (div = 1; div <= 16; div++)
		if ((sdhc_clk / (div * pre_div)) <= clock)
			break;

	pre_div >>= 1;
	div -= 1;

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

	/* For i.MX50 TO1, need to force slave override mode */
	if (get_board_rev() == (0x50000 | CHIP_REV_1_0)) {
		dll_control = readl(&regs->dllctrl);

		dll_control &= ~(ESDHC_DLLCTRL_SLV_OVERRIDE_VAL_MASK |
			ESDHC_DLLCTRL_SLV_OVERRIDE);
		dll_control |= ((ESDHC_DLLCTRL_SLV_OVERRIDE_VAL <<
			ESDHC_DLLCTRL_SLV_OVERRIDE_VAL_SHIFT) |
			ESDHC_DLLCTRL_SLV_OVERRIDE);

		writel(dll_control, &regs->dllctrl);
	} else {
		/* Disable auto clock gating for PERCLK, HCLK, and IPGCLK */
		writel(readl(&regs->sysctl) | 0x7, &regs->sysctl);
		/* Stop SDCLK while delay line is calibrated */
		writel(readl(&regs->sysctl) &= ~SYSCTL_SDCLKEN, &regs->sysctl);

		/* Reset DLL */
		writel(readl(&regs->dllctrl) | 0x2, &regs->dllctrl);

		/* Enable DLL */
		writel(readl(&regs->dllctrl) | 0x1, &regs->dllctrl);

		dll_control = readl(&regs->dllctrl);

		/* Set target delay */
		dll_control &= ~ESDHC_DLLCTRL_TARGET_MASK;
		dll_control |= (ESDHC_DLL_TARGET_DEFAULT_VAL <<
				ESDHC_DLLCTRL_TARGET_SHIFT);
		writel(dll_control, &regs->dllctrl);

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

static void esdhc_set_ios(struct mmc *mmc)
{
	struct fsl_esdhc_cfg *cfg = (struct fsl_esdhc_cfg *)mmc->priv;
	struct fsl_esdhc *regs = (struct fsl_esdhc *)cfg->esdhc_base;
	u32 tmp;

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
	} else if (mmc->bus_width == EMMC_MODE_4BIT_DDR) {
		tmp = readl(&regs->proctl) | PROCTL_DTW_4;
		writel(tmp, &regs->proctl);
		esdhc_dll_setup(mmc);
	} else if (mmc->bus_width == EMMC_MODE_8BIT_DDR) {
		tmp = readl(&regs->proctl) | PROCTL_DTW_8;
		writel(tmp, &regs->proctl);
		esdhc_dll_setup(mmc);
	}
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

	caps = readl(&regs->hostcapblt);
	if (caps & ESDHC_HOSTCAPBLT_VS30)
		mmc->voltages |= MMC_VDD_29_30 | MMC_VDD_30_31;
	if (caps & ESDHC_HOSTCAPBLT_VS33)
		mmc->voltages |= MMC_VDD_32_33 | MMC_VDD_33_34;

	mmc->host_caps = MMC_MODE_4BIT;

	if (caps & ESDHC_HOSTCAPBLT_HSS)
		mmc->host_caps |= MMC_MODE_HS_52MHz | MMC_MODE_HS;

	if (((readl(&regs->hostver) & ESDHC_HOSTVER_VVN_MASK)
		>> ESDHC_HOSTVER_VVN_SHIFT) >= ESDHC_HOSTVER_DDR_SUPPORT)
		mmc->host_caps |= EMMC_MODE_4BIT_DDR;

	mmc->f_min = 400000;
	mmc->f_max = MIN(mxc_get_clock(MXC_ESDHC_CLK), 50000000);

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
