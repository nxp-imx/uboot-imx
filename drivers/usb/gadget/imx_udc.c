/*
 * Copyright (C) 2010-2015 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <asm/types.h>
#include <malloc.h>
#include <command.h>
#include <asm/errno.h>
#include <usbdevice.h>
#include <usb/imx_udc.h>

#include "ep0.h"

#ifdef DEBUG
#define DBG(x...) printf(x)
#else
#define DBG(x...) do {} while (0)
#endif

#define mdelay(n) udelay((n)*1000)

#define EP_TQ_ITEM_SIZE 16

#define inc_index(x) (x = ((x+1) % EP_TQ_ITEM_SIZE))

#define ep_is_in(e, tx) ((e == 0) ? (mxc_udc.ep0_dir == USB_DIR_IN) : tx)

#define USB_RECIP_MASK	    0x03
#define USB_TYPE_MASK	    (0x03 << 5)
#define USB_MEM_ALIGN_BYTE  4096
#define USB_DEV_DQH_ALIGN   64
#define USB_DEV_DTD_ALIGN   64

/*fixed the dtd buffer to 4096 bytes, even it could be 20KB*/
#define USB_DEV_DTD_MAX_BUFFER_SIZE 4096

#define CACHE_ALIGNED_END(start, length) \
	(ALIGN((uint32_t)start + length, ARCH_DMA_MINALIGN))

struct mxc_ep_t{
	int epnum;
	int dir;
	/* change from int to u32 to make `min` happy */
	u32 max_pkt_size;
	struct usb_endpoint_instance *epi;
	struct ep_queue_item *ep_dtd[EP_TQ_ITEM_SIZE];
	int index; /* to index the free tx tqi */
	int done;  /* to index the complete rx tqi */
	struct ep_queue_item *tail; /* last item in the dtd chain */
	struct ep_queue_head *ep_qh;
} ;

struct mxc_udc_ctrl{
	int    max_ep;
	int    ep0_dir;
	int    setaddr;
	struct ep_queue_head *ep_qh;
	struct mxc_ep_t *mxc_ep;
	u32    qh_unaligned;
};

static int usb_highspeed;
static int usb_inited;
static struct mxc_udc_ctrl mxc_udc;
static struct usb_device_instance *udc_device;
static struct urb *ep0_urb;
/*
 * malloc an aligned memory
 * unaligned_addr: return a unaligned address for memory free
 * size   : memory size
 * align  : alignment for this memroy
 * return : aligned address(NULL when malloc failt)
*/
static void *malloc_aligned_buffer(u32 *unaligned_addr,
	int size, int align)
{
	int msize = (size + align  - 1);
	u32 vir, vir_align;

	/* force the allocated memory size to be aligned with min cache operation unit.
	*   So it is safe to flush/invalidate the cache.
	*/
	msize = (msize + ARCH_DMA_MINALIGN - 1) / ARCH_DMA_MINALIGN;
	msize = msize * ARCH_DMA_MINALIGN;

	vir = (u32)malloc(msize);
	memset((void *)vir, 0, msize);
	vir_align = (vir + align - 1) & (~(align - 1));
	*unaligned_addr = vir;

	DBG("alloc aligned vir addr %x\n", vir_align);
	return (void *)vir_align;
}

int is_usb_disconnected()
{
	int ret = 0;

	ret = readl(USB_OTGSC) & OTGSC_B_SESSION_VALID ? 0 : 1;
	return ret;
}

static int mxc_init_usb_qh(void)
{
	int size;
	memset(&mxc_udc, 0, sizeof(mxc_udc));
	mxc_udc.max_ep = (readl(USB_DCCPARAMS) & DCCPARAMS_DEN_MASK) * 2;
	DBG("udc max ep = %d\n", mxc_udc.max_ep);
	size = mxc_udc.max_ep * sizeof(struct ep_queue_head);
	mxc_udc.ep_qh = malloc_aligned_buffer(&mxc_udc.qh_unaligned,
					     size, USB_MEM_ALIGN_BYTE);
	if (!mxc_udc.ep_qh) {
		printf("malloc ep qh dma buffer failure\n");
		return -1;
	}
	memset(mxc_udc.ep_qh, 0, size);

	/*flush cache to physical memory*/
	flush_dcache_range((unsigned long)mxc_udc.ep_qh,
		CACHE_ALIGNED_END(mxc_udc.ep_qh, size));

	writel(virt_to_phys(mxc_udc.ep_qh) & 0xfffff800, USB_ENDPOINTLISTADDR);
	return 0;
}

static int mxc_destroy_usb_qh(void)
{
	if (mxc_udc.ep_qh && mxc_udc.qh_unaligned) {
		free((void *)mxc_udc.qh_unaligned);
		mxc_udc.ep_qh = 0;
		mxc_udc.qh_unaligned = 0;
		mxc_udc.max_ep = 0;
	}

	return 0;
}

