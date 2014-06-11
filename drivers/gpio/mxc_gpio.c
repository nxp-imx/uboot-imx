/*
 * Copyright (C) 2009
 * Guennadi Liakhovetski, DENX Software Engineering, <lg@denx.de>
 *
 * Copyright (C) 2011
 * Stefano Babic, DENX Software Engineering, <sbabic@denx.de>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <asm/arch/imx-regs.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <errno.h>
#ifdef CONFIG_MXC_RDC
#include <asm/imx-common/rdc-sema.h>
#include <asm/arch/imx-rdc.h>
#endif

enum mxc_gpio_direction {
	MXC_GPIO_DIRECTION_IN,
	MXC_GPIO_DIRECTION_OUT,
};

#define GPIO_TO_PORT(n)		(n / 32)

/* GPIO port description */
static unsigned long gpio_ports[] = {
	[0] = GPIO1_BASE_ADDR,
	[1] = GPIO2_BASE_ADDR,
	[2] = GPIO3_BASE_ADDR,
#if defined(CONFIG_MX25) || defined(CONFIG_MX27) || defined(CONFIG_MX51) || \
		defined(CONFIG_MX53) || defined(CONFIG_MX6)
	[3] = GPIO4_BASE_ADDR,
#endif
#if defined(CONFIG_MX27) || defined(CONFIG_MX53) || defined(CONFIG_MX6)
	[4] = GPIO5_BASE_ADDR,
	[5] = GPIO6_BASE_ADDR,
#endif
#if defined(CONFIG_MX53) || defined(CONFIG_MX6)
	[6] = GPIO7_BASE_ADDR,
#endif
};

#ifdef CONFIG_MXC_RDC
static unsigned int gpio_rdc[] = {
	RDC_PER_GPIO1,
	RDC_PER_GPIO2,
	RDC_PER_GPIO3,
	RDC_PER_GPIO4,
	RDC_PER_GPIO5,
	RDC_PER_GPIO6,
	RDC_PER_GPIO7,
};

#define RDC_CHECK(x) imx_rdc_check_permission(gpio_rdc[x])
#define RDC_SPINLOCK_UP(x) imx_rdc_sema_lock(gpio_rdc[x])
#define RDC_SPINLOCK_DOWN(x) imx_rdc_sema_unlock(gpio_rdc[x])
#else
#define RDC_CHECK(x) 0
#define RDC_SPINLOCK_UP(x)
#define RDC_SPINLOCK_DOWN(x)
#endif


static int mxc_gpio_direction(unsigned int gpio,
	enum mxc_gpio_direction direction)
{
	unsigned int port = GPIO_TO_PORT(gpio);
	struct gpio_regs *regs;
	u32 l;

	if (port >= ARRAY_SIZE(gpio_ports))
		return -1;

	if (RDC_CHECK(port))
		return -1;

	RDC_SPINLOCK_UP(port);

	gpio &= 0x1f;

	regs = (struct gpio_regs *)gpio_ports[port];

	l = readl(&regs->gpio_dir);

	switch (direction) {
	case MXC_GPIO_DIRECTION_OUT:
		l |= 1 << gpio;
		break;
	case MXC_GPIO_DIRECTION_IN:
		l &= ~(1 << gpio);
	}
	writel(l, &regs->gpio_dir);

	RDC_SPINLOCK_DOWN(port);

	return 0;
}

int gpio_set_value(unsigned gpio, int value)
{
	unsigned int port = GPIO_TO_PORT(gpio);
	struct gpio_regs *regs;
	u32 l;

	if (port >= ARRAY_SIZE(gpio_ports))
		return -1;

	if (RDC_CHECK(port))
		return -1;

	RDC_SPINLOCK_UP(port);

	gpio &= 0x1f;

	regs = (struct gpio_regs *)gpio_ports[port];

	l = readl(&regs->gpio_dr);
	if (value)
		l |= 1 << gpio;
	else
		l &= ~(1 << gpio);
	writel(l, &regs->gpio_dr);

	RDC_SPINLOCK_DOWN(port);

	return 0;
}

int gpio_get_value(unsigned gpio)
{
	unsigned int port = GPIO_TO_PORT(gpio);
	struct gpio_regs *regs;
	u32 val;

	if (port >= ARRAY_SIZE(gpio_ports))
		return -1;

	if (RDC_CHECK(port))
		return -1;

	RDC_SPINLOCK_UP(port);

	gpio &= 0x1f;

	regs = (struct gpio_regs *)gpio_ports[port];

	val = (readl(&regs->gpio_psr) >> gpio) & 0x01;

	RDC_SPINLOCK_DOWN(port);

	return val;
}

int gpio_request(unsigned gpio, const char *label)
{
	unsigned int port = GPIO_TO_PORT(gpio);
	if (port >= ARRAY_SIZE(gpio_ports))
		return -1;

	if (RDC_CHECK(port))
		return -1;

	return 0;
}

int gpio_free(unsigned gpio)
{
	return 0;
}

int gpio_direction_input(unsigned gpio)
{
	return mxc_gpio_direction(gpio, MXC_GPIO_DIRECTION_IN);
}

int gpio_direction_output(unsigned gpio, int value)
{
	int ret = gpio_set_value(gpio, value);

	if (ret < 0)
		return ret;

	return mxc_gpio_direction(gpio, MXC_GPIO_DIRECTION_OUT);
}
