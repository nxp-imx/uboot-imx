/*
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

static inline void poll_pmu_message_ready(void)
{
	unsigned int reg;

	do {
		reg = reg32_read(IP2APB_DDRPHY_IPS_BASE_ADDR(0)+4*0xd0004);
	} while (reg & 0x1);
}

static inline void ack_pmu_message_recieve(void)
{
	unsigned int reg;

	reg32_write(IP2APB_DDRPHY_IPS_BASE_ADDR(0)+4*0xd0031,0x0);

	do {
		reg = reg32_read(IP2APB_DDRPHY_IPS_BASE_ADDR(0)+4*0xd0004);
	} while (!(reg & 0x1));

	reg32_write(IP2APB_DDRPHY_IPS_BASE_ADDR(0)+4*0xd0031,0x1);
}

static inline unsigned int get_mail(void)
{
	unsigned int reg;

	poll_pmu_message_ready();

	reg = reg32_read(IP2APB_DDRPHY_IPS_BASE_ADDR(0)+4*0xd0032);

	ack_pmu_message_recieve();

	return reg;
}

static inline unsigned int get_stream_message(void)
{
	unsigned int reg, reg2;

	poll_pmu_message_ready();

	reg = reg32_read(IP2APB_DDRPHY_IPS_BASE_ADDR(0)+4*0xd0032);

	reg2 = reg32_read(IP2APB_DDRPHY_IPS_BASE_ADDR(0)+4*0xd0034);

	reg2 = (reg2 << 16) | reg;

	ack_pmu_message_recieve();

	return reg2;
}

static inline void decode_major_message(unsigned int mail)
{
	ddr_printf("[PMU Major message = 0x%08x]\n", mail);
}

static inline void decode_streaming_message(void)
{
	unsigned int string_index, arg __maybe_unused;
	int i = 0;

	string_index = get_stream_message();
	ddr_printf("	PMU String index = 0x%08x\n", string_index);
	while (i < (string_index & 0xffff)){
		arg = get_stream_message();
		ddr_printf("	arg[%d] = 0x%08x\n", i, arg);
		i++;
	}

	ddr_printf("\n");
}

int wait_ddrphy_training_complete(void)
{
	unsigned int mail;
	while (1) {
		mail = get_mail();
		decode_major_message(mail);
		if (mail == 0x08) {
			decode_streaming_message();
		} else if (mail == 0x07) {
			printf("Training PASS\n");
			return 0;
		} else if (mail == 0xff) {
			printf("Training FAILED\n");
			return -1;
		}
	}
}
