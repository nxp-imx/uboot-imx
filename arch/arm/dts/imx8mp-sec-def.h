/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef IMX8MP_SEC_DEF_H
#define IMX8MP_SEC_DEF_H

/* Domain ID */
#define DID0            0x0
#define DID1            0x1
#define DID2            0x2
#define DID3            0x3

/* Domain RD/WR permission */
#define LOCK           0x80000000
#define ENA            0x40000000
#define D3R            0x00000080
#define D3W            0x00000040
#define D2R            0x00000020
#define D2W            0x00000010
#define D1R            0x00000008
#define D1W            0x00000004
#define D0R            0x00000002
#define D0W            0x00000001

#define PDAP_D1_ACCESS 0x0000000C /* D1W|D1R */
#define PDAP_D0D1_ACCESS 0x0000000F /* D0R|D0W|D1W|D1R */
#define MEM_D1_ACCESS  0x4000000C /* ENA|D1W|D1R */
#define MEM_D0D1_ACCESS  0x4000000F /* ENA|D0W|D0R|D1W|D1R */

/* RDC type */
#define RDC_INVALID 0
#define RDC_MDA 1
#define RDC_PDAP 2
#define RDC_MEM_REGION 3

/* RDC MDA index */
#define RDC_MDA_A53  0
#define RDC_MDA_M7  1
#define RDC_MDA_SDMA3p  3
#define RDC_MDA_SDMA3b  4
#define RDC_MDA_LCDIF1  5
#define RDC_MDA_ISI  6
#define RDC_MDdA_NPU = 7
#define RDC_MDA_Coresight  8
#define RDC_MDA_DAP  9
#define RDC_MDA_CAAM  10
#define RDC_MDA_SDMA1p  11
#define RDC_MDA_SDMA1b  12
#define RDC_MDA_APBHDMA  13
#define RDC_MDA_RAWNAND  14
#define RDC_MDA_uSDHC1  15
#define RDC_MDA_uSDHC2  16
#define RDC_MDA_uSDHC3  17
#define RDC_MDA_ENET1_TX 22
#define RDC_MDA_ENET1_RX 23
#define RDC_MDA_SDMA3_SPBA2 25
#define RDC_MDA_LCDIF2  27
#define RDC_MDA_HDMI_TX  28
#define RDC_MDA_GPU3D  30
#define RDC_MDA_GPU2D  31
#define RDC_MDA_VPUG1  32
#define RDC_MDA_VPUG2  33
#define RDC_MDA_VC8000E  34

/* RDC Peripherals index */
#define RDC_PDAP_GPIO1  0
#define RDC_PDAP_GPIO2  1
#define RDC_PDAP_GPIO3  2
#define RDC_PDAP_GPIO4  3
#define RDC_PDAP_GPIO5  4
#define RDC_PDAP_ANA_TSENSOR  6
#define RDC_PDAP_ANA_OSC  7
#define RDC_PDAP_WDOG1  8
#define RDC_PDAP_WDOG2  9
#define RDC_PDAP_WDOG3  10
#define RDC_PDAP_SDMA2  12
#define RDC_PDAP_GPT1  13
#define RDC_PDAP_GPT2  14
#define RDC_PDAP_GPT3  15
#define RDC_PDAP_ROMCP  17
#define RDC_PDAP_IOMUXC  19
#define RDC_PDAP_IOMUXC_GPR  20
#define RDC_PDAP_OCOTP_CTRL  21
#define RDC_PDAP_ANA_PLL  22
#define RDC_PDAP_SNVS_HP  23
#define RDC_PDAP_CCM  24
#define RDC_PDAP_SRC  25
#define RDC_PDAP_GPC  26
#define RDC_PDAP_SEMAPHORE1  27
#define RDC_PDAP_SEMAPHORE2  28
#define RDC_PDAP_RDC  29
#define RDC_PDAP_CSU  30
#define RDC_PDAP_LCDIF  32
#define RDC_PDAP_MIPI_DSI  33
#define RDC_PDAP_ISI  34
#define RDC_PDAP_MIPI_CSI  35
#define RDC_PDAP_USB1  36
#define RDC_PDAP_PWM1  38
#define RDC_PDAP_PWM2  39
#define RDC_PDAP_PWM3  40
#define RDC_PDAP_PWM4  41
#define RDC_PDAP_System_Counter_RD  42
#define RDC_PDAP_System_Counter_CMP  43
#define RDC_PDAP_System_Counter_CTRL  44
#define RDC_PDAP_GPT6  46
#define RDC_PDAP_GPT5  47
#define RDC_PDAP_GPT4  48
#define RDC_PDAP_TZASC  56
#define RDC_PDAP_PERFMON1  60
#define RDC_PDAP_PERFMON2  61
#define RDC_PDAP_PLATFORM_CTRL  62
#define RDC_PDAP_QoSC  63
#define RDC_PDAP_I2C1  66
#define RDC_PDAP_I2C2  67
#define RDC_PDAP_I2C3  68
#define RDC_PDAP_I2C4  69
#define RDC_PDAP_UART4  70
#define RDC_PDAP_MU_A  74
#define RDC_PDAP_MU_B  75
#define RDC_PDAP_SEMAPHORE_HS  76
#define RDC_PDAP_SAI2  79
#define RDC_PDAP_SAI3  80
#define RDC_PDAP_SAI5  82
#define RDC_PDAP_SAI6  83
#define RDC_PDAP_uSDHC1  84
#define RDC_PDAP_uSDHC2  85
#define RDC_PDAP_uSDHC3  86
#define RDC_PDAP_SAI7  87
#define RDC_PDAP_SPBA2  90
#define RDC_PDAP_QSPI  91
#define RDC_PDAP_SDMA1  93
#define RDC_PDAP_ENET1  94
#define RDC_PDAP_SPDIF1  97
#define RDC_PDAP_eCSPI1  98
#define RDC_PDAP_eCSPI2  99
#define RDC_PDAP_eCSPI3  100
#define RDC_PDAP_MICFIL  101
#define RDC_PDAP_UART1  102
#define RDC_PDAP_UART3  104
#define RDC_PDAP_UART2  105
#define RDC_PDAP_ASRC  107
#define RDC_PDAP_SDMA3  109
#define RDC_PDAP_SPBA1  111
#define RDC_PDAP_CAAM  114

/* RDC MEMORY REGION */
#define TCM_START 0x7E0000
#define TCM_END   0x820000
#define M4_DDR_START 0x20000000
#define M4_DDR_END 0x20800000

#endif /* IMX8MP_SEC_DEF_H */
