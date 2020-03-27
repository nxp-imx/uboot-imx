// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2011, Marvell Semiconductor Inc.
 * Lei Wen <leiwen@marvell.com>
 *
 * Back ported to the 8xx platform (from the 8260 platform) by
 * Murray.Jensen@cmst.csiro.au, 27-Jan-01.
 */

#include <common.h>
#include <command.h>
#include <config.h>
#include <cpu_func.h>
#include <net.h>
#include <malloc.h>
#include <dm.h>
#include <clk.h>
#include <power-domain.h>
#include <asm/byteorder.h>
#include <linux/errno.h>
#include <asm/io.h>
#include <asm/unaligned.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/clock.h>
#include <asm/mach-imx/regs-usbphy.h>
#include <linux/types.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <linux/usb/otg.h>
#include <dm/pinctrl.h>
#include <usb/ci_udc.h>
#include <usb/ehci-ci.h>
#include <usb/usb_mx6_common.h>
#include "../host/ehci.h"
#include "ci_udc.h"

/*
 * Check if the system has too long cachelines. If the cachelines are
 * longer then 128b, the driver will not be able flush/invalidate data
 * cache over separate QH entries. We use 128b because one QH entry is
 * 64b long and there are always two QH list entries for each endpoint.
 */
#if ARCH_DMA_MINALIGN > 128
#error This driver can not work on systems with caches longer than 128b
#endif

/*
 * Every QTD must be individually aligned, since we can program any
 * QTD's address into HW. Cache flushing requires ARCH_DMA_MINALIGN,
 * and the USB HW requires 32-byte alignment. Align to both:
 */
#define ILIST_ALIGN		roundup(ARCH_DMA_MINALIGN, 32)
/* Each QTD is this size */
#define ILIST_ENT_RAW_SZ	sizeof(struct ept_queue_item)
/*
 * Align the size of the QTD too, so we can add this value to each
 * QTD's address to get another aligned address.
 */
#define ILIST_ENT_SZ		roundup(ILIST_ENT_RAW_SZ, ILIST_ALIGN)
/* For each endpoint, we need 2 QTDs, one for each of IN and OUT */
#define ILIST_SZ		(NUM_ENDPOINTS * 2 * ILIST_ENT_SZ)

#define EP_MAX_LENGTH_TRANSFER	0x4000

#ifndef DEBUG
#define DBG(x...) do {} while (0)
#else
#define DBG(x...) printf(x)
static const char *reqname(unsigned r)
{
	switch (r) {
	case USB_REQ_GET_STATUS: return "GET_STATUS";
	case USB_REQ_CLEAR_FEATURE: return "CLEAR_FEATURE";
	case USB_REQ_SET_FEATURE: return "SET_FEATURE";
	case USB_REQ_SET_ADDRESS: return "SET_ADDRESS";
	case USB_REQ_GET_DESCRIPTOR: return "GET_DESCRIPTOR";
	case USB_REQ_SET_DESCRIPTOR: return "SET_DESCRIPTOR";
	case USB_REQ_GET_CONFIGURATION: return "GET_CONFIGURATION";
	case USB_REQ_SET_CONFIGURATION: return "SET_CONFIGURATION";
	case USB_REQ_GET_INTERFACE: return "GET_INTERFACE";
	case USB_REQ_SET_INTERFACE: return "SET_INTERFACE";
	default: return "*UNKNOWN*";
	}
}
#endif

static struct usb_endpoint_descriptor ep0_desc = {
	.bLength = sizeof(struct usb_endpoint_descriptor),
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = USB_DIR_IN,
	.bmAttributes =	USB_ENDPOINT_XFER_CONTROL,
};

static int ci_pullup(struct usb_gadget *gadget, int is_on);
static int ci_ep_enable(struct usb_ep *ep,
		const struct usb_endpoint_descriptor *desc);
static int ci_ep_disable(struct usb_ep *ep);
static int ci_ep_queue(struct usb_ep *ep,
		struct usb_request *req, gfp_t gfp_flags);
static int ci_ep_dequeue(struct usb_ep *ep, struct usb_request *req);
static struct usb_request *
ci_ep_alloc_request(struct usb_ep *ep, unsigned int gfp_flags);
static void ci_ep_free_request(struct usb_ep *ep, struct usb_request *_req);
#if CONFIG_IS_ENABLED(DM_USB_GADGET)
static int ci_udc_gadget_start(struct usb_gadget *g,
			       struct usb_gadget_driver *driver);
static int ci_udc_gadget_stop(struct usb_gadget *g);
#endif

static struct usb_gadget_ops ci_udc_ops = {
	.pullup = ci_pullup,
#if CONFIG_IS_ENABLED(DM_USB_GADGET)
	.udc_start		= ci_udc_gadget_start,
	.udc_stop		= ci_udc_gadget_stop,
#endif
};

static struct usb_ep_ops ci_ep_ops = {
	.enable         = ci_ep_enable,
	.disable        = ci_ep_disable,
	.queue          = ci_ep_queue,
	.dequeue	= ci_ep_dequeue,
	.alloc_request  = ci_ep_alloc_request,
	.free_request   = ci_ep_free_request,
};

__weak void ci_init_after_reset(struct ehci_ctrl *ctrl)
{
}

/* Init values for USB endpoints. */
static const struct usb_ep ci_ep_init[5] = {
	[0] = {	/* EP 0 */
		.maxpacket	= 64,
		.name		= "ep0",
		.ops		= &ci_ep_ops,
	},
	[1] = {
		.maxpacket	= 512,
		.name		= "ep1in-bulk",
		.ops		= &ci_ep_ops,
	},
	[2] = {
		.maxpacket	= 512,
		.name		= "ep2out-bulk",
		.ops		= &ci_ep_ops,
	},
	[3] = {
		.maxpacket	= 512,
		.name		= "ep3in-int",
		.ops		= &ci_ep_ops,
	},
	[4] = {
		.maxpacket	= 512,
		.name		= "ep-",
		.ops		= &ci_ep_ops,
	},
};

static struct ci_drv controller = {
	.gadget	= {
		.name	= "ci_udc",
		.ops	= &ci_udc_ops,
		.is_dualspeed = 1,
		.max_speed = USB_SPEED_HIGH,
	},
};

