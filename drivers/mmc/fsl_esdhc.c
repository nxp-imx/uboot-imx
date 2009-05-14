/*
 * Copyright 2008-2009 Freescale Semiconductor, Inc.
 *
 * See file	CREDITS	for	list of	people who contributed to this
 * project.
 *
 * This	program	is free	software; you can redistribute it and/or
 * modify it under the terms of	the	GNU	General	Public License as
 * published by	the	Free Software Foundation; either version 2 of
 * the License,	or (at your	option)	any	later version.
 *
 * This	program	is distributed in the hope that	it will	be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A	PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received	a copy of the GNU General Public License
 * along with this program;	if not,	write to the Free Software
 * Foundation, Inc., 59	Temple Place, Suite	330, Boston,
 * MA 02111-1307 USA
 */

/*!
 * @file esdhc.c
 *
 * @brief source code for the mmc card operation
 *
 * @ingroup mmc
 */

#include <asm/arch/sdhc.h>
#include <linux/mmc/sdhci.h>
#include <asm/errno.h>
#include <common.h>
#include <linux/types.h>
#include <asm/io.h>

#ifdef CONFIG_MMC

#define RETRIES_TIMES 100

#define REG_WRITE_OR(val, reg) { \
		u32 temp = 0; \
		temp = readl(reg); \
		(temp) |= (val); \
		writel((temp), (reg)); \
		}

#define REG_WRITE_AND(val, reg) { \
		u32 temp = 0; \
		temp = readl(reg); \
		(temp) &= (val); \
		writel((temp), (reg)); \
		}

#define SDHC_DELAY_BY_100(x) { \
		u32 i; \
		for (i = 0; i < x; ++i) \
			udelay(100); \
		}


extern volatile u32 esdhc_base_pointer;

static void esdhc_cmd_config(esdhc_cmd_t *);
static u32 esdhc_check_response(void);
static u32 esdhc_wait_buf_rdy_intr(u32, u32);
static void esdhc_wait_op_done_intr(void);
static u32 esdhc_check_data(void);
static void esdhc_set_data_transfer_width(u32 data_transfer_width);
static u32 esdhc_poll_cihb_cdihb(data_present_select data_present);
static void esdhc_set_endianness(u32 endian_mode);
static void esdhc_clear_buf_rdy_intr(u32 mask);
static u32 esdhc_check_data_crc_status(void);

/*!
 * Send 80 SD clock to card and wait for INITA bit to get cleared.
 */
void interface_initialization_active(void)
{
	/* Send 80 clock ticks for card to power up */
	REG_WRITE_OR(ESDHC_SYSCTL_INITA, \
			esdhc_base_pointer + SDHCI_SYSTEM_CONTROL);

	/* Start a general purpose timer */
	udelay(ESDHC_CARD_INIT_TIMEOUT);
}

/*!
 * Execute a software reset and set data bus width for eSDHC.
 */
u32 interface_reset()
{
	u32 reset_status = 0;
	u32 u32Retries = 0;
	u32 u32Temp = 0;

	debug("Entry: interface_reset");

	/* Reset the entire host controller by writing
	1 to RSTA bit of SYSCTRL Register */
	REG_WRITE_OR(ESDHC_SOFTWARE_RESET, \
			esdhc_base_pointer + SDHCI_SYSTEM_CONTROL);

	/* Start a general purpose timer (3 millsec delay) */
	/* udelay(ESDHC_OPER_TIMEOUT); */

	/* Wait for clearance of CIHB and CDIHB Bits */
	for (u32Retries = RETRIES_TIMES; u32Retries > 0; --u32Retries) {
		if (!is_soc_rev(CHIP_REV_1_0)) {
			if (readl(esdhc_base_pointer + SDHCI_PRESENT_STATE) \
						& ESDHC_CMD_INHIBIT) {
				reset_status = 1;
			} else {
				reset_status = 0;
				break;
			}
		} else if (!is_soc_rev(CHIP_REV_2_0)) {
			if (readl(esdhc_base_pointer + SDHCI_SYSTEM_CONTROL) \
						& ESDHC_SOFTWARE_RESET) {
				reset_status = 1;
			} else {
				reset_status = 0;
				break;
			}
		}
	}

	if (!is_soc_rev(CHIP_REV_1_0)) {
		/* send 80 clock ticks for card to power up */
		REG_WRITE_OR(ESDHC_SYSCTL_INITA, \
			esdhc_base_pointer + SDHCI_SYSTEM_CONTROL);
	}

	/* Set data bus width of ESDCH */
	esdhc_set_data_transfer_width(0x00000000);

	/* Set Endianness of ESDHC */
	esdhc_set_endianness(0x00000020);

	/* set data timeout delay to max */
	u32Temp = (readl(esdhc_base_pointer + SDHCI_SYSTEM_CONTROL) & \
						0xfff0ffff) | 0x000e0000;
	writel(u32Temp, esdhc_base_pointer + SDHCI_SYSTEM_CONTROL);

	return reset_status;
}

