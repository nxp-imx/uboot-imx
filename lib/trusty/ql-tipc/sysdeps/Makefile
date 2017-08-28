#
# Copyright (C) 2016 The Android Open Source Project
#
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation
# files (the "Software"), to deal in the Software without
# restriction, including without limitation the rights to use, copy,
# modify, merge, publish, distribute, sublicense, and/or sell copies
# of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
# BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
# ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

# Sample Makefile for U-boot

#ccflags-y += -DTIPC_ENABLE_DEBUG

TRUSTY_DIR = lib/trusty
#ccflags-y += -I$(TRUSTY_DIR)/ql-tipc/include
ccflags-y += -I$(TRUSTY_DIR)/interface/include

QL_TIPC = ../
obj-y += \
    $(QL_TIPC)/avb.o \
    $(QL_TIPC)/keymaster.o \
    $(QL_TIPC)/ipc.o \
    $(QL_TIPC)/ipc_dev.o \
    $(QL_TIPC)/libtipc.o \
    $(QL_TIPC)/rpmb_proxy.o \
    sysdeps_uboot.o \
    storage_ops_uboot.o

obj-$(CONFIG_ARM) += \
    $(QL_TIPC)/arch/arm/trusty_mem.o \
    $(QL_TIPC)/arch/arm/trusty_dev.o
