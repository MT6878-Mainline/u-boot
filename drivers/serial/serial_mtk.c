// SPDX-License-Identifier: GPL-2.0
/*
 * MediaTek High-speed UART driver
 *
 * Copyright (C) 2018 MediaTek Inc.
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <clk.h>
#include <config.h>
#include <div64.h>
#include <dm.h>
#include <dm/device.h>
#include <dm/device_compat.h>
#include <errno.h>
#include <log.h>
#include <serial.h>
#include <watchdog.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <asm/types.h>
#include <linux/err.h>
#include <linux/printk.h>
#include <linux/delay.h>

struct mtk_serial_regs {
	u32 rbr;
	u32 ier;
	u32 fcr;
	u32 lcr;
	u32 mcr;
	u32 lsr;
	u32 msr;
	u32 spr;
	u32 mdr1;
	u32 highspeed;
	u32 sample_count;
	u32 sample_point;
	u32 fracdiv_l;
	u32 fracdiv_m;
	u32 escape_en;
	u32 guard;
	u32 rx_sel;
};

static inline void _debug_uart_putc(int ch);

#define thr rbr
#define iir fcr
#define dll rbr
#define dlm ier

#define UART_LCR_WLS_8	0x03		/* 8 bit character length */
#define UART_LCR_DLAB	0x80		/* Divisor latch access bit */

#define UART_LSR_DR	0x01		/* Data ready */
#define UART_LSR_THRE	0x20		/* Xmit holding register empty */
#define UART_LSR_TEMT	0x40		/* Xmitter empty */

#define UART_MCR_DTR	0x01		/* DTR   */
#define UART_MCR_RTS	0x02		/* RTS   */

#define UART_FCR_FIFO_EN	0x01	/* Fifo enable */
#define UART_FCR_RXSR		0x02	/* Receiver soft reset */
#define UART_FCR_TXSR		0x04	/* Transmitter soft reset */

#define UART_MCRVAL (UART_MCR_DTR | \
		     UART_MCR_RTS)

/* Clear & enable FIFOs */
#define UART_FCRVAL (UART_FCR_FIFO_EN | \
		     UART_FCR_RXSR |	\
		     UART_FCR_TXSR)

/* the data is correct if the real baud is within 3%. */
#define BAUD_ALLOW_MAX(baud)	((baud) + (baud) * 3 / 100)
#define BAUD_ALLOW_MIX(baud)	((baud) - (baud) * 3 / 100)

/* struct mtk_serial_priv -	Structure holding all information used by the
 *				driver
 * @regs:			Register base of the serial port
 * @clk:			The baud clock device
 * @clk_bus:			The bus clock device
 * @fixed_clk_rate:		Fallback fixed baud clock rate if baud clock
 *				device is not specified
 * @force_highspeed:		Force using high-speed mode
 * @upstream_highspeed_logic:	Apply upstream high-speed logic
 */
struct mtk_serial_priv {
	struct mtk_serial_regs __iomem *regs;
	struct clk clk;
	struct clk clk_bus;
	u32 fixed_clk_rate;
	bool force_highspeed;
	bool upstream_highspeed_logic;
};

static void _mtk_serial_setbrg(struct mtk_serial_priv *priv, int baud,
			       uint clk_rate)
{}


static int _mtk_serial_putc(struct mtk_serial_priv *priv, const char ch)
{
	_debug_uart_putc(ch);

	return 0;
}

static int _mtk_serial_getc(struct mtk_serial_priv *priv)
{
	return 0;
}

static int _mtk_serial_pending(struct mtk_serial_priv *priv, bool input)
{
	return 0;
}

#if CONFIG_IS_ENABLED(DM_SERIAL)
static int mtk_serial_setbrg(struct udevice *dev, int baudrate)
{
	struct mtk_serial_priv *priv = dev_get_priv(dev);
	u32 clk_rate;

	clk_rate = clk_get_rate(&priv->clk);
	if (IS_ERR_VALUE(clk_rate) || clk_rate == 0)
		clk_rate = priv->fixed_clk_rate;

	_mtk_serial_setbrg(priv, baudrate, clk_rate);

	return 0;
}

static int mtk_serial_putc(struct udevice *dev, const char ch)
{
	struct mtk_serial_priv *priv = dev_get_priv(dev);

	return _mtk_serial_putc(priv, ch);
}

static int mtk_serial_getc(struct udevice *dev)
{
	struct mtk_serial_priv *priv = dev_get_priv(dev);

	return _mtk_serial_getc(priv);
}