static int mxc_init_ep_struct(void)
{
	int i;

	DBG("init mxc ep\n");
	mxc_udc.mxc_ep = malloc(mxc_udc.max_ep * sizeof(struct mxc_ep_t));
	if (!mxc_udc.mxc_ep) {
		printf("malloc ep struct failure\n");
		return -1;
	}
	memset((void *)mxc_udc.mxc_ep, 0,
		sizeof(struct mxc_ep_t) * mxc_udc.max_ep);
	for (i = 0; i < mxc_udc.max_ep / 2; i++) {
		struct mxc_ep_t *ep;
		ep  = mxc_udc.mxc_ep + i * 2;
		ep->epnum = i;
		ep->index = ep->done = 0;
		ep->dir = USB_RECV;  /* data from host to device */
		ep->ep_qh = &mxc_udc.ep_qh[i * 2];

		ep  = mxc_udc.mxc_ep + (i * 2 + 1);
		ep->epnum = i;
		ep->index = ep->done = 0;
		ep->dir = USB_SEND;  /* data to host from device */
		ep->ep_qh = &mxc_udc.ep_qh[(i * 2 + 1)];
	}
	return 0;
}

static int mxc_destroy_ep_struct(void)
{
	if (mxc_udc.mxc_ep) {
		free(mxc_udc.mxc_ep);
		mxc_udc.mxc_ep = 0;
	}
	return 0;
}

static int mxc_init_ep_dtd(u8 index)
{
	struct mxc_ep_t *ep;
	u32 unaligned_addr;
	int i;

	if (index >= mxc_udc.max_ep)
		DBG("%s ep %d is not valid\n", __func__, index);

	ep = mxc_udc.mxc_ep + index;
	for (i = 0; i < EP_TQ_ITEM_SIZE; i++) {
		ep->ep_dtd[i] = malloc_aligned_buffer(&unaligned_addr,
			    sizeof(struct ep_queue_item), USB_DEV_DTD_ALIGN);
		ep->ep_dtd[i]->item_unaligned_addr = unaligned_addr;

		if (NULL == ep->ep_dtd[i]) {
			printf("%s malloc tq item failure\n", __func__);

			/*free other already allocated dtd*/
			while (i) {
				i--;
				free((void *)(ep->ep_dtd[i]->item_unaligned_addr));
				ep->ep_dtd[i] = 0;
			}
			return -1;
		}
	}
	return 0;
}

static int mxc_destroy_ep_dtd(u8 index)
{
	struct mxc_ep_t *ep;
	int i;

	if (index >= mxc_udc.max_ep)
		DBG("%s ep %d is not valid\n", __func__, index);

	ep = mxc_udc.mxc_ep + index;
	for (i = 0; i < EP_TQ_ITEM_SIZE; i++) {
		if (ep->ep_dtd[i]) {
			free((void *)(ep->ep_dtd[i]->item_unaligned_addr));
			ep->ep_dtd[i] = 0;
		}
	}

	return 0;
}

static void mxc_ep_qh_setup(u8 ep_num, u8 dir, u8 ep_type,
				 u32 max_pkt_len, u32 zlt, u8 mult)
{
	struct ep_queue_head *p_qh = mxc_udc.ep_qh + (2 * ep_num + dir);
	u32 tmp = 0;

	tmp = max_pkt_len << 16;
	switch (ep_type) {
	case USB_ENDPOINT_XFER_CONTROL:
		tmp |= (1 << 15);
		break;
	case USB_ENDPOINT_XFER_ISOC:
		tmp |= (mult << 30);
		break;
	case USB_ENDPOINT_XFER_BULK:
	case USB_ENDPOINT_XFER_INT:
		break;
	default:
		DBG("error ep type is %d\n", ep_type);
		return;
	}
	if (zlt)
		tmp |= (1<<29);

	p_qh->config = tmp;

	/*flush qh's config field to physical memory*/
	flush_dcache_range((unsigned long)p_qh,
		CACHE_ALIGNED_END(p_qh, sizeof(struct ep_queue_head)));
}

static void mxc_ep_setup(u8 ep_num, u8 dir, u8 ep_type)
{
	u32 epctrl = 0;
	epctrl = readl(USB_ENDPTCTRL(ep_num));
	if (dir) {
		if (ep_num)
			epctrl |= EPCTRL_TX_DATA_TOGGLE_RST;
		epctrl |= EPCTRL_TX_ENABLE;
		epctrl |= ((u32)(ep_type) << EPCTRL_TX_EP_TYPE_SHIFT);
	} else {
		if (ep_num)
			epctrl |= EPCTRL_RX_DATA_TOGGLE_RST;
		epctrl |= EPCTRL_RX_ENABLE;
		epctrl |= ((u32)(ep_type) << EPCTRL_RX_EP_TYPE_SHIFT);
	}
	writel(epctrl, USB_ENDPTCTRL(ep_num));
}

static void mxc_ep_destroy(u8 ep_num, u8 dir)
{
	u32 epctrl = 0;
	epctrl = readl(USB_ENDPTCTRL(ep_num));
	if (dir)
		epctrl &= ~EPCTRL_TX_ENABLE;
	else
		epctrl &= ~EPCTRL_RX_ENABLE;

	writel(epctrl, USB_ENDPTCTRL(ep_num));
}


