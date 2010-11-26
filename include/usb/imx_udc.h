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

#ifndef _IMX_UDC_H_
#define _IMX_UDC_H_

#define USB_ID               (OTG_BASE_ADDR + 0x0000)
#define USB_HWGENERAL        (OTG_BASE_ADDR + 0x0004)
#define USB_HWHOST           (OTG_BASE_ADDR + 0x0008)
#define USB_HWDEVICE         (OTG_BASE_ADDR + 0x000C)
#define USB_HWTXBUF          (OTG_BASE_ADDR + 0x0010)
#define USB_HWRXBUF          (OTG_BASE_ADDR + 0x0014)
#define USB_SBUSCFG          (OTG_BASE_ADDR + 0x0090)

#define USB_CAPLENGTH        (OTG_BASE_ADDR + 0x0100) /* 8 bit */
#define USB_HCIVERSION       (OTG_BASE_ADDR + 0x0102) /* 16 bit */
#define USB_HCSPARAMS        (OTG_BASE_ADDR + 0x0104)
#define USB_HCCPARAMS        (OTG_BASE_ADDR + 0x0108)
#define USB_DCIVERSION       (OTG_BASE_ADDR + 0x0120) /* 16 bit */
#define USB_DCCPARAMS        (OTG_BASE_ADDR + 0x0124)
#define USB_USBCMD           (OTG_BASE_ADDR + 0x0140)
#define USB_USBSTS           (OTG_BASE_ADDR + 0x0144)
#define USB_USBINTR          (OTG_BASE_ADDR + 0x0148)
#define USB_FRINDEX          (OTG_BASE_ADDR + 0x014C)
#define USB_DEVICEADDR       (OTG_BASE_ADDR + 0x0154)
#define USB_ENDPOINTLISTADDR (OTG_BASE_ADDR + 0x0158)
#define USB_BURSTSIZE        (OTG_BASE_ADDR + 0x0160)
#define USB_TXFILLTUNING     (OTG_BASE_ADDR + 0x0164)
#define USB_ULPI_VIEWPORT    (OTG_BASE_ADDR + 0x0170)
#define USB_ENDPTNAK         (OTG_BASE_ADDR + 0x0178)
#define USB_ENDPTNAKEN       (OTG_BASE_ADDR + 0x017C)
#define USB_PORTSC1          (OTG_BASE_ADDR + 0x0184)
#define USB_OTGSC            (OTG_BASE_ADDR + 0x01A4)
#define USB_USBMODE          (OTG_BASE_ADDR + 0x01A8)
#define USB_ENDPTSETUPSTAT   (OTG_BASE_ADDR + 0x01AC)
#define USB_ENDPTPRIME       (OTG_BASE_ADDR + 0x01B0)
#define USB_ENDPTFLUSH       (OTG_BASE_ADDR + 0x01B4)
#define USB_ENDPTSTAT        (OTG_BASE_ADDR + 0x01B8)
#define USB_ENDPTCOMPLETE    (OTG_BASE_ADDR + 0x01BC)
#define USB_ENDPTCTRL(n)     (OTG_BASE_ADDR + 0x01C0 + (4 * (n)))

#define USB_PHY1_CTRL        (OTG_BASE_ADDR + 0x80C)
#define USBCMD_RESET   2
#define USBCMD_ATTACH  1

#define USBMODE_DEVICE 2
#define USBMODE_HOST   3

struct ep_queue_head {
	volatile unsigned int config;
	volatile unsigned int current; /* read-only */

	volatile unsigned int next_queue_item;
	volatile unsigned int info;
	volatile unsigned int page0;
	volatile unsigned int page1;
	volatile unsigned int page2;
	volatile unsigned int page3;
	volatile unsigned int page4;
	volatile unsigned int reserved_0;

	volatile unsigned char setup_data[8];
	volatile unsigned int reserved[4];
};

#define CONFIG_MAX_PKT(n)     ((n) << 16)
#define CONFIG_ZLT            (1 << 29)    /* stop on zero-len xfer */
#define CONFIG_IOS            (1 << 15)    /* IRQ on setup */

struct ep_queue_item {
	volatile unsigned int next_item_ptr;
	volatile unsigned int info;
	volatile unsigned int page0;
	volatile unsigned int page1;
	volatile unsigned int page2;
	volatile unsigned int page3;
	volatile unsigned int page4;
	unsigned int item_dma;
	unsigned int page_vir;
	unsigned int page_dma;
	struct ep_queue_item *next_item_vir;
	volatile unsigned int reserved[5];
};

