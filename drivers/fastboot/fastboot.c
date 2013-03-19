/*
 * Copyright (C) 2010-2013 Freescale Semiconductor, Inc.
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
#include <sata.h>

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
#ifdef CONFIG_FASTBOOT_STORAGE_EMMC_SATA
static int fastboot_init_mmc_sata_ptable(void);
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
   Get mmc control number from passed string, eg, "mmc1" mean device 1. Only
   support "mmc0" to "mmc9" currently. It will be treated as device 0 for
   other string.
*/
static int get_mmc_no(char *env_str)
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
#ifdef CONFIG_FASTBOOT_STORAGE_EMMC_SATA
	fastboot_init_mmc_sata_ptable();
#endif
	udc_startup_events(device_instance);
	udc_connect();		/* Enable pullup for host detection */

	return 0;
}

#ifdef CONFIG_FASTBOOT_STORAGE_EMMC_SATA
/**
   @mmc_dos_partition_index: the partition index in mbr.
   @mmc_partition_index: the boot partition or user partition index,
   not related to the partition table.
 */
static int setup_ptable_mmc_partition(int ptable_index,
				      int mmc_dos_partition_index,
				      int mmc_partition_index,
				      const char *name,
				      block_dev_desc_t *dev_desc,
				      fastboot_ptentry *ptable)
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

static int fastboot_init_mmc_sata_ptable(void)
{
	int i;
#ifdef CONFIG_CMD_SATA
	int sata_device_no;
#endif
	int boot_partition = -1, user_partition = -1;
	/* mmc boot partition: -1 means no partition, 0 user part., 1 boot part.
	 * default is no partition, for emmc default user part, except emmc*/
	struct mmc *mmc;
	block_dev_desc_t *dev_desc;
	char *fastboot_env;
	fastboot_ptentry ptable[PTN_RECOVERY_INDEX + 1];

	fastboot_env = getenv("fastboot_dev");
	/* sata case in env */
	if (fastboot_env && !strcmp(fastboot_env, "sata")) {
		fastboot_devinfo.type = DEV_SATA;
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
#else
		puts("SATA isn't buildin\n");
		return -1;
#endif
	} else {
		int mmc_no = 0;

		mmc_no = get_mmc_no(fastboot_env);

		fastboot_devinfo.type = DEV_MMC;
		fastboot_devinfo.dev_id = mmc_no;

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
			boot_partition = 1;
			user_partition = 0;
		}
	}

	memset((char *)ptable, 0,
		    sizeof(fastboot_ptentry) * (PTN_RECOVERY_INDEX + 1));
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

	setup_ptable_mmc_partition(PTN_KERNEL_INDEX,
				   CONFIG_ANDROID_BOOT_PARTITION_MMC,
				   user_partition, "boot", dev_desc, ptable);
	setup_ptable_mmc_partition(PTN_RECOVERY_INDEX,
				   CONFIG_ANDROID_RECOVERY_PARTITION_MMC,
				   user_partition,
				   "recovery", dev_desc, ptable);
	setup_ptable_mmc_partition(PTN_SYSTEM_INDEX,
				   CONFIG_ANDROID_SYSTEM_PARTITION_MMC,
				   user_partition,
				   "system", dev_desc, ptable);

	for (i = 0; i <= PTN_RECOVERY_INDEX; i++)
		fastboot_flash_add_ptn(&ptable[i]);

	return 0;
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
			/*it will not unconfigure when disconnect
			from host, so here needs manual unconfigure
			anyway, it's just a workaround*/
			fastboot_configured_flag = 0;
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

static int fastboot_cdc_setup(struct usb_device_request *request, struct urb *urb)
{
	return 0;
}

/* export to lib_arm/board.c */
void check_fastboot_mode(void)
{
	if (fastboot_check_and_clean_flag())
		do_fastboot(NULL, 0, 0, 0);
}

u8 fastboot_debug_level;
void fastboot_dump_memory(u32 *ptr, u32 len)
{
    u32 i;
    for (i = 0; i < len; i++) {
	DBG_DEBUG("0x%p: %08x %08x %08x %08x\n", ptr,
			*ptr, *(ptr+1), *(ptr+2), *(ptr+3));
	ptr += 4;
    }
}

#define FASTBOOT_STS_CMD 0
#define FASTBOOT_STS_CMD_WAIT 1
#define FASTBOOT_STS_DATA 2
#define FASTBOOT_STS_DATA_WAIT 3

static u8 fastboot_status;
static u8 g_fastboot_recvbuf[MAX_PAKET_LEN];
static u8 g_fastboot_sendbuf[MAX_PAKET_LEN];

static u32 g_fastboot_datalen;
static u8 g_fastboot_outep_index, g_fastboot_inep_index;
static u8 g_usb_connected;

void fastboot_get_ep_num(u8 *in, u8 *out)
{
    if (out)
	*out = rx_endpoint + EP0_OUT_INDEX + 1;
    if (in)
	*in = tx_endpoint + EP0_IN_INDEX + 1;
}

static void fastboot_data_handler(u32 len, u8 *recvbuf)
{
    if (len != g_fastboot_datalen)
	DBG_ERR("Fastboot data recv error, want:%d, recv:%d\n",
					g_fastboot_datalen, len);
    sprintf((char *)g_fastboot_sendbuf, "OKAY");
    udc_send_data(g_fastboot_inep_index, g_fastboot_sendbuf, 4, NULL);
    fastboot_status = FASTBOOT_STS_CMD;
}

