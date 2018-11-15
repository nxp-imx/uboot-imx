/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _XEN_EVENTS_H
#define _XEN_EVENTS_H

#include <xen/interface/event_channel.h>
#include <asm/xen/hypercall.h>
#include <asm/xen/events.h>

static inline void notify_remote_via_evtchn(int port)
{
	struct evtchn_send send = { .port = port };
	(void)HYPERVISOR_event_channel_op(EVTCHNOP_send, &send);
}

#endif	/* _XEN_EVENTS_H */
