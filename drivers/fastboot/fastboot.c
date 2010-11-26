/*
 * Copyright (C) 2010 Freescale Semiconductor, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <common.h>
#include <config.h>
#include <malloc.h>
#include <fastboot.h>
#include <usb/imx_udc.h>
#include <asm/io.h>
#include <usbdevice.h>
#include <mmc.h>

/*
 * Defines
 */
#define NUM_ENDPOINTS  2

#define CONFIG_USBD_OUT_PKTSIZE	    0x200
#define CONFIG_USBD_IN_PKTSIZE	    0x200
#define MAX_BUFFER_SIZE		    0x200

#define STR_LANG_INDEX		    0x00
#define STR_MANUFACTURER_INDEX	    0x01
#define STR_PRODUCT_INDEX	    0x02
#define STR_SERIAL_INDEX	    0x03
#define STR_CONFIG_INDEX	    0x04
#define STR_DATA_INTERFACE_INDEX    0x05
#define STR_CTRL_INTERFACE_INDEX    0x06
#define STR_COUNT		    0x07

/*pentry index internally*/
enum {
    PTN_MBR_INDEX = 0,
    PTN_BOOTLOADER_INDEX,
    PTN_KERNEL_INDEX,
    PTN_URAMDISK_INDEX,
    PTN_SYSTEM_INDEX,
    PTN_RECOVERY_INDEX
};

/* defined and used by gadget/ep0.c */
extern struct usb_string_descriptor **usb_strings;

static struct usb_device_instance device_instance[1];
static struct usb_bus_instance bus_instance[1];
static struct usb_configuration_instance config_instance[1];
static struct usb_interface_instance interface_instance[1];
static struct usb_alternate_instance alternate_instance[1];
/* one extra for control endpoint */
static struct usb_endpoint_instance endpoint_instance[NUM_ENDPOINTS+1];

static struct cmd_fastboot_interface *fastboot_interface;
static int fastboot_configured_flag;
static int usb_disconnected;

/* Indicies, References */
static u8 rx_endpoint;
static u8 tx_endpoint;
static struct usb_string_descriptor *fastboot_string_table[STR_COUNT];

/* USB Descriptor Strings */
static u8 wstrLang[4] = {4, USB_DT_STRING, 0x9, 0x4};
static u8 wstrManufacturer[2 * (sizeof(CONFIG_FASTBOOT_MANUFACTURER_STR))];
static u8 wstrProduct[2 * (sizeof(CONFIG_FASTBOOT_PRODUCT_NAME_STR))];
static u8 wstrSerial[2*(sizeof(CONFIG_FASTBOOT_SERIAL_NUM))];
static u8 wstrConfiguration[2 * (sizeof(CONFIG_FASTBOOT_CONFIGURATION_STR))];
static u8 wstrDataInterface[2 * (sizeof(CONFIG_FASTBOOT_INTERFACE_STR))];

/* Standard USB Data Structures */
static struct usb_interface_descriptor interface_descriptors[1];
static struct usb_endpoint_descriptor *ep_descriptor_ptrs[NUM_ENDPOINTS];
static struct usb_configuration_descriptor *configuration_descriptor;
static struct usb_device_descriptor device_descriptor = {
	.bLength = sizeof(struct usb_device_descriptor),
	.bDescriptorType =	USB_DT_DEVICE,
	.bcdUSB =		cpu_to_le16(USB_BCD_VERSION),
	.bDeviceClass =		0xff,
	.bDeviceSubClass =	0xff,
	.bDeviceProtocol =	0xff,
	.bMaxPacketSize0 =	0x40,
	.idVendor =		cpu_to_le16(CONFIG_FASTBOOT_VENDOR_ID),
	.idProduct =		cpu_to_le16(CONFIG_FASTBOOT_PRODUCT_ID),
	.bcdDevice =		cpu_to_le16(CONFIG_FASTBOOT_BCD_DEVICE),
	.iManufacturer =	STR_MANUFACTURER_INDEX,
	.iProduct =		STR_PRODUCT_INDEX,
	.iSerialNumber =	STR_SERIAL_INDEX,
	.bNumConfigurations =	1
};