/**
 * ci_get_qh() - return queue head for endpoint
 * @ep_num:	Endpoint number
 * @dir_in:	Direction of the endpoint (IN = 1, OUT = 0)
 *
 * This function returns the QH associated with particular endpoint
 * and it's direction.
 */
static struct ept_queue_head *ci_get_qh(int ep_num, int dir_in)
{
	return &controller.epts[(ep_num * 2) + dir_in];
}

/**
 * ci_get_qtd() - return queue item for endpoint
 * @ep_num:	Endpoint number
 * @dir_in:	Direction of the endpoint (IN = 1, OUT = 0)
 *
 * This function returns the QH associated with particular endpoint
 * and it's direction.
 */
static struct ept_queue_item *ci_get_qtd(int ep_num, int dir_in)
{
	int index = (ep_num * 2) + dir_in;
	uint8_t *imem = controller.items_mem + (index * ILIST_ENT_SZ);
	return (struct ept_queue_item *)imem;
}

/**
 * ci_flush_qh - flush cache over queue head
 * @ep_num:	Endpoint number
 *
 * This function flushes cache over QH for particular endpoint.
 */
static void ci_flush_qh(int ep_num)
{
	struct ept_queue_head *head = ci_get_qh(ep_num, 0);
	const unsigned long start = (unsigned long)head;
	const unsigned long end = start + 2 * sizeof(*head);

	flush_dcache_range(start, end);
}

/**
 * ci_invalidate_qh - invalidate cache over queue head
 * @ep_num:	Endpoint number
 *
 * This function invalidates cache over QH for particular endpoint.
 */
static void ci_invalidate_qh(int ep_num)
{
	struct ept_queue_head *head = ci_get_qh(ep_num, 0);
	unsigned long start = (unsigned long)head;
	unsigned long end = start + 2 * sizeof(*head);

	invalidate_dcache_range(start, end);
}

/**
 * ci_flush_qtd - flush cache over queue item
 * @ep_num:	Endpoint number
 *
 * This function flushes cache over qTD pair for particular endpoint.
 */
static void ci_flush_qtd(int ep_num)
{
	struct ept_queue_item *item = ci_get_qtd(ep_num, 0);
	const unsigned long start = (unsigned long)item;
	const unsigned long end = start + 2 * ILIST_ENT_SZ;

	flush_dcache_range(start, end);
}

/**
 * ci_flush_td - flush cache over queue item
 * @td:	td pointer
 *
 * This function flushes cache for particular transfer descriptor.
 */
static void ci_flush_td(struct ept_queue_item *td)
{
	const unsigned long start = (unsigned long)td;
	const unsigned long end = (unsigned long)td + ILIST_ENT_SZ;
	flush_dcache_range(start, end);
}

/**
 * ci_invalidate_qtd - invalidate cache over queue item
 * @ep_num:	Endpoint number
 *
 * This function invalidates cache over qTD pair for particular endpoint.
 */
static void ci_invalidate_qtd(int ep_num)
{
	struct ept_queue_item *item = ci_get_qtd(ep_num, 0);
	const unsigned long start = (unsigned long)item;
	const unsigned long end = start + 2 * ILIST_ENT_SZ;

	invalidate_dcache_range(start, end);
}

/**
 * ci_invalidate_td - invalidate cache over queue item
 * @td:	td pointer
 *
 * This function invalidates cache for particular transfer descriptor.
 */
static void ci_invalidate_td(struct ept_queue_item *td)
{
	const unsigned long start = (unsigned long)td;
	const unsigned long end = start + ILIST_ENT_SZ;
	invalidate_dcache_range(start, end);
}

static struct usb_request *
ci_ep_alloc_request(struct usb_ep *ep, unsigned int gfp_flags)
{
	struct ci_ep *ci_ep = container_of(ep, struct ci_ep, ep);
	int num = -1;
	struct ci_req *ci_req;

	if (ci_ep->desc)
		num = ci_ep->desc->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK;

	if (num == 0 && controller.ep0_req)
		return &controller.ep0_req->req;

	ci_req = calloc(1, sizeof(*ci_req));
	if (!ci_req)
		return NULL;

	INIT_LIST_HEAD(&ci_req->queue);

	if (num == 0)
		controller.ep0_req = ci_req;

	return &ci_req->req;
}

static void ci_ep_free_request(struct usb_ep *ep, struct usb_request *req)
{
	struct ci_ep *ci_ep = container_of(ep, struct ci_ep, ep);
	struct ci_req *ci_req = container_of(req, struct ci_req, req);
	int num = -1;

	if (ci_ep->desc)
		num = ci_ep->desc->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK;

	if (num == 0) {
		if (!controller.ep0_req)
			return;
		controller.ep0_req = 0;
	}

	if (ci_req->b_buf)
		free(ci_req->b_buf);
	free(ci_req);
}

static void ep_enable(int num, int in, int maxpacket)
{
	struct ci_udc *udc = (struct ci_udc *)controller.ctrl->hcor;
	unsigned n;

	n = readl(&udc->epctrl[num]);
	if (in)
		n |= (CTRL_TXE | CTRL_TXR | CTRL_TXT_BULK);
	else
		n |= (CTRL_RXE | CTRL_RXR | CTRL_RXT_BULK);

	if (num != 0) {
		struct ept_queue_head *head = ci_get_qh(num, in);

		head->config = CONFIG_MAX_PKT(maxpacket) | CONFIG_ZLT;
		ci_flush_qh(num);
	}
	writel(n, &udc->epctrl[num]);
}

static int ci_ep_enable(struct usb_ep *ep,
		const struct usb_endpoint_descriptor *desc)
{
	struct ci_ep *ci_ep = container_of(ep, struct ci_ep, ep);
	int num, in;
	num = desc->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK;
	in = (desc->bEndpointAddress & USB_DIR_IN) != 0;
	ci_ep->desc = desc;
	ep->desc = desc;

	if (num) {
		int max = get_unaligned_le16(&desc->wMaxPacketSize);

		if ((max > 64) && (controller.gadget.speed == USB_SPEED_FULL))
			max = 64;
		if (ep->maxpacket != max) {
			DBG("%s: from %d to %d\n", __func__,
			    ep->maxpacket, max);
			ep->maxpacket = max;
		}
	}
	ep_enable(num, in, ep->maxpacket);
	DBG("%s: num=%d maxpacket=%d\n", __func__, num, ep->maxpacket);
	return 0;
}

