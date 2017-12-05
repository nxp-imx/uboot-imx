// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2017-2018,2020 NXP
 *
 */

#include <common.h>
#include <linux/errno.h>
#include <spi.h>
#include "sja1105_ll.h"

#ifndef CONFIG_DEFAULT_SPI_BUS
#   define CONFIG_DEFAULT_SPI_BUS	0
#endif

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

#define SJA_DSPI_MODE	(SPI_CPHA | SPI_FMSZ_16)
#define SJA_DSPI_HZ	5000

#define sja_debug(fmt, ...)	debug("[SJA1105]%s:%d " fmt, __func__, \
				__LINE__, ##__VA_ARGS__)

#define NUM_MAC_LVL_COUNTERS1 4
static char *mac_lvl_counters1[NUM_MAC_LVL_COUNTERS1] = {
	"N_RUNT         ",
	"N_SOFERR       ",
	"N_ALIGNERR     ",
	"N_MIIERR       ",
};

#define NUM_MAC_LVL_COUNTERS2 10
static char *mac_lvl_counters2[NUM_MAC_LVL_COUNTERS2] = {
	"RSVD           ",
	"SPCERRS        ",
	"DRN664ERRS     ",
	"RSVD           ",
	"BAGDROP        ",
	"LENDROPS       ",
	"PORTDROPS      ",
	"RSVD           ",
	"SPCPRIOR       ",
	"RSVD           ",
};

#define NUM_ETH_HIGH_LVL_COUNTERS1 16
static char *eth_high_lvl_counters1[NUM_ETH_HIGH_LVL_COUNTERS1] = {
	"N_TXBYTE       ",
	"N_TXBYTESH     ",
	"N_TXFRM        ",
	"N_TXFRMSH      ",
	"N_RXBYTE       ",
	"N_RXBYTESH     ",
	"N_RXFRM        ",
	"N_RXFRMSH      ",
	"N_POLERR       ",
	"RSVD           ",
	"RSVD           ",
	"N_CRCERR       ",
	"N_SIZERR       ",
	"RSVD           ",
	"N_VLANERR      ",
	"N_N664ERR      ",
};

#define NUM_ETH_HIGH_LVL_COUNTERS2 4
static char *eth_high_lvl_counters2[NUM_ETH_HIGH_LVL_COUNTERS2] = {
	"N_NOT_REACH    ",
	"N_ERG_DISABLED ",
	"N_PART_DROP    ",
	"N_QFULL        ",
};

struct sja_parms {
	u32 bus;
	u32 cs;
	u32 devid;
	u32 bin_len;
	u8 *cfg_bin;
};

static int sja1105_write(struct sja_parms *sjap, u32 *cmd, u8 nb_words)
{
	struct spi_slave *slave;
	int bitlen = (nb_words << 3) << 2;
	int ret = 0;

	slave = spi_setup_slave(sjap->bus, sjap->cs, SJA_DSPI_HZ,
				SJA_DSPI_MODE);
	if (!slave) {
		printf("Invalid device %d:%d\n", sjap->bus, sjap->cs);
		return -EINVAL;
	}

	ret = spi_claim_bus(slave);
	if (ret) {
		printf("Error %d while claiming bus\n", ret);
		goto done;
	}

	ret = spi_xfer(slave, bitlen, cmd, NULL,
		       SPI_XFER_BEGIN | SPI_XFER_END);

	if (ret)
		printf("Error %d during SPI transaction\n", ret);

done:
	spi_release_bus(slave);
	spi_free_slave(slave);

	return ret;
}

