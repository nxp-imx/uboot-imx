/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __ARCH_IMX8M_REGS_H
#define __ARCH_IMX8M_REGS_H

#include <asm/imx-common/regs-lcdif.h>

/* Based on version 0.2.2 */

#define M4_BOOTROM_BASE_ADDR	0x007E0000

#define SAI1_BASE_ADDR		0x30010000
#define SAI2_BASE_ADDR		0x30020000
#define SAI3_BASE_ADDR		0x30030000
#define SAI5_BASE_ADDR		0x30050000
#define SAI6_BASE_ADDR		0x30060000
#define MICFIL_BASE_ADDR	0x30080000
#define SPDIF1_BASE_ADDR	0x30090000
#define SPDIF2_BASE_ADDR	0x300A0000
#define SPBA2_BASE_ADDR		0x300F0000
#define AIPS1_BASE_ADDR		0x301F0000
#define GPIO1_BASE_ADDR		0x30200000
#define GPIO2_BASE_ADDR		0x30210000
#define GPIO3_BASE_ADDR		0x30220000
#define GPIO4_BASE_ADDR		0x30230000
#define GPIO5_BASE_ADDR		0x30240000
#define ANA_TSENSOR_ADDR	0x30260000
#define ANA_OSC_BASE_ADDR	0x30270000
#define WDOG1_BASE_ADDR		0x30280000
#define WDOG2_BASE_ADDR		0x30290000
#define WDOG3_BASE_ADDR		0x302A0000
#define SDMA3_BASE_ADDR		0x302B0000
#define SDMA2_BASE_ADDR		0x302C0000
#define GPT1_BASE_ADDR		0x302D0000
#define GPT2_BASE_ADDR		0x302E0000
#define GPT3_BASE_ADDR		0x302F0000
#define ROMCP_BASE_ADDR		0x30310000
#define IOMUXC_BASE_ADDR	0x30330000
#define IOMUXC_GPR_BASE_ADDR	0x30340000
#define OCOTP_BASE_ADDR		0x30350000
#define ANA_PLL_BASE_ADDR	0x30360000
#define SNVS_HP_BASE_ADDR	0x30370000
#define CCM_BASE_ADDR		0x30380000
#define SRC_BASE_ADDR		0x30390000
#define GPC_BASE_ADDR		0x303A0000
#define SEMA1_BASE_ADDR		0x303B0000
#define SEMA2_BASE_ADDR		0x303C0000
#define RDC_BASE_ADDR		0x303D0000
#define CSU_BASE_ADDR		0x303E0000

#define AIPS2_BASE_ADDR		0x305E0000
#define PWM1_BASE_ADDR		0x30650000
#define PWM2_BASE_ADDR		0x30660000
#define PWM3_BASE_ADDR		0x30670000
#define PWM4_BASE_ADDR		0x30680000
#define SCTR_BASE_ADDR		0x306C0000
#define GPT6_BASE_ADDR		0x306D0000
#define GPT5_BASE_ADDR		0x306E0000
#define GPT4_BASE_ADDR		0x306F0000
#define PERFMON1_ADDR		0x307B0000
#define PERFMON2_ADDR		0x307C0000
#define QOSC_BASE_ADDR		0x307E0000

#define ECSPI1_BASE_ADDR	0x30820000
#define ECSPI2_BASE_ADDR	0x30830000
#define ECSPI3_BASE_ADDR	0x30840000
#define UART1_BASE_ADDR		0x30860000
#define UART3_BASE_ADDR		0x30880000
#define UART2_BASE_ADDR		0x30890000
#define SPBA_BASE_ADDR		0x308F0000
#define CAAM_BASE_ADDR		0x30900000
#define AIPS3_BASE_ADDR		0x309F0000
#define I2C1_BASE_ADDR		0x30A20000
#define I2C2_BASE_ADDR		0x30A30000
#define I2C3_BASE_ADDR		0x30A40000
#define I2C4_BASE_ADDR		0x30A50000
#define UART4_BASE_ADDR		0x30A60000
#define MUA_BASE_ADDR		0x30AA0000
#define MUB_BASE_ADDR		0x30AB0000
#define SEMAHS_BASE_ADDR	0x30AC0000
#define USDHC1_BASE_ADDR	0x30B40000
#define USDHC2_BASE_ADDR	0x30B50000
#define USDHC3_BASE_ADDR	0x30B60000
#define QSPI_BASE_ADDR		0x30BB0000
#define QSPI0_BASE_ADDR		0x30BB0000
#define QSPI0_AMBA_BASE		0x08000000
#define SDMA1_BASE_ADDR		0x30BD0000
#define ENET1_BASE_ADDR		0x30BE0000

