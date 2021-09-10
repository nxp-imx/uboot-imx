// SPDX-License-Identifier: GPL-2.0+
/*
 * Register definitions for SVC I3C
 *
 * Copyright 2021 NXP
 * Author: Clark Wang (xiaoning.wang@nxp.com)
 */
#ifndef __IMX_I3C_H__
#define __IMX_I3C_H__

#include <clk.h>

struct imx_i3c_reg {
	u32 mconfig;
	u32 sconfig;
	u32 sstatus;
	u32 sctrl;
	u32 sintset;
	u32 sintclr;
	u32 sintmasked;
	u32 serrwarn;
	u32 sdmactrl;
	u8  reserved_0[8];
	u32 sdatactrl;
	u32 swdatab;
	u32 swdatabe;
	u32 swdatah;
	u32 swdatahe;
	u32 srdatab;
	u8  reserved_1[4];
	u32 srdatah;
	u8  reserved_2[20];
	u32 scapabilities;
	u32 sdynaddr;
	u32 smaxlimits;
	u32 sidpartno;
	u32 sidext;
	u32 svendorid;
	u32 stcclock;
	u32 smsgmapaddr;
	u8  reserved_3[4];
	u32 mctrl;
	u32 mstatus;
	u32 mibirules;
	u32 mintset;
	u32 mintclr;
	u32 mintmasked;
	u32 merrwarn;
	u32 mdmactrl;
	u8  reserved_4[8];
	u32 mdatactrl;
	u32 mwdatab;
	u32 mwdatabe;
	u32 mwdatah;
	u32 mwdatahe;
	u32 mrdatab;
	u8  reserved_5[4];
	u32 mrdatah;
	u32 mwmsg_sdr_data; /* the same addr with mwmsg_sdr_control */
	u32 mrmsg_sdr;
	u32 mwmsg_ddr_data; /* the same addr with mwmsg_ddr_control */
	u32 mrmsg_ddr;
	u8  reserved_6[4];
	u32 mdynaddr;
	u8  reserved_7[3860];
	u32 sid;
};

typedef enum i3c_master_state {
	I3C_MASTERSTATE_IDLE    = 0U, /* Bus stopped. */
	I3C_MASTERSTATE_SLVREQ  = 1U, /* Bus stopped but slave holding SDA low. */
	I3C_MASTERSTATE_MSGSDR  = 2U, /* In SDR Message mode from using MWMSG_SDR. */
	I3C_MASTERSTATE_NORMACT = 3U, /* In normal active SDR mode. */
	I3C_MASTERSTATE_DDR     = 4U, /* In DDR Message mode. */
	I3C_MASTERSTATE_DAA     = 5U, /* In ENTDAA mode. */
	I3C_MASTERSTATE_IBIACK  = 6U, /* Waiting on IBI ACK/NACK decision. */
	I3C_MASTERSTATE_IBIRCV  = 7U, /* receiving IBI. */
} i3c_master_state_t;

typedef enum i3c_status {
	I3C_SUCESS = 0,
	I3C_BUSY,/* The master is already performing a transfer. */
	I3C_IDLE,  /* The slave driver is idle. */
	I3C_NACK,  /* The slave device sent a NAK in response to an address. */
	I3C_WRITE_ABORT_ERR,  /* The slave device sent a NAK in response to a write. */
	I3C_TERM,  /* The master terminates slave read. */
	I3C_HDR_PARITY_ERR,  /* Parity error from DDR read. */
	I3C_CRC_ERR,  /* CRC error from DDR read. */
	I3C_READFIFO_ERR,  /* Read from M/SRDATAB register when FIFO empty. */
	I3C_WRITEFIFO_ERR,  /* Write to M/SWDATAB register when FIFO full. */
	I3C_MSG_ERR,  /* Message SDR/DDR mismatch or read/write message in wrong state */
	I3C_INVALID_REQ,  /* Invalid use of request. */
	I3C_TIMEOUT,  /* The module has stalled too long in a frame. */
	I3C_SLAVE_COUNT_EXCEED,  /* The I3C slave count has exceed the definition in I3C_MAX_DEVCNT. */
	I3C_IBI_WON,  /* The I3C slave event IBI or MR or HJ won the arbitration on a header address. */
	I3C_OVERRUN_ERR,  /* Slave internal from-bus buffer/FIFO overrun. */
	I3C_UNDERRUN_ERR,  /* Slave internal to-bus buffer/FIFO underrun */
	I3C_UNDERRUN_NACK_ERR,  /* Slave internal from-bus buffer/FIFO underrun and NACK error */
	I3C_INVALID_START,  /* Slave invalid start flag */
	I3C_SDR_PARITY_ERR,  /* SDR parity error */
	I3C_SO_S1_ERR,  /* S0 or S1 error */
} i3c_status_t;

/* I3C Register Masks */

/* MCONFIG - Master Configuration Register */
#define I3C_MCONFIG_MSTENA_MASK                  (0x3U)
#define I3C_MCONFIG_MSTENA_SHIFT                 (0U)
/* MSTENA - Master enable
 *  0b00..MASTER_OFF: Master is off (is not enabled). If MASTER_OFF is enabled, then the I3C module can only use slave mode.
 *  0b01..MASTER_ON: Master is on (is enabled). When used from start-up, this I3C module is master by default (the
 *        main master). The module will control the bus unless the master is handed off. If the master is handed
 *        off, then MSTENA must move to 2 after that happens. The handoff means emitting GETACCMST and if accepted,
 *        the module will emit a STOP and set the MSTENA bit to 2 (or 0).
 *  0b10..MASTER_CAPABLE: The I3C module is master-capable; however the module is operating as a slave now. When
 *        used from the start, the I3C module will start as a slave, but will be prepared to switch to master mode.
 *        To switch to master mode, the slave emits an Master Request (MR), or gets a GETACCMST CCC command and
 *        accepts it (to switch on the STOP).
 *  0b11..RESERVED
 */
#define I3C_MCONFIG_MSTENA(x)                    (((uint32_t)(((uint32_t)(x)) << I3C_MCONFIG_MSTENA_SHIFT)) & I3C_MCONFIG_MSTENA_MASK)
#define I3C_MCONFIG_DISTO_MASK                   (0x8U)
#define I3C_MCONFIG_DISTO_SHIFT                  (3U)
#define I3C_MCONFIG_DISTO(x)                     (((uint32_t)(((uint32_t)(x)) << I3C_MCONFIG_DISTO_SHIFT)) & I3C_MCONFIG_DISTO_MASK)
#define I3C_MCONFIG_HKEEP_MASK                   (0x30U)
#define I3C_MCONFIG_HKEEP_SHIFT                  (4U)
/* HKEEP - High-Keeper
 *  0b00..NONE: Use PUR (Pull-Up Resistor). Hold SCL High.
 *  0b01..WIRED_IN: Wired-in High Keeper controls; use pin_HK (High Keeper) controls.
 *  0b10..PASSIVE_SDA: Passive on SDA; can Hi-Z (high impedance) for Bus Free (IDLE) and hold.
 *  0b11..PASSIVE_ON_SDA_SCL: Passive on SDA and SCL; can Hi-Z (high impedance) both for Bus Free (IDLE), and can Hi-Z SDA for hold.
 */
#define I3C_MCONFIG_HKEEP(x)                     (((uint32_t)(((uint32_t)(x)) << I3C_MCONFIG_HKEEP_SHIFT)) & I3C_MCONFIG_HKEEP_MASK)
#define I3C_MCONFIG_ODSTOP_MASK                  (0x40U)
#define I3C_MCONFIG_ODSTOP_SHIFT                 (6U)
#define I3C_MCONFIG_ODSTOP(x)                    (((uint32_t)(((uint32_t)(x)) << I3C_MCONFIG_ODSTOP_SHIFT)) & I3C_MCONFIG_ODSTOP_MASK)
#define I3C_MCONFIG_PPBAUD_MASK                  (0xF00U)
#define I3C_MCONFIG_PPBAUD_SHIFT                 (8U)
#define I3C_MCONFIG_PPBAUD(x)                    (((uint32_t)(((uint32_t)(x)) << I3C_MCONFIG_PPBAUD_SHIFT)) & I3C_MCONFIG_PPBAUD_MASK)
#define I3C_MCONFIG_PPLOW_MASK                   (0xF000U)
#define I3C_MCONFIG_PPLOW_SHIFT                  (12U)
#define I3C_MCONFIG_PPLOW(x)                     (((uint32_t)(((uint32_t)(x)) << I3C_MCONFIG_PPLOW_SHIFT)) & I3C_MCONFIG_PPLOW_MASK)
#define I3C_MCONFIG_ODBAUD_MASK                  (0xFF0000U)
#define I3C_MCONFIG_ODBAUD_SHIFT                 (16U)
#define I3C_MCONFIG_ODBAUD(x)                    (((uint32_t)(((uint32_t)(x)) << I3C_MCONFIG_ODBAUD_SHIFT)) & I3C_MCONFIG_ODBAUD_MASK)
#define I3C_MCONFIG_ODHPP_MASK                   (0x1000000U)
#define I3C_MCONFIG_ODHPP_SHIFT                  (24U)
#define I3C_MCONFIG_ODHPP(x)                     (((uint32_t)(((uint32_t)(x)) << I3C_MCONFIG_ODHPP_SHIFT)) & I3C_MCONFIG_ODHPP_MASK)
#define I3C_MCONFIG_SKEW_MASK                    (0xE000000U)
#define I3C_MCONFIG_SKEW_SHIFT                   (25U)
#define I3C_MCONFIG_SKEW(x)                      (((uint32_t)(((uint32_t)(x)) << I3C_MCONFIG_SKEW_SHIFT)) & I3C_MCONFIG_SKEW_MASK)
#define I3C_MCONFIG_I2CBAUD_MASK                 (0xF0000000U)
#define I3C_MCONFIG_I2CBAUD_SHIFT                (28U)
#define I3C_MCONFIG_I2CBAUD(x)                   (((uint32_t)(((uint32_t)(x)) << I3C_MCONFIG_I2CBAUD_SHIFT)) & I3C_MCONFIG_I2CBAUD_MASK)

/* SCONFIG - Slave Configuration Register */
#define I3C_SCONFIG_SLVENA_MASK                  (0x1U)
#define I3C_SCONFIG_SLVENA_SHIFT                 (0U)
#define I3C_SCONFIG_SLVENA(x)                    (((uint32_t)(((uint32_t)(x)) << I3C_SCONFIG_SLVENA_SHIFT)) & I3C_SCONFIG_SLVENA_MASK)
#define I3C_SCONFIG_NACK_MASK                    (0x2U)
#define I3C_SCONFIG_NACK_SHIFT                   (1U)
#define I3C_SCONFIG_NACK(x)                      (((uint32_t)(((uint32_t)(x)) << I3C_SCONFIG_NACK_SHIFT)) & I3C_SCONFIG_NACK_MASK)
#define I3C_SCONFIG_MATCHSS_MASK                 (0x4U)
#define I3C_SCONFIG_MATCHSS_SHIFT                (2U)
#define I3C_SCONFIG_MATCHSS(x)                   (((uint32_t)(((uint32_t)(x)) << I3C_SCONFIG_MATCHSS_SHIFT)) & I3C_SCONFIG_MATCHSS_MASK)
#define I3C_SCONFIG_S0IGNORE_MASK                (0x8U)
#define I3C_SCONFIG_S0IGNORE_SHIFT               (3U)
#define I3C_SCONFIG_S0IGNORE(x)                  (((uint32_t)(((uint32_t)(x)) << I3C_SCONFIG_S0IGNORE_SHIFT)) & I3C_SCONFIG_S0IGNORE_MASK)
#define I3C_SCONFIG_DDROK_MASK                   (0x10U)
#define I3C_SCONFIG_DDROK_SHIFT                  (4U)
#define I3C_SCONFIG_DDROK(x)                     (((uint32_t)(((uint32_t)(x)) << I3C_SCONFIG_DDROK_SHIFT)) & I3C_SCONFIG_DDROK_MASK)
#define I3C_SCONFIG_IDRAND_MASK                  (0x100U)
#define I3C_SCONFIG_IDRAND_SHIFT                 (8U)
#define I3C_SCONFIG_IDRAND(x)                    (((uint32_t)(((uint32_t)(x)) << I3C_SCONFIG_IDRAND_SHIFT)) & I3C_SCONFIG_IDRAND_MASK)
#define I3C_SCONFIG_OFFLINE_MASK                 (0x200U)
#define I3C_SCONFIG_OFFLINE_SHIFT                (9U)
#define I3C_SCONFIG_OFFLINE(x)                   (((uint32_t)(((uint32_t)(x)) << I3C_SCONFIG_OFFLINE_SHIFT)) & I3C_SCONFIG_OFFLINE_MASK)
#define I3C_SCONFIG_BAMATCH_MASK                 (0xFF0000U)
#define I3C_SCONFIG_BAMATCH_SHIFT                (16U)
#define I3C_SCONFIG_BAMATCH(x)                   (((uint32_t)(((uint32_t)(x)) << I3C_SCONFIG_BAMATCH_SHIFT)) & I3C_SCONFIG_BAMATCH_MASK)
#define I3C_SCONFIG_SADDR_MASK                   (0xFE000000U)
#define I3C_SCONFIG_SADDR_SHIFT                  (25U)
#define I3C_SCONFIG_SADDR(x)                     (((uint32_t)(((uint32_t)(x)) << I3C_SCONFIG_SADDR_SHIFT)) & I3C_SCONFIG_SADDR_MASK)