static int ci_ep_disable(struct usb_ep *ep)
{
	struct ci_ep *ci_ep = container_of(ep, struct ci_ep, ep);

	ci_ep->desc = NULL;
	ep->desc = NULL;
	return 0;
}

static int ci_bounce(struct ci_req *ci_req, int in)
{
	struct usb_request *req = &ci_req->req;
	unsigned long addr = (unsigned long)req->buf;
	unsigned long hwaddr;
	uint32_t aligned_used_len;

	/* Input buffer address is not aligned. */
	if (addr & (ARCH_DMA_MINALIGN - 1))
		goto align;

	/* Input buffer length is not aligned. */
	if (req->length & (ARCH_DMA_MINALIGN - 1))
		goto align;

	/* The buffer is well aligned, only flush cache. */
	ci_req->hw_len = req->length;
	ci_req->hw_buf = req->buf;
	goto flush;

align:
	if (ci_req->b_buf && req->length > ci_req->b_len) {
		free(ci_req->b_buf);
		ci_req->b_buf = 0;
	}
	if (!ci_req->b_buf) {
		ci_req->b_len = roundup(req->length, ARCH_DMA_MINALIGN);
		ci_req->b_buf = memalign(ARCH_DMA_MINALIGN, ci_req->b_len);
		if (!ci_req->b_buf)
			return -ENOMEM;
	}
	ci_req->hw_len = ci_req->b_len;
	ci_req->hw_buf = ci_req->b_buf;

	if (in)
		memcpy(ci_req->hw_buf, req->buf, req->length);

flush:
	hwaddr = (unsigned long)ci_req->hw_buf;
	aligned_used_len = roundup(req->length, ARCH_DMA_MINALIGN);
	flush_dcache_range(hwaddr, hwaddr + aligned_used_len);

	return 0;
}

static void ci_debounce(struct ci_req *ci_req, int in)
{
	struct usb_request *req = &ci_req->req;
	unsigned long addr = (unsigned long)req->buf;
	unsigned long hwaddr = (unsigned long)ci_req->hw_buf;
	uint32_t aligned_used_len;

	if (in)
		return;

	aligned_used_len = roundup(req->actual, ARCH_DMA_MINALIGN);
	invalidate_dcache_range(hwaddr, hwaddr + aligned_used_len);

	if (addr == hwaddr)
		return; /* not a bounce */

	memcpy(req->buf, ci_req->hw_buf, req->actual);
}

static void ci_ep_submit_next_request(struct ci_ep *ci_ep)
{
	struct ci_udc *udc = (struct ci_udc *)controller.ctrl->hcor;
	struct ept_queue_item *item;
	struct ept_queue_head *head;
	int bit, num, len, in;
	struct ci_req *ci_req;
	u8 *buf;
	uint32_t len_left, len_this_dtd;
	struct ept_queue_item *dtd, *qtd;

	ci_ep->req_primed = true;

	num = ci_ep->desc->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK;
	in = (ci_ep->desc->bEndpointAddress & USB_DIR_IN) != 0;
	item = ci_get_qtd(num, in);
	head = ci_get_qh(num, in);

	ci_req = list_first_entry(&ci_ep->queue, struct ci_req, queue);
	len = ci_req->req.length;

	head->next = (unsigned long)item;
	head->info = 0;

	ci_req->dtd_count = 0;
	buf = ci_req->hw_buf;
	len_left = len;
	dtd = item;

	do {
		len_this_dtd = min(len_left, (unsigned)EP_MAX_LENGTH_TRANSFER);

		dtd->info = INFO_BYTES(len_this_dtd) | INFO_ACTIVE;
		dtd->page0 = (unsigned long)buf;
		dtd->page1 = ((unsigned long)buf & 0xfffff000) + 0x1000;
		dtd->page2 = ((unsigned long)buf & 0xfffff000) + 0x2000;
		dtd->page3 = ((unsigned long)buf & 0xfffff000) + 0x3000;
		dtd->page4 = ((unsigned long)buf & 0xfffff000) + 0x4000;

		len_left -= len_this_dtd;
		buf += len_this_dtd;

		if (len_left) {
			qtd = (struct ept_queue_item *)
			       memalign(ILIST_ALIGN, ILIST_ENT_SZ);
			dtd->next = (unsigned long)qtd;
			dtd = qtd;
			memset(dtd, 0, ILIST_ENT_SZ);
		}

		ci_req->dtd_count++;
	} while (len_left);

	item = dtd;
	/*
	 * When sending the data for an IN transaction, the attached host
	 * knows that all data for the IN is sent when one of the following
	 * occurs:
	 * a) A zero-length packet is transmitted.
	 * b) A packet with length that isn't an exact multiple of the ep's
	 *    maxpacket is transmitted.
	 * c) Enough data is sent to exactly fill the host's maximum expected
	 *    IN transaction size.
	 *
	 * One of these conditions MUST apply at the end of an IN transaction,
	 * or the transaction will not be considered complete by the host. If
	 * none of (a)..(c) already applies, then we must force (a) to apply
	 * by explicitly sending an extra zero-length packet.
	 */
	/*  IN    !a     !b                              !c */
	if (in && len && !(len % ci_ep->ep.maxpacket) && ci_req->req.zero) {
		/*
		 * Each endpoint has 2 items allocated, even though typically
		 * only 1 is used at a time since either an IN or an OUT but
		 * not both is queued. For an IN transaction, item currently
		 * points at the second of these items, so we know that we
		 * can use the other to transmit the extra zero-length packet.
		 */
		struct ept_queue_item *other_item = ci_get_qtd(num, 0);
		item->next = (unsigned long)other_item;
		item = other_item;
		item->info = INFO_ACTIVE;
	}

	item->next = TERMINATE;
	item->info |= INFO_IOC;

	ci_flush_qtd(num);

	item = (struct ept_queue_item *)(unsigned long)head->next;
	while (item->next != TERMINATE) {
		ci_flush_td((struct ept_queue_item *)(unsigned long)item->next);
		item = (struct ept_queue_item *)(unsigned long)item->next;
	}

	DBG("ept%d %s queue len %x, req %p, buffer %p\n",
	    num, in ? "in" : "out", len, ci_req, ci_req->hw_buf);
	ci_flush_qh(num);

	if (in)
		bit = EPT_TX(num);
	else
		bit = EPT_RX(num);

	writel(bit, &udc->epprime);
}

