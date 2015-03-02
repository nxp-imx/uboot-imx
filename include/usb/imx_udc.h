/*
 * Copyright (C) 2010-2015 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _IMX_UDC_H_
#define _IMX_UDC_H_

#include <usbdevice.h>

#define USB_OTGREGS_BASE	(OTG_BASE_ADDR + 0x000)
#define USB_H1REGS_BASE		(OTG_BASE_ADDR + 0x200)
#define USB_H2REGS_BASE		(OTG_BASE_ADDR + 0x400)
#if (defined CONFIG_MX51 || defined CONFIG_MX50 || defined CONFIG_MX6Q \
     || defined CONFIG_MX53 || defined CONFIG_MX6DL || defined CONFIG_MX6SL)
#define USB_H3REGS_BASE		(OTG_BASE_ADDR + 0x600)
#define USB_OTHERREGS_BASE	(OTG_BASE_ADDR + 0x800)
#elif (defined CONFIG_MX7)
#define USB_OTHERREGS_BASE	(OTG_BASE_ADDR + 0x200)
#else
#define USB_OTHERREGS_BASE	(OTG_BASE_ADDR + 0x600)
#endif

#define USBOTG_REG32(offset)	(USB_OTGREGS_BASE + (offset))
#define USBOTG_REG16(offset)	(USB_OTGREGS_BASE + (offset))
#define USBOTHER_REG(offset)	(USB_OTHERREGS_BASE + (offset))

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

/*
 * other regs (not part of ARC core)
 */
/* USB Control register */
#define USBCTRL			USBOTHER_REG(0x00)

/* USB OTG mirror register */
#define USB_OTG_MIRROR		USBOTHER_REG(0x04)

/* OTG UTMI PHY Function Control register */
#define USB_PHY_CTR_FUNC	USBOTHER_REG(0x08)

/* OTG UTMI PHY Function Control register */
#define USB_PHY_CTR_FUNC2	USBOTHER_REG(0x0c)

#define USB_CTRL_1		USBOTHER_REG(0x10)
#define USBCTRL_HOST2		USBOTHER_REG(0x14)	/* USB Cotrol Register 1*/
#define USBCTRL_HOST3		USBOTHER_REG(0x18)	/* USB Cotrol Register 1*/
#define USBH1_PHY_CTRL0		USBOTHER_REG(0x1c)	/* USB Cotrol Register 1*/
#define USBH1_PHY_CTRL1		USBOTHER_REG(0x20)	/* USB Cotrol Register 1*/

/* USB Clock on/off Control Register */
#define USB_CLKONOFF_CTRL       USBOTHER_REG(0x24)

/* mx6x other regs */
/* USB OTG Control register */
#define USB_OTG_CTRL			USBOTHER_REG(0x00)

/* USB H1 Control register */
#define USB_H1_CTRL			USBOTHER_REG(0x04)

/* USB H2 Control register */
#define USB_H2_CTRL			USBOTHER_REG(0x08)

/* USB H3 Control register */
#define USB_H3_CTRL			USBOTHER_REG(0x0c)

/* USB Host2 HSIC Control Register */
#define USB_UH2_HSIC_CTRL		USBOTHER_REG(0x10)

/* USB Host3 HSIC Control Register */
#define USB_UH3_HSIC_CTRL		USBOTHER_REG(0x14)

/* OTG UTMI PHY Control 0 Register */
#define USB_OTG_PHY_CTRL_0		USBOTHER_REG(0x18)

/* OTG UTMI PHY Control 1 Register */
#define USB_H1_PHY_CTRL_0		USBOTHER_REG(0x1c)

/* USB Host2 HSIC DLL Configuration Register 1 */
#define USB_UH2_HSIC_DLL_CFG1		USBOTHER_REG(0x20)

/* USB Host2 HSIC DLL Configuration Register 2 */
#define USB_UH2_HSIC_DLL_CFG2		USBOTHER_REG(0x24)

/* USB Host2 HSIC DLL Configuration Register 3 */
#define USB_UH2_HSIC_DLL_CFG3		USBOTHER_REG(0x28)

/* USB Host3 HSIC DLL Configuration Register 1 */
#define USB_UH3_HSIC_DLL_CFG1		USBOTHER_REG(0x30)

/* USB Host3 HSIC DLL Configuration Register 2 */
#define USB_UH3_HSIC_DLL_CFG2		USBOTHER_REG(0x34)

/* USB Host3 HSIC DLL Configuration Register 3 */
#define USB_UH3_HSIC_DLL_CFG3		USBOTHER_REG(0x38)


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
	unsigned int item_unaligned_addr;
	unsigned int page_vir;
	unsigned int page_unaligned;
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

/* mx6q's register bit begins*/