static void mxc_tqi_init_page(struct ep_queue_item *tqi)
{
	tqi->page0 = virt_to_phys((void *)(tqi->page_vir));
	tqi->page1 = tqi->page0 + 0x1000;
	tqi->page2 = tqi->page1 + 0x1000;
	tqi->page3 = tqi->page2 + 0x1000;
	tqi->page4 = tqi->page3 + 0x1000;
}

static int mxc_malloc_ep0_ptr(struct mxc_ep_t *ep)
{
	int i;
	struct ep_queue_item *tqi;
	int max_pkt_size = USB_MAX_CTRL_PAYLOAD;

	ep->max_pkt_size = max_pkt_size;
	for (i = 0; i < EP_TQ_ITEM_SIZE; i++) {
		tqi = ep->ep_dtd[i];
		tqi->page_vir = (u32)malloc_aligned_buffer(&tqi->page_unaligned,
						    max_pkt_size,
						    USB_MEM_ALIGN_BYTE);
		if ((void *)tqi->page_vir == NULL) {
			printf("malloc ep's dtd bufer failure, i=%d\n", i);
			return -1;
		}
		mxc_tqi_init_page(tqi);

		/*flush dtd's config field to physical memory*/
		flush_dcache_range((unsigned long)tqi,
			CACHE_ALIGNED_END(tqi, sizeof(struct ep_queue_item)));
	}
	return 0;
}

static int mxc_free_ep0_ptr(struct mxc_ep_t *ep)
{
	int i;
	struct ep_queue_item *tqi;
	for (i = 0; i < EP_TQ_ITEM_SIZE; i++) {
		tqi = ep->ep_dtd[i];
		if (tqi->page_vir) {
			free((void *)(tqi->page_unaligned));
			tqi->page_vir = 0;
			tqi->page_unaligned = 0;
			tqi->page0 = 0;
			tqi->page1 = 0;
			tqi->page2 = 0;
			tqi->page3 = 0;
			tqi->page4 = 0;

			/*flush dtd's config field to physical memory*/
			flush_dcache_range((unsigned long)tqi,
				CACHE_ALIGNED_END(tqi, sizeof(struct ep_queue_item)));
		}
	}

	return 0;
}

static void ep0_setup(void)
{
	mxc_ep_qh_setup(0, USB_RECV, USB_ENDPOINT_XFER_CONTROL,
			    USB_MAX_CTRL_PAYLOAD, 0, 0);
	mxc_ep_qh_setup(0, USB_SEND, USB_ENDPOINT_XFER_CONTROL,
			    USB_MAX_CTRL_PAYLOAD, 0, 0);
	mxc_ep_setup(0, USB_RECV, USB_ENDPOINT_XFER_CONTROL);
	mxc_ep_setup(0, USB_SEND, USB_ENDPOINT_XFER_CONTROL);
	mxc_init_ep_dtd(0 * 2 + USB_RECV);
	mxc_init_ep_dtd(0 * 2 + USB_SEND);
	mxc_malloc_ep0_ptr(mxc_udc.mxc_ep + (USB_RECV));
	mxc_malloc_ep0_ptr(mxc_udc.mxc_ep + (USB_SEND));
}

static void ep0_destroy(void)
{
	mxc_ep_destroy(0, USB_RECV);
	mxc_ep_destroy(0, USB_SEND);
	mxc_free_ep0_ptr(mxc_udc.mxc_ep + (USB_RECV));
	mxc_free_ep0_ptr(mxc_udc.mxc_ep + (USB_SEND));
	mxc_destroy_ep_dtd(0 * 2 + USB_RECV);
	mxc_destroy_ep_dtd(0 * 2 + USB_SEND);
}

static int mxc_tqi_is_busy(struct ep_queue_item *tqi)
{
	/* bit 7 is set by software when send, clear by controller
	   when finish */
	/*Invalidate cache to gain dtd content from physical memory*/
	invalidate_dcache_range((unsigned long)tqi,
		CACHE_ALIGNED_END(tqi, sizeof(struct ep_queue_item)));
	return tqi->info & (1 << 7);
}

static int mxc_ep_xfer_is_working(struct mxc_ep_t *ep, u32 in)
{
	/* in: means device -> host */
	u32 bitmask = 1 << (ep->epnum + in * 16);
	u32 temp, prime, tstat;

	prime = (bitmask & readl(USB_ENDPTPRIME));
	if (prime)
		return 1;
	do {
		temp = readl(USB_USBCMD);
		writel(temp|USB_CMD_ATDTW, USB_USBCMD);
		tstat = readl(USB_ENDPTSTAT) & bitmask;
	} while (!(readl(USB_USBCMD) & USB_CMD_ATDTW));
	writel(temp & (~USB_CMD_ATDTW), USB_USBCMD);

	if (tstat)
		return 1;
	return 0;
}

static void mxc_update_qh(struct mxc_ep_t *ep,
	struct ep_queue_item *tqi, u32 in)
{
	/* in: means device -> host */
	struct ep_queue_head *qh = ep->ep_qh;
	u32 bitmask = 1 << (ep->epnum + in * 16);
	DBG("%s, line %d, epnum=%d, in=%d\n", __func__,
		__LINE__, ep->epnum, in);
	qh->next_queue_item = virt_to_phys(tqi);
	qh->info = 0;

