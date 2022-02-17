// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019-2020 NXP.
 */

/*
 * Configuration of the Tamper pins in different mode:
 *  - default (no tamper pins): _default_
 *  - passive mode expecting VCC on the line: "_passive_vcc_"
 *  - passive mode expecting VCC on the line: "_passive_gnd_"
 *  - active mode: "_active_"
 *
 * WARNING:
 * The silicon revision B0 of the iMX8QM and iMX8QXP have a bug in the SECO ROM:
 * If the SSM of the SNVS changes state, the next call to SECO will trigger an
 * integrity check of the SECO firmware which will fail due to incorrect CAAM
 * keys hence the SECO will not respond to the call. The system will hang in
 * this state until a watchdog resets the board.
 */

#include <command.h>
#include <log.h>
#include <stddef.h>
#include <common.h>
#include <console.h>
#include <asm/arch/sci/sci.h>
#include <asm/arch-imx8/imx8-pins.h>
#include <asm/arch-imx8/snvs_security_sc.h>
#include <asm/global_data.h>
#include <asm/arch/sys_proto.h>
#include "snvs_security_sc_conf_board.h"
#include <imx_sip.h>
#include <linux/arm-smccc.h>

#define SC_WRITE_CONF 1

#define PGD_HEX_VALUE 0x41736166
#define SRTC_EN 0x1
#define DP_EN BIT(5)

#ifdef CONFIG_IMX_SNVS_SEC_SC_AUTO
static struct snvs_security_sc_conf *get_snvs_config(void)
{
	return &snvs_default_config;
}

static struct snvs_dgo_conf *get_snvs_dgo_config(void)
{
	return &snvs_dgo_default_config;
}
#endif

#define TAMPER_PIN_LIST_CHOSEN tamper_pin_list_default_config

static struct tamper_pin_cfg *get_tamper_pin_cfg_list(u32 *size)
{
	*size = sizeof(TAMPER_PIN_LIST_CHOSEN) /
		sizeof(TAMPER_PIN_LIST_CHOSEN[0]);

	return TAMPER_PIN_LIST_CHOSEN;
}

#define SC_CONF_OFFSET_OF(_field) \
	(offsetof(struct snvs_security_sc_conf, _field))

static u32 ptr_value(u32 *_p)
{
	return (_p) ? *_p : 0xdeadbeef;
}

static int check_write_secvio_config(u32 id, u32 *_p1, u32 *_p2,
					  u32 *_p3, u32 *_p4, u32 *_p5,
					  u32 _cnt)
{
	int err = 0;
	u32 d1 = ptr_value(_p1);
	u32 d2 = ptr_value(_p2);
	u32 d3 = ptr_value(_p3);
	u32 d4 = ptr_value(_p4);
	u32 d5 = ptr_value(_p5);

	err = sc_seco_secvio_config(-1, id, SC_WRITE_CONF, &d1, &d2, &d3,
				       &d4, &d4, _cnt);
	if (err) {
		printf("Failed to set secvio configuration\n");
		debug("Failed to set conf id 0x%x with values ", id);
		debug("0x%.8x 0x%.8x 0x%.8x 0x%.8x 0x%.8x (cnt: %d)\n",
		      d1, d2, d3, d4, d5, _cnt);
		goto exit;
	}

	if (_p1)
		*(u32 *)_p1 = d1;
	if (_p2)
		*(u32 *)_p2 = d2;
	if (_p3)
		*(u32 *)_p3 = d3;
	if (_p4)
		*(u32 *)_p4 = d4;
	if (_p5)
		*(u32 *)_p5 = d5;

exit:
	return err;
}

#define SC_CHECK_WRITE1(id, _p1) \
	check_write_secvio_config(id, _p1, NULL, NULL, NULL, NULL, 1)