static void fastboot_cmd_handler(u32 len, u8 *recvbuf)
{
    u32 *databuf = (u32 *)CONFIG_FASTBOOT_TRANSFER_BUF;

    if (len > sizeof(g_fastboot_recvbuf)) {
	DBG_ERR("%s, recv len=%d error\n", __func__, len);
	return;
    }
    recvbuf[len] = 0;
    DBG_ALWS("\nFastboot Cmd, len=%u, %s\n", len, recvbuf);

    if (memcmp(recvbuf, "download:", 9) == 0) {
	g_fastboot_datalen = simple_strtoul((const char *)recvbuf + 9,
								NULL, 16);
	if (g_fastboot_datalen > CONFIG_FASTBOOT_TRANSFER_BUF_SIZE) {
		DBG_ERR("Download too much data\n");
		sprintf((char *)g_fastboot_sendbuf, "FAIL");
		udc_send_data(g_fastboot_inep_index, g_fastboot_sendbuf,
								4, NULL);
		fastboot_status = FASTBOOT_STS_CMD;
	} else {
		sprintf((char *)g_fastboot_sendbuf, "DATA%08x",
							g_fastboot_datalen);
		udc_send_data(g_fastboot_inep_index, g_fastboot_sendbuf,
								12, NULL);
		DBG_ALWS("Fastboot is receiveing data...\n");
		udc_recv_data(g_fastboot_outep_index, (u8 *)databuf,
				g_fastboot_datalen, fastboot_data_handler);
		fastboot_status = FASTBOOT_STS_DATA_WAIT;
	}
    } else if (memcmp(recvbuf, "flash:", 6) == 0) {
		if (g_fastboot_datalen ==
			fastboot_write_storage(recvbuf+6, g_fastboot_datalen)) {
			DBG_ALWS("Fastboot write OK, send OKAY...\n");
			sprintf((char *)g_fastboot_sendbuf, "OKAY");
			udc_send_data(g_fastboot_inep_index, g_fastboot_sendbuf,
								4, NULL);
		} else {
			DBG_ERR("Fastboot write error, write 0x%x\n",
							g_fastboot_datalen);
			sprintf((char *)g_fastboot_sendbuf, "FAIL");
			udc_send_data(g_fastboot_inep_index, g_fastboot_sendbuf,
								4, NULL);
		}
		g_fastboot_datalen = 0;
		fastboot_status = FASTBOOT_STS_CMD;
    } else if (memcmp(recvbuf, "reboot", 6) == 0) {
			sprintf((char *)g_fastboot_sendbuf, "OKAY");
			udc_send_data(g_fastboot_inep_index, g_fastboot_sendbuf,
								4, NULL);
			udelay(100000); /* 1 sec */

			do_reset(NULL, 0, 0, NULL);

    } else {
		DBG_ERR("Not support command:%s\n", recvbuf);
		sprintf((char *)g_fastboot_sendbuf, "FAIL");
		udc_send_data(g_fastboot_inep_index, g_fastboot_sendbuf,
								4, NULL);
		g_fastboot_datalen = 0;
		fastboot_status = FASTBOOT_STS_CMD;
    }
}

static struct cmd_fastboot_interface interface = {
    .rx_handler            = NULL,
    .reset_handler         = NULL,
    .product_name          = NULL,
    .serial_no             = NULL,
    .nand_block_size       = 0,
    .transfer_buffer       = (unsigned char *)0xffffffff,
    .transfer_buffer_size  = 0,
};

/*
 * fastboot main process, only support 'download', 'flash' 'reboot' command now
 *
 * @debug  control debug level, support three level now,
 *	   0(normal), 1(debug), 2(info), default is 0
 */
void fastboot_quick(u8 debug)
{
    u32 plug_cnt = 0;
    if (debug > 2)
	debug = 0;
    fastboot_debug_level = debug;

    fastboot_init(&interface);
    fastboot_get_ep_num(&g_fastboot_inep_index, &g_fastboot_outep_index);
    DBG_INFO("g_fastboot_inep_index=%d, g_fastboot_outep_index=%d\n",
		g_fastboot_inep_index, g_fastboot_outep_index);
    while (++plug_cnt) {
	fastboot_status = FASTBOOT_STS_CMD;
	udc_hal_data_init();
	udc_run();
	if (plug_cnt > 1)
		DBG_ALWS("wait usb cable into the connector!\n");
	udc_wait_connect();
	g_usb_connected = 1;
	if (plug_cnt > 1)
		DBG_ALWS("USB Mini b cable Connected!\n");
	while (g_usb_connected) {
		int usb_irq = udc_irq_handler();
		if (usb_irq > 0) {
			if (fastboot_status == FASTBOOT_STS_CMD) {
				memset(g_fastboot_recvbuf, 0 , MAX_PAKET_LEN);
				udc_recv_data(g_fastboot_outep_index,
					g_fastboot_recvbuf, MAX_PAKET_LEN,
					fastboot_cmd_handler);
				fastboot_status = FASTBOOT_STS_CMD_WAIT;
			}
		}
		if (usb_irq < 0)
			g_usb_connected = 0;
	}
    }
}