/* SSTATUS - Slave Status Register */
#define I3C_SSTATUS_STNOTSTOP_MASK               (0x1U)
#define I3C_SSTATUS_STNOTSTOP_SHIFT              (0U)
#define I3C_SSTATUS_STNOTSTOP(x)                 (((uint32_t)(((uint32_t)(x)) << I3C_SSTATUS_STNOTSTOP_SHIFT)) & I3C_SSTATUS_STNOTSTOP_MASK)
#define I3C_SSTATUS_STMSG_MASK                   (0x2U)
#define I3C_SSTATUS_STMSG_SHIFT                  (1U)
#define I3C_SSTATUS_STMSG(x)                     (((uint32_t)(((uint32_t)(x)) << I3C_SSTATUS_STMSG_SHIFT)) & I3C_SSTATUS_STMSG_MASK)
#define I3C_SSTATUS_STCCCH_MASK                  (0x4U)
#define I3C_SSTATUS_STCCCH_SHIFT                 (2U)
#define I3C_SSTATUS_STCCCH(x)                    (((uint32_t)(((uint32_t)(x)) << I3C_SSTATUS_STCCCH_SHIFT)) & I3C_SSTATUS_STCCCH_MASK)
#define I3C_SSTATUS_STREQRD_MASK                 (0x8U)
#define I3C_SSTATUS_STREQRD_SHIFT                (3U)
#define I3C_SSTATUS_STREQRD(x)                   (((uint32_t)(((uint32_t)(x)) << I3C_SSTATUS_STREQRD_SHIFT)) & I3C_SSTATUS_STREQRD_MASK)
#define I3C_SSTATUS_STREQWR_MASK                 (0x10U)
#define I3C_SSTATUS_STREQWR_SHIFT                (4U)
#define I3C_SSTATUS_STREQWR(x)                   (((uint32_t)(((uint32_t)(x)) << I3C_SSTATUS_STREQWR_SHIFT)) & I3C_SSTATUS_STREQWR_MASK)
#define I3C_SSTATUS_STDAA_MASK                   (0x20U)
#define I3C_SSTATUS_STDAA_SHIFT                  (5U)
#define I3C_SSTATUS_STDAA(x)                     (((uint32_t)(((uint32_t)(x)) << I3C_SSTATUS_STDAA_SHIFT)) & I3C_SSTATUS_STDAA_MASK)
#define I3C_SSTATUS_STHDR_MASK                   (0x40U)
#define I3C_SSTATUS_STHDR_SHIFT                  (6U)
#define I3C_SSTATUS_STHDR(x)                     (((uint32_t)(((uint32_t)(x)) << I3C_SSTATUS_STHDR_SHIFT)) & I3C_SSTATUS_STHDR_MASK)
#define I3C_SSTATUS_START_MASK                   (0x100U)
#define I3C_SSTATUS_START_SHIFT                  (8U)
#define I3C_SSTATUS_START(x)                     (((uint32_t)(((uint32_t)(x)) << I3C_SSTATUS_START_SHIFT)) & I3C_SSTATUS_START_MASK)
#define I3C_SSTATUS_MATCHED_MASK                 (0x200U)
#define I3C_SSTATUS_MATCHED_SHIFT                (9U)
#define I3C_SSTATUS_MATCHED(x)                   (((uint32_t)(((uint32_t)(x)) << I3C_SSTATUS_MATCHED_SHIFT)) & I3C_SSTATUS_MATCHED_MASK)
#define I3C_SSTATUS_STOP_MASK                    (0x400U)
#define I3C_SSTATUS_STOP_SHIFT                   (10U)
#define I3C_SSTATUS_STOP(x)                      (((uint32_t)(((uint32_t)(x)) << I3C_SSTATUS_STOP_SHIFT)) & I3C_SSTATUS_STOP_MASK)
#define I3C_SSTATUS_RX_PEND_MASK                 (0x800U)
#define I3C_SSTATUS_RX_PEND_SHIFT                (11U)
#define I3C_SSTATUS_RX_PEND(x)                   (((uint32_t)(((uint32_t)(x)) << I3C_SSTATUS_RX_PEND_SHIFT)) & I3C_SSTATUS_RX_PEND_MASK)
#define I3C_SSTATUS_TXNOTFULL_MASK               (0x1000U)
#define I3C_SSTATUS_TXNOTFULL_SHIFT              (12U)
#define I3C_SSTATUS_TXNOTFULL(x)                 (((uint32_t)(((uint32_t)(x)) << I3C_SSTATUS_TXNOTFULL_SHIFT)) & I3C_SSTATUS_TXNOTFULL_MASK)
#define I3C_SSTATUS_DACHG_MASK                   (0x2000U)
#define I3C_SSTATUS_DACHG_SHIFT                  (13U)
#define I3C_SSTATUS_DACHG(x)                     (((uint32_t)(((uint32_t)(x)) << I3C_SSTATUS_DACHG_SHIFT)) & I3C_SSTATUS_DACHG_MASK)
#define I3C_SSTATUS_CCC_MASK                     (0x4000U)
#define I3C_SSTATUS_CCC_SHIFT                    (14U)
#define I3C_SSTATUS_CCC(x)                       (((uint32_t)(((uint32_t)(x)) << I3C_SSTATUS_CCC_SHIFT)) & I3C_SSTATUS_CCC_MASK)
#define I3C_SSTATUS_ERRWARN_MASK                 (0x8000U)
#define I3C_SSTATUS_ERRWARN_SHIFT                (15U)
#define I3C_SSTATUS_ERRWARN(x)                   (((uint32_t)(((uint32_t)(x)) << I3C_SSTATUS_ERRWARN_SHIFT)) & I3C_SSTATUS_ERRWARN_MASK)
#define I3C_SSTATUS_HDRMATCH_MASK                (0x10000U)
#define I3C_SSTATUS_HDRMATCH_SHIFT               (16U)
#define I3C_SSTATUS_HDRMATCH(x)                  (((uint32_t)(((uint32_t)(x)) << I3C_SSTATUS_HDRMATCH_SHIFT)) & I3C_SSTATUS_HDRMATCH_MASK)
#define I3C_SSTATUS_CHANDLED_MASK                (0x20000U)
#define I3C_SSTATUS_CHANDLED_SHIFT               (17U)
#define I3C_SSTATUS_CHANDLED(x)                  (((uint32_t)(((uint32_t)(x)) << I3C_SSTATUS_CHANDLED_SHIFT)) & I3C_SSTATUS_CHANDLED_MASK)
#define I3C_SSTATUS_EVENT_MASK                   (0x40000U)
#define I3C_SSTATUS_EVENT_SHIFT                  (18U)
#define I3C_SSTATUS_EVENT(x)                     (((uint32_t)(((uint32_t)(x)) << I3C_SSTATUS_EVENT_SHIFT)) & I3C_SSTATUS_EVENT_MASK)
#define I3C_SSTATUS_EVDET_MASK                   (0x300000U)
#define I3C_SSTATUS_EVDET_SHIFT                  (20U)
/* EVDET - Event details
 *  0b00..NONE: no event or no pending event
 *  0b01..NO_REQUEST: Request not sent yet. Either there was no START yet, or is waiting for Bus-Available or Bus-Idle (HJ).
 *  0b10..NACKED: Not acknowledged(Request sent and NACKed); the module will try again.
 *  0b11..ACKED: Acknowledged (Request sent and ACKed), so Done (unless the time control data is still being sent).
 */
#define I3C_SSTATUS_EVDET(x)                     (((uint32_t)(((uint32_t)(x)) << I3C_SSTATUS_EVDET_SHIFT)) & I3C_SSTATUS_EVDET_MASK)
#define I3C_SSTATUS_IBIDIS_MASK                  (0x1000000U)
#define I3C_SSTATUS_IBIDIS_SHIFT                 (24U)
#define I3C_SSTATUS_IBIDIS(x)                    (((uint32_t)(((uint32_t)(x)) << I3C_SSTATUS_IBIDIS_SHIFT)) & I3C_SSTATUS_IBIDIS_MASK)
#define I3C_SSTATUS_MRDIS_MASK                   (0x2000000U)
#define I3C_SSTATUS_MRDIS_SHIFT                  (25U)
#define I3C_SSTATUS_MRDIS(x)                     (((uint32_t)(((uint32_t)(x)) << I3C_SSTATUS_MRDIS_SHIFT)) & I3C_SSTATUS_MRDIS_MASK)
#define I3C_SSTATUS_HJDIS_MASK                   (0x8000000U)
#define I3C_SSTATUS_HJDIS_SHIFT                  (27U)
#define I3C_SSTATUS_HJDIS(x)                     (((uint32_t)(((uint32_t)(x)) << I3C_SSTATUS_HJDIS_SHIFT)) & I3C_SSTATUS_HJDIS_MASK)
#define I3C_SSTATUS_ACTSTATE_MASK                (0x30000000U)
#define I3C_SSTATUS_ACTSTATE_SHIFT               (28U)
/* ACTSTATE - Activity state from Common Command Codes (CCC)
 *  0b00..NO_LATENCY: normal bus operations
 *  0b01..LATENCY_1MS: 1 ms of latency
 *  0b10..LATENCY_100MS: 100 ms of latency
 *  0b11..LATENCY_10S: 10 seconds of latency
 */
#define I3C_SSTATUS_ACTSTATE(x)                  (((uint32_t)(((uint32_t)(x)) << I3C_SSTATUS_ACTSTATE_SHIFT)) & I3C_SSTATUS_ACTSTATE_MASK)
#define I3C_SSTATUS_TIMECTRL_MASK                (0xC0000000U)
#define I3C_SSTATUS_TIMECTRL_SHIFT               (30U)
/* TIMECTRL - Time control
 *  0b00..NO_TIME_CONTROL: No time control is enabled
 *  0b01..Reserved
 *  0b10..ASYNC_MODE: Asynchronous standard mode (0) is enabled
 *  0b11..RESERVED
 */
#define I3C_SSTATUS_TIMECTRL(x)                  (((uint32_t)(((uint32_t)(x)) << I3C_SSTATUS_TIMECTRL_SHIFT)) & I3C_SSTATUS_TIMECTRL_MASK)

/* SCTRL - Slave Control Register */
#define I3C_SCTRL_EVENT_MASK                     (0x3U)
#define I3C_SCTRL_EVENT_SHIFT                    (0U)
/* EVENT - EVENT
 *  0b00..NORMAL_MODE: If EVENT is set to 0 after was a non-0 value, event processing will cancel if the event
 *        processing has not yet started; if event processing has already been started, then event processing will not
 *        be be cancelled.
 *  0b01..IBI: Start an In-Band Interrupt. This will try to push an IBI interrupt onto the I3C bus. If data is
 *        associated with the IBI, then the data will be read from the SCTRL.IBIDATA field. If time control is
 *        enabled, then this data will also include any time control-related bytes; additionally, the IBIDATA byte will
 *        have bit 7 set to 1 automatically (as is required for time control). The IBI interrupt will occur after the
 *        1st (mandatory) IBIDATA, if any.
 *  0b10..MASTER_REQUEST: Start a Master-Request.
 *  0b11..HOT_JOIN_REQUEST: Start a Hot-Join request. A Hot-Join Request should only be used when the device is
 *        powered on after the I3C bus is already powered up, or when the device is connected using hot insertion
 *        methods (the device is powered up when it is physically inserted onto the powered-up I3C bus). The hot join
 *        will wait for Bus Idle, and SCTRL.EVENT=HOT_JOIN_REQUEST must be set before the slave enable
 *        (SCONFIG.SLVENA).
 */
#define I3C_SCTRL_EVENT(x)                       (((uint32_t)(((uint32_t)(x)) << I3C_SCTRL_EVENT_SHIFT)) & I3C_SCTRL_EVENT_MASK)
#define I3C_SCTRL_IBIDATA_MASK                   (0xFF00U)
#define I3C_SCTRL_IBIDATA_SHIFT                  (8U)
#define I3C_SCTRL_IBIDATA(x)                     (((uint32_t)(((uint32_t)(x)) << I3C_SCTRL_IBIDATA_SHIFT)) & I3C_SCTRL_IBIDATA_MASK)
#define I3C_SCTRL_PENDINT_MASK                   (0xF0000U)
#define I3C_SCTRL_PENDINT_SHIFT                  (16U)
#define I3C_SCTRL_PENDINT(x)                     (((uint32_t)(((uint32_t)(x)) << I3C_SCTRL_PENDINT_SHIFT)) & I3C_SCTRL_PENDINT_MASK)
#define I3C_SCTRL_ACTSTATE_MASK                  (0x300000U)
#define I3C_SCTRL_ACTSTATE_SHIFT                 (20U)
#define I3C_SCTRL_ACTSTATE(x)                    (((uint32_t)(((uint32_t)(x)) << I3C_SCTRL_ACTSTATE_SHIFT)) & I3C_SCTRL_ACTSTATE_MASK)
#define I3C_SCTRL_VENDINFO_MASK                  (0xFF000000U)
#define I3C_SCTRL_VENDINFO_SHIFT                 (24U)
#define I3C_SCTRL_VENDINFO(x)                    (((uint32_t)(((uint32_t)(x)) << I3C_SCTRL_VENDINFO_SHIFT)) & I3C_SCTRL_VENDINFO_MASK)

/* SINTSET - Slave Interrupt Set Register */
#define I3C_SINTSET_START_MASK                   (0x100U)
#define I3C_SINTSET_START_SHIFT                  (8U)
#define I3C_SINTSET_START(x)                     (((uint32_t)(((uint32_t)(x)) << I3C_SINTSET_START_SHIFT)) & I3C_SINTSET_START_MASK)
#define I3C_SINTSET_MATCHED_MASK                 (0x200U)
#define I3C_SINTSET_MATCHED_SHIFT                (9U)
#define I3C_SINTSET_MATCHED(x)                   (((uint32_t)(((uint32_t)(x)) << I3C_SINTSET_MATCHED_SHIFT)) & I3C_SINTSET_MATCHED_MASK)
#define I3C_SINTSET_STOP_MASK                    (0x400U)
#define I3C_SINTSET_STOP_SHIFT                   (10U)
#define I3C_SINTSET_STOP(x)                      (((uint32_t)(((uint32_t)(x)) << I3C_SINTSET_STOP_SHIFT)) & I3C_SINTSET_STOP_MASK)
#define I3C_SINTSET_RXPEND_MASK                  (0x800U)
#define I3C_SINTSET_RXPEND_SHIFT                 (11U)
#define I3C_SINTSET_RXPEND(x)                    (((uint32_t)(((uint32_t)(x)) << I3C_SINTSET_RXPEND_SHIFT)) & I3C_SINTSET_RXPEND_MASK)
#define I3C_SINTSET_TXSEND_MASK                  (0x1000U)
#define I3C_SINTSET_TXSEND_SHIFT                 (12U)
#define I3C_SINTSET_TXSEND(x)                    (((uint32_t)(((uint32_t)(x)) << I3C_SINTSET_TXSEND_SHIFT)) & I3C_SINTSET_TXSEND_MASK)
#define I3C_SINTSET_DACHG_MASK                   (0x2000U)
#define I3C_SINTSET_DACHG_SHIFT                  (13U)
#define I3C_SINTSET_DACHG(x)                     (((uint32_t)(((uint32_t)(x)) << I3C_SINTSET_DACHG_SHIFT)) & I3C_SINTSET_DACHG_MASK)
#define I3C_SINTSET_CCC_MASK                     (0x4000U)
#define I3C_SINTSET_CCC_SHIFT                    (14U)
#define I3C_SINTSET_CCC(x)                       (((uint32_t)(((uint32_t)(x)) << I3C_SINTSET_CCC_SHIFT)) & I3C_SINTSET_CCC_MASK)
#define I3C_SINTSET_ERRWARN_MASK                 (0x8000U)
#define I3C_SINTSET_ERRWARN_SHIFT                (15U)
#define I3C_SINTSET_ERRWARN(x)                   (((uint32_t)(((uint32_t)(x)) << I3C_SINTSET_ERRWARN_SHIFT)) & I3C_SINTSET_ERRWARN_MASK)
#define I3C_SINTSET_DDRMATCHED_MASK              (0x10000U)
#define I3C_SINTSET_DDRMATCHED_SHIFT             (16U)
#define I3C_SINTSET_DDRMATCHED(x)                (((uint32_t)(((uint32_t)(x)) << I3C_SINTSET_DDRMATCHED_SHIFT)) & I3C_SINTSET_DDRMATCHED_MASK)
#define I3C_SINTSET_CHANDLED_MASK                (0x20000U)
#define I3C_SINTSET_CHANDLED_SHIFT               (17U)
#define I3C_SINTSET_CHANDLED(x)                  (((uint32_t)(((uint32_t)(x)) << I3C_SINTSET_CHANDLED_SHIFT)) & I3C_SINTSET_CHANDLED_MASK)
#define I3C_SINTSET_EVENT_MASK                   (0x40000U)
#define I3C_SINTSET_EVENT_SHIFT                  (18U)
#define I3C_SINTSET_EVENT(x)                     (((uint32_t)(((uint32_t)(x)) << I3C_SINTSET_EVENT_SHIFT)) & I3C_SINTSET_EVENT_MASK)

