// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2019-2020 NXP
 */

#include <net/dsa.h>
#include <dm/test.h>
#include <test/ut.h>

extern int dsa_sandbox_port_mask;

/* this test sends ping requests with the local address through each DSA port
 * via the dummy DSA master Eth.
 * The dummy Eth filters traffic based on DSA port used to Tx and the port
 * mask set here, so we can check that port information gets trough correctly.
 */
static int dm_test_dsa(struct unit_test_state *uts)
{
	dsa_sandbox_port_mask = 0x5;

	env_set("ethrotate", "no");
	net_ping_ip = string_to_ip("1.2.3.4");

	env_set("ethact", "dsa-test-eth");
	ut_assertok(net_loop(PING));

	dsa_sandbox_port_mask = 0x7;
	env_set("ethact", "lan0");
	ut_assertok(net_loop(PING));
	env_set("ethact", "lan1");
	ut_assertok(net_loop(PING));
	env_set("ethact", "lan2");
	ut_assertok(net_loop(PING));

	dsa_sandbox_port_mask = 0x1;
	env_set("ethact", "lan0");
	ut_assertok(net_loop(PING));
	env_set("ethact", "lan1");
	ut_assert(net_loop(PING) != 0);
	env_set("ethact", "lan2");
	ut_assert(net_loop(PING) != 0);

	dsa_sandbox_port_mask = 0x6;
	env_set("ethact", "lan0");
	ut_assert(net_loop(PING) != 0);
	env_set("ethact", "lan1");
	ut_assertok(net_loop(PING));
	env_set("ethact", "lan2");
	ut_assertok(net_loop(PING));

	dsa_sandbox_port_mask = 0;
	env_set("ethact", "");
	env_set("ethrotate", "yes");

	return 0;
}

DM_TEST(dm_test_dsa, DM_TESTF_SCAN_FDT);