static int ci_ep_dequeue(struct usb_ep *_ep, struct usb_request *_req)
{
	struct ci_ep *ci_ep = container_of(_ep, struct ci_ep, ep);
	struct ci_req *ci_req;

	list_for_each_entry(ci_req, &ci_ep->queue, queue) {
		if (&ci_req->req == _req)
			break;
	}

	if (&ci_req->req != _req)
		return -EINVAL;

	list_del_init(&ci_req->queue);

	if (ci_req->req.status == -EINPROGRESS) {
		ci_req->req.status = -ECONNRESET;
		if (ci_req->req.complete)
			ci_req->req.complete(_ep, _req);
	}

	return 0;
}

static int ci_ep_queue(struct usb_ep *ep,
		struct usb_request *req, gfp_t gfp_flags)
{
	struct ci_ep *ci_ep = container_of(ep, struct ci_ep, ep);
	struct ci_req *ci_req = container_of(req, struct ci_req, req);
	int in, ret;
	int __maybe_unused num;

	num = ci_ep->desc->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK;
	in = (ci_ep->desc->bEndpointAddress & USB_DIR_IN) != 0;

	if (!num && ci_ep->req_primed) {
		/*
		 * The flipping of ep0 between IN and OUT relies on
		 * ci_ep_queue consuming the current IN/OUT setting
		 * immediately. If this is deferred to a later point when the
		 * req is pulled out of ci_req->queue, then the IN/OUT setting
		 * may have been changed since the req was queued, and state
		 * will get out of sync. This condition doesn't occur today,
		 * but could if bugs were introduced later, and this error
		 * check will save a lot of debugging time.
		 */
		printf("%s: ep0 transaction already in progress\n", __func__);
		return -EPROTO;
	}

	ret = ci_bounce(ci_req, in);
	if (ret)
		return ret;

	DBG("ept%d %s pre-queue req %p, buffer %p\n",
	    num, in ? "in" : "out", ci_req, ci_req->hw_buf);
	list_add_tail(&ci_req->queue, &ci_ep->queue);

	if (!ci_ep->req_primed)
		ci_ep_submit_next_request(ci_ep);

	return 0;
}

static void flip_ep0_direction(void)
{
	if (ep0_desc.bEndpointAddress == USB_DIR_IN) {
		DBG("%s: Flipping ep0 to OUT\n", __func__);
		ep0_desc.bEndpointAddress = 0;
	} else {
		DBG("%s: Flipping ep0 to IN\n", __func__);
		ep0_desc.bEndpointAddress = USB_DIR_IN;
	}
}

static void handle_ep_complete(struct ci_ep *ci_ep)
{
	struct ept_queue_item *item, *next_td;
	int num, in, len, j;
	struct ci_req *ci_req;

	num = ci_ep->desc->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK;
	in = (ci_ep->desc->bEndpointAddress & USB_DIR_IN) != 0;
	item = ci_get_qtd(num, in);
	ci_invalidate_qtd(num);
	ci_req = list_first_entry(&ci_ep->queue, struct ci_req, queue);

	next_td = item;
	len = 0;
	for (j = 0; j < ci_req->dtd_count; j++) {
		ci_invalidate_td(next_td);
		item = next_td;
		len += (item->info >> 16) & 0x7fff;
		if (item->info & 0xff)
			printf("EP%d/%s FAIL info=%x pg0=%x\n",
			       num, in ? "in" : "out", item->info, item->page0);
		if (j != ci_req->dtd_count - 1)
			next_td = (struct ept_queue_item *)(unsigned long)
				item->next;
		if (j != 0)
			free(item);
	}

	list_del_init(&ci_req->queue);
	ci_ep->req_primed = false;

	if (!list_empty(&ci_ep->queue))
		ci_ep_submit_next_request(ci_ep);

	ci_req->req.actual = ci_req->req.length - len;
	ci_debounce(ci_req, in);

	DBG("ept%d %s req %p, complete %x\n",
	    num, in ? "in" : "out", ci_req, len);
	if (num != 0 || controller.ep0_data_phase)
		ci_req->req.complete(&ci_ep->ep, &ci_req->req);
	if (num == 0 && controller.ep0_data_phase) {
		/*
		 * Data Stage is complete, so flip ep0 dir for Status Stage,
		 * which always transfers a packet in the opposite direction.
		 */
		DBG("%s: flip ep0 dir for Status Stage\n", __func__);
		flip_ep0_direction();
		controller.ep0_data_phase = false;
		ci_req->req.length = 0;
		usb_ep_queue(&ci_ep->ep, &ci_req->req, 0);
	}
}

#define SETUP(type, request) (((type) << 8) | (request))

