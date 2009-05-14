/*
 * Copyright 2008-2009 Freescale Semiconductor, Inc.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef SDHC_H
#define SDHC_H

#include <linux/types.h>

#define ESDHC_SOFTWARE_RESET_DATA ((u32)0x04000000)
#define ESDHC_SOFTWARE_RESET_CMD  ((u32)0x02000000)
#define ESDHC_SOFTWARE_RESET      ((u32)0x01000000)
#define ESDHC_CMD_INHIBIT       0x00000003
#define ESDHC_SYSCTL_INITA        ((u32)0x08000000)
#define ESDHC_LITTLE_ENDIAN_MODE  ((u32)0x00000020)
#define ESDHC_HW_BIG_ENDIAN_MODE  ((u32)0x00000010)
#define ESDHC_BIG_ENDIAN_MODE     ((u32)0x00000000)
#define ESDHC_ONE_BIT_SUPPORT     ((u32)0x00000000)
#define ESDHC_FOUR_BIT_SUPPORT    ((u32)0x00000002)
#define ESDHC_EIGHT_BIT_SUPPORT   ((u32)0x00000004)
#define ESDHC_CLOCK_ENABLE 		0x00000007
#define ESDHC_FREQ_MASK 0xffff0007
#define ESDHC_SYSCTL_FREQ_MASK    ((u32)0x000FFFF0)
#define ESDHC_SYSCTL_IDENT_FREQ_TO1   ((u32)0x0000800e)
#define ESDHC_SYSCTL_OPERT_FREQ_TO1   ((u32)0x00000200)
#define ESDHC_SYSCTL_IDENT_FREQ_TO2   ((u32)0x00002040)
#define ESDHC_SYSCTL_OPERT_FREQ_TO2   ((u32)0x00000050)
#define ESDHC_INTERRUPT_ENABLE    ((u32)0x007f0133)
#define ESDHC_CLEAR_INTERRUPT     ((u32)0x117f01ff)
#define ESDHC_SYSCTL_DTOCV_VAL    ((u32)0x000E0000)
#define ESDHC_IRQSTATEN_DTOESEN   ((u32)0x00100000)
#define ESDHC_ENDIAN_MODE_MASK    ((u32)0x00000030)
#define ESDHC_SYSCTRL_RSTC        ((u32)0x02000000)
#define ESDHC_SYSCTRL_RSTD        ((u32)0x04000000)
#define ESDHC_CONFIG_BLOCK 0x00010200
#define ESDHC_OPER_TIMEOUT (96 * 100)
#define ESDHC_ACMD41_TIMEOUT      (32000)
#define ESDHC_CMD1_TIMEOUT        (32000)
#define ESDHC_BLOCK_SHIFT         (16)
#define ESDHC_CARD_INIT_TIMEOUT   (64)

#define ESDHC_SYSCTL_SDCLKEN_MASK     ((u32)0x00000008)
#define ESDHC_PRSSTAT_SDSTB_BIT       ((u32)0x00000008)
#define ESDHC_SYSCTL_INPUT_CLOCK_MASK ((u32)0x00000007)

#define ESDHC_BUS_WIDTH_MASK                    ((u32)0x00000006)
#define ESDHC_DATA_TRANSFER_SHIFT               (4)
#define ESDHC_RESPONSE_FORMAT_SHIFT             (16)
#define ESDHC_DATA_PRESENT_SHIFT                (21)
#define ESDHC_CRC_CHECK_SHIFT                   (19)
#define ESDHC_CMD_INDEX_CHECK_SHIFT             (20)
#define ESDHC_CMD_INDEX_SHIFT                   (24)
#define ESDHC_BLOCK_COUNT_ENABLE_SHIFT          (1)
#define ESDHC_MULTI_SINGLE_BLOCK_SELECT_SHIFT   (5)
#define BLK_LEN                           		(512)
#define ESDHC_READ_WATER_MARK_LEVEL_BL_4       ((u32)0x00000001)
#define ESDHC_READ_WATER_MARK_LEVEL_BL_8       ((u32)0x00000002)
#define ESDHC_READ_WATER_MARK_LEVEL_BL_16      ((u32)0x00000004)
#define ESDHC_READ_WATER_MARK_LEVEL_BL_64      ((u32)0x00000010)
#define ESDHC_READ_WATER_MARK_LEVEL_BL_512     ((u32)0x00000080)

#define ESDHC_WRITE_WATER_MARK_LEVEL_BL_4      ((u32)0x00010000)
#define ESDHC_WRITE_WATER_MARK_LEVEL_BL_8      ((u32)0x00020000)
#define ESDHC_WRITE_WATER_MARK_LEVEL_BL_16     ((u32)0x00040000)
#define ESDHC_WRITE_WATER_MARK_LEVEL_BL_64     ((u32)0x00100000)
#define ESDHC_WRITE_WATER_MARK_LEVEL_BL_512    ((u32)0x00800000)

#define WRITE_READ_WATER_MARK_LEVEL 0x00800080

/* Present State register bit masks */
#define ESDHC_PRESENT_STATE_CIHB    ((u32)0x00000001)
#define ESDHC_PRESENT_STATE_CDIHB   ((u32)0x00000002)
#define ONE                         (1)
#define ESDHC_FIFO_SIZE             (128)