#define AIPS4_BASE_ADDR		0x32DF0000
#define LCDIF_BASE_ADDR		0x32E00000
#define MIPI_DSI_BASE_ADDR	0x32E10000
#define CSI_BASE_ADDR		0x32E20000
#define MIPI_CSI_BASE_ADDR	0x32E30000
#define USB1_BASE_ADDR		0x32E40000
#define USB2_BASE_ADDR		0x32E50000
#define PCIE_PHY1_BASE_ADDR	0x32F00000
#define TZASC_BASE_ADDR		0x32F80000
#define PLAT_CTRL_BASE_ADDR	0x32FE0000

#define MXS_APBH_BASE		0x33000000
#define MXS_GPMI_BASE		0x33002000
#define MXS_BCH_BASE		0x33004000

#define USB_BASE_ADDR		USB1_BASE_ADDR

#define MXS_LCDIF_BASE		LCDIF_BASE_ADDR

#define IOMUXC_GPR0		(IOMUXC_GPR_BASE_ADDR + 0x00)
#define IOMUXC_GPR1		(IOMUXC_GPR_BASE_ADDR + 0x04)
#define IOMUXC_GPR2		(IOMUXC_GPR_BASE_ADDR + 0x08)
#define IOMUXC_GPR3		(IOMUXC_GPR_BASE_ADDR + 0x0c)
#define IOMUXC_GPR4		(IOMUXC_GPR_BASE_ADDR + 0x10)
#define IOMUXC_GPR5		(IOMUXC_GPR_BASE_ADDR + 0x14)
#define IOMUXC_GPR6		(IOMUXC_GPR_BASE_ADDR + 0x18)
#define IOMUXC_GPR7		(IOMUXC_GPR_BASE_ADDR + 0x1c)
#define IOMUXC_GPR8		(IOMUXC_GPR_BASE_ADDR + 0x20)
#define IOMUXC_GPR9		(IOMUXC_GPR_BASE_ADDR + 0x24)
#define IOMUXC_GPR10		(IOMUXC_GPR_BASE_ADDR + 0x28)
#define IOMUXC_GPR11		(IOMUXC_GPR_BASE_ADDR + 0x2C)
#define IOMUXC_GPR22		(IOMUXC_GPR_BASE_ADDR + 0x58)

#define GPR_TZASC_EN		(1 << 0)
#define GPR_TZASC_SWAP_ID	(1 << 1)
#define GPR_TZASC_EN_LOCK	(1 << 16)

#define CNTCR_OFF	0x00
#define CNTFID0_OFF	0x20
#define CNTFID1_OFF	0x24

#define SC_CNTCR_ENABLE		(1 << 0)
#define SC_CNTCR_HDBG		(1 << 1)
#define SC_CNTCR_FREQ0		(1 << 8)
#define SC_CNTCR_FREQ1		(1 << 9)

#define IMX_CSPI1_BASE		0x30820000
#define IMX_CSPI2_BASE		0x30830000
#define IMX_CSPI3_BASE		0x30840000

#define MXC_SPI_BASE_ADDRESSES \
	IMX_CSPI1_BASE, \
	IMX_CSPI2_BASE, \
	IMX_CSPI3_BASE

#define SRC_IPS_BASE_ADDR	0x30390000
#define SRC_DDRC_RCR_ADDR	0x30391000
#define SRC_DDRC2_RCR_ADDR	0x30391004

#define DDR_CSD1_BASE_ADDR	0x40000000

#define CAAM_ARB_BASE_ADDR              (0x00100000)
#define CAAM_ARB_END_ADDR               (0x00107FFF)
#define CAAM_IPS_BASE_ADDR              (0x30900000)
#define CONFIG_SYS_FSL_SEC_OFFSET       (0)
#define CONFIG_SYS_FSL_SEC_ADDR         (CAAM_IPS_BASE_ADDR + \
					 CONFIG_SYS_FSL_SEC_OFFSET)
#define CONFIG_SYS_FSL_JR0_OFFSET       (0x1000)
#define CONFIG_SYS_FSL_JR0_ADDR         (CONFIG_SYS_FSL_SEC_ADDR + \
					 CONFIG_SYS_FSL_JR0_OFFSET)
#define CONFIG_SYS_FSL_MAX_NUM_OF_SEC   1

#if !(defined(__KERNEL_STRICT_NAMES) || defined(__ASSEMBLY__))
#include <asm/types.h>
struct ocotp_regs {
	u32	ctrl;
	u32	ctrl_set;
	u32     ctrl_clr;
	u32	ctrl_tog;
	u32	timing;
	u32     rsvd0[3];
	u32     data;
	u32     rsvd1[3];
	u32     read_ctrl;
	u32     rsvd2[3];
	u32	read_fuse_data;
	u32     rsvd3[3];
	u32	sw_sticky;
	u32     rsvd4[3];
	u32     scs;
	u32     scs_set;
	u32     scs_clr;
	u32     scs_tog;
	u32     crc_addr;
	u32     rsvd5[3];
	u32     crc_value;
	u32     rsvd6[3];
	u32     version;
	u32     rsvd7[0xdb];