static void handle_setup(void)
{
	struct ci_ep *ci_ep = &controller.ep[0];
	struct ci_req *ci_req;
	struct usb_request *req;
	struct ci_udc *udc = (struct ci_udc *)controller.ctrl->hcor;
	struct ept_queue_head *head;
	struct usb_ctrlrequest r;
	int status = 0;
	int num, in, _num, _in, i;
	char *buf;

	ci_req = controller.ep0_req;
	req = &ci_req->req;
	head = ci_get_qh(0, 0);	/* EP0 OUT */

	ci_invalidate_qh(0);
	memcpy(&r, head->setup_data, sizeof(struct usb_ctrlrequest));
#ifdef CONFIG_CI_UDC_HAS_HOSTPC
	writel(EPT_RX(0), &udc->epsetupstat);
#else
	writel(EPT_RX(0), &udc->epstat);
#endif
	DBG("handle setup %s, %x, %x index %x value %x length %x\n",
	    reqname(r.bRequest), r.bRequestType, r.bRequest, r.wIndex,
	    r.wValue, r.wLength);

	/* Set EP0 dir for Data Stage based on Setup Stage data */
	if (r.bRequestType & USB_DIR_IN) {
		DBG("%s: Set ep0 to IN for Data Stage\n", __func__);
		ep0_desc.bEndpointAddress = USB_DIR_IN;
	} else {
		DBG("%s: Set ep0 to OUT for Data Stage\n", __func__);
		ep0_desc.bEndpointAddress = 0;
	}
	if (r.wLength) {
		controller.ep0_data_phase = true;
	} else {
		/* 0 length -> no Data Stage. Flip dir for Status Stage */
		DBG("%s: 0 length: flip ep0 dir for Status Stage\n", __func__);
		flip_ep0_direction();
		controller.ep0_data_phase = false;
	}

	list_del_init(&ci_req->queue);
	ci_ep->req_primed = false;

	switch (SETUP(r.bRequestType, r.bRequest)) {
	case SETUP(USB_RECIP_ENDPOINT, USB_REQ_CLEAR_FEATURE):
		_num = r.wIndex & 15;
		_in = !!(r.wIndex & 0x80);

		if ((r.wValue == 0) && (r.wLength == 0)) {
			req->length = 0;
			for (i = 0; i < NUM_ENDPOINTS; i++) {
				struct ci_ep *ep = &controller.ep[i];

				if (!ep->desc)
					continue;
				num = ep->desc->bEndpointAddress
						& USB_ENDPOINT_NUMBER_MASK;
				in = (ep->desc->bEndpointAddress
						& USB_DIR_IN) != 0;
				if ((num == _num) && (in == _in)) {
					ep_enable(num, in, ep->ep.maxpacket);
					usb_ep_queue(controller.gadget.ep0,
							req, 0);
					break;
				}
			}
		}
		return;

	case SETUP(USB_RECIP_DEVICE, USB_REQ_SET_ADDRESS):
		/*
		 * write address delayed (will take effect
		 * after the next IN txn)
		 */
		writel((r.wValue << 25) | (1 << 24), &udc->devaddr);
		req->length = 0;
		usb_ep_queue(controller.gadget.ep0, req, 0);
		return;

	case SETUP(USB_DIR_IN | USB_RECIP_DEVICE, USB_REQ_GET_STATUS):
		req->length = 2;
		buf = (char *)req->buf;
		buf[0] = 1 << USB_DEVICE_SELF_POWERED;
		buf[1] = 0;
		usb_ep_queue(controller.gadget.ep0, req, 0);
		return;
	}
	/* pass request up to the gadget driver */
	if (controller.driver)
		status = controller.driver->setup(&controller.gadget, &r);
	else
		status = -ENODEV;

	if (!status)
		return;
	DBG("STALL reqname %s type %x value %x, index %x\n",
	    reqname(r.bRequest), r.bRequestType, r.wValue, r.wIndex);
	writel((1<<16) | (1 << 0), &udc->epctrl[0]);
}

static void stop_activity(void)
{
	int i, num, in;
	struct ept_queue_head *head;
	struct ci_udc *udc = (struct ci_udc *)controller.ctrl->hcor;
	writel(readl(&udc->epcomp), &udc->epcomp);
#ifdef CONFIG_CI_UDC_HAS_HOSTPC
	writel(readl(&udc->epsetupstat), &udc->epsetupstat);
#endif
	writel(readl(&udc->epstat), &udc->epstat);
	writel(0xffffffff, &udc->epflush);

	/* error out any pending reqs */
	for (i = 0; i < NUM_ENDPOINTS; i++) {
		if (i != 0)
			writel(0, &udc->epctrl[i]);
		if (controller.ep[i].desc) {
			num = controller.ep[i].desc->bEndpointAddress
				& USB_ENDPOINT_NUMBER_MASK;
			in = (controller.ep[i].desc->bEndpointAddress
				& USB_DIR_IN) != 0;
			head = ci_get_qh(num, in);
			head->info = INFO_ACTIVE;
			ci_flush_qh(num);
		}
	}
}

void udc_irq(void)
{
	struct ci_udc *udc = (struct ci_udc *)controller.ctrl->hcor;
	unsigned n = readl(&udc->usbsts);
	writel(n, &udc->usbsts);
	int bit, i, num, in;

	n &= (STS_SLI | STS_URI | STS_PCI | STS_UI | STS_UEI);
	if (n == 0)
		return;

	if (n & STS_URI) {
		DBG("-- reset --\n");
		stop_activity();
	}
	if (n & STS_SLI)
		DBG("-- suspend --\n");

	if (n & STS_PCI) {
		int max = 64;
		int speed = USB_SPEED_FULL;

#ifdef CONFIG_CI_UDC_HAS_HOSTPC
		bit = (readl(&udc->hostpc1_devlc) >> 25) & 3;
#else
		bit = (readl(&udc->portsc) >> 26) & 3;
#endif
		DBG("-- portchange %x %s\n", bit, (bit == 2) ? "High" : "Full");
		if (bit == 2) {
			speed = USB_SPEED_HIGH;
			max = 512;
		}
		controller.gadget.speed = speed;
		for (i = 1; i < NUM_ENDPOINTS; i++) {
			if (controller.ep[i].ep.maxpacket > max)
				controller.ep[i].ep.maxpacket = max;
		}
	}

	if (n & STS_UEI)
		printf("<UEI %x>\n", readl(&udc->epcomp));

	if ((n & STS_UI) || (n & STS_UEI)) {
#ifdef CONFIG_CI_UDC_HAS_HOSTPC
		n = readl(&udc->epsetupstat);
#else
		n = readl(&udc->epstat);
#endif
		if (n & EPT_RX(0))
			handle_setup();

		n = readl(&udc->epcomp);
		if (n != 0)
			writel(n, &udc->epcomp);

		for (i = 0; i < NUM_ENDPOINTS && n; i++) {
			if (controller.ep[i].desc) {
				num = controller.ep[i].desc->bEndpointAddress
					& USB_ENDPOINT_NUMBER_MASK;
				in = (controller.ep[i].desc->bEndpointAddress
						& USB_DIR_IN) != 0;
				bit = (in) ? EPT_TX(num) : EPT_RX(num);
				if (n & bit)
					handle_ep_complete(&controller.ep[i]);
			}
		}
	}
}