/*
 * Static Generic Serial specific data
 */

struct fastboot_config_desc {
	struct usb_configuration_descriptor configuration_desc;
	struct usb_interface_descriptor	interface_desc[1];
	struct usb_endpoint_descriptor data_endpoints[NUM_ENDPOINTS];

} __attribute__((packed));

static struct fastboot_config_desc
fastboot_configuration_descriptors[1] = {
	{
		.configuration_desc = {
			.bLength = sizeof(struct usb_configuration_descriptor),
			.bDescriptorType = USB_DT_CONFIG,
			.wTotalLength =
			    cpu_to_le16(sizeof(struct fastboot_config_desc)),
			.bNumInterfaces = 1,
			.bConfigurationValue = 1,
			.iConfiguration = STR_CONFIG_INDEX,
			.bmAttributes =
				BMATTRIBUTE_SELF_POWERED|BMATTRIBUTE_RESERVED,
			.bMaxPower = 0x32
		},
		.interface_desc = {
			{
				.bLength  =
					sizeof(struct usb_interface_descriptor),
				.bDescriptorType = USB_DT_INTERFACE,
				.bInterfaceNumber = 0,
				.bAlternateSetting = 0,
				.bNumEndpoints = NUM_ENDPOINTS,
				.bInterfaceClass =
					FASTBOOT_INTERFACE_CLASS,
				.bInterfaceSubClass =
					FASTBOOT_INTERFACE_SUB_CLASS,
				.bInterfaceProtocol =
					FASTBOOT_INTERFACE_PROTOCOL,
				.iInterface = STR_DATA_INTERFACE_INDEX
			},
		},
		.data_endpoints  = {
			{
				.bLength =
					sizeof(struct usb_endpoint_descriptor),
				.bDescriptorType =  USB_DT_ENDPOINT,
				.bEndpointAddress = UDC_OUT_ENDPOINT |
							 USB_DIR_OUT,
				.bmAttributes =	USB_ENDPOINT_XFER_BULK,
				.wMaxPacketSize =
					cpu_to_le16(CONFIG_USBD_OUT_PKTSIZE),
				.bInterval = 0x00,
			},
			{
				.bLength =
					sizeof(struct usb_endpoint_descriptor),
				.bDescriptorType =  USB_DT_ENDPOINT,
				.bEndpointAddress = UDC_IN_ENDPOINT |
							USB_DIR_IN,
				.bmAttributes =	USB_ENDPOINT_XFER_BULK,
				.wMaxPacketSize =
					cpu_to_le16(CONFIG_USBD_IN_PKTSIZE),
				.bInterval = 0x00,
			},
		},
	},
};

/* Static Function Prototypes */
static void fastboot_init_strings(void);
static void fastboot_init_instances(void);
static void fastboot_init_endpoints(void);
static void fastboot_event_handler(struct usb_device_instance *device,
				usb_device_event_t event, int data);
static int fastboot_cdc_setup(struct usb_device_request *request,
				struct urb *urb);
static int fastboot_usb_configured(void);
#ifdef CONFIG_FASTBOOT_STORAGE_EMMC
static void fastboot_init_mmc_ptable(void);
#endif

/* utility function for converting char* to wide string used by USB */
static void str2wide(char *str, u16 * wide)
{
	int i;
	for (i = 0; i < strlen(str) && str[i]; i++) {
		#if defined(__LITTLE_ENDIAN)
			wide[i] = (u16) str[i];
		#elif defined(__BIG_ENDIAN)
			wide[i] = ((u16)(str[i])<<8);
		#else
			#error "__LITTLE_ENDIAN or __BIG_ENDIAN undefined"
		#endif
	}
}

/*
 * Initialize fastboot
 */