static int apply_snvs_config(struct snvs_security_sc_conf *cnf)
{
	int err = 0;

	debug("%s\n", __func__);

	debug("Applying config:\n"
		  "\thp.lock = 0x%.8x\n"
		  "\thp.secvio_intcfg = 0x%.8x\n"
		  "\thp.secvio_ctl = 0x%.8x\n"
		  "\tlp.lock = 0x%.8x\n"
		  "\tlp.secvio_ctl = 0x%.8x\n"
		  "\tlp.tamper_filt_cfg = 0x%.8x\n"
		  "\tlp.tamper_det_cfg = 0x%.8x\n"
		  "\tlp.tamper_det_cfg2 = 0x%.8x\n"
		  "\tlp.tamper_filt1_cfg = 0x%.8x\n"
		  "\tlp.tamper_filt2_cfg = 0x%.8x\n"
		  "\tlp.act_tamper1_cfg = 0x%.8x\n"
		  "\tlp.act_tamper2_cfg = 0x%.8x\n"
		  "\tlp.act_tamper3_cfg = 0x%.8x\n"
		  "\tlp.act_tamper4_cfg = 0x%.8x\n"
		  "\tlp.act_tamper5_cfg = 0x%.8x\n"
		  "\tlp.act_tamper_ctl = 0x%.8x\n"
		  "\tlp.act_tamper_clk_ctl = 0x%.8x\n"
		  "\tlp.act_tamper_routing_ctl1 = 0x%.8x\n"
		  "\tlp.act_tamper_routing_ctl2 = 0x%.8x\n",
			cnf->hp.lock,
			cnf->hp.secvio_intcfg,
			cnf->hp.secvio_ctl,
			cnf->lp.lock,
			cnf->lp.secvio_ctl,
			cnf->lp.tamper_filt_cfg,
			cnf->lp.tamper_det_cfg,
			cnf->lp.tamper_det_cfg2,
			cnf->lp.tamper_filt1_cfg,
			cnf->lp.tamper_filt2_cfg,
			cnf->lp.act_tamper1_cfg,
			cnf->lp.act_tamper2_cfg,
			cnf->lp.act_tamper3_cfg,
			cnf->lp.act_tamper4_cfg,
			cnf->lp.act_tamper5_cfg,
			cnf->lp.act_tamper_ctl,
			cnf->lp.act_tamper_clk_ctl,
			cnf->lp.act_tamper_routing_ctl1,
			cnf->lp.act_tamper_routing_ctl2);

	err = check_write_secvio_config(SC_CONF_OFFSET_OF(lp.tamper_filt_cfg),
					   &cnf->lp.tamper_filt_cfg,
					   &cnf->lp.tamper_filt1_cfg,
					   &cnf->lp.tamper_filt2_cfg, NULL,
					   NULL, 3);
	if (err)
		goto exit;

	/* Configure AT */
	err = check_write_secvio_config(SC_CONF_OFFSET_OF(lp.act_tamper1_cfg),
					   &cnf->lp.act_tamper1_cfg,
					   &cnf->lp.act_tamper2_cfg,
					   &cnf->lp.act_tamper3_cfg,
					   &cnf->lp.act_tamper4_cfg,
					   &cnf->lp.act_tamper5_cfg, 5);
	if (err)
		goto exit;

	/* Configure AT routing */
	err = check_write_secvio_config(SC_CONF_OFFSET_OF(lp.act_tamper_routing_ctl1),
					   &cnf->lp.act_tamper_routing_ctl1,
					   &cnf->lp.act_tamper_routing_ctl2,
					   NULL, NULL, NULL, 2);
	if (err)
		goto exit;

	/* Configure AT frequency */
	err = SC_CHECK_WRITE1(SC_CONF_OFFSET_OF(lp.act_tamper_clk_ctl),
				 &cnf->lp.act_tamper_clk_ctl);
	if (err)
		goto exit;

	/* Activate the ATs */
	err = SC_CHECK_WRITE1(SC_CONF_OFFSET_OF(lp.act_tamper_ctl),
				 &cnf->lp.act_tamper_ctl);
	if (err)
		goto exit;

	/* Activate the detectors */
	err = check_write_secvio_config(SC_CONF_OFFSET_OF(lp.tamper_det_cfg),
					   &cnf->lp.tamper_det_cfg,
					   &cnf->lp.tamper_det_cfg2, NULL, NULL,
					   NULL, 2);
	if (err)
		goto exit;

	/* Configure LP secvio */
	err = SC_CHECK_WRITE1(SC_CONF_OFFSET_OF(lp.secvio_ctl),
				 &cnf->lp.secvio_ctl);
	if (err)
		goto exit;

	/* Configure HP secvio */
	err = SC_CHECK_WRITE1(SC_CONF_OFFSET_OF(hp.secvio_ctl),
				 &cnf->hp.secvio_ctl);
	if (err)
		goto exit;

	err = SC_CHECK_WRITE1(SC_CONF_OFFSET_OF(hp.secvio_intcfg),
				 &cnf->hp.secvio_intcfg);
	if (err)
		goto exit;

	/* Lock access */
	if (cnf->hp.lock) {
		err = SC_CHECK_WRITE1(SC_CONF_OFFSET_OF(hp.lock), &cnf->hp.lock);
		if (err)
			goto exit;
	}

	if (cnf->lp.lock) {
		err = SC_CHECK_WRITE1(SC_CONF_OFFSET_OF(lp.lock), &cnf->lp.lock);
		if (err)
			goto exit;
	}

exit:
	return err;
}

