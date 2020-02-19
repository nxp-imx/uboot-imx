/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2016 Cadence Design Systems - http://www.cadence.com
 * Copyright 2019 NXP
 */

#ifndef __REG_USBSS_DEV_ADDR_MAP_MACRO_H__
#define __REG_USBSS_DEV_ADDR_MAP_MACRO_H__

/* macros for field CFGRST */
#define USB_CONF__CFGRST__MASK                                      0x00000001U
#define USB_CONF__CFGSET__MASK                                      0x00000002U
#define USB_CONF__USB3DIS__MASK                                     0x00000008U
#define USB_CONF__DEVEN__MASK                                       0x00004000U
#define USB_CONF__DEVDS__MASK                                       0x00008000U
#define USB_CONF__L1EN__MASK                                        0x00010000U
#define USB_CONF__L1DS__MASK                                        0x00020000U
#define USB_CONF__CLK2OFFDS__MASK                                   0x00080000U
#define USB_CONF__U1EN__MASK                                        0x01000000U
#define USB_CONF__U1DS__MASK                                        0x02000000U
#define USB_CONF__U2EN__MASK                                        0x04000000U
#define USB_CONF__U2DS__MASK                                        0x08000000U

/* macros for field CFGSTS */
#define USB_STS__CFGSTS__MASK                                       0x00000001U
#define USB_STS__USBSPEED__READ(src)     (((u32)(src) & 0x00000070U) >> 4)

/* macros for field ENDIAN_MIRROR */
#define USB_STS__LPMST__READ(src)       (((u32)(src) & 0x000c0000U) >> 18)

/* macros for field USB2CONS */
#define USB_STS__U1ENS__MASK                                        0x01000000U
#define USB_STS__U2ENS__MASK                                        0x02000000U
#define USB_STS__LST__READ(src)         (((u32)(src) & 0x3c000000U) >> 26)

/* macros for field SET_ADDR */
#define USB_CMD__SET_ADDR__MASK                                     0x00000001U
#define USB_CMD__STMODE						0x00000200U
#define USB_CMD__TMODE_SEL(x)                                    (x << 10)
#define USB_CMD__FADDR__WRITE(src)       (((u32)(src) << 1) & 0x000000feU)

/* macros for field CONIEN */
#define USB_IEN__CONIEN__MASK                                       0x00000001U
#define USB_IEN__DISIEN__MASK                                       0x00000002U
#define USB_IEN__UWRESIEN__MASK                                     0x00000004U
#define USB_IEN__UHRESIEN__MASK                                     0x00000008U
#define USB_IEN__U3EXTIEN__MASK                                     0x00000020U
#define USB_IEN__CON2IEN__MASK                                      0x00010000U
#define USB_IEN__U2RESIEN__MASK                                     0x00040000U
#define USB_IEN__L2ENTIEN__MASK                                     0x00100000U
#define USB_IEN__L2EXTIEN__MASK                                     0x00200000U

/* macros for field CONI */
#define USB_ISTS__CONI__SHIFT                                                 0
#define USB_ISTS__DISI__SHIFT                                                 1
#define USB_ISTS__UWRESI__SHIFT                                               2
#define USB_ISTS__UHRESI__SHIFT                                               3
#define USB_ISTS__U3EXTI__SHIFT                                               5
#define USB_ISTS__CON2I__SHIFT                                               16
#define USB_ISTS__DIS2I__SHIFT                                               17
#define USB_ISTS__DIS2I__MASK                                       0x00020000U
#define USB_ISTS__U2RESI__SHIFT                                              18
#define USB_ISTS__L2ENTI__SHIFT                                              20
#define USB_ISTS__L2EXTI__SHIFT                                              21

/* macros for field TRADDR */
#define EP_TRADDR__TRADDR__WRITE(src)           ((u32)(src) & 0xffffffffU)

/* macros for field ENABLE */
#define EP_CFG__ENABLE__MASK                                        0x00000001U
#define EP_CFG__EPTYPE__WRITE(src)       (((u32)(src) << 1) & 0x00000006U)
#define EP_CFG__MAXBURST__WRITE(src)     (((u32)(src) << 8) & 0x00000f00U)
#define EP_CFG__MAXPKTSIZE__WRITE(src)  (((u32)(src) << 16) & 0x07ff0000U)
#define EP_CFG__BUFFERING__WRITE(src)   (((u32)(src) << 27) & 0xf8000000U)

/* macros for field EPRST */
#define EP_CMD__EPRST__MASK                                         0x00000001U
#define EP_CMD__SSTALL__MASK                                        0x00000002U
#define EP_CMD__CSTALL__MASK                                        0x00000004U
#define EP_CMD__ERDY__MASK                                          0x00000008U
#define EP_CMD__REQ_CMPL__MASK                                      0x00000020U
#define EP_CMD__DRDY__MASK                                          0x00000040U
#define EP_CMD__DFLUSH__MASK                                        0x00000080U

/* macros for field SETUP */
#define EP_STS__SETUP__MASK                                         0x00000001U
#define EP_STS__STALL__MASK                                         0x00000002U
#define EP_STS__IOC__MASK                                           0x00000004U
#define EP_STS__ISP__MASK                                           0x00000008U
#define EP_STS__DESCMIS__MASK                                       0x00000010U
#define EP_STS__TRBERR__MASK                                        0x00000080U
#define EP_STS__NRDY__MASK                                          0x00000100U
#define EP_STS__DBUSY__MASK                                         0x00000200U
#define EP_STS__BUFFEMPTY__MASK                                     0x00000400U
#define EP_STS__OUTSMM__MASK                                        0x00004000U
#define EP_STS__ISOERR__MASK                                        0x00008000U

/* macros for field SETUPEN */
#define EP_STS_EN__SETUPEN__MASK                                    0x00000001U
#define EP_STS_EN__DESCMISEN__MASK                                  0x00000010U
#define EP_STS_EN__TRBERREN__MASK                                   0x00000080U

/* macros for field EOUTEN0 */
#define EP_IEN__EOUTEN0__MASK                                       0x00000001U
#define EP_IEN__EINEN0__MASK                                        0x00010000U

/* macros for field EOUT0 */
#define EP_ISTS__EOUT0__MASK                                        0x00000001U
#define EP_ISTS__EIN0__MASK                                         0x00010000U

/* macros for field LFPS_MIN_DET_U1_EXIT */
#define DBG_LINK1__LFPS_MIN_GEN_U1_EXIT__WRITE(src) \
			(((u32)(src)\
			<< 8) & 0x0000ff00U)
#define DBG_LINK1__LFPS_MIN_GEN_U1_EXIT_SET__MASK                   0x02000000U

#endif /* __REG_USBSS_DEV_ADDR_MAP_MACRO_H__ */
