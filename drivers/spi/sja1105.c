// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2017-2018 NXP
 *
 */

#include <common.h>
#include <linux/errno.h>
#include <spi.h>
#include "sja1105_ll.h"

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

#define SJA_DSPI_MODE	(SPI_CPHA | SPI_FMSZ_16)
#define SJA_DSPI_HZ	5000

#define sja_debug(fmt, ...)	debug("[SJA1105]%s:%d " fmt, __func__, \
				__LINE__, ##__VA_ARGS__)

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
	u32 resp[4] = {0, };
	int i, maxtries = 3;

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

tryagain:
	ret = spi_xfer(slave, bitlen, cmd, resp,
		       SPI_XFER_BEGIN | SPI_XFER_END);

	if (ret)
		printf("Error %d during SPI transaction\n", ret);

	/* Sometimes the write can fail silently. Try again */
	if (maxtries) {
		maxtries--;
		for (i = 0; i < MIN(4, nb_words); i++) {
			if (cmd[i] != resp[i]) {
				sja_debug("cmd 0x%X 0x%X / resp 0x%X 0x%X\n",
					  cmd[0], cmd[1], resp[0], resp[1]);
				goto tryagain;
			}
		}
	}

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

	if (!sja1105_post_cfg_load_check(sjap))  {
		printf("SJA1105 configuration failed\n");
		return -ENXIO;
	}
	return 0;
}

int sja1105_probe(u32 cs, u32 bus)
{
	struct sja_parms sjap;
	int ret = 0;

	memset(&sjap, 0, sizeof(struct sja_parms));

	sjap.cs = cs;
	sjap.bus = bus;

	printf("Loading SJA1105 firmware over SPI\n");

	sjap.devid = sja1105_check_device_id(&sjap);

	sja_debug("devid %X\n", sjap.devid);

	ret = sja1105_get_cfg(sjap.devid, sjap.cs, &sjap.bin_len,
			      &sjap.cfg_bin);

	if (ret) {
		printf("Error SJA1105 configuration not completed\n");
		return -EINVAL;
	}

	return sja1105_configuration_load(&sjap);
}