static int dgo_write(u32 _id, u8 _access, u32 *_pdata)
{
	int err = sc_seco_secvio_dgo_config(-1, _id, _access, _pdata);

	if (err) {
		printf("Failed to set dgo configuration\n");
		debug("Failed to set conf id 0x%x : 0x%.8x", _id, *_pdata);
	}

	return err;
}

static int apply_snvs_dgo_config(struct snvs_dgo_conf *cnf)
{
	int err = 0;

	debug("%s\n", __func__);

	debug("Applying config:\n"
		"\ttamper_offset_ctl = 0x%.8x\n"
		"\ttamper_pull_ctl = 0x%.8x\n"
		"\ttamper_ana_test_ctl = 0x%.8x\n"
		"\ttamper_sensor_trim_ctl = 0x%.8x\n"
		"\ttamper_misc_ctl = 0x%.8x\n"
		"\ttamper_core_volt_mon_ctl = 0x%.8x\n",
			cnf->tamper_offset_ctl,
			cnf->tamper_pull_ctl,
			cnf->tamper_ana_test_ctl,
			cnf->tamper_sensor_trim_ctl,
			cnf->tamper_misc_ctl,
			cnf->tamper_core_volt_mon_ctl);

	err = dgo_write(0x04, 1, &cnf->tamper_offset_ctl);
	if (err)
		goto exit;

	err = dgo_write(0x14, 1, &cnf->tamper_pull_ctl);
	if (err)
		goto exit;

	err = dgo_write(0x24, 1, &cnf->tamper_ana_test_ctl);
	if (err)
		goto exit;

	err = dgo_write(0x34, 1, &cnf->tamper_sensor_trim_ctl);
	if (err)
		goto exit;

	err = dgo_write(0x54, 1, &cnf->tamper_core_volt_mon_ctl);
	if (err)
		goto exit;

	/* Last as it could lock the writes */
	err = dgo_write(0x44, 1, &cnf->tamper_misc_ctl);
	if (err)
		goto exit;

exit:
	return err;
}

static int pad_write(u32 _pad, u32 _value)
{
	int err = sc_pad_set(-1, _pad, _value);

	if (err) {
		printf("Failed to set pad configuration\n");
		debug("Failed to set conf pad 0x%x : 0x%.8x", _pad, _value);
	}

	return err;
}

static int pad_read(u32 _pad, u32 *_value)
{
	int err = sc_pad_get(-1, _pad, _value);

	if (err) {
		printf("Failed to get pad configuration\n");
		printf("Failed to get conf pad %d", _pad);
	}

	return err;
}