/*!
 * Clear interrupts at eSDHC level.
 */
void interface_clear_interrupt(void)
{
	/* Clear Interrupt status register */
	writel(ESDHC_CLEAR_INTERRUPT, esdhc_base_pointer + SDHCI_INT_STATUS);
}

/*!
 * Enable Clock and set operating frequency.
 */
void interface_configure_clock(sdhc_freq_t frequency)
{
	u32 ident_freq = 0;
	u32 oper_freq  = 0;

	if (!is_soc_rev(CHIP_REV_1_0)) {
		/* Enable ipg_perclk, HCLK enable and IPG Clock enable. */
		REG_WRITE_OR(ESDHC_CLOCK_ENABLE, \
			esdhc_base_pointer + SDHCI_SYSTEM_CONTROL);
		/* Clear DTOCV SDCLKFS bits */
		REG_WRITE_OR(ESDHC_FREQ_MASK, \
			esdhc_base_pointer + SDHCI_SYSTEM_CONTROL);
		ident_freq = ESDHC_SYSCTL_IDENT_FREQ_TO1;
		oper_freq  = ESDHC_SYSCTL_OPERT_FREQ_TO1;
	} else if (!is_soc_rev(CHIP_REV_2_0)) {
		/* Clear SDCLKEN bit */
		REG_WRITE_OR((~ESDHC_SYSCTL_SDCLKEN_MASK), \
			esdhc_base_pointer + SDHCI_SYSTEM_CONTROL);

		/* Clear DTOCV, SDCLKFS, DVFS bits */
		REG_WRITE_OR((~ESDHC_SYSCTL_FREQ_MASK), \
			esdhc_base_pointer + SDHCI_SYSTEM_CONTROL);
		ident_freq = ESDHC_SYSCTL_IDENT_FREQ_TO2;
		oper_freq  = ESDHC_SYSCTL_OPERT_FREQ_TO2;
	}

	if (!is_soc_rev(CHIP_REV_2_0)) {
		/* Disable the PEREN, HCKEN and IPGEN */
		REG_WRITE_OR((~ESDHC_SYSCTL_INPUT_CLOCK_MASK), \
			esdhc_base_pointer + SDHCI_SYSTEM_CONTROL);
	}

	if (frequency == IDENTIFICATION_FREQ) {
		/* Input frequecy to eSDHC is 36 MHZ */
		/* PLL3 is the source of input frequency*/
		/*Set DTOCV and SDCLKFS bit to get SD_CLK
		of frequency below 400 KHZ (70.31 KHZ) */
		REG_WRITE_OR(ident_freq, \
			esdhc_base_pointer + SDHCI_SYSTEM_CONTROL);
	} else if (frequency == OPERATING_FREQ) {
		/*Set DTOCV and SDCLKFS bit to get SD_CLK
		of frequency around 25 MHz.(18 MHz)*/
		REG_WRITE_OR(oper_freq, \
			esdhc_base_pointer + SDHCI_SYSTEM_CONTROL);
	}

	if (!is_soc_rev(CHIP_REV_2_0)) {
		/* Start a general purpose timer */
		/* Wait for clock to be stable */
		SDHC_DELAY_BY_100(96);

		/* Set SDCLKEN bit to enable clock */
		REG_WRITE_OR(ESDHC_SYSCTL_SDCLKEN_MASK, \
			esdhc_base_pointer + SDHCI_SYSTEM_CONTROL);

		/* Mask Data Timeout Error Status Enable Interrupt (DTOESEN) */
		REG_WRITE_AND((~ESDHC_IRQSTATEN_DTOESEN), \
			esdhc_base_pointer + SDHCI_INT_ENABLE);

		/* Set the Data Timeout Counter Value(DTOCV) */
		REG_WRITE_OR(ESDHC_SYSCTL_DTOCV_VAL, \
			esdhc_base_pointer + SDHCI_SYSTEM_CONTROL);

		/* Enable Data Timeout Error Status
			Enable Interrupt (DTOESEN) */
		REG_WRITE_OR(ESDHC_IRQSTATEN_DTOESEN, \
			esdhc_base_pointer + SDHCI_INT_ENABLE);
	}
}