/* SINTCLR - Slave Interrupt Clear Register */
#define I3C_SINTCLR_START_MASK                   (0x100U)
#define I3C_SINTCLR_START_SHIFT                  (8U)
#define I3C_SINTCLR_START(x)                     (((uint32_t)(((uint32_t)(x)) << I3C_SINTCLR_START_SHIFT)) & I3C_SINTCLR_START_MASK)
#define I3C_SINTCLR_MATCHED_MASK                 (0x200U)
#define I3C_SINTCLR_MATCHED_SHIFT                (9U)
#define I3C_SINTCLR_MATCHED(x)                   (((uint32_t)(((uint32_t)(x)) << I3C_SINTCLR_MATCHED_SHIFT)) & I3C_SINTCLR_MATCHED_MASK)
#define I3C_SINTCLR_STOP_MASK                    (0x400U)
#define I3C_SINTCLR_STOP_SHIFT                   (10U)
#define I3C_SINTCLR_STOP(x)                      (((uint32_t)(((uint32_t)(x)) << I3C_SINTCLR_STOP_SHIFT)) & I3C_SINTCLR_STOP_MASK)
#define I3C_SINTCLR_RXPEND_MASK                  (0x800U)
#define I3C_SINTCLR_RXPEND_SHIFT                 (11U)
#define I3C_SINTCLR_RXPEND(x)                    (((uint32_t)(((uint32_t)(x)) << I3C_SINTCLR_RXPEND_SHIFT)) & I3C_SINTCLR_RXPEND_MASK)
#define I3C_SINTCLR_TXSEND_MASK                  (0x1000U)
#define I3C_SINTCLR_TXSEND_SHIFT                 (12U)
#define I3C_SINTCLR_TXSEND(x)                    (((uint32_t)(((uint32_t)(x)) << I3C_SINTCLR_TXSEND_SHIFT)) & I3C_SINTCLR_TXSEND_MASK)
#define I3C_SINTCLR_DACHG_MASK                   (0x2000U)
#define I3C_SINTCLR_DACHG_SHIFT                  (13U)
#define I3C_SINTCLR_DACHG(x)                     (((uint32_t)(((uint32_t)(x)) << I3C_SINTCLR_DACHG_SHIFT)) & I3C_SINTCLR_DACHG_MASK)
#define I3C_SINTCLR_CCC_MASK                     (0x4000U)
#define I3C_SINTCLR_CCC_SHIFT                    (14U)
#define I3C_SINTCLR_CCC(x)                       (((uint32_t)(((uint32_t)(x)) << I3C_SINTCLR_CCC_SHIFT)) & I3C_SINTCLR_CCC_MASK)
#define I3C_SINTCLR_ERRWARN_MASK                 (0x8000U)
#define I3C_SINTCLR_ERRWARN_SHIFT                (15U)
#define I3C_SINTCLR_ERRWARN(x)                   (((uint32_t)(((uint32_t)(x)) << I3C_SINTCLR_ERRWARN_SHIFT)) & I3C_SINTCLR_ERRWARN_MASK)
#define I3C_SINTCLR_DDRMATCHED_MASK              (0x10000U)
#define I3C_SINTCLR_DDRMATCHED_SHIFT             (16U)
#define I3C_SINTCLR_DDRMATCHED(x)                (((uint32_t)(((uint32_t)(x)) << I3C_SINTCLR_DDRMATCHED_SHIFT)) & I3C_SINTCLR_DDRMATCHED_MASK)
#define I3C_SINTCLR_CHANDLED_MASK                (0x20000U)
#define I3C_SINTCLR_CHANDLED_SHIFT               (17U)
#define I3C_SINTCLR_CHANDLED(x)                  (((uint32_t)(((uint32_t)(x)) << I3C_SINTCLR_CHANDLED_SHIFT)) & I3C_SINTCLR_CHANDLED_MASK)
#define I3C_SINTCLR_EVENT_MASK                   (0x40000U)
#define I3C_SINTCLR_EVENT_SHIFT                  (18U)
#define I3C_SINTCLR_EVENT(x)                     (((uint32_t)(((uint32_t)(x)) << I3C_SINTCLR_EVENT_SHIFT)) & I3C_SINTCLR_EVENT_MASK)

/* SINTMASKED - Slave Interrupt Mask Register */
#define I3C_SINTMASKED_START_MASK                (0x100U)
#define I3C_SINTMASKED_START_SHIFT               (8U)
#define I3C_SINTMASKED_START(x)                  (((uint32_t)(((uint32_t)(x)) << I3C_SINTMASKED_START_SHIFT)) & I3C_SINTMASKED_START_MASK)
#define I3C_SINTMASKED_MATCHED_MASK              (0x200U)
#define I3C_SINTMASKED_MATCHED_SHIFT             (9U)
#define I3C_SINTMASKED_MATCHED(x)                (((uint32_t)(((uint32_t)(x)) << I3C_SINTMASKED_MATCHED_SHIFT)) & I3C_SINTMASKED_MATCHED_MASK)
#define I3C_SINTMASKED_STOP_MASK                 (0x400U)
#define I3C_SINTMASKED_STOP_SHIFT                (10U)
#define I3C_SINTMASKED_STOP(x)                   (((uint32_t)(((uint32_t)(x)) << I3C_SINTMASKED_STOP_SHIFT)) & I3C_SINTMASKED_STOP_MASK)
#define I3C_SINTMASKED_RXPEND_MASK               (0x800U)
#define I3C_SINTMASKED_RXPEND_SHIFT              (11U)
#define I3C_SINTMASKED_RXPEND(x)                 (((uint32_t)(((uint32_t)(x)) << I3C_SINTMASKED_RXPEND_SHIFT)) & I3C_SINTMASKED_RXPEND_MASK)
#define I3C_SINTMASKED_TXSEND_MASK               (0x1000U)
#define I3C_SINTMASKED_TXSEND_SHIFT              (12U)
#define I3C_SINTMASKED_TXSEND(x)                 (((uint32_t)(((uint32_t)(x)) << I3C_SINTMASKED_TXSEND_SHIFT)) & I3C_SINTMASKED_TXSEND_MASK)
#define I3C_SINTMASKED_DACHG_MASK                (0x2000U)
#define I3C_SINTMASKED_DACHG_SHIFT               (13U)
#define I3C_SINTMASKED_DACHG(x)                  (((uint32_t)(((uint32_t)(x)) << I3C_SINTMASKED_DACHG_SHIFT)) & I3C_SINTMASKED_DACHG_MASK)
#define I3C_SINTMASKED_CCC_MASK                  (0x4000U)
#define I3C_SINTMASKED_CCC_SHIFT                 (14U)
#define I3C_SINTMASKED_CCC(x)                    (((uint32_t)(((uint32_t)(x)) << I3C_SINTMASKED_CCC_SHIFT)) & I3C_SINTMASKED_CCC_MASK)
#define I3C_SINTMASKED_ERRWARN_MASK              (0x8000U)
#define I3C_SINTMASKED_ERRWARN_SHIFT             (15U)
#define I3C_SINTMASKED_ERRWARN(x)                (((uint32_t)(((uint32_t)(x)) << I3C_SINTMASKED_ERRWARN_SHIFT)) & I3C_SINTMASKED_ERRWARN_MASK)
#define I3C_SINTMASKED_DDRMATCHED_MASK           (0x10000U)
#define I3C_SINTMASKED_DDRMATCHED_SHIFT          (16U)
#define I3C_SINTMASKED_DDRMATCHED(x)             (((uint32_t)(((uint32_t)(x)) << I3C_SINTMASKED_DDRMATCHED_SHIFT)) & I3C_SINTMASKED_DDRMATCHED_MASK)
#define I3C_SINTMASKED_CHANDLED_MASK             (0x20000U)
#define I3C_SINTMASKED_CHANDLED_SHIFT            (17U)
#define I3C_SINTMASKED_CHANDLED(x)               (((uint32_t)(((uint32_t)(x)) << I3C_SINTMASKED_CHANDLED_SHIFT)) & I3C_SINTMASKED_CHANDLED_MASK)
#define I3C_SINTMASKED_EVENT_MASK                (0x40000U)
#define I3C_SINTMASKED_EVENT_SHIFT               (18U)
#define I3C_SINTMASKED_EVENT(x)                  (((uint32_t)(((uint32_t)(x)) << I3C_SINTMASKED_EVENT_SHIFT)) & I3C_SINTMASKED_EVENT_MASK)

/* SERRWARN - Slave Errors and Warnings Register */
#define I3C_SERRWARN_ORUN_MASK                   (0x1U)
#define I3C_SERRWARN_ORUN_SHIFT                  (0U)
#define I3C_SERRWARN_ORUN(x)                     (((uint32_t)(((uint32_t)(x)) << I3C_SERRWARN_ORUN_SHIFT)) & I3C_SERRWARN_ORUN_MASK)
#define I3C_SERRWARN_URUN_MASK                   (0x2U)
#define I3C_SERRWARN_URUN_SHIFT                  (1U)
#define I3C_SERRWARN_URUN(x)                     (((uint32_t)(((uint32_t)(x)) << I3C_SERRWARN_URUN_SHIFT)) & I3C_SERRWARN_URUN_MASK)
#define I3C_SERRWARN_URUNNACK_MASK               (0x4U)
#define I3C_SERRWARN_URUNNACK_SHIFT              (2U)
#define I3C_SERRWARN_URUNNACK(x)                 (((uint32_t)(((uint32_t)(x)) << I3C_SERRWARN_URUNNACK_SHIFT)) & I3C_SERRWARN_URUNNACK_MASK)
#define I3C_SERRWARN_TERM_MASK                   (0x8U)
#define I3C_SERRWARN_TERM_SHIFT                  (3U)
#define I3C_SERRWARN_TERM(x)                     (((uint32_t)(((uint32_t)(x)) << I3C_SERRWARN_TERM_SHIFT)) & I3C_SERRWARN_TERM_MASK)
#define I3C_SERRWARN_INVSTART_MASK               (0x10U)
#define I3C_SERRWARN_INVSTART_SHIFT              (4U)
#define I3C_SERRWARN_INVSTART(x)                 (((uint32_t)(((uint32_t)(x)) << I3C_SERRWARN_INVSTART_SHIFT)) & I3C_SERRWARN_INVSTART_MASK)
#define I3C_SERRWARN_SPAR_MASK                   (0x100U)
#define I3C_SERRWARN_SPAR_SHIFT                  (8U)
#define I3C_SERRWARN_SPAR(x)                     (((uint32_t)(((uint32_t)(x)) << I3C_SERRWARN_SPAR_SHIFT)) & I3C_SERRWARN_SPAR_MASK)
#define I3C_SERRWARN_HPAR_MASK                   (0x200U)
#define I3C_SERRWARN_HPAR_SHIFT                  (9U)
#define I3C_SERRWARN_HPAR(x)                     (((uint32_t)(((uint32_t)(x)) << I3C_SERRWARN_HPAR_SHIFT)) & I3C_SERRWARN_HPAR_MASK)
#define I3C_SERRWARN_HCRC_MASK                   (0x400U)
#define I3C_SERRWARN_HCRC_SHIFT                  (10U)
#define I3C_SERRWARN_HCRC(x)                     (((uint32_t)(((uint32_t)(x)) << I3C_SERRWARN_HCRC_SHIFT)) & I3C_SERRWARN_HCRC_MASK)
#define I3C_SERRWARN_S0S1_MASK                   (0x800U)
#define I3C_SERRWARN_S0S1_SHIFT                  (11U)
#define I3C_SERRWARN_S0S1(x)                     (((uint32_t)(((uint32_t)(x)) << I3C_SERRWARN_S0S1_SHIFT)) & I3C_SERRWARN_S0S1_MASK)
#define I3C_SERRWARN_OREAD_MASK                  (0x10000U)
#define I3C_SERRWARN_OREAD_SHIFT                 (16U)
#define I3C_SERRWARN_OREAD(x)                    (((uint32_t)(((uint32_t)(x)) << I3C_SERRWARN_OREAD_SHIFT)) & I3C_SERRWARN_OREAD_MASK)
#define I3C_SERRWARN_OWRITE_MASK                 (0x20000U)
#define I3C_SERRWARN_OWRITE_SHIFT                (17U)
#define I3C_SERRWARN_OWRITE(x)                   (((uint32_t)(((uint32_t)(x)) << I3C_SERRWARN_OWRITE_SHIFT)) & I3C_SERRWARN_OWRITE_MASK)

/* SDMACTRL - Slave DMA Control Register */
#define I3C_SDMACTRL_DMAFB_MASK                  (0x3U)
#define I3C_SDMACTRL_DMAFB_SHIFT                 (0U)
/* DMAFB - DMA Read (From-bus) trigger
 *  0b00..DMA not used
 *  0b01..DMA is enabled for 1 frame
 *  0b10..DMA enable
 */