static int sja1105_cfg_block_write(struct sja_parms *sjap, u32 reg_addr,
				   u32 *data, int nb_words)
{
	u32 cmd[SJA1105_CONFIG_WORDS_PER_BLOCK + 1], upper, down;
	int i = 0;

	cmd[0] = cpu_to_le32 (CMD_ENCODE_RWOP(CMD_WR_OP) |
			      CMD_ENCODE_ADDR(reg_addr));
	upper = (cmd[0] & 0x0000FFFF) << 16;
	down = (cmd[0] & 0xFFFF0000) >> 16;
	cmd[0] = upper | down;

	while (i < nb_words) {
		cmd[i + 1] = *data++;
		upper = (cmd[i + 1] & 0x0000FFFF) << 16;
		down = (cmd[i + 1] & 0xFFFF0000) >> 16;
		cmd[i + 1] = upper | down;
		sja_debug("config write 0x%08x\n", cmd[i + 1]);
		i++;
	}

	return sja1105_write(sjap, cmd, nb_words + 1);
}

static u32 sja1105_read_reg32(struct sja_parms *sjap, u32 reg_addr)
{
	u32 cmd[2], resp[2], upper, down;
	struct spi_slave *slave;
	int bitlen = sizeof(cmd) << 3;
	int rc;

	sja_debug("reading 4bytes @0x%08x tlen %d t.bits_per_word %d\n",
		  reg_addr, 8, 64);

	slave = spi_setup_slave(sjap->bus, sjap->cs, SJA_DSPI_HZ,
				SJA_DSPI_MODE);
	if (!slave) {
		printf("Invalid device %d:%d\n", sjap->bus, sjap->cs);
		return -EINVAL;
	}

	rc = spi_claim_bus(slave);
	if (rc)
		goto done;

	cmd[0] = cpu_to_le32 (CMD_ENCODE_RWOP(CMD_RD_OP) |
		CMD_ENCODE_ADDR(reg_addr) | CMD_ENCODE_WRD_CNT(1));
	cmd[1] = 0;

	upper = (cmd[0] & 0x0000FFFF) << 16;
	down = (cmd[0] & 0xFFFF0000) >> 16;
	cmd[0] = upper | down;

	rc = spi_xfer(slave, bitlen, cmd, resp,
		      SPI_XFER_BEGIN | SPI_XFER_END);
	if (rc)
		printf("Error %d during SPI transaction\n", rc);
	spi_release_bus(slave);
	spi_free_slave(slave);

	upper = (resp[1] & 0x0000FFFF) << 16;
	down = (resp[1] & 0xFFFF0000) >> 16;
	resp[1] = upper | down;

	return le32_to_cpu(resp[1]);
done:
	return rc;
}

static u32 sja1105_write_reg32(struct sja_parms *sjap, u32 reg_addr, u32 val)
{
	u32 cmd[2], resp[2], upper, down;
	struct spi_slave *slave;
	int bitlen = sizeof(cmd) << 3;
	int rc;

	sja_debug("writing 4bytes @0x%08x tlen %d t.bits_per_word %d\n",
		  reg_addr, 8, 64);

	slave = spi_setup_slave(sjap->bus, sjap->cs, SJA_DSPI_HZ,
				SJA_DSPI_MODE);
	if (!slave) {
		printf("Invalid device %d:%d\n", sjap->bus, sjap->cs);
		return -EINVAL;
	}

	rc = spi_claim_bus(slave);
	if (rc)
		goto done;

	cmd[0] = cpu_to_le32 (CMD_ENCODE_RWOP(CMD_WR_OP) |
		CMD_ENCODE_ADDR(reg_addr) | CMD_ENCODE_WRD_CNT(1));
	upper = (cmd[0] & 0x0000FFFF) << 16;
	down = (cmd[0] & 0xFFFF0000) >> 16;
	cmd[0] = upper | down;

	cmd[1] = val;
	upper = (cmd[1] & 0x0000FFFF) << 16;
	down = (cmd[1] & 0xFFFF0000) >> 16;
	cmd[1] = upper | down;

	rc = spi_xfer(slave, bitlen, cmd, resp, SPI_XFER_BEGIN | SPI_XFER_END);
	if (rc)
		printf("Error %d during SPI transaction\n", rc);
	spi_release_bus(slave);
	spi_free_slave(slave);

	upper = (resp[1] & 0x0000FFFF) << 16;
	down = (resp[1] & 0xFFFF0000) >> 16;
	resp[1] = upper | down;

	return le32_to_cpu(resp[1]);
done:
	return rc;
}

