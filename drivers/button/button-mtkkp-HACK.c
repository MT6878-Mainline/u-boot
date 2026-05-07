// SPDX-License-Identifier: GPL-2.0
/*
 * Pain and suffering ahead
 *
 * aka... why don't i just write a good driver and not just go fast
 */

#include <button.h>
#include <dm.h>
#include <dm/lists.h>
#include <dm/uclass-internal.h>
#include <log.h>
#include <asm/io.h>

#include <linux/input.h>

// Worse than MediaTek?
#define BASE 0x1c00e000

struct button_mtkkp_priv {
	int linux_code;
};

/* This... is a bit less awful, but still needs a rewrite for multiple btns. */
static enum button_state_t button_mtkkp_get_state(struct udevice *dev)
{
	struct button_mtkkp_priv *priv = dev_get_priv(dev);
	int ret;

	if (!priv)
		return -ENODATA;

	// Read data
	ret = readl(BASE);

	if (ret)
		return BUTTON_ON;

	return BUTTON_OFF;
}

static int button_mtkkp_get_code(struct udevice *dev)
{
	struct button_mtkkp_priv *priv = dev_get_priv(dev);
	if (!priv)
		return -ENODATA;
	int code = priv->linux_code;

	if (!code)
		return -ENODATA;

	return code;
}

static int button_mtkkp_probe(struct udevice *dev)
{
	struct button_uc_plat *uc_plat = dev_get_uclass_plat(dev);
	struct button_mtkkp_priv *priv = dev_get_priv(dev);
	int ret;

	/* Ignore the top-level button node */
	if (!uc_plat->label)
		return 0;

	ret = dev_read_u32(dev, "linux,code", &priv->linux_code);

	return ret;
}

static int button_mtkkp_remove(struct udevice *dev)
{
	return 0;
}

static int button_mtkkp_bind(struct udevice *parent)
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
		ret = device_bind_driver_to_node(parent, "button_mtkkp",
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

static const struct button_ops button_mtkkp_ops = {
	.get_state	= button_mtkkp_get_state,
	.get_code	= button_mtkkp_get_code,
};

static const struct udevice_id button_mtkkp_ids[] = {
	{ .compatible = "mediatek,kp-raw" },
	{ }
};

U_BOOT_DRIVER(button_mtkkp) = {
	.name		= "button_mtkkp",
	.id		= UCLASS_BUTTON,
	.of_match	= button_mtkkp_ids,
	.ops		= &button_mtkkp_ops,
	.priv_auto	= sizeof(struct button_mtkkp_priv),
	.bind		= button_mtkkp_bind,
	.probe		= button_mtkkp_probe,
	.remove		= button_mtkkp_remove,
};
