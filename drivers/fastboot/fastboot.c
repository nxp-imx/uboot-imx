/*
 * Copyright (C) 2010-2014 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <config.h>
#include <malloc.h>
#include <fastboot.h>
#include <usb/imx_udc.h>
#include <asm/io.h>
#include <usbdevice.h>
#include <mmc.h>
#include <sata.h>
#ifdef CONFIG_ANDROID_RECOVERY
#include <recovery.h>
#endif

/*
 * Defines
 */
#define NUM_ENDPOINTS  2

#define CONFIG_USBD_OUT_PKTSIZE	    0x200
#define CONFIG_USBD_IN_PKTSIZE	    0x200
#define MAX_BUFFER_SIZE		    0x200

/*
 * imx family android layout
 * mbr -  0 ~ 0x3FF byte
 * bootloader - 0x400 ~ 0xFFFFF byte
 * kernel - 0x100000 ~ 5FFFFF byte
 * uramedisk - 0x600000 ~ 0x6FFFFF  supposing 1M temporarily
 * SYSTEM partition - /dev/mmcblk0p2  or /dev/sda2
 * RECOVERY parittion - dev/mmcblk0p4 or /dev/sda4
 */
#define ANDROID_MBR_OFFSET	    0
#define ANDROID_MBR_SIZE	    0x200
#define ANDROID_BOOTLOADER_OFFSET   0x400
#define ANDROID_BOOTLOADER_SIZE	    0xFFC00
#define ANDROID_KERNEL_OFFSET	    0x100000
#define ANDROID_KERNEL_SIZE	    0x500000
#define ANDROID_URAMDISK_OFFSET	    0x600000
#define ANDROID_URAMDISK_SIZE	    0x100000

#define STR_LANG_INDEX		    0x00
#define STR_MANUFACTURER_INDEX	    0x01
#define STR_PRODUCT_INDEX	    0x02
#define STR_SERIAL_INDEX	    0x03
#define STR_CONFIG_INDEX	    0x04
#define STR_DATA_INTERFACE_INDEX    0x05
#define STR_CTRL_INTERFACE_INDEX    0x06
#define STR_COUNT		    0x07

#define FASTBOOT_FBPARTS_ENV_MAX_LEN 1024
/* To support the Android-style naming of flash */
#define MAX_PTN		    16


/*pentry index internally*/
enum {
    PTN_MBR_INDEX = 0,
    PTN_BOOTLOADER_INDEX,
    PTN_KERNEL_INDEX,
    PTN_URAMDISK_INDEX,
    PTN_SYSTEM_INDEX,
    PTN_RECOVERY_INDEX
};

struct fastboot_device_info fastboot_devinfo;

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
};

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



static struct fastboot_ptentry ptable[MAX_PTN];
static unsigned int pcount;


/* Static Function Prototypes */
static void _fastboot_init_strings(void);
static void _fastboot_init_instances(void);
static void _fastboot_init_endpoints(void);
static void _fastboot_event_handler(struct usb_device_instance *device,
				usb_device_event_t event, int data);
static int _fastboot_cdc_setup(struct usb_device_request *request,
				struct urb *urb);
static int _fastboot_usb_configured(void);
#if defined(CONFIG_FASTBOOT_STORAGE_SATA) \
	|| defined(CONFIG_FASTBOOT_STORAGE_MMC)
static int _fastboot_parts_load_from_ptable(void);
#endif
#if defined(CONFIG_FASTBOOT_STORAGE_NAND)
static int _fastboot_parts_load_from_env(void);
#endif
static int _fastboot_setup_dev(void);
static void _fastboot_load_partitions(void);

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
   Get mmc control number from passed string, eg, "mmc1" mean device 1. Only
   support "mmc0" to "mmc9" currently. It will be treated as device 0 for
   other string.