	/*flush qh''s config field to physical memory*/
	flush_dcache_range((unsigned long)qh,
		CACHE_ALIGNED_END(qh, sizeof(struct ep_queue_head)));

	writel(bitmask, USB_ENDPTPRIME);
}

static void _dump_buf(u8 *buf, u32 len)
{
#ifdef DEBUG
	char *data = (char *)buf;
	int i;
	for (i = 0; i < len; i++) {
		printf("0x%02x ", data[i]);
		if (i%16 == 15)
			printf("\n");
	}
	printf("\n");
#endif
}

static void mxc_udc_queue_update(u8 epnum,
	u8 *data, u32 len, u32 tx)
{
	struct mxc_ep_t *ep;
	struct ep_queue_item *tqi, *head, *last;
	int send = 0;
	int in;

	head = last = NULL;
	in = ep_is_in(epnum, tx);
	ep = mxc_udc.mxc_ep + (epnum * 2 + in);
	DBG("epnum = %d,  in = %d\n", epnum, in);
	do {
		tqi = ep->ep_dtd[ep->index];
		DBG("%s, index = %d, tqi = %p\n", __func__, ep->index, tqi);
		while (mxc_tqi_is_busy(tqi))
			;
		mxc_tqi_init_page(tqi);
		DBG("%s, line = %d, len = %d\n", __func__, __LINE__, len);
		inc_index(ep->index);
		send = min(len, ep->max_pkt_size);
		if (data) {
			memcpy((void *)tqi->page_vir, (void *)data, send);
			_dump_buf((u8 *)(tqi->page_vir), send);
		}
		flush_dcache_range((unsigned long)(tqi->page_vir),
				CACHE_ALIGNED_END(tqi->page_vir, ep->max_pkt_size));
		if (!head)
			last = head = tqi;
		else {
			last->next_item_ptr = virt_to_phys(tqi);
			last->next_item_vir = tqi;
			last = tqi;
		}
		if (!tx)
			tqi->reserved[0] = send;
		/* we set IOC for every dtd */
		tqi->info = ((send << 16) | (1 << 15) | (1 << 7));
		data += send;
		len -= send;

		flush_dcache_range((unsigned long)tqi,
			CACHE_ALIGNED_END(tqi, sizeof(struct ep_queue_item)));
	} while (len);

	last->next_item_ptr = 0x1; /* end */
	flush_dcache_range((unsigned long)last,
			CACHE_ALIGNED_END(last, sizeof(struct ep_queue_item)));

	if (ep->tail) {
		ep->tail->next_item_ptr = virt_to_phys(head);
		ep->tail->next_item_vir = head;
		flush_dcache_range((unsigned long)(ep->tail),
			CACHE_ALIGNED_END(ep->tail, sizeof(struct ep_queue_item)));

		if (mxc_ep_xfer_is_working(ep, in)) {
			DBG("ep is working\n");
			goto out;
		}
	}
	mxc_update_qh(ep, head, in);
out:
	ep->tail = last;
}

static void mxc_udc_txqueue_update(u8 ep, u8 *data, u32 len)
{
	printf("[SEND DATA] EP= %d, Len = 0x%x\n", ep, len);
	_dump_buf(data, len);
	mxc_udc_queue_update(ep, data, len, 1);
}

void mxc_udc_rxqueue_update(u8 ep, u32 len)
{
	mxc_udc_queue_update(ep, NULL, len, 0);
}

static void mxc_ep0_stall(void)
{
	u32 temp;
	temp = readl(USB_ENDPTCTRL(0));
	temp |= EPCTRL_TX_EP_STALL | EPCTRL_RX_EP_STALL;
	writel(temp, USB_ENDPTCTRL(0));
}

static void mxc_usb_run(void)
{
	unsigned int temp = 0;

	/* Enable DR irq reg */
	temp = USB_INTR_INT_EN | USB_INTR_ERR_INT_EN
		| USB_INTR_PTC_DETECT_EN | USB_INTR_RESET_EN
		| USB_INTR_DEVICE_SUSPEND | USB_INTR_SYS_ERR_EN;

	writel(temp, USB_USBINTR);

	/* Set controller to Run */
	temp = readl(USB_USBCMD);
	temp |= USB_CMD_RUN_STOP;
	writel(temp, USB_USBCMD);
}

static void mxc_usb_stop(void)
{
	unsigned int temp = 0;

	writel(temp, USB_USBINTR);

	/* Set controller to Stop */
	temp = readl(USB_USBCMD);
	temp &= ~USB_CMD_RUN_STOP;
	writel(temp, USB_USBCMD);
}

static void usb_phy_init(void)
{
#ifndef CONFIG_MX7
	u32 temp;
	/* select 24M clk */
	temp = readl(USB_PHY1_CTRL);
	temp &= ~3;
	temp |= 1;
	writel(temp, USB_PHY1_CTRL);
	/* Config PHY interface */
	temp = readl(USB_PORTSC1);
	temp &= ~(PORTSCX_PHY_TYPE_SEL | PORTSCX_PORT_WIDTH);
	temp |= PORTSCX_PTW_16BIT;
	writel(temp, USB_PORTSC1);
	DBG("Config PHY  END\n");
#endif
}

