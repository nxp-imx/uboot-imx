// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2021 NXP
 */

#include <log.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <asm/arch/sys_proto.h>

#include "upower_soc_defs.h"
#include "upower_api.h"
#include "upower_defs.h"

#define UPOWER_AP_MU1_ADDR	0x29280000

#define PS_RTD		BIT(0)
#define PS_DSP		BIT(1)
#define PS_A35_0	BIT(2)
#define PS_A35_1	BIT(3)
#define PS_L2		BIT(4)
#define PS_FAST_NIC	BIT(5)
#define PS_APD_PERIPH	BIT(6)
#define PS_GPU3D	BIT(7)
#define PS_HIFI4	BIT(8)
#define PS_DDR		GENMASK(12, 9)
#define PS_PXP_EPDC	BIT(13)
#define PS_MIPI_DSI	BIT(14)
#define PS_MIPI_CSI	BIT(15)
#define PS_NIC_LPAV	BIT(16)
#define PS_FUSION_AO	BIT(17)
#define PS_FUSE		BIT(18)
#define PS_UPOWER	BIT(19)

static struct MU_tag *muptr = (struct MU_tag *)UPOWER_AP_MU1_ADDR;

extern void upwr_txrx_isr(void);

void upower_apd_inst_isr(upwr_isr_callb txrx_isr, upwr_isr_callb excp_isr)
{
    printf("%s: entry\n", __func__);
}

void upower_wait_resp(void)
{
    while(muptr->RSR.B.RF0 == 0) {
        debug("%s: poll the mu:%x\n", __func__, muptr->RSR.R);
        udelay(100);
    }

    upwr_txrx_isr();
}

void usr_upwr_callb(upwr_sg_t sg, uint32_t func, upwr_resp_t errcode, int ret)
{

}

u32 upower_status(int status)
{
    u32 ret = -1;
    switch(status) {
        case 0:
            debug("%s: finished successfully!\n", __func__);
            ret = 0;
            break;
        case -1:
            printf("%s: memory allocation or resource failed!\n", __func__);
            break;
        case -2:
            printf("%s: invalid argument!\n", __func__);
            break;
        case -3:
            printf("%s: called in an invalid API state!\n", __func__);
            break;
        default:
            printf("%s: invalid return status\n", __func__);
            break;
    }
    return ret;
}

void user_upwr_rdy_callb(uint32_t soc, uint32_t vmajor, uint32_t vminor)
{
	printf("%s: soc=%x\n", __func__, soc);
	printf("%s: RAM version:%d.%d\n", __func__, vmajor, vminor);
}

int upower_pmic_i2c_write(u32 reg_addr, u32 reg_val)
{
	int ret, ret_val;
	upwr_resp_t err_code;

	ret = upwr_xcp_i2c_access(0x32, 1, 1, reg_addr, reg_val, NULL);
	if (ret) {
		printf("pmic i2c read failed ret %d\n", ret);
		return ret;
	}

	upower_wait_resp();
	ret = upwr_poll_req_status(UPWR_SG_EXCEPT, NULL, &err_code, &ret_val, 1000);
	if (ret != UPWR_REQ_OK) {
		printk("i2c poll Faliure %d, err_code %d, ret_val 0x%x\n", ret, err_code, ret_val);
		return ret;
	}

	debug("PMIC write reg[0x%x], val[0x%x]\n", reg_addr, reg_val);

	return 0;
}

int upower_pmic_i2c_read(u32 reg_addr, u32 *reg_val)
{
	int ret, ret_val;
	upwr_resp_t err_code;

	if (!reg_val)
		return -1;

	ret = upwr_xcp_i2c_access(0x32, -1, 1, reg_addr, 0, NULL);
	if (ret) {
		printf("pmic i2c read failed ret %d\n", ret);
		return ret;
	}

	upower_wait_resp();
	ret = upwr_poll_req_status(UPWR_SG_EXCEPT, NULL, &err_code, &ret_val, 1000);
	if (ret != UPWR_REQ_OK) {
		printk("i2c poll Faliure %d, err_code %d, ret_val 0x%x\n", ret, err_code, ret_val);
		return ret;
	}

	*reg_val = ret_val;

	debug("PMIC read reg[0x%x], val[0x%x]\n", reg_addr, *reg_val);

	return 0;
}