static int apply_tamper_pin_list_config(struct tamper_pin_cfg *confs, u32 size)
{
	int err = 0;
	u32 idx;

	debug("%s\n", __func__);

	for (idx = 0; idx < size; idx++) {
		if (confs[idx].pad == TAMPER_NOT_DEFINED)
			continue;

		debug("\t idx %d: pad %d: 0x%.8x\n", idx, confs[idx].pad,
		      confs[idx].mux_conf);
		err = pad_write(confs[idx].pad, 3 << 30 | confs[idx].mux_conf);
		if (err)
			goto exit;
	}

exit:
	return err;
}

#ifdef CONFIG_IMX_SNVS_SEC_SC_AUTO
int snvs_security_sc_init(void)
{
	int err = 0;

	struct snvs_security_sc_conf *snvs_conf;
	struct snvs_dgo_conf *snvs_dgo_conf;
	struct tamper_pin_cfg *tamper_pin_conf;
	u32 size;

	debug("%s\n", __func__);

	snvs_conf = get_snvs_config();
	snvs_dgo_conf = get_snvs_dgo_config();

	tamper_pin_conf = get_tamper_pin_cfg_list(&size);

	err = apply_tamper_pin_list_config(tamper_pin_conf, size);
	if (err) {
		debug("Failed to set pins\n");
		goto exit;
	}

	err = apply_snvs_dgo_config(snvs_dgo_conf);
	if (err) {
		debug("Failed to set dgo\n");
		goto exit;
	}

	err = apply_snvs_config(snvs_conf);
	if (err) {
		debug("Failed to set snvs\n");
		goto exit;
	}

exit:
	return err;
}
#endif /* CONFIG_IMX_SNVS_SEC_SC_AUTO */

static char snvs_cfg_help_text[] =
	"snvs_cfg\n"
	"\thp.lock\n"
	"\thp.secvio_intcfg\n"
	"\thp.secvio_ctl\n"
	"\tlp.lock\n"
	"\tlp.secvio_ctl\n"
	"\tlp.tamper_filt_cfg\n"
	"\tlp.tamper_det_cfg\n"
	"\tlp.tamper_det_cfg2\n"
	"\tlp.tamper_filt1_cfg\n"
	"\tlp.tamper_filt2_cfg\n"
	"\tlp.act_tamper1_cfg\n"
	"\tlp.act_tamper2_cfg\n"
	"\tlp.act_tamper3_cfg\n"
	"\tlp.act_tamper4_cfg\n"
	"\tlp.act_tamper5_cfg\n"
	"\tlp.act_tamper_ctl\n"
	"\tlp.act_tamper_clk_ctl\n"
	"\tlp.act_tamper_routing_ctl1\n"
	"\tlp.act_tamper_routing_ctl2\n"
	"\n"
	"ALL values should be in hexadecimal format";

#define NB_REGISTERS 19
static int do_snvs_cfg(struct cmd_tbl *cmdtp, int flag, int argc,
		       char *const argv[])
{
	int err = 0;
	u32 idx = 0;

	struct snvs_security_sc_conf conf = {0};

	if (argc != (NB_REGISTERS + 1))
		return CMD_RET_USAGE;

	conf.hp.lock = hextoul(argv[++idx], NULL);
	conf.hp.secvio_intcfg = hextoul(argv[++idx], NULL);
	conf.hp.secvio_ctl = hextoul(argv[++idx], NULL);
	conf.lp.lock = hextoul(argv[++idx], NULL);
	conf.lp.secvio_ctl = hextoul(argv[++idx], NULL);
	conf.lp.tamper_filt_cfg = hextoul(argv[++idx], NULL);
	conf.lp.tamper_det_cfg = hextoul(argv[++idx], NULL);
	conf.lp.tamper_det_cfg2 = hextoul(argv[++idx], NULL);
	conf.lp.tamper_filt1_cfg = hextoul(argv[++idx], NULL);
	conf.lp.tamper_filt2_cfg = hextoul(argv[++idx], NULL);
	conf.lp.act_tamper1_cfg = hextoul(argv[++idx], NULL);
	conf.lp.act_tamper2_cfg = hextoul(argv[++idx], NULL);
	conf.lp.act_tamper3_cfg = hextoul(argv[++idx], NULL);
	conf.lp.act_tamper4_cfg = hextoul(argv[++idx], NULL);
	conf.lp.act_tamper5_cfg = hextoul(argv[++idx], NULL);
	conf.lp.act_tamper_ctl = hextoul(argv[++idx], NULL);
	conf.lp.act_tamper_clk_ctl = hextoul(argv[++idx], NULL);
	conf.lp.act_tamper_routing_ctl1 = hextoul(argv[++idx], NULL);
	conf.lp.act_tamper_routing_ctl2 = hextoul(argv[++idx], NULL);

	err = apply_snvs_config(&conf);

	return err;
}