static void usb_set_mode_device(void)
{
	u32 temp;

	/* Set controller to stop */
	temp = readl(USB_USBCMD);
	temp &= ~USB_CMD_RUN_STOP;
	writel(temp, USB_USBCMD);

	while (readl(USB_USBCMD) & USB_CMD_RUN_STOP)
		;
	/* Do core reset */
	temp = readl(USB_USBCMD);
	temp |= USB_CMD_CTRL_RESET;
	writel(temp, USB_USBCMD);
	while (readl(USB_USBCMD) & USB_CMD_CTRL_RESET)
		;
	DBG("DOORE RESET END\n");

#if (defined(CONFIG_MX6) || defined(CONFIG_MX7))
	reset_usb_phy1();
#endif
	DBG("init core to device mode\n");
	temp = readl(USB_USBMODE);
	temp &= ~USB_MODE_CTRL_MODE_MASK;	/* clear mode bits */
	temp |= USB_MODE_CTRL_MODE_DEVICE;
	/* Disable Setup Lockout */
	temp |= USB_MODE_SETUP_LOCK_OFF;
	writel(temp, USB_USBMODE);

	temp = readl(USB_OTGSC);
	temp |= (1<<3);
	writel(temp, USB_OTGSC);
	DBG("init core to device mode end\n");
}

static void usb_init_eps(void)
{
	u32 temp;
	DBG("Flush begin\n");
	temp = readl(USB_ENDPTNAKEN);
	temp |= 0x10001;	/* clear mode bits */
	writel(temp, USB_ENDPTNAKEN);
	writel(readl(USB_ENDPTCOMPLETE), USB_ENDPTCOMPLETE);
	writel(readl(USB_ENDPTSETUPSTAT), USB_ENDPTSETUPSTAT);
	writel(0xffffffff, USB_ENDPTFLUSH);
	DBG("FLUSH END\n");
}

static void usb_udc_init(void)
{
	DBG("\n************************\n");
	DBG("         usb init start\n");
	DBG("\n************************\n");

	usb_phy_init();
	usb_set_mode_device();
	mxc_init_usb_qh();
	usb_init_eps();
	mxc_init_ep_struct();
	ep0_setup();
	usb_inited = 1;
}

static void usb_udc_destroy(void)
{
	usb_set_mode_device();

	usb_inited = 0;

	ep0_destroy();
	mxc_destroy_ep_struct();
	mxc_destroy_usb_qh();
}

void usb_shutdown(void)
{
	u32 temp;
	/* disable pullup */
	temp = readl(USB_USBCMD);
	temp &= ~USB_CMD_RUN_STOP;
	writel(temp, USB_USBCMD);
	mdelay(2);
}

static void ch9getstatus(u8 request_type,
	u16 value, u16 index, u16 length)
{
	u16 tmp;

	if ((request_type & USB_RECIP_MASK) == USB_RECIP_DEVICE) {
		tmp = 1 << 0; /* self powerd */
		tmp |= 0 << 1; /* not remote wakeup able */
	} else if ((request_type & USB_RECIP_MASK) == USB_RECIP_INTERFACE) {
		tmp = 0;
	} else if ((request_type & USB_RECIP_MASK) == USB_RECIP_ENDPOINT) {
		tmp = 0;
	}
	mxc_udc.ep0_dir = USB_DIR_IN;
	mxc_udc_queue_update(0, (u8 *)&tmp, 2, 0xffffffff);
}

static void mxc_udc_read_setup_pkt(struct usb_device_request *s)
{
	u32 temp;
	temp = readl(USB_ENDPTSETUPSTAT);
	writel(temp, USB_ENDPTSETUPSTAT);
	DBG("setup stat %x\n", temp);
	do {
		temp = readl(USB_USBCMD);
		temp |= USB_CMD_SUTW;
		writel(temp, USB_USBCMD);

		invalidate_dcache_range((unsigned long)(mxc_udc.mxc_ep[0].ep_qh),
			CACHE_ALIGNED_END((mxc_udc.mxc_ep[0].ep_qh),
				sizeof(struct ep_queue_head)));

		memcpy((void *)s,
			(void *)mxc_udc.mxc_ep[0].ep_qh->setup_data, 8);
	} while (!(readl(USB_USBCMD) & USB_CMD_SUTW));

	DBG("handle_setup s.type=%x req=%x len=%x\n",
		s->bmRequestType, s->bRequest, s->wLength);
	temp = readl(USB_USBCMD);
	temp &= ~USB_CMD_SUTW;
	writel(temp, USB_USBCMD);

	DBG("[SETUP] type=%x req=%x val=%x index=%x len=%x\n",
		s->bmRequestType, s->bRequest,
		s->wValue, s->wIndex,
		s->wLength);
}