/* OTG CTRL - H3 CTRL */
#define UCTRL_OWIR		(1 << 31)	/* OTG wakeup intr request received */
/* bit 18 - bit 30 is reserved at mx6q */
#define UCTRL_WKUP_VBUS_EN	(1 << 17)	/* OTG wake-up on VBUS change enable */
#define UCTRL_WKUP_ID_EN	(1 << 16)	/* OTG wake-up on ID change enable */
#define UCTRL_WKUP_SW		(1 << 15)	/* OTG Software Wake-up */
#define UCTRL_WKUP_SW_EN	(1 << 14)	/* OTG Software Wake-up enable */
#define UCTRL_UTMI_ON_CLOCK	(1 << 13)	/* Force OTG UTMI PHY clock output
										     on even if suspend mode */
#define UCTRL_SUSPENDM		(1 << 12)	/* Force OTG UTMI PHY Suspend */
#define UCTRL_RESET		(1 << 11)	/* Force OTG UTMI PHY Reset */
#define UCTRL_OWIE		(1 << 10)	/* OTG wakeup intr request received */
#define UCTRL_PM		(1 << 9)	/* OTG Power Mask */
#define UCTRL_OVER_CUR_POL	(1 << 8)	/* OTG Polarity of Overcurrent */
#define UCTRL_OVER_CUR_DIS	(1 << 7)	/* Disable OTG Overcurrent Detection */
/* bit 0 - bit 6 is reserved at mx6q */

/* Host2/3 HSIC Ctrl */
#define CLK_VLD		(1 << 31)	/* Indicating whether HSIC clock is valid */
#define HSIC_EN		(1 << 12)	/* HSIC enable */
#define HSIC_CLK_ON		(1 << 11)	/* Force HSIC module 480M clock on,
						 * even when in Host is in suspend mode
						 */
/* OTG/HOST1 Phy Ctrl */
#define PHY_UTMI_CLK_VLD	(1 << 31)/* Indicating whether OTG UTMI PHY Clock Valid*/

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
void mxc_udc_rxqueue_update(u8 ep, u32 len);
int is_usb_disconnected(void);
void reset_usb_phy1(void);
void set_usboh3_clk(void);
void set_usb_phy1_clk(void);
void enable_usb_phy1_clk(unsigned char enable);
void enable_usboh3_clk(unsigned char enable);
void udc_pins_setting(void);

/*destroy functions*/
void udc_destroy_ep(struct usb_device_instance *device,
		    struct usb_endpoint_instance *epi);
int udc_destroy(void);


#ifdef CONFIG_FASTBOOT

#define EP0_OUT_INDEX    0
#define EP0_IN_INDEX    16
#define EP1_OUT_INDEX    1
#define EP1_IN_INDEX    17
#define EP2_OUT_INDEX    2
#define EP2_IN_INDEX    18
#define EP3_OUT_INDEX    3
#define EP3_IN_INDEX    19
#define EP4_OUT_INDEX    4
#define EP4_IN_INDEX    20
#define EP5_OUT_INDEX    5
#define EP5_IN_INDEX    21
#define EP6_OUT_INDEX    6
#define EP6_IN_INDEX    22
#define EP7_OUT_INDEX    7
#define EP7_IN_INDEX    23
#define EP8_OUT_INDEX    8
#define EP8_IN_INDEX    24
#define EP9_OUT_INDEX    9
#define EP9_IN_INDEX    25
#define EP10_OUT_INDEX  10
#define EP10_IN_INDEX   26
#define EP11_OUT_INDEX  11
#define EP11_IN_INDEX   27
#define EP12_OUT_INDEX  12
#define EP12_IN_INDEX   28
#define EP13_OUT_INDEX  13
#define EP13_IN_INDEX   29
#define EP14_OUT_INDEX  14
#define EP14_IN_INDEX   30
#define EP15_OUT_INDEX  15
#define EP15_IN_INDEX   31

#define MAX_PAKET_LEN 512
typedef void (*EP_HANDLER_P)(u32 index, u8 *buf);

int  udc_irq_handler(void);
void udc_hal_data_init(void);
void udc_wait_connect(void);
void udc_run(void);
int  udc_recv_data(u32 index, u8 *recvbuf, u32 recvlen, EP_HANDLER_P cb);
int  udc_send_data(u32 index, u8 *buf, u32 sendlen, EP_HANDLER_P cb);
void udc_qh_dtd_init(u32 index);
void udc_dtd_setup(u32 index, u8 ep_type);
void udc_qh_setup(u32 index, u8 ep_type, u32 max_pkt_len, u32 zlt, u8 mult);
u8  *udc_get_descriptor(u8 type, u8 *plen);
void udc_set_addr(u8 addr);
void udc_set_configure(u8 config);

#endif  /* CONFIG_FASTBOOT */

#endif