U_BOOT_CMD(snvs_cfg,
	   NB_REGISTERS + 1, 1, do_snvs_cfg,
	   "Security violation configuration",
	   snvs_cfg_help_text
);

static char snvs_dgo_cfg_help_text[] =
	"snvs_dgo_cfg\n"
	"\ttamper_offset_ctl\n"
	"\ttamper_pull_ctl\n"
	"\ttamper_ana_test_ctl\n"
	"\ttamper_sensor_trim_ctl\n"
	"\ttamper_misc_ctl\n"
	"\ttamper_core_volt_mon_ctl\n"
	"\n"
	"ALL values should be in hexadecimal format";

static int do_snvs_dgo_cfg(struct cmd_tbl *cmdtp, int flag, int argc,
			   char *const argv[])
{
	int err = 0;
	u32 idx = 0;

	struct snvs_dgo_conf conf = {0};

	if (argc != (6 + 1))
		return CMD_RET_USAGE;

	conf.tamper_offset_ctl = hextoul(argv[++idx], NULL);
	conf.tamper_pull_ctl = hextoul(argv[++idx], NULL);
	conf.tamper_ana_test_ctl = hextoul(argv[++idx], NULL);
	conf.tamper_sensor_trim_ctl = hextoul(argv[++idx], NULL);
	conf.tamper_misc_ctl = hextoul(argv[++idx], NULL);
	conf.tamper_core_volt_mon_ctl = hextoul(argv[++idx], NULL);

	err = apply_snvs_dgo_config(&conf);

	return err;
}

U_BOOT_CMD(snvs_dgo_cfg,
	   7, 1, do_snvs_dgo_cfg,
	   "SNVS DGO configuration",
	   snvs_dgo_cfg_help_text
);

static char tamper_pin_cfg_help_text[] =
	"snvs_dgo_cfg\n"
	"\tpad\n"
	"\tvalue\n"
	"\n"
	"ALL values should be in hexadecimal format";

static int do_tamper_pin_cfg(struct cmd_tbl *cmdtp, int flag, int argc,
			     char *const argv[])
{
	int err = 0;
	u32 idx = 0;

	struct tamper_pin_cfg conf = {0};

	if (argc != (2 + 1))
		return CMD_RET_USAGE;

	conf.pad = dectoul(argv[++idx], NULL);
	conf.mux_conf = hextoul(argv[++idx], NULL);

	err = apply_tamper_pin_list_config(&conf, 1);

	return err;
}

U_BOOT_CMD(tamper_pin_cfg,
	   3, 1, do_tamper_pin_cfg,
	   "tamper pin configuration",
	   tamper_pin_cfg_help_text
);

static char snvs_clear_status_help_text[] =
	"snvs_clear_status\n"
	"\tLPSR\n"
	"\tLPTDSR\n"
	"\n"
	"Write the status registers with the value provided,"
	" clearing the status";