#define I3C_SDMACTRL_DMAFB(x)                    (((uint32_t)(((uint32_t)(x)) << I3C_SDMACTRL_DMAFB_SHIFT)) & I3C_SDMACTRL_DMAFB_MASK)
#define I3C_SDMACTRL_DMATB_MASK                  (0xCU)
#define I3C_SDMACTRL_DMATB_SHIFT                 (2U)
/* DMATB - DMA Write (To-bus) trigger
 *  0b00..NOT_USED: DMA is not used
 *  0b01..ENABLE_ONE_FRAME: DMA is enabled for 1 Frame (ended by DMA or terminated). DMATB auto-clears on a STOP
 *        or START (see the Match START or STOP bit (SCONFIG.MATCHSS).
 *  0b10..ENABLE: DMA is enabled until turned off. Normally, ENABLE should only be used with Master Message mode.
 */
#define I3C_SDMACTRL_DMATB(x)                    (((uint32_t)(((uint32_t)(x)) << I3C_SDMACTRL_DMATB_SHIFT)) & I3C_SDMACTRL_DMATB_MASK)
#define I3C_SDMACTRL_DMAWIDTH_MASK               (0x30U)
#define I3C_SDMACTRL_DMAWIDTH_SHIFT              (4U)
/* DMAWIDTH - Width of DMA operations
 *  0b00..BYTE
 *  0b01..BYTE_AGAIN
 *  0b10..HALF_WORD: Half word (16 bits). This will make sure that 2 bytes are free/available in the FIFO.
 *  0b11..RESERVED
 */
#define I3C_SDMACTRL_DMAWIDTH(x)                 (((uint32_t)(((uint32_t)(x)) << I3C_SDMACTRL_DMAWIDTH_SHIFT)) & I3C_SDMACTRL_DMAWIDTH_MASK)

/* SDATACTRL - Slave Data Control Register */
#define I3C_SDATACTRL_FLUSHTB_MASK               (0x1U)
#define I3C_SDATACTRL_FLUSHTB_SHIFT              (0U)
#define I3C_SDATACTRL_FLUSHTB(x)                 (((uint32_t)(((uint32_t)(x)) << I3C_SDATACTRL_FLUSHTB_SHIFT)) & I3C_SDATACTRL_FLUSHTB_MASK)
#define I3C_SDATACTRL_FLUSHFB_MASK               (0x2U)
#define I3C_SDATACTRL_FLUSHFB_SHIFT              (1U)
#define I3C_SDATACTRL_FLUSHFB(x)                 (((uint32_t)(((uint32_t)(x)) << I3C_SDATACTRL_FLUSHFB_SHIFT)) & I3C_SDATACTRL_FLUSHFB_MASK)
#define I3C_SDATACTRL_UNLOCK_MASK                (0x8U)
#define I3C_SDATACTRL_UNLOCK_SHIFT               (3U)
#define I3C_SDATACTRL_UNLOCK(x)                  (((uint32_t)(((uint32_t)(x)) << I3C_SDATACTRL_UNLOCK_SHIFT)) & I3C_SDATACTRL_UNLOCK_MASK)
#define I3C_SDATACTRL_TXTRIG_MASK                (0x30U)
#define I3C_SDATACTRL_TXTRIG_SHIFT               (4U)
/* TXTRIG - Trigger level for TX FIFO emptiness
 *  0b00..Trigger on empty
 *  0b01..Trigger on ¼ full or less
 *  0b10..Trigger on .5 full or less
 *  0b11..Trigger on 1 less than full or less (Default)
 */
#define I3C_SDATACTRL_TXTRIG(x)                  (((uint32_t)(((uint32_t)(x)) << I3C_SDATACTRL_TXTRIG_SHIFT)) & I3C_SDATACTRL_TXTRIG_MASK)
#define I3C_SDATACTRL_RXTRIG_MASK                (0xC0U)
#define I3C_SDATACTRL_RXTRIG_SHIFT               (6U)
/* RXTRIG - Trigger level for RX FIFO fullness
 *  0b00..Trigger on not empty
 *  0b01..Trigger on ¼ or more full
 *  0b10..Trigger on .5 or more full
 *  0b11..Trigger on 3/4 or more full
 */
#define I3C_SDATACTRL_RXTRIG(x)                  (((uint32_t)(((uint32_t)(x)) << I3C_SDATACTRL_RXTRIG_SHIFT)) & I3C_SDATACTRL_RXTRIG_MASK)
#define I3C_SDATACTRL_TXCOUNT_MASK               (0x1F0000U)
#define I3C_SDATACTRL_TXCOUNT_SHIFT              (16U)
#define I3C_SDATACTRL_TXCOUNT(x)                 (((uint32_t)(((uint32_t)(x)) << I3C_SDATACTRL_TXCOUNT_SHIFT)) & I3C_SDATACTRL_TXCOUNT_MASK)
#define I3C_SDATACTRL_RXCOUNT_MASK               (0x1F000000U)
#define I3C_SDATACTRL_RXCOUNT_SHIFT              (24U)
#define I3C_SDATACTRL_RXCOUNT(x)                 (((uint32_t)(((uint32_t)(x)) << I3C_SDATACTRL_RXCOUNT_SHIFT)) & I3C_SDATACTRL_RXCOUNT_MASK)
#define I3C_SDATACTRL_TXFULL_MASK                (0x40000000U)
#define I3C_SDATACTRL_TXFULL_SHIFT               (30U)
/* TXFULL - TX is full
 *  0b1..TX is full
 *  0b0..TX is not full
 */
#define I3C_SDATACTRL_TXFULL(x)                  (((uint32_t)(((uint32_t)(x)) << I3C_SDATACTRL_TXFULL_SHIFT)) & I3C_SDATACTRL_TXFULL_MASK)
#define I3C_SDATACTRL_RXEMPTY_MASK               (0x80000000U)
#define I3C_SDATACTRL_RXEMPTY_SHIFT              (31U)
/* RXEMPTY - RX is empty
 *  0b1..RX is empty
 *  0b0..RX is not empty
 */
#define I3C_SDATACTRL_RXEMPTY(x)                 (((uint32_t)(((uint32_t)(x)) << I3C_SDATACTRL_RXEMPTY_SHIFT)) & I3C_SDATACTRL_RXEMPTY_MASK)

/* SWDATAB - Slave Write Data Byte Register */
#define I3C_SWDATAB_DATA_MASK                    (0xFFU)
#define I3C_SWDATAB_DATA_SHIFT                   (0U)
#define I3C_SWDATAB_DATA(x)                      (((uint32_t)(((uint32_t)(x)) << I3C_SWDATAB_DATA_SHIFT)) & I3C_SWDATAB_DATA_MASK)
#define I3C_SWDATAB_END_MASK                     (0x100U)
#define I3C_SWDATAB_END_SHIFT                    (8U)
#define I3C_SWDATAB_END(x)                       (((uint32_t)(((uint32_t)(x)) << I3C_SWDATAB_END_SHIFT)) & I3C_SWDATAB_END_MASK)
#define I3C_SWDATAB_END_ALSO_MASK                (0x10000U)
#define I3C_SWDATAB_END_ALSO_SHIFT               (16U)
#define I3C_SWDATAB_END_ALSO(x)                  (((uint32_t)(((uint32_t)(x)) << I3C_SWDATAB_END_ALSO_SHIFT)) & I3C_SWDATAB_END_ALSO_MASK)

/* SWDATABE - Slave Write Data Byte End */
#define I3C_SWDATABE_DATA_MASK                   (0xFFU)
#define I3C_SWDATABE_DATA_SHIFT                  (0U)
#define I3C_SWDATABE_DATA(x)                     (((uint32_t)(((uint32_t)(x)) << I3C_SWDATABE_DATA_SHIFT)) & I3C_SWDATABE_DATA_MASK)

/* SWDATAH - Slave Write Data Half-word Register */
#define I3C_SWDATAH_DATA0_MASK                   (0xFFU)
#define I3C_SWDATAH_DATA0_SHIFT                  (0U)
#define I3C_SWDATAH_DATA0(x)                     (((uint32_t)(((uint32_t)(x)) << I3C_SWDATAH_DATA0_SHIFT)) & I3C_SWDATAH_DATA0_MASK)
#define I3C_SWDATAH_DATA1_MASK                   (0xFF00U)
#define I3C_SWDATAH_DATA1_SHIFT                  (8U)
#define I3C_SWDATAH_DATA1(x)                     (((uint32_t)(((uint32_t)(x)) << I3C_SWDATAH_DATA1_SHIFT)) & I3C_SWDATAH_DATA1_MASK)
#define I3C_SWDATAH_END_MASK                     (0x10000U)
#define I3C_SWDATAH_END_SHIFT                    (16U)
#define I3C_SWDATAH_END(x)                       (((uint32_t)(((uint32_t)(x)) << I3C_SWDATAH_END_SHIFT)) & I3C_SWDATAH_END_MASK)

/* SWDATAHE - Slave Write Data Half-word End Register */
#define I3C_SWDATAHE_DATA0_MASK                  (0xFFU)
#define I3C_SWDATAHE_DATA0_SHIFT                 (0U)
#define I3C_SWDATAHE_DATA0(x)                    (((uint32_t)(((uint32_t)(x)) << I3C_SWDATAHE_DATA0_SHIFT)) & I3C_SWDATAHE_DATA0_MASK)
#define I3C_SWDATAHE_DATA1_MASK                  (0xFF00U)
#define I3C_SWDATAHE_DATA1_SHIFT                 (8U)
#define I3C_SWDATAHE_DATA1(x)                    (((uint32_t)(((uint32_t)(x)) << I3C_SWDATAHE_DATA1_SHIFT)) & I3C_SWDATAHE_DATA1_MASK)

/* SRDATAB - Slave Read Data Byte Register */
#define I3C_SRDATAB_DATA0_MASK                   (0xFFU)
#define I3C_SRDATAB_DATA0_SHIFT                  (0U)
#define I3C_SRDATAB_DATA0(x)                     (((uint32_t)(((uint32_t)(x)) << I3C_SRDATAB_DATA0_SHIFT)) & I3C_SRDATAB_DATA0_MASK)

/* SRDATAH - Slave Read Data Half-word Register */
#define I3C_SRDATAH_LSB_MASK                     (0xFFU)
#define I3C_SRDATAH_LSB_SHIFT                    (0U)
#define I3C_SRDATAH_LSB(x)                       (((uint32_t)(((uint32_t)(x)) << I3C_SRDATAH_LSB_SHIFT)) & I3C_SRDATAH_LSB_MASK)
#define I3C_SRDATAH_MSB_MASK                     (0xFF00U)
#define I3C_SRDATAH_MSB_SHIFT                    (8U)
#define I3C_SRDATAH_MSB(x)                       (((uint32_t)(((uint32_t)(x)) << I3C_SRDATAH_MSB_SHIFT)) & I3C_SRDATAH_MSB_MASK)

/* SCAPABILITIES - Slave Capabilities Register */
#define I3C_SCAPABILITIES_IDENA_MASK             (0x3U)
#define I3C_SCAPABILITIES_IDENA_SHIFT            (0U)
/* IDENA - ID 48b handler
 *  0b00..APPLICATION: Application handles ID 48b
 *  0b01..HW: Hardware handles ID 48b
 *  0b10..HW_BUT: in hardware but the I3C module instance handles ID 48b.
 *  0b11..PARTNO: a part number register (PARTNO) handles ID 48b
 */
#define I3C_SCAPABILITIES_IDENA(x)               (((uint32_t)(((uint32_t)(x)) << I3C_SCAPABILITIES_IDENA_SHIFT)) & I3C_SCAPABILITIES_IDENA_MASK)
#define I3C_SCAPABILITIES_IDREG_MASK             (0x3CU)
#define I3C_SCAPABILITIES_IDREG_SHIFT            (2U)
#define I3C_SCAPABILITIES_IDREG(x)               (((uint32_t)(((uint32_t)(x)) << I3C_SCAPABILITIES_IDREG_SHIFT)) & I3C_SCAPABILITIES_IDREG_MASK)
#define I3C_SCAPABILITIES_HDRSUPP_MASK           (0x1C0U)
#define I3C_SCAPABILITIES_HDRSUPP_SHIFT          (6U)
#define I3C_SCAPABILITIES_HDRSUPP(x)             (((uint32_t)(((uint32_t)(x)) << I3C_SCAPABILITIES_HDRSUPP_SHIFT)) & I3C_SCAPABILITIES_HDRSUPP_MASK)
#define I3C_SCAPABILITIES_MASTER_MASK            (0x200U)
#define I3C_SCAPABILITIES_MASTER_SHIFT           (9U)
/* MASTER - Master
 *  0b0..MASTERNOTSUPPORTED: master capability is not supported.
 *  0b1..MASTERSUPPORTED: master capability is supported.
 */
#define I3C_SCAPABILITIES_MASTER(x)              (((uint32_t)(((uint32_t)(x)) << I3C_SCAPABILITIES_MASTER_SHIFT)) & I3C_SCAPABILITIES_MASTER_MASK)
#define I3C_SCAPABILITIES_SADDR_MASK             (0xC00U)
#define I3C_SCAPABILITIES_SADDR_SHIFT            (10U)
/* SADDR - Static address
 *  0b00..NO_STATIC: No static address
 *  0b01..STATIC: Static address is fixed in hardware
 *  0b10..HW_CONTROL: Hardware controls the static address dynamically (for example, from the pin strap)
 *  0b11..CONFIG: SCONFIG register supplies the static address
 */
#define I3C_SCAPABILITIES_SADDR(x)               (((uint32_t)(((uint32_t)(x)) << I3C_SCAPABILITIES_SADDR_SHIFT)) & I3C_SCAPABILITIES_SADDR_MASK)
#define I3C_SCAPABILITIES_CCCHANDLE_MASK         (0xF000U)
#define I3C_SCAPABILITIES_CCCHANDLE_SHIFT        (12U)
#define I3C_SCAPABILITIES_CCCHANDLE(x)           (((uint32_t)(((uint32_t)(x)) << I3C_SCAPABILITIES_CCCHANDLE_SHIFT)) & I3C_SCAPABILITIES_CCCHANDLE_MASK)
#define I3C_SCAPABILITIES_IBI_MR_HJ_MASK         (0x1F0000U)
#define I3C_SCAPABILITIES_IBI_MR_HJ_SHIFT        (16U)
#define I3C_SCAPABILITIES_IBI_MR_HJ(x)           (((uint32_t)(((uint32_t)(x)) << I3C_SCAPABILITIES_IBI_MR_HJ_SHIFT)) & I3C_SCAPABILITIES_IBI_MR_HJ_MASK)
#define I3C_SCAPABILITIES_TIMECTRL_MASK          (0x200000U)
#define I3C_SCAPABILITIES_TIMECTRL_SHIFT         (21U)
/* TIMECTRL - Time control
 *  0b0..NO_TIME_CONTROL_TYPE: No time control is enabled
 *  0b1..ATLEAST1_TIME_CONTROL: at least one time-control type is supported
 */