static bool sja1105_check_device_status(struct sja_parms *sjap,
					bool expected_status,
					bool *pstatus)
{
	u32 status;
	u32 expected_val = expected_status ? SJA1105_BIT_STATUS_CONFIG_DONE : 0;
	bool ret = true;
	u32 error;

	status = sja1105_read_reg32(sjap, SJA1105_REG_STATUS);

	/* Check status is valid: check if any error bit is set */
	error = SJA1105_BIT_STATUS_CRCCHKL |
		 SJA1105_BIT_STATUS_DEVID_MATCH |
		 SJA1105_BIT_STATUS_CRCCHKG;
	if (status & error) {
		sja_debug("Error: SJA1105_REG_STATUS=0x%08x - LocalCRCfail=%d - DevID unmatched=%d, GlobalCRCfail=%d\n",
			  status,
			  (int)(status & SJA1105_BIT_STATUS_CRCCHKL),
			  (int)(status & SJA1105_BIT_STATUS_DEVID_MATCH),
			  (int)(status & SJA1105_BIT_STATUS_CRCCHKG));
		return false;
	}

	*pstatus = (expected_val == (status & SJA1105_BIT_STATUS_CONFIG_DONE));

	if (expected_status && !*pstatus)
		ret = false;

	return ret;
}

static int sja1105_check_device_id(struct sja_parms *sjap)
{
	return sja1105_read_reg32(sjap, SJA1105_REG_DEVICEID);
}

bool sja1105_post_cfg_load_check(struct sja_parms *sjap)
{
	u32 chip_id;
	bool status;

	/* Trying to read back the SJA1105 status via SPI... */
	chip_id  = sja1105_check_device_id(sjap);
	if (sjap->devid != chip_id)
		return false;
	if (!sja1105_check_device_status(sjap, true, &status))
		return false;

	return status;
}

void sja1105_port_cfg(struct sja_parms *sjap)
{
	u32 i;

	for (i = 0; i < SJA1105_PORT_NB; i++) {
		u32 port_status;

		/* Get port type / speed */
		port_status = sja1105_read_reg32(sjap,
					SJA1105_PORT_STATUS_MII_PORT(i));

		switch (port_status & SJA1105_PORT_STATUS_MII_MODE) {
		case e_mii_mode_rgmii:
			/* Set slew rate of TX Pins to high speed */
			sja1105_write_reg32(sjap,
					    SJA1105_CFG_PAD_MIIX_TX_PORT(i),
					    SJA1105_CFG_PAD_MIIX_TX_SLEW_RGMII);

			/* Set Clock delay */
			sja1105_write_reg32(sjap,
					    SJA1105_CFG_PAD_MIIX_ID_PORT(i),
					    SJA1105_CFG_PAD_MIIX_ID_RGMII);

			/* Disable IDIV */
			sja1105_write_reg32(sjap, SJA1105_CGU_IDIV_PORT(i),
					    SJA1105_CGU_IDIV_DISABLE);

			/* Set Clock source to PLL0 */
			sja1105_write_reg32(sjap,
					    SJA1105_CGU_MII_TX_CLK_PORT(i),
					    SJA1105_CGU_MII_CLK_SRC_PLL0);
			break;

		default:
			break;
		}
	}
}

