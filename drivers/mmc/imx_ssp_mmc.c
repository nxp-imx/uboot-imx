/*
 * Copyright (C) 2008-2011 Freescale Semiconductor, Inc.
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
#include <common.h>
#include <malloc.h>
#include <mmc.h>
#include <asm/arch/regs-ssp.h>
#include <asm/arch/regs-clkctrl.h>
#include <imx_ssp_mmc.h>

#undef IMX_SSP_MMC_DEBUG

static inline int ssp_mmc_read(struct mmc *mmc, uint reg)
{
	struct imx_ssp_mmc_cfg *cfg = (struct imx_ssp_mmc_cfg *)mmc->priv;
	return REG_RD(cfg->ssp_mmc_base, reg);
}

static inline void ssp_mmc_write(struct mmc *mmc, uint reg, uint val)
{
	struct imx_ssp_mmc_cfg *cfg = (struct imx_ssp_mmc_cfg *)mmc->priv;
	REG_WR(cfg->ssp_mmc_base, reg, val);
}

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

/*
 * Sends a command out on the bus.  Takes the mmc pointer,
 * a command pointer, and an optional data pointer.
 */
static int
ssp_mmc_send_cmd(struct mmc *mmc, struct mmc_cmd *cmd, struct mmc_data *data)
{
	int i;

#ifdef IMX_SSP_MMC_DEBUG
	printf("MMC%d: CMD%d\n", mmc->block_dev.dev, cmd->cmdidx);
#endif

	/* Check bus busy */
	i = 0;
	while (ssp_mmc_read(mmc, HW_SSP_STATUS) & (BM_SSP_STATUS_BUSY |
		BM_SSP_STATUS_DATA_BUSY | BM_SSP_STATUS_CMD_BUSY)) {
		mdelay(1);
		i++;
		if (i == 1000) {
			printf("MMC%d: Bus busy timeout!\n",
				mmc->block_dev.dev);
			return TIMEOUT;
		}
	}

	/* See if card is present */
	if (ssp_mmc_read(mmc, HW_SSP_STATUS) & BM_SSP_STATUS_CARD_DETECT) {
		printf("MMC%d: No card detected!\n", mmc->block_dev.dev);
		return NO_CARD_ERR;
	}

	/* Clear all control bits except bus width */
	ssp_mmc_write(mmc, HW_SSP_CTRL0_CLR, 0xff3fffff);

	/* Set up command */
	if (!(cmd->resp_type & MMC_RSP_CRC))
		ssp_mmc_write(mmc, HW_SSP_CTRL0_SET, BM_SSP_CTRL0_IGNORE_CRC);
	if (cmd->resp_type & MMC_RSP_PRESENT)	/* Need to get response */
		ssp_mmc_write(mmc, HW_SSP_CTRL0_SET, BM_SSP_CTRL0_GET_RESP);
	if (cmd->resp_type & MMC_RSP_136)	/* It's a 136 bits response */
		ssp_mmc_write(mmc, HW_SSP_CTRL0_SET, BM_SSP_CTRL0_LONG_RESP);

	/* Command index */
	ssp_mmc_write(mmc, HW_SSP_CMD0,
		(ssp_mmc_read(mmc, HW_SSP_CMD0) & ~BM_SSP_CMD0_CMD) |
		(cmd->cmdidx << BP_SSP_CMD0_CMD));
	/* Command argument */
	ssp_mmc_write(mmc, HW_SSP_CMD1, cmd->cmdarg);

	/* Set up data */
	if (data) {
		/* READ or WRITE */
		if (data->flags & MMC_DATA_READ) {
			ssp_mmc_write(mmc, HW_SSP_CTRL0_SET,
				BM_SSP_CTRL0_READ);
		} else if (ssp_mmc_is_wp(mmc)) {
			printf("MMC%d: Can not write a locked card!\n",
				mmc->block_dev.dev);
			return UNUSABLE_ERR;
		}
		ssp_mmc_write(mmc, HW_SSP_CTRL0_SET, BM_SSP_CTRL0_DATA_XFER);
		ssp_mmc_write(mmc, HW_SSP_BLOCK_SIZE,
			((data->blocks - 1) <<
				BP_SSP_BLOCK_SIZE_BLOCK_COUNT) |
			((ffs(data->blocksize) - 1) <<
				BP_SSP_BLOCK_SIZE_BLOCK_SIZE));
		ssp_mmc_write(mmc, HW_SSP_XFER_SIZE,
			data->blocksize * data->blocks);
	}

	/* Kick off the command */
	ssp_mmc_write(mmc, HW_SSP_CTRL0_SET, BM_SSP_CTRL0_WAIT_FOR_IRQ);
	ssp_mmc_write(mmc, HW_SSP_CTRL0_SET, BM_SSP_CTRL0_ENABLE);
	ssp_mmc_write(mmc, HW_SSP_CTRL0_SET, BM_SSP_CTRL0_RUN);

	/* Wait for the command to complete */
	i = 0;
	do {
		mdelay(10);
		if (i++ == 100) {
			printf("MMC%d: Command %d busy\n",
				mmc->block_dev.dev,
				cmd->cmdidx);
			break;
		}
	} while (ssp_mmc_read(mmc, HW_SSP_STATUS) &
		BM_SSP_STATUS_CMD_BUSY);

	/* Check command timeout */
	if (ssp_mmc_read(mmc, HW_SSP_STATUS) &
		BM_SSP_STATUS_RESP_TIMEOUT) {
#ifdef IMX_SSP_MMC_DEBUG
		printf("MMC%d: Command %d timeout\n", mmc->block_dev.dev,
			cmd->cmdidx);
#endif
		return TIMEOUT;
	}

	/* Check command errors */
	if (ssp_mmc_read(mmc, HW_SSP_STATUS) &
		(BM_SSP_STATUS_RESP_CRC_ERR | BM_SSP_STATUS_RESP_ERR)) {
		printf("MMC%d: Command %d error (status 0x%08x)!\n",
			mmc->block_dev.dev, cmd->cmdidx,
			ssp_mmc_read(mmc, HW_SSP_STATUS));
		return COMM_ERR;
	}

	/* Copy response to response buffer */
	if (cmd->resp_type & MMC_RSP_136) {
		cmd->response[3] = ssp_mmc_read(mmc, HW_SSP_SDRESP0);
		cmd->response[2] = ssp_mmc_read(mmc, HW_SSP_SDRESP1);
		cmd->response[1] = ssp_mmc_read(mmc, HW_SSP_SDRESP2);
		cmd->response[0] = ssp_mmc_read(mmc, HW_SSP_SDRESP3);
	} else
		cmd->response[0] = ssp_mmc_read(mmc, HW_SSP_SDRESP0);

	/* Return if no data to process */
	if (!data)
		return 0;

	/* Process the data */
	u32 xfer_cnt = data->blocksize * data->blocks;
	u32 *tmp_ptr;

	if (data->flags & MMC_DATA_READ) {
		tmp_ptr = (u32 *)data->dest;
		while (xfer_cnt > 0) {
			if ((ssp_mmc_read(mmc, HW_SSP_STATUS) &
				BM_SSP_STATUS_FIFO_EMPTY) == 0) {
				*tmp_ptr++ = ssp_mmc_read(mmc, HW_SSP_DATA);
				xfer_cnt -= 4;
			}
		}
	} else {
		tmp_ptr = (u32 *)data->src;
		while (xfer_cnt > 0) {
			if ((ssp_mmc_read(mmc, HW_SSP_STATUS) &
				BM_SSP_STATUS_FIFO_FULL) == 0) {
				ssp_mmc_write(mmc, HW_SSP_DATA, *tmp_ptr++);
				xfer_cnt -= 4;
			}
		}
	}

	/* Check data errors */
	if (ssp_mmc_read(mmc, HW_SSP_STATUS) &
		(BM_SSP_STATUS_TIMEOUT | BM_SSP_STATUS_DATA_CRC_ERR |
		BM_SSP_STATUS_FIFO_OVRFLW | BM_SSP_STATUS_FIFO_UNDRFLW)) {
		printf("MMC%d: Data error with command %d (status 0x%08x)!\n",
			mmc->block_dev.dev, cmd->cmdidx,
			ssp_mmc_read(mmc, HW_SSP_STATUS));
		return COMM_ERR;
	}

	return 0;
}

