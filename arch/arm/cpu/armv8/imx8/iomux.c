/*
 * Copyright (C) 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <asm/io.h>
#include <asm/arch/iomux.h>
#include <asm/imx-common/sci/sci.h>

DECLARE_GLOBAL_DATA_PTR;

/*
 * configures a single pad in the iomuxer
 */
void imx8_iomux_setup_pad(iomux_cfg_t pad)
{
	sc_err_t err;
	sc_ipc_t ipc;

	sc_pad_t pin_id = pad & PIN_ID_MASK;

	uint32_t val = (uint32_t)((pad & MUX_PAD_CTRL_MASK) >> MUX_PAD_CTRL_SHIFT);

	ipc = gd->arch.ipc_channel_handle;

	val |= PADRING_IFMUX_EN_MASK;
	val |= PADRING_GP_EN_MASK;

	err = sc_pad_set(ipc, pin_id, val);
	if (err != SC_ERR_NONE)
		printf("imx8_iomux sc_pad_set failed!, pin = %u, val = 0x%x\n", pin_id, val);

	debug("iomux: pin %d, val = 0x%x\n", pin_id, val);
}

/* configures a list of pads within declared with IOMUX_PADS macro */
void imx8_iomux_setup_multiple_pads(iomux_cfg_t const *pad_list,
				      unsigned count)
{
	iomux_cfg_t const *p = pad_list;
	int i;

	for (i = 0; i < count; i++) {
		imx8_iomux_setup_pad(*p);
		p++;
	}
}