#define I3C_SCAPABILITIES_TIMECTRL(x)            (((uint32_t)(((uint32_t)(x)) << I3C_SCAPABILITIES_TIMECTRL_SHIFT)) & I3C_SCAPABILITIES_TIMECTRL_MASK)
#define I3C_SCAPABILITIES_EXTFIFO_MASK           (0x3800000U)
#define I3C_SCAPABILITIES_EXTFIFO_SHIFT          (23U)
/* EXTFIFO - External FIFO
 *  0b000..NO_EXT_FIFO: No external FIFO is available
 *  0b001..STD_EXT_FIFO: standard available/free external FIFO
 *  0b010..REQUEST_EXT_FIFO: request track external FIFO
 *  0b011..RESERVED
 */
#define I3C_SCAPABILITIES_EXTFIFO(x)             (((uint32_t)(((uint32_t)(x)) << I3C_SCAPABILITIES_EXTFIFO_SHIFT)) & I3C_SCAPABILITIES_EXTFIFO_MASK)
#define I3C_SCAPABILITIES_FIFOTX_MASK            (0xC000000U)
#define I3C_SCAPABILITIES_FIFOTX_SHIFT           (26U)
/* FIFOTX - FIFO transmit
 *  0b00..FIFO_2BYTE: 2-byte TX FIFO, the default FIFO transmit value (FIFOTX)
 *  0b01..FIFO_4BYTE: 4-byte TX FIFO
 *  0b10..FIFO_8BYTE: 8-byte TX FIFO
 *  0b11..FIFO_16BYTE: 16-byte TX FIFO
 */
#define I3C_SCAPABILITIES_FIFOTX(x)              (((uint32_t)(((uint32_t)(x)) << I3C_SCAPABILITIES_FIFOTX_SHIFT)) & I3C_SCAPABILITIES_FIFOTX_MASK)
#define I3C_SCAPABILITIES_FIFORX_MASK            (0x30000000U)
#define I3C_SCAPABILITIES_FIFORX_SHIFT           (28U)
/* FIFORX - FIFO receive
 *  0b00..FIFO_2BYTE: 2 (or 3)-byte RX FIFO, the default FIFO receive value (FIFORX)
 *  0b01..FIFO_4BYTE: 4-byte RX FIFO
 *  0b10..FIFO_8BYTE: 8-byte RX FIFO
 *  0b11..FIFO_16BYTE: 16-byte RX FIFO
 */
#define I3C_SCAPABILITIES_FIFORX(x)              (((uint32_t)(((uint32_t)(x)) << I3C_SCAPABILITIES_FIFORX_SHIFT)) & I3C_SCAPABILITIES_FIFORX_MASK)
#define I3C_SCAPABILITIES_INT_MASK               (0x40000000U)
#define I3C_SCAPABILITIES_INT_SHIFT              (30U)
/* INT - INT
 *  0b1..Interrupts are supported
 *  0b0..Interrupts are not supported
 */
#define I3C_SCAPABILITIES_INT(x)                 (((uint32_t)(((uint32_t)(x)) << I3C_SCAPABILITIES_INT_SHIFT)) & I3C_SCAPABILITIES_INT_MASK)
#define I3C_SCAPABILITIES_DMA_MASK               (0x80000000U)
#define I3C_SCAPABILITIES_DMA_SHIFT              (31U)
/* DMA - DMA
 *  0b1..DMA is supported
 *  0b0..DMA is not supported
 */
#define I3C_SCAPABILITIES_DMA(x)                 (((uint32_t)(((uint32_t)(x)) << I3C_SCAPABILITIES_DMA_SHIFT)) & I3C_SCAPABILITIES_DMA_MASK)

/* SDYNADDR - Slave Dynamic Address Register */
#define I3C_SDYNADDR_DAVALID_MASK                (0x1U)
#define I3C_SDYNADDR_DAVALID_SHIFT               (0U)
/* DAVALID - DAVALID
 *  0b0..DANOTASSIGNED: a Dynamic Address is not assigned
 *  0b1..DAASSIGNED: a Dynamic Address is assigned
 */
#define I3C_SDYNADDR_DAVALID(x)                  (((uint32_t)(((uint32_t)(x)) << I3C_SDYNADDR_DAVALID_SHIFT)) & I3C_SDYNADDR_DAVALID_MASK)
#define I3C_SDYNADDR_DADDR_MASK                  (0xFEU)
#define I3C_SDYNADDR_DADDR_SHIFT                 (1U)
#define I3C_SDYNADDR_DADDR(x)                    (((uint32_t)(((uint32_t)(x)) << I3C_SDYNADDR_DADDR_SHIFT)) & I3C_SDYNADDR_DADDR_MASK)
#define I3C_SDYNADDR_MAPIDX_MASK                 (0xF00U)
#define I3C_SDYNADDR_MAPIDX_SHIFT                (8U)
#define I3C_SDYNADDR_MAPIDX(x)                   (((uint32_t)(((uint32_t)(x)) << I3C_SDYNADDR_MAPIDX_SHIFT)) & I3C_SDYNADDR_MAPIDX_MASK)
#define I3C_SDYNADDR_MAPSA_MASK                  (0x1000U)
#define I3C_SDYNADDR_MAPSA_SHIFT                 (12U)
#define I3C_SDYNADDR_MAPSA(x)                    (((uint32_t)(((uint32_t)(x)) << I3C_SDYNADDR_MAPSA_SHIFT)) & I3C_SDYNADDR_MAPSA_MASK)
#define I3C_SDYNADDR_KEY_MASK                    (0xFFFF0000U)
#define I3C_SDYNADDR_KEY_SHIFT                   (16U)
#define I3C_SDYNADDR_KEY(x)                      (((uint32_t)(((uint32_t)(x)) << I3C_SDYNADDR_KEY_SHIFT)) & I3C_SDYNADDR_KEY_MASK)

/* SMAXLIMITS - Slave Maximum Limits Register */
#define I3C_SMAXLIMITS_MAXRD_MASK                (0xFFFU)
#define I3C_SMAXLIMITS_MAXRD_SHIFT               (0U)
#define I3C_SMAXLIMITS_MAXRD(x)                  (((uint32_t)(((uint32_t)(x)) << I3C_SMAXLIMITS_MAXRD_SHIFT)) & I3C_SMAXLIMITS_MAXRD_MASK)
#define I3C_SMAXLIMITS_MAXWR_MASK                (0xFFF0000U)
#define I3C_SMAXLIMITS_MAXWR_SHIFT               (16U)
#define I3C_SMAXLIMITS_MAXWR(x)                  (((uint32_t)(((uint32_t)(x)) << I3C_SMAXLIMITS_MAXWR_SHIFT)) & I3C_SMAXLIMITS_MAXWR_MASK)

/* SIDPARTNO - Slave ID Part Number Register */
#define I3C_SIDPARTNO_PARTNO_MASK                (0xFFFFFFFFU)
#define I3C_SIDPARTNO_PARTNO_SHIFT               (0U)
#define I3C_SIDPARTNO_PARTNO(x)                  (((uint32_t)(((uint32_t)(x)) << I3C_SIDPARTNO_PARTNO_SHIFT)) & I3C_SIDPARTNO_PARTNO_MASK)

/* SIDEXT - Slave ID Extension Register */
#define I3C_SIDEXT_DCR_MASK                      (0xFF00U)
#define I3C_SIDEXT_DCR_SHIFT                     (8U)
#define I3C_SIDEXT_DCR(x)                        (((uint32_t)(((uint32_t)(x)) << I3C_SIDEXT_DCR_SHIFT)) & I3C_SIDEXT_DCR_MASK)
#define I3C_SIDEXT_BCR_MASK                      (0xFF0000U)
#define I3C_SIDEXT_BCR_SHIFT                     (16U)
#define I3C_SIDEXT_BCR(x)                        (((uint32_t)(((uint32_t)(x)) << I3C_SIDEXT_BCR_SHIFT)) & I3C_SIDEXT_BCR_MASK)

/* SVENDORID - Slave Vendor ID Register */
#define I3C_SVENDORID_VID_MASK                   (0x7FFFU)
#define I3C_SVENDORID_VID_SHIFT                  (0U)
#define I3C_SVENDORID_VID(x)                     (((uint32_t)(((uint32_t)(x)) << I3C_SVENDORID_VID_SHIFT)) & I3C_SVENDORID_VID_MASK)

/* STCCLOCK - Slave Time Control Clock Register */
#define I3C_STCCLOCK_ACCURACY_MASK               (0xFFU)
#define I3C_STCCLOCK_ACCURACY_SHIFT              (0U)
#define I3C_STCCLOCK_ACCURACY(x)                 (((uint32_t)(((uint32_t)(x)) << I3C_STCCLOCK_ACCURACY_SHIFT)) & I3C_STCCLOCK_ACCURACY_MASK)
#define I3C_STCCLOCK_FREQ_MASK                   (0xFF00U)
#define I3C_STCCLOCK_FREQ_SHIFT                  (8U)
#define I3C_STCCLOCK_FREQ(x)                     (((uint32_t)(((uint32_t)(x)) << I3C_STCCLOCK_FREQ_SHIFT)) & I3C_STCCLOCK_FREQ_MASK)

/* SMSGMAPADDR - Slave Message-Mapped Address Register */
#define I3C_SMSGMAPADDR_MAPLAST_MASK             (0xFU)
#define I3C_SMSGMAPADDR_MAPLAST_SHIFT            (0U)
#define I3C_SMSGMAPADDR_MAPLAST(x)               (((uint32_t)(((uint32_t)(x)) << I3C_SMSGMAPADDR_MAPLAST_SHIFT)) & I3C_SMSGMAPADDR_MAPLAST_MASK)
#define I3C_SMSGMAPADDR_MAPLASTM1_MASK           (0xF00U)
#define I3C_SMSGMAPADDR_MAPLASTM1_SHIFT          (8U)
#define I3C_SMSGMAPADDR_MAPLASTM1(x)             (((uint32_t)(((uint32_t)(x)) << I3C_SMSGMAPADDR_MAPLASTM1_SHIFT)) & I3C_SMSGMAPADDR_MAPLASTM1_MASK)
#define I3C_SMSGMAPADDR_MAPLASTM2_MASK           (0xF0000U)
#define I3C_SMSGMAPADDR_MAPLASTM2_SHIFT          (16U)
#define I3C_SMSGMAPADDR_MAPLASTM2(x)             (((uint32_t)(((uint32_t)(x)) << I3C_SMSGMAPADDR_MAPLASTM2_SHIFT)) & I3C_SMSGMAPADDR_MAPLASTM2_MASK)

/* MCTRL - Master Main Control Register */
#define I3C_MCTRL_REQUEST_MASK                   (0x7U)
#define I3C_MCTRL_REQUEST_SHIFT                  (0U)
/* REQUEST - Request
 *  0b000..NONE: Returns to this when finished with any request. The MSTATUS register indicates the master's
 *         state. See also AutoIBI mode. NONE is only written as 0: when setting RDTERM to 1 (to stop a read in
 *         progress) or when setting IBI reponse field (IBIRESP) for MSG use
 *  0b001..EMITSTARTADDR: Emit START with address and direction from a stopped state or in the middle of a Single
 *         Data Rate (SDR) message. If from a stopped state (IDLE), then emit start may be prevented by an event
 *         (like IBI, MR, HJ), in which case the appropriate interrupt is signaled; note that Emit START can be
 *         resubmitted.
 *  0b010..EMITSTOP: Emit a STOP on bus. Must be in Single Data Rate (SDR) mode. If in Dynamic Address Assignment
 *         (DAA) mode, Emit stop will exit DAA mode.
 *  0b011..IBIACKNACK: Manual In-Band Interrupt (IBI) Acknowledge (ACK) or Not Acknowledge (NACK). When IBIRESP
 *         has indicated a hold on an In-Band Interrupt to allow a manual decision, this request completes it. Uses
 *         IBIRESP to provide the information.
 *  0b100..PROCESSDAA: If not in Dynamic Address Assignment (DAA) mode now, will issue START, 7E, ENTDAA, and then
 *         will emit 7E/R to process each slave. Will stop just before the new Dynamic Address (DA) is to be
 *         emitted. The next Process DAA request will use the Addr field as the new DA to assign. If NACKed on the 7E/R,
 *         then the interrupt will indicate this situation, and a STOP will be emitted.
 *  0b101..RESERVED
 *  0b110..FORCEEXIT and IBHR: Emit an Exit Pattern from any state, but end Double Data Rate (DDR) (including
 *         MSGDDR), if in DDR mode now. Includes a STOP afterward. If TYPE != 0, then it will perform an IBHR (In-Band
 *         Hardware Reset). If TYPE=2, then it does a normal reset (DEFRST can prevent the reset). If TYPE=3, it
 *         does a forced reset (will always reset).
 *  0b111..AUTOIBI: Hold in a stopped state, but auto-emit START,7E when the slave is holding down SDA to get an
 *         In-Band Interrupt (IBI). Actual In-Band Interrupt handling is defined by IBIRESP.
 */
#define I3C_MCTRL_REQUEST(x)                     (((uint32_t)(((uint32_t)(x)) << I3C_MCTRL_REQUEST_SHIFT)) & I3C_MCTRL_REQUEST_MASK)
#define I3C_MCTRL_TYPE_MASK                      (0x30U)
#define I3C_MCTRL_TYPE_SHIFT                     (4U)
/* TYPE - Bus type with START
 *  0b00..I3C: Normally the SDR mode of I3C. For ForceExit, the Exit pattern.
 *  0b01..I2C: Normally the Standard I2C protocol.
 *  0b10..DDR: (Double Data Rate): Normally the HDR-DDR mode of I3C. Enter DDR mode (7E and then ENTHDR0), if the
 *        module is not already in DDR mode. The 1st byte written to the TX FIFO should be a command, and should
 *        already be in the FIFO. To end DDR mode, use ForceExit. For ForceExit, the normal IBHR (In-Band Hardware
 *        Reset).
 *  0b11..For ForcedExit, this is forced IBHR.
 */