/*!
 * Set data transfer width for e-SDHC.
 */
static void esdhc_set_data_transfer_width(u32 data_transfer_width)
{

	/* Set DWT bit of protocol control register according to bus_width */
	if (!is_soc_rev(CHIP_REV_2_0))
		REG_WRITE_AND((~ESDHC_BUS_WIDTH_MASK), \
			esdhc_base_pointer + SDHCI_HOST_CONTROL);

	REG_WRITE_OR((data_transfer_width), \
			esdhc_base_pointer + SDHCI_HOST_CONTROL);
}

/*!
 * Set endianness mode for e-SDHC.
 */
static void esdhc_set_endianness(u32 endian_mode)
{
	if (!is_soc_rev(CHIP_REV_2_0)) {
		REG_WRITE_AND((~ESDHC_ENDIAN_MODE_MASK), \
			esdhc_base_pointer + SDHCI_HOST_CONTROL);
	}
	/* Set DWT bit of protocol control register according to bus_width */
	REG_WRITE_OR((endian_mode), \
			esdhc_base_pointer + SDHCI_HOST_CONTROL);
}

/*!
 * Poll the CIHB & CDIHB bits of the present
 * state register and wait until it goes low.
 */
static u32 esdhc_poll_cihb_cdihb(data_present_select data_present)
{
	u32 init_status = 0;
	u32 u32Retries = 0;

	/* Start a general purpose timer */
	for (u32Retries = RETRIES_TIMES; u32Retries > 0; u32Retries--) {
		if (!(readl(esdhc_base_pointer + SDHCI_PRESENT_STATE) & \
					ESDHC_PRESENT_STATE_CIHB)) {
			init_status = 0;
			break;
		}
		SDHC_DELAY_BY_100(10);
	}

	/*
	 * Wait for the data line to be free (poll the CDIHB bit of
	 * the present state register).
	 */
	if ((0 == init_status) && (data_present == DATA_PRESENT)) {
		/* Start a general purpose timer */
		SDHC_DELAY_BY_100(32);

		if (readl(esdhc_base_pointer + SDHCI_PRESENT_STATE) & \
			ESDHC_PRESENT_STATE_CDIHB) {
			init_status =  1;
		}
	}

	return init_status;
}

/*!
 * Wait until the command and data lines are free.
 */
u32 interface_wait_cmd_data_lines(data_present_select data_present)
{
	u32 cmd_status = 0;

	cmd_status = esdhc_poll_cihb_cdihb(data_present);

	return cmd_status;
}

u32 interface_set_bus_width(u32 bus_width)
{
	u32 tmp;

	tmp = readl(esdhc_base_pointer + SDHCI_HOST_CONTROL);

	tmp &= ~SDHCI_CTRL_8BITBUS;
	tmp |= SDHCI_CTRL_4BITBUS;

	writel(tmp, esdhc_base_pointer + SDHCI_HOST_CONTROL);

	return 0;
}

/*!
 * Execute a command and wait for the response.
 */