static int do_snvs_clear_status(struct cmd_tbl *cmdtp, int flag, int argc,
				char *const argv[])
{
	int err = 0;
	u32 idx = 0;

	struct snvs_security_sc_conf conf = {0};

	if (argc != (2 + 1))
		return CMD_RET_USAGE;

	conf.lp.status = hextoul(argv[++idx], NULL);
	conf.lp.tamper_det_status = hextoul(argv[++idx], NULL);

	err = check_write_secvio_config(SC_CONF_OFFSET_OF(lp.status),
					   &conf.lp.status, NULL, NULL, NULL,
					   NULL, 1);
	if (err)
		goto exit;

	err = check_write_secvio_config(SC_CONF_OFFSET_OF(lp.tamper_det_status),
					   &conf.lp.tamper_det_status, NULL,
					   NULL, NULL, NULL, 1);
	if (err)
		goto exit;

exit:
	return err;
}

U_BOOT_CMD(snvs_clear_status,
	   3, 1, do_snvs_clear_status,
	   "snvs clear status",
	   snvs_clear_status_help_text
);

static char snvs_sec_status_help_text[] =
	"snvs_sec_status\n"
	"Display information about the security related to tamper and secvio";

static int do_snvs_sec_status(struct cmd_tbl *cmdtp, int flag, int argc,
			      char *const argv[])
{
	int err;
	u32 idx;
	u32 nb_pins;
	u32 data[5];
	struct tamper_pin_cfg *pin_cfg_list = get_tamper_pin_cfg_list(&nb_pins);

	u32 fuses[] = {
		14,
		30,
		31,
		260,
		261,
		262,
		263,
		768,
	};

	struct snvs_reg {
		u32 id;
		u32 nb;
	} snvs[] = {
		/* Locks */
		{0x0,  1},
		{0x34, 1},
		/* Security violation */
		{0xc,  1},
		{0x10, 1},
		{0x18, 1},
		{0x40, 1},
		/* Temper detectors */
		{0x48, 2},
		{0x4c, 1},
		{0xa4, 1},
		/* */
		{0x44, 3},
		{0xe0, 1},
		{0xe4, 1},
		{0xe8, 2},
		/* Misc */
		{0x3c, 1},
		{0x5c, 2},
		{0x64, 1},
		{0xf8, 2},
	};

	u32 dgo[] = {
		0x0,
		0x10,
		0x20,
		0x30,
		0x40,
		0x50,
	};

	/* Pins */
	printf("Pins:\n");
	for (idx = 0; idx < nb_pins; idx++) {
		struct tamper_pin_cfg *cfg = &pin_cfg_list[idx];

		if (cfg->pad == TAMPER_NOT_DEFINED)
			continue;

		err = sc_pad_get(-1, cfg->pad, &data[0]);
		if (err == 0)
			printf("\t- Pin %d: %.8x\n", cfg->pad, data[0]);
		else
			printf("Failed to read Pin %d\n", cfg->pad);
	}

	/* Fuses */
	printf("Fuses:\n");
	for (idx = 0; idx < ARRAY_SIZE(fuses); idx++) {
		u32 fuse_id = fuses[idx];

		err = sc_misc_otp_fuse_read(-1, fuse_id, &data[0]);
		if (err == 0)
			printf("\t- Fuse %d: %.8x\n", fuse_id, data[0]);
		else
			printf("Failed to read Fuse %d\n", fuse_id);
	}

	/* SNVS */
	printf("SNVS:\n");
	for (idx = 0; idx < ARRAY_SIZE(snvs); idx++) {
		struct snvs_reg *reg = &snvs[idx];

		err = sc_seco_secvio_config(-1, reg->id, 0, &data[0],
					       &data[1], &data[2], &data[3],
					       &data[4], reg->nb);
		if (err == 0) {
			int subidx;

			printf("\t- SNVS %.2x(%d):", reg->id, reg->nb);
			for (subidx = 0; subidx < reg->nb; subidx++)
				printf(" %.8x", data[subidx]);
			printf("\n");
		} else {
			printf("Failed to read SNVS %d\n", reg->id);
		}
	}

	/* DGO */
	printf("DGO:\n");
	for (idx = 0; idx < ARRAY_SIZE(dgo); idx++) {
		u8 dgo_id = dgo[idx];

		err = sc_seco_secvio_dgo_config(-1, dgo_id, 0, &data[0]);
		if (err == 0)
			printf("\t- DGO %.2x: %.8x\n", dgo_id, data[0]);
		else
			printf("Failed to read DGO %d\n", dgo_id);
	}

	return 0;
}

