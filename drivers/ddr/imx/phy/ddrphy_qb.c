// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2022 NXP
 */

#include <linux/errno.h>
#include <linux/kernel.h>
#include <asm/arch/ddr.h>

struct ddrphy_qb_state qb_state;

static void ddrphy_w(uint32_t addr, uint32_t offset, uint32_t value)
{
	uint32_t val = dwc_ddrphy_apb_rd(addr);
	bool hight = (offset % 2);

	val &= (hight ? 0x00FF : 0xFF00);
	val |= value << (hight ? 8 : 0);

	dwc_ddrphy_apb_wr(addr, val);
}

static int ddrphy_qb_restore(struct dram_timing_info *info, int fsp_id)
{
	int i;
	uint32_t to_addr;
	struct dram_fsp_msg *fsp_msg;
	struct dram_cfg_param *dram_cfg;

	fsp_msg = &info->fsp_msg[fsp_id];
	for (i = 0; i < fsp_msg->fsp_cfg_num; i++) {
		dram_cfg = &fsp_msg->fsp_cfg[i];
		dwc_ddrphy_apb_wr(dram_cfg->reg, dram_cfg->val);
	}

	/* enable the ddrphy apb */
	dwc_ddrphy_apb_wr(0xd0000, 0x00);
	dwc_ddrphy_apb_wr(0x54008, 0x01); /* SequenceCtrl = 0x1 (DevInit Only)*/
	ddrphy_w(0x5400c, 0x19, 0x1);     /* Lp4Quickboot = 0x1 */

	/* Adjust MR14_xy if pstate=0 and 2D training data collected during training phase */
	if (fsp_id == 0 && (qb_state.flags & DDRPHY_QB_FLAG_2D)) {
		ddrphy_w(0x5401c, 0x39, qb_state.fsp[0] >> 8);   /* TrainedVREFDQ_A0 -> MR14_A0 */
		ddrphy_w(0x54022, 0x45, qb_state.fsp[1] & 0xFF); /* TrainedVREFDQ_A1 -> MR14_A1 */
		ddrphy_w(0x54036, 0x6c, qb_state.fsp[2] & 0xFF); /* TrainedVREFDQ_B0 -> MR14_B0 */
		ddrphy_w(0x5403c, 0x78, qb_state.fsp[2] >> 8);   /* TrainedVREFDQ_B1 -> MR14_B1 */
	}

	/* restore errata registers */
	for (i = 0; i < DDRPHY_QB_ERR_SIZE; i++)
		dwc_ddrphy_apb_wr(ddrphy_err_cfg[i], qb_state.err[i]);

	/* save CSRs to address starting with 0x54800 */
	for (i = 0, to_addr = 0x54800; i < DDRPHY_QB_CSR_SIZE; i++, to_addr++)
		dwc_ddrphy_apb_wr(to_addr, qb_state.csr[i]);

	/* disable the ddrphy apb */
	dwc_ddrphy_apb_wr(0xd0000, 0x01);

	return 0;
}

int ddr_cfg_phy_qb(struct dram_timing_info *dram_timing, int fsp_id)
{
	int ret;

	/* MemReset Toggle */
	dwc_ddrphy_apb_wr(0xd0000, 0x0);
	dwc_ddrphy_apb_wr(0x20060, 0x3);
	dwc_ddrphy_apb_wr(0x2008F, 0x1);

	/* load the dram quick boot firmware image */
	dwc_ddrphy_apb_wr(0xd0000, 0x0);
	ddr_load_train_firmware(FW_1D_IMAGE);
	ddrphy_qb_restore(dram_timing, fsp_id);

	/* excute the firmware */
	dwc_ddrphy_apb_wr(0xd0000, 0x0);
	dwc_ddrphy_apb_wr(0xc0080, 0x3);
	dwc_ddrphy_apb_wr(0xd0031, 0x1);
	dwc_ddrphy_apb_wr(0xd0000, 0x1);
	dwc_ddrphy_apb_wr(0xd0099, 0x9);
	dwc_ddrphy_apb_wr(0xd0099, 0x1);
	dwc_ddrphy_apb_wr(0xd0099, 0x0);

	/* Wait for the quick boot firmware to complete */
	ret = wait_ddrphy_training_complete();
	if (ret)
		return ret;

	/* Halt the microcontroller. */
	dwc_ddrphy_apb_wr(0xd0099, 0x1);
	dwc_ddrphy_apb_wr(0xd0000, 0x0);

	get_trained_CDD(0);

	/* step I Configure PHY for hardware control */
	dwc_ddrphy_apb_wr(0xd00e7, 0x400);
	dwc_ddrphy_apb_wr(0xc0080, 0x2);
	dwc_ddrphy_apb_wr(0xd0000, 0x1);

	return 0;
}