*/
static int _fastboot_get_mmc_no(char *env_str)
{
	int digit = 0;
	unsigned char a;

	if (env_str && (strlen(env_str) >= 4) &&
	    !strncmp(env_str, "mmc", 3)) {
		a = env_str[3];
		if (a >= '0' && a <= '9')
			digit = a - '0';
	}

	return digit;
}

static int _fastboot_setup_dev(void)
{
	char *fastboot_env;
	fastboot_env = getenv("fastboot_dev");

	if (fastboot_env) {
		if (!strcmp(fastboot_env, "sata")) {
			fastboot_devinfo.type = DEV_SATA;
			fastboot_devinfo.dev_id = 0;
		} else if (!strcmp(fastboot_env, "nand")) {
			fastboot_devinfo.type = DEV_NAND;
			fastboot_devinfo.dev_id = 0;
		} else if (!strncmp(fastboot_env, "mmc", 3)) {
			fastboot_devinfo.type = DEV_MMC;
			fastboot_devinfo.dev_id = _fastboot_get_mmc_no(fastboot_env);
		}
	} else {
		return 1;
	}

	return 0;
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

	_fastboot_init_strings();
	/* Basic USB initialization */
	udc_init();

	_fastboot_init_instances();

	udc_startup_events(device_instance);
	udc_connect();		/* Enable pullup for host detection */

	return 0;
}

static void _fastboot_init_strings(void)
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
	string->bLength = sizeof(wstrSerial);
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