static int sja1105_configuration_load(struct sja_parms *sjap)
{
	int remaining_words;
	int nb_words;
	u32 *data;
	u32 dev_addr;
	u32 val;
	bool swap_required;
	int i;

	if (!sjap->cfg_bin) {
		printf("Error: SJA1105 Switch configuration is NULL\n");
		return -EINVAL;
	}

	if (!sjap->bin_len) {
		printf("Error: SJA1105 Switch configuration is empty\n");
		return -EINVAL;
	}

	if (sjap->bin_len % 4 != 0) {
		printf("Error: SJA1105 Switch configuration is not valid\n");
		return -EINVAL;
	}

	data = (u32 *)&sjap->cfg_bin[0];

	nb_words = (sjap->bin_len >> 2);

	val = data[0];

	if (val == __builtin_bswap32(sjap->devid)) {
		printf("Config bin requires swap, incorrect endianness\n");
		swap_required = true;
	} else if (val == sjap->devid) {
		swap_required = false;
	} else {
		printf("Error: SJA1105 unhandled revision Switch incompatible configuration file (%x - %x)\n",
		       val, sjap->devid);
		return -EINVAL;
	}

	if (swap_required)
		for (i = 0; i < nb_words; i++) {
			val = data[i];
			data[i] = __builtin_bswap32(val);
		}

	sja_debug("swap_required %d nb_words %d dev_addr 0x%08x\n",
		  swap_required, nb_words, (u32)SJA1105_CONFIG_START_ADDRESS);

	remaining_words = nb_words;
	dev_addr = SJA1105_CONFIG_START_ADDRESS;

	i = 0;
	while (remaining_words > 0) {
		int block_size_words =
			MIN(SJA1105_CONFIG_WORDS_PER_BLOCK, remaining_words);

		sja_debug("block_size_words %d remaining_words %d\n",
			  block_size_words, remaining_words);

		if (sja1105_cfg_block_write(sjap, dev_addr, data,
					    block_size_words) < 0)
			return 1;

		sja_debug("Loaded block %d @0x%08x\n", i, dev_addr);

		dev_addr += block_size_words;
		data += block_size_words;
		remaining_words -= block_size_words;
		i++;

		if (i % 10 == 0)
			sja1105_post_cfg_load_check(sjap);
	}

	if (!sja1105_post_cfg_load_check(sjap)) {
		printf("SJA1105 configuration failed\n");
		return -ENXIO;
	}

	sja1105_port_cfg(sjap);

	return 0;
}

void sja1105_reset_ports(u32 cs, u32 bus)
{
	struct sja_parms sjap;
	int i, val;

	sjap.cs = cs;
	sjap.bus = bus;

	for (i = 0; i < SJA1105_PORT_NB; i++) {
		val = sja1105_read_reg32(&sjap,
					 SJA1105_CFG_PAD_MIIX_ID_PORT(i));

		/* Toggle RX Clock PullDown and Bypass */

		val |= SJA1105_CFG_PAD_MIIX_ID_RXC_PD;
		val |= SJA1105_CFG_PAD_MIIX_ID_RXC_BYPASS;

		sja1105_write_reg32(&sjap, SJA1105_CFG_PAD_MIIX_ID_PORT(i),
				    val);

		val &= ~SJA1105_CFG_PAD_MIIX_ID_RXC_PD;
		val &= ~SJA1105_CFG_PAD_MIIX_ID_RXC_BYPASS;

		sja1105_write_reg32(&sjap, SJA1105_CFG_PAD_MIIX_ID_PORT(i),
				    val);
	}
}

int sja1105_probe(u32 cs, u32 bus)
{
	struct sja_parms sjap;
	int ret = 0;

	memset(&sjap, 0, sizeof(struct sja_parms));

	sjap.cs = cs;
	sjap.bus = bus;

	sjap.devid = sja1105_check_device_id(&sjap);

	sja_debug("devid %X\n", sjap.devid);

	if (sja1105_post_cfg_load_check(&sjap)) {
		sja_debug("SJA1105 configuration already done. Skipping switch configuration\n");
		return 0;
	}

	printf("Loading SJA1105 firmware over SPI %d:%d\n", bus, cs);

	ret = sja1105_get_cfg(sjap.devid, sjap.cs, &sjap.bin_len,
			      &sjap.cfg_bin);

	if (ret) {
		printf("Error SJA1105 configuration not completed\n");
		return -EINVAL;
	}

	return sja1105_configuration_load(&sjap);
}