#define I3C_MCTRL_TYPE(x)                        (((uint32_t)(((uint32_t)(x)) << I3C_MCTRL_TYPE_SHIFT)) & I3C_MCTRL_TYPE_MASK)
#define I3C_MCTRL_IBIRESP_MASK                   (0xC0U)
#define I3C_MCTRL_IBIRESP_SHIFT                  (6U)
/* IBIRESP - In-Band Interrupt (IBI) response
 *  0b00..ACK: Acknowledge. A mandatory byte (or not) is decided by the Master In-band Interrupt Registry and
 *        Rules Register (MIBIRULES). To limit the maximum number of IBI bytes, configure the Read Termination field
 *        (MCTRL.RDTERM).
 *  0b01..NACK: Not acknowledge
 *  0b10..ACK_WITH_MANDATORY: Acknowledge with mandatory byte (ignores the MIBIRULES register). Acknowledge with
 *        mandatory byte should not be used, unless only slaves with a mandatory byte can cause an In-Band Interrupt.
 *  0b11..MANUAL: stop and wait for a decision using the IBIAckNack request
 */
#define I3C_MCTRL_IBIRESP(x)                     (((uint32_t)(((uint32_t)(x)) << I3C_MCTRL_IBIRESP_SHIFT)) & I3C_MCTRL_IBIRESP_MASK)
#define I3C_MCTRL_DIR_MASK                       (0x100U)
#define I3C_MCTRL_DIR_SHIFT                      (8U)
/* DIR - DIR
 *  0b0..DIRWRITE: Write
 *  0b1..DIRREAD: Read
 */
#define I3C_MCTRL_DIR(x)                         (((uint32_t)(((uint32_t)(x)) << I3C_MCTRL_DIR_SHIFT)) & I3C_MCTRL_DIR_MASK)
#define I3C_MCTRL_ADDR_MASK                      (0xFE00U)
#define I3C_MCTRL_ADDR_SHIFT                     (9U)
#define I3C_MCTRL_ADDR(x)                        (((uint32_t)(((uint32_t)(x)) << I3C_MCTRL_ADDR_SHIFT)) & I3C_MCTRL_ADDR_MASK)
#define I3C_MCTRL_RDTERM_MASK                    (0xFF0000U)
#define I3C_MCTRL_RDTERM_SHIFT                   (16U)
#define I3C_MCTRL_RDTERM(x)                      (((uint32_t)(((uint32_t)(x)) << I3C_MCTRL_RDTERM_SHIFT)) & I3C_MCTRL_RDTERM_MASK)

/* MSTATUS - Master Status Register */
#define I3C_MSTATUS_STATE_MASK                   (0x7U)
#define I3C_MSTATUS_STATE_SHIFT                  (0U)
/* STATE - State of the master
 *  0b000..IDLE: the bus has STOPped.
 *  0b001..SLVREQ: (Slave Request state) the bus has STOPped but a slave is holding SDA low. If using auto-emit
 *         IBI (MCTRL.AutoIBI), then the master will not stay in the Slave Request state.
 *  0b010..MSGSDR: in Single Data Rate (SDR) Message state (from using MWMSG_SDR)
 *  0b011..NORMACT: normal active Single Data Rate (SDR) state (from using MCTRL and MWDATAn and MRDATAn
 *         registers). The master will stay in the NORMACT state until a STOP is issued.
 *  0b100..MSGDDR: in Double Data Rate (DDR) Message mode (from using MWMSG_DDR or using the normal method with
 *         DDR). The master will stay in the DDR state, until the master exits using EXIT (emits the Exit pattern).
 *  0b101..DAA: in Enter Dynamic Address Assignment (ENTDAA) mode
 *  0b110..IBIACK: waiting for an In-Band Interrupt (IBI) ACK/NACK decision
 *  0b111..IBIRCV: Receiving an In-Band Interrupt (IBI); this IBIRCV state is used after IBI/MR/HJ has won the
 *         arbitration, and IBIRCV state is also used for IBI mandatory byte (if any) and any bytes that follow.
 */
#define I3C_MSTATUS_STATE(x)                     (((uint32_t)(((uint32_t)(x)) << I3C_MSTATUS_STATE_SHIFT)) & I3C_MSTATUS_STATE_MASK)
#define I3C_MSTATUS_BETWEEN_MASK                 (0x10U)
#define I3C_MSTATUS_BETWEEN_SHIFT                (4U)
#define I3C_MSTATUS_BETWEEN(x)                   (((uint32_t)(((uint32_t)(x)) << I3C_MSTATUS_BETWEEN_SHIFT)) & I3C_MSTATUS_BETWEEN_MASK)
#define I3C_MSTATUS_NACKED_MASK                  (0x20U)
#define I3C_MSTATUS_NACKED_SHIFT                 (5U)
#define I3C_MSTATUS_NACKED(x)                    (((uint32_t)(((uint32_t)(x)) << I3C_MSTATUS_NACKED_SHIFT)) & I3C_MSTATUS_NACKED_MASK)
#define I3C_MSTATUS_IBITYPE_MASK                 (0xC0U)
#define I3C_MSTATUS_IBITYPE_SHIFT                (6U)
/* IBITYPE - In-Band Interrupt (IBI) type
 *  0b00..NONE: cleared when IBI Won bit (MSTATUS.IBIWON) is cleared
 *  0b01..IBI: In-Band Interrupt
 *  0b10..MR: Master Request
 *  0b11..HJ: Hot-Join
 */
#define I3C_MSTATUS_IBITYPE(x)                   (((uint32_t)(((uint32_t)(x)) << I3C_MSTATUS_IBITYPE_SHIFT)) & I3C_MSTATUS_IBITYPE_MASK)
#define I3C_MSTATUS_SLVSTART_MASK                (0x100U)
#define I3C_MSTATUS_SLVSTART_SHIFT               (8U)
#define I3C_MSTATUS_SLVSTART(x)                  (((uint32_t)(((uint32_t)(x)) << I3C_MSTATUS_SLVSTART_SHIFT)) & I3C_MSTATUS_SLVSTART_MASK)
#define I3C_MSTATUS_MCTRLDONE_MASK               (0x200U)
#define I3C_MSTATUS_MCTRLDONE_SHIFT              (9U)
#define I3C_MSTATUS_MCTRLDONE(x)                 (((uint32_t)(((uint32_t)(x)) << I3C_MSTATUS_MCTRLDONE_SHIFT)) & I3C_MSTATUS_MCTRLDONE_MASK)
#define I3C_MSTATUS_COMPLETE_MASK                (0x400U)
#define I3C_MSTATUS_COMPLETE_SHIFT               (10U)
#define I3C_MSTATUS_COMPLETE(x)                  (((uint32_t)(((uint32_t)(x)) << I3C_MSTATUS_COMPLETE_SHIFT)) & I3C_MSTATUS_COMPLETE_MASK)
#define I3C_MSTATUS_RXPEND_MASK                  (0x800U)
#define I3C_MSTATUS_RXPEND_SHIFT                 (11U)
#define I3C_MSTATUS_RXPEND(x)                    (((uint32_t)(((uint32_t)(x)) << I3C_MSTATUS_RXPEND_SHIFT)) & I3C_MSTATUS_RXPEND_MASK)
#define I3C_MSTATUS_TXNOTFULL_MASK               (0x1000U)
#define I3C_MSTATUS_TXNOTFULL_SHIFT              (12U)
#define I3C_MSTATUS_TXNOTFULL(x)                 (((uint32_t)(((uint32_t)(x)) << I3C_MSTATUS_TXNOTFULL_SHIFT)) & I3C_MSTATUS_TXNOTFULL_MASK)
#define I3C_MSTATUS_IBIWON_MASK                  (0x2000U)
#define I3C_MSTATUS_IBIWON_SHIFT                 (13U)
#define I3C_MSTATUS_IBIWON(x)                    (((uint32_t)(((uint32_t)(x)) << I3C_MSTATUS_IBIWON_SHIFT)) & I3C_MSTATUS_IBIWON_MASK)
#define I3C_MSTATUS_ERRWARN_MASK                 (0x8000U)
#define I3C_MSTATUS_ERRWARN_SHIFT                (15U)
#define I3C_MSTATUS_ERRWARN(x)                   (((uint32_t)(((uint32_t)(x)) << I3C_MSTATUS_ERRWARN_SHIFT)) & I3C_MSTATUS_ERRWARN_MASK)
#define I3C_MSTATUS_NOWMASTER_MASK               (0x80000U)
#define I3C_MSTATUS_NOWMASTER_SHIFT              (19U)
#define I3C_MSTATUS_NOWMASTER(x)                 (((uint32_t)(((uint32_t)(x)) << I3C_MSTATUS_NOWMASTER_SHIFT)) & I3C_MSTATUS_NOWMASTER_MASK)
#define I3C_MSTATUS_IBIADDR_MASK                 (0x7F000000U)
#define I3C_MSTATUS_IBIADDR_SHIFT                (24U)
#define I3C_MSTATUS_IBIADDR(x)                   (((uint32_t)(((uint32_t)(x)) << I3C_MSTATUS_IBIADDR_SHIFT)) & I3C_MSTATUS_IBIADDR_MASK)

/* MIBIRULES - Master In-band Interrupt Registry and Rules Register */
#define I3C_MIBIRULES_ADDR0_MASK                 (0x3FU)
#define I3C_MIBIRULES_ADDR0_SHIFT                (0U)
#define I3C_MIBIRULES_ADDR0(x)                   (((uint32_t)(((uint32_t)(x)) << I3C_MIBIRULES_ADDR0_SHIFT)) & I3C_MIBIRULES_ADDR0_MASK)
#define I3C_MIBIRULES_ADDR1_MASK                 (0xFC0U)
#define I3C_MIBIRULES_ADDR1_SHIFT                (6U)
#define I3C_MIBIRULES_ADDR1(x)                   (((uint32_t)(((uint32_t)(x)) << I3C_MIBIRULES_ADDR1_SHIFT)) & I3C_MIBIRULES_ADDR1_MASK)
#define I3C_MIBIRULES_ADDR2_MASK                 (0x3F000U)
#define I3C_MIBIRULES_ADDR2_SHIFT                (12U)
#define I3C_MIBIRULES_ADDR2(x)                   (((uint32_t)(((uint32_t)(x)) << I3C_MIBIRULES_ADDR2_SHIFT)) & I3C_MIBIRULES_ADDR2_MASK)
#define I3C_MIBIRULES_ADDR3_MASK                 (0xFC0000U)
#define I3C_MIBIRULES_ADDR3_SHIFT                (18U)
#define I3C_MIBIRULES_ADDR3(x)                   (((uint32_t)(((uint32_t)(x)) << I3C_MIBIRULES_ADDR3_SHIFT)) & I3C_MIBIRULES_ADDR3_MASK)
#define I3C_MIBIRULES_ADDR4_MASK                 (0x3F000000U)
#define I3C_MIBIRULES_ADDR4_SHIFT                (24U)
#define I3C_MIBIRULES_ADDR4(x)                   (((uint32_t)(((uint32_t)(x)) << I3C_MIBIRULES_ADDR4_SHIFT)) & I3C_MIBIRULES_ADDR4_MASK)
#define I3C_MIBIRULES_MSB0_MASK                  (0x40000000U)
#define I3C_MIBIRULES_MSB0_SHIFT                 (30U)
#define I3C_MIBIRULES_MSB0(x)                    (((uint32_t)(((uint32_t)(x)) << I3C_MIBIRULES_MSB0_SHIFT)) & I3C_MIBIRULES_MSB0_MASK)
#define I3C_MIBIRULES_NOBYTE_MASK                (0x80000000U)
#define I3C_MIBIRULES_NOBYTE_SHIFT               (31U)
#define I3C_MIBIRULES_NOBYTE(x)                  (((uint32_t)(((uint32_t)(x)) << I3C_MIBIRULES_NOBYTE_SHIFT)) & I3C_MIBIRULES_NOBYTE_MASK)

/* MINTSET - Master Interrupt Set Register */
#define I3C_MINTSET_SLVSTART_MASK                (0x100U)
#define I3C_MINTSET_SLVSTART_SHIFT               (8U)
#define I3C_MINTSET_SLVSTART(x)                  (((uint32_t)(((uint32_t)(x)) << I3C_MINTSET_SLVSTART_SHIFT)) & I3C_MINTSET_SLVSTART_MASK)
#define I3C_MINTSET_MCTRLDONE_MASK               (0x200U)
#define I3C_MINTSET_MCTRLDONE_SHIFT              (9U)
#define I3C_MINTSET_MCTRLDONE(x)                 (((uint32_t)(((uint32_t)(x)) << I3C_MINTSET_MCTRLDONE_SHIFT)) & I3C_MINTSET_MCTRLDONE_MASK)
#define I3C_MINTSET_COMPLETE_MASK                (0x400U)
#define I3C_MINTSET_COMPLETE_SHIFT               (10U)
#define I3C_MINTSET_COMPLETE(x)                  (((uint32_t)(((uint32_t)(x)) << I3C_MINTSET_COMPLETE_SHIFT)) & I3C_MINTSET_COMPLETE_MASK)
#define I3C_MINTSET_RXPEND_MASK                  (0x800U)
#define I3C_MINTSET_RXPEND_SHIFT                 (11U)
#define I3C_MINTSET_RXPEND(x)                    (((uint32_t)(((uint32_t)(x)) << I3C_MINTSET_RXPEND_SHIFT)) & I3C_MINTSET_RXPEND_MASK)
#define I3C_MINTSET_TXNOTFULL_MASK               (0x1000U)
#define I3C_MINTSET_TXNOTFULL_SHIFT              (12U)
#define I3C_MINTSET_TXNOTFULL(x)                 (((uint32_t)(((uint32_t)(x)) << I3C_MINTSET_TXNOTFULL_SHIFT)) & I3C_MINTSET_TXNOTFULL_MASK)
#define I3C_MINTSET_IBIWON_MASK                  (0x2000U)
#define I3C_MINTSET_IBIWON_SHIFT                 (13U)
#define I3C_MINTSET_IBIWON(x)                    (((uint32_t)(((uint32_t)(x)) << I3C_MINTSET_IBIWON_SHIFT)) & I3C_MINTSET_IBIWON_MASK)
#define I3C_MINTSET_ERRWARN_MASK                 (0x8000U)
#define I3C_MINTSET_ERRWARN_SHIFT                (15U)
#define I3C_MINTSET_ERRWARN(x)                   (((uint32_t)(((uint32_t)(x)) << I3C_MINTSET_ERRWARN_SHIFT)) & I3C_MINTSET_ERRWARN_MASK)
#define I3C_MINTSET_NOWMASTER_MASK               (0x80000U)
#define I3C_MINTSET_NOWMASTER_SHIFT              (19U)
#define I3C_MINTSET_NOWMASTER(x)                 (((uint32_t)(((uint32_t)(x)) << I3C_MINTSET_NOWMASTER_SHIFT)) & I3C_MINTSET_NOWMASTER_MASK)