	/* fuse banks */
	struct fuse_bank {
		u32	fuse_regs[0x10];
	} bank[0];
};

struct fuse_bank0_regs {
	u32 lock;
	u32 rsvd0[3];
	u32 uid_low;
	u32 rsvd1[3];
	u32 uid_high;
	u32 rsvd2[7];
};

struct fuse_bank1_regs {
	u32 tester3;
	u32 rsvd0[3];
	u32 tester4;
	u32 rsvd1[3];
	u32 tester5;
	u32 rsvd2[3];
	u32 cfg0;
	u32 rsvd3[3];
};

struct fuse_bank9_regs {
	u32 mac_addr0;
	u32 rsvd0[3];
	u32 mac_addr1;
	u32 rsvd1[11];
};

/* System Reset Controller (SRC) */
struct src {
	u32 scr;
	u32 a53rcr;
	u32 a53rcr1;
	u32 m4rcr;
	u32 reserved1[4];
	u32 usbophy1_rcr;
	u32 usbophy2_rcr;
	u32 mipiphy_rcr;
	u32 pciephy_rcr;
	u32 reserved2;
	u32 disp_rcr;
	u32 reserved3[2];
	u32 gpu_rcr;
	u32 vpu_rcr;
	u32 reserved4[4];
	u32 sbmr1;
	u32 srsr;
	u32 reserved5[2];
	u32 sisr;
	u32 simr;
	u32 sbmr2;
	u32 gpr1;
	u32 gpr2;
	u32 gpr3;
	u32 gpr4;
	u32 gpr5;
	u32 gpr6;
	u32 gpr7;
	u32 gpr8;
	u32 gpr9;
	u32 gpr10;
	u32 reserved6[985];
	u32 ddr1_rcr;
};

struct wdog_regs {
	u16	wcr;	/* Control */
	u16	wsr;	/* Service */
	u16	wrsr;	/* Reset Status */
	u16	wicr;	/* Interrupt Control */
	u16	wmcr;	/* Miscellaneous Control */
};

/* Boot device type */
#define BOOT_TYPE_SD		0x1
#define BOOT_TYPE_MMC		0x2
#define BOOT_TYPE_NAND		0x3
#define BOOT_TYPE_QSPI		0x4
#define BOOT_TYPE_WEIM		0x5
#define BOOT_TYPE_SPINOR	0x6
#define BOOT_TYPE_USB		0xF

#define ROM_SW_INFO_ADDR_A0	0x000009e8
#define ROM_SW_INFO_ADDR	0x000009e8

struct bootrom_sw_info {
	u8 reserved_1;
	u8 boot_dev_instance;
	u8 boot_dev_type;
	u8 reserved_2;
	u32 core_freq;
	u32 axi_freq;
	u32 ddr_freq;
	u32 tick_freq;
	u32 reserved_3[3];
};

/* ECSPI registers */
struct cspi_regs {
	u32 rxdata;
	u32 txdata;
	u32 ctrl;
	u32 cfg;
	u32 intr;
	u32 dma;
	u32 stat;
	u32 period;
};

/*
 * CSPI register definitions
 */
#define MXC_ECSPI
#define MXC_CSPICTRL_EN		(1 << 0)
#define MXC_CSPICTRL_MODE	(1 << 1)
#define MXC_CSPICTRL_XCH	(1 << 2)
#define MXC_CSPICTRL_MODE_MASK (0xf << 4)
#define MXC_CSPICTRL_CHIPSELECT(x)	(((x) & 0x3) << 12)
#define MXC_CSPICTRL_BITCOUNT(x)	(((x) & 0xfff) << 20)
#define MXC_CSPICTRL_PREDIV(x)	(((x) & 0xF) << 12)
#define MXC_CSPICTRL_POSTDIV(x)	(((x) & 0xF) << 8)
#define MXC_CSPICTRL_SELCHAN(x)	(((x) & 0x3) << 18)
#define MXC_CSPICTRL_MAXBITS	0xfff
#define MXC_CSPICTRL_TC		(1 << 7)
#define MXC_CSPICTRL_RXOVF	(1 << 6)
#define MXC_CSPIPERIOD_32KHZ	(1 << 15)
#define MAX_SPI_BYTES	32

/* Bit position inside CTRL register to be associated with SS */
#define MXC_CSPICTRL_CHAN	18

/* Bit position inside CON register to be associated with SS */
#define MXC_CSPICON_PHA		0  /* SCLK phase control */
#define MXC_CSPICON_POL		4  /* SCLK polarity */
#define MXC_CSPICON_SSPOL	12 /* SS polarity */
#define MXC_CSPICON_CTL		20 /* inactive state of SCLK */

#include <stdbool.h>
bool is_usb_boot(void);
#define is_boot_from_usb  is_usb_boot
#define disconnect_from_pc(void) clrbits_le32(USB1_BASE_ADDR + 0xc704, (1 << 31));

#endif
#endif