int ci_udc_handle_interrupts(void)
{
	u32 value;
	struct ci_udc *udc = (struct ci_udc *)controller.ctrl->hcor;

	value = readl(&udc->usbsts);
	if (value)
		udc_irq();

	return value;
}

void udc_disconnect(void)
{
	struct ci_udc *udc = (struct ci_udc *)controller.ctrl->hcor;
	/* disable pullup */
	stop_activity();
	writel(USBCMD_FS2, &udc->usbcmd);
	udelay(800);
	if (controller.driver)
		controller.driver->disconnect(&controller.gadget);
}

static int ci_pullup(struct usb_gadget *gadget, int is_on)
{
	struct ci_udc *udc = (struct ci_udc *)controller.ctrl->hcor;
	if (is_on) {
		/* RESET */
		writel(USBCMD_ITC(MICRO_8FRAME) | USBCMD_RST, &udc->usbcmd);
		udelay(200);

		ci_init_after_reset(controller.ctrl);

		writel((unsigned long)controller.epts, &udc->epinitaddr);

		/* select DEVICE mode */
		writel(USBMODE_DEVICE, &udc->usbmode);

#if !defined(CONFIG_USB_GADGET_DUALSPEED)
		/* Port force Full-Speed Connect */
		setbits_le32(&udc->portsc, PFSC);
#endif

		writel(0xffffffff, &udc->epflush);

		/* Turn on the USB connection by enabling the pullup resistor */
		setbits_le32(&udc->usbcmd, USBCMD_ITC(MICRO_8FRAME) |
			     USBCMD_RUN);
	} else {
		udc_disconnect();
	}

	return 0;
}

static int ci_udc_probe(void)
{
	struct ept_queue_head *head;
	int i;

	const int num = 2 * NUM_ENDPOINTS;

	const int eplist_min_align = 4096;
	const int eplist_align = roundup(eplist_min_align, ARCH_DMA_MINALIGN);
	const int eplist_raw_sz = num * sizeof(struct ept_queue_head);
	const int eplist_sz = roundup(eplist_raw_sz, ARCH_DMA_MINALIGN);

	/* The QH list must be aligned to 4096 bytes. */
	controller.epts = memalign(eplist_align, eplist_sz);
	if (!controller.epts)
		return -ENOMEM;
	memset(controller.epts, 0, eplist_sz);

	controller.items_mem = memalign(ILIST_ALIGN, ILIST_SZ);
	if (!controller.items_mem) {
		free(controller.epts);
		return -ENOMEM;
	}
	memset(controller.items_mem, 0, ILIST_SZ);

	for (i = 0; i < 2 * NUM_ENDPOINTS; i++) {
		/*
		 * Configure QH for each endpoint. The structure of the QH list
		 * is such that each two subsequent fields, N and N+1 where N is
		 * even, in the QH list represent QH for one endpoint. The Nth
		 * entry represents OUT configuration and the N+1th entry does
		 * represent IN configuration of the endpoint.
		 */
		head = controller.epts + i;
		if (i < 2)
			head->config = CONFIG_MAX_PKT(EP0_MAX_PACKET_SIZE)
				| CONFIG_ZLT | CONFIG_IOS;
		else
			head->config = CONFIG_MAX_PKT(EP_MAX_PACKET_SIZE)
				| CONFIG_ZLT;
		head->next = TERMINATE;
		head->info = 0;

		if (i & 1) {
			ci_flush_qh(i / 2);
			ci_flush_qtd(i / 2);
		}
	}

	INIT_LIST_HEAD(&controller.gadget.ep_list);

	/* Init EP 0 */
	memcpy(&controller.ep[0].ep, &ci_ep_init[0], sizeof(*ci_ep_init));
	controller.ep[0].desc = &ep0_desc;
	INIT_LIST_HEAD(&controller.ep[0].queue);
	controller.ep[0].req_primed = false;
	controller.gadget.ep0 = &controller.ep[0].ep;
	INIT_LIST_HEAD(&controller.gadget.ep0->ep_list);

	/* Init EP 1..3 */
	for (i = 1; i < 4; i++) {
		memcpy(&controller.ep[i].ep, &ci_ep_init[i],
		       sizeof(*ci_ep_init));
		INIT_LIST_HEAD(&controller.ep[i].queue);
		controller.ep[i].req_primed = false;
		list_add_tail(&controller.ep[i].ep.ep_list,
			      &controller.gadget.ep_list);
	}

	/* Init EP 4..n */
	for (i = 4; i < NUM_ENDPOINTS; i++) {
		memcpy(&controller.ep[i].ep, &ci_ep_init[4],
		       sizeof(*ci_ep_init));
		INIT_LIST_HEAD(&controller.ep[i].queue);
		controller.ep[i].req_primed = false;
		list_add_tail(&controller.ep[i].ep.ep_list,
			      &controller.gadget.ep_list);
	}

	ci_ep_alloc_request(&controller.ep[0].ep, 0);
	if (!controller.ep0_req) {
		free(controller.items_mem);
		free(controller.epts);
		return -ENOMEM;
	}

	return 0;
}

bool dfu_usb_get_reset(void)
{
	struct ci_udc *udc = (struct ci_udc *)controller.ctrl->hcor;

	return !!(readl(&udc->usbsts) & STS_URI);
}

static int ci_udc_otg_phy_mode2(ulong phy_addr)
{
	void *__iomem phy_ctrl, *__iomem phy_status;
	void *__iomem phy_base = (void *__iomem)phy_addr;
	u32 val;

	if (is_mx6() || is_mx7ulp() || is_imx8()) {
		phy_ctrl = (void __iomem *)(phy_base + USBPHY_CTRL);
		val = readl(phy_ctrl);
		if (val & USBPHY_CTRL_OTG_ID)
			return USB_INIT_DEVICE;
		else
			return USB_INIT_HOST;
	} else if (is_mx7() || is_imx8mm() || is_imx8mn()) {
		phy_status = (void __iomem *)(phy_base +
					      USBNC_PHY_STATUS_OFFSET);
		val = readl(phy_status);
		if (val & USBNC_PHYSTATUS_ID_DIG)
			return USB_INIT_DEVICE;
		else
			return USB_INIT_HOST;
	} else {
		return -EINVAL;
	}
}