#define TERMINATE 1

#define INFO_BYTES(n)         ((n) << 16)
#define INFO_IOC              (1 << 15)
#define INFO_ACTIVE           (1 << 7)
#define INFO_HALTED           (1 << 6)
#define INFO_BUFFER_ERROR     (1 << 5)
#define INFO_TX_ERROR         (1 << 3)

/* Device Controller Capability Parameter register */
#define DCCPARAMS_DC				0x00000080
#define DCCPARAMS_DEN_MASK			0x0000001f

/* Frame Index Register Bit Masks */
#define	USB_FRINDEX_MASKS			(0x3fff)
/* USB CMD  Register Bit Masks */
#define  USB_CMD_RUN_STOP                     (0x00000001)
#define  USB_CMD_CTRL_RESET                   (0x00000002)
#define  USB_CMD_PERIODIC_SCHEDULE_EN         (0x00000010)
#define  USB_CMD_ASYNC_SCHEDULE_EN            (0x00000020)
#define  USB_CMD_INT_AA_DOORBELL              (0x00000040)
#define  USB_CMD_ASP                          (0x00000300)
#define  USB_CMD_ASYNC_SCH_PARK_EN            (0x00000800)
#define  USB_CMD_SUTW                         (0x00002000)
#define  USB_CMD_ATDTW                        (0x00004000)
#define  USB_CMD_ITC                          (0x00FF0000)

/* bit 15,3,2 are frame list size */
#define  USB_CMD_FRAME_SIZE_1024              (0x00000000)
#define  USB_CMD_FRAME_SIZE_512               (0x00000004)
#define  USB_CMD_FRAME_SIZE_256               (0x00000008)
#define  USB_CMD_FRAME_SIZE_128               (0x0000000C)
#define  USB_CMD_FRAME_SIZE_64                (0x00008000)
#define  USB_CMD_FRAME_SIZE_32                (0x00008004)
#define  USB_CMD_FRAME_SIZE_16                (0x00008008)
#define  USB_CMD_FRAME_SIZE_8                 (0x0000800C)

/* bit 9-8 are async schedule park mode count */
#define  USB_CMD_ASP_00                       (0x00000000)
#define  USB_CMD_ASP_01                       (0x00000100)
#define  USB_CMD_ASP_10                       (0x00000200)
#define  USB_CMD_ASP_11                       (0x00000300)
#define  USB_CMD_ASP_BIT_POS                  (8)

/* bit 23-16 are interrupt threshold control */
#define  USB_CMD_ITC_NO_THRESHOLD             (0x00000000)
#define  USB_CMD_ITC_1_MICRO_FRM              (0x00010000)
#define  USB_CMD_ITC_2_MICRO_FRM              (0x00020000)
#define  USB_CMD_ITC_4_MICRO_FRM              (0x00040000)
#define  USB_CMD_ITC_8_MICRO_FRM              (0x00080000)
#define  USB_CMD_ITC_16_MICRO_FRM             (0x00100000)
#define  USB_CMD_ITC_32_MICRO_FRM             (0x00200000)
#define  USB_CMD_ITC_64_MICRO_FRM             (0x00400000)
#define  USB_CMD_ITC_BIT_POS                  (16)

/* USB STS Register Bit Masks */
#define  USB_STS_INT                          (0x00000001)
#define  USB_STS_ERR                          (0x00000002)
#define  USB_STS_PORT_CHANGE                  (0x00000004)
#define  USB_STS_FRM_LST_ROLL                 (0x00000008)
#define  USB_STS_SYS_ERR                      (0x00000010)
#define  USB_STS_IAA                          (0x00000020)
#define  USB_STS_RESET                        (0x00000040)
#define  USB_STS_SOF                          (0x00000080)
#define  USB_STS_SUSPEND                      (0x00000100)
#define  USB_STS_HC_HALTED                    (0x00001000)
#define  USB_STS_RCL                          (0x00002000)
#define  USB_STS_PERIODIC_SCHEDULE            (0x00004000)
#define  USB_STS_ASYNC_SCHEDULE               (0x00008000)