int fastboot_init(struct cmd_fastboot_interface *interface)
{
	printf("fastboot is in init......");

	fastboot_interface = interface;
	fastboot_interface->product_name = CONFIG_FASTBOOT_PRODUCT_NAME_STR;
	fastboot_interface->serial_no = CONFIG_FASTBOOT_SERIAL_NUM;
	fastboot_interface->nand_block_size = 4096;
	fastboot_interface->transfer_buffer =
				(unsigned char *)CONFIG_FASTBOOT_TRANSFER_BUF;
	fastboot_interface->transfer_buffer_size =
				CONFIG_FASTBOOT_TRANSFER_BUF_SIZE;
	fastboot_init_strings();
	/* Basic USB initialization */
	udc_init();

	fastboot_init_instances();
#ifdef CONFIG_FASTBOOT_STORAGE_EMMC
	fastboot_init_mmc_ptable();
#endif
	udc_startup_events(device_instance);
	udc_connect();		/* Enable pullup for host detection */

	return 0;
}

#ifdef CONFIG_FASTBOOT_STORAGE_EMMC
static void fastboot_init_mmc_ptable(void)
{
	int i;
	struct mmc *mmc;
	block_dev_desc_t *dev_desc;
	disk_partition_t info;
	fastboot_ptentry ptable[PTN_RECOVERY_INDEX + 1];

	mmc = find_mmc_device(CONFIG_FASTBOOT_MMC_NO);
	if (mmc && mmc_init(mmc))
		printf("MMC card init failed!\n");

	dev_desc = get_dev("mmc", CONFIG_FASTBOOT_MMC_NO);
	if (NULL == dev_desc) {
		printf("** Block device MMC %d not supported\n",
			CONFIG_FASTBOOT_MMC_NO);
		return;
	}

	memset((char *)ptable, 0,
		    sizeof(fastboot_ptentry) * (PTN_RECOVERY_INDEX + 1));

	/*
	 * imx family android layout
	 * mbr -  0 ~ 0x3FF byte
	 * bootloader - 0x400 ~ 0xFFFFF byte
	 * kernel - 0x100000 ~ 3FFFFF byte
	 * uramedisk - 0x400000 ~ 0x4FFFFF  supposing 1M temporarily
	 * SYSTEM partition - /dev/mmcblk0p2
	 * RECOVERY parittion - dev/mmcblk0p6
	 */
	/* MBR */
	strcpy(ptable[PTN_MBR_INDEX].name, "mbr");
	ptable[PTN_MBR_INDEX].start = 0;
	ptable[PTN_MBR_INDEX].length = 0x200;
	/* Bootloader */
	strcpy(ptable[PTN_BOOTLOADER_INDEX].name, "bootloader");
	ptable[PTN_BOOTLOADER_INDEX].start = 0x400;
	ptable[PTN_BOOTLOADER_INDEX].length = 0xFFC00;
	/* kernel */
	strcpy(ptable[PTN_KERNEL_INDEX].name, "kernel");
	ptable[PTN_KERNEL_INDEX].start = 0x100000;  /* 1M byte offset */
	ptable[PTN_KERNEL_INDEX].length = 0x300000; /* 3M byte */
	/* uramdisk */
	strcpy(ptable[PTN_URAMDISK_INDEX].name, "uramdisk");
	ptable[PTN_URAMDISK_INDEX].start = 0x400000; /* 4M byte offset */
	ptable[PTN_URAMDISK_INDEX].length = 0x100000;

	/* system partition */
	strcpy(ptable[PTN_SYSTEM_INDEX].name, "system");
	if (get_partition_info(dev_desc,
				CONFIG_ANDROID_SYSTEM_PARTITION_MMC, &info))
		printf("Bad partition index:%d\n",
			CONFIG_ANDROID_SYSTEM_PARTITION_MMC);
	else {
		ptable[PTN_SYSTEM_INDEX].start = info.start *
						    mmc->write_bl_len;
		ptable[PTN_SYSTEM_INDEX].length = info.size *
						    mmc->write_bl_len;
	}
	/* recovery partition */
	strcpy(ptable[PTN_RECOVERY_INDEX].name, "recovery");
	if (get_partition_info(dev_desc,
				CONFIG_ANDROID_RECOVERY_PARTITION_MMC, &info))
		printf("Bad partition index:%d\n",
			CONFIG_ANDROID_RECOVERY_PARTITION_MMC);
	else {
		ptable[PTN_RECOVERY_INDEX].start = info.start *
							mmc->write_bl_len;
		ptable[PTN_RECOVERY_INDEX].length = info.size *
							mmc->write_bl_len;
	}

	for (i = 0; i <= PTN_RECOVERY_INDEX; i++)
		fastboot_flash_add_ptn(&ptable[i]);
}
#endif

