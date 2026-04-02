// SPDX-License-Identifier: BSD-2-Clause
/* Copyright 2025 NXP
 *
 */

#include <efi.h>
#include <efi_gbl_fastboot_transport_protocol.h>
#include <efi_loader.h>
#include <fastboot.h>
#include <g_dnl.h>
#include <malloc.h>
#include <time.h>
#include <watchdog.h>
#include <u-boot/schedule.h>
#include <linux/usb/composite.h>
#include <linux/usb/gadget.h>

#define EP_BUFFER_SIZE (4096)
#define FASTBOOT_POLLING_TIMEOUT (5000)

static const efi_guid_t efi_gbl_fastboot_transport_protocol_guid = \
					EFI_GBL_FASTBOOT_TRANSPORT_GUID;

/* Defination from drivers/usb/gadget/f_fastboot.c */
typedef struct usb_req usb_req;
struct usb_req {
	struct usb_request *in_req;
	usb_req *next;
};

struct f_fastboot {
	struct usb_function usb_function;
	/* IN/OUT EP's and corresponding requests */
	struct usb_ep *in_ep, *out_ep;
	struct usb_request *in_req, *out_req;
	usb_req *front, *rear;
};

/* External reference to fastboot function - defined in f_fastboot.c */
extern struct f_fastboot *fastboot_func;

/* Fastboot transport context structure */
struct fb_transport_ctx {
	struct udevice *udc;
	size_t rx_len;
	size_t rx_remaining;
	uint8_t rx_buf[EP_BUFFER_SIZE];
	bool rx_ready;
	bool transport_started;
};

static struct fb_transport_ctx g_fb_ctx;

static unsigned int rx_bytes_expected(struct usb_ep *ep)
{
	unsigned int maxpacket = usb_endpoint_maxp(ep->desc);
	unsigned int rem;

	if (g_fb_ctx.rx_remaining == 0 || \
		g_fb_ctx.rx_remaining > EP_BUFFER_SIZE)
		return EP_BUFFER_SIZE;

	/*
	 * Some controllers e.g. DWC3 don't like OUT transfers to be
	 * not ending in maxpacket boundary. So just make them happy by
	 * always requesting for integral multiple of maxpackets.
	 * This shouldn't bother controllers that don't care about it.
	 */
	rem = g_fb_ctx.rx_remaining % maxpacket;
	if (rem > 0)
		return g_fb_ctx.rx_remaining + (maxpacket - rem);

	return g_fb_ctx.rx_remaining;
}

/* Custom OUT endpoint completion handler for transport receive */
static void fb_transport_out_complete(struct usb_ep *ep, struct usb_request *req)
{
	if (req->status != 0 || req->length == 0)
		return;

	if (req->actual <= req->length) {
		/* TODO use pointer for better speed? */
		memcpy((void *)g_fb_ctx.rx_buf, req->buf, req->actual);
		g_fb_ctx.rx_len = req->actual;
		g_fb_ctx.rx_remaining -= req->actual;
		g_fb_ctx.rx_ready = true;
	} else {
		log_err("fastboot receive buffer overflow!\n");
	}

	/* Re-queue the request for continuous receiving */
	req->length = rx_bytes_expected(ep);
	req->actual = 0;
	usb_ep_queue(ep, req, 0);
}

static int fastboot_usb_polling(void) {
	if (g_dnl_detach()) {
		log_err("USB device detached!");
		return -1;
	}
	schedule();
	dm_usb_gadget_handle_interrupts(g_fb_ctx.udc);

	/* The Out endpoint may be reset when usb disconnect
	 * and reconnect. Check the complete callback to make
	 * sure it's what we desired.
	 */
	if (fastboot_func->out_req && \
		fastboot_func->out_req->complete != fb_transport_out_complete)
		fastboot_func->out_req->complete = fb_transport_out_complete;

	return 0;
}

static void reset_fb_ctx(bool start_transport) {
	g_fb_ctx.rx_ready = false;
	g_fb_ctx.rx_len = 0;
	g_fb_ctx.rx_remaining = 0;
	g_fb_ctx.transport_started = start_transport;
}