static int mtk_serial_pending(struct udevice *dev, bool input)
{
	struct mtk_serial_priv *priv = dev_get_priv(dev);

	return _mtk_serial_pending(priv, input);
}

static int mtk_serial_probe(struct udevice *dev)
{
	struct mtk_serial_priv *priv = dev_get_priv(dev);

	/* Disable interrupt */
	writel(0, &priv->regs->ier);

	writel(UART_MCRVAL, &priv->regs->mcr);
	writel(UART_FCRVAL, &priv->regs->fcr);

	clk_enable(&priv->clk);
	if (priv->clk_bus.dev)
		clk_enable(&priv->clk_bus);

	return 0;
}

static int mtk_serial_of_to_plat(struct udevice *dev)
{
	return 0;
}

static const struct dm_serial_ops mtk_serial_ops = {
	.putc = mtk_serial_putc,
	.pending = mtk_serial_pending,
	.getc = mtk_serial_getc,
	.setbrg = mtk_serial_setbrg,
};

static const struct udevice_id mtk_serial_ids[] = {
	{ .compatible = "mediatek,hsuart" },
	{ .compatible = "mediatek,mt6577-uart" },
	{ }
};

U_BOOT_DRIVER(serial_mtk) = {
	.name = "serial_mtk",
	.id = UCLASS_SERIAL,
	.of_match = mtk_serial_ids,
	.of_to_plat = mtk_serial_of_to_plat,
	.priv_auto	= sizeof(struct mtk_serial_priv),
	.probe = mtk_serial_probe,
	.ops = &mtk_serial_ops,
	.flags = DM_FLAG_PRE_RELOC,
};
#else

DECLARE_GLOBAL_DATA_PTR;

#define DECLARE_HSUART_PRIV(port) \
	static struct mtk_serial_priv mtk_hsuart##port = { \
	.regs = (struct mtk_serial_regs *)CFG_SYS_NS16550_COM##port, \
	.fixed_clk_rate = CFG_SYS_NS16550_CLK \
};