static void mxc_udc_recv_setup(void)
{
	struct usb_device_request *s = &ep0_urb->device_request;

	mxc_udc_read_setup_pkt(s);
	if (s->wLength)	{
		/* If has a data phase,
		  * then prime a dtd for status stage which has zero length DATA0.
		  * The direction of status stage should oppsite to direction of data phase.
		  */
		mxc_udc.ep0_dir = (s->bmRequestType & USB_DIR_IN) ?
					USB_DIR_OUT : USB_DIR_IN;
		mxc_udc_queue_update(0, NULL, 0, 0xffffffff);
	}
	if (ep0_recv_setup(ep0_urb)) {
		mxc_ep0_stall();
		return;
	}
	switch (s->bRequest) {
	case USB_REQ_GET_STATUS:
		if ((s->bmRequestType & (USB_DIR_IN | USB_TYPE_MASK)) !=
					 (USB_DIR_IN | USB_TYPE_STANDARD))
			break;
		ch9getstatus(s->bmRequestType, s->wValue,
				s->wIndex, s->wLength);

		DBG("[SETUP] REQ_GET_STATUS\n");
		return;
	case USB_REQ_SET_ADDRESS:
		if (s->bmRequestType != (USB_DIR_OUT |
			    USB_TYPE_STANDARD | USB_RECIP_DEVICE))
			break;
		mxc_udc.setaddr = 1;
		mxc_udc.ep0_dir = USB_DIR_IN;
		mxc_udc_queue_update(0, NULL, 0, 0xffffffff);
		usbd_device_event_irq(udc_device, DEVICE_ADDRESS_ASSIGNED, 0);
		DBG("[SETUP] REQ_SET_ADDRESS\n");
		return;
	case USB_REQ_SET_CONFIGURATION:
		usbd_device_event_irq(udc_device, DEVICE_CONFIGURED, 0);
		DBG("[SETUP] REQ_SET_CONFIGURATION\n");
	case USB_REQ_CLEAR_FEATURE:
	case USB_REQ_SET_FEATURE:
	{
		int rc = -1;
		if ((s->bmRequestType & (USB_RECIP_MASK | USB_TYPE_MASK)) ==
				 (USB_RECIP_ENDPOINT | USB_TYPE_STANDARD))
			rc = 0;
		else if ((s->bmRequestType &
			    (USB_RECIP_MASK | USB_TYPE_MASK)) ==
			     (USB_RECIP_DEVICE | USB_TYPE_STANDARD))
			rc = 0;
		else
			break;
		if (rc == 0) {
			mxc_udc.ep0_dir = USB_DIR_IN;
			mxc_udc_queue_update(0, NULL, 0, 0xffffffff);
		}
		return;
	}
	default:
		break;
	}
	if (s->wLength) {
		mxc_udc.ep0_dir = (s->bmRequestType & USB_DIR_IN) ?
					USB_DIR_IN : USB_DIR_OUT;
		mxc_udc_queue_update(0, ep0_urb->buffer,
				ep0_urb->actual_length, 0xffffffff);
		ep0_urb->actual_length = 0;
	} else {
		mxc_udc.ep0_dir = USB_DIR_IN;
		mxc_udc_queue_update(0, NULL, 0, 0xffffffff);
	}
}

static int mxc_udc_tqi_empty(struct ep_queue_item *tqi)
{
	int ret;

	invalidate_dcache_range((unsigned long)tqi,
		CACHE_ALIGNED_END(tqi, sizeof(struct ep_queue_item)));

	ret = tqi->info & (1 << 7);
	return ret;
}

static struct usb_endpoint_instance *mxc_get_epi(u8 epnum)
{
	int i;
	for (i = 0; i < udc_device->bus->max_endpoints; i++) {
		if ((udc_device->bus->endpoint_array[i].endpoint_address &
			 USB_ENDPOINT_NUMBER_MASK) == epnum)
			return &udc_device->bus->endpoint_array[i];
	}
	return NULL;
}

static u32 _mxc_ep_recv_data(u8 epnum, struct ep_queue_item *tqi)
{
	struct usb_endpoint_instance *epi = mxc_get_epi(epnum);
	struct urb *urb;
	u32 len = 0;

	if (!epi)
		return 0;

	invalidate_dcache_range((unsigned long)tqi,
		CACHE_ALIGNED_END(tqi, sizeof(struct ep_queue_item)));

	urb = epi->rcv_urb;
	if (urb) {
		u8 *data = urb->buffer + urb->actual_length;
		int remain_len = (tqi->info >> 16) & (0xefff);
		len = tqi->reserved[0] - remain_len;
		DBG("recv len %d-%d-%d\n", len, tqi->reserved[0], remain_len);


		invalidate_dcache_range((unsigned long)tqi->page_vir,
			CACHE_ALIGNED_END(tqi->page_vir, len));
		memcpy(data, (void *)tqi->page_vir, len);

		_dump_buf(data, len);
	}
	return len;
}

static void mxc_udc_ep_recv(u8 epnum)
{
	struct mxc_ep_t *ep = mxc_udc.mxc_ep + (epnum * 2 + USB_RECV);
	struct ep_queue_item *tqi;
	while (1) {
		u32 nbytes;
		tqi = ep->ep_dtd[ep->done];
		if (mxc_udc_tqi_empty(tqi))
			break;
		nbytes = _mxc_ep_recv_data(epnum, tqi);
		usbd_rcv_complete(ep->epi, nbytes, 0);
		inc_index(ep->done);
		if (ep->done == ep->index)
			break;
	}
}