u32 interface_send_cmd_wait_resp(esdhc_cmd_t *cmd)
{
	u32 cmd_status = 0;

	/* Clear Interrupt status register */
	writel(ESDHC_CLEAR_INTERRUPT, \
			esdhc_base_pointer + SDHCI_INT_STATUS);

	/* Enable Interrupt */
	REG_WRITE_OR(ESDHC_INTERRUPT_ENABLE, \
			esdhc_base_pointer + SDHCI_INT_ENABLE);

	if (!is_soc_rev(CHIP_REV_2_0)) {
		cmd_status = interface_wait_cmd_data_lines(cmd->data_present);

		if (cmd_status == 1)
			return 1;
	}

	/* Configure Command */
	esdhc_cmd_config(cmd);

	/* Wait for interrupt CTOE or CC */
	SDHC_DELAY_BY_100(96);

	/* Mask all interrupts */
	writel(0, esdhc_base_pointer + SDHCI_SIGNAL_ENABLE);

	/* Check if an error occured */
	return esdhc_check_response();
}

/*!
 * Configure ESDHC registers for sending a command to MMC.
 */
static void esdhc_cmd_config(esdhc_cmd_t *cmd)
{
	u32 u32Temp = 0;

	/* Write Command Argument in Command Argument Register */
	writel(cmd->arg, esdhc_base_pointer + SDHCI_ARGUMENT);

	/*
	*Configure e-SDHC Register value according to Command
	*/
	u32Temp = \
		(((cmd->data_transfer)<<ESDHC_DATA_TRANSFER_SHIFT) |
		((cmd->response_format)<<ESDHC_RESPONSE_FORMAT_SHIFT) |
		((cmd->data_present)<<ESDHC_DATA_PRESENT_SHIFT) |
		((cmd->crc_check) << ESDHC_CRC_CHECK_SHIFT) |
		((cmd->cmdindex_check) << ESDHC_CMD_INDEX_CHECK_SHIFT) |
		((cmd->command) << ESDHC_CMD_INDEX_SHIFT) |
		((cmd->block_count_enable_check) << \
			ESDHC_BLOCK_COUNT_ENABLE_SHIFT) |
		((cmd->multi_single_block) << \
			ESDHC_MULTI_SINGLE_BLOCK_SELECT_SHIFT));

	writel(u32Temp, esdhc_base_pointer + SDHCI_TRANSFER_MODE);
}

/*!
 * Wait a END_CMD_RESP interrupt by interrupt status register.
 * e-SDHC sets this bit after receving command response.
 */
static u32 esdhc_check_response(void)
{
	u32 status = 1;

	/* Check whether the interrupt is an END_CMD_RESP
	* or a response time out or a CRC error
	*/
	if ((readl(esdhc_base_pointer + SDHCI_INT_STATUS) & \
		ESDHC_STATUS_END_CMD_RESP_MSK) &&
		!(readl(esdhc_base_pointer + SDHCI_INT_STATUS) & \
		ESDHC_STATUS_TIME_OUT_RESP_MSK) &&
		!(readl(esdhc_base_pointer + SDHCI_INT_STATUS) & \
		ESDHC_STATUS_RESP_CRC_ERR_MSK) &&
		!(readl(esdhc_base_pointer + SDHCI_INT_STATUS) & \
		ESDHC_STATUS_RESP_CMD_INDEX_ERR_MSK))
		status = 0;

	return status;
}

/*!
 * This function will read response from e-SDHC
 * register according to reponse format.
 */
void interface_read_response(esdhc_resp_t *cmd_resp)
{
	/* get response values from e-SDHC CMDRSP registers.*/
	cmd_resp->cmd_rsp0 = (u32)readl(esdhc_base_pointer + SDHCI_RESPONSE);
	cmd_resp->cmd_rsp1 = (u32)readl(esdhc_base_pointer + \
							SDHCI_RESPONSE + 4);
	cmd_resp->cmd_rsp2 = (u32)readl(esdhc_base_pointer + \
							SDHCI_RESPONSE + 8);
	cmd_resp->cmd_rsp3 = (u32)readl(esdhc_base_pointer + \
							SDHCI_RESPONSE + 12);
}

/*!
 * This function will read response from e-SDHC register
 * according to reponse format.
 */