static void fastboot_init_strings(void)
{
	struct usb_string_descriptor *string;

	fastboot_string_table[STR_LANG_INDEX] =
		(struct usb_string_descriptor *)wstrLang;

	string = (struct usb_string_descriptor *)wstrManufacturer;
	string->bLength = sizeof(wstrManufacturer);
	string->bDescriptorType = USB_DT_STRING;
	str2wide(CONFIG_FASTBOOT_MANUFACTURER_STR, string->wData);
	fastboot_string_table[STR_MANUFACTURER_INDEX] = string;

	string = (struct usb_string_descriptor *)wstrProduct;
	string->bLength = sizeof(wstrProduct);
	string->bDescriptorType = USB_DT_STRING;
	str2wide(CONFIG_FASTBOOT_PRODUCT_NAME_STR, string->wData);
	fastboot_string_table[STR_PRODUCT_INDEX] = string;

	string = (struct usb_string_descriptor *)wstrSerial;
	string->bLength = strlen(CONFIG_FASTBOOT_SERIAL_NUM);
	string->bDescriptorType = USB_DT_STRING;
	str2wide(CONFIG_FASTBOOT_SERIAL_NUM, string->wData);
	fastboot_string_table[STR_SERIAL_INDEX] = string;

	string = (struct usb_string_descriptor *)wstrConfiguration;
	string->bLength = sizeof(wstrConfiguration);
	string->bDescriptorType = USB_DT_STRING;
	str2wide(CONFIG_FASTBOOT_CONFIGURATION_STR, string->wData);
	fastboot_string_table[STR_CONFIG_INDEX] = string;

	string = (struct usb_string_descriptor *) wstrDataInterface;
	string->bLength = sizeof(wstrDataInterface);
	string->bDescriptorType = USB_DT_STRING;
	str2wide(CONFIG_FASTBOOT_INTERFACE_STR, string->wData);
	fastboot_string_table[STR_DATA_INTERFACE_INDEX] = string;

	/* Now, initialize the string table for ep0 handling */
	usb_strings = fastboot_string_table;
}