static void set_bit_clock(struct mmc *mmc, u32 clock)
{
	const u32 sspclk = 480000 * 18 / 29 / 1;	/* 297931 KHz */
	u32 divide, rate, tgtclk;

	/*
	 * SSP bit rate = SSPCLK / (CLOCK_DIVIDE * (1 + CLOCK_RATE)),
	 * CLOCK_DIVIDE has to be an even value from 2 to 254, and
	 * CLOCK_RATE could be any integer from 0 to 255.
	 */
	clock /= 1000;		/* KHz */
	for (divide = 2; divide < 254; divide += 2) {
		rate = sspclk / clock / divide;
		if (rate <= 256)
			break;
	}

	tgtclk = sspclk / divide / rate;
	while (tgtclk > clock) {
		rate++;
		tgtclk = sspclk / divide / rate;
	}
	if (rate > 256)
		rate = 256;

	/* Always set timeout the maximum */
	ssp_mmc_write(mmc, HW_SSP_TIMING, BM_SSP_TIMING_TIMEOUT |
		divide << BP_SSP_TIMING_CLOCK_DIVIDE |
		(rate - 1) << BP_SSP_TIMING_CLOCK_RATE);

#ifdef IMX_SSP_MMC_DEBUG
	printf("MMC%d: Set clock rate to %d KHz (requested %d KHz)\n",
		mmc->block_dev.dev, tgtclk, clock);
#endif
}

