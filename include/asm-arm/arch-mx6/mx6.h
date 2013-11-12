/*
 * Copyright (C) 2011-2013 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __ASM_ARCH_MXC_MX6_H__
#define __ASM_ARCH_MXC_MX6_H__

/*
 * Some of i.MX 6 Series SoC registers are associated with four addresses
 * used for different operations - read/write, set, clear and toggle bits.
 *
 * Some of registers do not implement such feature and, thus, should be
 * accessed/manipulated via single address in common way.
 */
#define REG_RD(base, reg) \
	(*(volatile unsigned int *)((base) + (reg)))
#define REG_WR(base, reg, value) \
	((*(volatile unsigned int *)((base) + (reg))) = (value))
#define REG_SET(base, reg, value) \
	((*(volatile unsigned int *)((base) + (reg ## _SET))) = (value))
#define REG_CLR(base, reg, value) \
	((*(volatile unsigned int *)((base) + (reg ## _CLR))) = (value))
#define REG_TOG(base, reg, value) \
	((*(volatile unsigned int *)((base) + (reg ## _TOG))) = (value))

#define REG_RD_ADDR(addr) \
	(*(volatile unsigned int *)((addr)))
#define REG_WR_ADDR(addr, value) \
	((*(volatile unsigned int *)((addr))) = (value))
#define REG_SET_ADDR(addr, value) \
	((*(volatile unsigned int *)((addr) + 0x4)) = (value))
#define REG_CLR_ADDR(addr, value) \
	((*(volatile unsigned int *)((addr) + 0x8)) = (value))
#define REG_TOG_ADDR(addr, value) \
	((*(volatile unsigned int *)((addr) + 0xc)) = (value))


#define __REG(x)        (*((volatile u32 *)(x)))
#define __REG16(x)      (*((volatile u16 *)(x)))
#define __REG8(x)       (*((volatile u8 *)(x)))

/*
 *ROM address which denotes silicon rev
 */
#define ROM_SI_REV     0x48

/*!
 * @file arch-mxc/mx6.h
 * @brief This file contains register definitions.
 *
 * @ingroup MSL_MX6
 */

/*
 * IPU
 */
#define IPU_CTRL_BASE_ADDR	0x02400000

/*!
 * Register an interrupt handler for the SMN as well as the SCC.  In some
 * implementations, the SMN is not connected at all, and in others, it is
 * on the same interrupt line as the SCM. Comment this line out accordingly
 */
#define USE_SMN_INTERRUPT

/*!
 * This option is used to set or clear the RXDMUXSEL bit in control reg 3.
 * Certain platforms need this bit to be set in order to receive Irda data.
 */
#define MXC_UART_IR_RXDMUX      0x0004
/*!
 * This option is used to set or clear the RXDMUXSEL bit in control reg 3.
 * Certain platforms need this bit to be set in order to receive UART data.
 */
#define MXC_UART_RXDMUX         0x0004

/*!
 * This option is used to set or clear the dspdma bit in the SDMA config
 * register.
 */
#define MXC_DMA_REQ_DSPDMA         0

/*!
 * Define this option to specify we are using the newer SDMA module.
 */
#define MXC_DMA_REQ_V2

/*!
 * The maximum frequency that the pixel clock can be at so as to
 * activate DVFS-PER.
 */
#define DVFS_MAX_PIX_CLK	54000000

/*!
 * The Low 32 bits of CPU Serial Number Fuse Index
 */
#define CPU_UID_LOW_FUSE_INDEX         1

/*!
 * The High 32 bits of CPU Serial Number Fuse Index
 */
#define CPU_UID_HIGH_FUSE_INDEX         2


/* IROM
 */
#define IROM_BASE_ADDR		0x0
#define IROM_SIZE			SZ_64K

/* CPU Memory Map */
#ifdef CONFIG_MX6SL
#define MMDC0_ARB_BASE_ADDR             0x80000000
#define MMDC0_ARB_END_ADDR              0xFFFFFFFF
#define MMDC1_ARB_BASE_ADDR             0xC0000000
#define MMDC1_ARB_END_ADDR              0xFFFFFFFF
#else
#define MMDC0_ARB_BASE_ADDR             0x10000000
#define MMDC0_ARB_END_ADDR              0x7FFFFFFF
#define MMDC1_ARB_BASE_ADDR             0x80000000
#define MMDC1_ARB_END_ADDR              0xFFFFFFFF
#endif
#define OCRAM_ARB_BASE_ADDR             0x00900000
#define OCRAM_ARB_END_ADDR              0x009FFFFF
#define IRAM_BASE_ADDR                  OCRAM_ARB_BASE_ADDR
#define PCIE_ARB_BASE_ADDR              0x01000000
#define PCIE_ARB_END_ADDR               0x01FFFFFF

/* Blocks connected via pl301periph */
#define ROMCP_ARB_BASE_ADDR             0x00000000
#define ROMCP_ARB_END_ADDR              0x00017FFF
#define BOOT_ROM_BASE_ADDR              ROMCP_ARB_BASE_ADDR
#ifdef CONFIG_MX6SL
#define GPU_2D_ARB_BASE_ADDR            0x02200000
#define GPU_2D_ARB_END_ADDR             0x02203FFF
#define OPENVG_ARB_BASE_ADDR            0x02204000
#define OPENVG_ARB_END_ADDR             0x02207FFF
#else
#define CAAM_ARB_BASE_ADDR              0x00100000
#define CAAM_ARB_END_ADDR               0x00103FFF
#define APBH_DMA_ARB_BASE_ADDR          0x00110000
#define APBH_DMA_ARB_END_ADDR           0x00117FFF
#define HDMI_ARB_BASE_ADDR              0x00120000
#define HDMI_ARB_END_ADDR               0x00128FFF
#define GPU_3D_ARB_BASE_ADDR            0x00130000
#define GPU_3D_ARB_END_ADDR             0x00133FFF
#define GPU_2D_ARB_BASE_ADDR            0x00134000
#define GPU_2D_ARB_END_ADDR             0x00137FFF
#define DTCP_ARB_BASE_ADDR              0x00138000
#define DTCP_ARB_END_ADDR               0x0013BFFF
#define GPU_MEM_BASE_ADDR               GPU_3D_ARB_BASE_ADDR
#endif

/* GPV - PL301 configuration ports */
#define GPV0_BASE_ADDR                  0x00B00000
#define GPV1_BASE_ADDR                  0x00C00000
#ifdef CONFIG_MX6SL
#define GPV2_BASE_ADDR                  0x00D00000
#else
#define GPV2_BASE_ADDR                  0x00200000
#define GPV3_BASE_ADDR                  0x00300000
#define GPV4_BASE_ADDR                  0x00800000
#endif

#define AIPS1_ARB_BASE_ADDR             0x02000000
#define AIPS1_ARB_END_ADDR              0x020FFFFF
#define AIPS2_ARB_BASE_ADDR             0x02100000
#define AIPS2_ARB_END_ADDR              0x021FFFFF

#define SATA_ARB_BASE_ADDR              0x02200000
#define SATA_ARB_END_ADDR               0x02203FFF
#define OPENVG_ARB_BASE_ADDR            0x02204000
#define OPENVG_ARB_END_ADDR             0x02207FFF
#define HSI_ARB_BASE_ADDR               0x02208000
#define HSI_ARB_END_ADDR                0x0220BFFF
#define IPU1_ARB_BASE_ADDR              0x02400000
#define IPU1_ARB_END_ADDR               0x027FFFFF
#define IPU2_ARB_BASE_ADDR              0x02800000
#define IPU2_ARB_END_ADDR               0x02BFFFFF

#define WEIM_ARB_BASE_ADDR              0x08000000
#define WEIM_ARB_END_ADDR               0x0FFFFFFF

/* Legacy Defines */
#define CSD0_DDR_BASE_ADDR              MMDC0_ARB_BASE_ADDR
#define CSD1_DDR_BASE_ADDR              MMDC1_ARB_BASE_ADDR
#define CS0_BASE_ADDR                   WEIM_ARB_BASE_ADDR
#define NAND_FLASH_BASE_ADDR            APBH_DMA_ARB_BASE_ADDR
#define ABPHDMA_BASE_ADDR               APBH_DMA_ARB_BASE_ADDR
#define GPMI_BASE_ADDR                  (APBH_DMA_ARB_BASE_ADDR + 0x02000)
#define BCH_BASE_ADDR                   (APBH_DMA_ARB_BASE_ADDR + 0x04000)

/* Defines for Blocks connected via AIPS (SkyBlue) */
#define ATZ1_BASE_ADDR              AIPS1_ARB_BASE_ADDR
#define ATZ2_BASE_ADDR              AIPS2_ARB_BASE_ADDR

/* slots 0,7 of SDMA reserved, therefore left unused in IPMUX3 */
#define SPDIF_BASE_ADDR             (ATZ1_BASE_ADDR + 0x04000)
#define ECSPI1_BASE_ADDR            (ATZ1_BASE_ADDR + 0x08000)
#define ECSPI2_BASE_ADDR            (ATZ1_BASE_ADDR + 0x0C000)
#define ECSPI3_BASE_ADDR            (ATZ1_BASE_ADDR + 0x10000)
#define ECSPI4_BASE_ADDR            (ATZ1_BASE_ADDR + 0x14000)
#ifdef CONFIG_MX6SL
#define UART5_IPS_BASE_ADDR         (ATZ1_BASE_ADDR + 0x18000)
#define UART1_IPS_BASE_ADDR         (ATZ1_BASE_ADDR + 0x20000)
#define UART2_IPS_BASE_ADDR         (ATZ1_BASE_ADDR + 0x24000)
#define SSI1_IPS_BASE_ADDR          (ATZ1_BASE_ADDR + 0x28000)
#define SSI2_IPS_BASE_ADDR          (ATZ1_BASE_ADDR + 0x2C000)
#define SSI3_IPS_BASE_ADDR          (ATZ1_BASE_ADDR + 0x30000)
#define UART3_IPS_BASE_ADDR         (ATZ1_BASE_ADDR + 0x34000)
#define UART4_IPS_BASE_ADDR         (ATZ1_BASE_ADDR + 0x38000)
#else
#define ECSPI5_BASE_ADDR            (ATZ1_BASE_ADDR + 0x18000)
#define UART1_BASE_ADDR             (ATZ1_BASE_ADDR + 0x20000)
#define ESAI1_BASE_ADDR             (ATZ1_BASE_ADDR + 0x24000)
#define SSI1_BASE_ADDR              (ATZ1_BASE_ADDR + 0x28000)
#define SSI2_BASE_ADDR              (ATZ1_BASE_ADDR + 0x2C000)
#define SSI3_BASE_ADDR              (ATZ1_BASE_ADDR + 0x30000)
#define ASRC_BASE_ADDR              (ATZ1_BASE_ADDR + 0x34000)
#endif
#define SPBA_BASE_ADDR              (ATZ1_BASE_ADDR + 0x3C000)
#define VPU_BASE_ADDR               (ATZ1_BASE_ADDR + 0x40000)

/* ATZ#1- On Platform */
#define AIPS1_ON_BASE_ADDR          (ATZ1_BASE_ADDR + 0x7C000)

/* ATZ#1- Off Platform */
#define AIPS1_OFF_BASE_ADDR         (ATZ1_BASE_ADDR + 0x80000)

#define PWM1_BASE_ADDR              (AIPS1_OFF_BASE_ADDR + 0x0000)
#define PWM2_BASE_ADDR              (AIPS1_OFF_BASE_ADDR + 0x4000)
#define PWM3_BASE_ADDR              (AIPS1_OFF_BASE_ADDR + 0x8000)
#define PWM4_BASE_ADDR              (AIPS1_OFF_BASE_ADDR + 0xC000)
#ifdef CONFIG_MX6SL
#define DBGMON_IPS_BASE_ADDR        (AIPS1_OFF_BASE_ADDR + 0x10000)
#define QOSC_IPS_BASE_ADDR          (AIPS1_OFF_BASE_ADDR + 0x14000)
#else
#define CAN1_BASE_ADDR              (AIPS1_OFF_BASE_ADDR + 0x10000)
#define CAN2_BASE_ADDR              (AIPS1_OFF_BASE_ADDR + 0x14000)
#endif
#define GPT_BASE_ADDR               (AIPS1_OFF_BASE_ADDR + 0x18000)
#define GPIO1_BASE_ADDR             (AIPS1_OFF_BASE_ADDR + 0x1C000)
#define GPIO2_BASE_ADDR             (AIPS1_OFF_BASE_ADDR + 0x20000)
#define GPIO3_BASE_ADDR             (AIPS1_OFF_BASE_ADDR + 0x24000)
#define GPIO4_BASE_ADDR             (AIPS1_OFF_BASE_ADDR + 0x28000)
#define GPIO5_BASE_ADDR             (AIPS1_OFF_BASE_ADDR + 0x2C000)
#define GPIO6_BASE_ADDR             (AIPS1_OFF_BASE_ADDR + 0x30000)
#define GPIO7_BASE_ADDR             (AIPS1_OFF_BASE_ADDR + 0x34000)
#define KPP_BASE_ADDR               (AIPS1_OFF_BASE_ADDR + 0x38000)
#define WDOG1_BASE_ADDR             (AIPS1_OFF_BASE_ADDR + 0x3C000)
#define WDOG2_BASE_ADDR             (AIPS1_OFF_BASE_ADDR + 0x40000)
#define CCM_BASE_ADDR               (AIPS1_OFF_BASE_ADDR + 0x44000)
#define ANATOP_BASE_ADDR            (AIPS1_OFF_BASE_ADDR + 0x48000)
#define USB_PHY0_BASE_ADDR          (AIPS1_OFF_BASE_ADDR + 0x49000)
#define USB_PHY1_BASE_ADDR          (AIPS1_OFF_BASE_ADDR + 0x4A000)
#define SNVS_BASE_ADDR              (AIPS1_OFF_BASE_ADDR + 0x4C000)
#define EPIT1_BASE_ADDR             (AIPS1_OFF_BASE_ADDR + 0x50000)
#define EPIT2_BASE_ADDR             (AIPS1_OFF_BASE_ADDR + 0x54000)
#define SRC_BASE_ADDR               (AIPS1_OFF_BASE_ADDR + 0x58000)
#define GPC_BASE_ADDR               (AIPS1_OFF_BASE_ADDR + 0x5C000)
#define IOMUXC_BASE_ADDR            (AIPS1_OFF_BASE_ADDR + 0x60000)
#ifdef CONFIG_MX6SL
#define CSI_BASE_ADDR               (AIPS1_OFF_BASE_ADDR + 0x64000)
#define SIPIX_BASE_ADDR             (AIPS1_OFF_BASE_ADDR + 0x68000)
#define SDMA_PORT_HOST_BASE_ADDR    (AIPS1_OFF_BASE_ADDR + 0x6C000)
#else
#define DCIC1_BASE_ADDR             (AIPS1_OFF_BASE_ADDR + 0x64000)
#define DCIC2_BASE_ADDR             (AIPS1_OFF_BASE_ADDR + 0x68000)
#define DMA_REQ_PORT_HOST_BASE_ADDR (AIPS1_OFF_BASE_ADDR + 0x6C000)
#endif
#define EPXP_BASE_ADDR              (AIPS1_OFF_BASE_ADDR + 0x70000)
#define EPDC_BASE_ADDR              (AIPS1_OFF_BASE_ADDR + 0x74000)
#define ELCDIF_BASE_ADDR            (AIPS1_OFF_BASE_ADDR + 0x78000)
#define DCP_BASE_ADDR               (AIPS1_OFF_BASE_ADDR + 0x7C000)

/* ATZ#2- On Platform */
#define AIPS2_ON_BASE_ADDR          (ATZ2_BASE_ADDR + 0x7C000)

/* ATZ#2- Off Platform */
#define AIPS2_OFF_BASE_ADDR         (ATZ2_BASE_ADDR + 0x80000)

/* ATZ#2  - Global enable (0) */
#define CAAM_BASE_ADDR              (ATZ2_BASE_ADDR)
#define ARM_BASE_ADDR		    (ATZ2_BASE_ADDR + 0x40000)

#ifdef CONFIG_MX6SL
#define USBO2H_PL301_IPS_BASE_ADDR  (AIPS2_OFF_BASE_ADDR + 0x0000)
#define USBO2H_USB_BASE_ADDR        (AIPS2_OFF_BASE_ADDR + 0x4000)
#else
#define USBOH3_PL301_BASE_ADDR      (AIPS2_OFF_BASE_ADDR + 0x0000)
#define USBOH3_USB_BASE_ADDR        (AIPS2_OFF_BASE_ADDR + 0x4000)
#endif
#define ENET_BASE_ADDR              (AIPS2_OFF_BASE_ADDR + 0x8000)
/* Frank Li Need IC confirm OTG base address*/
#ifdef CONFIG_MX6SL
#define OTG_BASE_ADDR               USBO2H_USB_BASE_ADDR
#define MSHC_IPS_BASE_ADDR          (AIPS2_OFF_BASE_ADDR + 0xC000)
#else
#define OTG_BASE_ADDR               USBOH3_USB_BASE_ADDR
#define MLB_BASE_ADDR               (AIPS2_OFF_BASE_ADDR + 0xC000)
#endif

#define USDHC1_BASE_ADDR            (AIPS2_OFF_BASE_ADDR + 0x10000)
#define USDHC2_BASE_ADDR            (AIPS2_OFF_BASE_ADDR + 0x14000)
#define USDHC3_BASE_ADDR            (AIPS2_OFF_BASE_ADDR + 0x18000)
#define USDHC4_BASE_ADDR            (AIPS2_OFF_BASE_ADDR + 0x1C000)
#define I2C1_BASE_ADDR              (AIPS2_OFF_BASE_ADDR + 0x20000)
#define I2C2_BASE_ADDR              (AIPS2_OFF_BASE_ADDR + 0x24000)
#define I2C3_BASE_ADDR              (AIPS2_OFF_BASE_ADDR + 0x28000)
#define ROMCP_BASE_ADDR             (AIPS2_OFF_BASE_ADDR + 0x2C000)
#define MMDC_P0_BASE_ADDR           (AIPS2_OFF_BASE_ADDR + 0x30000)
#ifdef CONFIG_MX6SL
#define RNGB_IPS_BASE_ADDR          (AIPS2_OFF_BASE_ADDR + 0x34000)
#else
#define MMDC_P1_BASE_ADDR           (AIPS2_OFF_BASE_ADDR + 0x34000)
#endif
#define WEIM_BASE_ADDR              (AIPS2_OFF_BASE_ADDR + 0x38000)
#define OCOTP_BASE_ADDR             (AIPS2_OFF_BASE_ADDR + 0x3C000)
#define CSU_BASE_ADDR               (AIPS2_OFF_BASE_ADDR + 0x40000)
#define IP2APB_PERFMON1_BASE_ADDR   (AIPS2_OFF_BASE_ADDR + 0x44000)
#define IP2APB_PERFMON2_BASE_ADDR   (AIPS2_OFF_BASE_ADDR + 0x48000)
#define IP2APB_PERFMON3_BASE_ADDR   (AIPS2_OFF_BASE_ADDR + 0x4C000)
#define IP2APB_TZASC1_BASE_ADDR     (AIPS2_OFF_BASE_ADDR + 0x50000)
#define IP2APB_TZASC2_BASE_ADDR     (AIPS2_OFF_BASE_ADDR + 0x54000)
#define AUDMUX_BASE_ADDR            (AIPS2_OFF_BASE_ADDR + 0x58000)
#define MIPI_CSI2_BASE_ADDR         (AIPS2_OFF_BASE_ADDR + 0x5C000)
#define MIPI_DSI_BASE_ADDR          (AIPS2_OFF_BASE_ADDR + 0x60000)
#define VDOA_BASE_ADDR              (AIPS2_OFF_BASE_ADDR + 0x64000)
#define UART2_BASE_ADDR             (AIPS2_OFF_BASE_ADDR + 0x68000)
#define UART3_BASE_ADDR             (AIPS2_OFF_BASE_ADDR + 0x6C000)
#define UART4_BASE_ADDR             (AIPS2_OFF_BASE_ADDR + 0x70000)
#define UART5_BASE_ADDR             (AIPS2_OFF_BASE_ADDR + 0x74000)
#define IP2APB_USBPHY1_BASE_ADDR    (AIPS2_OFF_BASE_ADDR + 0x78000)
#define IP2APB_USBPHY2_BASE_ADDR    (AIPS2_OFF_BASE_ADDR + 0x7C000)

/* Cortex-A9 MPCore private memory region */
#define ARM_PERIPHBASE              0x00A00000
#define SCU_BASE_ADDR               (ARM_PERIPHBASE)
#define IC_INTERFACES_BASE_ADDR     (ARM_PERIPHBASE + 0x0100)
#define GLOBAL_TIMER_BASE_ADDR      (ARM_PERIPHBASE + 0x0200)
#define PRIVATE_TIMERS_WD_BASE_ADDR (ARM_PERIPHBASE + 0x0600)
#define IC_DISTRIBUTOR_BASE_ADDR    (ARM_PERIPHBASE + 0x1000)

/*!
 * Defines for modules using static and dynamic DMA channels
 */
#define MXC_DMA_CHANNEL_IRAM         30
#define MXC_DMA_CHANNEL_SPDIF_TX        MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_SPDIF_RX        MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_UART1_RX	MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_UART1_TX	MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_UART2_RX	MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_UART2_TX	MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_UART3_RX	MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_UART3_TX	MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_UART4_RX	MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_UART4_TX	MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_UART5_RX	MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_UART5_TX	MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_MMC1		MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_MMC2		MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_SSI1_RX		MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_SSI1_TX		MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_SSI2_RX		MXC_DMA_DYNAMIC_CHANNEL
#ifdef CONFIG_DMA_REQ_IRAM
#define MXC_DMA_CHANNEL_SSI2_TX		(MXC_DMA_CHANNEL_IRAM + 1)
#else				/*CONFIG_DMA_REQ_IRAM */
#define MXC_DMA_CHANNEL_SSI2_TX		MXC_DMA_DYNAMIC_CHANNEL
#endif				/*CONFIG_DMA_REQ_IRAM */
#define MXC_DMA_CHANNEL_SSI3_RX		MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_SSI3_TX		MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_CSPI1_RX	MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_CSPI1_TX	MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_CSPI2_RX	MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_CSPI2_TX	MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_CSPI3_RX	MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_CSPI3_TX	MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_ATA_RX	MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_ATA_TX	MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_MEMORY	MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_ASRCA_RX  MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_ASRCA_TX  MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_ASRCB_RX  MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_ASRCB_TX  MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_ASRCC_RX  MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_ASRCC_TX  MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_ESAI_RX  MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_ESAI_TX  MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_ASRCA_ESAI MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_ASRCB_ESAI MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_ASRCC_ESAI MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_ASRCA_SSI1_TX0 MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_ASRCA_SSI1_TX1 MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_ASRCA_SSI2_TX0 MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_ASRCA_SSI2_TX1 MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_ASRCB_SSI1_TX0 MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_ASRCB_SSI1_TX1 MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_ASRCB_SSI2_TX0 MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_ASRCB_SSI2_TX1 MXC_DMA_DYNAMIC_CHANNEL

/* define virtual address */
#define PERIPBASE_VIRT 0xF0000000
#define AIPS1_BASE_ADDR_VIRT (PERIPBASE_VIRT + AIPS1_ARB_BASE_ADDR)
#define AIPS2_BASE_ADDR_VIRT (PERIPBASE_VIRT + AIPS2_ARB_BASE_ADDR)
#define ARM_PERIPHBASE_VIRT	 (PERIPBASE_VIRT + ARM_PERIPHBASE)
#define AIPS1_SIZE SZ_1M
#define AIPS2_SIZE SZ_1M
#define ARM_PERIPHBASE_SIZE SZ_8K

#define MX6_IO_ADDRESS(x) ((x)  - PERIPBASE_VIRT)

/*!
 * This macro defines the physical to virtual address mapping for all the
 * peripheral modules. It is used by passing in the physical address as x
 * and returning the virtual address. If the physical address is not mapped,
 * it returns 0xDEADBEEF
 */
#define IO_ADDRESS(x)   \
	(void __force __iomem *) \
	(((((x) >= (unsigned long)AIPS1_ARB_BASE_ADDR) && \
	  ((x) <= (unsigned long)AIPS2_ARB_END_ADDR)) || \
	  ((x) >= (unsigned long)ARM_PERIPHBASE && \
	  ((x) <= (unsigned long)(ARM_PERIPHBASE + ARM_PERIPHBASE)))) ? \
	   MX6_IO_ADDRESS(x) : 0xDEADBEEF)

/*
 * DMA request assignments
 */
#define DMA_REQ_VPU                      0
#define DMA_REQ_GPC	                     1
#define DMA_REQ_IPU1                     2
#define DMA_REQ_EXT_DMA_REQ_0            2
#define DMA_REQ_CSPI1_RX                 3
#define DMA_REQ_I2C3_A                   3
#define DMA_REQ_CSPI1_TX                 4
#define DMA_REQ_I2C2_A                   4
#define DMA_REQ_CSPI2_RX                 5
#define DMA_REQ_I2C1_A                   5
#define DMA_REQ_CSPI2_TX                 6
#define DMA_REQ_CSPI3_RX                 7
#define DMA_REQ_CSPI3_TX                 8
#define DMA_REQ_CSPI4_RX                 9
#define DMA_REQ_EPIT2                    9
#define DMA_REQ_CSPI4_TX                 10
#define DMA_REQ_I2C1_B                   10
#define DMA_REQ_CSPI5_RX                 11
#define DMA_REQ_CSPI5_TX                 12
#define DMA_REQ_GPT	                     13
#define DMA_REQ_SPDIF_RX                 14
#define DMA_REQ_EXT_DMA_REQ_1            14
#define DMA_REQ_SPDIF_TX                 15
#define DMA_REQ_EPIT1                    16
#define DMA_REQ_ASRC_RX1                 17
#define DMA_REQ_ASRC_RX2                 18
#define DMA_REQ_ASRC_RX3                 19
#define DMA_REQ_ASRC_TX1                 20
#define DMA_REQ_ASRC_TX2                 21
#define DMA_REQ_ASRC_TX3                 22
#define DMA_REQ_ESAI_RX                 23
#define DMA_REQ_I2C3_B                   23
#define DMA_REQ_ESAI_TX                 24
#define DMA_REQ_UART1_RX                 25
#define DMA_REQ_UART1_TX                 26
#define DMA_REQ_UART2_RX                 27
#define DMA_REQ_UART2_TX                 28
#define DMA_REQ_UART3_RX                 29
#define DMA_REQ_UART3_TX				 30
#define DMA_REQ_UART4_RX                 31
#define DMA_REQ_UART4_TX                 32
#define DMA_REQ_UART5_RX                 33
#define DMA_REQ_UART5_TX                 34
#define DMA_REQ_SSI1_RX1                 35
#define DMA_REQ_SSI1_TX1                 36
#define DMA_REQ_SSI1_RX0                 37
#define DMA_REQ_SSI1_TX0                 38
#define DMA_REQ_SSI2_RX1                 39
#define DMA_REQ_SSI2_TX1                 40
#define DMA_REQ_SSI2_RX0                 41
#define DMA_REQ_SSI2_TX0                 42
#define DMA_REQ_SSI3_RX1                 43
#define DMA_REQ_SSI3_TX1                 44
#define DMA_REQ_SSI3_RX0                 45
#define DMA_REQ_SSI3_TX0                 46
#define DMA_REQ_DTCP                     47

/*
 * Interrupt numbers
 */
#define MXC_INT_GPR                                32
#define MXC_INT_CHEETAH_CSYSPWRUPREQ               33
#define MXC_INT_SDMA                               34
#ifndef CONFIG_MX6SL
#define MXC_INT_VPU_JPG                            35
#define MXC_INT_INTERRUPT_36_NUM                   36
#define MXC_INT_IPU1_ERR                           37
#define MXC_INT_IPU1_FUNC                          38
#define MXC_INT_IPU2_ERR                           39
#define MXC_INT_IPU2_FUNC                          40
#define MXC_INT_GPU3D_IRQ                          41
#else
#define MXC_INT_MSHC                               35
#define MXC_INT_SNVS_PMIC_OFF                      36
#define MXC_INT_RNGB                               37
#define MXC_INT_SPDC                               38
#define MXC_INT_CSI                                39
#endif
#define MXC_INT_GPU2D_IRQ                          42
#define MXC_INT_OPENVG_XAQ2                        43
#define MXC_INT_VPU_IPI                            44
#define MXC_INT_APBHDMA_DMA                        45
#define MXC_INT_WEIM                               46
#define MXC_INT_RAWNAND_BCH                        47
#define MXC_INT_RAWNAND_GPMI                       48
#define MXC_INT_DTCP                               49
#define MXC_INT_VDOA                               50
#define MXC_INT_SNVS                               51
#define MXC_INT_SNVS_SEC                           52
#define MXC_INT_CSU                                53
#define MXC_INT_USDHC1                             54
#define MXC_INT_USDHC2                             55
#define MXC_INT_USDHC3                             56
#define MXC_INT_USDHC4                             57
#define MXC_INT_UART1_ANDED                        58
#define MXC_INT_UART2_ANDED                        59
#define MXC_INT_UART3_ANDED                        60
#define MXC_INT_UART4_ANDED                        61
#define MXC_INT_UART5_ANDED                        62
#define MXC_INT_ECSPI1                             63
#define MXC_INT_ECSPI2                             64
#define MXC_INT_ECSPI3                             65
#define MXC_INT_ECSPI4                             66
#define MXC_INT_ECSPI5                             67
#define MXC_INT_I2C1                               68
#define MXC_INT_I2C2                               69
#define MXC_INT_I2C3                               70
#ifdef CONFIG_MX6Q
#define MXC_INT_SATA                               71
#define MXC_INT_USBOH3_UH1                         72
#define MXC_INT_USBOH3_UH2                         73
#define MXC_INT_USBOH3_UH3                         74
#define MXC_INT_USBOH3_UOTG                        75
#else
#define MXC_INT_LCDIF                              71
#define MXC_INT_USBO2H_UH1                         72
#define MXC_INT_USBO2H_UH2                         73
#define MXC_INT_USBO2H_UH3                         74
#define MXC_INT_USBO2H_UOTG                        75
#endif
#define MXC_INT_ANATOP_UTMI0                       76
#define MXC_INT_ANATOP_UTMI1                       77
#define MXC_INT_SSI1                               78
#define MXC_INT_SSI2                               79
#define MXC_INT_SSI3                               80
#define MXC_INT_ANATOP_TEMPSNSR                    81
#define MXC_INT_ASRC                               82
#define MXC_INT_ESAI                               83
#define MXC_INT_SPDIF                              84
#define MXC_INT_MLB                                85
#define MXC_INT_ANATOP_ANA1                        86
#define MXC_INT_GPT                                87
#define MXC_INT_EPIT1                              88
#define MXC_INT_EPIT2                              89
#define MXC_INT_GPIO1_INT7_NUM                             90
#define MXC_INT_GPIO1_INT6_NUM                             91
#define MXC_INT_GPIO1_INT5_NUM                             92
#define MXC_INT_GPIO1_INT4_NUM                             93
#define MXC_INT_GPIO1_INT3_NUM                             94
#define MXC_INT_GPIO1_INT2_NUM                             95
#define MXC_INT_GPIO1_INT1_NUM                             96
#define MXC_INT_GPIO1_INT0_NUM                             97
#define MXC_INT_GPIO1_INT15_0_NUM                          98
#define MXC_INT_GPIO1_INT31_16_NUM                         99
#define MXC_INT_GPIO2_INT15_0_NUM                          100
#define MXC_INT_GPIO2_INT31_16_NUM                         101
#define MXC_INT_GPIO3_INT15_0_NUM                          102
#define MXC_INT_GPIO3_INT31_16_NUM                         103
#define MXC_INT_GPIO4_INT15_0_NUM                          104
#define MXC_INT_GPIO4_INT31_16_NUM                         105
#define MXC_INT_GPIO5_INT15_0_NUM                          106
#define MXC_INT_GPIO5_INT31_16_NUM                         107
#define MXC_INT_GPIO6_INT15_0_NUM                          108
#define MXC_INT_GPIO6_INT31_16_NUM                         109
#define MXC_INT_GPIO7_INT15_0_NUM                          110
#define MXC_INT_GPIO7_INT31_16_NUM                         111
#define MXC_INT_WDOG1                              112
#define MXC_INT_WDOG2                              113
#define MXC_INT_KPP                                114
#define MXC_INT_PWM1                               115
#define MXC_INT_PWM2                               116
#define MXC_INT_PWM3                               117
#define MXC_INT_PWM4                               118
#define MXC_INT_CCM_INT1_NUM                               119
#define MXC_INT_CCM_INT2_NUM                               120
#define MXC_INT_GPC_INT1_NUM                               121
#define MXC_INT_GPC_INT2_NUM                               122
#define MXC_INT_SRC                                123
#define MXC_INT_CHEETAH_L2                         124
#define MXC_INT_CHEETAH_PARITY                     125
#define MXC_INT_CHEETAH_PERFORM                    126
#define MXC_INT_CHEETAH_TRIGGER                    127
#define MXC_INT_SRC_CPU_WDOG                       128
#ifndef CONFIG_MX6SL
#define MXC_INT_INTERRUPT_129_NUM                  129
#define MXC_INT_INTERRUPT_130_NUM                  130
#define MXC_INT_INTERRUPT_13                       131
#define MXC_INT_CSI_INTR1                          132
#define MXC_INT_CSI_INTR2                          133
#define MXC_INT_DSI                                134
#define MXC_INT_HSI                                135
#else
#define MXC_INT_EPDC                               129
#define MXC_INT_PXP                                130
#define MXC_INT_DCP_GP                             131
#define MXC_INT_DCP_CH0                            132
#define MXC_INT_DCP_SEC                            133
#endif
#define MXC_INT_SJC                                136
#ifndef CONFIG_MX6SL
#define MXC_INT_CAAM_INT0_NUM                      137
#define MXC_INT_CAAM_INT1_NUM                      138
#define MXC_INT_INTERRUPT_139_NUM                  139
#define MXC_INT_TZASC1                             140
#define MXC_INT_TZASC2                             141
#define MXC_INT_CAN1                               142
#define MXC_INT_CAN2                               143
#define MXC_INT_PERFMON1                           144
#define MXC_INT_PERFMON2                           145
#define MXC_INT_PERFMON3                           146
#define MXC_INT_HDMI_TX                            147
#define MXC_INT_HDMI_TX_WAKEUP                     148
#define MXC_INT_MLB_AHB0                           149
#define MXC_INT_ENET1                              150
#define MXC_INT_ENET2                              151
#define MXC_INT_PCIE_0                             152
#define MXC_INT_PCIE_1                             153
#define MXC_INT_PCIE_2                             154
#define MXC_INT_PCIE_3                             155
#define MXC_INT_DCIC1                              156
#define MXC_INT_DCIC2                              157
#define MXC_INT_MLB_AHB1                           158
#else
#define MXC_INT_TZASC1                             140
#define MXC_INT_FEC                                146
#endif
#define MXC_INT_ANATOP_ANA2                        159

/* gpio and gpio based interrupt handling */
#define GPIO_DR                 0x00
#define GPIO_GDIR               0x04
#define GPIO_PSR                0x08
#define GPIO_ICR1               0x0C
#define GPIO_ICR2               0x10
#define GPIO_IMR                0x14
#define GPIO_ISR                0x18
#define GPIO_INT_LOW_LEV        0x0
#define GPIO_INT_HIGH_LEV       0x1
#define GPIO_INT_RISE_EDGE      0x2
#define GPIO_INT_FALL_EDGE      0x3
#define GPIO_INT_NONE           0x4

#define CLKCTL_CCR              0x00
#define	CLKCTL_CCDR             0x04
#define CLKCTL_CSR              0x08
#define CLKCTL_CCSR             0x0C
#define CLKCTL_CACRR            0x10
#define CLKCTL_CBCDR            0x14
#define CLKCTL_CBCMR            0x18
#define CLKCTL_CSCMR1           0x1C
#define CLKCTL_CSCMR2           0x20
#define CLKCTL_CSCDR1           0x24
#define CLKCTL_CS1CDR           0x28
#define CLKCTL_CS2CDR           0x2C
#define CLKCTL_CDCDR            0x30
#define CLKCTL_CHSCCDR          0x34
#define CLKCTL_CSCDR2           0x38
#define CLKCTL_CSCDR3           0x3C
#define CLKCTL_CSCDR4           0x40
#define CLKCTL_CWDR             0x44
#define CLKCTL_CDHIPR           0x48
#define CLKCTL_CDCR             0x4C
#define CLKCTL_CTOR             0x50
#define CLKCTL_CLPCR            0x54
#define CLKCTL_CISR             0x58
#define CLKCTL_CIMR             0x5C
#define CLKCTL_CCOSR            0x60
#define CLKCTL_CGPR             0x64
#define CLKCTL_CCGR0            0x68
#define CLKCTL_CCGR1            0x6C
#define CLKCTL_CCGR2            0x70
#define CLKCTL_CCGR3            0x74
#define CLKCTL_CCGR4            0x78
#define CLKCTL_CCGR5            0x7C
#define CLKCTL_CCGR6            0x80
#define CLKCTL_CCGR7            0x84
#define CLKCTL_CMEOR            0x88

#define ANATOP_USB1		0x10
#define ANATOP_USB2		0x20
#define ANATOP_PLL_VIDEO	0xA0

#define CHIP_TYPE_DQ		0x63000
#define CHIP_TYPE_DL		0x61000
#define CHIP_TYPE_SOLO		0x61000
#define CHIP_TYPE_SOLOLITE	0x60000

#define CHIP_REV_1_0            0x10
#define CHIP_REV_1_2		0x12
#define CHIP_REV_1_5		0x15
#define CHIP_REV_2_0            0x20
#define CHIP_REV_2_1            0x21
#define CHIP_REV_MASK           0xff

#define BOARD_REV_1             0x000
#define BOARD_REV_2             0x100
#define BOARD_REV_3             0x200
#define BOARD_REV_4             0x300
#define BOARD_REV_5             0x400
#define BOARD_REV_MASK          0xf00

#define PLATFORM_ICGC           0x14

#define SRC_GPR9		0x40
#define SRC_GPR10		0x44

#define SNVS_LPGPR              0x68

/* Get Board ID */
#define board_is_rev(system_rev, rev) (((system_rev & 0x0F00) == rev) ? 1 : 0)
#define chip_is_type(system_rev, rev) \
	(((system_rev & 0xFF000) == rev) ? 1 : 0)

#define mx6_board_is_unknown()  board_is_rev(fsl_system_rev, BOARD_REV_1)
#define mx6_board_is_reva()	board_is_rev(fsl_system_rev, BOARD_REV_2)
#define mx6_board_is_revb()	board_is_rev(fsl_system_rev, BOARD_REV_3)
#define mx6_board_is_revc()     board_is_rev(fsl_system_rev, BOARD_REV_4)

#define mx6_chip_is_dq()	chip_is_type(fsl_system_rev, CHIP_TYPE_DQ)
#define mx6_chip_is_dl()	chip_is_type(fsl_system_rev, CHIP_TYPE_DL)
#define mx6_chip_is_solo()      chip_is_type(fsl_system_rev, CHIP_TYPE_SOLO)
#define mx6_chip_is_sololite()	chip_is_type(fsl_system_rev, CHIP_TYPE_SOLOLITE)
#define mx6_chip_rev()		(fsl_system_rev & 0xFF)

#define mx6_chip_dq_name	"i.MX6Q"
#define mx6_chip_dl_solo_name	"i.MX6DL/Solo"
#define mx6_chip_sololite_name	"i.MX6SoloLite"
#define mx6_chip_name()		(mx6_chip_is_dq() ? mx6_chip_dq_name : \
	((mx6_chip_is_dl() | mx6_chip_is_solo()) ? mx6_chip_dl_solo_name : \
	(mx6_chip_is_sololite() ? mx6_chip_sololite_name : "unknown-chip")))
#define mx6_board_rev_name()	(mx6_board_is_reva() ? "RevA" : \
	(mx6_board_is_revb() ? "RevB" : \
	(mx6_board_is_revc() ? "RevC" : "unknown-board")))

#ifndef __ASSEMBLER__

enum boot_device {
	WEIM_NOR_BOOT,
	ONE_NAND_BOOT,
	PATA_BOOT,
	SATA_BOOT,
	I2C_BOOT,
	SPI_NOR_BOOT,
	SD_BOOT,
	MMC_BOOT,
	NAND_BOOT,
	UNKNOWN_BOOT,
	BOOT_DEV_NUM = UNKNOWN_BOOT,
};

enum mxc_clock {
	MXC_ARM_CLK = 0,
	MXC_PER_CLK,
	MXC_AHB_CLK,
	MXC_IPG_CLK,
	MXC_IPG_PERCLK,
	MXC_UART_CLK,
	MXC_CSPI_CLK,
	MXC_AXI_CLK,
	MXC_EMI_SLOW_CLK,
	MXC_DDR_CLK,
	MXC_ESDHC_CLK,
	MXC_ESDHC2_CLK,
	MXC_ESDHC3_CLK,
	MXC_ESDHC4_CLK,
	MXC_SATA_CLK,
	MXC_NFC_CLK,
	MXC_GPMI_CLK,
	MXC_BCH_CLK,
};

enum mxc_peri_clocks {
	MXC_UART1_BAUD,
	MXC_UART2_BAUD,
	MXC_UART3_BAUD,
	MXC_SSI1_BAUD,
	MXC_SSI2_BAUD,
	MXC_CSI_BAUD,
	MXC_MSTICK1_CLK,
	MXC_MSTICK2_CLK,
	MXC_SPI1_CLK,
	MXC_SPI2_CLK,
};

extern unsigned int fsl_system_rev;
extern unsigned int mxc_get_clock(enum mxc_clock clk);
extern unsigned int get_board_rev(void);
extern int is_soc_rev(int rev);
extern enum boot_device get_boot_device(void);
extern void fsl_set_system_rev(void);
extern int cpu_is_mx6q(void);

#endif /* __ASSEMBLER__*/

#endif /*  __ASM_ARCH_MXC_MX6_H__ */
