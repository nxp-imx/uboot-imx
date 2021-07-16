// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2021 NXP
 */

#include <common.h>
#include <dm.h>
#include <power-domain-uclass.h>
#include <scmi_agent.h>
#include <scmi_protocols.h>
#include <asm/types.h>
#include <dm/device-internal.h>
#include <dm/device.h>

struct scmi_power_domain_priv {
	struct scmi_channel *channel;
};

static int scmi_power_domain_request(struct power_domain *power_domain)
{
	return 0;
}

static int scmi_power_domain_free(struct power_domain *power_domain)
{
	return 0;
}

static int scmi_power_state_set(struct power_domain *power_domain, u32 state)
{
	struct scmi_power_domain_priv *priv = dev_get_priv(power_domain->dev);
	struct scmi_power_set_state in = {
		.domain = power_domain->id,
		.flags = 0,
		.state = state,
	};
	struct scmi_power_set_state_out out;
	struct scmi_msg msg = SCMI_MSG_IN(SCMI_PROTOCOL_ID_POWER_DOMAIN,
					  SCMI_POWER_STATE_SET,
					  in, out);
	int ret;

	ret = devm_scmi_process_msg(power_domain->dev, priv->channel, &msg);
	if (ret)
		return ret;

	return scmi_to_linux_errno(out.status);
}

static int scmi_power_state_get(struct power_domain *power_domain, u32 *state)
{
	struct scmi_power_domain_priv *priv = dev_get_priv(power_domain->dev);
	struct scmi_power_get_state in = {
		.domain = power_domain->id,
	};

	struct scmi_power_get_state_out out;
	struct scmi_msg msg = SCMI_MSG_IN(SCMI_PROTOCOL_ID_POWER_DOMAIN,
					  SCMI_POWER_STATE_GET,
					  in, out);
	int ret;

	ret = devm_scmi_process_msg(power_domain->dev, priv->channel, &msg);
	if (ret)
		return ret;

	ret = scmi_to_linux_errno(out.status);
	if (ret < 0)
		return ret;

	*state = out.state;

	return ret;
}

static int scmi_power_domain_power(struct power_domain *power_domain, bool power_on)
{
	int ret;
	u32 state, ret_state;

	if (power_on)
		state = SCMI_POWER_STATE_GENERIC_ON;
	else
		state = SCMI_POWER_STATE_GENERIC_OFF;

	ret = scmi_power_state_set(power_domain, state);
	if (!ret)
		ret = scmi_power_state_get(power_domain, &ret_state);
	if (!ret && state != ret_state)
		return -EIO;

	return ret;
}

static int scmi_power_domain_on(struct power_domain *power_domain)
{
	debug("%s: id %lu\n", __func__, power_domain->id);

	return scmi_power_domain_power(power_domain, true);
}

static int scmi_power_domain_off(struct power_domain *power_domain)
{
	debug("%s: id %lu\n", __func__, power_domain->id);

	return scmi_power_domain_power(power_domain, false);
}

static int scmi_power_domain_probe(struct udevice *dev)
{
	struct scmi_power_domain_priv *priv = dev_get_priv(dev);

	return devm_scmi_of_get_channel(dev, &priv->channel);
}

struct power_domain_ops scmi_power_domain_ops = {
	.request = scmi_power_domain_request,
	.rfree = scmi_power_domain_free,
	.on = scmi_power_domain_on,
	.off = scmi_power_domain_off,
};

U_BOOT_DRIVER(scmi_power_domain) = {
	.name = "scmi_power_domain",
	.id = UCLASS_POWER_DOMAIN,
	.ops = &scmi_power_domain_ops,
	.probe = scmi_power_domain_probe,
	.priv_auto = sizeof(struct scmi_power_domain_priv *),
};
