// SPDX-License-Identifier: GPL-2.0+
/*
 * TI SYSCON regmap reset driver
 *
 * Copyright (C) 2025 Igor Belwon <igor.belwon@mentallysanemainliners.org>
 *
 * Loosely based on Linux kernel reset-ti-syscon.c
 */

#include "syscon.h"
#include <dm.h>
#include <dm/devres.h>
#include <errno.h>
#include <malloc.h>
#include <regmap.h>
#include <log.h>
#include <malloc.h>
#include <reset-uclass.h>
#include <dm/device_compat.h>

#include <dt-bindings/reset/ti-syscon.h>

/**
 * struct ti_syscon_reset_control - reset control structure
 * @assert_offset: reset assert control register offset from syscon base
 * @assert_bit: reset assert bit in the reset assert control register
 * @deassert_offset: reset deassert control register offset from syscon base
 * @deassert_bit: reset deassert bit in the reset deassert control register
 * @status_offset: reset status register offset from syscon base
 * @status_bit: reset status bit in the reset status register
 * @flags: reset flag indicating how the (de)assert and status are handled
 */
struct ti_syscon_reset_control {
	unsigned int assert_offset;
	unsigned int assert_bit;
	unsigned int deassert_offset;
	unsigned int deassert_bit;
	unsigned int status_offset;
	unsigned int status_bit;
	u32 flags;
};

/**
 * struct ti_syscon_reset_data - reset controller information structure
 * @rcdev: reset controller entity
 * @regmap: regmap handle containing the memory-mapped reset registers
 * @controls: array of reset controls
 * @nr_controls: number of controls in control array
 */
struct ti_syscon_reset_data {
	struct regmap *regmap;
	struct ti_syscon_reset_control *controls;
	unsigned int nr_controls;
};

/**
 * ti_syscon_reset_assert() - assert device reset
 * @rst: Handle to a single reset signal
 *
 * This function implements the reset driver op to assert a device's reset
 * using the TI SCI protocol. This invokes the function ti_syscon_reset_set()
 * with the corresponding parameters as passed in, but with the @assert
 * argument set to true for asserting the reset.
 *
 * Return: 0 for successful request, else a corresponding error value
 */
static int ti_syscon_reset_assert(struct reset_ctl *rst)
{
	struct ti_syscon_reset_data *data = dev_get_priv(rst->dev);
	struct ti_syscon_reset_control *control;
	unsigned int mask, value;

	if (rst->id >= data->nr_controls)
		return -EINVAL;

	control = &data->controls[rst->id];

	if (control->flags & ASSERT_NONE)
		return -ENOTSUPP; /* assert not supported for this reset */

	mask = BIT(control->assert_bit);
	value = (control->flags & ASSERT_SET) ? mask : 0x0;

	return regmap_update_bits(data->regmap, control->assert_offset, mask, value);
}

/**
 * ti_syscon_reset_deassert() - deassert device reset
 * @rcdev: reset controller entity
 * @id: ID of reset to be deasserted
 *
 * This function implements the reset driver op to deassert a device's reset.
 * This deasserts the reset in a manner prescribed by the reset flags.
 *
 * Return: 0 for successful request, else a corresponding error value
 */
static int ti_syscon_reset_deassert(struct reset_ctl *rst)
{
	struct ti_syscon_reset_data *data = dev_get_priv(rst->dev);
	struct ti_syscon_reset_control *control;
	unsigned int mask, value;

	if (rst->id >= data->nr_controls)
		return -EINVAL;

	control = &data->controls[rst->id];

	if (control->flags & DEASSERT_NONE)
		return -ENOTSUPP; /* deassert not supported for this reset */

	mask = BIT(control->deassert_bit);
	value = (control->flags & DEASSERT_SET) ? mask : 0x0;

	return regmap_update_bits(data->regmap, control->deassert_offset, mask, value);
}

/**
 * ti_syscon_reset_status() - check device reset status
 * @rcdev: reset controller entity
 * @id: ID of the reset for which the status is being requested
 *
 * This function implements the reset driver op to return the status of a
 * device's reset.
 *
 * Return: 0 if reset is deasserted, true if reset is asserted, else a
 * corresponding error value
 */
static int ti_syscon_reset_status(struct reset_ctl *rst)
{
	struct ti_syscon_reset_data *data = dev_get_priv(rst->dev);
	struct ti_syscon_reset_control *control;
	unsigned int reset_state;
	int ret;

	if (rst->id >= data->nr_controls)
		return -EINVAL;

	control = &data->controls[rst->id];

	if (control->flags & STATUS_NONE)
		return -ENOTSUPP; /* status not supported for this reset */

	ret = regmap_read(data->regmap, control->status_offset, &reset_state);
	if (ret)
		return ret;

	return !(reset_state & BIT(control->status_bit)) ==
			!(control->flags & STATUS_SET);
}

static int ti_syscon_reset_probe(struct udevice *dev)
{
	struct ti_syscon_reset_data *data = dev_get_priv(dev);
	ofnode node = dev_ofnode(dev);
	ofnode parent;
	struct regmap *regmap;
	const __be32 *list;
	struct ti_syscon_reset_control *controls = NULL;
	int size, nr_controls, i;

	debug("%s(dev=%p)\n", __func__, dev);

	if (!data)
		return -ENOMEM;

	/* Find the parent syscon node explicitly (reset controller is child) */
	parent = ofnode_get_parent(node);
	if (!ofnode_valid(parent)) {
		dev_err(dev, "no parent syscon node\n");
		return -EINVAL;
	}

	/* Get regmap for the parent syscon */
	regmap = syscon_node_to_regmap(parent);
	if (IS_ERR(regmap)) {
		dev_err(dev, "failed to get syscon regmap\n");
		return PTR_ERR(regmap);
	}

	/* Read reset description */
	list = ofnode_get_property(node, "ti,reset-bits", &size);
	if (!list || (size / sizeof(*list)) % 7 != 0) {
		dev_err(dev, "invalid DT reset description\n");
		return -EINVAL;
	}

	nr_controls = (size / sizeof(*list)) / 7;

	controls = calloc(nr_controls, sizeof(*controls));
	if (!controls)
		return -ENOMEM;

	for (i = 0; i < nr_controls; i++) {
		controls[i].assert_offset = be32_to_cpup(list++);
		controls[i].assert_bit = be32_to_cpup(list++);
		controls[i].deassert_offset = be32_to_cpup(list++);
		controls[i].deassert_bit = be32_to_cpup(list++);
		controls[i].status_offset = be32_to_cpup(list++);
		controls[i].status_bit = be32_to_cpup(list++);
		controls[i].flags = be32_to_cpup(list++);
	}

	data->regmap = regmap;
	data->controls = controls;
	data->nr_controls = nr_controls;

	printf("ti-syscon reset: probed %d controls\n", nr_controls);

	return 0;
}


static const struct udevice_id ti_syscon_reset_of_match[] = {
	{ .compatible = "ti,syscon-reset", },
	{ /* sentinel */ },
};

static struct reset_ops ti_syscon_reset_ops = {
	.rst_assert = ti_syscon_reset_assert,
	.rst_deassert = ti_syscon_reset_deassert,
	.rst_status = ti_syscon_reset_status,
};

U_BOOT_DRIVER(ti_syscon_reset) = {
	.name = "ti-syscon-reset",
	.id = UCLASS_RESET,
	.of_match = ti_syscon_reset_of_match,
	.probe = ti_syscon_reset_probe,
	.priv_auto	= sizeof(struct ti_syscon_reset_data),
	.ops = &ti_syscon_reset_ops,
};
