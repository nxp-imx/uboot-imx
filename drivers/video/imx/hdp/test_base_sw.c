/******************************************************************************
 *
 * Copyright (C) 2016-2017 Cadence Design Systems, Inc.
 * All rights reserved worldwide.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. THE SOFTWARE IS PROVIDED "AS IS",
 * WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
 * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Copyright 2017-2018 NXP
 *
 ******************************************************************************
 *
 * test_base_sw.c
 *
 ******************************************************************************
 */

#ifndef __UBOOT__
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#else
#include <common.h>
#include <asm/io.h>

#ifdef CONFIG_ARCH_IMX8M
/* mscale */
#define HDMI_BASE     0x32c00000
#define HDMI_PHY_BASE 0x32c80000
#define HDMI_SEC_BASE 0x32e40000
#endif
#ifdef CONFIG_ARCH_IMX8
/* QM */
#define HDMI_BASE 0x56268000
#define HDMI_SEC_BASE 0x56269000
#define HDMI_OFFSET_ADDR 0x56261008
#define HDMI_SEC_OFFSET_ADDR 0x5626100c

#define HDMI_RX_BASE 0x58268000
#define HDMI_RX_SEC_BASE 0x58269000
#define HDMI_RX_OFFSET_ADDR 0x58261004
#define HDMI_RX_SEC_OFFSET_ADDR 0x58261008
#endif

#endif

#ifdef CONFIG_ARCH_IMX8M
int cdn_apb_read(unsigned int addr, unsigned int *value)
{
	unsigned int temp;
	uint64_t tmp_addr = addr + HDMI_BASE;
	temp = __raw_readl(tmp_addr);
	*value = temp;
	return 0;
}

int cdn_apb_write(unsigned int addr, unsigned int value)
{
	uint64_t tmp_addr = addr + HDMI_BASE;

	__raw_writel(value, tmp_addr);
	return 0;
}

int cdn_sapb_read(unsigned int addr, unsigned int *value)
{
	unsigned int temp;
	uint64_t tmp_addr = addr + HDMI_SEC_BASE;
	temp = __raw_readl(tmp_addr);
	*value = temp;
	return 0;
}

int cdn_sapb_write(unsigned int addr, unsigned int value)
{
	uint64_t tmp_addr = addr + HDMI_SEC_BASE;
	__raw_writel(value, tmp_addr);
	return 0;
}

void cdn_sleep(uint32_t ms)
{
	mdelay(ms);
}

void cdn_usleep(uint32_t us)
{
	udelay(us);
}
#endif
#ifdef CONFIG_ARCH_IMX8
int cdn_apb_read(unsigned int addr, unsigned int *value)
{
	unsigned int temp;
	uint64_t tmp_addr = (addr & 0xfff) + HDMI_BASE;

	/* printf("%s():%d addr = 0x%08x, tmp_addr = 0x%08x, offset = 0x%08x\n",
	      __func__, __LINE__, addr, (unsigned int)tmp_addr, addr>>12); */

	__raw_writel(addr >> 12, HDMI_OFFSET_ADDR);

	temp = __raw_readl(tmp_addr);
	/* printf("%s():%d temp = 0x%08x\n", __func__, __LINE__, temp ); */

	*value = temp;
	return 0;
}

int cdn_apb_write(unsigned int addr, unsigned int value)
{
	uint64_t tmp_addr = (addr & 0xfff) + HDMI_BASE;

	/*printf("%s():%d addr=0x%08x, taddr=0x%08x, off=0x%08x, val=0x%08x\n",
	      __func__, __LINE__, addr, (unsigned int)tmp_addr,
	      addr>>12,  value);*/

	__raw_writel(addr >> 12, HDMI_OFFSET_ADDR);

	/* printf("%s():%d\n", __func__, __LINE__); */
	__raw_writel(value, tmp_addr);

	return 0;
}

int cdn_sapb_read(unsigned int addr, unsigned int *value)
{
	unsigned int temp;
	uint64_t tmp_addr = (addr & 0xfff) + HDMI_SEC_BASE;

	__raw_writel(addr >> 12, HDMI_SEC_OFFSET_ADDR);

	temp = __raw_readl(tmp_addr);
	*value = temp;
	return 0;
}

int cdn_sapb_write(unsigned int addr, unsigned int value)
{
	uint64_t tmp_addr = (addr & 0xfff) + HDMI_SEC_BASE;

	__raw_writel(addr >> 12, HDMI_SEC_OFFSET_ADDR);
	__raw_writel(value, tmp_addr);

	return 0;
}

int hdp_rx_apb_read(unsigned int addr, unsigned int *value)
{
	unsigned int temp;
	uint64_t tmp_addr = (addr & 0xfff) + HDMI_RX_BASE;

	__raw_writel(addr >> 12, HDMI_RX_OFFSET_ADDR);

	temp = __raw_readl(tmp_addr);

	*value = temp;
	return 0;
}

int hdp_rx_apb_write(unsigned int addr, unsigned int value)
{
	uint64_t tmp_addr = (addr & 0xfff) + HDMI_RX_BASE;

	__raw_writel(addr >> 12, HDMI_RX_OFFSET_ADDR);

	__raw_writel(value, tmp_addr);

	return 0;
}

int hdp_rx_sapb_read(unsigned int addr, unsigned int *value)
{
	unsigned int temp;
	uint64_t tmp_addr = (addr & 0xfff) + HDMI_RX_SEC_BASE;

	__raw_writel(addr >> 12, HDMI_RX_SEC_OFFSET_ADDR);

	temp = __raw_readl(tmp_addr);
	*value = temp;
	return 0;
}

int hdp_rx_sapb_write(unsigned int addr, unsigned int value)
{
	uint64_t tmp_addr = (addr & 0xfff) + HDMI_RX_SEC_BASE;

	__raw_writel(addr >> 12, HDMI_RX_SEC_OFFSET_ADDR);
	__raw_writel(value, tmp_addr);

	return 0;
}

void cdn_sleep(uint32_t ms)
{
	mdelay(ms);
}

void cdn_usleep(uint32_t us)
{
	udelay(us);
}
#endif