static efi_status_t EFIAPI start(struct efi_gbl_fastboot_transport_protocol* this) {
	EFI_ENTRY("%p", this);

	int ret;
	int controller_index = 0;
	ulong start_time = 0;

	if (g_fb_ctx.transport_started) {
		return EFI_EXIT(EFI_ALREADY_STARTED);
	}

#ifdef CONFIG_FASTBOOT_USB_DEV
	controller_index = CONFIG_FASTBOOT_USB_DEV;
#endif

	/* Get UDC device by index (default 0) */
	ret = udc_device_get_by_index(controller_index, &g_fb_ctx.udc);
	if (ret) {
		log_err("Failed to get UDC device: %d\n", ret);
		return EFI_EXIT(EFI_DEVICE_ERROR);
	}

	g_dnl_clear_detach();
	ret = g_dnl_register("usb_dnl_fastboot");
	if (ret || !fastboot_func) {
		log_err("Failed to register usb function: %d\n", ret);
		goto exit;
	}

	if (!g_dnl_board_usb_cable_connected()) {
		log_err("USB cable not detected, exit fastboot.\n");
		goto exit;
	}

	/* polling to wait for the fastboot select */
	start_time = get_timer(0);
	while (!fastboot_func->out_req) {
		if (fastboot_usb_polling()) {
			goto exit;
		}

		/* Check for timeout */
		if (get_timer(start_time) > FASTBOOT_POLLING_TIMEOUT) {
			goto exit;
		}
	}
	/* Hijack the OUT endpoint for fastboot transport usage */
	fastboot_func->out_req->complete = fb_transport_out_complete;

	/* Initialize fastboot transport context */
	reset_fb_ctx(true);

	return EFI_EXIT(EFI_SUCCESS);

exit:
	if (g_fb_ctx.udc) {
		udc_device_put(g_fb_ctx.udc);
		g_fb_ctx.udc = NULL;
	}
	g_dnl_unregister();
	g_dnl_clear_detach();

	return EFI_EXIT(EFI_DEVICE_ERROR);
}

static efi_status_t EFIAPI stop(struct efi_gbl_fastboot_transport_protocol* this) {
	EFI_ENTRY("%p", this);

	if (!g_fb_ctx.transport_started) {
		return EFI_EXIT(EFI_NOT_STARTED);
	}

	/* Release UDC device */
	if (g_fb_ctx.udc) {
		udc_device_put(g_fb_ctx.udc);
		g_fb_ctx.udc = NULL;
	}
	g_dnl_unregister();
	g_dnl_clear_detach();

	/* reset fastboot transport context */
	reset_fb_ctx(false);

	return EFI_EXIT(EFI_SUCCESS);
}

static efi_status_t EFIAPI receive(struct efi_gbl_fastboot_transport_protocol* this,
				   size_t* buffer_size, void* buffer,
				   EFI_GBL_FASTBOOT_RX_MODE mode) {
	EFI_ENTRY("%p %p %p %d", this, buffer_size, buffer, mode);

	void *current = NULL;

	if (!this || !buffer_size || !buffer) {
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	}
	if (mode != SINGLE_PACKET && mode != FIXED_LENGTH) {
		log_err("Invalid fastboot transport mode!");
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	}

	if (!g_fb_ctx.transport_started || !fastboot_func || !fastboot_func->out_ep) {
		return EFI_EXIT(EFI_NOT_STARTED);
	}

	/* Return success for 0 buffer size */
	if (*buffer_size == 0) {
		return EFI_EXIT(EFI_SUCCESS);
	}

	if (mode == FIXED_LENGTH)
		g_fb_ctx.rx_remaining = *buffer_size;
	else
		g_fb_ctx.rx_remaining = 0;

	current = buffer;
	/* loop to receive data */
	while (1) {
		if (fastboot_usb_polling()) {
			return EFI_EXIT(EFI_DEVICE_ERROR);
		}

		/* Data arrived. */
		if (g_fb_ctx.rx_ready) {
			/* First clear rx state */
			g_fb_ctx.rx_ready = false;

			if (mode == SINGLE_PACKET) {
				/* Single packet */
				if (*buffer_size < g_fb_ctx.rx_len) {
					log_err("Receive buffer is too small!");
					*buffer_size = g_fb_ctx.rx_len;
					return EFI_EXIT(EFI_BUFFER_TOO_SMALL);
				}

				memcpy(current, g_fb_ctx.rx_buf, g_fb_ctx.rx_len);
				*buffer_size = g_fb_ctx.rx_len;

				return EFI_EXIT(EFI_SUCCESS);
			} else {
				/* FIXED_LENGTH */
				memcpy(current, g_fb_ctx.rx_buf, g_fb_ctx.rx_len);
				current += g_fb_ctx.rx_len;

				if (!g_fb_ctx.rx_remaining) {
					/* All expected data received */
					return EFI_EXIT(EFI_SUCCESS);
				}
			}
		}
	}
}

