/*
 * (C) Copyright 2008-2009 Freescale Semiconductor, Inc.
 * Terry Lv
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
#include <mmc.h>
#include <malloc.h>
#include <mmc.h>
#include <fsl_esdhc.h>
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
	char	reserved2[168];
	uint	hostver;
	char	reserved3[780];
	uint	scr;
};


static inline void mdelay(unsigned long msec)
{
	unsigned long i;
	for (i = 0; i < msec; i++)
		udelay(1000);
}

static inline void sdelay(unsigned long sec)
{
	unsigned long i;
	for (i = 0; i < sec; i++)
		mdelay(1000);
}

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
	struct fsl_esdhc *regs = mmc->priv;

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
	timeout = __ilog2(mmc->tran_speed/10);
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
	volatile struct fsl_esdhc *regs = mmc->priv;

	writel(-1, &regs->irqstat);

	sync();

	tmp = readl(&regs->irqstaten) | SDHCI_IRQ_EN_BITS;
	writel(tmp, &regs->irqstaten);

	/* Wait for the bus to be idle */
	while ((readl(&regs->prsstat) & PRSSTAT_CICHB) ||
			(readl(&regs->prsstat) & PRSSTAT_CIDHB))
			mdelay(1);

	while (readl(&regs->prsstat) & PRSSTAT_DLA);

	/* Wait at least 8 SD clock cycles before the next command */
	/*
	 * Note: This is way more than 8 cycles, but 1ms seems to
	 * resolve timing issues with some cards
	 */
	mdelay(10);

	/* Set up for a data transfer if we have one */
	if (data) {
		int err;

		err = esdhc_setup_data(mmc, data);
		if(err)
			return err;
	}

	/* Figure out the transfer arguments */
	xfertyp = esdhc_xfertyp(cmd, data);

	/* Send the command */
	writel(cmd->cmdarg, &regs->cmdarg);
	writel(xfertyp, &regs->xfertyp);

	mdelay(10);

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
					mdelay(1);

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
					mdelay(1);

				for (j = 0; j < (block_size >> 2); ++j, ++tmp_ptr) {
					writel(*tmp_ptr, &regs->datport);
				}

				tmp = readl(&regs->irqstat) & (IRQSTAT_BWR);
				writel(tmp, &regs->irqstat);
			}
		}

		while (!(readl(&regs->irqstat) & IRQSTAT_TC)) ;
	}

	writel(-1, &regs->irqstat);

	return 0;
}

void set_sysctl(struct mmc *mmc, uint clock)
{
	int sdhc_clk = mxc_get_clock(MXC_ESDHC_CLK);
	int div, pre_div;
	volatile struct fsl_esdhc *regs = mmc->priv;
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

	mdelay(100);

#ifdef CONFIG_IMX_ESDHC_V1
	tmp = readl(&regs->sysctl) | SYSCTL_PEREN;
	writel(tmp, &regs->sysctl);
#else
	while (!(readl(&regs->prsstat) & PRSSTAT_SDSTB)) ;

	tmp = readl(&regs->sysctl) | (SYSCTL_SDCLKEN);
	writel(tmp, &regs->sysctl);
#endif
}

static void esdhc_set_ios(struct mmc *mmc)
{
	struct fsl_esdhc *regs = mmc->priv;
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
	}
}

static int esdhc_init(struct mmc *mmc)
{
	struct fsl_esdhc *regs = mmc->priv;
	int timeout = 1000;
	u32 tmp;

	/* Reset the eSDHC by writing 1 to RSTA bit of SYSCTRL Register */
	tmp = readl(&regs->sysctl) | SYSCTL_RSTA;
	writel(tmp, &regs->sysctl);

	while (readl(&regs->sysctl) & SYSCTL_RSTA)
	    mdelay(1);

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
		mdelay(1);

	if (timeout <= 0) {
		printf("No MMC card detected!\n");
		return NO_CARD_ERR;
	}
	*/

#ifndef CONFIG_IMX_ESDHC_V1
	tmp = readl(&regs->sysctl) | SYSCTL_INITA;
	writel(tmp, &regs->sysctl);

	while (readl(&regs->sysctl) & SYSCTL_INITA)
		mdelay(1);
#endif

	return 0;
}

#ifndef CONFIG_SYS_FSL_ESDHC_ADDR
extern u32 *imx_esdhc_base_addr;
#endif

static int esdhc_initialize(bd_t *bis)
{
#ifdef CONFIG_SYS_FSL_ESDHC_ADDR
	struct fsl_esdhc *regs = (struct fsl_esdhc *)CONFIG_SYS_IMX_ESDHC_ADDR;
#else
	struct fsl_esdhc *regs = (struct fsl_esdhc *)imx_esdhc_base_addr;
#endif
	struct mmc *mmc;
	u32 caps;

	mmc = malloc(sizeof(struct mmc));

	sprintf(mmc->name, "FSL_ESDHC");
	mmc->priv = regs;
	mmc->send_cmd = esdhc_send_cmd;
	mmc->set_ios = esdhc_set_ios;
	mmc->init = esdhc_init;

	/*
	caps = regs->hostcapblt;

	if (caps & ESDHC_HOSTCAPBLT_VS18)
		mmc->voltages |= MMC_VDD_165_195;
	if (caps & ESDHC_HOSTCAPBLT_VS30)
		mmc->voltages |= MMC_VDD_29_30 | MMC_VDD_30_31;
	if (caps & ESDHC_HOSTCAPBLT_VS33) {
		mmc->voltages |= MMC_VDD_32_33 | MMC_VDD_33_34;
	}
	*/
	mmc->voltages = MMC_VDD_35_36 | MMC_VDD_34_35 | MMC_VDD_33_34 |
			MMC_VDD_32_33 | MMC_VDD_31_32 | MMC_VDD_30_31 |
			MMC_VDD_29_30 | MMC_VDD_28_29 | MMC_VDD_27_28;

	mmc->host_caps = MMC_MODE_4BIT | MMC_MODE_8BIT;

	if (caps & ESDHC_HOSTCAPBLT_HSS)
		mmc->host_caps |= MMC_MODE_HS_52MHz | MMC_MODE_HS;

	mmc->f_min = 400000;
	mmc->f_max = MIN(mxc_get_clock(MXC_ESDHC_CLK), 50000000);

	mmc_register(mmc);

	return 0;
}

int fsl_esdhc_mmc_init(bd_t *bis)
{
	return esdhc_initialize(bis);
}

