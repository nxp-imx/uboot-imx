/*
 * Copyright 2018 NXP
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <common.h>
#include <errno.h>
#include <asm/io.h>
#include <asm/arch/ddr.h>
#include <asm/arch/clock.h>
#include "lpddr4_define.h"

void dwc_ddrphy_phyinit_userCustom_E_setDfiClk(int pstate) 
{
	if(pstate==2)
		dram_pll_init(DRAM_PLL_OUT_100M);
	else if(pstate==1)
		dram_pll_init(DRAM_PLL_OUT_667M);
	else
		dram_pll_init(DRAM_PLL_OUT_750M);
}

int dwc_ddrphy_phyinit_userCustom_G_waitFwDone(void)
{
	volatile unsigned int tmp, tmp_t;
	volatile unsigned int train_ok;
	volatile unsigned int train_fail;
	volatile unsigned int stream_msg;
	int ret = 0;

	train_ok = 0;
	train_fail = 0;
	stream_msg = 0;
	while (train_ok == 0 && train_fail == 0) {
		tmp = reg32_read(IP2APB_DDRPHY_IPS_BASE_ADDR(0) + 4 * 0xd0004);
		tmp_t = tmp & 0x01;
		while (tmp_t){
		    tmp = reg32_read(IP2APB_DDRPHY_IPS_BASE_ADDR(0) + 4 * 0xd0004);
		    tmp_t = tmp & 0x01;
		}
#ifdef PRINT_PMU
		printf("get the training message\n");
#endif  
		tmp = reg32_read(IP2APB_DDRPHY_IPS_BASE_ADDR(0) + 4 * 0xd0032);
#ifdef PRINT_PMU
		printf("PMU major stream =0x%x\n",tmp);
#endif  
		if (tmp==0x08) {
			stream_msg = 1;

#ifdef DDR_PRINT_ALL_MESSAGE
			reg32_write(IP2APB_DDRPHY_IPS_BASE_ADDR(0) + 4 * 0xd0031, 0x0);

			do {
			    tmp_t = reg32_read(IP2APB_DDRPHY_IPS_BASE_ADDR(0) + 4 * 0xd0004);
			}while((tmp_t & 0x1) == 0x0);
			reg32_write(IP2APB_DDRPHY_IPS_BASE_ADDR(0) + 4 * 0xd0031, 0x1);
			
			do {
			    tmp_t = reg32_read(IP2APB_DDRPHY_IPS_BASE_ADDR(0) + 4 * 0xd0004);
			}while((tmp_t & 0x1) == 0x1);
			
			/* read_mbox_mssg */
			stream_nb_args = reg32_read(IP2APB_DDRPHY_IPS_BASE_ADDR(0) +4 * 0xd0032);
			
			/* read_mbox_msb */
			stream_index = reg32_read(IP2APB_DDRPHY_IPS_BASE_ADDR(0) + 4 * 0xd0034);
			stream_index = (stream_index << 16) | stream_nb_args;
#ifdef PRINT_PMU
			printf("PMU stream_index=0x%x nb_args=%d\n",stream_index, stream_nb_args);
#endif  

			stream_arg_pos = 0;
			while (stream_nb_args > 0) {
				/* Need to complete previous handshake first... */
				reg32_write(IP2APB_DDRPHY_IPS_BASE_ADDR(0) + 4 * 0xd0031, 0x0);
				/* poll_mbox_from_uc(1); */

				do {
				    tmp_t = reg32_read(IP2APB_DDRPHY_IPS_BASE_ADDR(0) + 4 * 0xd0004);
				} while((tmp_t & 0x1) == 0x0);
				reg32_write(IP2APB_DDRPHY_IPS_BASE_ADDR(0) + 4 * 0xd0031, 0x1);

				/* Read the next argument... */
				do {
				    tmp_t = reg32_read(IP2APB_DDRPHY_IPS_BASE_ADDR(0) + 4 * 0xd0004);
				}while((tmp_t & 0x1) == 0x1);

				/* read_mbox_mssg */
				message  = reg32_read(IP2APB_DDRPHY_IPS_BASE_ADDR(0) + 4 * 0xd0032);
				/* read_mbox_msb */
				stream_arg_val = reg32_read(IP2APB_DDRPHY_IPS_BASE_ADDR(0) + 4 * 0xd0034);
				stream_arg_val = (stream_arg_val << 16) | message;
#ifdef PRINT_PMU
				printf("PMU stream_arg[%d]=0x%x\n",stream_arg_pos, stream_arg_val);
#endif  
				stream_nb_args--;
				stream_arg_pos++;
			}
#endif
		} else if(tmp==0x07) {
			train_ok = 1;  
			ret = 0;
		} else if(tmp==0xff) {
			train_fail = 1; 
			printf("%c[31;40m",0x1b);
			printf("------- training vt_fail\n");
			printf("%c[0m",0x1b);
			
			ret = -1;
		} else {
			train_ok = 0;
			train_fail = 0;
			stream_msg = 0;
		}

		reg32_write(IP2APB_DDRPHY_IPS_BASE_ADDR(0)+4*0xd0031,0x0);

		if (stream_msg == 1) {
		    tmp_t = reg32_read(IP2APB_DDRPHY_IPS_BASE_ADDR(0)+4*0xd0034);
		    tmp_t = reg32_read(IP2APB_DDRPHY_IPS_BASE_ADDR(0)+4*0xd0034);
		}
	
		tmp = reg32_read(IP2APB_DDRPHY_IPS_BASE_ADDR(0)+4*0xd0004);
		tmp_t = tmp & 0x01;
		while(tmp_t==0){
		    tmp = reg32_read(IP2APB_DDRPHY_IPS_BASE_ADDR(0)+4*0xd0004);
		    tmp_t = tmp & 0x01;
		}
		reg32_write(IP2APB_DDRPHY_IPS_BASE_ADDR(0)+4*0xd0031,0x1);
    }

    return ret;
}

void dwc_ddrphy_phyinit_userCustom_H_readMsgBlock(unsigned long run_2D)
{
}

void dwc_ddrphy_phyinit_userCustom_overrideUserInput(void)
{
}
void dwc_ddrphy_phyinit_userCustom_A_bringupPower(void)
{
}
void dwc_ddrphy_phyinit_userCustom_B_startClockResetPhy(void)
{
}
void dwc_ddrphy_phyinit_userCustom_customPostTrain(void)
{
}
void dwc_ddrphy_phyinit_userCustom_J_enterMissionMode(void)
{
}
