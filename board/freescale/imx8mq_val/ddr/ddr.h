/*
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef SRC_DDRC_RCR_ADDR
#define SRC_DDRC_RCR_ADDR SRC_IPS_BASE_ADDR +0x1000
#endif
#ifndef DDR_CSD1_BASE_ADDR
#define DDR_CSD1_BASE_ADDR 0x40000000
#endif

void ddr_load_train_code(enum fw_type type);
int wait_ddrphy_training_complete(void);
void ddr3_phyinit_train_1600mts(void);
void ddr4_phyinit_train_2400mts(void);