U_BOOT_CMD(snvs_sec_status,
	   1, 1, do_snvs_sec_status,
	   "tamper pin configuration",
	   snvs_sec_status_help_text
);

static char gpio_conf_help_text[] =
	"gpio_conf <pad> <hexval>\n"
	"Configure the GPIO of an IOMUX:\n"
	" - pad:\n"
	" - hexval:";

static int do_gpio_conf(struct cmd_tbl *cmdtp, int flag, int argc,
			char *const argv[])
{
	int err = -EIO;
	u32 pad, val, valcheck;

	pad = simple_strtoul(argv[1], NULL, 10);
	val = simple_strtoul(argv[2], NULL, 16);

	printf("Configuring GPIO %d with %x\n", pad, val);

	err = pad_write(pad, 3 << 30 | val);
	if (err) {
		printf("Error writing conf\n");
		goto exit;
	}

	err = pad_read(pad, &valcheck);
	if (err) {
		printf("Error reading conf\n");
		goto exit;
	}

	if (valcheck != val) {
		printf("Error: configured %x instead of %x\n", valcheck, val);
		goto exit;
	}

exit:
	return err;
}

U_BOOT_CMD(gpio_conf,
	   3, 1, do_gpio_conf,
	   "gpio configuration",
	   gpio_conf_help_text
);

static
int do_set_fips_mode(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	u8 fips_mode = 0;
	struct arm_smccc_res res;

	if (argc < 2)
		return CMD_RET_USAGE;

	fips_mode = simple_strtoul(argv[1], NULL, 16);

	if (argc == 2) {
		printf("Warning: Setting FIPS mode [%x] will burn a fuse and\n"
		       "is permanent\n"
		       "Really perform this fuse programming? <y/N>\n",
		       fips_mode);

		/* If the user does not answer yes (1), we return */
		if (confirm_yesno() != 1)
			return 0;
	}

	if (argc == 3 && !(argv[2][0] == '-' && argv[2][1] == 'y'))
		return CMD_RET_USAGE;

	arm_smccc_smc(IMX_SIP_FIPS_CONFIG, IMX_SIP_FIPS_CONFIG_SET,
		      fips_mode, 0, 0, 0, 0, 0, &res);
	if (res.a0) {
		printf("Failed to set fips mode %d. err: %ld\n",
		       fips_mode, res.a0);
	}

	return (res.a0) ? CMD_RET_FAILURE : CMD_RET_SUCCESS;
}

U_BOOT_CMD(set_fips_mode,
	   3, 0, do_set_fips_mode,
	   "Set FIPS mode",
	   "<mode in hex> [-y] \n"
	   "    The SoC will be configured in FIPS <mode> (PERMANENT)\n"
	   "    If \"-y\" is not passed, the function will ask for validation\n"
	   "ex: set_fips_mode 1\n"
);

static
int do_check_fips_mode(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	int err = -EIO;
	u32 fuse_value = 0;

	/* The FIPS bit is the bit 3 in the word 0xA */
	err = sc_misc_otp_fuse_read(-1, 0xA, &fuse_value);
	if (err)
		return err;

	printf("FIPS mode: %x\n", fuse_value >> 3 & 0x1);

	return 0;
}

U_BOOT_CMD(check_fips_mode,
	   1, 0, do_check_fips_mode,
	   "Display the FIPS mode of the SoC by reading fuse 0xA, bit 3",
	   NULL
);