/* MINTCLR - Master Interrupt Clear Register */
#define I3C_MINTCLR_SLVSTART_MASK                (0x100U)
#define I3C_MINTCLR_SLVSTART_SHIFT               (8U)
#define I3C_MINTCLR_SLVSTART(x)                  (((uint32_t)(((uint32_t)(x)) << I3C_MINTCLR_SLVSTART_SHIFT)) & I3C_MINTCLR_SLVSTART_MASK)
#define I3C_MINTCLR_MCTRLDONE_MASK               (0x200U)
#define I3C_MINTCLR_MCTRLDONE_SHIFT              (9U)
#define I3C_MINTCLR_MCTRLDONE(x)                 (((uint32_t)(((uint32_t)(x)) << I3C_MINTCLR_MCTRLDONE_SHIFT)) & I3C_MINTCLR_MCTRLDONE_MASK)
#define I3C_MINTCLR_COMPLETE_MASK                (0x400U)
#define I3C_MINTCLR_COMPLETE_SHIFT               (10U)
#define I3C_MINTCLR_COMPLETE(x)                  (((uint32_t)(((uint32_t)(x)) << I3C_MINTCLR_COMPLETE_SHIFT)) & I3C_MINTCLR_COMPLETE_MASK)
#define I3C_MINTCLR_RXPEND_MASK                  (0x800U)
#define I3C_MINTCLR_RXPEND_SHIFT                 (11U)
#define I3C_MINTCLR_RXPEND(x)                    (((uint32_t)(((uint32_t)(x)) << I3C_MINTCLR_RXPEND_SHIFT)) & I3C_MINTCLR_RXPEND_MASK)
#define I3C_MINTCLR_TXNOTFULL_MASK               (0x1000U)
#define I3C_MINTCLR_TXNOTFULL_SHIFT              (12U)
#define I3C_MINTCLR_TXNOTFULL(x)                 (((uint32_t)(((uint32_t)(x)) << I3C_MINTCLR_TXNOTFULL_SHIFT)) & I3C_MINTCLR_TXNOTFULL_MASK)
#define I3C_MINTCLR_IBIWON_MASK                  (0x2000U)
#define I3C_MINTCLR_IBIWON_SHIFT                 (13U)
#define I3C_MINTCLR_IBIWON(x)                    (((uint32_t)(((uint32_t)(x)) << I3C_MINTCLR_IBIWON_SHIFT)) & I3C_MINTCLR_IBIWON_MASK)
#define I3C_MINTCLR_ERRWARN_MASK                 (0x8000U)
#define I3C_MINTCLR_ERRWARN_SHIFT                (15U)
#define I3C_MINTCLR_ERRWARN(x)                   (((uint32_t)(((uint32_t)(x)) << I3C_MINTCLR_ERRWARN_SHIFT)) & I3C_MINTCLR_ERRWARN_MASK)
#define I3C_MINTCLR_NOWMASTER_MASK               (0x80000U)
#define I3C_MINTCLR_NOWMASTER_SHIFT              (19U)
#define I3C_MINTCLR_NOWMASTER(x)                 (((uint32_t)(((uint32_t)(x)) << I3C_MINTCLR_NOWMASTER_SHIFT)) & I3C_MINTCLR_NOWMASTER_MASK)

/* MINTMASKED - Master Interrupt Mask Register */
#define I3C_MINTMASKED_SLVSTART_MASK             (0x100U)
#define I3C_MINTMASKED_SLVSTART_SHIFT            (8U)
#define I3C_MINTMASKED_SLVSTART(x)               (((uint32_t)(((uint32_t)(x)) << I3C_MINTMASKED_SLVSTART_SHIFT)) & I3C_MINTMASKED_SLVSTART_MASK)
#define I3C_MINTMASKED_MCTRLDONE_MASK            (0x200U)
#define I3C_MINTMASKED_MCTRLDONE_SHIFT           (9U)
#define I3C_MINTMASKED_MCTRLDONE(x)              (((uint32_t)(((uint32_t)(x)) << I3C_MINTMASKED_MCTRLDONE_SHIFT)) & I3C_MINTMASKED_MCTRLDONE_MASK)
#define I3C_MINTMASKED_COMPLETE_MASK             (0x400U)
#define I3C_MINTMASKED_COMPLETE_SHIFT            (10U)
#define I3C_MINTMASKED_COMPLETE(x)               (((uint32_t)(((uint32_t)(x)) << I3C_MINTMASKED_COMPLETE_SHIFT)) & I3C_MINTMASKED_COMPLETE_MASK)
#define I3C_MINTMASKED_RXPEND_MASK               (0x800U)
#define I3C_MINTMASKED_RXPEND_SHIFT              (11U)
#define I3C_MINTMASKED_RXPEND(x)                 (((uint32_t)(((uint32_t)(x)) << I3C_MINTMASKED_RXPEND_SHIFT)) & I3C_MINTMASKED_RXPEND_MASK)
#define I3C_MINTMASKED_TXNOTFULL_MASK            (0x1000U)
#define I3C_MINTMASKED_TXNOTFULL_SHIFT           (12U)
#define I3C_MINTMASKED_TXNOTFULL(x)              (((uint32_t)(((uint32_t)(x)) << I3C_MINTMASKED_TXNOTFULL_SHIFT)) & I3C_MINTMASKED_TXNOTFULL_MASK)
#define I3C_MINTMASKED_IBIWON_MASK               (0x2000U)
#define I3C_MINTMASKED_IBIWON_SHIFT              (13U)
#define I3C_MINTMASKED_IBIWON(x)                 (((uint32_t)(((uint32_t)(x)) << I3C_MINTMASKED_IBIWON_SHIFT)) & I3C_MINTMASKED_IBIWON_MASK)
#define I3C_MINTMASKED_ERRWARN_MASK              (0x8000U)
#define I3C_MINTMASKED_ERRWARN_SHIFT             (15U)
#define I3C_MINTMASKED_ERRWARN(x)                (((uint32_t)(((uint32_t)(x)) << I3C_MINTMASKED_ERRWARN_SHIFT)) & I3C_MINTMASKED_ERRWARN_MASK)
#define I3C_MINTMASKED_NOWMASTER_MASK            (0x80000U)
#define I3C_MINTMASKED_NOWMASTER_SHIFT           (19U)
#define I3C_MINTMASKED_NOWMASTER(x)              (((uint32_t)(((uint32_t)(x)) << I3C_MINTMASKED_NOWMASTER_SHIFT)) & I3C_MINTMASKED_NOWMASTER_MASK)

/* MERRWARN - Master Errors and Warnings Register */
#define I3C_MERRWARN_NACK_MASK                   (0x4U)
#define I3C_MERRWARN_NACK_SHIFT                  (2U)
#define I3C_MERRWARN_NACK(x)                     (((uint32_t)(((uint32_t)(x)) << I3C_MERRWARN_NACK_SHIFT)) & I3C_MERRWARN_NACK_MASK)
#define I3C_MERRWARN_WRABT_MASK                  (0x8U)
#define I3C_MERRWARN_WRABT_SHIFT                 (3U)
#define I3C_MERRWARN_WRABT(x)                    (((uint32_t)(((uint32_t)(x)) << I3C_MERRWARN_WRABT_SHIFT)) & I3C_MERRWARN_WRABT_MASK)
#define I3C_MERRWARN_TERM_MASK                   (0x10U)
#define I3C_MERRWARN_TERM_SHIFT                  (4U)
#define I3C_MERRWARN_TERM(x)                     (((uint32_t)(((uint32_t)(x)) << I3C_MERRWARN_TERM_SHIFT)) & I3C_MERRWARN_TERM_MASK)
#define I3C_MERRWARN_HPAR_MASK                   (0x200U)
#define I3C_MERRWARN_HPAR_SHIFT                  (9U)
#define I3C_MERRWARN_HPAR(x)                     (((uint32_t)(((uint32_t)(x)) << I3C_MERRWARN_HPAR_SHIFT)) & I3C_MERRWARN_HPAR_MASK)
#define I3C_MERRWARN_HCRC_MASK                   (0x400U)
#define I3C_MERRWARN_HCRC_SHIFT                  (10U)
#define I3C_MERRWARN_HCRC(x)                     (((uint32_t)(((uint32_t)(x)) << I3C_MERRWARN_HCRC_SHIFT)) & I3C_MERRWARN_HCRC_MASK)
#define I3C_MERRWARN_OREAD_MASK                  (0x10000U)
#define I3C_MERRWARN_OREAD_SHIFT                 (16U)
#define I3C_MERRWARN_OREAD(x)                    (((uint32_t)(((uint32_t)(x)) << I3C_MERRWARN_OREAD_SHIFT)) & I3C_MERRWARN_OREAD_MASK)
#define I3C_MERRWARN_OWRITE_MASK                 (0x20000U)
#define I3C_MERRWARN_OWRITE_SHIFT                (17U)
#define I3C_MERRWARN_OWRITE(x)                   (((uint32_t)(((uint32_t)(x)) << I3C_MERRWARN_OWRITE_SHIFT)) & I3C_MERRWARN_OWRITE_MASK)
#define I3C_MERRWARN_MSGERR_MASK                 (0x40000U)
#define I3C_MERRWARN_MSGERR_SHIFT                (18U)
#define I3C_MERRWARN_MSGERR(x)                   (((uint32_t)(((uint32_t)(x)) << I3C_MERRWARN_MSGERR_SHIFT)) & I3C_MERRWARN_MSGERR_MASK)
#define I3C_MERRWARN_INVREQ_MASK                 (0x80000U)
#define I3C_MERRWARN_INVREQ_SHIFT                (19U)
#define I3C_MERRWARN_INVREQ(x)                   (((uint32_t)(((uint32_t)(x)) << I3C_MERRWARN_INVREQ_SHIFT)) & I3C_MERRWARN_INVREQ_MASK)
#define I3C_MERRWARN_TIMEOUT_MASK                (0x100000U)
#define I3C_MERRWARN_TIMEOUT_SHIFT               (20U)
#define I3C_MERRWARN_TIMEOUT(x)                  (((uint32_t)(((uint32_t)(x)) << I3C_MERRWARN_TIMEOUT_SHIFT)) & I3C_MERRWARN_TIMEOUT_MASK)

/* MDMACTRL - Master DMA Control Register */
#define I3C_MDMACTRL_DMAFB_MASK                  (0x3U)
#define I3C_MDMACTRL_DMAFB_SHIFT                 (0U)
/* DMAFB - DMA from bus
 *  0b00..NOT_USED: DMA is not used
 *  0b01..ENABLE_ONE_FRAME: DMA is enabled for 1 frame. DMAFB auto-clears on STOP or repeated START. See MCONFIG.MATCHSS.
 *  0b10..ENABLE: DMA is enabled until the DMA is turned off.
 */
#define I3C_MDMACTRL_DMAFB(x)                    (((uint32_t)(((uint32_t)(x)) << I3C_MDMACTRL_DMAFB_SHIFT)) & I3C_MDMACTRL_DMAFB_MASK)
#define I3C_MDMACTRL_DMATB_MASK                  (0xCU)
#define I3C_MDMACTRL_DMATB_SHIFT                 (2U)
/* DMATB - DMA to bus
 *  0b00..NOT_USED: DMA is not used
 *  0b01..ENABLE_ONE_FRAME: DMA is enabled for 1 frame (ended by DMA or Terminated). DMATB auto-clears on STOP or START. See MCONFIG.MATCHSS.
 *  0b10..ENABLE: DMA is enabled until DMA is turned off. Normally DMA ENABLE should only be used in Master Message mode.
 */
#define I3C_MDMACTRL_DMATB(x)                    (((uint32_t)(((uint32_t)(x)) << I3C_MDMACTRL_DMATB_SHIFT)) & I3C_MDMACTRL_DMATB_MASK)
#define I3C_MDMACTRL_DMAWIDTH_MASK               (0x30U)
#define I3C_MDMACTRL_DMAWIDTH_SHIFT              (4U)
/* DMAWIDTH - DMA width
 *  0b00..BYTE
 *  0b01..BYTE_AGAIN
 *  0b10..HALF_WORD: Half-word (16 bits). This will make sure that 2 bytes are free/available in FIFO.
 *  0b11..RESERVED
 */
#define I3C_MDMACTRL_DMAWIDTH(x)                 (((uint32_t)(((uint32_t)(x)) << I3C_MDMACTRL_DMAWIDTH_SHIFT)) & I3C_MDMACTRL_DMAWIDTH_MASK)