static void ssp_mmc_set_ios(struct mmc *mmc)
{
	u32 regval;

	/* Set the clock speed */
	if (mmc->clock)
		set_bit_clock(mmc, mmc->clock);

	/* Set the bus width */
	regval = ssp_mmc_read(mmc, HW_SSP_CTRL0);
	regval &= ~BM_SSP_CTRL0_BUS_WIDTH;
	switch (mmc->bus_width) {
	case 1:
		regval |= (BV_SSP_CTRL0_BUS_WIDTH__ONE_BIT <<
				BP_SSP_CTRL0_BUS_WIDTH);
		break;
	case 4:
		regval |= (BV_SSP_CTRL0_BUS_WIDTH__FOUR_BIT <<
				BP_SSP_CTRL0_BUS_WIDTH);
		break;
	case 8:
		regval |= (BV_SSP_CTRL0_BUS_WIDTH__EIGHT_BIT <<
				BP_SSP_CTRL0_BUS_WIDTH);
	}
	ssp_mmc_write(mmc, HW_SSP_CTRL0, regval);

#ifdef IMX_SSP_MMC_DEBUG
	printf("MMC%d: Set %d bits bus width\n",
		mmc->block_dev.dev, mmc->bus_width);
#endif
}

static int ssp_mmc_init(struct mmc *mmc)
{
	struct imx_ssp_mmc_cfg *cfg = (struct imx_ssp_mmc_cfg *)mmc->priv;
	u32 regval;

	/*
	 * Set up SSPCLK
	 */
	/* Set REF_IO0 at 297.731 MHz */
	regval = REG_RD(REGS_CLKCTRL_BASE, HW_CLKCTRL_FRAC0);
	regval &= ~BM_CLKCTRL_FRAC0_IO0FRAC;
	REG_WR(REGS_CLKCTRL_BASE, HW_CLKCTRL_FRAC0,
		regval | (29 << BP_CLKCTRL_FRAC0_IO0FRAC));
	/* Enable REF_IO0 */
	REG_CLR(REGS_CLKCTRL_BASE, HW_CLKCTRL_FRAC0,
		BM_CLKCTRL_FRAC0_CLKGATEIO0);

	/* Source SSPCLK from REF_IO0 */
	REG_CLR(REGS_CLKCTRL_BASE, HW_CLKCTRL_CLKSEQ,
		cfg->clkctrl_clkseq_ssp_offset);
	/* Turn on SSPCLK */
	REG_WR(REGS_CLKCTRL_BASE, cfg->clkctrl_ssp_offset,
		REG_RD(REGS_CLKCTRL_BASE, cfg->clkctrl_ssp_offset) &
		~BM_CLKCTRL_SSP_CLKGATE);
	/* Set SSPCLK divide 1 */
	regval = REG_RD(REGS_CLKCTRL_BASE, cfg->clkctrl_ssp_offset);
	regval &= ~(BM_CLKCTRL_SSP_DIV_FRAC_EN | BM_CLKCTRL_SSP_DIV);
	REG_WR(REGS_CLKCTRL_BASE, cfg->clkctrl_ssp_offset,
		regval | (1 << BP_CLKCTRL_SSP_DIV));
	/* Wait for new divide ready */
	do {
		udelay(10);
	} while (REG_RD(REGS_CLKCTRL_BASE, cfg->clkctrl_ssp_offset) &
		BM_CLKCTRL_SSP_BUSY);

	/* Prepare for software reset */
	ssp_mmc_write(mmc, HW_SSP_CTRL0_CLR, BM_SSP_CTRL0_SFTRST);
	ssp_mmc_write(mmc, HW_SSP_CTRL0_CLR, BM_SSP_CTRL0_CLKGATE);
	/* Assert reset */
	ssp_mmc_write(mmc, HW_SSP_CTRL0_SET, BM_SSP_CTRL0_SFTRST);
	/* Wait for confirmation */
	while (!(ssp_mmc_read(mmc, HW_SSP_CTRL0) & BM_SSP_CTRL0_CLKGATE))
		;
	/* Done */
	ssp_mmc_write(mmc, HW_SSP_CTRL0_CLR, BM_SSP_CTRL0_SFTRST);
	ssp_mmc_write(mmc, HW_SSP_CTRL0_CLR, BM_SSP_CTRL0_CLKGATE);

	/* 8 bits word length in MMC mode */
	regval = ssp_mmc_read(mmc, HW_SSP_CTRL1);
	regval &= ~(BM_SSP_CTRL1_SSP_MODE | BM_SSP_CTRL1_WORD_LENGTH);
	ssp_mmc_write(mmc, HW_SSP_CTRL1, regval |
		(BV_SSP_CTRL1_SSP_MODE__SD_MMC << BP_SSP_CTRL1_SSP_MODE) |
		(BV_SSP_CTRL1_WORD_LENGTH__EIGHT_BITS <<
			BP_SSP_CTRL1_WORD_LENGTH));

	/* Set initial bit clock 400 KHz */
	set_bit_clock(mmc, 400000);

	/* Send initial 74 clock cycles (185 us @ 400 KHz)*/
	ssp_mmc_write(mmc, HW_SSP_CMD0_SET, BM_SSP_CMD0_CONT_CLKING_EN);
	udelay(200);
	ssp_mmc_write(mmc, HW_SSP_CMD0_CLR, BM_SSP_CMD0_CONT_CLKING_EN);

	return 0;
}