static void mxc_udc_handle_xfer_complete(void)
{
	int i;
	u32 bitpos = readl(USB_ENDPTCOMPLETE);

	writel(bitpos, USB_ENDPTCOMPLETE);

	for (i = 0; i < mxc_udc.max_ep; i++) {
		int epnum = i >> 1;
		int dir = i % 2;
		u32 bitmask = 1 << (epnum + 16 * dir);
		if (!(bitmask & bitpos))
			continue;
		DBG("ep %d, dir %d, complete\n", epnum, dir);
		if (!epnum) {
			if (mxc_udc.setaddr) {
				writel(udc_device->address << 25,
					USB_DEVICEADDR);
				mxc_udc.setaddr = 0;
			}
			continue;
		}
		DBG("############### dir = %d ***************\n", dir);
		if (dir == USB_SEND)
			continue;
		mxc_udc_ep_recv(epnum);
	}
}

static void usb_dev_hand_usbint(void)
{
	if (readl(USB_ENDPTSETUPSTAT)) {
		DBG("recv one setup packet\n");
		mxc_udc_recv_setup();
	}
	if (readl(USB_ENDPTCOMPLETE)) {
		DBG("Dtd complete irq\n");
		mxc_udc_handle_xfer_complete();
	}
}

static void usb_dev_hand_reset(void)
{
	u32 temp;
	temp = readl(USB_DEVICEADDR);
	temp &= ~0xfe000000;
	writel(temp, USB_DEVICEADDR);
	writel(readl(USB_ENDPTSETUPSTAT), USB_ENDPTSETUPSTAT);
	writel(readl(USB_ENDPTCOMPLETE), USB_ENDPTCOMPLETE);
	while (readl(USB_ENDPTPRIME))
		;
	writel(0xffffffff, USB_ENDPTFLUSH);
	DBG("reset-PORTSC=%x\n", readl(USB_PORTSC1));
	usbd_device_event_irq(udc_device, DEVICE_RESET, 0);
}

void usb_dev_hand_pci(void)
{
	u32 speed;
	while (readl(USB_PORTSC1) & PORTSCX_PORT_RESET)
		;
	speed = readl(USB_PORTSC1) & PORTSCX_PORT_SPEED_MASK;
	switch (speed) {
	case PORTSCX_PORT_SPEED_HIGH:
		usb_highspeed = 2;
		break;
	case PORTSCX_PORT_SPEED_FULL:
		usb_highspeed = 1;
		break;
	case PORTSCX_PORT_SPEED_LOW:
		usb_highspeed = 0;
		break;
	default:
		break;
	}
	DBG("portspeed=%d, speed = %x, PORTSC = %x\n",
		usb_highspeed, speed, readl(USB_PORTSC1));
}

void usb_dev_hand_suspend(void)
{
}

void mxc_irq_poll(void)
{
	unsigned irq_src = readl(USB_USBSTS) & readl(USB_USBINTR);
	writel(irq_src, USB_USBSTS);

	if (irq_src == 0)
		return;

	if (irq_src & USB_STS_INT)
		usb_dev_hand_usbint();

	if (irq_src & USB_STS_RESET) {
		printf("USB_RESET\n");
		usb_dev_hand_reset();
	}
	if (irq_src & USB_STS_PORT_CHANGE) {
		printf("USB_PORT_CHANGE 0x%x\n", irq_src);
		usb_dev_hand_pci();
	}
	if (irq_src & USB_STS_SUSPEND)
		printf("USB_SUSPEND\n");
	if (irq_src & USB_STS_ERR)
		printf("USB_ERR\n");
}

void mxc_udc_wait_cable_insert(void)
{
	u32 temp;
	int cable_connect = 1;

	do {
		udelay(50);

		temp = readl(USB_OTGSC);
		if (temp & (OTGSC_B_SESSION_VALID)) {
			printf("USB Mini b cable Connected!\n");
			break;
		} else if (cable_connect == 1) {
			printf("wait usb cable into the connector!\n");
			cable_connect = 0;
		}
	} while (1);
}

void udc_disable_over_current(void)
{
	u32 temp;
	temp = readl(USB_OTG_CTRL);
	temp |= (UCTRL_OVER_CUR_POL);
	writel(temp, USB_OTG_CTRL);
}

/*
 * mxc_udc_init function
 */
int mxc_udc_init(void)
{
    udc_pins_setting();
	set_usb_phy1_clk();
	enable_usboh3_clk(1);
#if (defined(CONFIG_MX6) || defined(CONFIG_MX7))
	udc_disable_over_current();
#endif
	enable_usb_phy1_clk(1);
	usb_udc_init();

	return 0;
}

/*
 * mxc_udc_init function
 */
int mxc_udc_destroy(void)
{
	usb_udc_destroy();
	enable_usboh3_clk(0);
	enable_usb_phy1_clk(0);

	return 0;
}

void mxc_udc_poll(void)
{
	mxc_irq_poll();
}

/*
 * Functions for gadget APIs
 */
int udc_init(void)
{
	mxc_udc_init();
	return 0;
}

int udc_destroy(void)
{
	udc_disable();
	mxc_udc_destroy();
	return 0;
}

