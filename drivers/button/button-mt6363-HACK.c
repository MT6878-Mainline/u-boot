// SPDX-License-Identifier: GPL-2.0
/*
 * Pain and suffering ahead
 *
 * aka... why don't i just write a fucking spmi driver...
 * Too lazy!
 */

#include <button.h>
#include <dm.h>
#include <dm/lists.h>
#include <dm/uclass-internal.h>
#include <log.h>
#include <asm/io.h>

#include <linux/input.h>

#define PRESSED_VOLUP 0x16
#define PRESSED_POWER 0x1c

struct button_mt6363_priv {
	int linux_code;
};

/* This... is awful. */
static enum button_state_t button_mt6363_get_state(struct udevice *dev)
{
	struct button_mt6363_priv *priv = dev_get_priv(dev);
	int ret;

	if (!priv)
		return -ENODATA;

	// SPMI send read cmd
	writel(0xC400001e, 0x1cc04800);

	// Read data
	ret = readl(0x1cc04814);

	// Set clear PMIC buffer bit
	writel(0x1, 0x1cc04824);

	if (priv->linux_code == KEY_POWER && ret == PRESSED_POWER)
		return BUTTON_ON;
	else if (priv->linux_code == KEY_VOLUMEUP && ret == PRESSED_VOLUP)
		return BUTTON_ON;

	return BUTTON_OFF;
}

static int button_mt6363_get_code(struct udevice *dev)
{
	struct button_mt6363_priv *priv = dev_get_priv(dev);
	if (!priv)
		return -ENODATA;
	int code = priv->linux_code;

	if (!code)
		return -ENODATA;

	return code;
}

static int button_mt6363_probe(struct udevice *dev)
{
	struct button_uc_plat *uc_plat = dev_get_uclass_plat(dev);
	struct button_mt6363_priv *priv = dev_get_priv(dev);
	int ret;

	/* Ignore the top-level button node */
	if (!uc_plat->label)
		return 0;

	ret = dev_read_u32(dev, "linux,code", &priv->linux_code);

	return ret;
}

static int button_mt6363_remove(struct udevice *dev)
{
	return 0;
}

static int button_mt6363_bind(struct udevice *parent)
{
	struct udevice *dev;
	ofnode node;
	int ret;

	dev_for_each_subnode(node, parent) {
		struct button_uc_plat *uc_plat;
		const char *label;

		label = ofnode_read_string(node, "label");
		if (!label) {
			debug("%s: node %s has no label\n", __func__,
				  ofnode_get_name(node));
			return -EINVAL;
		}
		ret = device_bind_driver_to_node(parent, "button_mt6363",
										 ofnode_get_name(node),
										 node, &dev);
		if (ret)
			return ret;
		uc_plat = dev_get_uclass_plat(dev);
		uc_plat->label = label;
		debug("Button '%s' bound to driver '%s'\n", label,
			  dev->driver->name);
	}

	return 0;
}

static const struct button_ops button_mt6363_ops = {
	.get_state	= button_mt6363_get_state,
	.get_code	= button_mt6363_get_code,
};

static const struct udevice_id button_mt6363_ids[] = {
	{ .compatible = "mediatek,mt6363-buttons-raw" },
	{ }
};

U_BOOT_DRIVER(button_mt6363) = {
	.name		= "button_mt6363",
	.id		= UCLASS_BUTTON,
	.of_match	= button_mt6363_ids,
	.ops		= &button_mt6363_ops,
	.priv_auto	= sizeof(struct button_mt6363_priv),
	.bind		= button_mt6363_bind,
	.probe		= button_mt6363_probe,
	.remove		= button_mt6363_remove,
};