u32 interface_data_read(u32 *dest_ptr, u32 blk_len)
{
	u32 i = 0;
	u32 j = 0;
	u32 status = 1;
	u32 *tmp_ptr = dest_ptr;

	debug("Entry: interface_data_read()\n");

	/* Enable Interrupt */
	REG_WRITE_OR(ESDHC_INTERRUPT_ENABLE, \
			esdhc_base_pointer + SDHCI_INT_ENABLE);

	for (i = 0; i < (blk_len) / (ESDHC_FIFO_SIZE * 4); ++i) {
		/* Wait for BRR bit to be set */
		status = esdhc_wait_buf_rdy_intr(ESDHC_STATUS_BUF_READ_RDY_MSK,
						 ESDHC_READ_DATA_TIME_OUT);

		debug("esdhc_wait_buf_rdy_intr: %d\n", status);

		if (!status) {
			for (j = 0; j < ESDHC_FIFO_SIZE; ++j) {
				*tmp_ptr++ = \
				readl(esdhc_base_pointer + SDHCI_BUFFER);
			}
			if (!is_soc_rev(CHIP_REV_2_0)) {
				/* Clear the BRR */
				esdhc_clear_buf_rdy_intr(ESDHC_STATUS_BUF_READ_RDY_MSK);
			}
		} else {
			debug("esdhc_wait_buf_rdy_intr failed\n");
			break;
		}
	}

	esdhc_wait_op_done_intr();

	status = esdhc_check_data();

	if (!is_soc_rev(CHIP_REV_2_0) && !status)
		status = 0;

	debug("esdhc_check_data: %d\n", status);

	debug("Exit: interface_data_read()\n");

	return status;
}

/*!
 * Wait a BUF_READ_READY  interrupt by pooling STATUS register.
 */
static u32 esdhc_wait_buf_rdy_intr(u32 mask, u32 multi_single_block)
{
	u32 status = 0;
	u32 u32Retries = 0;

	/* Wait interrupt (BUF_READ_RDY)
	*/

	for (u32Retries = RETRIES_TIMES; u32Retries > 0; --u32Retries) {
		if (!(readl(esdhc_base_pointer + SDHCI_INT_STATUS) & mask)) {
			status = 1;
		} else {
			status = 0;
			break;
		}
		SDHC_DELAY_BY_100(10);
	}

	if (multi_single_block == MULTIPLE && \
		readl(esdhc_base_pointer + SDHCI_INT_STATUS) & mask)
		REG_WRITE_OR(mask, (esdhc_base_pointer + SDHCI_INT_STATUS));

	return status;
}

/*!
 * Clear BUF_READ_READY/BUF_WRITE_READY interrupt
 * by writing 1 to STATUS register.
 */
static void esdhc_clear_buf_rdy_intr(u32 mask)
{
	writel(mask, (esdhc_base_pointer + SDHCI_INT_STATUS));
}

/*!
 * Wait for TC, DEBE, DCE or DTOE by polling Interrupt STATUS register.
 */
static void esdhc_wait_op_done_intr(void)
{
	while (!(readl(esdhc_base_pointer + SDHCI_INT_STATUS) & \
			ESDHC_STATUS_TRANSFER_COMPLETE_MSK))
	;
}

/*!
 * If READ_OP_DONE occured check ESDHC_STATUS_TIME_OUT_READ
 * and RD_CRC_ERR_CODE and
 * to determine if an error occured
 */
static u32 esdhc_check_data(void)
{

	u32 status = 1;

	debug("Entry: esdhc_check_data()\n");

	/* Check whether the interrupt is an OP_DONE
	* or a data time out or a CRC error
	*/
	if ((readl(esdhc_base_pointer + SDHCI_INT_STATUS) & \
		ESDHC_STATUS_TRANSFER_COMPLETE_MSK) &&
		!(readl(esdhc_base_pointer + SDHCI_INT_STATUS) & \
		ESDHC_STATUS_TIME_OUT_READ_MASK) &&
		!(readl(esdhc_base_pointer + SDHCI_INT_STATUS) & \
		ESDHC_STATUS_READ_CRC_ERR_MSK)) {
		if (!is_soc_rev(CHIP_REV_2_0)) {
			writel(ESDHC_STATUS_TRANSFER_COMPLETE_MSK, \
				(esdhc_base_pointer + SDHCI_INT_STATUS));
		}
		status = 0;
	} else {
		status = 1;
	}

	debug("Exit: esdhc_check_data()\n");

	return status;
}