void udc_setup_ep(struct usb_device_instance *device, u32 index,
		    struct usb_endpoint_instance *epi)
{
	u8 dir, epnum, zlt, mult;
	u8 ep_type;
	u32 max_pkt_size;
	int ep_addr;
	struct mxc_ep_t *ep;

	if (epi) {
		zlt = 1;
		mult = 0;
		ep_addr = epi->endpoint_address;
		epnum = ep_addr & USB_ENDPOINT_NUMBER_MASK;
		DBG("setup ep %d\n", epnum);
		if ((ep_addr & USB_ENDPOINT_DIR_MASK) == USB_DIR_IN) {
			dir = USB_SEND;
			ep_type = epi->tx_attributes;
			max_pkt_size = epi->tx_packetSize;
		} else {
			dir = USB_RECV;
			ep_type = epi->rcv_attributes;
			max_pkt_size = epi->rcv_packetSize;
		}
		if (ep_type == USB_ENDPOINT_XFER_ISOC) {
			mult = (u32)(1 + ((max_pkt_size >> 11) & 0x03));
			max_pkt_size = max_pkt_size & 0x7ff;
			DBG("mult = %d\n", mult);
		}
		ep = mxc_udc.mxc_ep + (epnum * 2 + dir);
		ep->epi = epi;
		if (epnum) {
			struct ep_queue_item *tqi;
			int i;

			mxc_ep_qh_setup(epnum, dir, ep_type,
					    max_pkt_size, zlt, mult);
			mxc_ep_setup(epnum, dir, ep_type);
			mxc_init_ep_dtd(epnum * 2 + dir);

			/* malloc endpoint's dtd's data buffer*/
			ep->max_pkt_size = max_pkt_size;
			for (i = 0; i < EP_TQ_ITEM_SIZE; i++) {
				tqi = ep->ep_dtd[i];
				tqi->page_vir = (u32)malloc_aligned_buffer(
					    &tqi->page_unaligned, max_pkt_size,
					    USB_MEM_ALIGN_BYTE);
				if ((void *)tqi->page_vir == NULL) {
					printf("malloc dtd bufer failure\n");
					return;
				}
				mxc_tqi_init_page(tqi);

				flush_dcache_range((unsigned long)tqi,
					CACHE_ALIGNED_END(tqi, sizeof(struct ep_queue_item)));
			}
		}
	}
}

void udc_destroy_ep(struct usb_device_instance *device,
		    struct usb_endpoint_instance *epi)
{
	struct mxc_ep_t *ep;
	int ep_addr;
	u8 dir, epnum;

	if (epi) {
		ep_addr = epi->endpoint_address;
		epnum = ep_addr & USB_ENDPOINT_NUMBER_MASK;

		if ((ep_addr & USB_ENDPOINT_DIR_MASK) == USB_DIR_IN)
			dir = USB_SEND;
		else
			dir = USB_RECV;

		ep = mxc_udc.mxc_ep + (epnum * 2 + dir);
		ep->epi = 0;

		if (epnum) {
			struct ep_queue_item *tqi;
			int i;

			for (i = 0; i < EP_TQ_ITEM_SIZE; i++) {
				tqi = ep->ep_dtd[i];
				if (tqi->page_vir) {
					free((void *)(tqi->page_unaligned));
					tqi->page_unaligned = 0;
					tqi->page_vir = 0;
					tqi->page0 = 0;
					tqi->page1 = 0;
					tqi->page2 = 0;
					tqi->page3 = 0;
					tqi->page4 = 0;
				}
			}

			mxc_destroy_ep_dtd(epnum * 2 + dir);
			mxc_ep_destroy(epnum, dir);
		}
	}
}

int udc_endpoint_write(struct usb_endpoint_instance *epi)
{
	struct urb *urb = epi->tx_urb;
	int ep_num = epi->endpoint_address & USB_ENDPOINT_NUMBER_MASK;
	u8 *data = (u8 *)urb->buffer + epi->sent;
	int n = urb->actual_length - epi->sent;
	mxc_udc_txqueue_update(ep_num, data, n);
	epi->last = n;

	/* usbd_tx_complete will take care of updating 'sent' */
	usbd_tx_complete(epi);
	return 0;
}

void udc_enable(struct usb_device_instance *device)
{
	udc_device = device;
	ep0_urb = usbd_alloc_urb(udc_device, udc_device->bus->endpoint_array);
}

void udc_disable(void)
{
	usbd_dealloc_urb(ep0_urb);
	udc_device = NULL;
}

void udc_startup_events(struct usb_device_instance *device)
{
	usbd_device_event_irq(device, DEVICE_INIT, 0);
	usbd_device_event_irq(device, DEVICE_CREATE, 0);
	udc_enable(device);
}

void udc_irq(void)
{
	mxc_irq_poll();
}

void udc_connect(void)
{
	mxc_usb_run();
	mxc_udc_wait_cable_insert();
}

void udc_disconnect(void)
{
	/* imx6 will hang if access usb register without init oh3
	 * clock, so not access it if not init. */
	if (usb_inited)
		mxc_usb_stop();
}

void udc_set_nak(int epid)
{
}

void udc_unset_nak(int epid)
{
}