bool udc_irq_reset(void)
{
	struct ci_udc *udc = (struct ci_udc *)controller.ctrl->hcor;
	unsigned n = readl(&udc->usbsts);
	writel(n, &udc->usbsts);

	n &= (STS_SLI | STS_URI | STS_PCI | STS_UI | STS_UEI);
	if (n == 0)
		return false;

	if (n & STS_URI) {
		DBG("-- reset --\n");
		return true;
	}

	if (n & STS_SLI)
		DBG("-- suspend --\n");

	return false;
}

bool ci_udc_check_bus_active(ulong ehci_addr, ulong phy_addr, int index)
{
	struct usb_ehci *ehci = (struct usb_ehci *)ehci_addr;
	struct ehci_ctrl ctrl;
	int ret;
	bool active = false;

	ret = ehci_mx6_common_init(ehci, index);
	if (ret)
		return false;

	if (ci_udc_otg_phy_mode2(phy_addr) != USB_INIT_DEVICE)
		return false;

	ctrl.hccr = (struct ehci_hccr *)((ulong)&ehci->caplength);
	ctrl.hcor = (struct ehci_hcor *)((ulong)ctrl.hccr +
			HC_LENGTH(ehci_readl(&(ctrl.hccr)->cr_capbase)));
	controller.ctrl = &ctrl;

	ret = ci_udc_probe();
	if (ret) {
		return false;
	}

	ci_pullup(NULL, 1);

	int count = 100;
	while (count > 0) {
		if (udc_irq_reset()) {
			active = true;
			break;
		}
		mdelay(10);
		count--;
	}

	ci_pullup(NULL, 0);

	ci_ep_free_request(&controller.ep[0].ep, &controller.ep0_req->req);
	free(controller.items_mem);
	free(controller.epts);

	return active;
}


#if !CONFIG_IS_ENABLED(DM_USB_GADGET)
int usb_gadget_handle_interrupts(int index)
{
	return ci_udc_handle_interrupts();
}

int usb_gadget_register_driver(struct usb_gadget_driver *driver)
{
	int ret;

	if (!driver)
		return -EINVAL;
	if (!driver->bind || !driver->setup || !driver->disconnect)
		return -EINVAL;

#if CONFIG_IS_ENABLED(DM_USB)
	ret = usb_setup_ehci_gadget(&controller.ctrl);
#else
	ret = usb_lowlevel_init(0, USB_INIT_DEVICE, (void **)&controller.ctrl);
#endif
	if (ret)
		return ret;

	ret = ci_udc_probe();
	if (ret) {
		DBG("udc probe failed, returned %d\n", ret);
		return ret;
	}

	ret = driver->bind(&controller.gadget);
	if (ret) {
		DBG("driver->bind() returned %d\n", ret);
		return ret;
	}
	controller.driver = driver;

	return 0;
}

int usb_gadget_unregister_driver(struct usb_gadget_driver *driver)
{
	udc_disconnect();

	driver->unbind(&controller.gadget);
	controller.driver = NULL;

	ci_ep_free_request(&controller.ep[0].ep, &controller.ep0_req->req);
	free(controller.items_mem);
	free(controller.epts);

#if CONFIG_IS_ENABLED(DM_USB)
	usb_remove_ehci_gadget(&controller.ctrl);
#else
	usb_lowlevel_stop(0);
	controller.ctrl = NULL;
#endif

	return 0;
}
#else /* !CONFIG_IS_ENABLED(DM_USB_GADGET) */

static int ci_udc_gadget_start(struct usb_gadget *g,
			       struct usb_gadget_driver *driver)
{
	if (!driver)
		return -EINVAL;
	if (!driver->bind || !driver->setup || !driver->disconnect)
		return -EINVAL;

	controller.driver = driver;
	return 0;
}

static int ci_udc_gadget_stop(struct usb_gadget *g)
{
	controller.driver = NULL;

	ci_ep_free_request(&controller.ep[0].ep, &controller.ep0_req->req);
	free(controller.items_mem);
	free(controller.epts);
	return 0;
}

struct ci_udc_priv_data {
	struct ehci_ctrl ctrl;
	struct udevice otgdev;
	struct clk_bulk		clks;
	int phy_off;
	struct power_domain otg_pd;
	struct clk phy_clk;
	struct power_domain phy_pd;
};

static int ci_udc_gadget_handle_interrupts(struct udevice *dev)
{
	return ci_udc_handle_interrupts();
}

static int ci_udc_phy_setup(struct udevice *dev, struct ci_udc_priv_data *priv)
{
	struct udevice __maybe_unused phy_dev;
	priv->phy_off = fdtdec_lookup_phandle(gd->fdt_blob,
					      dev_of_offset(dev),
					      "fsl,usbphy");
	if (priv->phy_off < 0)
		return -EINVAL;

	phy_dev.node = offset_to_ofnode(priv->phy_off);

#if CONFIG_IS_ENABLED(POWER_DOMAIN)
	/* Need to power on the PHY before access it */
	if (!power_domain_get(&phy_dev, &priv->phy_pd)) {
		if (power_domain_on(&priv->phy_pd))
			return -EINVAL;
	}
#endif

#if CONFIG_IS_ENABLED(CLK)
	int ret;

	ret = clk_get_by_index(&phy_dev, 0, &priv->phy_clk);
	if (ret) {
		printf("Failed to get phy_clk\n");
		return ret;
	}

	ret = clk_enable(&priv->phy_clk);
	if (ret) {
		printf("Failed to enable phy_clk\n");
		return ret;
	}
#endif

	return 0;
}

static int ci_udc_phy_shutdown(struct ci_udc_priv_data *priv)
{
	int ret = 0;

#if CONFIG_IS_ENABLED(CLK)
	if (priv->phy_clk.dev) {
		ret = clk_disable(&priv->phy_clk);
		if (ret)
			return ret;

		ret = clk_free(&priv->phy_clk);
		if (ret)
			return ret;
	}
#endif

#if CONFIG_IS_ENABLED(POWER_DOMAIN)
	ret = power_domain_off(&priv->phy_pd);
	if (ret)
		printf("Power down USB PHY failed! (error = %d)\n", ret);
#endif
	return ret;
}