#define ESDHC_STATUS_END_CMD_RESP_MSK         ((u32)0x00000001)
#define ESDHC_STATUS_END_CMD_RESP_TIME_MSK    ((u32)0x000F0001)
#define ESDHC_STATUS_TIME_OUT_RESP_MSK        ((u32)0x00010000)
#define ESDHC_STATUS_RESP_CRC_ERR_MSK         ((u32)0x00020000)
#define ESDHC_STATUS_RESP_CMD_INDEX_ERR_MSK   ((u32)0x00080000)
#define ESDHC_STATUS_BUF_READ_RDY_MSK         ((u32)0x00000020)
#define ESDHC_STATUS_BUF_WRITE_RDY_MSK        ((u32)0x00000010)
#define ESDHC_STATUS_TRANSFER_COMPLETE_MSK    ((u32)0x00000002)
#define ESDHC_STATUS_DATA_RW_MSK              ((u32)0x00700002)
#define ESDHC_STATUS_TRANSFER_COMPLETE_MSK    ((u32)0x00000002)
#define ESDHC_STATUS_TIME_OUT_READ_MASK       ((u32)0x00100000)
#define ESDHC_STATUS_READ_CRC_ERR_MSK         ((u32)0x00200000)
#define ESDHC_STATUS_RESP_CMD_END_BIT_ERR_MSK ((u32)0x00040000)
#define ESDHC_STATUS_RW_DATA_END_BIT_ERR_MSK  ((u32)0x00400000)

#define ESDHC_STATUS_TIME_OUT_READ  (3200)
#define ESDHC_READ_DATA_TIME_OUT    (3200)
#define ESDHC_WRITE_DATA_TIME_OUT   (8000)

#define ESDHC_CONFIG_BLOCK_512      ((u32)0x00000200)
#define ESDHC_CONFIG_BLOCK_64       ((u32)0x00000040)
#define ESDHC_CONFIG_BLOCK_8        ((u32)0x00000008)
#define ESDHC_CONFIG_BLOCK_4        ((u32)0x00000004)

#define ESDHC_MAX_BLOCK_COUNT       ((u32)0x0000ffff)

typedef enum {
	ESDHC1,
	ESDHC2,
	ESDHC3
} esdhc_num_t;

typedef enum {
	WRITE,
	READ,
} xfer_type_t;

typedef enum {
	RESPONSE_NONE,
	RESPONSE_136,
	RESPONSE_48,
	RESPONSE_48_CHECK_BUSY
} response_format_t;


typedef enum {
	DATA_PRESENT_NONE,
	DATA_PRESENT
} data_present_select;

typedef enum {
	DISABLE,
	ENABLE
} crc_check_enable, cmdindex_check_enable, block_count_enable;

typedef enum {
	SINGLE,
	MULTIPLE
} multi_single_block_select;

typedef struct {
	u32 command;
	u32 arg;
	xfer_type_t data_transfer;
	response_format_t response_format;
	data_present_select data_present;
	crc_check_enable crc_check;
	cmdindex_check_enable cmdindex_check;
	block_count_enable block_count_enable_check;
	multi_single_block_select multi_single_block;
} esdhc_cmd_t;

typedef struct {
	response_format_t format;
	u32 cmd_rsp0;
	u32 cmd_rsp1;
	u32 cmd_rsp2;
	u32 cmd_rsp3;
} esdhc_resp_t;

typedef enum {
	BIG_ENDIAN,
	HALF_WORD_BIG_ENDIAN,
	LITTLE_ENDIAN
} endian_mode_t;

typedef enum {
	OPERATING_FREQ = 20000,   /* in kHz */
	IDENTIFICATION_FREQ = 400   /* in kHz */
} sdhc_freq_t;

enum esdhc_data_status {
	ESDHC_DATA_ERR = 3,
	ESDHC_DATA_OK = 4
};

enum esdhc_int_cntr_val {
	ESDHC_INT_CNTR_END_CD_RESP = 0x4,
	ESDHC_INT_CNTR_BUF_WR_RDY = 0x8
};

enum esdhc_reset_status {
	ESDHC_WRONG_RESET = 0,
	ESDHC_CORRECT_RESET = 1
};

typedef enum {
	WEAK = 0,
	STRONG = 1
} esdhc_pullup_t;

extern u32 interface_reset(void);
extern void interface_configure_clock(sdhc_freq_t);
extern void interface_read_response(esdhc_resp_t *);
extern u32 interface_send_cmd_wait_resp(esdhc_cmd_t *);
extern u32 interface_data_read(u32 *, u32);
extern void interface_config_block_info(u32, u32, u32);
extern u32 interface_data_write(u32 *, u32);
extern void interface_clear_interrupt(void);
extern void interface_initialization_active(void);
extern void esdhc_set_cmd_pullup(esdhc_pullup_t pull_up);
extern void esdhc_soft_reset(u32 mask);
extern u32 interface_set_bus_width(u32 bus_width);
/*================================================================================================*/
#endif  /* ESDHC_H */