static void fastboot_init_instances(void)
{
	int i;

	/* Assign endpoint descriptors */
	ep_descriptor_ptrs[0] =
		&fastboot_configuration_descriptors[0].data_endpoints[0];
	ep_descriptor_ptrs[1] =
		&fastboot_configuration_descriptors[0].data_endpoints[1];

	/* Configuration Descriptor */
	configuration_descriptor =
		(struct usb_configuration_descriptor *)
		&fastboot_configuration_descriptors;

	/* initialize device instance */
	memset(device_instance, 0, sizeof(struct usb_device_instance));
	device_instance->device_state = STATE_INIT;
	device_instance->device_descriptor = &device_descriptor;
	device_instance->event = fastboot_event_handler;
	device_instance->cdc_recv_setup = fastboot_cdc_setup;
	device_instance->bus = bus_instance;
	device_instance->configurations = 1;
	device_instance->configuration_instance_array = config_instance;

	/* initialize bus instance */
	memset(bus_instance, 0, sizeof(struct usb_bus_instance));
	bus_instance->device = device_instance;
	bus_instance->endpoint_array = endpoint_instance;
	bus_instance->max_endpoints = NUM_ENDPOINTS + 1;
	bus_instance->maxpacketsize = 512;
	bus_instance->serial_number_str = CONFIG_FASTBOOT_SERIAL_NUM;

	/* configuration instance */
	memset(config_instance, 0,
		sizeof(struct usb_configuration_instance));
	config_instance->interfaces = 1;
	config_instance->configuration_descriptor = configuration_descriptor;
	config_instance->interface_instance_array = interface_instance;

	/* interface instance */
	memset(interface_instance, 0,
		sizeof(struct usb_interface_instance));
	interface_instance->alternates = 1;
	interface_instance->alternates_instance_array = alternate_instance;

	/* alternates instance */
	memset(alternate_instance, 0,
		sizeof(struct usb_alternate_instance));
	alternate_instance->interface_descriptor = interface_descriptors;
	alternate_instance->endpoints = NUM_ENDPOINTS;
	alternate_instance->endpoints_descriptor_array = ep_descriptor_ptrs;

	/* endpoint instances */
	memset(&endpoint_instance[0], 0,
		sizeof(struct usb_endpoint_instance));
	endpoint_instance[0].endpoint_address = 0;
	endpoint_instance[0].rcv_packetSize = EP0_MAX_PACKET_SIZE;
	endpoint_instance[0].rcv_attributes = USB_ENDPOINT_XFER_CONTROL;
	endpoint_instance[0].tx_packetSize = EP0_MAX_PACKET_SIZE;
	endpoint_instance[0].tx_attributes = USB_ENDPOINT_XFER_CONTROL;
	udc_setup_ep(device_instance, 0, &endpoint_instance[0]);

	for (i = 1; i <= NUM_ENDPOINTS; i++) {
		memset(&endpoint_instance[i], 0,
			sizeof(struct usb_endpoint_instance));

		endpoint_instance[i].endpoint_address =
			ep_descriptor_ptrs[i - 1]->bEndpointAddress;

		endpoint_instance[i].rcv_attributes =
			ep_descriptor_ptrs[i - 1]->bmAttributes;

		endpoint_instance[i].rcv_packetSize =
			le16_to_cpu(ep_descriptor_ptrs[i - 1]->wMaxPacketSize);

		endpoint_instance[i].tx_attributes =
			ep_descriptor_ptrs[i - 1]->bmAttributes;

		endpoint_instance[i].tx_packetSize =
			le16_to_cpu(ep_descriptor_ptrs[i - 1]->wMaxPacketSize);

		endpoint_instance[i].tx_attributes =
			ep_descriptor_ptrs[i - 1]->bmAttributes;

		urb_link_init(&endpoint_instance[i].rcv);
		urb_link_init(&endpoint_instance[i].rdy);
		urb_link_init(&endpoint_instance[i].tx);
		urb_link_init(&endpoint_instance[i].done);

		if (endpoint_instance[i].endpoint_address & USB_DIR_IN) {
			tx_endpoint = i;
			endpoint_instance[i].tx_urb =
				usbd_alloc_urb(device_instance,
						&endpoint_instance[i]);
		} else {
			rx_endpoint = i;
			endpoint_instance[i].rcv_urb =
				usbd_alloc_urb(device_instance,
						&endpoint_instance[i]);
		}
	}
}

static void fastboot_init_endpoints(void)
{
	int i;

	bus_instance->max_endpoints = NUM_ENDPOINTS + 1;
	for (i = 1; i <= NUM_ENDPOINTS; i++)
		udc_setup_ep(device_instance, i, &endpoint_instance[i]);
}

static int fill_buffer(u8 *buf)
{
	struct usb_endpoint_instance *endpoint =
					&endpoint_instance[rx_endpoint];

	if (endpoint->rcv_urb && endpoint->rcv_urb->actual_length) {
		unsigned int nb = 0;
		char *src = (char *)endpoint->rcv_urb->buffer;
		unsigned int rx_avail = MAX_BUFFER_SIZE;

		if (rx_avail >= endpoint->rcv_urb->actual_length) {
			nb = endpoint->rcv_urb->actual_length;
			memcpy(buf, src, nb);
			endpoint->rcv_urb->actual_length = 0;
		}
		return nb;
	}
	return 0;
}

static struct urb *next_urb(struct usb_device_instance *device,
				struct usb_endpoint_instance *endpoint)
{
	struct urb *current_urb = NULL;
	int space;

	/* If there's a queue, then we should add to the last urb */
	if (!endpoint->tx_queue)
		current_urb = endpoint->tx_urb;
	else
		/* Last urb from tx chain */
		current_urb =
		    p2surround(struct urb, link, endpoint->tx.prev);

	/* Make sure this one has enough room */
	space = current_urb->buffer_length - current_urb->actual_length;
	if (space > 0)
		return current_urb;
	else {    /* No space here */
		/* First look at done list */
		current_urb = first_urb_detached(&endpoint->done);
		if (!current_urb)
			current_urb = usbd_alloc_urb(device, endpoint);

		urb_append(&endpoint->tx, current_urb);
		endpoint->tx_queue++;
	}
	return current_urb;
}

