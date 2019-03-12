/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <debug_uart.h>
#include <dm.h>
#include <errno.h>
#include <fdtdec.h>
#include <linux/compiler.h>
#include <serial.h>

#include <xen/events.h>
#include <xen/hvm.h>
#include <xen/interface/sched.h>
#include <xen/interface/hvm/hvm_op.h>
#include <xen/interface/hvm/params.h>
#include <xen/interface/io/console.h>
#include <xen/interface/io/ring.h>

DECLARE_GLOBAL_DATA_PTR;

#if CONFIG_IS_ENABLED(DM_SERIAL)
struct xen_uart_priv {
	struct xencons_interface *intf;
	u32 evtchn;
	int vtermno;
	struct hvc_struct *hvc;
	grant_ref_t gntref;
};

int xen_serial_setbrg(struct udevice *dev, int baudrate)
{
	return 0;
}

static int xen_serial_probe(struct udevice *dev)
{
	struct xen_uart_priv *priv = dev_get_priv(dev);
	u64 v = 0;
	unsigned long gfn;
	int r;

	r = hvm_get_parameter(HVM_PARAM_CONSOLE_EVTCHN, &v);
	if (r < 0 || v == 0)
		return r;

	priv->evtchn = v;

	r = hvm_get_parameter(HVM_PARAM_CONSOLE_PFN, &v);
	if (r < 0 || v == 0)
		return -ENODEV;

	gfn = v;

	priv->intf = (struct xencons_interface *)(gfn << XEN_PAGE_SHIFT);
	if (!priv->intf)
		return -EINVAL;

	return 0;
}

static int xen_serial_pending(struct udevice *dev, bool input)
{
	struct xen_uart_priv *priv = dev_get_priv(dev);
	struct xencons_interface *intf = priv->intf;

	if (!input || intf->in_cons == intf->in_prod)
		return 0;

	return 1;
}

static int xen_serial_getc(struct udevice *dev)
{
	struct xen_uart_priv *priv = dev_get_priv(dev);
	struct xencons_interface *intf = priv->intf;
	XENCONS_RING_IDX cons;
	char c;

	while (intf->in_cons == intf->in_prod) {
		mb(); /* wait */
	}

	cons = intf->in_cons;
	mb();			/* get pointers before reading ring */

	c = intf->in[MASK_XENCONS_IDX(cons++, intf->in)];

	mb();			/* read ring before consuming */
	intf->in_cons = cons;

	notify_remote_via_evtchn(priv->evtchn);

	return c;
}

static int __write_console(struct udevice *dev, const char *data, int len)
{
	struct xen_uart_priv *priv = dev_get_priv(dev);
	struct xencons_interface *intf = priv->intf;
	XENCONS_RING_IDX cons, prod;
	int sent = 0;

	cons = intf->out_cons;
	prod = intf->out_prod;
	mb(); /* Update pointer */

	WARN_ON((prod - cons) > sizeof(intf->out));

	while ((sent < len) && ((prod - cons) < sizeof(intf->out)))
		intf->out[MASK_XENCONS_IDX(prod++, intf->out)] = data[sent++];

	mb(); /* Update data before pointer */
	intf->out_prod = prod;

	if (sent)
		notify_remote_via_evtchn(priv->evtchn);

	if (data[sent - 1] == '\n')
		serial_puts("\r");

	return sent;
}

static int write_console(struct udevice *dev, const char *data, int len)
{
	/*
	 * Make sure the whole buffer is emitted, polling if
	 * necessary.  We don't ever want to rely on the hvc daemon
	 * because the most interesting console output is when the
	 * kernel is crippled.
	 */
	while (len) {
		int sent = __write_console(dev, data, len);

		data += sent;
		len -= sent;

		if (unlikely(len))
			HYPERVISOR_sched_op(SCHEDOP_yield, NULL);
	}

	return 0;
}

static int xen_serial_puts(struct udevice *dev, const char *str)
{
#ifdef CONFIG_SPL_BUILD
	(void)HYPERVISOR_console_io(CONSOLEIO_write, strlen(str), (char *)str);
#else
	write_console(dev, str, strlen(str));
#endif

	return 0;
}

static int xen_serial_putc(struct udevice *dev, const char ch)
{
#ifdef CONFIG_SPL_BUILD
	(void)HYPERVISOR_console_io(CONSOLEIO_write, 1, (char *)&ch);
#else
	write_console(dev, &ch, 1);
#endif

	return 0;
}

static const struct dm_serial_ops xen_serial_ops = {
	.puts = xen_serial_puts,
	.putc = xen_serial_putc,
	.getc = xen_serial_getc,
	.pending = xen_serial_pending,
};

static const struct udevice_id xen_serial_ids[] = {
	{ .compatible = "xen,xen" },
	{ }
};

U_BOOT_DRIVER(serial_xen) = {
	.name	= "serial_xen",
	.id	= UCLASS_SERIAL,
	.of_match = xen_serial_ids,
	.priv_auto_alloc_size = sizeof(struct xen_uart_priv),
	.probe = xen_serial_probe,
	.ops	= &xen_serial_ops,
	.flags = DM_FLAG_PRE_RELOC | DM_FLAG_IGNORE_DEFAULT_CLKS,
};
#else
static void xen_serial_putc(const char c)
{
	(void)HYPERVISOR_console_io(CONSOLEIO_write, 1, (char *)&c);
}

static void xen_serial_puts(const char *str)
{
	(void)HYPERVISOR_console_io(CONSOLEIO_write, strlen(str), (char *)str);
}

static int xen_serial_tstc(void)
{
	return 0;
}

static int xen_serial_init(void)
{
	return 0;
}

static void xen_serial_setbrg(void)
{
}

static struct serial_device xen_serial_drv = {
	.name	= "xen_serial",
	.start	= xen_serial_init,
	.stop	= NULL,
	.setbrg	= xen_serial_setbrg,
	.getc	= NULL,
	.putc	= xen_serial_putc,
	.puts	= xen_serial_puts,
	.tstc	= xen_serial_tstc,
};

__weak struct serial_device *default_serial_console(void)
{
	return &xen_serial_drv;
}

#endif

#ifdef CONFIG_DEBUG_UART_XEN
void _debug_uart_init(void)
{
}

void _debug_uart_putc(int ch)
{
	/* If \n, also do \r */
	if (ch == '\n')
		serial_putc('\r');

	(void)HYPERVISOR_console_io(CONSOLEIO_write, 1, (char *)&ch);

	return;
}

DEBUG_UART_FUNCS

#endif