int do_sja_regs(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	u32 val32;
	char  *cp = 0;
	int i, j;
	struct sja_parms sjap;

	if (argc == 2) {
		sjap.bus = simple_strtoul(argv[1], &cp, 10);
		if (*cp == ':') {
			sjap.cs = simple_strtoul(cp + 1, &cp, 10);
		} else {
			sjap.cs = sjap.bus;
			sjap.bus = CONFIG_DEFAULT_SPI_BUS;
		}
	}

	printf("\nGeneral Status\n");
	val32 = sja1105_read_reg32(&sjap, SJA1105_REG_GENERAL_STATUS1);
	printf("general_status_1    = %08x\n", val32);
	val32 = sja1105_read_reg32(&sjap, SJA1105_REG_GENERAL_STATUS2);
	printf("general_status_2    = %08x\n", val32);
	val32 = sja1105_read_reg32(&sjap, SJA1105_REG_GENERAL_STATUS3);
	printf("general_status_3    = %08x\n", val32);
	val32 = sja1105_read_reg32(&sjap, SJA1105_REG_GENERAL_STATUS4);
	printf("general_status_4    = %08x\n", val32);
	val32 = sja1105_read_reg32(&sjap, SJA1105_REG_GENERAL_STATUS5);
	printf("general_status_5    = %08x\n", val32);
	val32 = sja1105_read_reg32(&sjap, SJA1105_REG_GENERAL_STATUS6);
	printf("general_status_6    = %08x\n", val32);
	val32 = sja1105_read_reg32(&sjap, SJA1105_REG_GENERAL_STATUS7);
	printf("general_status_7    = %08x\n", val32);
	val32 = sja1105_read_reg32(&sjap, SJA1105_REG_GENERAL_STATUS8);
	printf("general_status_8    = %08x\n", val32);
	val32 = sja1105_read_reg32(&sjap, SJA1105_REG_GENERAL_STATUS9);
	printf("general_status_9    = %08x\n", val32);

	for (i = 0; i < SJA1105_PORT_NB; i++) {
		printf("\nEthernet MAC-level status port%d\n", i);
		val32 = sja1105_read_reg32(&sjap,
					   SJA1105_REG_PORT_MAC_STATUS(i));
		for (j = 0; j < NUM_MAC_LVL_COUNTERS1; j++)
			printf("port%d %s    = %u\n", i, mac_lvl_counters1[j],
			       (val32 >> (j * 8)) & 0xFF);

		val32 = sja1105_read_reg32(&sjap,
					   SJA1105_REG_PORT_MAC_STATUS(i) + 1);
		for (j = 0; j < NUM_MAC_LVL_COUNTERS2; j++)
			printf("port%d %s    = %u\n", i, mac_lvl_counters2[j],
			       (val32 >> j) & 1);
	}

	for (i = 0; i < SJA1105_PORT_NB; i++) {
		printf("\nEthernet High-level status port%d\n", i);
		for (j = 0; j < NUM_ETH_HIGH_LVL_COUNTERS1; j++) {
			val32 = sja1105_read_reg32(&sjap,
						SJA1105_REG_PORT_HIGH_STATUS1(i)
						+ j);
			printf("port%d %s    = %u\n", i,
			       eth_high_lvl_counters1[j], val32);
		}
		for (j = 0; j < NUM_ETH_HIGH_LVL_COUNTERS2; j++) {
			val32 = sja1105_read_reg32(&sjap,
						SJA1105_REG_PORT_HIGH_STATUS2(i)
						+ j);
			printf("port%d %s    = %u\n", i,
			       eth_high_lvl_counters2[j], val32);
		}
	}

	return 0;
}

U_BOOT_CMD(sja, 2, 1, do_sja_regs,
	   "SJA1105 register dump",
	   "[<bus>:]<cs> - View registers for SJA\n"
);