int upower_init(void)
{
	u32 fw_major, fw_minor, fw_vfixes;
	u32 soc_id;
	int status;
	upwr_resp_t err_code;

	uint32_t swton;
	uint64_t memon;
	int ret, ret_val;

	struct upwr_dom_bias_cfg_t bias;

	do {
		status = upwr_init(1, muptr, NULL, NULL, upower_apd_inst_isr, NULL);
		if (upower_status(status)) {
			printf("%s: upower init failure\n", __func__);
			break;
		}

		soc_id = upwr_rom_version(&fw_major, &fw_minor, &fw_vfixes);
		if (soc_id == 0) {
			printf("%s:, soc_id not initialized\n", __func__);
			break;
		} else {
			printf("%s: soc_id=%d\n", __func__, soc_id);
			printf("%s: version:%d.%d.%d\n", __func__, fw_major, fw_minor, fw_vfixes);
		}

		printf("%s: start uPower RAM service\n", __func__);
		status = upwr_start(1, user_upwr_rdy_callb);
		upower_wait_resp();
		if (upower_status(status)) {
			printf("%s: upower init failure\n", __func__);
			break;
		}
	} while(0);

	swton = PS_UPOWER | PS_FUSE | PS_FUSION_AO | PS_NIC_LPAV | PS_PXP_EPDC | PS_DDR |
		PS_HIFI4 | PS_GPU3D | PS_MIPI_DSI;
	ret = upwr_pwm_power_on(&swton, NULL /* no memories */, NULL /* no callback */);
	if (ret)
		printf("Turn on switches fail %d\n", ret);
	else
		printf("Turning on switches...\n");

	upower_wait_resp();
	ret = upwr_poll_req_status(UPWR_SG_PWRMGMT, NULL, &err_code, &ret_val, 1000);
	if (ret != UPWR_REQ_OK)
		printf("Turn on switches faliure %d, err_code %d, ret_val 0x%x\n", ret, err_code, ret_val);
	else
		printf("Turn on switches ok\n");

	/*
	 * Ascending Order -> bit [0:54)
	 * CA35 Core 0 L1 cache
	 * CA35 Core 1 L1 cache
	 * L2 Cache 0
	 * L2 Cache 1
	 * L2 Cache victim/tag
	 * CAAM Secure RAM
	 * DMA1 RAM
	 * FlexSPI2 FIFO, Buffer
	 * SRAM0
	 * AD ROM
	 * USB0 TX/RX RAM
	 * uSDHC0 FIFO RAM
	 * uSDHC1 FIFO RAM
	 * uSDHC2 FIFO and USB1 TX/RX RAM
	 * GIC RAM
	 * ENET TX FIXO
	 * Reserved(Brainshift)
	 * DCNano Tile2Linear and RGB Correction
	 * DCNano Cursor and FIFO
	 * EPDC LUT
	 * EPDC FIFO
	 * DMA2 RAM
	 * GPU2D RAM Group 1
	 * GPU2D RAM Group 2
	 * GPU3D RAM Group 1
	 * GPU3D RAM Group 2
	 * HIFI4 Caches, IRAM, DRAM
	 * ISI Buffers
	 * MIPI-CSI FIFO
	 * MIPI-DSI FIFO
	 * PXP Caches, Buffers
	 * SRAM1
	 * Casper RAM
	 * DMA0 RAM
	 * FlexCAN RAM
	 * FlexSPI0 FIFO, Buffer
	 * FlexSPI1 FIFO, Buffer
	 * CM33 Cache
	 * PowerQuad RAM
	 * ETF RAM
	 * Sentinel PKC, Data RAM1, Inst RAM0/1
	 * Sentinel ROM
	 * uPower IRAM/DRAM
	 * uPower ROM
	 * CM33 ROM
	 * SSRAM Partition 0
	 * SSRAM Partition 1
	 * SSRAM Partition 2,3,4
	 * SSRAM Partition 5
	 * SSRAM Partition 6
	 * SSRAM Partition 7_a(128KB)
	 * SSRAM Partition 7_b(64KB)
	 * SSRAM Partition 7_c(64KB)
	 * Sentinel Data RAM0, Inst RAM2
	 */
	/* MIPI-CSI FIFO BIT28 not set */
	memon = 0x3FFFFFEFFFFFFCUL;
	ret = upwr_pwm_power_on(NULL, (const uint32_t *)&memon /* no memories */, NULL /* no callback */);
	if (ret)
		printf("Turn on memories fail %d\n", ret);
	else
		printf("Turning on memories...\n");

	upower_wait_resp();
	ret = upwr_poll_req_status(UPWR_SG_PWRMGMT, NULL, &err_code, &ret_val, 1000);
	if (ret != UPWR_REQ_OK)
		printf("Turn on memories faliure %d, err_code %d, ret_val 0x%x\n", ret, err_code, ret_val);
	else
		printf("Turn on memories ok\n");

	mdelay(1);

	ret = upwr_xcp_set_ddr_retention(APD_DOMAIN, 0, NULL);
	if (ret)
		printf("Clear DDR retention fail %d\n", ret);
	else
		printf("Clearing DDR retention...\n");

	upower_wait_resp();
	ret = upwr_poll_req_status(UPWR_SG_EXCEPT, NULL, &err_code, &ret_val, 1000);
	if (ret != UPWR_REQ_OK)
		printf("Clear DDR retention fail %d, err_code %d, ret_val 0x%x\n", ret, err_code, ret_val);
	else
		printf("Clear DDR retention ok\n");

	if (is_soc_rev(CHIP_REV_1_0)) {
		/* Enable AFBB for AP domain */
		bias.apply = BIAS_APPLY_APD;
		bias.dommode = AFBB_BIAS_MODE;
		ret = upwr_pwm_chng_dom_bias(&bias, NULL);

		if (ret)
			printf("Enable AFBB for APD bias fail %d\n", ret);
		else
			printf("Enabling AFBB for APD bias...\n");

		upower_wait_resp();
		ret = upwr_poll_req_status(UPWR_SG_PWRMGMT, NULL, &err_code, &ret_val, 1000);
		if (ret != UPWR_REQ_OK)
			printf("Enable AFBB fail %d, err_code %d, ret_val 0x%x\n", ret, err_code, ret_val);
		else
			printf("Enable AFBB for APD bias ok\n");

	}

	return 0;
}