/* USB INTR Register Bit Masks */
#define  USB_INTR_INT_EN                      (0x00000001)
#define  USB_INTR_ERR_INT_EN                  (0x00000002)
#define  USB_INTR_PTC_DETECT_EN               (0x00000004)
#define  USB_INTR_FRM_LST_ROLL_EN             (0x00000008)
#define  USB_INTR_SYS_ERR_EN                  (0x00000010)
#define  USB_INTR_ASYN_ADV_EN                 (0x00000020)
#define  USB_INTR_RESET_EN                    (0x00000040)
#define  USB_INTR_SOF_EN                      (0x00000080)
#define  USB_INTR_DEVICE_SUSPEND              (0x00000100)

/* Device Address bit masks */
#define  USB_DEVICE_ADDRESS_MASK              (0xFE000000)
#define  USB_DEVICE_ADDRESS_BIT_POS           (25)

/* endpoint list address bit masks */
#define USB_EP_LIST_ADDRESS_MASK              (0xfffff800)

/* PORTSCX  Register Bit Masks */
#define  PORTSCX_CURRENT_CONNECT_STATUS       (0x00000001)
#define  PORTSCX_CONNECT_STATUS_CHANGE        (0x00000002)
#define  PORTSCX_PORT_ENABLE                  (0x00000004)
#define  PORTSCX_PORT_EN_DIS_CHANGE           (0x00000008)
#define  PORTSCX_OVER_CURRENT_ACT             (0x00000010)
#define  PORTSCX_OVER_CURRENT_CHG             (0x00000020)
#define  PORTSCX_PORT_FORCE_RESUME            (0x00000040)
#define  PORTSCX_PORT_SUSPEND                 (0x00000080)
#define  PORTSCX_PORT_RESET                   (0x00000100)
#define  PORTSCX_LINE_STATUS_BITS             (0x00000C00)
#define  PORTSCX_PORT_POWER                   (0x00001000)
#define  PORTSCX_PORT_INDICTOR_CTRL           (0x0000C000)
#define  PORTSCX_PORT_TEST_CTRL               (0x000F0000)
#define  PORTSCX_WAKE_ON_CONNECT_EN           (0x00100000)
#define  PORTSCX_WAKE_ON_CONNECT_DIS          (0x00200000)
#define  PORTSCX_WAKE_ON_OVER_CURRENT         (0x00400000)
#define  PORTSCX_PHY_LOW_POWER_SPD            (0x00800000)
#define  PORTSCX_PORT_FORCE_FULL_SPEED        (0x01000000)
#define  PORTSCX_PORT_SPEED_MASK              (0x0C000000)
#define  PORTSCX_PORT_WIDTH                   (0x10000000)
#define  PORTSCX_PHY_TYPE_SEL                 (0xC0000000)

/* bit 11-10 are line status */
#define  PORTSCX_LINE_STATUS_SE0              (0x00000000)
#define  PORTSCX_LINE_STATUS_JSTATE           (0x00000400)
#define  PORTSCX_LINE_STATUS_KSTATE           (0x00000800)
#define  PORTSCX_LINE_STATUS_UNDEF            (0x00000C00)
#define  PORTSCX_LINE_STATUS_BIT_POS          (10)

/* bit 15-14 are port indicator control */
#define  PORTSCX_PIC_OFF                      (0x00000000)
#define  PORTSCX_PIC_AMBER                    (0x00004000)
#define  PORTSCX_PIC_GREEN                    (0x00008000)
#define  PORTSCX_PIC_UNDEF                    (0x0000C000)
#define  PORTSCX_PIC_BIT_POS                  (14)

/* bit 19-16 are port test control */
#define  PORTSCX_PTC_DISABLE                  (0x00000000)
#define  PORTSCX_PTC_JSTATE                   (0x00010000)
#define  PORTSCX_PTC_KSTATE                   (0x00020000)
#define  PORTSCX_PTC_SEQNAK                   (0x00030000)
#define  PORTSCX_PTC_PACKET                   (0x00040000)
#define  PORTSCX_PTC_FORCE_EN                 (0x00050000)
#define  PORTSCX_PTC_BIT_POS                  (16)

/* bit 27-26 are port speed */
#define  PORTSCX_PORT_SPEED_FULL              (0x00000000)
#define  PORTSCX_PORT_SPEED_LOW               (0x04000000)
#define  PORTSCX_PORT_SPEED_HIGH              (0x08000000)
#define  PORTSCX_PORT_SPEED_UNDEF             (0x0C000000)
#define  PORTSCX_SPEED_BIT_POS                (26)