/*!
 * Check for Data timeout error, data CRC error and data end bit error
 * to determine if an error occured.
 */
static u32 esdhc_check_data_crc_status(void)
{
	u32 status = 1;

	/* Check whether the interrupt is DTOE/DCE/DEBE */
	if (!(readl(esdhc_base_pointer + SDHCI_INT_STATUS) & \
		ESDHC_STATUS_TIME_OUT_READ_MASK) &&
		!(readl(esdhc_base_pointer + SDHCI_INT_STATUS) & \
		ESDHC_STATUS_READ_CRC_ERR_MSK) &&
		!(readl(esdhc_base_pointer + SDHCI_INT_STATUS) & \
		ESDHC_STATUS_RW_DATA_END_BIT_ERR_MSK)) {
		status = 0;
	} else {
		status = 1;
	}

	return status;
}

/*!
 * Set Block length.
 */
void interface_config_block_info(u32 blk_len, u32 nob, u32 wml)
{
	/* Configre block Attributes register */
	writel(((nob << ESDHC_BLOCK_SHIFT) | blk_len), \
			(esdhc_base_pointer + SDHCI_BLOCK_SIZE));

	/* Set Read Water MArk Level register */
	writel(wml, esdhc_base_pointer + SDHCI_WML_LEV);
}

/*!
 * This function will write data to device  attached to interface.
 */
u32 interface_data_write(u32 *dest_ptr, u32 blk_len)
{
	u32 i = 0;
	u32 j = 0;
	u32 status = 1;
	u32 *tmp_ptr = dest_ptr;

	debug("Entry: interface_data_write()\n");

	/* Enable Interrupt */
	REG_WRITE_OR(ESDHC_INTERRUPT_ENABLE, \
			(esdhc_base_pointer + SDHCI_INT_ENABLE));

	for (i = 0; i < (blk_len) / (ESDHC_FIFO_SIZE * 4); ++i) {
		/* Wait for BWR bit to be set */
		esdhc_wait_buf_rdy_intr(ESDHC_STATUS_BUF_WRITE_RDY_MSK, \
						SINGLE);

		for (j = 0; j < ESDHC_FIFO_SIZE; ++j) {
			writel((*tmp_ptr), esdhc_base_pointer + SDHCI_BUFFER);
			++tmp_ptr;
		}
		esdhc_clear_buf_rdy_intr(ESDHC_STATUS_BUF_WRITE_RDY_MSK);
	}

	/* Wait for transfer complete operation interrupt */
	esdhc_wait_op_done_intr();

	/* Check for status errors */
	status = esdhc_check_data();

	debug("Exit: interface_data_write()\n");

	return status;
}

/*!
 * Configure the CMD line PAD configuration for strong or weak pull-up.
 */
/*
void esdhc_set_cmd_pullup(esdhc_pullup_t pull_up)
{
	u32 interface_esdhc = 0;
	u32 pad_val = 0;

	interface_esdhc = (readl(0x53ff080c)) & (0x000000C0) >> 6;

	if (pull_up == STRONG) {
		pad_val = PAD_CTL_PUE_PUD | PAD_CTL_PKE_ENABLE |
			PAD_CTL_HYS_SCHMITZ | PAD_CTL_DRV_HIGH |
			PAD_CTL_22K_PU | PAD_CTL_SRE_FAST;
	} else {
		pad_val = PAD_CTL_PUE_PUD | PAD_CTL_PKE_ENABLE |
			PAD_CTL_HYS_SCHMITZ | PAD_CTL_DRV_MAX |
			PAD_CTL_100K_PU | PAD_CTL_SRE_FAST;
	}

	switch (interface_esdhc) {
	case ESDHC1:
		mxc_iomux_set_pad(MX51_PIN_SD1_CMD, pad_val);
		break;
	case ESDHC2:
		mxc_iomux_set_pad(MX51_PIN_SD2_CMD, pad_val);
		break;
	case ESDHC3:
	default:
		break;
	}
}
*/

#endif