static int ci_udc_otg_clk_init(struct udevice *dev,
			       struct clk_bulk *clks)
{
	int ret;

	ret = clk_get_bulk(dev, clks);
	if (ret == -ENOSYS)
		return 0;

	if (ret)
		return ret;

#if CONFIG_IS_ENABLED(CLK)
	ret = clk_enable_bulk(clks);
	if (ret) {
		clk_release_bulk(clks);
		return ret;
	}
#endif

	return 0;
}

static int ci_udc_otg_phy_mode(struct udevice *dev)
{
	struct ci_udc_priv_data *priv = dev_get_priv(dev);

	void *__iomem phy_ctrl, *__iomem phy_status;
	void *__iomem phy_base = (void *__iomem)devfdt_get_addr(&priv->otgdev);
	u32 val;

	if (is_mx6() || is_mx7ulp() || is_imx8()) {
		phy_base = (void __iomem *)fdtdec_get_addr(gd->fdt_blob,
							   priv->phy_off,
							   "reg");
		if ((fdt_addr_t)phy_base == FDT_ADDR_T_NONE)
			return -EINVAL;

		phy_ctrl = (void __iomem *)(phy_base + USBPHY_CTRL);
		val = readl(phy_ctrl);
		if (val & USBPHY_CTRL_OTG_ID)
			return USB_INIT_DEVICE;
		else
			return USB_INIT_HOST;
	} else if (is_mx7() || is_imx8mm() || is_imx8mn()) {
		phy_status = (void __iomem *)(phy_base +
					      USBNC_PHY_STATUS_OFFSET);
		val = readl(phy_status);
		if (val & USBNC_PHYSTATUS_ID_DIG)
			return USB_INIT_DEVICE;
		else
			return USB_INIT_HOST;
	} else {
		return -EINVAL;
	}
}

static int ci_udc_otg_ofdata_to_platdata(struct udevice *dev)
{
	struct ci_udc_priv_data *priv = dev_get_priv(dev);
	int node = dev_of_offset(dev);
	int usbotg_off;

	if (usb_get_dr_mode(node) != USB_DR_MODE_PERIPHERAL) {
		dev_dbg(dev, "Invalid mode\n");
		return -ENODEV;
	}

	usbotg_off = fdtdec_lookup_phandle(gd->fdt_blob,
					   node,
					   "chipidea,usb");
	if (usbotg_off < 0)
		return -EINVAL;
	priv->otgdev.node = offset_to_ofnode(usbotg_off);
	priv->otgdev.parent = dev->parent;

	return 0;
}

static int ci_udc_otg_probe(struct udevice *dev)
{
	struct ci_udc_priv_data *priv = dev_get_priv(dev);
	struct usb_ehci *ehci;
	int ret;

	ehci = (struct usb_ehci *)devfdt_get_addr(&priv->otgdev);

	pinctrl_select_state(&priv->otgdev, "default");

#if defined(CONFIG_MX6)
	if (mx6_usb_fused((u32)ehci)) {
		printf("USB@0x%x is fused, disable it\n", (u32)ehci);
		return -ENODEV;
	}
#endif

	ret = board_usb_init(dev->seq, USB_INIT_DEVICE);
	if (ret) {
		printf("Failed to initialize board for USB\n");
		return ret;
	}

#if CONFIG_IS_ENABLED(POWER_DOMAIN)
	if (!power_domain_get(&priv->otgdev, &priv->otg_pd)) {
		if (power_domain_on(&priv->otg_pd))
			return -EINVAL;
	}
#endif

	ret = ci_udc_phy_setup(&priv->otgdev, priv);
	if (ret)
		return ret;

	ret = ci_udc_otg_clk_init(&priv->otgdev, &priv->clks);
	if (ret)
		return ret;

	ret = ehci_mx6_common_init(ehci, dev->seq);
	if (ret)
		return ret;

	if (ci_udc_otg_phy_mode(dev) != USB_INIT_DEVICE)
		return -ENODEV;

	priv->ctrl.hccr = (struct ehci_hccr *)((ulong)&ehci->caplength);
	priv->ctrl.hcor = (struct ehci_hcor *)((ulong)priv->ctrl.hccr +
			HC_LENGTH(ehci_readl(&(priv->ctrl.hccr)->cr_capbase)));
	controller.ctrl = &priv->ctrl;

	ret = ci_udc_probe();
	if (ret) {
		DBG("udc probe failed, returned %d\n", ret);
		return ret;
	}

	ret = usb_add_gadget_udc((struct device *)dev, &controller.gadget);

	return ret;
}

static int ci_udc_otg_remove(struct udevice *dev)
{
	struct ci_udc_priv_data *priv = dev_get_priv(dev);

	usb_del_gadget_udc(&controller.gadget);

	clk_release_bulk(&priv->clks);
	ci_udc_phy_shutdown(priv);
#if CONFIG_IS_ENABLED(POWER_DOMAIN)
	if (power_domain_off(&priv->otg_pd)) {
		printf("Power down USB controller failed!\n");
		return -EINVAL;
	}
#endif
	board_usb_cleanup(dev->seq, USB_INIT_DEVICE);

	controller.ctrl = NULL;
	return 0;
}

static const struct udevice_id ci_udc_otg_ids[] = {
	{ .compatible = "fsl,imx27-usb-gadget" },
	{ }
};

U_BOOT_DRIVER(ci_udc_otg) = {
	.name	= "ci-udc-otg",
	.id	= UCLASS_USB_GADGET_GENERIC,
	.of_match = ci_udc_otg_ids,
	.ofdata_to_platdata = ci_udc_otg_ofdata_to_platdata,
	.probe = ci_udc_otg_probe,
	.remove = ci_udc_otg_remove,
	.handle_interrupts = ci_udc_gadget_handle_interrupts,
	.priv_auto_alloc_size = sizeof(struct ci_udc_priv_data),
};

#endif /* !CONFIG_IS_ENABLED(DM_USB_GADGET) */