int imx_ssp_mmc_initialize(bd_t *bis, struct imx_ssp_mmc_cfg *cfg)
{
	struct mmc *mmc;

	mmc = malloc(sizeof(struct mmc));
	sprintf(mmc->name, "IMX_SSP_MMC");
	mmc->send_cmd = ssp_mmc_send_cmd;
	mmc->set_ios = ssp_mmc_set_ios;
	mmc->init = ssp_mmc_init;
	mmc->priv = cfg;

	mmc->voltages = MMC_VDD_32_33 | MMC_VDD_31_32 | MMC_VDD_30_31 |
			MMC_VDD_29_30 | MMC_VDD_28_29 | MMC_VDD_27_28;

	mmc->host_caps = MMC_MODE_4BIT | MMC_MODE_8BIT |
			 MMC_MODE_HS_52MHz | MMC_MODE_HS;

	/*
	 * SSPCLK = 480 * 18 / 29 / 1 = 297.731 MHz
	 * SSP bit rate = SSPCLK / (CLOCK_DIVIDE * (1 + CLOCK_RATE)),
	 * CLOCK_DIVIDE has to be an even value from 2 to 254, and
	 * CLOCK_RATE could be any integer from 0 to 255.
	 */
	mmc->f_min = 400000;
	mmc->f_max = 148000000;	/* 297.731 MHz / 2 */

	mmc_register(mmc);
	return 0;
}