/* OTGSC Register Bit Masks */
#define  OTGSC_B_SESSION_VALID_IRQ_EN           (1 << 27)
#define  OTGSC_B_SESSION_VALID_IRQ_STS          (1 << 19)
#define  OTGSC_B_SESSION_VALID                  (1 << 11)

/* bit 28 is parallel transceiver width for UTMI interface */
#define  PORTSCX_PTW                          (0x10000000)
#define  PORTSCX_PTW_8BIT                     (0x00000000)
#define  PORTSCX_PTW_16BIT                    (0x10000000)

/* bit 31-30 are port transceiver select */
#define  PORTSCX_PTS_UTMI                     (0x00000000)
#define  PORTSCX_PTS_ULPI                     (0x80000000)
#define  PORTSCX_PTS_FSLS                     (0xC0000000)
#define  PORTSCX_PTS_BIT_POS                  (30)

/* USB MODE Register Bit Masks */
#define  USB_MODE_CTRL_MODE_IDLE              (0x00000000)
#define  USB_MODE_CTRL_MODE_DEVICE            (0x00000002)
#define  USB_MODE_CTRL_MODE_HOST              (0x00000003)
#define  USB_MODE_CTRL_MODE_MASK              0x00000003
#define  USB_MODE_CTRL_MODE_RSV               (0x00000001)
#define  USB_MODE_ES                          0x00000004 /* (big) Endian Sel */
#define  USB_MODE_SETUP_LOCK_OFF              (0x00000008)
#define  USB_MODE_STREAM_DISABLE              (0x00000010)
/* Endpoint Flush Register */
#define EPFLUSH_TX_OFFSET		      (0x00010000)
#define EPFLUSH_RX_OFFSET		      (0x00000000)

/* Endpoint Setup Status bit masks */
#define  EP_SETUP_STATUS_MASK                 (0x0000003F)
#define  EP_SETUP_STATUS_EP0		      (0x00000001)

/* ENDPOINTCTRLx  Register Bit Masks */
#define  EPCTRL_TX_ENABLE                     (0x00800000)
#define  EPCTRL_TX_DATA_TOGGLE_RST            (0x00400000)	/* Not EP0 */
#define  EPCTRL_TX_DATA_TOGGLE_INH            (0x00200000)	/* Not EP0 */
#define  EPCTRL_TX_TYPE                       (0x000C0000)
#define  EPCTRL_TX_DATA_SOURCE                (0x00020000)	/* Not EP0 */
#define  EPCTRL_TX_EP_STALL                   (0x00010000)
#define  EPCTRL_RX_ENABLE                     (0x00000080)
#define  EPCTRL_RX_DATA_TOGGLE_RST            (0x00000040)	/* Not EP0 */
#define  EPCTRL_RX_DATA_TOGGLE_INH            (0x00000020)	/* Not EP0 */
#define  EPCTRL_RX_TYPE                       (0x0000000C)
#define  EPCTRL_RX_DATA_SINK                  (0x00000002)	/* Not EP0 */
#define  EPCTRL_RX_EP_STALL                   (0x00000001)

/* bit 19-18 and 3-2 are endpoint type */
#define  EPCTRL_EP_TYPE_CONTROL               (0)
#define  EPCTRL_EP_TYPE_ISO                   (1)
#define  EPCTRL_EP_TYPE_BULK                  (2)
#define  EPCTRL_EP_TYPE_INTERRUPT             (3)
#define  EPCTRL_TX_EP_TYPE_SHIFT              (18)
#define  EPCTRL_RX_EP_TYPE_SHIFT              (2)

/* SNOOPn Register Bit Masks */
#define  SNOOP_ADDRESS_MASK                   (0xFFFFF000)
#define  SNOOP_SIZE_ZERO                      (0x00)	/* snooping disable */
#define  SNOOP_SIZE_4KB                       (0x0B)	/* 4KB snoop size */
#define  SNOOP_SIZE_8KB                       (0x0C)
#define  SNOOP_SIZE_16KB                      (0x0D)
#define  SNOOP_SIZE_32KB                      (0x0E)
#define  SNOOP_SIZE_64KB                      (0x0F)
#define  SNOOP_SIZE_128KB                     (0x10)
#define  SNOOP_SIZE_256KB                     (0x11)
#define  SNOOP_SIZE_512KB                     (0x12)
#define  SNOOP_SIZE_1MB                       (0x13)
#define  SNOOP_SIZE_2MB                       (0x14)
#define  SNOOP_SIZE_4MB                       (0x15)
#define  SNOOP_SIZE_8MB                       (0x16)
#define  SNOOP_SIZE_16MB                      (0x17)
#define  SNOOP_SIZE_32MB                      (0x18)
#define  SNOOP_SIZE_64MB                      (0x19)
#define  SNOOP_SIZE_128MB                     (0x1A)
#define  SNOOP_SIZE_256MB                     (0x1B)
#define  SNOOP_SIZE_512MB                     (0x1C)
#define  SNOOP_SIZE_1GB                       (0x1D)
#define  SNOOP_SIZE_2GB                       (0x1E)	/* 2GB snoop size */