/* MDATACTRL - Master Data Control Register */
#define I3C_MDATACTRL_FLUSHTB_MASK               (0x1U)
#define I3C_MDATACTRL_FLUSHTB_SHIFT              (0U)
#define I3C_MDATACTRL_FLUSHTB(x)                 (((uint32_t)(((uint32_t)(x)) << I3C_MDATACTRL_FLUSHTB_SHIFT)) & I3C_MDATACTRL_FLUSHTB_MASK)
#define I3C_MDATACTRL_FLUSHFB_MASK               (0x2U)
#define I3C_MDATACTRL_FLUSHFB_SHIFT              (1U)
#define I3C_MDATACTRL_FLUSHFB(x)                 (((uint32_t)(((uint32_t)(x)) << I3C_MDATACTRL_FLUSHFB_SHIFT)) & I3C_MDATACTRL_FLUSHFB_MASK)
#define I3C_MDATACTRL_UNLOCK_MASK                (0x4U)
#define I3C_MDATACTRL_UNLOCK_SHIFT               (2U)
#define I3C_MDATACTRL_UNLOCK(x)                  (((uint32_t)(((uint32_t)(x)) << I3C_MDATACTRL_UNLOCK_SHIFT)) & I3C_MDATACTRL_UNLOCK_MASK)
#define I3C_MDATACTRL_TXTRIG_MASK                (0x30U)
#define I3C_MDATACTRL_TXTRIG_SHIFT               (4U)
#define I3C_MDATACTRL_TXTRIG(x)                  (((uint32_t)(((uint32_t)(x)) << I3C_MDATACTRL_TXTRIG_SHIFT)) & I3C_MDATACTRL_TXTRIG_MASK)
#define I3C_MDATACTRL_RXTRIG_MASK                (0xC0U)
#define I3C_MDATACTRL_RXTRIG_SHIFT               (6U)
#define I3C_MDATACTRL_RXTRIG(x)                  (((uint32_t)(((uint32_t)(x)) << I3C_MDATACTRL_RXTRIG_SHIFT)) & I3C_MDATACTRL_RXTRIG_MASK)
#define I3C_MDATACTRL_TXCOUNT_MASK               (0x1F0000U)
#define I3C_MDATACTRL_TXCOUNT_SHIFT              (16U)
#define I3C_MDATACTRL_TXCOUNT(x)                 (((uint32_t)(((uint32_t)(x)) << I3C_MDATACTRL_TXCOUNT_SHIFT)) & I3C_MDATACTRL_TXCOUNT_MASK)
#define I3C_MDATACTRL_RXCOUNT_MASK               (0x1F000000U)
#define I3C_MDATACTRL_RXCOUNT_SHIFT              (24U)
#define I3C_MDATACTRL_RXCOUNT(x)                 (((uint32_t)(((uint32_t)(x)) << I3C_MDATACTRL_RXCOUNT_SHIFT)) & I3C_MDATACTRL_RXCOUNT_MASK)
#define I3C_MDATACTRL_TXFULL_MASK                (0x40000000U)
#define I3C_MDATACTRL_TXFULL_SHIFT               (30U)
#define I3C_MDATACTRL_TXFULL(x)                  (((uint32_t)(((uint32_t)(x)) << I3C_MDATACTRL_TXFULL_SHIFT)) & I3C_MDATACTRL_TXFULL_MASK)
#define I3C_MDATACTRL_RXEMPTY_MASK               (0x80000000U)
#define I3C_MDATACTRL_RXEMPTY_SHIFT              (31U)
#define I3C_MDATACTRL_RXEMPTY(x)                 (((uint32_t)(((uint32_t)(x)) << I3C_MDATACTRL_RXEMPTY_SHIFT)) & I3C_MDATACTRL_RXEMPTY_MASK)

/* MWDATAB - Master Write Data Byte Register */
#define I3C_MWDATAB_VALUE_MASK                   (0xFFU)
#define I3C_MWDATAB_VALUE_SHIFT                  (0U)
#define I3C_MWDATAB_VALUE(x)                     (((uint32_t)(((uint32_t)(x)) << I3C_MWDATAB_VALUE_SHIFT)) & I3C_MWDATAB_VALUE_MASK)
#define I3C_MWDATAB_END_MASK                     (0x100U)
#define I3C_MWDATAB_END_SHIFT                    (8U)
#define I3C_MWDATAB_END(x)                       (((uint32_t)(((uint32_t)(x)) << I3C_MWDATAB_END_SHIFT)) & I3C_MWDATAB_END_MASK)
#define I3C_MWDATAB_END_ALSO_MASK                (0x10000U)
#define I3C_MWDATAB_END_ALSO_SHIFT               (16U)
#define I3C_MWDATAB_END_ALSO(x)                  (((uint32_t)(((uint32_t)(x)) << I3C_MWDATAB_END_ALSO_SHIFT)) & I3C_MWDATAB_END_ALSO_MASK)

/* MWDATABE - Master Write Data Byte End Register */
#define I3C_MWDATABE_VALUE_MASK                  (0xFFU)
#define I3C_MWDATABE_VALUE_SHIFT                 (0U)
#define I3C_MWDATABE_VALUE(x)                    (((uint32_t)(((uint32_t)(x)) << I3C_MWDATABE_VALUE_SHIFT)) & I3C_MWDATABE_VALUE_MASK)

/* MWDATAH - Master Write Data Half-word Register */
#define I3C_MWDATAH_DATA0_MASK                   (0xFFU)
#define I3C_MWDATAH_DATA0_SHIFT                  (0U)
#define I3C_MWDATAH_DATA0(x)                     (((uint32_t)(((uint32_t)(x)) << I3C_MWDATAH_DATA0_SHIFT)) & I3C_MWDATAH_DATA0_MASK)
#define I3C_MWDATAH_DATA1_MASK                   (0xFF00U)
#define I3C_MWDATAH_DATA1_SHIFT                  (8U)
#define I3C_MWDATAH_DATA1(x)                     (((uint32_t)(((uint32_t)(x)) << I3C_MWDATAH_DATA1_SHIFT)) & I3C_MWDATAH_DATA1_MASK)
#define I3C_MWDATAH_END_MASK                     (0x10000U)
#define I3C_MWDATAH_END_SHIFT                    (16U)
#define I3C_MWDATAH_END(x)                       (((uint32_t)(((uint32_t)(x)) << I3C_MWDATAH_END_SHIFT)) & I3C_MWDATAH_END_MASK)

/* MWDATAHE - Master Write Data Byte End Register */
#define I3C_MWDATAHE_DATA0_MASK                  (0xFFU)
#define I3C_MWDATAHE_DATA0_SHIFT                 (0U)
#define I3C_MWDATAHE_DATA0(x)                    (((uint32_t)(((uint32_t)(x)) << I3C_MWDATAHE_DATA0_SHIFT)) & I3C_MWDATAHE_DATA0_MASK)
#define I3C_MWDATAHE_DATA1_MASK                  (0xFF00U)
#define I3C_MWDATAHE_DATA1_SHIFT                 (8U)
#define I3C_MWDATAHE_DATA1(x)                    (((uint32_t)(((uint32_t)(x)) << I3C_MWDATAHE_DATA1_SHIFT)) & I3C_MWDATAHE_DATA1_MASK)

/* MRDATAB - Master Read Data Byte Register */
#define I3C_MRDATAB_VALUE_MASK                   (0xFFU)
#define I3C_MRDATAB_VALUE_SHIFT                  (0U)
#define I3C_MRDATAB_VALUE(x)                     (((uint32_t)(((uint32_t)(x)) << I3C_MRDATAB_VALUE_SHIFT)) & I3C_MRDATAB_VALUE_MASK)

/* MRDATAH - Master Read Data Half-word Register */
#define I3C_MRDATAH_LSB_MASK                     (0xFFU)
#define I3C_MRDATAH_LSB_SHIFT                    (0U)
#define I3C_MRDATAH_LSB(x)                       (((uint32_t)(((uint32_t)(x)) << I3C_MRDATAH_LSB_SHIFT)) & I3C_MRDATAH_LSB_MASK)
#define I3C_MRDATAH_MSB_MASK                     (0xFF00U)
#define I3C_MRDATAH_MSB_SHIFT                    (8U)
#define I3C_MRDATAH_MSB(x)                       (((uint32_t)(((uint32_t)(x)) << I3C_MRDATAH_MSB_SHIFT)) & I3C_MRDATAH_MSB_MASK)

/* MWMSG_SDR_CONTROL - Master Write Message in SDR mode */
#define I3C_MWMSG_SDR_CONTROL_DIR_MASK           (0x1U)
#define I3C_MWMSG_SDR_CONTROL_DIR_SHIFT          (0U)
/* DIR - Direction
 *  0b0..Write
 *  0b1..Read
 */
#define I3C_MWMSG_SDR_CONTROL_DIR(x)             (((uint32_t)(((uint32_t)(x)) << I3C_MWMSG_SDR_CONTROL_DIR_SHIFT)) & I3C_MWMSG_SDR_CONTROL_DIR_MASK)
#define I3C_MWMSG_SDR_CONTROL_ADDR_MASK          (0xFEU)
#define I3C_MWMSG_SDR_CONTROL_ADDR_SHIFT         (1U)
#define I3C_MWMSG_SDR_CONTROL_ADDR(x)            (((uint32_t)(((uint32_t)(x)) << I3C_MWMSG_SDR_CONTROL_ADDR_SHIFT)) & I3C_MWMSG_SDR_CONTROL_ADDR_MASK)
#define I3C_MWMSG_SDR_CONTROL_END_MASK           (0x100U)
#define I3C_MWMSG_SDR_CONTROL_END_SHIFT          (8U)
#define I3C_MWMSG_SDR_CONTROL_END(x)             (((uint32_t)(((uint32_t)(x)) << I3C_MWMSG_SDR_CONTROL_END_SHIFT)) & I3C_MWMSG_SDR_CONTROL_END_MASK)
#define I3C_MWMSG_SDR_CONTROL_I2C_MASK           (0x400U)
#define I3C_MWMSG_SDR_CONTROL_I2C_SHIFT          (10U)
/* I2C - I2C
 *  0b0..I3C message
 *  0b1..I2C message
 */
#define I3C_MWMSG_SDR_CONTROL_I2C(x)             (((uint32_t)(((uint32_t)(x)) << I3C_MWMSG_SDR_CONTROL_I2C_SHIFT)) & I3C_MWMSG_SDR_CONTROL_I2C_MASK)
#define I3C_MWMSG_SDR_CONTROL_LEN_MASK           (0xF800U)
#define I3C_MWMSG_SDR_CONTROL_LEN_SHIFT          (11U)
#define I3C_MWMSG_SDR_CONTROL_LEN(x)             (((uint32_t)(((uint32_t)(x)) << I3C_MWMSG_SDR_CONTROL_LEN_SHIFT)) & I3C_MWMSG_SDR_CONTROL_LEN_MASK)

/* MWMSG_SDR_DATA - Master Write Message Data in SDR mode */
#define I3C_MWMSG_SDR_DATA_DATA16B_MASK          (0xFFFFU)
#define I3C_MWMSG_SDR_DATA_DATA16B_SHIFT         (0U)
#define I3C_MWMSG_SDR_DATA_DATA16B(x)            (((uint32_t)(((uint32_t)(x)) << I3C_MWMSG_SDR_DATA_DATA16B_SHIFT)) & I3C_MWMSG_SDR_DATA_DATA16B_MASK)
#define I3C_MWMSG_SDR_DATA_END_MASK              (0x10000U)
#define I3C_MWMSG_SDR_DATA_END_SHIFT             (16U)
#define I3C_MWMSG_SDR_DATA_END(x)                (((uint32_t)(((uint32_t)(x)) << I3C_MWMSG_SDR_DATA_END_SHIFT)) & I3C_MWMSG_SDR_DATA_END_MASK)

/* MRMSG_SDR - Master Read Message in SDR mode */
#define I3C_MRMSG_SDR_DATA_MASK                  (0xFFFFU)
#define I3C_MRMSG_SDR_DATA_SHIFT                 (0U)
#define I3C_MRMSG_SDR_DATA(x)                    (((uint32_t)(((uint32_t)(x)) << I3C_MRMSG_SDR_DATA_SHIFT)) & I3C_MRMSG_SDR_DATA_MASK)

/* MWMSG_DDR_CONTROL - Master Write Message in DDR mode */
#define I3C_MWMSG_DDR_CONTROL_LEN_MASK           (0x3FFU)
#define I3C_MWMSG_DDR_CONTROL_LEN_SHIFT          (0U)
#define I3C_MWMSG_DDR_CONTROL_LEN(x)             (((uint32_t)(((uint32_t)(x)) << I3C_MWMSG_DDR_CONTROL_LEN_SHIFT)) & I3C_MWMSG_DDR_CONTROL_LEN_MASK)
#define I3C_MWMSG_DDR_CONTROL_END_MASK           (0x4000U)
#define I3C_MWMSG_DDR_CONTROL_END_SHIFT          (14U)
#define I3C_MWMSG_DDR_CONTROL_END(x)             (((uint32_t)(((uint32_t)(x)) << I3C_MWMSG_DDR_CONTROL_END_SHIFT)) & I3C_MWMSG_DDR_CONTROL_END_MASK)

/* MWMSG_DDR_DATA - Master Write Message Data in DDR mode */
#define I3C_MWMSG_DDR_DATA_DATA16B_MASK          (0xFFFFU)
#define I3C_MWMSG_DDR_DATA_DATA16B_SHIFT         (0U)
#define I3C_MWMSG_DDR_DATA_DATA16B(x)            (((uint32_t)(((uint32_t)(x)) << I3C_MWMSG_DDR_DATA_DATA16B_SHIFT)) & I3C_MWMSG_DDR_DATA_DATA16B_MASK)
#define I3C_MWMSG_DDR_DATA_END_MASK              (0x10000U)
#define I3C_MWMSG_DDR_DATA_END_SHIFT             (16U)
#define I3C_MWMSG_DDR_DATA_END(x)                (((uint32_t)(((uint32_t)(x)) << I3C_MWMSG_DDR_DATA_END_SHIFT)) & I3C_MWMSG_DDR_DATA_END_MASK)

/* MRMSG_DDR - Master Read Message in DDR mode */
#define I3C_MRMSG_DDR_DATA_MASK                  (0xFFFFU)
#define I3C_MRMSG_DDR_DATA_SHIFT                 (0U)
#define I3C_MRMSG_DDR_DATA(x)                    (((uint32_t)(((uint32_t)(x)) << I3C_MRMSG_DDR_DATA_SHIFT)) & I3C_MRMSG_DDR_DATA_MASK)
#define I3C_MRMSG_DDR_CLEN_MASK                  (0x3FF0000U)
#define I3C_MRMSG_DDR_CLEN_SHIFT                 (16U)
#define I3C_MRMSG_DDR_CLEN(x)                    (((uint32_t)(((uint32_t)(x)) << I3C_MRMSG_DDR_CLEN_SHIFT)) & I3C_MRMSG_DDR_CLEN_MASK)

/* MDYNADDR - Master Dynamic Address Register */
#define I3C_MDYNADDR_DAVALID_MASK                (0x1U)
#define I3C_MDYNADDR_DAVALID_SHIFT               (0U)
#define I3C_MDYNADDR_DAVALID(x)                  (((uint32_t)(((uint32_t)(x)) << I3C_MDYNADDR_DAVALID_SHIFT)) & I3C_MDYNADDR_DAVALID_MASK)
#define I3C_MDYNADDR_DADDR_MASK                  (0xFEU)
#define I3C_MDYNADDR_DADDR_SHIFT                 (1U)
#define I3C_MDYNADDR_DADDR(x)                    (((uint32_t)(((uint32_t)(x)) << I3C_MDYNADDR_DADDR_SHIFT)) & I3C_MDYNADDR_DADDR_MASK)

/* SID - Slave Module ID Register */
#define I3C_SID_ID_MASK                          (0xFFFFFFFFU)
#define I3C_SID_ID_SHIFT                         (0U)
#define I3C_SID_ID(x)                            (((uint32_t)(((uint32_t)(x)) << I3C_SID_ID_SHIFT)) & I3C_SID_ID_MASK)

/* end of group I3C_Register_Masks */

#endif /* __IMX_I3C_H__ */