/*!
 * Function to receive data from host through channel
 *
 * @buf  buffer to fill in
 * @count  read data size
 *
 * @return 0
 */
int fastboot_usb_recv(u8 *buf, int count)
{
	int len = 0;

	while (!fastboot_usb_configured())
		udc_irq();

	/* update rxqueue to wait new data */
	mxc_udc_rxqueue_update(2, count);

	while (!len) {
		if (is_usb_disconnected()) {
			usb_disconnected = 1;
			return 0;
		}
		udc_irq();
		if (fastboot_usb_configured())
			len = fill_buffer(buf);
	}
	return len;
}

int fastboot_getvar(const char *rx_buffer, char *tx_buffer)
{
	/* Place board specific variables here */
	return 0;
}

int fastboot_poll()
{
	u8 buffer[MAX_BUFFER_SIZE];
	int length = 0;

	memset(buffer, 0, MAX_BUFFER_SIZE);

	length = fastboot_usb_recv(buffer, MAX_BUFFER_SIZE);

	/* If usb disconnected, blocked here to wait */
	if (usb_disconnected) {
		udc_disconnect();
		udc_connect();
	}

	if (!length)
		return FASTBOOT_INACTIVE;

	/* Pass this up to the interface's handler */
	if (fastboot_interface && fastboot_interface->rx_handler) {
		if (!fastboot_interface->rx_handler(buffer, length))
			return FASTBOOT_OK;
	}
	return FASTBOOT_OK;
}

int fastboot_tx(unsigned char *buffer, unsigned int buffer_size)
{
	/* Not realized yet */
	return 0;
}

static int write_buffer(const char *buffer, unsigned int buffer_size)
{
	struct usb_endpoint_instance *endpoint =
		(struct usb_endpoint_instance *)&endpoint_instance[tx_endpoint];
	struct urb *current_urb = NULL;

	if (!fastboot_usb_configured())
		return 0;

	current_urb = next_urb(device_instance, endpoint);
	if (buffer_size) {
		char *dest;
		int space_avail, popnum, count, total = 0;

		/* Break buffer into urb sized pieces,
		 * and link each to the endpoint
		 */
		count = buffer_size;
		while (count > 0) {
			if (!current_urb) {
				printf("current_urb is NULL, buffer_size %d\n",
				    buffer_size);
				return total;
			}

			dest = (char *)current_urb->buffer +
			current_urb->actual_length;

			space_avail = current_urb->buffer_length -
					current_urb->actual_length;
			popnum = MIN(space_avail, count);
			if (popnum == 0)
				break;

			memcpy(dest, buffer + total, popnum);
			printf("send: %s\n", (char *)buffer);

			current_urb->actual_length += popnum;
			total += popnum;

			if (udc_endpoint_write(endpoint))
				/* Write pre-empted by RX */
				return 0;
			count -= popnum;
		} /* end while */
		return total;
	}
	return 0;
}

int fastboot_tx_status(const char *buffer, unsigned int buffer_size)
{
	int len = 0;

	while (buffer_size > 0) {
		len = write_buffer(buffer + len, buffer_size);
		buffer_size -= len;

		udc_irq();
	}
	udc_irq();

	return 0;
}

void fastboot_shutdown(void)
{
	usb_shutdown();

	/* Reset some globals */
	fastboot_interface = NULL;
}

static int fastboot_usb_configured(void)
{
	return fastboot_configured_flag;
}

static void fastboot_event_handler(struct usb_device_instance *device,
				  usb_device_event_t event, int data)
{
	switch (event) {
	case DEVICE_RESET:
	case DEVICE_BUS_INACTIVE:
		fastboot_configured_flag = 0;
		break;
	case DEVICE_CONFIGURED:
		fastboot_configured_flag = 1;
		fastboot_init_endpoints();
		break;
	case DEVICE_ADDRESS_ASSIGNED:
	default:
		break;
	}
}

int fastboot_cdc_setup(struct usb_device_request *request, struct urb *urb)
{
	return 0;
}