/* pri_ctrl Register Bit Masks */
#define  PRI_CTRL_PRI_LVL1                    (0x0000000C)
#define  PRI_CTRL_PRI_LVL0                    (0x00000003)

/* si_ctrl Register Bit Masks */
#define  SI_CTRL_ERR_DISABLE                  (0x00000010)
#define  SI_CTRL_IDRC_DISABLE                 (0x00000008)
#define  SI_CTRL_RD_SAFE_EN                   (0x00000004)
#define  SI_CTRL_RD_PREFETCH_DISABLE          (0x00000002)
#define  SI_CTRL_RD_PREFEFETCH_VAL            (0x00000001)

/* control Register Bit Masks */
#define  USB_CTRL_IOENB                       (0x00000004)
#define  USB_CTRL_ULPI_INT0EN                 (0x00000001)
#define  USB_CTRL_OTG_WUIR                   (0x80000000)
#define  USB_CTRL_OTG_WUIE                   (0x08000000)
#define  USB_CTRL_OTG_VWUE			(0x00001000)
#define  USB_CTRL_OTG_IWUE			(0x00100000)



#define INTR_UE		      (1 << 0)
#define INTR_UEE	      (1 << 1)
#define INTR_PCE	      (1 << 2)
#define INTR_SEE	      (1 << 4)
#define INTR_URE	      (1 << 6)
#define INTR_SRE	      (1 << 7)
#define INTR_SLE	      (1 << 8)


/* bits used in all the endpoint status registers */
#define EPT_TX(n) (1 << ((n) + 16))
#define EPT_RX(n) (1 << (n))


#define CTRL_TXE              (1 << 23)
#define CTRL_TXR              (1 << 22)
#define CTRL_TXI              (1 << 21)
#define CTRL_TXD              (1 << 17)
#define CTRL_TXS              (1 << 16)
#define CTRL_RXE              (1 << 7)
#define CTRL_RXR              (1 << 6)
#define CTRL_RXI              (1 << 5)
#define CTRL_RXD              (1 << 1)
#define CTRL_RXS              (1 << 0)

#define CTRL_TXT_CTRL         (0 << 18)
#define CTRL_TXT_ISOCH        (1 << 18)
#define CTRL_TXT_BULK         (2 << 18)
#define CTRL_TXT_INT          (3 << 18)

#define CTRL_RXT_CTRL         (0 << 2)
#define CTRL_RXT_ISOCH        (1 << 2)
#define CTRL_RXT_BULK         (2 << 2)
#define CTRL_RXT_INT          (3 << 2)

#define USB_RECV 0
#define USB_SEND 1
#define USB_MAX_CTRL_PAYLOAD 64

/* UDC device defines */
#define EP0_MAX_PACKET_SIZE     USB_MAX_CTRL_PAYLOAD
#define UDC_OUT_ENDPOINT        0x02
#define UDC_OUT_PACKET_SIZE     USB_MAX_CTRL_PAYLOAD
#define UDC_IN_ENDPOINT         0x03
#define UDC_IN_PACKET_SIZE      USB_MAX_CTRL_PAYLOAD
#define UDC_INT_ENDPOINT        0x01
#define UDC_INT_PACKET_SIZE     USB_MAX_CTRL_PAYLOAD
#define UDC_BULK_PACKET_SIZE    USB_MAX_CTRL_PAYLOAD

int  udc_init(void);

void udc_enable(struct usb_device_instance *device);
void udc_disable(void);

void udc_connect(void);
void udc_disconnect(void);

void udc_startup_events(struct usb_device_instance *device);
void udc_setup_ep(struct usb_device_instance *device,
	unsigned int ep, struct usb_endpoint_instance *endpoint);
int udc_endpoint_write(struct usb_endpoint_instance *epi);
void udc_irq(void);
void usb_shutdown(void);

int is_usb_disconnected(void);

#endif