static efi_status_t EFIAPI send(struct efi_gbl_fastboot_transport_protocol* this,
				size_t* buffer_size, const void* buffer) {
	EFI_ENTRY("%p %p %p", this, buffer_size, buffer);

	size_t remaining = 0, chunk_size = 0;
	void *data = NULL;
	ulong start_time = 0;

	if (!this || !buffer_size || !buffer) {
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	}

	if (!g_fb_ctx.transport_started || !fastboot_func || !fastboot_func->out_ep) {
		return EFI_EXIT(EFI_NOT_STARTED);
	}

	/* Return success if nothing is going to be sent */
	if (*buffer_size == 0) {
		return EFI_EXIT(EFI_SUCCESS);
	}

	/* Split large data into chunks if needed */
	data = (void *)buffer;
	remaining = *buffer_size;
	while (remaining > 0) {
		chunk_size = (remaining > EP_BUFFER_SIZE) ? EP_BUFFER_SIZE : remaining;

		/* Use fastboot's multi-send function to queue the data */
		if (fastboot_tx_write_more_s(data, chunk_size) != 0) {
			return EFI_EXIT(EFI_DEVICE_ERROR);
		}

		data += chunk_size;
		remaining -= chunk_size;
	}

	if (fastboot_func->front == NULL) {
		log_err("Quene fastboot tx request failed!\n");
		return EFI_EXIT(EFI_DEVICE_ERROR);
	}

	/* Wait for all queued transfers to complete - timeout in 5 seconds */
	start_time = get_timer(0);
	while (fastboot_func->front != NULL) {
		if (fastboot_usb_polling()) {
			return EFI_EXIT(EFI_DEVICE_ERROR);
		}

		/* Check for timeout */
		if (get_timer(start_time) > FASTBOOT_POLLING_TIMEOUT) {
			return EFI_EXIT(EFI_DEVICE_ERROR);
		}
	}

	return EFI_EXIT(EFI_SUCCESS);
}

static efi_status_t EFIAPI fastboot_flush(struct efi_gbl_fastboot_transport_protocol* this) {
	EFI_ENTRY("%p", this);

	/* TX buffers should be flush when send,
	 * return directly for this call.
	 */
	return EFI_EXIT(EFI_SUCCESS);
}

static struct efi_gbl_fastboot_transport_protocol efi_gbl_fastboot_transport_proto = {
  .revision = EFI_GBL_FASTBOOT_TRANSPORT_PROTOCOL_REVISION,
  .description = "Android Fastboot",
  .start = start,
  .stop = stop,
  .receive = receive,
  .send = send,
  .flush = fastboot_flush,
};

efi_status_t efi_gbl_fastboot_transport_register(void)
{
	efi_status_t ret =
		efi_add_protocol(efi_root, &efi_gbl_fastboot_transport_protocol_guid,
				 &efi_gbl_fastboot_transport_proto);
	if (ret != EFI_SUCCESS) {
		log_err("Failed to install EFI_GBL_FASTBOOT_TRANSPORT_PROTOCOL: 0x%lx\n",
			ret);
	}

	return ret;
}