static void _fastboot_init_instances(void)
{
	int i;
	u16 temp;

	/* Assign endpoint descriptors */
	ep_descriptor_ptrs[0] =
		&fastboot_configuration_descriptors[0].data_endpoints[0];
	ep_descriptor_ptrs[1] =
		&fastboot_configuration_descriptors[0].data_endpoints[1];

	/* Configuration Descriptor */
	configuration_descriptor =
		(struct usb_configuration_descriptor *)
		&fastboot_configuration_descriptors;

	fastboot_configured_flag = 0;

	/* initialize device instance */
	memset(device_instance, 0, sizeof(struct usb_device_instance));
	device_instance->device_state = STATE_INIT;
	device_instance->device_descriptor = &device_descriptor;
	device_instance->event = _fastboot_event_handler;
	device_instance->cdc_recv_setup = _fastboot_cdc_setup;
	device_instance->bus = bus_instance;
	device_instance->configurations = 1;
	device_instance->configuration_instance_array = config_instance;

	/* initialize bus instance */
	memset(bus_instance, 0, sizeof(struct usb_bus_instance));
	bus_instance->device = device_instance;
	bus_instance->endpoint_array = endpoint_instance;
	bus_instance->max_endpoints = NUM_ENDPOINTS + 1;
	bus_instance->maxpacketsize = 0xFF;
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

		/*fix the abort caused by unalignment*/
		temp = *(u8 *)&ep_descriptor_ptrs[i - 1]->wMaxPacketSize;
		temp |=
			(*(((u8 *)&ep_descriptor_ptrs[i - 1]->wMaxPacketSize) + 1) << 8);

		endpoint_instance[i].rcv_packetSize =
			le16_to_cpu(temp);

		endpoint_instance[i].tx_attributes =
			ep_descriptor_ptrs[i - 1]->bmAttributes;

		endpoint_instance[i].tx_packetSize =
			le16_to_cpu(temp);

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

static void _fastboot_init_endpoints(void)
{
	int i;

	bus_instance->max_endpoints = NUM_ENDPOINTS + 1;
	for (i = 1; i <= NUM_ENDPOINTS; i++)
		udc_setup_ep(device_instance, i, &endpoint_instance[i]);
}

static void _fastboot_destroy_endpoints(void)
{
	int i;
	struct urb *tx_urb;

	for (i = 1; i <= NUM_ENDPOINTS; i++) {
		/*dealloc urb*/
		if (endpoint_instance[i].endpoint_address & USB_DIR_IN) {
			if (endpoint_instance[i].tx_urb)
				usbd_dealloc_urb(endpoint_instance[i].tx_urb);

			while (endpoint_instance[i].tx_queue) {
				tx_urb = first_urb_detached(&endpoint_instance[i].tx);
				if (tx_urb) {
					usbd_dealloc_urb(tx_urb);
					endpoint_instance[i].tx_queue--;
				} else {
					break;
				}
			}
			endpoint_instance[i].tx_queue = 0;

			do {
				tx_urb = first_urb_detached(&endpoint_instance[i].done);
				if (tx_urb)
					usbd_dealloc_urb(tx_urb);
			} while (tx_urb);

		} else {
			if (endpoint_instance[i].rcv_urb)
				usbd_dealloc_urb(endpoint_instance[i].rcv_urb);
		}

		udc_destroy_ep(device_instance, &endpoint_instance[i]);
	}
}


static int _fill_buffer(u8 *buf)
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

static struct urb *_next_urb(struct usb_device_instance *device,
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

static int _fastboot_usb_configured(void)
{
	return fastboot_configured_flag;
}

static void _fastboot_event_handler(struct usb_device_instance *device,
				  usb_device_event_t event, int data)
{
	switch (event) {
	case DEVICE_RESET:
	case DEVICE_BUS_INACTIVE:
		fastboot_configured_flag = 0;
		break;
	case DEVICE_CONFIGURED:
		fastboot_configured_flag = 1;
		_fastboot_init_endpoints();
		break;
	case DEVICE_ADDRESS_ASSIGNED:
	default:
		break;
	}
}

static int _fastboot_cdc_setup(struct usb_device_request *request,
	struct urb *urb)
{
	return 0;
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

	while (!_fastboot_usb_configured())
		udc_irq();

	/* update rxqueue to wait new data */
	mxc_udc_rxqueue_update(2, count);

	while (!len) {
		if (is_usb_disconnected()) {
			/*it will not unconfigure when disconnect
			from host, so here needs manual unconfigure
			anyway, it's just a workaround*/
			fastboot_configured_flag = 0;
			usb_disconnected = 1;
			return 0;
		}
		udc_irq();
		if (_fastboot_usb_configured())
			len = _fill_buffer(buf);
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
		/*the udc_connect will be blocked until connect to host
		  so, the usb_disconnect should be 0 after udc_connect,
		  and should be set manually. Anyway, it's just a workaround*/
		usb_disconnected = 0;
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

static int _fastboot_write_buffer(const char *buffer,
	unsigned int buffer_size)
{
	struct usb_endpoint_instance *endpoint =
		(struct usb_endpoint_instance *)&endpoint_instance[tx_endpoint];
	struct urb *current_urb = NULL;

	if (!_fastboot_usb_configured())
		return 0;

	current_urb = _next_urb(device_instance, endpoint);
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
		len = _fastboot_write_buffer(buffer + len, buffer_size);
		buffer_size -= len;

		udc_irq();
	}
	udc_irq();

	return 0;
}

void fastboot_shutdown(void)
{
	usb_shutdown();

	/* Reset interface*/
	if (fastboot_interface &&
		fastboot_interface->reset_handler) {
		fastboot_interface->reset_handler();
	}

	/* Reset some globals */
	_fastboot_destroy_endpoints();
	fastboot_interface = NULL;
	fastboot_configured_flag = 0;
	usb_disconnected = 0;

	/*free memory*/
	udc_destroy();
}

/*
 * CPU and board-specific fastboot initializations.  Aliased function
 * signals caller to move on
 */
static void __def_fastboot_setup(void)
{
	/*do nothing here*/
}
void board_fastboot_setup(void) \
	__attribute__((weak, alias("__def_fastboot_setup")));


void fastboot_setup(void)
{
	/*execute board relevant initilizations for preparing fastboot */
	board_fastboot_setup();

	/*get the fastboot dev*/
	_fastboot_setup_dev();

	/*check if we need to setup recovery*/
#ifdef CONFIG_ANDROID_RECOVERY
    check_recovery_mode();
#endif

	/*load partitions information for the fastboot dev*/
	_fastboot_load_partitions();
}

/* export to lib_arm/board.c */
void check_fastboot(void)
{
	if (fastboot_check_and_clean_flag())
		do_fastboot(NULL, 0, 0, 0);
}

#if defined(CONFIG_FASTBOOT_STORAGE_SATA) \
	|| defined(CONFIG_FASTBOOT_STORAGE_MMC)
/**
   @mmc_dos_partition_index: the partition index in mbr.
   @mmc_partition_index: the boot partition or user partition index,
   not related to the partition table.
 */
static int _fastboot_parts_add_ptable_entry(int ptable_index,
				      int mmc_dos_partition_index,
				      int mmc_partition_index,
				      const char *name,
				      block_dev_desc_t *dev_desc,
				      struct fastboot_ptentry *ptable)
{
	disk_partition_t info;
	strcpy(ptable[ptable_index].name, name);

	if (get_partition_info(dev_desc,
			       mmc_dos_partition_index, &info)) {
		printf("Bad partition index:%d for partition:%s\n",
		       mmc_dos_partition_index, name);
		return -1;
	} else {
		ptable[ptable_index].start = info.start;
		ptable[ptable_index].length = info.size;
		ptable[ptable_index].partition_id = mmc_partition_index;
	}
	return 0;
}

static int _fastboot_parts_load_from_ptable(void)
{
	int i;
#ifdef CONFIG_CMD_SATA
	int sata_device_no;
#endif

	/* mmc boot partition: -1 means no partition, 0 user part., 1 boot part.
	 * default is no partition, for emmc default user part, except emmc*/
	int boot_partition = FASTBOOT_MMC_NONE_PARTITION_ID;
    int user_partition = FASTBOOT_MMC_NONE_PARTITION_ID;

	struct mmc *mmc;
	block_dev_desc_t *dev_desc;
	struct fastboot_ptentry ptable[PTN_RECOVERY_INDEX + 1];

	/* sata case in env */
	if (fastboot_devinfo.type == DEV_SATA) {
#ifdef CONFIG_CMD_SATA
		puts("flash target is SATA\n");
		if (sata_initialize())
			return -1;
		sata_device_no = CONFIG_FASTBOOT_SATA_NO;
		if (sata_device_no >= CONFIG_SYS_SATA_MAX_DEVICE) {
			printf("Unknown SATA(%d) device for fastboot\n",
				sata_device_no);
			return -1;
		}
		dev_desc = sata_get_dev(sata_device_no);
#else /*! CONFIG_CMD_SATA*/
		puts("SATA isn't buildin\n");
		return -1;
#endif /*! CONFIG_CMD_SATA*/
	} else if (fastboot_devinfo.type == DEV_MMC) {
		int mmc_no = 0;
		mmc_no = fastboot_devinfo.dev_id;

		printf("flash target is MMC:%d\n", mmc_no);
		mmc = find_mmc_device(mmc_no);
		if (mmc && mmc_init(mmc))
			printf("MMC card init failed!\n");

		dev_desc = get_dev("mmc", mmc_no);
		if (NULL == dev_desc) {
			printf("** Block device MMC %d not supported\n",
				mmc_no);
			return -1;
		}

		/* multiple boot paritions for eMMC 4.3 later */
		if (mmc->part_config != MMCPART_NOAVAILABLE) {
			boot_partition = FASTBOOT_MMC_BOOT_PARTITION_ID;
			user_partition = FASTBOOT_MMC_USER_PARTITION_ID;
		}
	} else {
		printf("Can't setup partition table on this device %d\n",
			fastboot_devinfo.type);
		return -1;
	}

	memset((char *)ptable, 0,
		    sizeof(struct fastboot_ptentry) * (PTN_RECOVERY_INDEX + 1));
	/* MBR */
	strcpy(ptable[PTN_MBR_INDEX].name, "mbr");
	ptable[PTN_MBR_INDEX].start = ANDROID_MBR_OFFSET / dev_desc->blksz;
	ptable[PTN_MBR_INDEX].length = ANDROID_MBR_SIZE / dev_desc->blksz;
	ptable[PTN_MBR_INDEX].partition_id = user_partition;
	/* Bootloader */
	strcpy(ptable[PTN_BOOTLOADER_INDEX].name, "bootloader");
	ptable[PTN_BOOTLOADER_INDEX].start =
				ANDROID_BOOTLOADER_OFFSET / dev_desc->blksz;
	ptable[PTN_BOOTLOADER_INDEX].length =
				 ANDROID_BOOTLOADER_SIZE / dev_desc->blksz;
	ptable[PTN_BOOTLOADER_INDEX].partition_id = boot_partition;

	_fastboot_parts_add_ptable_entry(PTN_KERNEL_INDEX,
				   CONFIG_ANDROID_BOOT_PARTITION_MMC,
				   user_partition, "boot", dev_desc, ptable);
	_fastboot_parts_add_ptable_entry(PTN_RECOVERY_INDEX,
				   CONFIG_ANDROID_RECOVERY_PARTITION_MMC,
				   user_partition,
				   "recovery", dev_desc, ptable);
	_fastboot_parts_add_ptable_entry(PTN_SYSTEM_INDEX,
				   CONFIG_ANDROID_SYSTEM_PARTITION_MMC,
				   user_partition,
				   "system", dev_desc, ptable);

	for (i = 0; i <= PTN_RECOVERY_INDEX; i++)
		fastboot_flash_add_ptn(&ptable[i]);

	return 0;
}
#endif /*CONFIG_FASTBOOT_STORAGE_SATA || CONFIG_FASTBOOT_STORAGE_MMC*/

#if defined(CONFIG_FASTBOOT_STORAGE_NAND)
static unsigned long long _memparse(char *ptr, char **retptr)
{
	char *endptr;	/* local pointer to end of parsed string */

	unsigned long ret = simple_strtoul(ptr, &endptr, 0);

	switch (*endptr) {
	case 'M':
	case 'm':
		ret <<= 10;
	case 'K':
	case 'k':
		ret <<= 10;
		endptr++;
	default:
		break;
	}

	if (retptr)
		*retptr = endptr;

	return ret;
}

static int _fastboot_parts_add_env_entry(char *s, char **retptr)
{
	unsigned long size;
	unsigned long offset = 0;
	char *name;
	int name_len;
	int delim;
	unsigned int flags;
	struct fastboot_ptentry part;

	size = _memparse(s, &s);
	if (0 == size) {
		printf("Error:FASTBOOT size of parition is 0\n");
		return 1;
	}

	/* fetch partition name and flags */
	flags = 0; /* this is going to be a regular partition */
	delim = 0;
	/* check for offset */
	if (*s == '@') {
		s++;
		offset = _memparse(s, &s);
	} else {
		printf("Error:FASTBOOT offset of parition is not given\n");
		return 1;
	}

	/* now look for name */
	if (*s == '(')
		delim = ')';

	if (delim) {
		char *p;

		name = ++s;
		p = strchr((const char *)name, delim);
		if (!p) {
			printf("Error:FASTBOOT no closing %c found in partition name\n",
				delim);
			return 1;
		}
		name_len = p - name;
		s = p + 1;
	} else {
		printf("Error:FASTBOOT no partition name for \'%s\'\n", s);
		return 1;
	}

	/* check for options */
	while (1) {
		if (strncmp(s, "i", 1) == 0) {
			flags |= FASTBOOT_PTENTRY_FLAGS_WRITE_I;
			s += 1;
		} else if (strncmp(s, "ubifs", 5) == 0) {
			/* ubifs */
			flags |= FASTBOOT_PTENTRY_FLAGS_WRITE_TRIMFFS;
			s += 5;
		} else {
			break;
		}
		if (strncmp(s, "|", 1) == 0)
			s += 1;
	}

	/* enter this partition (offset will be calculated later if it is zero at this point) */
	part.length = size;
	part.start = offset;
	part.flags = flags;

	if (name) {
		if (name_len >= sizeof(part.name)) {
			printf("Error:FASTBOOT partition name is too long\n");
			return 1;
		}
		strncpy(&part.name[0], name, name_len);
		/* name is not null terminated */
		part.name[name_len] = '\0';
	} else {
		printf("Error:FASTBOOT no name\n");
		return 1;
	}

	fastboot_flash_add_ptn(&part);

	/*if the nand partitions envs are not initialized, try to init them*/
	if (check_parts_values(&part))
		save_parts_values(&part, part.start, part.length);

	/* return (updated) pointer command line string */
	*retptr = s;

	/* return partition table */
	return 0;
}

static int _fastboot_parts_load_from_env(void)
{
	char fbparts[FASTBOOT_FBPARTS_ENV_MAX_LEN], *env;

	env = getenv("fbparts");
	if (env) {
		unsigned int len;
		len = strlen(env);
		if (len && len < FASTBOOT_FBPARTS_ENV_MAX_LEN) {
			char *s, *e;

			memcpy(&fbparts[0], env, len + 1);
			printf("Fastboot: Adding partitions from environment\n");
			s = &fbparts[0];
			e = s + len;
			while (s < e) {
				if (_fastboot_parts_add_env_entry(s, &s)) {
					printf("Error:Fastboot: Abort adding partitions\n");
					pcount = 0;
					return 1;
				}
				/* Skip a bunch of delimiters */
				while (s < e) {
					if ((' ' == *s) ||
					    ('\t' == *s) ||
					    ('\n' == *s) ||
					    ('\r' == *s) ||
					    (',' == *s)) {
						s++;
					} else {
						break;
					}
				}
			}
		}
	}

	return 0;
}
#endif /*CONFIG_FASTBOOT_STORAGE_NAND*/

static void _fastboot_load_partitions(void)
{
	pcount = 0;
#if defined(CONFIG_FASTBOOT_STORAGE_NAND)
	_fastboot_parts_load_from_env();
#elif defined(CONFIG_FASTBOOT_STORAGE_SATA) \
	|| defined(CONFIG_FASTBOOT_STORAGE_MMC)
	_fastboot_parts_load_from_ptable();
#endif
}

/*
 * Android style flash utilties */
void fastboot_flash_add_ptn(struct fastboot_ptentry *ptn)
{
	if (pcount < MAX_PTN) {
		memcpy(ptable + pcount, ptn, sizeof(struct fastboot_ptentry));
		pcount++;
	}
}

void fastboot_flash_dump_ptn(void)
{
	unsigned int n;
	for (n = 0; n < pcount; n++) {
		struct fastboot_ptentry *ptn = ptable + n;
		printf("ptn %d name='%s' start=%d len=%d\n",
			n, ptn->name, ptn->start, ptn->length);
	}
}


struct fastboot_ptentry *fastboot_flash_find_ptn(const char *name)
{
	unsigned int n;

	for (n = 0; n < pcount; n++) {
		/* Make sure a substring is not accepted */
		if (strlen(name) == strlen(ptable[n].name)) {
			if (0 == strcmp(ptable[n].name, name))
				return ptable + n;
		}
	}

	printf("can't find partition: %s, dump the partition table\n", name);
	fastboot_flash_dump_ptn();
	return 0;
}

struct fastboot_ptentry *fastboot_flash_get_ptn(unsigned int n)
{
	if (n < pcount)
		return ptable + n;
	else
		return 0;
}

unsigned int fastboot_flash_get_ptn_count(void)
{
	return pcount;
}
