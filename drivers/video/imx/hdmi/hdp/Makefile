#
# Copyright 2018 NXP
#
# SPDX-License-Identifier:      GPL-2.0+
#
obj-$(CONFIG_VIDEO_IMX_HDP_LOAD) += API_General.o util.o test_base_sw.o

obj-$(CONFIG_VIDEO_IMX8M_HDMI) += \
	API_General.o \
	test_base_sw.o \
	API_AVI.o \
	API_Infoframe.o \
	util.o \
	vic_table.o \
	edid_parser.o \
	API_AFE.o \
	API_HDMITX.o \
	API_AFE_t28hpc_hdmitx.o

# common objects
#obj-y += \
#	API_General.o  API_AVI.o API_Infoframe.o \
#	util.o vic_table.o test_base_sw.o \
#	avgen_drv.o edid_parser.o \
#	API_AFE.o

#DP objects
#	API_DPTX.o \
#	API_AFE_mcu2_dp.o\
#	mhdp_firmware.o

#hdmi objects
#obj-y += \
#	API_HDMITX.o \
#	API_HDCP.o \
#	API_AFE_t28hpc_hdmitx.o
#

# USE for QM
#	blob/API_AFE_mcu1_dp.o
#	blob/API_AFE_ss28fdsoi_kiran_hdmitx.o
#	blob/ss28fdsoi_hdmitx_table.o
#	blob/hdmitx_firmware.o
#	blob/mhdp_firmware.o

# Use for mscale
#	API_AFE_mcu2_dp.o ()
#	API_AFE_t28hpc_hdmitx.c
#