#define DECLARE_HSUART_FUNCTIONS(port) \
	static int mtk_serial##port##_init(void) \
	{ \
		writel(0, &mtk_hsuart##port.regs->ier); \
		writel(UART_MCRVAL, &mtk_hsuart##port.regs->mcr); \
		writel(UART_FCRVAL, &mtk_hsuart##port.regs->fcr); \
		_mtk_serial_setbrg(&mtk_hsuart##port, gd->baudrate, \
				   mtk_hsuart##port.fixed_clk_rate); \
		return 0 ; \
	} \
	static void mtk_serial##port##_setbrg(void) \
	{ \
		_mtk_serial_setbrg(&mtk_hsuart##port, gd->baudrate, \
				   mtk_hsuart##port.fixed_clk_rate); \
	} \
	static int mtk_serial##port##_getc(void) \
	{ \
		int err; \
		do { \
			err = _mtk_serial_getc(&mtk_hsuart##port); \
			if (err == -EAGAIN) \
				schedule(); \
		} while (err == -EAGAIN); \
		return err >= 0 ? err : 0; \
	} \
	static int mtk_serial##port##_tstc(void) \
	{ \
		return _mtk_serial_pending(&mtk_hsuart##port, true); \
	} \
	static void mtk_serial##port##_putc(const char c) \
	{ \
		int err; \
		if (c == '\n') \
			mtk_serial##port##_putc('\r'); \
		do { \
			err = _mtk_serial_putc(&mtk_hsuart##port, c); \
		} while (err == -EAGAIN); \
	} \
	static void mtk_serial##port##_puts(const char *s) \
	{ \
		while (*s) { \
			mtk_serial##port##_putc(*s++); \
		} \
	}

/* Serial device descriptor */
#define INIT_HSUART_STRUCTURE(port, __name) {	\
	.name	= __name,			\
	.start	= mtk_serial##port##_init,	\
	.stop	= NULL,				\
	.setbrg	= mtk_serial##port##_setbrg,	\
	.getc	= mtk_serial##port##_getc,	\
	.tstc	= mtk_serial##port##_tstc,	\
	.putc	= mtk_serial##port##_putc,	\
	.puts	= mtk_serial##port##_puts,	\
}

#define DECLARE_HSUART(port, __name) \
	DECLARE_HSUART_PRIV(port); \
	DECLARE_HSUART_FUNCTIONS(port); \
	struct serial_device mtk_hsuart##port##_device = \
		INIT_HSUART_STRUCTURE(port, __name);

#if !defined(CONFIG_CONS_INDEX)
#elif (CONFIG_CONS_INDEX < 1) || (CONFIG_CONS_INDEX > 6)
#error	"Invalid console index value."
#endif

#if CONFIG_CONS_INDEX == 1 && !defined(CFG_SYS_NS16550_COM1)
#error	"Console port 1 defined but not configured."
#elif CONFIG_CONS_INDEX == 2 && !defined(CFG_SYS_NS16550_COM2)
#error	"Console port 2 defined but not configured."
#elif CONFIG_CONS_INDEX == 3 && !defined(CFG_SYS_NS16550_COM3)
#error	"Console port 3 defined but not configured."
#elif CONFIG_CONS_INDEX == 4 && !defined(CFG_SYS_NS16550_COM4)
#error	"Console port 4 defined but not configured."
#elif CONFIG_CONS_INDEX == 5 && !defined(CFG_SYS_NS16550_COM5)
#error	"Console port 5 defined but not configured."
#elif CONFIG_CONS_INDEX == 6 && !defined(CFG_SYS_NS16550_COM6)
#error	"Console port 6 defined but not configured."
#endif

#if defined(CFG_SYS_NS16550_COM1)
DECLARE_HSUART(1, "mtk-hsuart0");
#endif
#if defined(CFG_SYS_NS16550_COM2)
DECLARE_HSUART(2, "mtk-hsuart1");
#endif
#if defined(CFG_SYS_NS16550_COM3)
DECLARE_HSUART(3, "mtk-hsuart2");
#endif
#if defined(CFG_SYS_NS16550_COM4)
DECLARE_HSUART(4, "mtk-hsuart3");
#endif
#if defined(CFG_SYS_NS16550_COM5)
DECLARE_HSUART(5, "mtk-hsuart4");
#endif
#if defined(CFG_SYS_NS16550_COM6)
DECLARE_HSUART(6, "mtk-hsuart5");
#endif

__weak struct serial_device *default_serial_console(void)
{
#if CONFIG_CONS_INDEX == 1
	return &mtk_hsuart1_device;
#elif CONFIG_CONS_INDEX == 2
	return &mtk_hsuart2_device;
#elif CONFIG_CONS_INDEX == 3
	return &mtk_hsuart3_device;
#elif CONFIG_CONS_INDEX == 4
	return &mtk_hsuart4_device;
#elif CONFIG_CONS_INDEX == 5
	return &mtk_hsuart5_device;
#elif CONFIG_CONS_INDEX == 6
	return &mtk_hsuart6_device;
#else
#error "Bad CONFIG_CONS_INDEX."
#endif
}

void mtk_serial_initialize(void)
{
#if defined(CFG_SYS_NS16550_COM1)
	serial_register(&mtk_hsuart1_device);
#endif
#if defined(CFG_SYS_NS16550_COM2)
	serial_register(&mtk_hsuart2_device);
#endif
#if defined(CFG_SYS_NS16550_COM3)
	serial_register(&mtk_hsuart3_device);
#endif
#if defined(CFG_SYS_NS16550_COM4)
	serial_register(&mtk_hsuart4_device);
#endif
#if defined(CFG_SYS_NS16550_COM5)
	serial_register(&mtk_hsuart5_device);
#endif
#if defined(CFG_SYS_NS16550_COM6)
	serial_register(&mtk_hsuart6_device);
#endif
}

#endif


#include <debug_uart.h>

static inline void _debug_uart_init(void)
{
	//struct mtk_serial_priv priv;

	//memset(&priv, 0, sizeof(struct mtk_serial_priv));
	//priv.regs = (void *) CONFIG_VAL(DEBUG_UART_BASE);
	//priv.fixed_clk_rate = CONFIG_DEBUG_UART_CLOCK;

	//writel(0, &priv.regs->ier);
	//writel(UART_MCRVAL, &priv.regs->mcr);
	//writel(UART_FCRVAL, &priv.regs->fcr);

	//_mtk_serial_setbrg(&priv, CONFIG_BAUDRATE, priv.fixed_clk_rate);
}

static inline void _debug_uart_putc(int ch)
{
	struct mtk_serial_regs __iomem *regs =
		(void *) 0x11001000;

	mdelay(2);

	writel(ch, &regs->thr);
}

DEBUG_UART_FUNCS

